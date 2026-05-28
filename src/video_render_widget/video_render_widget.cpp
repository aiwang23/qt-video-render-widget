#include "video_render_widget.h"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QDebug>

#include <array>
#include <cstring>
#include <vector>

#define GLVERSION "#version 330 core\n"
#define GET_SHADER(arg) GLVERSION #arg

static const char *vertex_shader = GET_SHADER(
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    uniform vec2 u_scale;

    void main() {
    gl_Position = vec4(aPos.x * u_scale.x, aPos.y * u_scale.y, aPos.z, 1.0);
    TexCoord = aTexCoord;
    }
);

static const char *fragment_shader = GET_SHADER(
    in vec2 TexCoord;
    out vec4 FragColor;

    uniform sampler2D tex0;
    uniform sampler2D tex1;
    uniform sampler2D tex2;

    // 1 = YUV planar
    // 2 = NV12
    // 3 = RGBA
    // 4 = NV21
    // 5 = RGB24
    uniform int u_format;

    uniform float u_y_offset;
    uniform float u_y_scale;

    uniform vec4 u_yuv_coeff;

    vec3 yuv_to_rgb(float y, float u, float v) {
    y = (y - u_y_offset) * u_y_scale;
    u = u - 0.5;
    v = v - 0.5;

    float r = y + u_yuv_coeff.x * v;
    float g = y + u_yuv_coeff.y * u + u_yuv_coeff.z * v;
    float b = y + u_yuv_coeff.w * u;

    return vec3(r, g, b);
    }

    void main() {
    if (u_format == 3) {
    FragColor = texture(tex0, TexCoord);
    return;
    }

    if (u_format == 5) {
    FragColor = vec4(texture(tex0, TexCoord).rgb, 1.0);
    return;
    }

    if (u_format == 2) {
    float y = texture(tex0, TexCoord).r;
    vec2 uv = texture(tex1, TexCoord).rg;
    vec3 rgb = yuv_to_rgb(y, uv.x, uv.y);
    FragColor = vec4(rgb, 1.0);
    return;
    }

    if (u_format == 4) {
    float y = texture(tex0, TexCoord).r;
    vec2 vu = texture(tex1, TexCoord).rg;
    vec3 rgb = yuv_to_rgb(y, vu.y, vu.x);
    FragColor = vec4(rgb, 1.0);
    return;
    }

    float y = texture(tex0, TexCoord).r;
    float u = texture(tex1, TexCoord).r;
    float v = texture(tex2, TexCoord).r;
    vec3 rgb = yuv_to_rgb(y, u, v);
    FragColor = vec4(rgb, 1.0);
    }
);

struct VideoRenderWidget::Impl {
    QOpenGLShaderProgram *shaderProgram = nullptr;
    QOpenGLVertexArrayObject *vao = nullptr;
    QOpenGLBuffer *vbo = nullptr;

    std::array<std::unique_ptr<QOpenGLTexture>, 4> textures;

    std::array<std::vector<uint8_t>, 4> planeData;
    std::array<int, 4> planeWidth = {0, 0, 0, 0};
    std::array<int, 4> planeHeight = {0, 0, 0, 0};

    int width = 0;
    int height = 0;

    VideoPixelFormat format = VideoPixelFormat::Unknown;
    VideoFitMode fitMode = VideoFitMode::Contain;
    VideoColorRange colorRange = VideoColorRange::Limited;
    VideoColorSpace colorSpace = VideoColorSpace::BT601;

    float scaleX = 1.0f;
    float scaleY = 1.0f;

    bool hasFrame = false;
    bool needRecreateTexture = true;
    bool dirty = false;
};

static bool is_supported_format(VideoPixelFormat format) {
    return format == VideoPixelFormat::YUV420P ||
           format == VideoPixelFormat::YUV422P ||
           format == VideoPixelFormat::YUV444P ||
           format == VideoPixelFormat::NV12 ||
           format == VideoPixelFormat::NV21 ||
           format == VideoPixelFormat::RGB24 ||
           format == VideoPixelFormat::RGBA;
}

static int plane_count(VideoPixelFormat format) {
    switch (format) {
        case VideoPixelFormat::YUV420P:
        case VideoPixelFormat::YUV422P:
        case VideoPixelFormat::YUV444P:
            return 3;

        case VideoPixelFormat::NV12:
        case VideoPixelFormat::NV21:
            return 2;

        case VideoPixelFormat::RGB24:
        case VideoPixelFormat::RGBA:
            return 1;

        default:
            return 0;
    }
}

static bool plane_layout(VideoPixelFormat format,
                         int width,
                         int height,
                         int plane,
                         int &texture_width,
                         int &texture_height,
                         int &row_bytes) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    switch (format) {
        case VideoPixelFormat::YUV420P:
            if (plane == 0) {
                texture_width = width;
                texture_height = height;
                row_bytes = width;
                return true;
            }
            if (plane == 1 || plane == 2) {
                texture_width = width / 2;
                texture_height = height / 2;
                row_bytes = width / 2;
                return true;
            }
            return false;

        case VideoPixelFormat::YUV422P:
            if (plane == 0) {
                texture_width = width;
                texture_height = height;
                row_bytes = width;
                return true;
            }
            if (plane == 1 || plane == 2) {
                texture_width = width / 2;
                texture_height = height;
                row_bytes = width / 2;
                return true;
            }
            return false;

        case VideoPixelFormat::YUV444P:
            if (plane == 0 || plane == 1 || plane == 2) {
                texture_width = width;
                texture_height = height;
                row_bytes = width;
                return true;
            }
            return false;

        case VideoPixelFormat::NV12:
            if (plane == 0) {
                texture_width = width;
                texture_height = height;
                row_bytes = width;
                return true;
            }
            if (plane == 1) {
                texture_width = width / 2;
                texture_height = height / 2;
                row_bytes = width;
                return true;
            }
            return false;

        case VideoPixelFormat::RGBA:
            if (plane == 0) {
                texture_width = width;
                texture_height = height;
                row_bytes = width * 4;
                return true;
            }
            return false;
        case VideoPixelFormat::NV21:
            if (plane == 0) {
                texture_width = width;
                texture_height = height;
                row_bytes = width;
                return true;
            }
            if (plane == 1) {
                texture_width = width / 2;
                texture_height = height / 2;
                row_bytes = width;
                return true;
            }
            return false;

        case VideoPixelFormat::RGB24:
            if (plane == 0) {
                texture_width = width;
                texture_height = height;
                row_bytes = width * 3;
                return true;
            }
            return false;
        default:
            return false;
    }
}

VideoRenderWidget::VideoRenderWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      impl_(std::make_unique<Impl>()) {
}

VideoRenderWidget::~VideoRenderWidget() {
    makeCurrent();

    for (auto &texture: impl_->textures) {
        if (texture) {
            texture->destroy();
            texture.reset();
        }
    }

    if (impl_->vao) {
        impl_->vao->destroy();
        delete impl_->vao;
        impl_->vao = nullptr;
    }

    if (impl_->vbo) {
        impl_->vbo->destroy();
        delete impl_->vbo;
        impl_->vbo = nullptr;
    }

    delete impl_->shaderProgram;
    impl_->shaderProgram = nullptr;

    doneCurrent();
}

bool VideoRenderWidget::setFrame(const VideoFrameView &frame) {
    if (frame.width <= 0 || frame.height <= 0) {
        return false;
    }

    if (!is_supported_format(frame.format)) {
        qWarning() << "unsupported pixel format";
        return false;
    }

    const int count = plane_count(frame.format);
    if (count <= 0) {
        return false;
    }

    for (int i = 0; i < count; ++i) {
        if (!frame.data[i] || frame.linesize[i] <= 0) {
            return false;
        }
    }

    const bool format_changed =
            impl_->format != frame.format ||
            impl_->width != frame.width ||
            impl_->height != frame.height;

    impl_->format = frame.format;
    impl_->width = frame.width;
    impl_->height = frame.height;

    for (int i = 0; i < count; ++i) {
        int texture_width = 0;
        int texture_height = 0;
        int row_bytes = 0;

        if (!plane_layout(frame.format, frame.width, frame.height, i,
                          texture_width, texture_height, row_bytes)) {
            return false;
        }

        impl_->planeWidth[i] = texture_width;
        impl_->planeHeight[i] = texture_height;
        impl_->planeData[i].resize(static_cast<size_t>(row_bytes * texture_height));

        const uint8_t *src = frame.data[i];
        uint8_t *dst = impl_->planeData[i].data();

        for (int row = 0; row < texture_height; ++row) {
            std::memcpy(dst + row * row_bytes, src + row * frame.linesize[i], row_bytes);
        }
    }

    impl_->hasFrame = true;
    impl_->dirty = true;

    if (format_changed) {
        impl_->needRecreateTexture = true;
        updateAspectRatio();
    }

    update();
    return true;
}

void VideoRenderWidget::setTextureYUVData(uint8_t *data[3], int linesize[3], int width, int height) {
    VideoFrameView frame;
    frame.data[0] = data[0];
    frame.data[1] = data[1];
    frame.data[2] = data[2];

    frame.linesize[0] = linesize[0];
    frame.linesize[1] = linesize[1];
    frame.linesize[2] = linesize[2];

    frame.width = width;
    frame.height = height;
    frame.format = VideoPixelFormat::YUV420P;

    setFrame(frame);
}

void VideoRenderWidget::clearFrame() {
    for (auto &plane: impl_->planeData) {
        plane.clear();
    }

    impl_->hasFrame = false;
    impl_->dirty = false;

    update();
}

void VideoRenderWidget::setFitMode(VideoFitMode mode) {
    impl_->fitMode = mode;
    updateAspectRatio();
    update();
}

void VideoRenderWidget::setColorRange(VideoColorRange range) {
    impl_->colorRange = range;
    update();
}

void VideoRenderWidget::setColorSpace(VideoColorSpace space) {
    impl_->colorSpace = space;
    update();
}

VideoPixelFormat VideoRenderWidget::pixelFormat() const {
    return impl_->format;
}

int VideoRenderWidget::videoWidth() const {
    return impl_->width;
}

int VideoRenderWidget::videoHeight() const {
    return impl_->height;
}

void VideoRenderWidget::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    impl_->shaderProgram = new QOpenGLShaderProgram(this);

    if (!impl_->shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader)) {
        qWarning() << "vertex shader error:" << impl_->shaderProgram->log();
    }

    if (!impl_->shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader)) {
        qWarning() << "fragment shader error:" << impl_->shaderProgram->log();
    }

    if (!impl_->shaderProgram->link()) {
        qWarning() << "shader link error:" << impl_->shaderProgram->log();
    }

    float vertices[] = {
        // position          // texcoord
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    };

    impl_->vao = new QOpenGLVertexArrayObject(this);
    impl_->vao->create();
    impl_->vao->bind();

    impl_->vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    impl_->vbo->create();
    impl_->vbo->bind();
    impl_->vbo->allocate(vertices, sizeof(vertices));

    impl_->shaderProgram->bind();

    impl_->shaderProgram->enableAttributeArray(0);
    impl_->shaderProgram->enableAttributeArray(1);

    impl_->shaderProgram->setAttributeBuffer(
        0,
        GL_FLOAT,
        0,
        3,
        5 * sizeof(float)
    );

    impl_->shaderProgram->setAttributeBuffer(
        1,
        GL_FLOAT,
        3 * sizeof(float),
        2,
        5 * sizeof(float)
    );

    impl_->shaderProgram->release();
    impl_->vbo->release();
    impl_->vao->release();
}

void VideoRenderWidget::resizeGL(int w, int h) {
    Q_UNUSED(w);
    Q_UNUSED(h);

    updateAspectRatio();
}

void VideoRenderWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (!impl_->shaderProgram || !impl_->vao || !impl_->hasFrame) {
        return;
    }

    const int count = plane_count(impl_->format);
    if (count <= 0) {
        return;
    }

    if (impl_->needRecreateTexture) {
        for (auto &texture: impl_->textures) {
            if (texture) {
                texture->destroy();
                texture.reset();
            }
        }

        for (int i = 0; i < count; ++i) {
            impl_->textures[i] = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);

            if ((impl_->format == VideoPixelFormat::NV12 ||
                 impl_->format == VideoPixelFormat::NV21) && i == 1) {
                impl_->textures[i]->setFormat(QOpenGLTexture::RG8_UNorm);
            } else if (impl_->format == VideoPixelFormat::RGBA && i == 0) {
                impl_->textures[i]->setFormat(QOpenGLTexture::RGBA8_UNorm);
            } else if (impl_->format == VideoPixelFormat::RGB24 && i == 0) {
                impl_->textures[i]->setFormat(QOpenGLTexture::RGB8_UNorm);
            } else {
                impl_->textures[i]->setFormat(QOpenGLTexture::R8_UNorm);
            }

            impl_->textures[i]->setSize(impl_->planeWidth[i], impl_->planeHeight[i]);
            impl_->textures[i]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
            impl_->textures[i]->setWrapMode(QOpenGLTexture::ClampToEdge);
            impl_->textures[i]->allocateStorage();
        }

        impl_->needRecreateTexture = false;
        impl_->dirty = true;
    }

    impl_->shaderProgram->bind();
    impl_->vao->bind();

    impl_->shaderProgram->setUniformValue("u_scale", impl_->scaleX, impl_->scaleY);

    if (impl_->colorRange == VideoColorRange::Limited) {
        impl_->shaderProgram->setUniformValue("u_y_offset", 16.0f / 255.0f);
        impl_->shaderProgram->setUniformValue("u_y_scale", 255.0f / 219.0f);
    } else {
        impl_->shaderProgram->setUniformValue("u_y_offset", 0.0f);
        impl_->shaderProgram->setUniformValue("u_y_scale", 1.0f);
    }

    if (impl_->colorSpace == VideoColorSpace::BT709) {
        impl_->shaderProgram->setUniformValue(
            "u_yuv_coeff",
            QVector4D(
                1.5748f,
                -0.187324f,
                -0.468124f,
                1.8556f
            )
        );
    } else {
        impl_->shaderProgram->setUniformValue(
            "u_yuv_coeff",
            QVector4D(
                1.402f,
                -0.344136f,
                -0.714136f,
                1.772f
            )
        );
    }

    if (impl_->dirty) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        for (int i = 0; i < count; ++i) {
            if (!impl_->textures[i] || impl_->planeData[i].empty()) {
                continue;
            }

            impl_->textures[i]->bind(i);

            QOpenGLTexture::PixelFormat pixel_format = QOpenGLTexture::Red;

            if ((impl_->format == VideoPixelFormat::NV12 ||
                 impl_->format == VideoPixelFormat::NV21) && i == 1) {
                pixel_format = QOpenGLTexture::RG;
            } else if (impl_->format == VideoPixelFormat::RGBA && i == 0) {
                pixel_format = QOpenGLTexture::RGBA;
            } else if (impl_->format == VideoPixelFormat::RGB24 && i == 0) {
                pixel_format = QOpenGLTexture::RGB;
            }

            impl_->textures[i]->setData(
                pixel_format,
                QOpenGLTexture::UInt8,
                impl_->planeData[i].data()
            );
        }

        impl_->dirty = false;
    }

    for (int i = 0; i < count; ++i) {
        if (impl_->textures[i]) {
            impl_->textures[i]->bind(i);
        }
    }

    int shader_format = 1;

    if (impl_->format == VideoPixelFormat::NV12) {
        shader_format = 2;
    } else if (impl_->format == VideoPixelFormat::RGBA) {
        shader_format = 3;
    } else if (impl_->format == VideoPixelFormat::NV21) {
        shader_format = 4;
    } else if (impl_->format == VideoPixelFormat::RGB24) {
        shader_format = 5;
    }

    impl_->shaderProgram->setUniformValue("u_format", shader_format);
    impl_->shaderProgram->setUniformValue("tex0", 0);
    impl_->shaderProgram->setUniformValue("tex1", 1);
    impl_->shaderProgram->setUniformValue("tex2", 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    for (int i = 0; i < count; ++i) {
        if (impl_->textures[i]) {
            impl_->textures[i]->release();
        }
    }

    impl_->vao->release();
    impl_->shaderProgram->release();
}

void VideoRenderWidget::updateAspectRatio() {
    if (impl_->width <= 0 || impl_->height <= 0 || width() <= 0 || height() <= 0) {
        impl_->scaleX = 1.0f;
        impl_->scaleY = 1.0f;
        return;
    }

    if (impl_->fitMode == VideoFitMode::Stretch) {
        impl_->scaleX = 1.0f;
        impl_->scaleY = 1.0f;
        return;
    }

    float windowAspect = static_cast<float>(width()) / static_cast<float>(height());
    float videoAspect = static_cast<float>(impl_->width) / static_cast<float>(impl_->height);

    if (impl_->fitMode == VideoFitMode::Contain) {
        if (windowAspect > videoAspect) {
            impl_->scaleX = videoAspect / windowAspect;
            impl_->scaleY = 1.0f;
        } else {
            impl_->scaleX = 1.0f;
            impl_->scaleY = windowAspect / videoAspect;
        }
        return;
    }

    if (impl_->fitMode == VideoFitMode::Cover) {
        if (windowAspect > videoAspect) {
            impl_->scaleX = 1.0f;
            impl_->scaleY = windowAspect / videoAspect;
        } else {
            impl_->scaleX = videoAspect / windowAspect;
            impl_->scaleY = 1.0f;
        }
    }
}

#include "yuvWidget.h"
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QDebug>

#define GLVERSION  "#version 330 core\n"
#define GET_SHADER(arg) GLVERSION#arg

const char *vertex_shader = GET_SHADER(
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;
    uniform vec2 u_scale;

    void main()
    {
    gl_Position = vec4(aPos.x * u_scale.x, aPos.y * u_scale.y, aPos.z, 1.0);
    TexCoord = aTexCoord;
    }
);

const char *frag_shader = GET_SHADER(
    in vec2 TexCoord;
    out vec4 FragColor;

    uniform sampler2D texY;
    uniform sampler2D texU;
    uniform sampler2D texV;

    void main()
    {
    vec3 yuv;
    yuv.x = texture(texY, TexCoord).r - 0.0625;
    yuv.y = texture(texU, TexCoord).r - 0.5;
    yuv.z = texture(texV, TexCoord).r - 0.5;

    vec3 rgb = mat3(1.164, 1.164, 1.164,
        0.0, -0.213, 2.112,
        1.793, -0.533, 0.0) * yuv;

    FragColor = vec4(rgb, 1.0);
    }
);

yuvWidget::yuvWidget(QWidget *parent, AVPixelFormat format)
    : QOpenGLWidget(parent),
      m_shaderProgram(nullptr),
      m_vao(nullptr),
      m_vbo_yuv(nullptr),
      m_width(0),
      m_height(0),
      m_scaleX(1.0f),
      m_scaleY(1.0f),
      m_format(format) // 初始化像素格式
{
}

yuvWidget::~yuvWidget() {
    makeCurrent();
    if (m_vao) {
        m_vao->destroy();
        delete m_vao;
    }
    if (m_vbo_yuv) {
        m_vbo_yuv->destroy();
        delete m_vbo_yuv;
    }
    delete m_shaderProgram;
    doneCurrent();
}

void yuvWidget::updateAspectRatio() {
    if (m_width <= 0 || m_height <= 0) {
        m_scaleX = 1.0f;
        m_scaleY = 1.0f;
        return;
    }

    float windowAspect = static_cast<float>(width()) / height();
    float videoAspect = static_cast<float>(m_width) / m_height;

    if (windowAspect > videoAspect) {
        m_scaleX = videoAspect / windowAspect;
        m_scaleY = 1.0f;
    } else {
        m_scaleX = 1.0f;
        m_scaleY = windowAspect / videoAspect;
    }
}

void yuvWidget::setTextureYUVData(uint8_t *Buffer[3], int Stride[3], int width, int height) {
    if (width <= 0 || height <= 0)
        return;

    if (m_width != width || m_height != height) {
        makeCurrent();

        for (int i = 0; i < 3; i++) {
            m_texture_2d_array[i].reset(new QOpenGLTexture(QOpenGLTexture::Target2D));
            m_texture_2d_array[i]->setFormat(QOpenGLTexture::R8_UNorm);
            m_texture_2d_array[i]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        }

        // 设置纹理大小
        switch (m_format) {
            case AV_PIX_FMT_YUV420P:
                m_texture_2d_array[0]->setSize(width, height);
                m_texture_2d_array[1]->setSize(width / 2, height / 2);
                m_texture_2d_array[2]->setSize(width / 2, height / 2);
                break;
            case AV_PIX_FMT_YUV422P:
                m_texture_2d_array[0]->setSize(width, height);
                m_texture_2d_array[1]->setSize(width / 2, height);
                m_texture_2d_array[2]->setSize(width / 2, height);
                break;
            case AV_PIX_FMT_YUV444P:
                m_texture_2d_array[0]->setSize(width, height);
                m_texture_2d_array[1]->setSize(width, height);
                m_texture_2d_array[2]->setSize(width, height);
                break;
            default:
                qWarning() << "Unsupported pixel format!";
                return;
        }

        for (int i = 0; i < 3; i++) {
            m_texture_2d_array[i]->allocateStorage();
        }

        m_width = width;
        m_height = height;
        updateAspectRatio();
        doneCurrent();
    }

    int ySize = width * height;
    int uSize, vSize;

    switch (m_format) {
        case AV_PIX_FMT_YUV420P:
            uSize = vSize = ySize / 4;
            break;
        case AV_PIX_FMT_YUV422P:
            uSize = vSize = ySize / 2;
            break;
        case AV_PIX_FMT_YUV444P:
            uSize = vSize = ySize;
            break;
        default:
            return;
    }

    m_yTexture_data.reset(new uint8_t[ySize]);
    m_uTexture_data.reset(new uint8_t[uSize]);
    m_vTexture_data.reset(new uint8_t[vSize]);

    const uint8_t *pY = Buffer[0], *pU = Buffer[1], *pV = Buffer[2];
    uint8_t *pYData = m_yTexture_data.get(), *pUData = m_uTexture_data.get(), *pVData = m_vTexture_data.get();

    for (int i = 0; i < height; ++i) {
        memcpy(pYData, pY, width);
        pY += Stride[0];
        pYData += width;
    }

    int uHeight = (m_format == AV_PIX_FMT_YUV420P) ? height / 2 : height;
    int uWidth = (m_format == AV_PIX_FMT_YUV444P) ? width : width / 2;

    for (int i = 0; i < uHeight; ++i) {
        memcpy(pUData, pU, uWidth);
        pU += Stride[1];
        pUData += uWidth;
    }

    for (int i = 0; i < uHeight; ++i) {
        memcpy(pVData, pV, uWidth);
        pV += Stride[2];
        pVData += uWidth;
    }

    update();
}


void yuvWidget::textureYUVData(QList<QByteArray> &buffers, int &width, int &height) {
    width = m_width;
    height = m_height;

    // 获取Y, U, V 数据
    uint8_t *y = m_yTexture_data.get();
    uint8_t *u = m_uTexture_data.get();
    uint8_t *v = m_vTexture_data.get();

    // 组装成字符串形式
    buffers.clear();
    buffers << QByteArray(reinterpret_cast<const char *>(y), width * height) // Y数据
            << QByteArray(reinterpret_cast<const char *>(u), width * height / 4) // U数据
            << QByteArray(reinterpret_cast<const char *>(v), width * height / 4); // V数据
}

void yuvWidget::clearTextureColor() {
    m_yTexture_data.reset();
    m_uTexture_data.reset();
    m_vTexture_data.reset();
    update();
}

void yuvWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    m_shaderProgram = new QOpenGLShaderProgram(this);
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader)) {
        qDebug() << "Vertex shader error:" << m_shaderProgram->log();
    }
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, frag_shader)) {
        qDebug() << "Fragment shader error:" << m_shaderProgram->log();
    }
    if (!m_shaderProgram->link()) {
        qDebug() << "Shader program link error:" << m_shaderProgram->log();
    }

    // 顶点数据
    float vertices[] = {
        // 位置              // 纹理坐标
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 左下
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f, // 右下
        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, // 左上
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f // 右上
    };

    // 设置VAO和VBO
    m_vao = new QOpenGLVertexArrayObject(this);
    m_vao->create();
    m_vao->bind();

    m_vbo_yuv = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbo_yuv->create();
    m_vbo_yuv->bind();
    m_vbo_yuv->allocate(vertices, sizeof(vertices));

    // 设置顶点属性指针
    m_shaderProgram->bind();
    m_shaderProgram->enableAttributeArray(0);
    m_shaderProgram->enableAttributeArray(1);
    m_shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 5 * sizeof(float));
    m_shaderProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));

    m_vao->release();
    m_vbo_yuv->release();
    m_shaderProgram->release();
}

void yuvWidget::resizeGL(int w, int h) {
    Q_UNUSED(w);
    Q_UNUSED(h);
    updateAspectRatio();
}

void yuvWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_shaderProgram || !m_vao)
        return;

    m_shaderProgram->bind();
    m_vao->bind();

    // 设置缩放参数
    GLuint scaleLoc = m_shaderProgram->uniformLocation("u_scale");
    m_shaderProgram->setUniformValue(scaleLoc, m_scaleX, m_scaleY);

    if (m_yTexture_data && m_uTexture_data && m_vTexture_data) {
        // 绑定Y纹理
        if (m_texture_2d_array[0]) {
            m_texture_2d_array[0]->bind(0);
            m_texture_2d_array[0]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, m_yTexture_data.get());
            m_shaderProgram->setUniformValue("texY", 0);
        }

        // 绑定U纹理
        if (m_texture_2d_array[1]) {
            m_texture_2d_array[1]->bind(1);
            m_texture_2d_array[1]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, m_uTexture_data.get());
            m_shaderProgram->setUniformValue("texU", 1);
        }

        // 绑定V纹理
        if (m_texture_2d_array[2]) {
            m_texture_2d_array[2]->bind(2);
            m_texture_2d_array[2]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, m_vTexture_data.get());
            m_shaderProgram->setUniformValue("texV", 2);
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 解绑纹理
        for (int i = 0; i < 3; ++i) {
            if (m_texture_2d_array[i]) {
                m_texture_2d_array[i]->release();
            }
        }
    }

    m_vao->release();
    m_shaderProgram->release();
}

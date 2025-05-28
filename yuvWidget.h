#ifndef QVIDEORENDERWIDGET_H
#define QVIDEORENDERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <memory>
#include <QWidget>

#if __has_include("libavutil/pixfmt.h")
extern "C" {
#include <libavutil/pixfmt.h>
}
#else
enum AVPixelFormat {AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV444P};
#endif


class QOpenGLVertexArrayObject;
class QOpenGLBuffer;

class yuvWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    // AV_PIX_FMT_YUV420P AV_PIX_FMT_YUV422P AV_PIX_FMT_YUV444P
    explicit yuvWidget(QWidget *parent = nullptr, AVPixelFormat format = AV_PIX_FMT_YUV420P);

    ~yuvWidget();

    void setTextureYUVData(uint8_t *Buffer[3], int Stride[3], int width, int height);

    void textureYUVData(QList<QByteArray> &buffers, int &width, int &height);

    void clearTextureColor();

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

private:
    void updateAspectRatio();

    QSharedPointer<QOpenGLTexture> m_texture_2d_array[3];
    QOpenGLShaderProgram *m_shaderProgram;
    QOpenGLVertexArrayObject *m_vao;
    QOpenGLBuffer *m_vbo_yuv;
    std::shared_ptr<uint8_t[]> m_yTexture_data;
    std::shared_ptr<uint8_t[]> m_uTexture_data;
    std::shared_ptr<uint8_t[]> m_vTexture_data;

    int m_width;
    int m_height;
    float m_scaleX;
    float m_scaleY;

    AVPixelFormat m_format;
};

#endif // QVIDEORENDERWIDGET_H

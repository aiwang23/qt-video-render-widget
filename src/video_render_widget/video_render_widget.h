#ifndef VIDEO_RENDER_WIDGET_H
#define VIDEO_RENDER_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <cstdint>
#include <memory>

#include "video_frame.h"

enum class VideoFitMode {
    Contain,
    Cover,
    Stretch
};

enum class VideoColorRange {
    Limited,
    Full
};

enum class VideoColorSpace {
    BT601,
    BT709
};


class VideoRenderWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit VideoRenderWidget(QWidget *parent = nullptr);

    ~VideoRenderWidget() override;

    VideoRenderWidget(const VideoRenderWidget &) = delete;

    VideoRenderWidget &operator=(const VideoRenderWidget &) = delete;

    bool setFrame(const VideoFrameView &frame);

    [[deprecated("use setFrame instead")]]
    void setTextureYUVData(uint8_t *data[3], int linesize[3], int width, int height);

    void clearFrame();

    void setFitMode(VideoFitMode mode);

    void setColorRange(VideoColorRange range);

    void setColorSpace(VideoColorSpace space);

    VideoPixelFormat pixelFormat() const;

    int videoWidth() const;

    int videoHeight() const;

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

private:
    void updateAspectRatio();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
};

#endif // VIDEO_RENDER_WIDGET_H

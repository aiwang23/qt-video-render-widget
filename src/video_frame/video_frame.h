//
// Created by 10484 on 2026/5/28.
//

#ifndef VIDEO_FRAME_H
#define VIDEO_FRAME_H
#include <cstdint>

enum class VideoPixelFormat {
    Unknown,

    YUV420P,
    YUV422P,
    YUV444P,

    NV12,
    NV21,

    RGB24,
    RGBA
};

struct VideoFrameView {
    const uint8_t *data[4] = {nullptr, nullptr, nullptr, nullptr};
    int linesize[4] = {0, 0, 0, 0};

    int width = 0;
    int height = 0;

    VideoPixelFormat format = VideoPixelFormat::Unknown;

    int64_t pts_us = 0;
};

#endif //VIDEO_FRAME_H

//
// Created by 10484 on 2026/5/28.
//

#ifndef H264_DECODER_H
#define H264_DECODER_H

#include "video_frame.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class H264Decoder {
public:
    using FrameHandler = std::function<void(const VideoFrameView & frame)>;

    H264Decoder();

    ~H264Decoder();

    H264Decoder(const H264Decoder &) = delete;

    H264Decoder &operator=(const H264Decoder &) = delete;

    bool open();

    bool open(VideoPixelFormat output_format);

    void close();

    bool decode(const uint8_t *data, int size);

    bool decode(const std::vector<uint8_t> &frame);

    void onFrame(FrameHandler handler);

    VideoPixelFormat outputFormat() const;

    const char *lastError() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // H264_DECODER_H

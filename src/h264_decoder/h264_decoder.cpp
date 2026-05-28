//
// Created by 10484 on 2026/5/28.
//

#include "h264_decoder.h"


#include <cstring>
#include <string>
#include <utility>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

static std::string ffmpeg_error_string(int err) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_make_error_string(buf, sizeof(buf), err);
    return buf;
}

static AVPixelFormat to_av_pixel_format(VideoPixelFormat format) {
    switch (format) {
        case VideoPixelFormat::YUV420P:
            return AV_PIX_FMT_YUV420P;
        case VideoPixelFormat::YUV422P:
            return AV_PIX_FMT_YUV422P;
        case VideoPixelFormat::YUV444P:
            return AV_PIX_FMT_YUV444P;
        case VideoPixelFormat::NV12:
            return AV_PIX_FMT_NV12;
        case VideoPixelFormat::NV21:
            return AV_PIX_FMT_NV21;
        case VideoPixelFormat::RGB24:
            return AV_PIX_FMT_RGB24;
        case VideoPixelFormat::RGBA:
            return AV_PIX_FMT_RGBA;
        case VideoPixelFormat::Unknown:
        default:
            return AV_PIX_FMT_NONE;
    }
}

static VideoPixelFormat from_av_pixel_format(AVPixelFormat format) {
    switch (format) {
        case AV_PIX_FMT_YUV420P:
            return VideoPixelFormat::YUV420P;
        case AV_PIX_FMT_YUV422P:
            return VideoPixelFormat::YUV422P;
        case AV_PIX_FMT_YUV444P:
            return VideoPixelFormat::YUV444P;
        case AV_PIX_FMT_NV12:
            return VideoPixelFormat::NV12;
        case AV_PIX_FMT_NV21:
            return VideoPixelFormat::NV21;
        case AV_PIX_FMT_RGB24:
            return VideoPixelFormat::RGB24;
        case AV_PIX_FMT_RGBA:
            return VideoPixelFormat::RGBA;
        default:
            return VideoPixelFormat::Unknown;
    }
}

struct H264Decoder::Impl {
    const AVCodec *codec = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;

    SwsContext *sws_ctx = nullptr;
    AVFrame *converted_frame = nullptr;

    VideoPixelFormat output_format = VideoPixelFormat::Unknown;
    AVPixelFormat converted_av_format = AV_PIX_FMT_NONE;

    int converted_width = 0;
    int converted_height = 0;

    FrameHandler frame_handler;
    std::string last_error;

    void setError(const std::string &error) {
        last_error = error;
    }

    void setError(const std::string &prefix, int err) {
        last_error = prefix + ": " + ffmpeg_error_string(err);
    }
};

H264Decoder::H264Decoder()
    : impl_(std::make_unique<Impl>()) {
}

H264Decoder::~H264Decoder() {
    close();
}

bool H264Decoder::open() {
    return open(VideoPixelFormat::Unknown);
}

bool H264Decoder::open(VideoPixelFormat output_format) {
    close();

    impl_->output_format = output_format;

    if (output_format != VideoPixelFormat::Unknown) {
        impl_->converted_av_format = to_av_pixel_format(output_format);
        if (impl_->converted_av_format == AV_PIX_FMT_NONE) {
            impl_->setError("unsupported output pixel format");
            return false;
        }
    }

    impl_->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!impl_->codec) {
        impl_->setError("H264 decoder not found");
        return false;
    }

    impl_->codec_ctx = avcodec_alloc_context3(impl_->codec);
    if (!impl_->codec_ctx) {
        impl_->setError("avcodec_alloc_context3 failed");
        return false;
    }

    int ret = avcodec_open2(impl_->codec_ctx, impl_->codec, nullptr);
    if (ret < 0) {
        impl_->setError("avcodec_open2 failed", ret);
        close();
        return false;
    }

    impl_->packet = av_packet_alloc();
    if (!impl_->packet) {
        impl_->setError("av_packet_alloc failed");
        close();
        return false;
    }

    impl_->frame = av_frame_alloc();
    if (!impl_->frame) {
        impl_->setError("av_frame_alloc failed");
        close();
        return false;
    }

    if (output_format != VideoPixelFormat::Unknown) {
        impl_->converted_frame = av_frame_alloc();
        if (!impl_->converted_frame) {
            impl_->setError("converted av_frame_alloc failed");
            close();
            return false;
        }
    }

    impl_->last_error.clear();
    return true;
}

void H264Decoder::close() {
    if (!impl_) {
        return;
    }

    if (impl_->converted_frame) {
        av_frame_free(&impl_->converted_frame);
    }

    if (impl_->sws_ctx) {
        sws_freeContext(impl_->sws_ctx);
        impl_->sws_ctx = nullptr;
    }

    if (impl_->frame) {
        av_frame_free(&impl_->frame);
    }

    if (impl_->packet) {
        av_packet_free(&impl_->packet);
    }

    if (impl_->codec_ctx) {
        avcodec_free_context(&impl_->codec_ctx);
    }

    impl_->codec = nullptr;
    impl_->converted_av_format = AV_PIX_FMT_NONE;
    impl_->converted_width = 0;
    impl_->converted_height = 0;
}

bool H264Decoder::decode(const std::vector<uint8_t> &frame) {
    return decode(frame.data(), static_cast<int>(frame.size()));
}

bool H264Decoder::decode(const uint8_t *data, int size) {
    if (!impl_->codec_ctx || !impl_->packet || !impl_->frame) {
        impl_->setError("decoder not opened");
        return false;
    }

    if (!data || size <= 0) {
        impl_->setError("invalid h264 data");
        return false;
    }

    std::vector<uint8_t> padded_data(size + AV_INPUT_BUFFER_PADDING_SIZE);
    std::memcpy(padded_data.data(), data, size);
    std::memset(padded_data.data() + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    av_packet_unref(impl_->packet);
    impl_->packet->data = padded_data.data();
    impl_->packet->size = size;

    int ret = avcodec_send_packet(impl_->codec_ctx, impl_->packet);
    if (ret < 0) {
        impl_->setError("avcodec_send_packet failed", ret);
        return false;
    }

    while (true) {
        ret = avcodec_receive_frame(impl_->codec_ctx, impl_->frame);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }

        if (ret < 0) {
            impl_->setError("avcodec_receive_frame failed", ret);
            return false;
        }

        AVFrame *output_frame = impl_->frame;
        VideoPixelFormat output_format = from_av_pixel_format(
            static_cast<AVPixelFormat>(impl_->frame->format)
        );

        if (impl_->output_format != VideoPixelFormat::Unknown) {
            AVPixelFormat src_format = static_cast<AVPixelFormat>(impl_->frame->format);
            AVPixelFormat dst_format = impl_->converted_av_format;

            impl_->sws_ctx = sws_getCachedContext(
                impl_->sws_ctx,
                impl_->frame->width,
                impl_->frame->height,
                src_format,
                impl_->frame->width,
                impl_->frame->height,
                dst_format,
                SWS_BILINEAR,
                nullptr,
                nullptr,
                nullptr
            );

            if (!impl_->sws_ctx) {
                impl_->setError("sws_getCachedContext failed");
                av_frame_unref(impl_->frame);
                return false;
            }

            if (impl_->converted_width != impl_->frame->width ||
                impl_->converted_height != impl_->frame->height ||
                impl_->converted_frame->format != dst_format) {
                av_frame_unref(impl_->converted_frame);

                impl_->converted_frame->format = dst_format;
                impl_->converted_frame->width = impl_->frame->width;
                impl_->converted_frame->height = impl_->frame->height;

                ret = av_frame_get_buffer(impl_->converted_frame, 32);
                if (ret < 0) {
                    impl_->setError("av_frame_get_buffer failed", ret);
                    av_frame_unref(impl_->frame);
                    return false;
                }

                impl_->converted_width = impl_->frame->width;
                impl_->converted_height = impl_->frame->height;
            }

            ret = av_frame_make_writable(impl_->converted_frame);
            if (ret < 0) {
                impl_->setError("av_frame_make_writable failed", ret);
                av_frame_unref(impl_->frame);
                return false;
            }

            sws_scale(
                impl_->sws_ctx,
                impl_->frame->data,
                impl_->frame->linesize,
                0,
                impl_->frame->height,
                impl_->converted_frame->data,
                impl_->converted_frame->linesize
            );

            output_frame = impl_->converted_frame;
            output_format = impl_->output_format;
        }

        if (impl_->frame_handler) {
            VideoFrameView video_frame;

            for (int i = 0; i < 4; ++i) {
                video_frame.data[i] = output_frame->data[i];
                video_frame.linesize[i] = output_frame->linesize[i];
            }

            video_frame.width = output_frame->width;
            video_frame.height = output_frame->height;
            video_frame.format = output_format;
            video_frame.pts_us  = output_frame->pts;

            impl_->frame_handler(video_frame);
        }

        av_frame_unref(impl_->frame);
    }

    impl_->last_error.clear();
    return true;
}

void H264Decoder::onFrame(FrameHandler handler) {
    impl_->frame_handler = std::move(handler);
}

VideoPixelFormat H264Decoder::outputFormat() const {
    return impl_->output_format;
}

const char *H264Decoder::lastError() const {
    return impl_->last_error.c_str();
}

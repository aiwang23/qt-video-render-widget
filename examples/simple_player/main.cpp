#include "video_render_widget.h"

#include <QApplication>
#include <QTimer>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static VideoPixelFormat parse_format(const std::string& s) {
    if (s == "yuv420p")
        return VideoPixelFormat::YUV420P;
    if (s == "yuv422p")
        return VideoPixelFormat::YUV422P;
    if (s == "yuv444p")
        return VideoPixelFormat::YUV444P;
    if (s == "nv12")
        return VideoPixelFormat::NV12;
    if (s == "nv21")
        return VideoPixelFormat::NV21;
    if (s == "rgb24")
        return VideoPixelFormat::RGB24;
    if (s == "rgba")
        return VideoPixelFormat::RGBA;
    return VideoPixelFormat::Unknown;
}

static bool plane_layout(VideoPixelFormat format, int width, int height, int plane, int& row_bytes,
                         int& rows) {
    switch (format) {
    case VideoPixelFormat::YUV420P:
        if (plane == 0) {
            row_bytes = width;
            rows = height;
            return true;
        }
        if (plane == 1 || plane == 2) {
            row_bytes = width / 2;
            rows = height / 2;
            return true;
        }
        return false;

    case VideoPixelFormat::YUV422P:
        if (plane == 0) {
            row_bytes = width;
            rows = height;
            return true;
        }
        if (plane == 1 || plane == 2) {
            row_bytes = width / 2;
            rows = height;
            return true;
        }
        return false;

    case VideoPixelFormat::YUV444P:
        if (plane == 0 || plane == 1 || plane == 2) {
            row_bytes = width;
            rows = height;
            return true;
        }
        return false;

    case VideoPixelFormat::NV12:
    case VideoPixelFormat::NV21:
        if (plane == 0) {
            row_bytes = width;
            rows = height;
            return true;
        }
        if (plane == 1) {
            row_bytes = width;
            rows = height / 2;
            return true;
        }
        return false;

    case VideoPixelFormat::RGB24:
        if (plane == 0) {
            row_bytes = width * 3;
            rows = height;
            return true;
        }
        return false;

    case VideoPixelFormat::RGBA:
        if (plane == 0) {
            row_bytes = width * 4;
            rows = height;
            return true;
        }
        return false;

    default:
        return false;
    }
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

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    if (argc < 5) {
        std::cerr << "usage:\n"
                  << "  simple_player <file> <width> <height> <format> [fps]\n\n"
                  << "formats:\n"
                  << "  yuv420p yuv422p yuv444p nv12 nv21 rgb24 rgba\n";
        return -1;
    }

    std::string path = argv[1];
    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);
    VideoPixelFormat format = parse_format(argv[4]);
    int fps = argc >= 6 ? std::stoi(argv[5]) : 30;

    if (format == VideoPixelFormat::Unknown || width <= 0 || height <= 0 || fps <= 0) {
        std::cerr << "invalid args\n";
        return -1;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "open file failed: " << path << "\n";
        return -1;
    }

    VideoRenderWidget widget;
    widget.resize(960, 540);
    widget.show();

    const int count = plane_count(format);

    std::vector<std::vector<uint8_t>> planes(count);
    int linesize[4] = {0, 0, 0, 0};

    for (int i = 0; i < count; ++i) {
        int row_bytes = 0;
        int rows = 0;

        if (!plane_layout(format, width, height, i, row_bytes, rows)) {
            std::cerr << "invalid plane layout\n";
            return -1;
        }

        linesize[i] = row_bytes;
        planes[i].resize(static_cast<size_t>(row_bytes * rows));
    }

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&] {
        for (int i = 0; i < count; ++i) {
            file.read(reinterpret_cast<char*>(planes[i].data()),
                      static_cast<std::streamsize>(planes[i].size()));

            if (file.gcount() != static_cast<std::streamsize>(planes[i].size())) {
                file.clear();
                file.seekg(0, std::ios::beg);
                return;
            }
        }

        VideoFrameView frame;
        frame.width = width;
        frame.height = height;
        frame.format = format;

        for (int i = 0; i < count; ++i) {
            frame.data[i] = planes[i].data();
            frame.linesize[i] = linesize[i];
        }

        widget.setFrame(frame);
    });

    timer.start(1000 / fps);

    return QApplication::exec();
}
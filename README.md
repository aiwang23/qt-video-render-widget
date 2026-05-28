# qt-video-render-widget

[English](./README.md) | [中文](./README_CN.md)

A Qt OpenGL video render widget.

It supports rendering raw video frames in these pixel formats:

* YUV420P
* YUV422P
* YUV444P
* NV12
* NV21
* RGB24
* RGBA

It also provides an optional FFmpeg-based H264 decoder.

## Features

* Qt `QOpenGLWidget` based rendering
* GPU shader YUV/NV12/NV21 to RGB conversion
* Supports planar YUV, semi-planar YUV, RGB and RGBA
* Keeps aspect ratio by default
* Simple `VideoFrameView` input interface
* Optional FFmpeg H264 decoding module

## Project Structure

```text
qt-video-render-widget/
├── CMakeLists.txt
├── examples/
│   └── simple_player/
└── src/
    ├── video_frame/
    ├── video_render_widget/
    └── h264_decoder/
```

## Modules

### video_frame

Common video frame data structures.

```cpp
VideoFrameView
VideoPixelFormat
```

### video_render_widget

Qt OpenGL video render widget.

```cpp
VideoRenderWidget
```

### h264_decoder

Optional FFmpeg H264 decoder.

```cpp
H264Decoder
```

This module is disabled by default.

## Build

### Build core library

```bash
cmake -B build -DBUILD_H264_DECODER=OFF
cmake --build build
```

### Build with examples

```bash
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

### Build with H264 decoder

Set `FFMPEG_ROOT` first:

```bash
cmake -B build -DBUILD_H264_DECODER=ON -DFFMPEG_ROOT="C:/Software/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1"
cmake --build build
```

## Basic Usage

```cpp
#include "video_render_widget.h"

VideoRenderWidget* widget = new VideoRenderWidget(parent);

VideoFrameView frame;
frame.width = 640;
frame.height = 360;
frame.format = VideoPixelFormat::YUV420P;

frame.data[0] = y_data;
frame.data[1] = u_data;
frame.data[2] = v_data;

frame.linesize[0] = y_stride;
frame.linesize[1] = u_stride;
frame.linesize[2] = v_stride;

widget->setFrame(frame);
```

## Use in CMake Project

```cmake
add_subdirectory(qt-video-render-widget)

target_link_libraries(your_app PRIVATE
        video_render_widget
)
```

If you need the H264 decoder:

```cmake
set(BUILD_H264_DECODER ON)
set(FFMPEG_ROOT "C:/Software/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1")

add_subdirectory(qt-video-render-widget)

target_link_libraries(your_app PRIVATE
        video_render_widget
        h264_decoder
)
```

## simple_player Example

`simple_player` plays raw video files.

Usage:

```bash
simple_player <file> <width> <height> <format> [fps]
```

Supported formats:

```text
yuv420p
yuv422p
yuv444p
nv12
nv21
rgb24
rgba
```

Example:

```bash
simple_player test_640x360_yuv420p.yuv 640 360 yuv420p 25
```

```bash
simple_player test_640x360_nv12.yuv 640 360 nv12 25
```

```bash
simple_player test_640x360_rgba.rgba 640 360 rgba 25
```

## Generate Test Raw Video Files

Prepare a H264 test file:

```bash
ffprobe test.h264
```

Example output:

```text
Video: h264, yuv420p, 640x360, 25 fps
```

Generate raw video files:

```bash
ffmpeg -i test.h264 -pix_fmt yuv420p -f rawvideo test_640x360_yuv420p.yuv
ffmpeg -i test.h264 -pix_fmt yuv422p -f rawvideo test_640x360_yuv422p.yuv
ffmpeg -i test.h264 -pix_fmt yuv444p -f rawvideo test_640x360_yuv444p.yuv
ffmpeg -i test.h264 -pix_fmt nv12 -f rawvideo test_640x360_nv12.yuv
ffmpeg -i test.h264 -pix_fmt nv21 -f rawvideo test_640x360_nv21.yuv
ffmpeg -i test.h264 -pix_fmt rgb24 -f rawvideo test_640x360_rgb24.rgb
ffmpeg -i test.h264 -pix_fmt rgba -f rawvideo test_640x360_rgba.rgba
```

Run example:

```bash
simple_player test_640x360_yuv420p.yuv 640 360 yuv420p 25
simple_player test_640x360_nv12.yuv 640 360 nv12 25
simple_player test_640x360_nv21.yuv 640 360 nv21 25
simple_player test_640x360_rgb24.rgb 640 360 rgb24 25
simple_player test_640x360_rgba.rgba 640 360 rgba 25
```

## Notes

Raw video files are usually very large.

Do not commit generated `.yuv`, `.rgb`, `.rgba`, `.h264`, `.mp4`, `.mkv` files into Git.

Recommended `.gitignore`:

```gitignore
build/
cmake-build-*/
.cache/
*.yuv
*.rgb
*.rgba
*.h264
*.mp4
*.avi
*.mkv
```

## License

This project is licensed under the Apache License 2.0.

See [LICENSE](LICENSE) for details.

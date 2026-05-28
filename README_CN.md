# qt-video-render-widget

[English](./README.md) | [中文](./README_CN.md)

一个基于 Qt OpenGL 的视频渲染组件。

支持渲染以下原始视频帧格式：

* YUV420P
* YUV422P
* YUV444P
* NV12
* NV21
* RGB24
* RGBA

同时提供一个可选的 FFmpeg H264 解码模块。

## 功能

* 基于 Qt `QOpenGLWidget`
* 使用 GPU shader 完成 YUV/NV12/NV21 到 RGB 的转换
* 支持 planar YUV、semi-planar YUV、RGB、RGBA
* 默认保持视频比例显示
* 使用简单的 `VideoFrameView` 输入接口
* 可选支持 FFmpeg H264 解码

## 项目结构

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

## 模块说明

### video_frame

通用视频帧结构定义。

```cpp
VideoFrameView
VideoPixelFormat
```

### video_render_widget

Qt OpenGL 视频渲染组件。

```cpp
VideoRenderWidget
```

### h264_decoder

可选的 FFmpeg H264 解码模块。

```cpp
H264Decoder
```

该模块默认不启用。

## 编译

### 只编译核心库

```bash
cmake -B build -DBUILD_H264_DECODER=OFF
cmake --build build
```

### 编译示例程序

```bash
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

### 编译 H264 解码模块

先设置 `FFMPEG_ROOT`：

```bash
cmake -B build -DBUILD_H264_DECODER=ON -DFFMPEG_ROOT="C:/Software/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1"
cmake --build build
```

## 基本用法

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

## 在 CMake 项目中使用

```cmake
add_subdirectory(qt-video-render-widget)

target_link_libraries(your_app PRIVATE
        video_render_widget
)
```

如果需要 H264 解码模块：

```cmake
set(BUILD_H264_DECODER ON)
set(FFMPEG_ROOT "C:/Software/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1")

add_subdirectory(qt-video-render-widget)

target_link_libraries(your_app PRIVATE
        video_render_widget
        h264_decoder
)
```

## simple_player 示例

`simple_player` 用于播放原始视频文件。

用法：

```bash
simple_player <file> <width> <height> <format> [fps]
```

支持格式：

```text
yuv420p
yuv422p
yuv444p
nv12
nv21
rgb24
rgba
```

示例：

```bash
simple_player test_640x360_yuv420p.yuv 640 360 yuv420p 25
```

```bash
simple_player test_640x360_nv12.yuv 640 360 nv12 25
```

```bash
simple_player test_640x360_rgba.rgba 640 360 rgba 25
```

## 生成测试用原始视频文件

先准备一个 H264 测试文件：

```bash
ffprobe test.h264
```

示例输出：

```text
Video: h264, yuv420p, 640x360, 25 fps
```

生成不同格式的 raw video：

```bash
ffmpeg -i test.h264 -pix_fmt yuv420p -f rawvideo test_640x360_yuv420p.yuv
ffmpeg -i test.h264 -pix_fmt yuv422p -f rawvideo test_640x360_yuv422p.yuv
ffmpeg -i test.h264 -pix_fmt yuv444p -f rawvideo test_640x360_yuv444p.yuv
ffmpeg -i test.h264 -pix_fmt nv12 -f rawvideo test_640x360_nv12.yuv
ffmpeg -i test.h264 -pix_fmt nv21 -f rawvideo test_640x360_nv21.yuv
ffmpeg -i test.h264 -pix_fmt rgb24 -f rawvideo test_640x360_rgb24.rgb
ffmpeg -i test.h264 -pix_fmt rgba -f rawvideo test_640x360_rgba.rgba
```

运行示例：

```bash
simple_player test_640x360_yuv420p.yuv 640 360 yuv420p 25
simple_player test_640x360_nv12.yuv 640 360 nv12 25
simple_player test_640x360_nv21.yuv 640 360 nv21 25
simple_player test_640x360_rgb24.rgb 640 360 rgb24 25
simple_player test_640x360_rgba.rgba 640 360 rgba 25
```

## 注意事项

原始视频文件通常非常大。

不要把生成的 `.yuv`、`.rgb`、`.rgba`、`.h264`、`.mp4`、`.mkv` 文件提交到 Git。

推荐 `.gitignore`：

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

请在这里补充许可证。

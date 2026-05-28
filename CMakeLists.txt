cmake_minimum_required(VERSION 3.20)
project(qt_video_render_widget)

set(CMAKE_PREFIX_PATH "C:/Qt/6.6.2/msvc2019_64/lib/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(BUILD_H264_DECODER "Build FFmpeg H264 decoder" OFF)

option(BUILD_EXAMPLES "Build examples" ON)

add_subdirectory(src/video_frame)
add_subdirectory(src/video_render_widget)

if(BUILD_H264_DECODER)
    add_subdirectory(src/h264_decoder)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples/simple_player)
endif()

set(FFMPEG_ROOT "C:/Software/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1" CACHE PATH "FFmpeg root path")

if(NOT EXISTS "${FFMPEG_ROOT}/include/libavcodec/avcodec.h")
        message(FATAL_ERROR "FFmpeg not found. Please set FFMPEG_ROOT.")
endif()

add_library(h264_decoder STATIC
        h264_decoder.cpp
        h264_decoder.h
)

target_include_directories(h264_decoder PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${FFMPEG_ROOT}/include
)

target_link_libraries(h264_decoder PUBLIC
        video_frame
        ${FFMPEG_ROOT}/lib/avcodec.lib
        ${FFMPEG_ROOT}/lib/avutil.lib
        ${FFMPEG_ROOT}/lib/swscale.lib
)

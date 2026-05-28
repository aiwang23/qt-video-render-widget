find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        OpenGL
        OpenGLWidgets
        REQUIRED)

add_library(video_render_widget STATIC
        video_render_widget.cpp
        video_render_widget.h
)

target_include_directories(video_render_widget PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(video_render_widget PUBLIC
        video_frame
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
)
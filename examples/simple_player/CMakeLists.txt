find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    REQUIRED)

add_executable(simple_player
    main.cpp
)

target_link_libraries(simple_player PRIVATE
    video_render_widget

    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

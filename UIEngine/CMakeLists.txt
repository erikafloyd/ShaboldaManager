add_library(ui_engine STATIC
    microui/microui.c
    microui/microui.h
    gui_renderer.cpp
    gui_renderer.hpp
    gui_manager.cpp
    gui_manager.hpp
)

target_include_directories(ui_engine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/microui
        ${CMAKE_SOURCE_DIR}/raylib/include
        ${CMAKE_SOURCE_DIR}/api/libsodium/include
        ${CMAKE_SOURCE_DIR}/api/json/include
        ${CMAKE_SOURCE_DIR}
)

cmake_minimum_required(VERSION 3.12)
project(RaylibMicroUIApp C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(UIEngine)
add_subdirectory(assets)
add_subdirectory(api)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/api/libsodium/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ui_engine  
    vault_backend
    "${CMAKE_CURRENT_SOURCE_DIR}/api/libsodium/lib/libsodium.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/raylib/lib/libraylib.a"
    opengl32
    gdi32
    winmm
    advapi32
    user32
    shell32
)

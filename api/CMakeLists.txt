project(api CXX)

add_library(vault_backend STATIC 
    vault_backend.cpp
    vault_backend.h
)

target_include_directories(vault_backend PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/json/include
)

target_include_directories(vault_backend PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/libsodium/include
)

target_link_libraries(vault_backend PRIVATE 
    ui_engine
)

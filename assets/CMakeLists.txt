# Этот CMakeLists.txt автоматически копирует файлы шрифтов и картинок из папки assets 
# прямо в папку сборки (build), туда где лежит твой готовый RaylibMicroUIApp.exe.
# Благодаря этому программа всегда будет видеть шрифт font.ttf при запуске!

cmake_minimum_required(VERSION 3.12)
project(VaultAssets NONE)

# Находим все файлы в текущей папке assets
file(GLOB ASSET_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ico"
)

# Переносим их в папку, где собирается твой исполняемый файл .exe
foreach(ASSET_PATH ${ASSET_FILES})
    get_filename_component(ASSET_NAME ${ASSET_PATH} NAME)
    
    # Говорим CMake скопировать файл, если его еще нет в папке сборки или он изменился
    configure_file(
        ${ASSET_PATH}
        ${CMAKE_BINARY_DIR}/${ASSET_NAME}
        COPYONLY
    )
endforeach()

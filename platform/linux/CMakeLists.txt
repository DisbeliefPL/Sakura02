cmake_minimum_required(VERSION 3.22.1)
project(sakura_linux LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ncnn_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ncnn_linux/lib/cmake/ncnn/")
set(NCNN_INC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ncnn_linux/include/")
find_package(ncnn REQUIRED)

find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLESv2 REQUIRED glesv2)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../core core_build)

target_include_directories(core PUBLIC ${NCNN_INC})

add_executable(sakura src/main.cpp src/display_x11.cpp)
target_link_libraries(sakura PRIVATE ncnn core EGL GLESv2 X11)
target_include_directories(sakura PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NCNN_INC}
    ${CLI11_INC}
)

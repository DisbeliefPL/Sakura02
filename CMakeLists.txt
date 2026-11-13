cmake_minimum_required(VERSION 3.10)
project(sakura02 LANGUAGES CXX)

# 如果没有指定构建类型，默认使用 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 针对 GCC / Clang 开启极限优化
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ncnn_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ncnn-install/lib/cmake/ncnn/")
set(NCNN_INC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ncnn-install/include/")
set(CLI11_INC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/CLI/")
find_package(ncnn REQUIRED)


find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLESv2 REQUIRED glesv2)

add_library(s_core STATIC src/engine.cpp)
target_include_directories(s_core PUBLIC include)
target_include_directories(s_core PRIVATE ${NCNN_INC})
target_link_libraries(s_core PUBLIC ncnn)

add_library(draw STATIC src/display.cpp)
target_include_directories(draw PUBLIC include)
target_link_libraries(draw PUBLIC X11 EGL GLESv2)

add_executable(sakura src/main.cpp)
target_link_libraries(sakura PRIVATE ncnn s_core draw)
target_include_directories(sakura PRIVATE ${NCNN_INC})
target_include_directories(sakura PRIVATE ${CLI11_INC})

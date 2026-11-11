cmake_minimum_required(VERSION 3.10)
project(sakura02 LANGUAGES CXX)

# 如果没有指定构建类型，默认使用 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 针对 GCC / Clang 开启极限优化
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # -O3 是最高级别优化，-DNDEBUG 会关掉 assert 检查
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ncnn_DIR "~/ncnn-install/lib/cmake/ncnn/")
set(NCNN_INC "~/ncnn-install/include/")
find_package(ncnn REQUIRED)

add_library(s_core STATIC src/engine.cpp)
target_include_directories(s_core PUBLIC include)
target_include_directories(s_core PRIVATE ${NCNN_INC})
target_link_libraries(s_core PUBLIC ncnn)

add_executable(sakura src/main.cpp)
target_link_libraries(sakura PRIVATE ncnn s_core)
target_include_directories(sakura PRIVATE ${NCNN_INC})

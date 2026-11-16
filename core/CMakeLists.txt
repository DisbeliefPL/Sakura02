cmake_minimum_required(VERSION 3.22.1)
project(Core)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(GLOB_RECURSE CORE_SOURCES "src/*.cpp")

#编译静态库
add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

#一定要在对应平台的cmake里为core链接ncnn!
#一定要在对应平台的cmake里为core链接ncnn!
#一定要在对应平台的cmake里为core链接ncnn!

#线程专属链接包
find_package(Threads REQUIRED)
target_link_libraries(core PUBLIC Threads::Threads)

#直接让cmake读取着色器代码,就不用单独搞文件读取函数了
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert" VERTEX_SHADER_CONTENT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag" FRAGMENT_SHADER_CONTENT)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/shaders_data.h"
)

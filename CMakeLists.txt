cmake_minimum_required(VERSION 3.10)
project(sakura02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory(core)

#android有gradle总控,所以不用管
if(NOT ANDROID)
    add_subdirectory(platform/linux)
endif()

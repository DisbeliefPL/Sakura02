# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project("sakura_android")
set(CMAKE_BUILD_TYPE "Release")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../)

add_subdirectory(${ROOT_DIR}/core core_bin)

target_link_libraries(core PUBLIC ${NCNN_LIB_PATH})

set(ncnn_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ncnn/arm64-v8a/lib/cmake/ncnn)
find_package(ncnn REQUIRED)

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        src/native-lib.cpp
        src/display_android.cpp
)

#一定要在对应平台的cmake里为core链接ncnn!
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ncnn/arm64-v8a/include)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ncnn/arm64-v8a/include
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        ncnn
        core
        android
        log
        EGL
        GLESv2
        z
)


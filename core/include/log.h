#pragma once


#define ENABLE_LOG 1
/*
#ifdef __ANDROID__
    #include <android/log.h>
    #define LOG_TAG "Sakura"
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
    #if ENABLE_LOG
        #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
        #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
    #else
        #define LOGD(...)
        #define LOGI(...)
    #endif
    */
//#else
    #include <cstdio>
    
    #define LOGE(...) \
        do { \
            std::fprintf(stderr, "\033[31m[ERROR] " __VA_ARGS__); \
            std::fprintf(stderr, "\033[0m\n"); \
        } while(0)
    #if ENABLE_LOG
        #define LOGD(...) \
        do { \
            std::fprintf(stdout, "\033[33m[DEBUG] " __VA_ARGS__); \
            std::fprintf(stdout, "\033[0m\n"); \
        } while(0)
        #define LOGI(...)  \
        do { \
            std::fprintf(stdout, "[INFO] " __VA_ARGS__); \
            std::fprintf(stdout, "\n"); \
        } while(0)
    #else
        #define LOGI(...)
        #define LOGD(...)
    #endif
//#endif

    

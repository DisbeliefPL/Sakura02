#include <jni.h>
#include <android/native_window_jni.h>
#include <memory>
#include <string>
#include "display_android.h"
#include "display.h"
#include "engine.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

std::shared_ptr<Renderer> g_renderer = nullptr;
Engine en(128,16,1,1,1,1);

std::string jstr2str(JNIEnv *env, jstring jStr)
{
    if (!jStr) return "";
    const char *chars = env->GetStringUTFChars(jStr, nullptr);
    std::string str(chars);
    env->ReleaseStringUTFChars(jStr, chars);
    return str;
}

extern "C" JNIEXPORT void JNICALL
Java_com_papyrus_sakura_MainActivity_loadModel(JNIEnv* env, jobject, jstring paramPath, jstring binPath)
{
    en.init(jstr2str(env, paramPath), jstr2str(env, binPath));
}

//窗口初始化层
extern "C" JNIEXPORT void JNICALL
Java_com_papyrus_sakura_MainActivity_setSurface(JNIEnv* env, jobject/*this*/, jobject surface)
{
    if (surface == nullptr) {
        if (g_renderer) {
            g_renderer->Destroy();
            g_renderer.reset();
        }
        return;
    }

    if (!g_renderer)
    {
        g_renderer = std::make_shared<AndroidRenderer>();
    }

    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);

    g_renderer->Init(window);

    //C++从此接管窗口
    ANativeWindow_release(window);
}

extern "C" JNIEXPORT void JNICALL
Java_com_papyrus_sakura_MainActivity_drawPic(JNIEnv *env, jobject, jstring imgPath)
{
    const char* img_path = env->GetStringUTFChars(imgPath, nullptr);

    int width, height, channels;
    unsigned char *img_data = stbi_load(img_path, &width, &height, &channels, 3);

    if (!img_data)
    {
        env->ReleaseStringUTFChars(imgPath, img_path);
        return;
    }
    int scale = 4;
    int out_w = width * scale;
    int out_h = height * scale;
    std::unique_ptr<unsigned char[]> out_data(new unsigned char[out_w*out_h*3]);

    en.process(img_data, width, height, out_data.get(), out_w, out_h, scale);

    g_renderer->DrawWindow(out_data.get(), out_w, out_h);

    stbi_image_free(img_data);
    env->ReleaseStringUTFChars(imgPath, img_path);
}

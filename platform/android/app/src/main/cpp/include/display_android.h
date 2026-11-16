#pragma once

#include "display.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>

class AndroidRenderer : public Renderer
{
private:
    ANativeWindow *window = nullptr;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    GLuint program = 0;
    GLuint textureId = 0;
public:
    bool Init(void *) override;
    bool DrawWindow(const unsigned char *, int, int) override;
    void Destroy() override;
};


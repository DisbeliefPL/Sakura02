#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "display.h"

class LinuxRenderer : public Renderer
{
public:
    LinuxRenderer(int w, int h):win_w(w), win_h(h) { }
    bool Init(void *) override;
    bool DrawWindow(const unsigned char *, int, int) override;
    void Destroy() override;
private:
    Display *dp = nullptr;
    Window root = None;
    Window win = None;
    GLuint program = EGL_NONE;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    int win_w = 0;
    int win_h = 0;
};

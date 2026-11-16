#include "display_android.h"
#include "shaders_data.h"
#include "log.h"
#include <EGL/eglplatform.h>

bool AndroidRenderer::Init(void *nativeWindow)
{
    window = static_cast<ANativeWindow*>(nativeWindow);
    if (!window)
    {
        LOGE("window加载失败");
        return false;
    }

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!display)
    {
        LOGE("display加载失败");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor))
    {
        LOGE("egl初始化失败");
        return false;
    }


    EGLint cfg_attribs[] =
            {
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
                    EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                    EGL_NONE
            };
    EGLConfig config;
    EGLint cfg_cnt;
    if (!eglChooseConfig(display, cfg_attribs,&config, 1,&cfg_cnt))
    {
        LOGE("egl配置加载失败");
        return false;
    }

    EGLint ctx_attribs[] ={ EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (context == EGL_NO_CONTEXT)
    {
        LOGE("egl上下文加载失败");
        return false;
    }

    surface = eglCreateWindowSurface(display, config, reinterpret_cast<EGLNativeWindowType>(window), NULL);
    if (surface == EGL_NO_SURFACE)
    {
        LOGE("surface加载失败");
        return false;
    }

    //必须先绑定上下文再使用着色器! 不然上下文中断
    if (!eglMakeCurrent(display, surface, surface, context))
    {
        LOGE("egl上下文绑定失败");
        return false;
    }

    LOGI("Opengl es版本: %s", glGetString(GL_VERSION));

    const char* vertexSrc = VERTEX_SHADER_SRC;
    const char* fragSrc = FRAGMENT_SHADER_SRC;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, NULL);
    glCompileShader(vs);

    GLint vs_success;
    GLchar vs_log[1024];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &vs_success);
    if (!vs_success) {
        glGetShaderInfoLog(vs, 1024, NULL, vs_log);
        LOGE("顶点 Shader 编译失败: %s", vs_log);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragSrc, NULL);
    glCompileShader(fs);

    GLint fs_success;
    GLchar fs_log[1024];
    glGetShaderiv(fs, GL_COMPILE_STATUS, &fs_success);
    if (!fs_success) {
        glGetShaderInfoLog(fs, 1024, NULL, fs_log);
        LOGE("片元 Shader 编译失败: %s", fs_log);
    }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    GLchar infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        LOGE("Shader Program 链接失败: %s", infoLog);
    }
    return true;
}

bool AndroidRenderer::DrawWindow(const unsigned char *data, int img_w, int img_h)
{
    int win_w = ANativeWindow_getWidth(window);
    int win_h = ANativeWindow_getHeight(window);

    glUseProgram(program);

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //取消gl的默认4字节对齐 逐一字节读取(因为超分和stb_image数据流都是rgb三通道)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_w, img_h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    //使用Letterboxing 缩放方案
    float win_aspect = (float)win_w/win_h;
    float img_aspect = (float)img_w/img_h;
    float x_scale = 1.0f;
    float y_scale = 1.0f;

    //图片宽了就填满宽度,上下留黑边:1.0 y_scale
    if (img_aspect > win_aspect)
    {
        y_scale = win_aspect/img_aspect;
    }
    else //窗口宽了就填满高度,左右留黑边:x_scale 1.0
    {
        x_scale = img_aspect/win_aspect;
    }

    float vertices[] =
            {
                    -1.0f * x_scale, 1.0f * y_scale, //左上
                    -1.0f * x_scale, -1.0f * y_scale, //左下
                    1.0f * x_scale, 1.0f * y_scale, //右上
                    1.0f * x_scale, -1.0f * y_scale //右下
            };

    float texCoords[] =
            {
                    0.0f, 0.0f, //左上
                    0.0f, 1.0f, //左下
                    1.0f, 0.0f, //右上
                    1.0f, 1.0f //右下
            };

    /*
     * 标准流程:
     * 创建并绑定缓冲区->填入坐标数组->获取着色器attribute->启用attribute数组->用pointer告诉gpu绘制方法
     */

    //存顶点坐标
    GLuint vbo_pos;
    glGenBuffers(1, &vbo_pos);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    GLint pos = glGetAttribLocation(program, "a_Position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 0, 0);

    //存纹理坐标
    GLuint vbo_tex;
    glGenBuffers(1, &vbo_tex);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    GLint tex_pos = glGetAttribLocation(program, "a_TexCoord");
    glEnableVertexAttribArray(tex_pos);
    glVertexAttribPointer(tex_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glViewport(0, 0, win_w, win_h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(display, surface);

    return true;
}

void AndroidRenderer::Destroy()
{
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
    }
    if (window)
    {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

#include "display.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
using namespace std::chrono;

std::string read_shader(const char *file)
{
    std::ifstream in(file);
    if (!in.is_open())
    {
        std::cerr << "读取着色器文件失败" << std::endl;
        return "";
    }
    std::stringstream ss;
    std::string s;
    ss << in.rdbuf(); //整文件读取
    return ss.str();
}

bool DrawWindow(const unsigned char *data, int win_w, int win_h, int w, int h)
{
    Display *dp = XOpenDisplay(NULL);
    if (!dp)
    {
        std::cerr << "启动X11失败,检查是否开启X11服务" << std::endl;
        return false;
    }
    Window root = DefaultRootWindow(dp);
    Window win = XCreateSimpleWindow(dp, root, 0, 0, win_w, win_h, 0, 0, 0);
    XMapWindow(dp, win);
    XFlush(dp);

    auto d_s = steady_clock::now();
    EGLDisplay display = eglGetDisplay
        ((EGLNativeDisplayType)dp);
    EGLint major, minor;
    eglInitialize(display, &major, &minor);

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
    eglChooseConfig(display, cfg_attribs, 
            &config, 1,&cfg_cnt);

    EGLint ctx_attribs[] = 
    { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, NULL);
    eglMakeCurrent(display, surface, surface, context);


    std::string vtx_s = read_shader("../shader/shader.vert");
    std::string frg_s = read_shader("../shader/shader.frag");

    const char *vertexSrc = vtx_s.c_str();
    const char *fragSrc = frg_s.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragSrc, NULL);
    glCompileShader(fs);

    //创建程序
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glUseProgram(program);

    //生成纹理
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    //使用线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);


    //使用Letterboxing 缩放方案
    float win_aspect = (float)win_w/win_h;
    float img_aspect = (float)w/h;
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

    //顶点坐标
    float vertices[] = 
    {
        -1.0f * x_scale, 1.0f * y_scale, //左上
        -1.0f * x_scale, -1.0f * y_scale, //左下
        1.0f * x_scale, 1.0f * y_scale, //右上
        1.0f * x_scale, -1.0f * y_scale //右下
    };

    //纹理坐标
    float texCoords[] =
    {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    //vbo:VertexBufferObject 顶点缓冲对象 存顶点坐标
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

    //清屏准备放纹理
    glViewport(0, 0, 1280, 720);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //启用绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(display, surface);
    auto d_e = steady_clock::now();
    auto d_t = duration_cast<milliseconds>(d_e-d_s).count();
    std::cout << "超分图像已经生成在X11界面上,用时: " << d_t << "ms" << std::endl
        << "按任意键以停止生成" << std::endl;
    std::cin.get();

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
    XDestroyWindow(dp, win);
    XCloseDisplay(dp);
    return true;
}

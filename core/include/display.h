#pragma once

//接口不允许有平台私有头文件 必须纯虚基类

//渲染基类
class Renderer
{
public:
    //根据安卓窗口生命周期顺序 不应该使用构造函数传入窗口数据
    virtual bool Init(void *) = 0;
    virtual bool DrawWindow(const unsigned char *, int, int) = 0;
    virtual void Destroy() = 0;
    virtual ~Renderer() = default;
};




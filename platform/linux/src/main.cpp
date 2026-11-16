#include "CLI/CLI.hpp"
#include "engine.h"
#include "display_x11.h"
#include "log.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string>

using namespace std;
using namespace std::chrono;


int main(int argc, char **argv)
{
    //我使用cli11实现更加现代化的命令行参数管理
    CLI::App app{"Sakura_02(夜乃樱) 基于ncnn的图片超分辨率工具"};

    int model = 0;
    int tile_size = 128;
    int overlap = 16;
    int ncnn_threads = 1;
    int threads = 1;
    int pic_fmt = 0;
    bool vulkan = false;
    bool fp16 = false;
    bool render = false;
    std::string input_file;
    std::string output_file;

    app.add_option("-m,--model", model, "选择模型(默认是0)\n0:x4plus-anime\n1.x4plus\n2.animevideov3-x4");
    app.add_option("--tile", tile_size, "每线程处理的图像块大小");
    app.add_option("--overlap", overlap, "重叠区大小\n--以上两个参数请根据原图大小合理赋值");
    app.add_option("--ncnn-threads", ncnn_threads, "使用ncnn自带的线程的数量");
    app.add_option("--threads", threads, "使用外置线程的数量\n--以上两种线程的数量请根据cpu性能合理分配");
    app.add_flag("-v,--enable-vulkan", vulkan, "启用vulkan加速");
    app.add_flag("-f,--enable-fp16", fp16, "启用fp16加速");
    app.add_flag("-r,--render", render, "是否绘制在屏幕上(需要X11支持)");
    app.add_option("-i,--input", input_file, "输入的图片路径")->required();
    app.add_option("-p,--picture", pic_fmt, "输出的图片格式(默认0)\n0:png\n1:jpg");
    app.add_option("-o,--output", output_file, "输出的图片路径")->required();

    //手动拦截CLI11解析错误(CLI11_PARSE宏不好使了)
    for (int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            LOGI("%s", app.help().c_str());
            return 0;
        }
    }
    try
    {
        app.parse(argc, argv);
    }
    catch(const CLI::ParseError &e)
    {
        LOGE("参数错误: %s", e.what());
        return 1;
    }
    if (input_file.empty())
    {
        LOGE("错误: --input与--output不能为空");
        return 1;
    }

    //块 重叠 ncnn线程 外置线程 vulkan加速 fp16加速
    Engine en(tile_size, overlap, ncnn_threads, threads, vulkan, fp16);
    LinuxRenderer ren(1280, 720);
    auto l_s = steady_clock::now();
    bool load = false;

    switch (model)
    {
        case 0:
            load = en.init("../../../models/realesrgan-x4plus-anime.param", 
                "../../../models/realesrgan-x4plus-anime.bin");
                break;
        case 1:
            load = en.init("../../../models/realesrgan-x4plus.param", 
                "../../../models/realesrgan-x4plus.bin");
                break;
        case 2:
            load = en.init("../../../models/realesr-animevideov3-x4.param", 
                "../../../models/realesrgan-animevideov3-x4.bin");
                break;
        default:
                LOGE("无效模型参数");
    }
    auto l_e = steady_clock::now();
    if (!load) return -1;
    auto l_t = duration_cast<milliseconds>(l_e-l_s).count();
    LOGI("模型加载用时: %lldms", l_t);

    int scale = 4;
    
    auto i_s = steady_clock::now();
    
    int img_w, img_h, channel;
    const unsigned char *img = stbi_load(input_file.c_str(), &img_w, &img_h, &channel, 3);

    auto i_e = steady_clock::now();
    auto i_t = duration_cast<milliseconds>(i_e-i_s).count();
    LOGI("图片加载用时: %lldms", i_t);

    if (img == nullptr)
    {
        LOGE("读取图片失败");
        return -2;
    }
    LOGI("原图尺寸: %d*%d", img_w, img_h);
    int out_w = img_w * scale;
    int out_h = img_h * scale;

    //提前开大数组
    unique_ptr<unsigned char[]> out_data(new unsigned char[out_w*out_h*3]);
    auto p_s = steady_clock::now();
    if (en.process(img, img_w, img_h, out_data.get(), out_w, out_h, scale))
    {
        auto p_e = steady_clock::now();
        auto p_t = duration_cast<milliseconds>(p_e-p_s).count();
        LOGI("图像处理用时: %lldms", p_t);

        auto w_s = steady_clock::now();
        if (!pic_fmt)
        {
            //注意png最后是out_w*3而不是和jpg一样的100,否则会图片会花
            stbi_write_png(output_file.c_str(), out_w, out_h, 3, out_data.get(), out_w*3);
        }
        else 
        {
            stbi_write_jpg(output_file.c_str(), out_w, out_h, 3, out_data.get(), 100);
        }
        auto w_e = steady_clock::now();
        auto w_t = duration_cast<milliseconds>(w_e-w_s).count();
        LOGI("写入图片用时: %lldms", w_t);

        LOGI("处理完成! 输出图大小为 %d*%d", out_w, out_h);
        if (render)
        {
            LOGI("按任意键以在X11输出绘制流");
            cin.get();
            ren.Init(nullptr);
            ren.DrawWindow((unsigned char *)out_data.get(), out_w, out_h);
            cin.get();
            ren.Destroy();
        }
        stbi_image_free((void*)img);
    }
    return 0;
}

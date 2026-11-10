#include <iostream>
#include <simpleocv.h>
#include "engine.h"
#include <chrono>
using namespace std;
using namespace std::chrono;


int main(int argc, char **argv)
{
    Engine en(256, 32);
    auto l_s = steady_clock::now();
    bool load = en.init("../models/realesrgan-x4plus-anime.param", 
            "../models/realesrgan-x4plus-anime.bin", 8);
    auto l_e = steady_clock::now();
    if (!load) return -1;
    auto l_t = duration_cast<milliseconds>(l_e-l_s).count();
    cout << "模型加载用时: " << l_t << "ms" << endl;

    int scale = 4;
    
    auto i_s = steady_clock::now();
    cv::Mat img = cv::imread(argv[1]);
    auto i_e = steady_clock::now();
    auto i_t = duration_cast<milliseconds>(i_e-i_s).count();
    cout << "图片加载用时: " << i_t << "ms" << endl;

    if (img.empty())
    {
        std::cerr << "读取图片失败" << std::endl;
        return -2;
    }
    else
    {
        std::cout << "原图尺寸:" << img.cols << "*"
            << img.rows << std::endl;
    }
    int out_w = img.cols * scale;
    int out_h = img.rows * scale;

    //提前开大数组
    unsigned char *out_data = new unsigned char[out_w * out_h *3];
    auto p_s = steady_clock::now();
    if (en.process(img.data, img.cols, img.rows, out_data, out_w, out_h, scale))
    {
        auto p_e = steady_clock::now();
        auto p_t = duration_cast<milliseconds>(p_e-p_s).count();
        cout << "处理用时: " << p_t << "ms" << endl;

        auto w_s = steady_clock::now();
        cv::Mat out_img(out_h, out_w, CV_8UC3, out_data);
        cv::imwrite(argv[2], out_img);
        auto w_e = steady_clock::now();
        auto w_t = duration_cast<milliseconds>(w_e-w_s).count();
        cout << "写入图片用时: " << w_t << "ms" << endl;

        std::cout << "处理完成! " << "输出图为" << out_w
            << "*" << out_h << std::endl;
    }
    delete []out_data;
    return 0;
}

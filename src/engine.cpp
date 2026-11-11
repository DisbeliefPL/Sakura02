#include "engine.h"
#include <algorithm>
#include <iostream>
#include <net.h>
#include <vector>
#include <thread>
#include <atomic>

bool Engine::init(const std::string &param_path, 
         const std::string &bin_path)
{
    net.opt.num_threads = set_ncnn_threads;
    net.opt.use_vulkan_compute = is_vulkan;
    net.opt.use_fp16_arithmetic = is_fp16;
    net.opt.use_fp16_packed = is_fp16;
    net.opt.use_fp16_storage = is_fp16;

    net.opt.use_packing_layout = false;
    net.opt.lightmode = true; //回收利用内存

    if(net.load_param(param_path.c_str()) != 0 || net.load_model(bin_path.c_str()) != 0)
    {
        std::cerr << "读取模型出现错误" << std::endl;
        return false;
    }
    is_load = true;
    return true;
}

bool Engine::process(const unsigned char* in_data, int in_w, 
        int in_h, unsigned char* out_data, int &out_w, int &out_h, int scale)
{
    if (!is_load) return false;

    out_w = in_w * scale;
    out_h = in_h * scale;

    //整数向上取整公式:(A+B-1)/B 计算切成几块
    int x_tile_cnt = (in_w+TILE_SIZE-1)/TILE_SIZE;
    int y_tile_cnt = (in_h+TILE_SIZE-1)/TILE_SIZE;
    int total_tiles = x_tile_cnt*y_tile_cnt;

    std::atomic<int> task(0);

    auto work = [&]()
    {
        ncnn::Mat out_tile;
        //加载多线程经典做法:死循环
        while (true)
        {
            int tile_id = task.fetch_add(1);
            if (tile_id >= total_tiles) break;

            //计算每块的左上角坐标
            //(本质是二维数组压缩为一维)
            //这两个式子抓住了块id和线程id对应的关系
            int x = (tile_id % x_tile_cnt) * TILE_SIZE;
            int y = (tile_id / x_tile_cnt) * TILE_SIZE;

            int x_start = std::max(0, x-OVERLAP);
            int y_start = std::max(0, y-OVERLAP);
            int x_end = std::min(in_w, x+TILE_SIZE+OVERLAP);
            int y_end = std::min(in_h, y+TILE_SIZE+OVERLAP);

            int tile_w = x_end - x_start;
            int tile_h = y_end - y_start;

            ncnn::Mat in_tile(tile_w, tile_h, 3);


            //进入块坐标系
            for (int c = 0; c < 3; c++)
            {
                float* ptr = in_tile.channel(c);
                for (int row = 0; row < tile_h; row++)
                {
                    for (int col = 0; col < tile_w; col++)
                    {
                        ptr[row*tile_w + col] = 
                        in_data[((row+y_start)*in_w+(col+x_start))*3+(2-c)]*(1.0f/255.0f);
                    }
                }
            }
           
            //***Extractor必须要在线程里面定义,否则多线程共用产生垃圾数据
            //是核心处理函数,所以一定是线程私有
            ncnn::Extractor ex = net.create_extractor();
            ex.input("data", in_tile); 
            

            if (ex.extract("output", out_tile) != 0 || out_tile.empty())
            {
                continue;
            }

            //R G B提取
            const float* c0 = out_tile.channel(0);
            const float* c1 = out_tile.channel(1);
            const float* c2 = out_tile.channel(2);

            //框定输出后的每个区块 
            int x_core_start = x * scale;
            int x_core_end = std::min(in_w, x + TILE_SIZE) * scale;
            int y_core_start = y * scale;
            int y_core_end = std::min(in_h, y + TILE_SIZE) * scale;


            //把提取的数据存入
            for (int row = 0; row < out_tile.h; row++)
            {
                for (int col = 0; col < out_tile.w; col++)
                {
                    int x_out = x_start*scale + col;
                    int y_out = y_start*scale + row;
                    if (x_out >= x_core_start && x_out < x_core_end && 
                        y_out >= y_core_start && y_out < y_core_end)
                    {
                        int tile_idx = row*out_tile.w + col;
                        if (tile_idx >= out_tile.w * out_tile.h) continue;

                        //模型输入边界[0.0,1.0],因此要放大155.0倍
                        float r = std::max(0.0f, std::min(255.0f, c0[tile_idx]*255.0f));

                        float g = std::max(0.0f, std::min(255.0f, c1[tile_idx]*255.0f));

                        float b = std::max(0.0f, std::min(255.0f, c2[tile_idx]*255.0f));
                        //每三个元素一个像素
                        int out_idx = (y_out*out_w + x_out)*3;
                        //最后转为BGR
                        out_data[out_idx+2] = (unsigned char)r;
                        out_data[out_idx+1] = (unsigned char)g;
                        out_data[out_idx] = (unsigned char)b;
                    }
                }
            }
        }
    };
    std::vector<std::thread> threads;
    for (int i = 0; i < set_threads; i++)
    {
        //线程不可拷贝,所以不用push_back
        threads.emplace_back(work);
    }

    for (auto &t : threads)
    {
        t.join();
    }
    return true;
}

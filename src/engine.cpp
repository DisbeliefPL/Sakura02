#include "engine.h"

bool Engine::init(const std::string &param_path, 
         const std::string &bin_path, int set_threads = 4)
{
    net.opt.num_threads = set_threads;
    net.opt.use_vulkan_compute = false;
    net.opt.use_fp16_arithmetic = true;
    net.opt.use_fp16_packed = true;
    net.opt.use_fp16_storage = true;
    net.opt.use_packing_layout = false;
    net.opt.lightmode = true;

    if(net.load_param(param_path.c_str()) != 0 || net.load_model(bin_path.c_str()) != 0)
    {
        std::cerr << "读取模型出现错误" << std::endl;
        return false;
    }
    is_load = true;
    return true;
}

bool Engine::process(const unsigned char* in_data, int in_w, 
        int in_h, unsigned char* out_data, int out_w, int out_h, int scale)
{
    if (!is_load) return false;

    out_w = in_w * scale;
    out_h = in_h * scale;

    for (int y = 0; y < in_h; y += TILE_SIZE)
    {
        for (int x = 0; x < in_w; x += TILE_SIZE)
        {
            int x_start = std::max(0, x-OVERLAP);
            int y_start = std::max(0, y-OVERLAP);
            int x_end = std::min(in_w, x+TILE_SIZE+OVERLAP);
            int y_end = std::min(in_h, y+TILE_SIZE+OVERLAP);

            int tile_w = x_end - x_start;
            int tile_h = y_end - y_start;

            //ncnn开始处理切块
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

            ncnn::Extractor ex = net.create_extractor();
            ex.input("data", in_tile);
            ncnn::Mat out_tile;
            ex.extract("output", out_tile);
            
            //R G B提取
            const float* c0 = out_tile.channel(0);
            const float* c1 = out_tile.channel(1);
            const float* c2 = out_tile.channel(2);

            for (int row = 0; row < (y_end-y_start)*scale; row++)
            {
                for (int col = 0; col < (x_end-x_start)*scale; col++)
                {
                    int x_out = x_start*scale + col;
                    int y_out = y_start*scale + row;
                    if (x_out>x && x_out<(x+TILE_SIZE)*scale 
                        && y_out>y && y_out<(y+TILE_SIZE)*scale)
                    {
                        int tile_idx = row*out_tile.w + col;

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
    }
    return true;
}

#include <algorithm>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include "engine.h"
#include "log.h"

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
        LOGE("读取模型出现错误");
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
        std::unique_ptr<unsigned char[]>
                out_tiles(new unsigned char[(TILE_SIZE+2*OVERLAP)*scale*(TILE_SIZE+2*OVERLAP)*scale*3]);

        //加载多线程经典做法:死循环
        while (true)
        {
            LOGD("成功进入线程任务");
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

            ncnn::Mat in_tile = ncnn::Mat::from_pixels_roi(in_data, ncnn::Mat::PIXEL_RGB,
            in_w, in_h, in_w*3 ,x_start, y_start, tile_w, tile_h);

            if (in_tile.empty() || in_tile.data == nullptr)
            {
                LOGE("intile是空的,tile_w: %d, tile_h: %d", tile_w, tile_h);
                continue;
            }

            float mean[] = {0.0f, 0.0f, 0.0f};
            float norm[] = {1.0f/255.0f, 1.0f/255.0f, 1.0f/255.0f};
            //本质:(raw-mean)*norm
            in_tile.substract_mean_normalize(mean, norm);
           
            //***Extractor必须要在线程里面定义,否则多线程共用产生垃圾数据
            //是核心处理函数,所以一定是线程私有
            ncnn::Extractor ex = net.create_extractor();
            ex.input("data", in_tile);
            int ret = ex.extract("output", out_tile);
            if (ret != 0 || out_tile.empty() || out_tile.data == nullptr) {
                LOGE("out_tile提取失败");
                continue;
            }

            int out_tile_w = out_tile.w;
            int out_tile_h = out_tile.h;

            float norm_out[] = {255.0f, 255.0f, 255.0f};
            out_tile.substract_mean_normalize(mean, norm_out);

            //***这里就是把通道优先转换为像素优先了!!!!!!!!
            out_tile.to_pixels(out_tiles.get(), ncnn::Mat::PIXEL_RGB);

            //框定超分后的核心区
            int x_core_start = x * scale;
            int x_core_end = std::min(in_w, x + TILE_SIZE) * scale;
            int y_core_start = y * scale;
            int y_core_end = std::min(in_h, y + TILE_SIZE) * scale;
            int copy_w = x_core_end - x_core_start;
            int copy_h = y_core_end - y_core_start;

            //重叠区大小
            int x_offset = x_core_start - x_start*scale;
            int y_offset = y_core_start - y_start*scale;

            /*核心拼接逻辑:
             * 认识1:图像数据是二维数组压缩的一维数组 每三个通道确定一个像素 所以跨行遍历是核心
             * 认识2:ncnn是[rrr...][ggg...][bbb...]的排布模式(通道优先) 而stb是[rgb][rgb][rgb]...(像素优先)
             * 认识3:memcpy是从首地址开始按指定字节数截取
             * 基于此,像素映射策略就是跨行遍历,每次截取一行的通道信息
             * 解释:用offset跳过重叠区.然后逐行读取,再映射到src_ptr
             */
            for (int row = 0; row < copy_h; row++)
            {
                unsigned char *src_ptr = out_tiles.get() +
                    ((row+y_offset)*out_tile_w + x_offset)*3;
                unsigned char *dst_ptr = out_data +
                    ((row+y_core_start)*out_w + x_core_start)*3;
                int row_bytes = copy_w*3;
                memcpy(dst_ptr, src_ptr, row_bytes);
            }
            LOGD("成功结束线程任务");
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

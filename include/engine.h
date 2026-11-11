#include <ncnn/net.h>
#include <string>
#include <ncnn/simpleocv.h>

class Engine
{
public:
    Engine(int ts, int rpt, int n_th, int th, bool v, bool fp16):TILE_SIZE(ts), OVERLAP(rpt), set_ncnn_threads(n_th),
    is_vulkan(v), is_fp16(fp16), set_threads(th){ }
    ~Engine() {};

    bool init(const std::string &param_path, const std::string &bin_path);
    bool process(const unsigned char* in_data, int in_w, int in_h, 
            unsigned char* out_data, int &out_w, int &out_h, int scale);
private:
    ncnn::Net net;
    int set_ncnn_threads = 4;
    int set_threads = 4;
    bool is_vulkan = false;
    bool is_fp16 = false;
    bool is_load = false;
    int TILE_SIZE = 256;
    int OVERLAP = 32;
};

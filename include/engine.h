#include <ncnn/net.h>
#include <string>
#include <iostream>
#include <ncnn/simpleocv.h>
#include <algorithm>

class Engine
{
public:
    Engine(int ts, int rpt):TILE_SIZE(ts), OVERLAP(rpt) { }
    ~Engine() {};

    bool init(const std::string &param_path, const std::string &bin_path,
            int set_threads);
    bool process(const unsigned char* in_data, int in_w, int in_h, 
            unsigned char* out_data, int out_w, int out_h, int scale);
private:
    ncnn::Net net;
    bool is_load = false;
    int TILE_SIZE = 256;
    int OVERLAP = 32;
};

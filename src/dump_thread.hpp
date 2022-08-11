#pragma once
#include <iostream>
#include <fstream>
#include "base_thread.hpp"
#include "global_frame_data.hpp"
#include "color_log.hpp"
#include "global_camera_info.hpp"
class dump_thread final : public base_thread {
    virtual void process() override {
        char *data = nullptr;
        size_t len = 0;
        while (true) {
            len = G_FRAME_DATA.get(&data);
            if (len > 0) {
                dump_image(data, len);
                delete data;
                data = nullptr;
            }
        }
    }
public:
    dump_thread(int n) : dump_limit(n) {
    }
private:
    void dump_image(const char *image, size_t len) {
        if (dump_count >= dump_limit) {
            return;
        }
        ++dump_count;
        char path[64] = "";
        snprintf(path, sizeof(path), "/usr/tmp/UYVY_%d_%dx%d.uyvy", dump_count, G_CAMERA_INFO.camera_width, G_CAMERA_INFO.camera_height);
        std::ofstream  ofs;
        ofs.open(path, std::ios::in | std::ios::trunc);
        if (!ofs.is_open()) {
            LOG_E("can not open file:%s", path);
            return;
        }
        ofs.write(image, len);
        ofs.close();
        LOG_D("dump image:%s,count:%d success!", path, dump_count);
    }
private:
    int dump_count = 0;
    const int dump_limit = 10;
};

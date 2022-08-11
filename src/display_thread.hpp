#pragma once
#include <iostream>
#include "base_thread.hpp"
#include "global_frame_data.hpp"
#include "qnx_screen_display.h"
#include "color_log.hpp"
#include "global_camera_info.hpp"
class display_thread final : public base_thread {
    virtual void process() override {
        char *data = nullptr;
        size_t len = 0;
        while (true) {
            len = G_FRAME_DATA.get(&data);
            if (len > 0) {
                //std::cout << "I am a dump thread, get frame size:" << len << std::endl;
                display_->display_image(data);
                delete data;
                data = nullptr;
            }
        }
    }
public:
    display_thread() = default;
    virtual ~display_thread() {
        if (display_) {
            delete display_;
            display_ = nullptr;
        }
    }
    bool init() {
        display_ = new qnx_screen_display;
        if (!display_) {
            LOG_E("new qnx_screen_display failed!");
            return false;
        }
        if (!display_->init()) {
            return false;
        }
        display_->set_image_pos(0, 0);
        display_->set_image_size(G_CAMERA_INFO.camera_width, G_CAMERA_INFO.camera_height);
        display_->set_clip_size(G_CAMERA_INFO.camera_width, G_CAMERA_INFO.camera_height);
        int ret = display_->prepare_image();
        if (ret) {
            LOG_E("prepare for image failed, error code:%d", ret);
            return false;
        }
        return true;
    }
private:
    qnx_screen_display *display_ = nullptr;
};

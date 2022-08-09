#pragma once
#include <iostream>
#include "base_thread.hpp"
#include "global_frame_data.hpp"
class dump_thread final : public base_thread {
    virtual void process() override {
        char *data = nullptr;
        size_t len = 0;
        while (true) {
            len = G_FRAME_DATA.get(&data);
            if (len > 0) {
                std::cout << "I am a dump thread, get frame size:" << len << std::endl;
                delete data;
                data = nullptr;
            }
        }
    }
};

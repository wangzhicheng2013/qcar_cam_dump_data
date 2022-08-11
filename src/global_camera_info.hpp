#pragma once
#include "single_instance.hpp"
struct global_camera_info {
    int camera_width = 0;
    int camera_height = 0;
};
#define G_CAMERA_INFO single_instance<global_camera_info>::instance()
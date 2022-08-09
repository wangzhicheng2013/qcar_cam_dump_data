#pragma once
#include <mutex>
#include <condition_variable>
#include "single_instance.hpp"
class global_frame_data {
public:
    void set(const char *data, size_t len) {
        if (!data || !len) {
            return;
        }
        std::unique_lock<std::mutex>lock(mutex_);
        frame_ptr_ = data;
        frame_size_ = len;
        lock.unlock();
        cond_.notify_one();
    }
    size_t get(char **data) {
        std::unique_lock<std::mutex>lock(mutex_);
        while (0 == frame_size_) {
            cond_.wait(lock);
        }
        *data = new char[frame_size_];
        if (nullptr == *data) {
            return 0;
        }
        memcpy(*data, frame_ptr_, frame_size_);
        size_t sz = frame_size_;
        frame_ptr_ = nullptr;
        frame_size_ = 0;
        return sz;
    }
private:
    const char *frame_ptr_ = nullptr;
    size_t frame_size_ = 0;
    std::mutex                mutex_;
    std::condition_variable   cond_;
};
#define G_FRAME_DATA single_instance<global_frame_data>::instance()
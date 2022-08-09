#pragma once
#include <thread>
class base_thread {
public:
	base_thread() = default;
	base_thread(const base_thread &other) {
	}
	virtual ~base_thread()= default;
public:
	void run() {
		thd_ = std::thread([this] {
					this->process();
			});
	}
	void join() {
		if (thd_.joinable()) {
			thd_.join();
		}
	}
protected:
	virtual void process() = 0;
private:
	std::thread thd_;
};
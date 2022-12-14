#include "qcar_cam.hpp"
#include "qnx_car_camera.hpp"
#include "dump_thread.hpp"
void process(const char *img, size_t len) {
	G_FRAME_DATA.set(img, len);
}
int main() {
	qnx_car_camera qnx_camera(6);
	camera_image_process_type camera_image_process(process);
	qnx_camera.set_camera_image_processor(camera_image_process);
	int camera_id;
	std::cout << "camera id:";
	std::cin >> camera_id;
	if (!qnx_camera.init(camera_id)) {
		LOG_I("qnx camera init failed!");
		return -1;
	}
	dump_thread dt(10);
	dt.run();
	dt.join();

	return 0;
}
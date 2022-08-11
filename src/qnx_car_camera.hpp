#pragma once
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <functional>
#include "qcar_cam.hpp"
#include "color_log.hpp"
#include "global_camera_info.hpp"
#define QCARCAM_VER_477
static long QCARCAM_TEST_DEFAULT_GET_FRAME_TIMEOUT = 500000000;
using camera_image_process_type = std::function<void(const char *, size_t)>;
typedef struct qnx_car_camera_map {
    void *handler;
    void *camera;
} qnx_car_camera_map;
qnx_car_camera_map QNX_CAR_CAMERA_MAP = { 0 };
class qnx_car_camera {
public:
    qnx_car_camera(int buf_num) : buf_num_(buf_num) {
        memset(&cam_buffers_, 0, sizeof(cam_buffers_));
    }
    ~qnx_car_camera() {
        if (cam_handle_ != nullptr) {
            qcarcam_stop(cam_handle_);
        	qcarcam_close(cam_handle_);
            cam_handle_ = nullptr;
            memset(&QNX_CAR_CAMERA_MAP, 0, sizeof(QNX_CAR_CAMERA_MAP));
        }
        for (unsigned int i = 0;i < cam_buffers_.n_buffers;i++) {
            if (cam_buffers_.buffers != nullptr && cam_buffers_.buffers[i].planes[0].p_buf != nullptr) {
                pmem_free(cam_buffers_.buffers[i].planes[0].p_buf);
                cam_buffers_.buffers[i].planes[0].p_buf = nullptr;
            }
        }
        if (cam_buffers_.buffers != nullptr) {
            free(cam_buffers_.buffers);
            cam_buffers_.buffers = nullptr;
        }
        if (virtual_address_ != nullptr) {
            delete[] virtual_address_;
            virtual_address_ = nullptr;
        }
        qcar_cam::uninit();
    }
    bool init(int camera_id) {
        if (false == set_camera_id(camera_id)) {
            return false;
        }
        int ret = qcar_cam::init();
        if (ret) {
            LOG_E("qcar cam init failed, errorcode:%d", ret);
            return false;
        }
        if (false == get_camera_info()) {
            return false;
        }
        if (false == open_camera()) {
            return false;
        }
        set_qnx_car_camera_map();
        if (false == set_camera_parameters()) {
            return false;
        }
        ret = qcarcam_start(cam_handle_);
        if (ret != QCARCAM_RET_OK) {
            LOG_E("qcarcam_start failed,errorcode:%d", ret);
            return false;
        }
        LOG_I("camera start success!");
        return true;
	}
    void set_camera_image_processor(camera_image_process_type &processor) {
        camera_image_processor_ = processor;
    }
private:
    inline bool set_camera_id(int camera_id) {
        cam_id_ = (qcarcam_input_desc_t)camera_id;
        if (cam_id_ < 0 || cam_id_ > QCARCAM_INPUT_MAX) {
            return false;
        }
        LOG_I("camera id:%d", cam_id_);
        return true;
    }
    bool open_camera() {
        static const int try_count = 10;
        for (int i = 0;i < try_count;i++) {
            cam_handle_ = qcarcam_open(cam_id_);
            if (cam_handle_ != nullptr) {
                LOG_I("open camera success!");
                return true;
            }
            sleep(1);
            LOG_E("qcarcam_open() failed, try %d again...", i + 1);
        }
        return false;
    }
    bool set_camera_parameters() {
        qcarcam_param_value_t param = { 0 };
        param.ptr_value = (void *)qcarcam_event_cb;
        qcarcam_ret_t ret = qcarcam_s_param(cam_handle_, QCARCAM_PARAM_EVENT_CB, &param);
        if (ret != QCARCAM_RET_OK) {
            LOG_E("qcarcam_s_param for qcarcam_event_cb failed!");
            return false;
        }
        param.uint_value = QCARCAM_EVENT_FRAME_READY | QCARCAM_EVENT_INPUT_SIGNAL | QCARCAM_EVENT_ERROR;
        ret = qcarcam_s_param(cam_handle_, QCARCAM_PARAM_EVENT_MASK, &param);
        if (ret != QCARCAM_RET_OK) {
            LOG_E("qcarcam_s_param for uint_value failed!");
            return false;
        }
        cam_buffers_.n_buffers = buf_num_;
        cam_buffers_.color_fmt = fmt_;
        cam_buffers_.buffers = (qcarcam_buffer_t *)calloc(cam_buffers_.n_buffers, sizeof(qcarcam_buffer_t));
        if (!cam_buffers_.buffers) {
            LOG_E("calloc for cam_buffers_ failed, buf num:%d", buf_num_);
            return false;
        }
        LOG_I("calloc for cam_buffers_ success, n_buffers:%d,color_fmt:%d,flags:%d", cam_buffers_.n_buffers, cam_buffers_.color_fmt, cam_buffers_.flags);
        virtual_address_ = new void*[buf_num_];
        if (!virtual_address_) {
            LOG_E("new for virtual_address_ failed, buf num:%d", buf_num_);
            return false;
        }
        for (unsigned int i = 0;i < buf_num_;i++) {
            cam_buffers_.buffers[i].n_planes = 1;
            cam_buffers_.buffers[i].planes[0].width = width_;
            cam_buffers_.buffers[i].planes[0].height = height_;
            cam_buffers_.buffers[i].planes[0].stride = width_ * 2;      // uyvy
            cam_buffers_.buffers[i].planes[0].size = cam_buffers_.buffers[i].planes[0].stride * cam_buffers_.buffers[i].planes[0].height;
        #ifdef QCARCAM_VER_462
            cam_buffers_.buffers[i].planes[0].p_buf = pmem_malloc_ext(
                 cam_buffers_.buffers[i].planes[0].size,
				 PMEM_CAMERA_ID,
				 PMEM_FLAGS_PHYS_NON_CONTIG,
				 PMEM_ALIGNMENT_4K);
		    LOG_D("QCARCAM_VER_462 qcarcam_s_buffers(%d),p_buf:%p,size:%u", i, cam_buffers_.buffers[i].planes[0].p_buf,	cam_buffers_.buffers[i].planes[0].size);
		#endif
		#ifdef QCARCAM_VER_477
            cam_buffers_.flags |= QCARCAM_BUFFER_FLAG_OS_HNDL;
            pmem_handle_t mem_handle;
            virtual_address_[i] = pmem_malloc_ext_v2(
                    cam_buffers_.buffers[i].planes[0].size,
                    PMEM_CAMERA_ID,
                    PMEM_FLAGS_SHMEM | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_CACHE_NONE,
                    PMEM_ALIGNMENT_4K,
                    0,
                    &mem_handle,
                    NULL);
		    cam_buffers_.buffers[i].planes[0].p_buf = mem_handle;
            LOG_D("QCARCAM_VER_477 qcarcam_s_buffers(%d),p_buf:%p,size:%u", i, cam_buffers_.buffers[i].planes[0].p_buf,	cam_buffers_.buffers[i].planes[0].size);
		#endif
	    }
        ret = qcarcam_s_buffers(cam_handle_, &cam_buffers_);
        if (ret != QCARCAM_RET_OK) {
            LOG_E("qcarcam_s_buffers() failed, errorcode:%d", ret);
            return false;
        }
        return true;
    }
    inline void set_qnx_car_camera_map() {
        QNX_CAR_CAMERA_MAP.handler = cam_handle_;
        QNX_CAR_CAMERA_MAP.camera = this;
    }
    static void qcarcam_event_cb(qcarcam_hndl_t hndl, qcarcam_event_t event_id, qcarcam_event_payload_t *p_payload) {
        if (hndl != QNX_CAR_CAMERA_MAP.handler) {
            LOG_E("QNX_CAR_CAMERA_MAP error!");
            return;
        }
        qnx_car_camera *camera = static_cast<qnx_car_camera *>(QNX_CAR_CAMERA_MAP.camera);
        if (nullptr == camera) {
            LOG_E("camera is null!");
            return;
        }
        if (nullptr == p_payload) {
            LOG_E("payload is null!");
            return;
        }
        switch (event_id) {
            case QCARCAM_EVENT_FRAME_READY:
                camera->process_frame();
                break;
            case QCARCAM_EVENT_ERROR:
            {
                switch (p_payload->uint_payload)
                {
                case QCARCAM_CONN_ERROR:
                    LOG_E("QCARCAM_CONN_ERROR!");
                    break;
                case QCARCAM_FATAL_ERROR:
                    LOG_E("QCARCAM_FATAL_ERROR!");
                    break;
                case QCARCAM_IFE_OVERFLOW_ERROR:
                    LOG_E("QCARCAM_IFE_OVERFLOW_ERROR!");
                    break;
                case QCARCAM_FRAMESYNC_ERROR:
                    LOG_E("QCARCAM_FRAMESYNC_ERROR!");
                    break;
                default:
                    LOG_E("QCARCAM UNKNOWN EVENT ERROR!");
                    break;
                }
                break;
            }
            case QCARCAM_EVENT_INPUT_SIGNAL:
            {
                switch (p_payload->uint_payload)
                {
                case QCARCAM_INPUT_SIGNAL_LOST:
                    LOG_E("QCARCAM_INPUT_SIGNAL_LOST!");
                    break;
                case QCARCAM_INPUT_SIGNAL_VALID:
                    LOG_D("QCARCAM_INPUT_SIGNAL_VALID!");
                    break;
                default:
                    LOG_E("QCARCAM UNKNOWN INPUT SIGNAL!");
                    break;
                }
            }
            default:
                LOG_E("received unsupported event:%d", event_id);
                break;
        }
    }
    bool process_frame() {
        if (!cam_handle_) {
            LOG_E("camera handle is null!");
            return false;
        }
        qcarcam_frame_info_t frame_info = { 0 };
        qcarcam_ret_t ret = qcarcam_get_frame(cam_handle_, &frame_info, QCARCAM_TEST_DEFAULT_GET_FRAME_TIMEOUT, 0);
        if (QCARCAM_RET_OK != ret) {
            LOG_E("qcarcam_get_frame failed errorcode:%d\n", ret);
            return false;
        }
        if ((unsigned int)frame_info.idx >= cam_buffers_.n_buffers) {
            LOG_E("camera buffer overflow frame index:%d frame buffers:%d\n", frame_info.idx, cam_buffers_.n_buffers);
            return false;
        }
    #ifdef QCARCAM_VER_462
        camera_image_processor_((const char *)cam_buffers_.buffers[frame_info.idx].planes[0].p_buf, cam_buffers_.buffers[frame_info.idx].planes[0].size);
	#endif
	#ifdef QCARCAM_VER_477
        camera_image_processor_((const char *)virtual_address_[frame_info.idx], cam_buffers_.buffers[frame_info.idx].planes[0].size);
	#endif
    	ret = qcarcam_release_frame(cam_handle_, frame_info.idx);
	    if (QCARCAM_RET_OK != ret) {
		    LOG_E("qcarcam_release_frame failed errorcode:%d\n", ret);
		    return false;
    	}
        return true;
    }
    bool get_camera_info() {
        unsigned int queryNumInputs = 0, queryFilled = 0;
        int ret = qcarcam_query_inputs(NULL, 0, &queryNumInputs);
        if (QCARCAM_RET_OK != ret || queryNumInputs == 0) {
            LOG_E("qcarcam_query_inputs failed, error code:%d", ret);
            return false;
        }
        qcarcam_input_t *pInputs = (qcarcam_input_t *)calloc(queryNumInputs, sizeof(*pInputs));
        if (!pInputs) {
            LOG_E("calloc for qcarcam_input_t failed");
            return false;
        }
        ret = qcarcam_query_inputs(pInputs, queryNumInputs, &queryFilled);
        if (QCARCAM_RET_OK != ret || queryFilled != queryNumInputs) {
            free(pInputs); 
            LOG_E("Failed qcarcam_query_inputs failed error code:%d,queryNumInputs:%d,queryFilled:%d", ret, queryFilled, queryNumInputs);
            return false;
        }
        bool find_camera = false;
        for (unsigned int i = 0;i < queryFilled;i++) {
            if (pInputs[i].desc == cam_id_) {
                width_ = pInputs[i].res[0].width;
                height_ = pInputs[i].res[0].height;
                fmt_ = pInputs[i].color_fmt[0];
                find_camera = true;
            }
            LOG_I("item:%d,camera id:%d,size:%dx%d,fortmat:0x%08x,fps=%.2f,flags=0x%x", i + 1, 
                                                                                        pInputs[i].desc,
                                                                                        pInputs[i].res[0].width,
                                                                                        pInputs[i].res[0].height,
                                                                                        pInputs[i].color_fmt[0], 
                                                                                        pInputs[i].res[0].fps, 
                                                                                        pInputs[i].flags);
        }
        free(pInputs);
        if (!find_camera) {
            LOG_E("can not find the target camera id:%d", cam_id_);
            return false;
        }
        if (fmt_ != QCARCAM_FMT_UYVY_8) {
            LOG_E("camera format is not uyvy!");
            return false;
        }
        set_global_camera_info();
        return true;
    }
    inline void set_global_camera_info() {
        G_CAMERA_INFO.camera_width = width_;
        G_CAMERA_INFO.camera_height = height_;
    }
private:
    qcarcam_input_desc_t cam_id_ = QCARCAM_INPUT_MAX;
    qcarcam_color_fmt_t fmt_ = QCARCAM_FMT_MAX;
    qcarcam_hndl_t cam_handle_ = nullptr;                 // camera handler
    qcarcam_buffers_t cam_buffers_;
    void **virtual_address_ = nullptr;
    unsigned int buf_num_ = 6;
    unsigned int width_;
    unsigned int height_;
private:
    camera_image_process_type camera_image_processor_;
};
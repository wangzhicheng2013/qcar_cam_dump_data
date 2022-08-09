#pragma once
#include <unistd.h>
#include <string>
#include <pthread.h>
#include "qcarcam.h"
#include "pmem.h"
struct qcar_cam {
    static int init() {
        qcarcam_ret_t ret = QCARCAM_RET_OK;
        qcarcam_init_t qcarcam_init = { 0 };
        qcarcam_init.version = QCARCAM_VERSION;
        ret = qcarcam_initialize(&qcarcam_init);
        if (ret != QCARCAM_RET_OK) {
            return ret;
        }
        return 0;
    }
    static int uninit() {
        qcarcam_ret_t ret = qcarcam_uninitialize();
        if (ret != QCARCAM_RET_OK) {
            return ret;
        }
        return 0;
    }
};


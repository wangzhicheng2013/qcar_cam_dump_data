#ifndef _QCARCAM_DIAG_TYPES_H_
#define _QCARCAM_DIAG_TYPES_H_

/* ===========================================================================
 * Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file qcarcam_diag_types.h
 * @brief QCARCAM Diagnostic information header
 *
=========================================================================== */

#include "qcarcam_types.h"

#define MAX_USR_CLIENTS 64
#define MAX_NUM_INPUT_DEVICES 8
#define MAX_NUM_CSIPHY_DEVICES 4
#define MAX_NUM_IFE_DEVICES 8
#define MAX_SOURCES_PER_DEVICE 4
#define IFE_OUTPUT_RDI_MAX 4
#define MAX_ERR_QUEUE_SIZE 64
#define MAX_NUM_ERROR_PAYLOAD_SIZE 18

typedef struct
{
    unsigned int bufId;
    unsigned int bufStatus;
}BufferInfo;

typedef struct
{
    unsigned int rdiId;
    unsigned int rdiStatus;
}RdiInfo;

typedef struct
{
    unsigned int cciDevId;
    unsigned int cciPortId;
}CciInfo;

typedef struct
{
    qcarcam_hndl_t usrHdl;
    unsigned int state;
    qcarcam_input_desc_t inputId;
    qcarcam_opmode_type opMode;
    unsigned int inputDevId;
    unsigned int csiphyDevId;
    unsigned int ifeDevId;
    unsigned int rdiId;
    unsigned int frameRate;
    unsigned long long timeStampStart;
    unsigned long long sofCounter;
    unsigned long long frameCounter;
    BufferInfo bufInfo[QCARCAM_MAX_NUM_BUFFERS];
}AisDiagClientInfo;

typedef struct
{
    unsigned int inputSrcId; /* Input source Id */
    unsigned int status; /* Input source Id - whether detected or not */
    qcarcam_mode_t sensorMode;
}InputSourceInfo;

typedef struct
{
    unsigned int inputDevId;
    unsigned int numSensors;
    unsigned int state;
    unsigned int srcIdEnableMask;
    CciInfo cciMap;
    InputSourceInfo inputSourceInfo[MAX_SOURCES_PER_DEVICE];	
}AisDiagInputDeviceInfo;

typedef struct
{
    unsigned int csiphyDevId;
    unsigned int csiLaneMapping;
    unsigned int numIfeMap;
    unsigned int ifeMap;
}AisDiagCsiDeviceInfo;

typedef struct
{
    unsigned int ifeDevId;
    unsigned int csiDevId;
    unsigned int numRdi;
    unsigned long long csidPktsRcvd;
    RdiInfo rdiInfo[IFE_OUTPUT_RDI_MAX];
}AisDiagIfeDeviceInfo;

typedef struct
{
    unsigned int errorType;
    qcarcam_hndl_t usrHdl;
    unsigned int inputSrcId;
    unsigned int inputDevId;
    unsigned int csiphyDevId;
    unsigned int ifeDevId;
    unsigned int rdiId;
    unsigned long long errorTimeStamp;
    unsigned int payload[MAX_NUM_ERROR_PAYLOAD_SIZE];
}AisDiagErrorInfo;

typedef struct
{
    AisDiagClientInfo aisDiagClientInfo[MAX_USR_CLIENTS];
    AisDiagInputDeviceInfo aisDiagInputDevInfo[MAX_NUM_INPUT_DEVICES];
    AisDiagCsiDeviceInfo aisDiagCsiDevInfo[MAX_NUM_CSIPHY_DEVICES];
    AisDiagIfeDeviceInfo aisDiagIfeDevInfo[MAX_NUM_IFE_DEVICES];
    AisDiagErrorInfo aisDiagErrInfo[MAX_ERR_QUEUE_SIZE];
}AisDiagnosticInfo;

#endif /* _QCARCAM_DIAG_TYPES_H_ */

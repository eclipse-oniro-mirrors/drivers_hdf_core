/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <utils/hdf_log.h>
#include <osal/osal_mem.h>
#include "camera_config_parser.h"
#include "camera_device_manager.h"
#include "camera_utils.h"
#include "camera_dispatch.h"

#define HDF_LOG_TAG HDF_CAMERA_DISPATCH

static int32_t CheckDeviceOps(int camId, struct CameraDeviceDriver *deviceDriver, struct CameraConfigRoot *rootConfig)
{
    int32_t i;
    if (rootConfig->deviceConfig[camId].sensor.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].sensor.sensorNum; i++) {
            if (deviceDriver->sensor[i]->devOps == NULL) {
                HDF_LOGE("%s: sensorOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].isp.mode == DEVICE_SUPPORT) {
        if (deviceDriver->isp->devOps == NULL) {
            HDF_LOGE("%s: ispOps is null!", __func__);
            return HDF_FAILURE;
        }
    }
    if (rootConfig->deviceConfig[camId].lens.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].lens.lensNum; i++) {
            if (deviceDriver->lens[i]->devOps == NULL) {
                HDF_LOGE("%s: lensOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].vcm.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].vcm.vcmNum; i++) {
            if (deviceDriver->vcm[i]->devOps == NULL) {
                HDF_LOGE("%s: vcmOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].flash.mode == DEVICE_SUPPORT) {
        if (deviceDriver->flash->devOps == NULL) {
            HDF_LOGE("%s: flashOps is null!", __func__);
            return HDF_FAILURE;
        }
    }
    if (rootConfig->deviceConfig[camId].stream.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].stream.streamNum; i++) {
            if (deviceDriver->stream[i]->streamOps == NULL) {
                HDF_LOGE("%s: streamOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraOpenCamera(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t camId = 0;
    int32_t ret;
    struct CameraDevice *camDev = NULL;
    const char *driverName = NULL;
    const char *deviceName = NULL;
    struct CameraConfigRoot *rootConfig = NULL;
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    deviceName = HdfSbufReadString(reqData);
    if (deviceName == NULL) {
        HDF_LOGE("%s: fail to get deviceName!", __func__);
        return HDF_FAILURE;
    }
    CHECK_RETURN_RET(GetCameraId(deviceName, strlen(deviceName), &camId));
    if (camId > CAMERA_DEVICE_MAX_NUM) {
        HDF_LOGE("%s: wrong camId, camId = %{public}d", __func__, camId);
        return HDF_FAILURE;
    }
    driverName = HdfSbufReadString(reqData);
    if (driverName == NULL) {
        HDF_LOGE("%s: Read driverName failed!", __func__);
        return HDF_FAILURE;
    }

    camDev = CameraDeviceGetByName(deviceName);
    if (camDev == NULL) {
        HDF_LOGE("%s: camDev not found! deviceName=%{public}s", __func__, deviceName);
        return HDF_FAILURE;
    }

    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    ret = CheckDeviceOps(camId, deviceDriver, rootConfig);

    return ret;
}

static int32_t CameraCmdOpenCamera(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t permissionId;
    int32_t ret;
    uint32_t index = 0;

    if (client == NULL || reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadInt32(reqData, &permissionId)) {
        HDF_LOGE("%s: Read request data failed! permissionId = %{public}d", __func__, permissionId);
        return HDF_FAILURE;
    }
    if (FindAvailableId(&index)) {
        AddPermissionId(index, permissionId);
    } else {
        HDF_LOGE("%s: AddPermissionId failed!", __func__);
        return HDF_FAILURE;
    }

    ret = CameraOpenCamera(client, reqData, rspData);

    return ret;
}

static int32_t CameraCmdCloseCamera(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t permissionId;
    int32_t ret;
    const char *deviceName = NULL;

    if (client == NULL || reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadInt32(reqData, &permissionId)) {
        HDF_LOGE("%s: Read request data failed! permissionId = %{public}d", __func__, permissionId);
        return HDF_FAILURE;
    }
    CHECK_RETURN_RET(CheckPermission(permissionId));
    deviceName = HdfSbufReadString(reqData);
    if (deviceName == NULL) {
        HDF_LOGE("%s: fail to get deviceName!", __func__);
        return HDF_FAILURE;
    }
    ret = RemovePermissionId(permissionId);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: RemovePermissionId failed, ret=%{public}d", __func__, ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static struct CameraCmdHandle g_cameraCmdHandle[CAMERA_MAX_CMD_ID] = {
    {CMD_OPEN_CAMERA, CameraCmdOpenCamera},
    {CMD_CLOSE_CAMERA, CameraCmdCloseCamera},
    {CMD_POWER_UP, NULL},
    {CMD_POWER_DOWN, NULL},
    {CMD_QUERY_CONFIG, NULL},
    {CMD_GET_CONFIG, NULL},
    {CMD_SET_CONFIG, NULL},
    {CMD_GET_FMT, NULL},
    {CMD_SET_FMT, NULL},
    {CMD_GET_CROP, NULL},
    {CMD_SET_CROP, NULL},
    {CMD_GET_FPS, NULL},
    {CMD_SET_FPS, NULL},
    {CMD_ENUM_FMT, NULL},
    {CMD_ENUM_DEVICES, NULL},
    {CMD_GET_ABILITY, NULL},
    {CMD_QUEUE_INIT, NULL},
    {CMD_REQ_MEMORY, NULL},
    {CMD_QUERY_MEMORY, NULL},
    {CMD_STREAM_QUEUE, NULL},
    {CMD_STREAM_DEQUEUE, NULL},
    {CMD_STREAM_ON, NULL},
    {CMD_STREAM_OFF, NULL},
};

int32_t HdfCameraDispatch(struct HdfDeviceIoClient *client, int cmdId,
    struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    uint32_t i;
    HDF_LOGD("%s: [cmdId = %{public}d].", __func__, cmdId);
    if (client == NULL || reqData == NULL) {
        HDF_LOGE("%s: Input params check error", __func__);
        return HDF_FAILURE;
    }

    if ((cmdId >= CAMERA_MAX_CMD_ID) || (cmdId < 0)) {
        HDF_LOGE("%s: Invalid [cmdId=%{public}d].", __func__, cmdId);
        return HDF_ERR_INVALID_PARAM;
    }

    for (i = 0; i < HDF_ARRAY_SIZE(g_cameraCmdHandle); ++i) {
        if ((cmdId == (int)(g_cameraCmdHandle[i].cmd)) && (g_cameraCmdHandle[i].func != NULL)) {
            return g_cameraCmdHandle[i].func(client, reqData, rspData);
        }
    }
    return HDF_FAILURE;
}
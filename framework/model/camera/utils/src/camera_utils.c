/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <utils/hdf_log.h>
#include "camera_config_parser.h"
#include "camera_device_manager.h"
#include "camera_utils.h"

#define HDF_LOG_TAG HDF_CAMERA_UTILS

#define CAMERA_DEVICE_BIT_MAX 2
#define DEC_CALC_COEFF 10
#define FPS_CALC_COEFF 1000
#define UINT_MAX_VAL 0xFFFFFFFF

static int32_t g_cameraIdMap[ID_MAX_NUM];

bool FindAvailableId(uint32_t *id)
{
    uint32_t i;
    if (id == NULL) {
        HDF_LOGE("%s: param is null ptr", __func__);
        return false;
    }
    for (i = 0; i < ID_MAX_NUM; i++) {
        if (g_cameraIdMap[i] == 0) {
            *id = i;
            return true;
        }
    }
    return false;
}

int32_t AddPermissionId(int32_t id, int32_t permissionId)
{
    if (id >= ID_MAX_NUM) {
        HDF_LOGE("%s: error: id out of range!", __func__);
        return HDF_FAILURE;
    }
    if (g_cameraIdMap[id] != 0) {
        HDF_LOGE("%s: error: g_cameraIdMap[%{public}d] is already exist!", __func__, id);
        return HDF_FAILURE;
    }
    g_cameraIdMap[id] = permissionId;
    return HDF_SUCCESS;
}

int32_t RemovePermissionId(int32_t id)
{
    int32_t i;
    for (i = 0; i < ID_MAX_NUM; i++) {
        if (g_cameraIdMap[i] != 0) {
            if (g_cameraIdMap[i] == id) {
                g_cameraIdMap[i] = 0;
                return HDF_SUCCESS;
            }
        }
    }
    return HDF_FAILURE;
}

int32_t GetCameraId(const char *str, int length, int *id)
{
    int32_t ret[CAMERA_DEVICE_BIT_MAX];
    int32_t i = 0;
    int32_t j = 0;

    if (str == NULL || id == NULL) {
        HDF_LOGE("%s: param is null ptr", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    for (i = 0; i < length; i++) {
        if ((*(str + i) < '0') || (*(str + i) > '9')) {
            continue;
        }
        if (j >= CAMERA_DEVICE_BIT_MAX) {
            HDF_LOGE("%s: wrong num: %{public}d", __func__, j);
            return HDF_ERR_INVALID_PARAM;
        }
        ret[j] = (*(str + i) - '0');
        j++;
    }

    for (i = 0; i < j; i++) {
        *id = ret[i] + (*id) * DEC_CALC_COEFF;
    }
    if (*id > CAMERA_MAX_NUM) {
        HDF_LOGE("%s: wrong id: %{public}d", __func__, id);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

struct CameraDeviceDriver *GetDeviceDriver(const struct CameraDevice *cameraDev)
{
    if (cameraDev == NULL) {
        HDF_LOGE("%s: cameraDev is null!", __func__);
        return NULL;
    }

    return cameraDev->deviceDriver;
}

int32_t CheckPermission(int permissionId)
{
    if (permissionId != CAMERA_MASTER) {
        HDF_LOGE("%s: No permissions set ctrl!", __func__);
        return HDF_ERR_NOPERM;
    }
    return HDF_SUCCESS;
}

static int32_t CheckSwitchType(int type, struct CameraConfigRoot *rootConfig, int32_t camId)
{
    switch (type) {
        case SENSOR_TYPE:
            if (rootConfig->deviceConfig[camId].sensor.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sendor device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case ISP_TYPE:
            if (rootConfig->deviceConfig[camId].isp.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case VCM_TYPE:
            if (rootConfig->deviceConfig[camId].vcm.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case LENS_TYPE:
            if (rootConfig->deviceConfig[camId].lens.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: lens device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case FLASH_TYPE:
            if (rootConfig->deviceConfig[camId].flash.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: flash device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case UVC_TYPE:
            if (rootConfig->uvcMode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: uvc device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case STREAM_TYPE:
            if (rootConfig->deviceConfig[camId].stream.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: stream device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CheckCameraDevice(const char *deviceName, int type)
{
    int32_t camId = 0;
    int32_t ret;
    struct CameraConfigRoot *rootConfig = NULL;
    if (deviceName == NULL) {
        HDF_LOGE("%s: get deviceName failed!", __func__);
        return HDF_FAILURE;
    }
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    CHECK_RETURN_RET(GetCameraId(deviceName, strlen(deviceName), &camId));
    ret = CheckSwitchType(type, rootConfig, camId);

    return ret;
}

static int32_t GetSensorNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].sensor.sensorNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].sensor.sensor[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

static int32_t GetVcmNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].vcm.vcmNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].vcm.vcm[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

static int32_t GetLensNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].lens.lensNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].lens.lens[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

static int32_t GetStreamNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].stream.streamNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].stream.stream[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

int32_t GetDeviceNum(const char *driverName, int camId, int type)
{
    int32_t ret;
    struct CameraConfigRoot *rootConfig = NULL;
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    switch (type) {
        case SENSOR_TYPE:
            ret = GetSensorNum(driverName, camId, rootConfig);
            break;
        case VCM_TYPE:
            ret = GetVcmNum(driverName, camId, rootConfig);
            break;
        case LENS_TYPE:
            ret = GetLensNum(driverName, camId, rootConfig);
            break;
        case STREAM_TYPE:
            ret = GetStreamNum(driverName, camId, rootConfig);
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return ret;
}

int32_t CheckFormatNum(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig)
{
    int32_t deviceNum;
    struct CameraConfigRoot *rootConfig = NULL;
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    deviceNum = GetDeviceNum(driverName, camId, type);

    switch (type) {
        case STREAM_TYPE:
            if (ctrlConfig->pixelFormat.pixel.width > rootConfig->deviceConfig[camId].
                                                        stream.stream[deviceNum].widthMaxNum) {
                HDF_LOGE("%s: stream%{public}d wrong width = %{public}d", __func__, deviceNum,
                    ctrlConfig->pixelFormat.pixel.width);
                return HDF_ERR_INVALID_PARAM;
            } else if (ctrlConfig->pixelFormat.pixel.height > rootConfig->
                                                        deviceConfig[camId].stream.stream[deviceNum].heightMaxNum) {
                HDF_LOGE("%s: stream%{public}d wrong height = %{public}d", __func__, deviceNum,
                    ctrlConfig->pixelFormat.pixel.height);
                return HDF_ERR_INVALID_PARAM;
            }
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CheckFrameRate(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig)
{
    int32_t fps, deviceNum;
    struct CameraConfigRoot *rootConfig = NULL;
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    deviceNum = GetDeviceNum(driverName, camId, type);
    if (ctrlConfig->pixelFormat.fps.numerator == 0 || ctrlConfig->pixelFormat.fps.denominator == 0) {
        HDF_LOGE("%s: wrong numerator %{public}d or wrong denominator %{public}d!", __func__,
            ctrlConfig->pixelFormat.fps.numerator, ctrlConfig->pixelFormat.fps.denominator);
        return HDF_ERR_INVALID_PARAM;
    }
    if (FPS_CALC_COEFF > UINT_MAX_VAL / ctrlConfig->pixelFormat.fps.denominator) {
        HDF_LOGE("%s: wrong num!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    fps = FPS_CALC_COEFF * (ctrlConfig->pixelFormat.fps.denominator) / ctrlConfig->pixelFormat.fps.numerator;

    switch (type) {
        case STREAM_TYPE:
            if (fps > FPS_CALC_COEFF * (rootConfig->deviceConfig[camId].stream.stream[deviceNum].frameRateMaxNum)) {
                HDF_LOGE("%s: stream%{public}d wrong fps = %{public}d", __func__, deviceNum, fps);
                return HDF_ERR_INVALID_PARAM;
            }
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CheckFmtType(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig)
{
    uint32_t format_size, i, num;
    struct CameraConfigRoot *rootConfig = NULL;

    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    switch (type) {
        case STREAM_TYPE:
            num = GetDeviceNum(driverName, camId, STREAM_TYPE);
            format_size = rootConfig->deviceConfig[camId].stream.stream[num].formatTypeNum;
            for (i = 0; i < format_size; i++) {
                if (ctrlConfig->pixelFormat.pixel.format ==
                    rootConfig->deviceConfig[camId].stream.stream[num].formatType[i]) {
                    HDF_LOGD("%s: formatType: %{public}d", __func__, ctrlConfig->pixelFormat.pixel.format);
                    return HDF_SUCCESS;
                }
            }
            HDF_LOGE("%s: not support formatType: %{public}d", __func__, ctrlConfig->pixelFormat.pixel.format);
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_FAILURE;
}
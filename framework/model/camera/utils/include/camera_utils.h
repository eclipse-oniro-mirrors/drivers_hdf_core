/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_UTILS_H
#define CAMERA_UTILS_H

#include <utils/hdf_base.h>
#include "camera_device_manager.h"

#define ID_MAX_NUM 10

bool FindAvailableId(uint32_t *id);
int32_t AddPermissionId(int32_t id, int32_t permissionId);
int32_t RemovePermissionId(int32_t id);
int32_t GetCameraId(const char* str, int length, int* id);
struct CameraDeviceDriver *GetDeviceDriver(const struct CameraDevice *cameraDev);
int32_t CheckPermission(int permissionId);
int32_t CheckCameraDevice(const char *deviceName, int type);
int32_t GetDeviceNum(const char *driverName, int camId, int type);
int32_t CheckFormatNum(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig);
int32_t CheckFrameRate(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig);
int32_t CheckFmtType(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig);

#endif  // CAMERA_UTILS_H
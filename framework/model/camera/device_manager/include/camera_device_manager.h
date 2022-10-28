/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_DEVICE_MANAGER_H
#define CAMERA_DEVICE_MANAGER_H

#include <utils/device_resource_if.h>
#include <camera/camera_product.h>
#include "camera_buffer_manager.h"
#include "camera_buffer_manager_adapter.h"

#define DEVICE_NAME_SIZE 20
#define DEVICE_DRIVER_MAX_NUM 20
#define CAMERA_COMPONENT_NAME_MAX_LEN 20
#define DRIVER_NAME_SIZE 20
#define DEVICE_NUM 3

#define CHECK_RETURN_RET(ret) do { \
    if ((ret) != 0) { \
        HDF_LOGE("%s: failed, ret = %{public}d, line: %{public}d", __func__, ret, __LINE__); \
        return ret; \
    } \
} while (0)

struct Pixel {
    char* description;
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t maxwidth;
    uint32_t maxheight;
    uint32_t minwidth;
    uint32_t minheight;
    uint32_t sizeImage;
};

struct FPS {
    uint32_t numerator;
    uint32_t denominator;
};

struct CameraControl {
    uint32_t id;
    uint32_t value;
};

struct Rect {
    int32_t left;
    int32_t top;
    uint32_t width;
    uint32_t height;
};

struct Capability {
    char driver[16];
    char card[32];
    char busInfo[32];
    uint32_t capabilities;
};

struct CropCap {
    struct Rect bounds;
    struct Rect defrect;
    struct FPS pixelaspect;
};

struct PixelFormat {
    struct FPS fps;
    struct Pixel pixel;
    struct Rect cop;
    struct CropCap cropCap;
};

struct CameraCtrlConfig {
    struct PixelFormat pixelFormat;
    struct CameraControl ctrl;
};

struct CameraDeviceDriverFactory {
    const char *deviceName;
    void (*ReleaseFactory)(struct CameraDeviceDriverFactory *factory);
    struct CameraDeviceDriver *(*Build)(const char *deviceName);
    void (*Release)(struct CameraDeviceDriver *deviceDriver);
};

struct CameraDevice {
    char deviceName[DEVICE_NAME_SIZE];
    char driverName[DRIVER_NAME_SIZE];
    struct CameraDeviceDriver *deviceDriver;
    struct CameraQueueImp queueImp;
};

struct DeviceOps {
    int32_t (*PowerUp)(struct CameraDeviceDriver *regDev);
    int32_t (*PowerDown)(struct CameraDeviceDriver *regDev);
    int32_t (*SetConfig)(struct CameraDeviceDriver *regDev, struct CameraCtrlConfig *config, const char *driverName);
    int32_t (*GetConfig)(struct CameraDeviceDriver *regDev, struct CameraCtrlConfig *config, const char *driverName);
};

struct SensorDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct IspDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct VcmDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct LensDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct FlashDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct StreamDevice {
    struct StreamOps *streamOps;
};

struct UvcDevice {
    uint8_t id;
};

struct StreamOps {
    int32_t (*StreamSetFormat)(struct CameraCtrlConfig *config,
        struct StreamDevice *streamDev, const char *driverName);
    int32_t (*StreamGetFormat)(struct CameraCtrlConfig *config,
        struct StreamDevice *streamDev, const char *driverName);
    int32_t (*StreamSetCrop)(struct CameraCtrlConfig *config,
        struct StreamDevice *streamDev, const char *driverName);
    int32_t (*StreamGetCrop)(struct CameraCtrlConfig *config,
        struct StreamDevice *streamDev, const char *driverName);
    int32_t (*StreamSetFps)(struct CameraCtrlConfig *config,
        struct StreamDevice *streamDev, const char *driverName);
    int32_t (*StreamGetFps)(struct CameraCtrlConfig *config,
        struct StreamDevice *streamDev, const char *driverName);
    int32_t (*StreamGetAbility)(struct Capability *capability,
        struct StreamDevice *streamDev, const char *driverName);
    int32_t (*StreamEnumFormat)(struct PixelFormat *config, struct StreamDevice *streamDev,
        uint32_t index, uint32_t cmd, const char *driverName);
    int32_t (*StreamQueueInit)(struct BufferQueue *queue);
};

struct CameraDeviceDriverManager {
    struct CameraDeviceDriverFactory **deviceFactoryInsts;
    int32_t (*RegDeviceDriverFactory)(struct CameraDeviceDriverFactory *factoryInst);
    struct CameraDeviceDriverFactory *(*GetDeviceDriverFactoryByName)(const char *name);
};

struct CameraDeviceDriver {
    char name[CAMERA_COMPONENT_NAME_MAX_LEN];
    struct SensorDevice *sensor[DEVICE_NUM];
    struct IspDevice *isp;
    struct VcmDevice *vcm[DEVICE_NUM];
    struct LensDevice *lens[DEVICE_NUM];
    struct FlashDevice *flash;
    struct UvcDevice *uvc[DEVICE_NUM];
    struct StreamDevice *stream[DEVICE_NUM];
    int32_t (*init)(struct CameraDeviceDriver *deviceDriver, struct CameraDevice *camDev);
    int32_t (*deinit)(struct CameraDeviceDriver *deviceDriver, struct CameraDevice *camDev);
};

struct CameraDevice *CameraDeviceCreate(const char *deviceName, uint32_t len);
int32_t CameraDeviceRelease(struct CameraDevice *camDev);
struct CameraDeviceDriverFactory *CameraDeviceDriverFactoryGetByName(const char *deviceName);
int32_t CameraDeviceDriverFactoryRegister(struct CameraDeviceDriverFactory *obj);
int32_t DeviceDriverManagerDeInit(void);
struct CameraDevice *CameraDeviceGetByName(const char *deviceName);
struct CameraDeviceDriverManager *CameraDeviceDriverManagerGet(void);

#endif  // CAMERA_DEVICE_MANAGER_H
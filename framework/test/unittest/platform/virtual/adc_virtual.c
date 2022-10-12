/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include<math.h>
#include "adc/adc_core.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"

#define HDF_LOG_TAG adc_virtual_driver
#define MATCH_ATTR  adc_virtual
#define BASE_NUMBER         2
#define ADC_MAX_DATA_WIDTH  24

struct VirtualAdcDevice {
    struct AdcDevice device;
    uint32_t devNum;
    uint32_t dataWidth;
    uint32_t channels;
    uint32_t maxValue;
};

static int32_t VirtualAdcStart(struct AdcDevice *device)
{
    struct VirtualAdcDevice *virtual = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    virtual = (struct VirtualAdcDevice *)device;
    virtual->maxValue = pow(BASE_NUMBER, virtual->dataWidth) - 1;

    return HDF_SUCCESS;
}

static int32_t VirtualAdcRead(struct AdcDevice *device, uint32_t channel, uint32_t *val)
{
    struct VirtualAdcDevice *virtual = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    virtual = (struct VirtualAdcDevice *)device;
    if (channel >= virtual->channels || val == NULL) {
        HDF_LOGE("%s: invalid channel or val is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    *val = virtual->maxValue;
    return HDF_SUCCESS;
}

static int32_t VirtualAdcStop(struct AdcDevice *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static const struct AdcMethod g_method = {
    .read = VirtualAdcRead,
    .stop = VirtualAdcStop,
    .start = VirtualAdcStart,
};

static int32_t VirtualAdcReadDrs(struct VirtualAdcDevice *virtual, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    ret = drsOps->GetUint32(node, "devNum", &virtual->devNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read devNum failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "dataWidth", &virtual->dataWidth, 0);
    if (ret != HDF_SUCCESS || virtual->dataWidth == 0 || virtual->dataWidth >= ADC_MAX_DATA_WIDTH) {
        HDF_LOGE("%s: read dataWidth failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "channels", &virtual->channels, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read channels failed", __func__);
        return ret;
    }
    HDF_LOGD("%s: success, devNum=%u, dataWidth=%u, channels=%u",
        __func__, virtual->devNum, virtual->dataWidth, virtual->channels);

    return HDF_SUCCESS;
}

static int32_t VirtualAdcInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct VirtualAdcDevice *virtual = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    virtual = (struct VirtualAdcDevice *)OsalMemCalloc(sizeof(*virtual));
    if (virtual == NULL) {
        HDF_LOGE("%s: alloc virtual failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = VirtualAdcReadDrs(virtual, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read drs failed:%d", __func__, ret);
        OsalMemFree(virtual);
        return ret;
    }

    virtual->device.priv = (void *)device->property;
    virtual->device.devNum = virtual->devNum;
    virtual->device.ops = &g_method;
    ret = AdcDeviceAdd(&virtual->device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: add adc virtual device:%u failed", __func__, virtual->devNum);
        OsalMemFree(virtual);
        return ret;
    }

    HDF_LOGI("%s: adc virtual driver init success.", __func__);
    return ret;
}

static void VirtualAdcRelease(struct HdfDeviceObject *device)
{
    int32_t ret;
    uint32_t devNum;
    struct AdcDevice *dev = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    HDF_LOGI("%s: enter", __func__);
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null", __func__);
        return;
    }

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops", __func__);
        return;
    }

    ret = drsOps->GetUint32(device->property, "devNum", (uint32_t *)&devNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read devNum failed", __func__);
        return;
    }

    dev = AdcDeviceGet(devNum);
    AdcDevicePut(dev);
    if (dev != NULL && dev->priv == device->property) {
        AdcDeviceRemove(dev);
        OsalMemFree(dev);
    }
}

static struct HdfDriverEntry g_virtualAdcDriverEntry = {
    .moduleVersion = 1,
    .Init = VirtualAdcInit,
    .Release = VirtualAdcRelease,
    .moduleName = "virtual_adc_driver",
};
HDF_INIT(g_virtualAdcDriverEntry);

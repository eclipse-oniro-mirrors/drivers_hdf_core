/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "gpio_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "securec.h"

static struct GpioTestConfig g_config;

static int32_t GpioTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    HDF_LOGD("%s: enter!", __func__);

    (void)client;
    (void)data;
    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("%s: reply is null!", __func__);
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
            HDF_LOGE("%s: failed to write reply!", __func__);
            return HDF_ERR_IO;
        }
    } else {
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t GpioReadNameTestInfos(struct GpioTestConfig *config, const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps)
{
    int32_t ret;
    const char *tempName = NULL;

    ret = drsOps->GetString(node, "testNameOne", &tempName, "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to read testNameOne!", __func__);
        return ret;
    }

    if (strcpy_s(config->testNameOne, NAME_SIZE_MAX, tempName) != EOK) {
        HDF_LOGE("%s: failed to copy testNameOne!", __func__);
        return HDF_FAILURE;
    }

    ret = drsOps->GetString(node, "testNameTwo", &tempName, "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to read testNameTwo!", __func__);
        return ret;
    }

    if (strcpy_s(config->testNameTwo, NAME_SIZE_MAX, tempName) != EOK) {
        HDF_LOGE("%s: failed to copy testNameTwo!", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t GpioTestReadConfig(struct GpioTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    uint16_t tmp;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint16 == NULL) {
        HDF_LOGE("%s: invalid drs ops!", __func__);
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint16(node, "gpio", &tmp, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to read gpio!", __func__);
        return ret;
    }
    config->gpio = (uint16_t)tmp;

    ret = drsOps->GetUint16(node, "gpioTestTwo", &tmp, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to read gpioTestTwo!", __func__);
        return ret;
    }
    config->gpioTestTwo = (uint16_t)tmp;

    ret = drsOps->GetUint16(node, "gpioIrq", &tmp, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to read gpioIrq!", __func__);
        return ret;
    }
    config->gpioIrq = (uint16_t)tmp;

    ret = drsOps->GetUint16(node, "testUserApi", &tmp, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGW("%s: failed to read gpioIrq, using 0 as default!", __func__);
        config->testUserApi = 0;
    }
    config->testUserApi = (uint16_t)tmp;

    ret = GpioReadNameTestInfos(config, node, drsOps);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to read name test infos!", __func__);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t GpioTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or config is NULL!", __func__);
        return HDF_ERR_IO;
    }

    ret = GpioTestReadConfig(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to read config!", __func__);
        return ret;
    }

    service.Dispatch = GpioTestDispatch;
    device->service = &service;

    return HDF_SUCCESS;
}

static int32_t GpioTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static void GpioTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    return;
}

struct HdfDriverEntry g_gpioTestEntry = {
    .moduleVersion = 1,
    .Bind = GpioTestBind,
    .Init = GpioTestInit,
    .Release = GpioTestRelease,
    .moduleName = "PLATFORM_GPIO_TEST",
};
HDF_INIT(g_gpioTestEntry);

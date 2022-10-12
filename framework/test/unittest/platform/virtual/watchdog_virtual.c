/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "osal_mem.h"
#include "watchdog/watchdog_core.h"

#define HDF_LOG_TAG watchdog_virtual

struct VirtualWatchdogCntlr {
    struct WatchdogCntlr wdt;
    int32_t seconds;
    int32_t status;
};

enum VirtualWatchdogStatus {
    WATCHDOG_STOP,  /**< Stopped */
    WATCHDOG_START, /**< Started */
};

static int32_t VirtualWatchdogGetStatus(struct WatchdogCntlr *wdt, int32_t *status)
{
    struct VirtualWatchdogCntlr *virtual = NULL;

    if (wdt == NULL) {
        HDF_LOGE("%s: wdt is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    virtual = (struct VirtualWatchdogCntlr *)wdt;

    if (status == NULL) {
        HDF_LOGE("%s: status is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    *status = virtual->status;

    return HDF_SUCCESS;
}

static int32_t VirtualWatchdogStart(struct WatchdogCntlr *wdt)
{
    struct VirtualWatchdogCntlr *virtual = NULL;

    if (wdt == NULL) {
        HDF_LOGE("%s: wdt is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    virtual = (struct VirtualWatchdogCntlr *)wdt;
    virtual->status = WATCHDOG_START;

    return HDF_SUCCESS;
}

static int32_t VirtualWatchdogStop(struct WatchdogCntlr *wdt)
{
    struct VirtualWatchdogCntlr *virtual = NULL;

    if (wdt == NULL) {
        HDF_LOGE("%s: wdt is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    virtual = (struct VirtualWatchdogCntlr *)wdt;
    virtual->status = WATCHDOG_STOP;

    return HDF_SUCCESS;
}

static int32_t VirtualWatchdogSetTimeout(struct WatchdogCntlr *wdt, uint32_t seconds)
{
    struct VirtualWatchdogCntlr *virtual = NULL;

    if (wdt == NULL) {
        HDF_LOGE("%s: wdt is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    virtual = (struct VirtualWatchdogCntlr *)wdt;
    virtual->seconds = seconds;

    return HDF_SUCCESS;
}

static int32_t VirtualWatchdogGetTimeout(struct WatchdogCntlr *wdt, uint32_t *seconds)
{
    struct VirtualWatchdogCntlr *virtual = NULL;

    if (wdt == NULL || seconds == NULL) {
        HDF_LOGE("%s: wdt or seconds is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    virtual = (struct VirtualWatchdogCntlr *)wdt;
    *seconds = virtual->seconds;

    return HDF_SUCCESS;
}

static int32_t VirtualWatchdogFeed(struct WatchdogCntlr *wdt)
{
    (void)wdt;
    return HDF_SUCCESS;
}

static struct WatchdogMethod g_method = {
    .getStatus = VirtualWatchdogGetStatus,
    .start = VirtualWatchdogStart,
    .stop = VirtualWatchdogStop,
    .setTimeout = VirtualWatchdogSetTimeout,
    .getTimeout = VirtualWatchdogGetTimeout,
    .feed = VirtualWatchdogFeed,
};

static int32_t VirtualWatchdogBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct WatchdogCntlr *wdt = NULL;

    HDF_LOGI("%s: enter", __func__);
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    wdt = (struct WatchdogCntlr *)OsalMemCalloc(sizeof(*wdt));
    if (wdt == NULL) {
        HDF_LOGE("%s: malloc wdt fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    wdt->priv = (void *)device->property;
    wdt->ops = &g_method;
    wdt->device = device;
    ret = WatchdogCntlrAdd(wdt);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: err add watchdog:%d", __func__, ret);
        OsalMemFree(wdt);
        return ret;
    }
    HDF_LOGI("%s: dev service %s bind success!", __func__, HdfDeviceGetServiceName(device));
    return HDF_SUCCESS;
}

static int32_t VirtualWatchdogInit(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: enter", __func__);
    (void)device;
    return HDF_SUCCESS;
}

static void VirtualWatchdogRelease(struct HdfDeviceObject *device)
{
    struct WatchdogCntlr *wdt = NULL;

    HDF_LOGI("%s: enter", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL!", __func__);
        return;
    }

    wdt = WatchdogCntlrFromDevice(device);
    if (wdt == NULL) {
        HDF_LOGE("%s: wdt is NULL!", __func__);
        return;
    }
    WatchdogCntlrRemove(wdt);
    OsalMemFree(wdt);
}

struct HdfDriverEntry g_virtualWatchdogDriverEntry = {
    .moduleVersion = 1,
    .Bind = VirtualWatchdogBind,
    .Init = VirtualWatchdogInit,
    .Release = VirtualWatchdogRelease,
    .moduleName = "virtual_watchdog_driver",
};
HDF_INIT(g_virtualWatchdogDriverEntry);

/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "can/can_manager.h"
#include "can/can_core.h"

#define HDF_LOG_TAG can_manager

#define CAN_NUMBER_MAX 32
#define CAN_MSG_POOL_SIZE_DFT 8

static struct PlatformManager *g_manager;

static struct PlatformManager *CanManagerGet(void)
{
    int32_t ret;

    if (g_manager == NULL) {
        ret = PlatformManagerCreate("CAN_BUS_MANAGER", &g_manager);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("CanManagerGet: careate can manager failed:%d", ret);
        }
    }
    return g_manager;
}

static void CanManagerDestroyIfNeed(void)
{
    if (g_manager != NULL && DListIsEmpty(&g_manager->devices)) {
        PlatformManagerDestroy(g_manager);
        g_manager = NULL;
    }
}

static int32_t CanCntlrCheckAndInit(struct CanCntlr *cntlr)
{
    if (cntlr == NULL || cntlr->ops == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->number < 0 || cntlr->number >= CAN_NUMBER_MAX) {
        HDF_LOGE("CanCntlrCheckAndInit: invlaid can num:%d", cntlr->number);
        return HDF_ERR_INVALID_OBJECT;
    }

    DListHeadInit(&cntlr->rxBoxList);

    if (OsalMutexInit(&cntlr->lock) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCheckAndInit: init lock failed");
        return HDF_FAILURE;
    }

    if (OsalMutexInit(&cntlr->rboxListLock) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCheckAndInit: init rx box list lock failed");
        (void)OsalMutexDestroy(&cntlr->lock);
        return HDF_FAILURE;
    }

    if (cntlr->msgPoolSize <= 0) {
        cntlr->msgPoolSize = CAN_MSG_POOL_SIZE_DFT;
    }

    cntlr->msgPool = CanMsgPoolCreate(cntlr->msgPoolSize);
    if (cntlr->msgPool == NULL) {
        HDF_LOGE("CanCntlrCheckAndInit: create can msg pool failed");
        (void)OsalMutexDestroy(&cntlr->rboxListLock);
        (void)OsalMutexDestroy(&cntlr->lock);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void CanCntlrDeInit(struct CanCntlr *cntlr)
{
    HDF_LOGD("CanCntlrDeInit: enter");
    CanMsgPoolDestroy(cntlr->msgPool);
    cntlr->msgPool = NULL;
    (void)OsalMutexDestroy(&cntlr->rboxListLock);
    (void)OsalMutexDestroy(&cntlr->lock);
    HDF_LOGD("CanCntlrDeInit: exit");
}

int32_t CanCntlrAdd(struct CanCntlr *cntlr)
{
    int32_t ret;

    ret = CanCntlrCheckAndInit(cntlr);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    cntlr->device.number = cntlr->number;
    ret = PlatformDeviceSetName(&cntlr->device, "CAN%d", cntlr->number);
    if (ret != HDF_SUCCESS) {
        CanCntlrDeInit(cntlr);
        return ret;
    }

    cntlr->device.manager = CanManagerGet();
    if (cntlr->device.manager == NULL) {
        PlatformDeviceClearName(&cntlr->device);
        CanCntlrDeInit(cntlr);
        return HDF_PLT_ERR_DEV_GET;
    }

    if ((ret = PlatformDeviceAdd(&cntlr->device)) != HDF_SUCCESS) {
        PlatformDeviceClearName(&cntlr->device);
        CanCntlrDeInit(cntlr);
        return ret;
    }

    HDF_LOGI("CanCntlrAdd: add controller %d success", cntlr->number);
    return HDF_SUCCESS;
}

int32_t CanCntlrDel(struct CanCntlr *cntlr)
{
    if (cntlr == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    PlatformDeviceDel(&cntlr->device);
    PlatformDeviceClearName(&cntlr->device);

    CanCntlrDeInit(cntlr);

    CanManagerDestroyIfNeed();
    HDF_LOGI("CanCntlrDel: del controller %d success", cntlr->number);
    return HDF_SUCCESS;
}

static struct CanCntlr *CanCntlrFromPlatformDevice(const struct PlatformDevice *pdevice)
{
    return CONTAINER_OF(pdevice, struct CanCntlr, device);
}

struct CanCntlr *CanCntlrGetByName(const char *name)
{
    static struct PlatformManager *manager = NULL;
    struct PlatformDevice *pdevice = NULL;

    manager = CanManagerGet();
    if (manager == NULL) {
        return NULL;
    }

    pdevice = PlatformManagerGetDeviceByName(manager, name);
    if (pdevice == NULL) {
        return NULL;
    }

    return CanCntlrFromPlatformDevice(pdevice);
}

struct CanCntlr *CanCntlrGetByNumber(int32_t number)
{
    static struct PlatformManager *manager = NULL;
    struct PlatformDevice *pdevice = NULL;

    manager = CanManagerGet();
    if (manager == NULL) {
        return NULL;
    }

    pdevice = PlatformManagerGetDeviceByNumber(manager, number);
    if (pdevice == NULL) {
        return NULL;
    }

    return CanCntlrFromPlatformDevice(pdevice);
}

void CanCntlrPut(struct CanCntlr *cntlr)
{
    if (cntlr != NULL) {
        PlatformDevicePut(&cntlr->device);
    }
}

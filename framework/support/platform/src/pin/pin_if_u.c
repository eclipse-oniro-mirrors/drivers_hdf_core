/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_base.h"
#include "hdf_dlist.h"
#include "hdf_io_service_if.h"
#include "osal_mem.h"
#include "pin_if.h"
#include "platform_core.h"
#include "securec.h"

#define HDF_LOG_TAG   pin_if_u_c
#define PIN_NAME_LEN  128
#define FUNC_NAME_LEN 30

#define PIN_SERVICE_NAME "HDF_PLATFORM_PIN_MANAGER"

static struct DListHead g_listHead;
static OsalSpinlock g_listLock;

struct PinInfo {
    char pinName[PIN_NAME_LEN];
    char funcName[FUNC_NAME_LEN];
    struct DListHead node;
};

static const char *AddNode(const char *pinName)
{
    struct PinInfo *pin = NULL;
    static struct DListHead *head = NULL;

    if (head == NULL) {
        head = &g_listHead;
        DListHeadInit(head);
        OsalSpinInit(&g_listLock);
    }
    pin = OsalMemCalloc(sizeof(struct PinInfo));
    if (pin == NULL) {
        HDF_LOGE("%s:malloc pin fail", __func__);
        return NULL;
    }
    DListHeadInit(&pin->node);
    if (strcpy_s(pin->pinName, PIN_NAME_LEN, pinName) != EOK) {
        HDF_LOGE("%s:copy pinName fail", __func__);
        OsalMemFree(pin);
        return NULL;
    }
    (void)OsalSpinLock(&g_listLock);
    DListInsertTail(&pin->node, head);
    (void)OsalSpinUnlock(&g_listLock);
    return pin->pinName;
}

static void RemoveNode(const char *pinName)
{
    struct DListHead *head = &g_listHead;
    struct PinInfo *pos = NULL;
    struct PinInfo *tmp = NULL;

    (void)OsalSpinLock(&g_listLock);
    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, head, struct PinInfo, node) {
        if (strcmp(pos->pinName, pinName) == 0) {
            DListRemove(&pos->node);
            OsalMemFree(pos);
        }
    }
    (void)OsalSpinUnlock(&g_listLock);
}

static struct HdfIoService *PinManagerServiceGet(void)
{
    static struct HdfIoService *service = NULL;

    if (service != NULL) {
        return service;
    }
    service = HdfIoServiceBind(PIN_SERVICE_NAME);
    if (service == NULL) {
        HDF_LOGE("PinServiceGetService: fail to get pin service!");
    }
    return service;
}

DevHandle PinGet(const char *pinName)
{
    int32_t ret;
    const char *copyName = NULL;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (pinName == NULL) {
        HDF_LOGE("%s:pinName is NULL", __func__);
        return NULL;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        return NULL;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: obtain data fail!", __func__);
        return NULL;
    }

    if (!HdfSbufWriteString(data, pinName)) {
        HDF_LOGE("%s: write dec fail!", __func__);
        HdfSbufRecycle(data);
        return NULL;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_GET service process fail:%d", __func__, ret);
        HdfSbufRecycle(data);
        return NULL;
    }

    copyName = AddNode(pinName);
    if (copyName == NULL) {
        HDF_LOGE("%s:copyName is NULL", __func__);
        HdfSbufRecycle(data);
        return NULL;
    }

    HdfSbufRecycle(data);
    return (void *)copyName;
}

void PinPut(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("%s: handle is NULL!", __func__);
        return;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        RemoveNode((const char *)handle);
        return;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: obtain data fail!", __func__);
        RemoveNode((const char *)handle);
        return;
    }
    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("%s: write handle fail!", __func__);
        RemoveNode((const char *)handle);
        HdfSbufRecycle(data);
        return;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_PUT, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_PUT service process fail:%d", __func__, ret);
    }
    RemoveNode((const char *)handle);
    HdfSbufRecycle(data);
}

int32_t PinSetPull(DevHandle handle, enum PinPullType pullType)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("%s: handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: failed to obtain data buf", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("%s: write handle fail!", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)pullType)) {
        HDF_LOGE("%s: write pulltype fail", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_SET_PULL, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_SET_PULL service process fail:%d", __func__, ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t PinGetPull(DevHandle handle, enum PinPullType *pullType)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL || pullType == NULL) {
        HDF_LOGE("%s: handle or pullType is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: failed to obtain data buf", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("%s: failed to obtain reply buf", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("%s: write handle fail!", __func__);
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET_PULL, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_GET_PULL service process fail:%d", __func__, ret);
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return ret;
    }

    if (!HdfSbufReadUint32(reply, pullType)) {
        HDF_LOGE("%s: read pulltype fail", __func__);
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    HdfSbufRecycle(data);
    return ret;
}

int32_t PinSetStrength(DevHandle handle, uint32_t strength)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("%s: handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: failed to obtain data buf", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("%s: write handle fail", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint32(data, strength)) {
        HDF_LOGE("%s: write strength fail", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_SET_STRENGTH, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_SET_STRENGTH service process fail:%d", __func__, ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t PinGetStrength(DevHandle handle, uint32_t *strength)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL || strength == NULL) {
        HDF_LOGE("%s: handle or strength is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: failed to obtain data buf", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("%s: failed to obtain reply buf", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("%s: write handle fail", __func__);
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET_STRENGTH, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_GET_STRENGTH service process fail:%d", __func__, ret);
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return ret;
    }

    if (!HdfSbufReadUint32(reply, strength)) {
        HDF_LOGE("%s: read strength fail", __func__);
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

int32_t PinSetFunc(DevHandle handle, const char *funcName)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL || funcName == NULL) {
        HDF_LOGE("%s:  handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: failed to obtain write buf", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("%s: write handle fail", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteString(data, funcName)) {
        HDF_LOGE("%s: write funcName failed", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_SET_FUNC, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_SET_FUNC service process fail:%d", __func__, ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

static int32_t CopyFuncName(const char *pinName, const char *tempName, const char **funcName)
{
    struct DListHead *head = &g_listHead;
    struct PinInfo *pos = NULL;
    struct PinInfo *tmp = NULL;

    (void)OsalSpinLock(&g_listLock);
    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, head, struct PinInfo, node) {
        if (strcmp(pos->pinName, pinName) == 0) {
            if (strcpy_s(pos->funcName, FUNC_NAME_LEN, tempName) != EOK) {
                HDF_LOGE("%s:copy tempName fail", __func__);
                (void)OsalSpinUnlock(&g_listLock);
                return HDF_FAILURE;
            }
            *funcName = (const char *)&pos->funcName;
            (void)OsalSpinUnlock(&g_listLock);
            return HDF_SUCCESS;
        }
    }
    (void)OsalSpinUnlock(&g_listLock);
    return HDF_FAILURE;
}

int32_t PinGetFunc(DevHandle handle, const char **funcName)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfSBuf *data = NULL;
    const char *tempName = NULL;

    if (handle == NULL || funcName == NULL) {
        HDF_LOGE("%s: handle or funcName is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: service is invalid!", __func__);
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("%s: failed to obtain data buf", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("%s: failed to obtain reply buf", __func__);
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("%s: write handle fail", __func__);
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET_FUNC, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PIN_IO_GET_FUNC service process fail:%d", __func__, ret);
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return ret;
    }

    tempName = HdfSbufReadString(reply);
    if (tempName == NULL) {
        HDF_LOGE("%s: tempName is NULL", __func__);
        return HDF_ERR_IO;
    }
    ret = CopyFuncName((const char *)handle, tempName, funcName);
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

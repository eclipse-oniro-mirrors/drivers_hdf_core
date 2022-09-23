/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pin/pin_core.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "pin_if.h"

#define HDF_LOG_TAG pin_core

#define PIN_MAX_CNT_PER_CNTLR    32

struct PinManager {
    struct IDeviceIoService service;
    struct HdfDeviceObject *device;
    struct DListHead cntlrListHead;
    OsalSpinlock listLock;
    uint32_t irqSave;
};

static struct PinManager *g_pinmanager = NULL;

static struct DListHead *PinCntlrListGet(void)
{
    int32_t ret;
    static struct DListHead *head = NULL;
    if (head == NULL) {
        head = &g_pinmanager->cntlrListHead;
        DListHeadInit(head);
    }
    ret = OsalSpinLockIrqSave(&g_pinmanager->listLock, &g_pinmanager->irqSave);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: spin lock save fail", __func__);
        return NULL;
    }
    return head;
}

static void PinCntlrListPut(void)
{
    int32_t ret;
    ret = OsalSpinUnlockIrqRestore(&g_pinmanager->listLock, &g_pinmanager->irqSave);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: spin lock restore fail", __func__);
        return;
    }
}

int32_t PinCntlrAdd(struct PinCntlr *cntlr)
{
    struct DListHead *head = NULL;

    if (cntlr == NULL) {
        HDF_LOGE("%s: invalid object cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    DListHeadInit(&cntlr->node);

    if (cntlr->method == NULL) {
        HDF_LOGE("%s: no method supplied!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->pinCount >= PIN_MAX_CNT_PER_CNTLR) {
        HDF_LOGE("%s: invalid pinCount:%u", __func__, cntlr->pinCount);
        return HDF_ERR_INVALID_PARAM;
    }

    OsalSpinInit(&cntlr->spin);

    head = PinCntlrListGet();
    DListInsertTail(&cntlr->node, head);
    PinCntlrListPut();
    return HDF_SUCCESS;
}

void PinCntlrRemove(struct PinCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return;
    }

    (void)PinCntlrListGet();
    DListRemove(&cntlr->node);
    PinCntlrListPut();
    (void)OsalSpinDestroy(&cntlr->spin);
}

struct PinDesc *PinCntlrGetPinDescByName(const char *pinName)
{
    struct DListHead *head = NULL;
    struct PinCntlr *cntlr = NULL;
    struct PinCntlr *tmp = NULL;
    uint16_t num;

    if (pinName == NULL) {
        HDF_LOGE("%s: pinName is NULL!", __func__);
        return NULL;
    }

    head = PinCntlrListGet();

    DLIST_FOR_EACH_ENTRY_SAFE(cntlr, tmp, head, struct PinCntlr, node) {
        for (num = 0; num < cntlr->pinCount; num++) {
            if (cntlr->pins[num].pinName == NULL) {
                continue;
            }
            if (!strcmp(cntlr->pins[num].pinName, pinName)) {
                PinCntlrListPut();
                HDF_LOGI("%s: cntlr->pins[%d].pinName is %s!", __func__, num, cntlr->pins[num].pinName);
                return &cntlr->pins[num];
            }
        }
    }
    PinCntlrListPut();
    HDF_LOGE("%s: pinName:%s doesn't matching!", __func__, pinName);
    return NULL;
}

struct PinCntlr *PinCntlrGetByNumber(uint16_t number)
{
    struct DListHead *head = NULL;
    struct PinCntlr *cntlr = NULL;
    struct PinCntlr *tmp = NULL;

    head = PinCntlrListGet();

    DLIST_FOR_EACH_ENTRY_SAFE(cntlr, tmp, head, struct PinCntlr, node) {
        if (cntlr->number == number) {
            PinCntlrListPut();
            HDF_LOGI("%s: get cntlr by number success!", __func__);
            return cntlr;
        }
    }
    PinCntlrListPut();
    HDF_LOGE("%s: get cntlr by number error!", __func__);
    return NULL;
}

struct PinCntlr *PinCntlrGetByPin(const struct PinDesc *desc)
{
    struct DListHead *head = NULL;
    struct PinCntlr *cntlr = NULL;
    struct PinCntlr *tmp = NULL;
    int32_t num;

    head = PinCntlrListGet();

    DLIST_FOR_EACH_ENTRY_SAFE(cntlr, tmp, head, struct PinCntlr, node) {
        for (num = 0; num < cntlr->pinCount; num++) {
            if (desc == &cntlr->pins[num]) {
                PinCntlrListPut();
                HDF_LOGI("%s: get cntlr by desc success!", __func__);
                return cntlr;
            }
        }
    }
    PinCntlrListPut();
    HDF_LOGE("%s: pinCtrl:%s not in any controllers!", __func__, desc->pinName);
    return NULL;
}

static int32_t GetPinIndex(struct PinCntlr *cntlr, struct PinDesc *desc)
{
    uint16_t index;
    int32_t ret;

    for (index = 0; index < cntlr->pinCount; index++) {
        if (cntlr->pins[index].pinName == NULL) {
            HDF_LOGE("%s: cntlr->pin[index].pinName is NULL!", __func__);
            break;
        }
        ret = strcmp(cntlr->pins[index].pinName, desc->pinName);
        if (ret == 0) {
            HDF_LOGI("%s: get pin index:%hu success!", __func__, index);
            return (int32_t)index;
        }
    }
    HDF_LOGE("%s:  get pin index failed!", __func__);
    return HDF_ERR_INVALID_PARAM;
}

void PinCntlrPutPin(const struct PinDesc *desc)
{
    (void)desc;
}

int32_t PinCntlrSetPinPull(struct PinCntlr *cntlr, struct PinDesc *desc, enum PinPullType pullType)
{
    int32_t ret;
    uint32_t index;

    if (cntlr == NULL) {
        HDF_LOGE("%s: invalid object cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->method == NULL || cntlr->method->SetPinPull == NULL) {
        HDF_LOGE("%s: method or SetPinPull is NULL", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (desc == NULL) {
        HDF_LOGE("%s: desc is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    index = (uint32_t)GetPinIndex(cntlr, desc);
    if (index < HDF_SUCCESS) {
        HDF_LOGE("%s: get pin index fail!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLockIrqSave(&cntlr->spin, &g_pinmanager->irqSave);
    ret = cntlr->method->SetPinPull(cntlr, index, pullType);
    (void)OsalSpinUnlockIrqRestore(&cntlr->spin, &g_pinmanager->irqSave);
    return ret;
}

int32_t PinCntlrGetPinPull(struct PinCntlr *cntlr, struct PinDesc *desc, enum PinPullType *pullType)
{
    int32_t ret;
    uint32_t index;

    if (cntlr == NULL) {
        HDF_LOGE("%s: invalid object cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->method == NULL || cntlr->method->GetPinPull == NULL) {
        HDF_LOGE("%s: method or GetPinPull is NULL", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (desc == NULL) {
        HDF_LOGE("%s: desc is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (pullType == NULL) {
        HDF_LOGE("%s: pullType is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    index = (uint32_t)GetPinIndex(cntlr, desc);
    if (index < HDF_SUCCESS) {
        HDF_LOGE("%s: get pin index failed!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLockIrqSave(&cntlr->spin, &g_pinmanager->irqSave);
    ret = cntlr->method->GetPinPull(cntlr, index, pullType);
    (void)OsalSpinUnlockIrqRestore(&cntlr->spin, &g_pinmanager->irqSave);

    return ret;
}

int32_t PinCntlrSetPinStrength(struct PinCntlr *cntlr, struct PinDesc *desc, uint32_t strength)
{
    int32_t ret;
    uint32_t index;

    if (cntlr == NULL) {
        HDF_LOGE("%s: invalid object cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->method == NULL || cntlr->method->SetPinStrength == NULL) {
        HDF_LOGE("%s: method or SetStrength is NULL", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (desc == NULL) {
        HDF_LOGE("%s: desc is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    index = (uint32_t)GetPinIndex(cntlr, desc);
    if (index < HDF_SUCCESS) {
        HDF_LOGE("%s: get pin index fail!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLockIrqSave(&cntlr->spin, &g_pinmanager->irqSave);
    ret = cntlr->method->SetPinStrength(cntlr, index, strength);
    (void)OsalSpinUnlockIrqRestore(&cntlr->spin, &g_pinmanager->irqSave);
    return ret;
}

int32_t PinCntlrGetPinStrength(struct PinCntlr *cntlr, struct PinDesc *desc, uint32_t *strength)
{
    int32_t ret;
    uint32_t index;

    if (cntlr == NULL) {
        HDF_LOGE("%s: invalid object cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->method == NULL || cntlr->method->GetPinStrength == NULL) {
        HDF_LOGE("%s: method or GetStrength is NULL", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (desc == NULL) {
        HDF_LOGE("%s: desc is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (strength == NULL) {
        HDF_LOGE("%s: strength is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    index = (uint32_t)GetPinIndex(cntlr, desc);
    if (index < HDF_SUCCESS) {
        HDF_LOGE("%s: get pin index failed!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLockIrqSave(&cntlr->spin, &g_pinmanager->irqSave);
    ret = cntlr->method->GetPinStrength(cntlr, index, strength);
    (void)OsalSpinUnlockIrqRestore(&cntlr->spin, &g_pinmanager->irqSave);

    return ret;
}

int32_t PinCntlrSetPinFunc(struct PinCntlr *cntlr, struct PinDesc *desc, const char *funcName)
{
    int32_t ret;
    uint32_t index;

    if (cntlr == NULL) {
        HDF_LOGE("%s: invalid object cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->method == NULL || cntlr->method->SetPinFunc == NULL) {
        HDF_LOGE("%s: method or SetPinFunc is NULL", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (desc == NULL) {
        HDF_LOGE("%s: desc is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    index = (uint32_t)GetPinIndex(cntlr, desc);
    if (index < HDF_SUCCESS) {
        HDF_LOGE("%s: get pin index failed!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (funcName == NULL) {
        HDF_LOGE("%s: invalid funcName pointer", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLockIrqSave(&cntlr->spin, &g_pinmanager->irqSave);
    ret = cntlr->method->SetPinFunc(cntlr, index, funcName);
    (void)OsalSpinUnlockIrqRestore(&cntlr->spin, &g_pinmanager->irqSave);
    return ret;
}

int32_t PinCntlrGetPinFunc(struct PinCntlr *cntlr, struct PinDesc *desc, const char **funcName)
{
    int32_t ret;
    uint32_t index;

    if (cntlr == NULL) {
        HDF_LOGE("%s: invalid object cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->method == NULL || cntlr->method->GetPinFunc == NULL) {
        HDF_LOGE("%s: method or SetPinFunc is NULL", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (desc == NULL) {
        HDF_LOGE("%s: desc is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    index = (uint32_t)GetPinIndex(cntlr, desc);
    if (index < HDF_SUCCESS) {
        HDF_LOGE("%s: get pin index failed!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLockIrqSave(&cntlr->spin, &g_pinmanager->irqSave);
    ret = cntlr->method->GetPinFunc(cntlr, index, funcName);
    (void)OsalSpinUnlockIrqRestore(&cntlr->spin, &g_pinmanager->irqSave);
    return ret;
}

static DevHandle HandleByData(struct HdfSBuf *data)
{
    const char *pinNameData = NULL;

    pinNameData = HdfSbufReadString(data);
    if (pinNameData == NULL) {
        HDF_LOGE("%s: pinNameData is null", __func__);
        return NULL;
    }

    return PinCntlrGetPinDescByName(pinNameData);
}

static int32_t PinIoGet(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    const char *pinNameData = NULL;
    DevHandle handle = NULL;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("%s: data is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    pinNameData = HdfSbufReadString(data);
    if (pinNameData == NULL) {
        HDF_LOGE("%s: pinNameData is null", __func__);
        return HDF_ERR_IO;
    }

    handle = PinGet(pinNameData);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t PinIoPut(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    DevHandle handle = NULL;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("%s: data is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    handle = HandleByData(data);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    PinPut(handle);
    return HDF_SUCCESS;
}

static int32_t PinIoSetPull(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t pullType;
    DevHandle handle = NULL;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("%s: data is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    handle = HandleByData(data);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &pullType)) {
        HDF_LOGE("%s: read pin pulltype fail", __func__);
        return HDF_ERR_IO;
    }

    ret = PinSetPull(handle, pullType);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pin set pull fail:%d", __func__, ret);
        return ret;
    }
    return ret;
}

static int32_t PinIoGetPull(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t pullType;
    DevHandle handle = NULL;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("%s: data or reply is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    handle = HandleByData(data);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PinGetPull(handle, &pullType);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pin get pull fail:%d", __func__, ret);
        return ret;
    }

    if (!HdfSbufWriteUint32(reply, pullType)) {
        HDF_LOGE("%s: write pin pulltype fail", __func__);
        return HDF_ERR_IO;
    }
    return ret;
}

static int32_t PinIoSetStrength(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t strength;
    DevHandle handle = NULL;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("%s: data is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    handle = HandleByData(data);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &strength)) {
        HDF_LOGE("%s: read pin strength fail", __func__);
        return HDF_ERR_IO;
    }

    ret = PinSetStrength(handle, strength);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pin set strength fail:%d", __func__, ret);
        return ret;
    }
    return ret;
}

static int32_t PinIoGetStrength(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t strength;
    DevHandle handle = NULL;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("%s: data or reply is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    handle = HandleByData(data);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PinGetStrength(handle, &strength);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pin get strength fail:%d", __func__, ret);
        return ret;
    }

    if (!HdfSbufWriteUint32(reply, strength)) {
        HDF_LOGE("%s: write pin strength fail", __func__);
        return HDF_ERR_IO;
    }
    return ret;
}

static int32_t PinIoSetFunc(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    DevHandle handle = NULL;
    const char *funcNameData = NULL;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("%s: data is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    handle = HandleByData(data);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    funcNameData = HdfSbufReadString(data);
    if (funcNameData == NULL) {
        HDF_LOGE("%s: funcnamedata is null", __func__);
        return HDF_ERR_IO;
    }

    ret = PinSetFunc(handle, funcNameData);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pin set func fail:%d", __func__, ret);
        return ret;
    }
    return ret;
}

static int32_t PinIoGetFunc(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    DevHandle handle = NULL;
    char *funcName = NULL;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("%s: data or reply is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    handle = HandleByData(data);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PinGetFunc(handle, (const char **)&funcName);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pin get func fail:%d", __func__, ret);
        return ret;
    }

    if (!HdfSbufWriteString(reply, funcName)) {
        HDF_LOGE("%s: write pin funcName fail", __func__);
        return HDF_ERR_IO;
    }
    return ret;
}

int32_t PinIoManagerDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;

    (void)client;
    switch (cmd) {
        case PIN_IO_GET:
            return PinIoGet(data, reply);
        case PIN_IO_PUT:
            return PinIoPut(data, reply);
        case PIN_IO_SET_PULL:
            return PinIoSetPull(data, reply);
        case PIN_IO_GET_PULL:
            return PinIoGetPull(data, reply);
        case PIN_IO_SET_STRENGTH:
            return PinIoSetStrength(data, reply);
        case PIN_IO_GET_STRENGTH:
            return PinIoGetStrength(data, reply);
        case PIN_IO_SET_FUNC:
            return PinIoSetFunc(data, reply);
        case PIN_IO_GET_FUNC:
            return PinIoGetFunc(data, reply);
        default:
            ret = HDF_ERR_NOT_SUPPORT;
            break;
    }
    return ret;
}

static int32_t pinManagerBind(struct HdfDeviceObject *device)
{
    struct PinManager *manager = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    manager = (struct PinManager *)OsalMemCalloc(sizeof(*manager));
    if (manager == NULL) {
        HDF_LOGE("%s: malloc manager fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    manager->device = device;
    device->service = &manager->service;
    device->service->Dispatch = PinIoManagerDispatch;
    g_pinmanager = manager;
    HDF_LOGI("%s: pin manager bind success", __func__);
    return HDF_SUCCESS;
}

static int32_t pinManagerInit(struct HdfDeviceObject *device)
{
    (void)device;
    OsalSpinInit(&g_pinmanager->listLock);
    return HDF_SUCCESS;
}

static void pinManagerRelease(struct HdfDeviceObject *device)
{
    struct PinManager *manager = NULL;

    HDF_LOGI("pinManagerRelease: enter");
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return;
    }
    manager = (struct PinManager *)device->service;
    if (manager == NULL) {
        HDF_LOGE("%s: no manager binded", __func__);
        return;
    }
    OsalMemFree(manager);
    g_pinmanager = NULL;
}

struct HdfDriverEntry g_pinManagerEntry = {
    .moduleVersion = 1,
    .Bind = pinManagerBind,
    .Init = pinManagerInit,
    .Release = pinManagerRelease,
    .moduleName = "HDF_PLATFORM_PIN_MANAGER",
};
HDF_INIT(g_pinManagerEntry);

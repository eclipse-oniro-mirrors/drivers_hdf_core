/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "device_resource_if.h"
#include "hdf_log.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "pin/pin_core.h"

#define HDF_LOG_TAG pin_virtual

#define VIRTUAL_PIN_FUNC_MAX  6
#define VIRTUAL_PIN_MAX_NUMBER 16
#define VIRTUAL_PIN_MAX_PIN_COUNT 32


struct VirtualPinDesc {
    const char *pinName;
    uint32_t init;
    uint32_t index;
    int32_t pullType;
    int32_t strength;
    const char *curFunc;
    const char *func[VIRTUAL_PIN_FUNC_MAX];
};

struct VirtualPinCntlr {
    struct PinCntlr cntlr;
    struct VirtualPinDesc *desc;
    uint16_t number;
    uint32_t pinCount;
};

static int32_t VirtualPinSetPull(struct PinCntlr *cntlr, uint32_t index, enum PinPullType pullType)
{
    struct VirtualPinCntlr *virtual = NULL;

    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (index >= cntlr->pinCount) {
        HDF_LOGE("%s: invalid index:%u", __func__, index);
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualPinCntlr *)cntlr;
    virtual->desc[index].pullType = pullType;
    return HDF_SUCCESS;
}

static int32_t VirtualPinGetPull(struct PinCntlr *cntlr, uint32_t index, enum PinPullType *pullType)
{
    struct VirtualPinCntlr *virtual = NULL;

    if (cntlr == NULL || pullType == NULL) {
        HDF_LOGE("%s: cntlr or pullType is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (index >= cntlr->pinCount) {
        HDF_LOGE("%s: invalid index:%u", __func__, index);
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualPinCntlr *)cntlr;
    *pullType = virtual->desc[index].pullType;
    return HDF_SUCCESS;
}

static int32_t VirtualPinSetStrength(struct PinCntlr *cntlr, uint32_t index, uint32_t strength)
{
    struct VirtualPinCntlr *virtual = NULL;

    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (index >= cntlr->pinCount) {
        HDF_LOGE("%s: invalid index:%u", __func__, index);
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualPinCntlr *)cntlr;
    virtual->desc[index].strength = strength;
    return HDF_SUCCESS;
}

static int32_t VirtualPinGetStrength(struct PinCntlr *cntlr, uint32_t index, uint32_t *strength)
{
    struct VirtualPinCntlr *virtual = NULL;

    if (cntlr == NULL || strength == NULL) {
        HDF_LOGE("%s: cntlr or strength is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (index >= cntlr->pinCount) {
        HDF_LOGE("%s: invalid index:%u", __func__, index);
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualPinCntlr *)cntlr;
    *strength = virtual->desc[index].strength;
    return HDF_SUCCESS;
}

static int32_t VirtualPinSetFunc(struct PinCntlr *cntlr, uint32_t index, const char *funcName)
{
    int ret;
    uint32_t funcNum;
    struct VirtualPinCntlr *virtual = NULL;

    if (cntlr == NULL || funcName == NULL) {
        HDF_LOGE("%s: cntlr or funcName is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (index >= cntlr->pinCount) {
        HDF_LOGE("%s: invalid index:%u", __func__, index);
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualPinCntlr *)cntlr;

    for (funcNum = 0; funcNum < VIRTUAL_PIN_FUNC_MAX; funcNum++) {
        ret = strcmp(funcName, virtual->desc[index].func[funcNum]);
        if (ret == 0) {
            virtual->desc[index].curFunc = virtual->desc[index].func[funcNum];
            return HDF_SUCCESS;
        }
    }
    HDF_LOGE("%s: set pin function failed.", __func__);
    return HDF_ERR_IO;
}

static int32_t VirtualPinGetFunc(struct PinCntlr *cntlr, uint32_t index, const char **funcName)
{
    struct VirtualPinCntlr *virtual = NULL;

    if (cntlr == NULL || funcName == NULL) {
        HDF_LOGE("%s: cntlr or funcName is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (index >= cntlr->pinCount) {
        HDF_LOGE("%s: invalid index:%u", __func__, index);
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualPinCntlr *)cntlr;
    *funcName = virtual->desc[index].curFunc;
    HDF_LOGD("%s: get pin function success.", __func__);
    return HDF_SUCCESS;
}

static struct PinCntlrMethod g_method = {
    .SetPinPull = VirtualPinSetPull,
    .GetPinPull = VirtualPinGetPull,
    .SetPinStrength = VirtualPinSetStrength,
    .GetPinStrength = VirtualPinGetStrength,
    .SetPinFunc = VirtualPinSetFunc,
    .GetPinFunc = VirtualPinGetFunc,
};

static int32_t VirtualPinReadFunc(struct VirtualPinDesc *desc,
    const struct DeviceResourceNode *node, struct DeviceResourceIface *drsOps)
{
    int32_t ret;
    uint32_t funcNum = 0;
    
    ret = drsOps->GetString(node, "F0", &desc->func[funcNum], "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read F0 failed", __func__);
        return ret;
    }

    funcNum++;
    ret = drsOps->GetString(node, "F1", &desc->func[funcNum], "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read F1 failed", __func__);
        return ret;
    }

    funcNum++;
    ret = drsOps->GetString(node, "F2", &desc->func[funcNum], "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read F2 failed", __func__);
        return ret;
    }

    funcNum++;
    ret = drsOps->GetString(node, "F3", &desc->func[funcNum], "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read F3 failed", __func__);
        return ret;
    }

    funcNum++;
    ret = drsOps->GetString(node, "F4", &desc->func[funcNum], "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read F4 failed", __func__);
        return ret;
    }

    funcNum++;
    ret = drsOps->GetString(node, "F5", &desc->func[funcNum], "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read F5 failed", __func__);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t VirtualPinParsePinNode(const struct DeviceResourceNode *node,
    struct VirtualPinCntlr *virtual, int32_t index)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetString == NULL) {
        HDF_LOGE("%s: invalid drs or ops!", __func__);
        return HDF_FAILURE;
    }
    ret = drsOps->GetString(node, "pinName", &virtual->desc[index].pinName, "NULL");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read pinName failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "init", &virtual->desc[index].init, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read init failed", __func__);
        return ret;
    }

    ret = VirtualPinReadFunc(&virtual->desc[index], node, drsOps);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s:Pin read func failed", __func__);
        return ret;
    }
    virtual->cntlr.pins[index].pinName = virtual->desc[index].pinName;
    virtual->cntlr.pins[index].priv = (void *)node;
    virtual->desc[index].curFunc = virtual->desc[index].func[0]; // default func
    HDF_LOGD("%s:Pin Parse Pin Node success. %s", __func__, virtual->cntlr.pins[index].pinName);
    return HDF_SUCCESS;
}

static int32_t VirtualPinCntlrInit(struct HdfDeviceObject *device, struct VirtualPinCntlr *virtual)
{
    struct DeviceResourceIface *drsOps = NULL;
    int32_t ret;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetUint16 == NULL) {
        HDF_LOGE("%s: invalid drs ops fail!", __func__);
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint16(device->property, "number", &virtual->number, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read number failed", __func__);
        return ret;
    }
    if (virtual->number > VIRTUAL_PIN_MAX_NUMBER) {
        HDF_LOGE("%s: invalid number:%hu", __func__, virtual->number);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = drsOps->GetUint32(device->property, "pinCount", &virtual->pinCount, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read pinCount failed", __func__);
        return ret;
    }
    if (virtual->pinCount >= VIRTUAL_PIN_MAX_PIN_COUNT) {
        HDF_LOGE("%s: invalid pinCount:%u", __func__, virtual->pinCount);
        return HDF_ERR_INVALID_PARAM;
    }

    virtual->cntlr.pinCount = virtual->pinCount;
    virtual->cntlr.number = virtual->number;
    virtual->desc = (struct VirtualPinDesc *)OsalMemCalloc(sizeof(struct VirtualPinDesc) * virtual->pinCount);
    if (virtual->desc == NULL) {
        HDF_LOGE("%s: memcalloc virtual desc failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    virtual->cntlr.pins = (struct PinDesc *)OsalMemCalloc(sizeof(struct PinDesc) * virtual->pinCount);
    if (virtual->cntlr.pins == NULL) {
        HDF_LOGE("%s: memcalloc virtual cntlr pins failed", __func__);
        OsalMemFree(virtual->desc);
        return HDF_ERR_MALLOC_FAIL;
    }
    return HDF_SUCCESS;
}

static int32_t VirtualPinBind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int32_t VirtualPinInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    int32_t index = 0;
    const struct DeviceResourceNode *childNode = NULL;
    struct VirtualPinCntlr *virtual = NULL;

    virtual = (struct VirtualPinCntlr *)OsalMemCalloc(sizeof(*virtual));
    if (virtual == NULL) {
        HDF_LOGE("%s: memcalloc virtual failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = VirtualPinCntlrInit(device, virtual);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pin cntlr init failed", __func__);
        OsalMemFree(virtual);
        return ret;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        ret = VirtualPinParsePinNode(childNode, virtual, index);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: pin parse node failed", __func__);
            OsalMemFree(virtual);
            OsalMemFree(virtual->desc);
            OsalMemFree(virtual->cntlr.pins);
            return ret;
        }
        index++;
    }

    virtual->cntlr.method = &g_method;
    ret = PinCntlrAdd(&virtual->cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: add pin cntlr failed", __func__);
        OsalMemFree(virtual);
        OsalMemFree(virtual->desc);
        OsalMemFree(virtual->cntlr.pins);
        return ret;
    }
    HDF_LOGI("%s: pin init success", __func__);
    return HDF_SUCCESS;
}

static void VirtualPinRelease(struct HdfDeviceObject *device)
{
    int32_t ret;
    uint16_t number;
    struct PinCntlr *cntlr = NULL;
    struct VirtualPinCntlr *virtual = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    HDF_LOGI("%s: enter", __func__);
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null", __func__);
        return;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint16 == NULL) {
        HDF_LOGE("%s: invalid drs ops", __func__);
        return;
    }
   
    ret = drsOps->GetUint16(device->property, "number", &number, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read cntlr number failed", __func__);
        return;
    }

    cntlr = PinCntlrGetByNumber(number);
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL", __func__);
        return;
    }

    PinCntlrRemove(cntlr);
    virtual = (struct VirtualPinCntlr *)cntlr;
    OsalMemFree(virtual->desc);
    OsalMemFree(virtual->cntlr.pins);
    OsalMemFree(virtual);
}

static struct HdfDriverEntry g_virtualPinDriverEntry = {
    .moduleVersion = 1,
    .Bind = VirtualPinBind,
    .Init = VirtualPinInit,
    .Release = VirtualPinRelease,
    .moduleName = "virtual_pin_driver",
};
HDF_INIT(g_virtualPinDriverEntry);

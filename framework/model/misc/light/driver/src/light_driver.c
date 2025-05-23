/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "light_driver.h"
#include <securec.h>
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "osal_mem.h"
#include "osal_mutex.h"

#define HDF_LOG_TAG    khdf_light_driver

#define LIGHT_WORK_QUEUE_NAME    "light_queue"

struct LightDriverData *g_lightDrvData = NULL;

static struct LightDriverData *GetLightDrvData(void)
{
    return g_lightDrvData;
}

static int32_t GetAllLightInfo(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)data;
    uint32_t i;
    struct LightInfo lightInfo;
    struct LightDriverData *drvData = NULL;

    drvData = GetLightDrvData();
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(reply, HDF_ERR_INVALID_PARAM);

    if (!HdfSbufWriteUint32(reply, drvData->lightNum)) {
        HDF_LOGE("%s: write sbuf failed", __func__);
        return HDF_FAILURE;
    }

    for (i = 0; i < LIGHT_ID_BUTT; ++i) {
        if (drvData->info[i] == NULL) {
            continue;
        }
        lightInfo.lightId = i;

        if (!HdfSbufWriteUint32(reply, lightInfo.lightId)) {
            HDF_LOGE("%s: write lightId failed", __func__);
            return HDF_FAILURE;
        }

        if (strcpy_s(lightInfo.lightName, NAME_MAX_LEN, drvData->info[i]->lightInfo.lightName) != EOK) {
            HDF_LOGE("%s:copy lightName failed!", __func__);
            return HDF_FAILURE;
        }

        if (!HdfSbufWriteString(reply, (const char *)lightInfo.lightName)) {
            HDF_LOGE("%s: write lightName failed", __func__);
            return HDF_FAILURE;
        }

        lightInfo.lightNumber = drvData->info[i]->lightInfo.lightNumber;
        if (!HdfSbufWriteUint32(reply, lightInfo.lightNumber)) {
            HDF_LOGE("%s: write lightNumber failed", __func__);
            return HDF_FAILURE;
        }

        lightInfo.lightType = HDF_LIGHT_TYPE_RGB_COLOR;
        if (!HdfSbufWriteUint32(reply, lightInfo.lightType)) {
            HDF_LOGE("%s: write lightType failed", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t WriteGpio(int32_t busNum, uint32_t lightOn)
{
    int32_t level;

    if (busNum < 0) {
        HDF_LOGE("%s: GPIO is wrong", __func__);
        return HDF_SUCCESS;
    }

    if (lightOn == LIGHT_STATE_START) {
        level = GPIO_VAL_HIGH;
    } else {
        level = GPIO_VAL_LOW;
    }

    return GpioWrite(busNum, level);
}

static int32_t UpdateLight(uint32_t lightId, uint32_t lightOn)
{
    int32_t ret;
    uint32_t lightBrightness;
    struct LightDriverData *drvData = NULL;

    drvData = GetLightDrvData();
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    if (drvData->info[lightId]->lightBrightness == 0) {
        lightBrightness = drvData->info[lightId]->defaultBrightness;
    } else {
        lightBrightness = drvData->info[lightId]->lightBrightness;
    }

    if ((lightBrightness & LIGHT_MAKE_B_BIT) != 0) {
        ret = WriteGpio(drvData->info[lightId]->busBNum, lightOn);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: write blue light's gpio failed", __func__);
            return HDF_FAILURE;
        }
    }

    if ((lightBrightness & LIGHT_MAKE_G_BIT) != 0) {
        ret = WriteGpio(drvData->info[lightId]->busGNum, lightOn);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: write green light's gpio failed", __func__);
            return HDF_FAILURE;
        }
    }

    if ((lightBrightness & LIGHT_MAKE_R_BIT) != 0) {
        ret = WriteGpio(drvData->info[lightId]->busRNum, lightOn);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: write red light's gpio failed", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

int32_t StartLight(uint32_t lightId)
{
    return UpdateLight(lightId, LIGHT_STATE_START);
}

int32_t StopLight(uint32_t lightId)
{
    return UpdateLight(lightId, LIGHT_STATE_STOP);
}

void LightTimerEntry(uintptr_t para)
{
    uint32_t duration;
    uint32_t lightId;
    struct LightDriverData *drvData = NULL;

    drvData = GetLightDrvData();
    if (drvData == NULL) {
        HDF_LOGE("%s: drvData is null", __func__);
        return;
    }

    lightId = (uint32_t)para;
    drvData->lightId = lightId;

    if (drvData->info[lightId]->lightState == LIGHT_STATE_START) {
        duration = drvData->info[lightId]->offTime;
    }
    if (drvData->info[lightId]->lightState == LIGHT_STATE_STOP) {
        duration = drvData->info[lightId]->onTime;
    }

    HdfAddWork(&drvData->workQueue, &drvData->work);

    if ((OsalTimerSetTimeout(&drvData->timer, duration) == HDF_SUCCESS)) {
        return;
    }

    if (drvData->timer.realTimer != NULL) {
        if (OsalTimerDelete(&drvData->timer) != HDF_SUCCESS) {
            HDF_LOGE("%s: delete light timer fail!", __func__);
        }
    }

    return;
}

static int32_t TurnOnLight(uint32_t lightId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)reply;
    uint32_t len;
    struct LightEffect *buf = NULL;
    struct LightDriverData *drvData = NULL;

    drvData = GetLightDrvData();
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    if (drvData->info[lightId] == NULL) {
        HDF_LOGE("%s: light id info is null", __func__);
        return HDF_FAILURE;
    }

    if (!HdfSbufReadBuffer(data, (const void **)&buf, &len)) {
        HDF_LOGE("%s: light read data failed", __func__);
        return HDF_FAILURE;
    }

    if (buf->lightColor.colorValue.rgbColor.r != 0) {
        drvData->info[lightId]->lightBrightness |= 0X00FF0000;
    }

    if (buf->lightColor.colorValue.rgbColor.g != 0) {
        drvData->info[lightId]->lightBrightness |= 0X0000FF00;
    }

    if (buf->lightColor.colorValue.rgbColor.b != 0) {
        drvData->info[lightId]->lightBrightness |= 0X000000FF;
    }

    if (buf->flashEffect.flashMode == LIGHT_FLASH_NONE) {
        return UpdateLight(lightId, LIGHT_STATE_START);
    }

    if (buf->flashEffect.flashMode == LIGHT_FLASH_BLINK) {
        drvData->info[lightId]->onTime = (buf->flashEffect.onTime < drvData->info[lightId]->onTime) ?
        drvData->info[lightId]->onTime : buf->flashEffect.onTime;
        drvData->info[lightId]->offTime = (buf->flashEffect.offTime < drvData->info[lightId]->offTime) ?
        drvData->info[lightId]->offTime : buf->flashEffect.offTime;

        if (OsalTimerCreate(&drvData->timer, LIGHT_WAIT_TIME, LightTimerEntry, (uintptr_t)lightId) != HDF_SUCCESS) {
            HDF_LOGE("%s: create light timer fail!", __func__);
            return HDF_FAILURE;
        }

        if (OsalTimerStartLoop(&drvData->timer) != HDF_SUCCESS) {
            HDF_LOGE("%s: start light timer fail!", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t TurnOnMultiLights(uint32_t lightId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)lightId;
    (void)data;
    (void)reply;
    HDF_LOGI("%s: temporarily not supported turn on multi lights ", __func__);
    return HDF_SUCCESS;
}

static int32_t TurnOffLight(uint32_t lightId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)data;
    (void)reply;
    struct LightDriverData *drvData = NULL;

    drvData = GetLightDrvData();
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    if (drvData->info[lightId] == NULL) {
        HDF_LOGE("%s: light id info is null", __func__);
        return HDF_FAILURE;
    }

    if (UpdateLight(lightId, LIGHT_STATE_STOP) != HDF_SUCCESS) {
        HDF_LOGE("%s: gpio write failed", __func__);
        return HDF_FAILURE;
    }

    drvData->info[lightId]->lightState = LIGHT_STATE_STOP;
    drvData->info[lightId]->lightBrightness = 0;

    if (drvData->timer.realTimer != NULL) {
        if (OsalTimerDelete(&drvData->timer) != HDF_SUCCESS) {
            HDF_LOGE("%s: delete light timer fail!", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static struct LightCmdHandleList g_lightCmdHandle[] = {
    {LIGHT_OPS_IO_CMD_ENABLE, TurnOnLight},
    {LIGHT_OPS_IO_CMD_DISABLE, TurnOffLight},
    {LIGHT_OPS_IO_CMD_ENABLE_MULTI_LIGHTS, TurnOnMultiLights},
};

static int32_t DispatchCmdHandle(uint32_t lightId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t opsCmd;
    int32_t loop;
    int32_t count;

    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(data, HDF_ERR_INVALID_PARAM);

    if (!HdfSbufReadInt32(data, &opsCmd)) {
        HDF_LOGE("%s: sbuf read opsCmd failed", __func__);
        return HDF_FAILURE;
    }

    if ((opsCmd >= LIGHT_OPS_IO_CMD_END) || (opsCmd < LIGHT_OPS_IO_CMD_ENABLE)) {
        HDF_LOGE("%s: invalid cmd = %d", __func__, opsCmd);
        return HDF_FAILURE;
    }

    count = sizeof(g_lightCmdHandle) / sizeof(g_lightCmdHandle[0]);
    for (loop = 0; loop < count; ++loop) {
        if ((opsCmd == g_lightCmdHandle[loop].cmd) && (g_lightCmdHandle[loop].func != NULL)) {
            return g_lightCmdHandle[loop].func(lightId, data, reply);
        }
    }

    return HDF_FAILURE;
}

static int32_t DispatchLight(struct HdfDeviceIoClient *client,
    int32_t cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t lightId;
    struct LightDriverData *drvData = NULL;

    drvData = GetLightDrvData();
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(client, HDF_ERR_INVALID_PARAM);

    if (cmd >= LIGHT_IO_CMD_END) {
        HDF_LOGE("%s: light cmd invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (cmd == LIGHT_IO_CMD_GET_INFO_LIST) {
        CHECK_LIGHT_NULL_PTR_RETURN_VALUE(reply, HDF_ERR_INVALID_PARAM);
        return GetAllLightInfo(data, reply);
    }

    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(data, HDF_ERR_INVALID_PARAM);
    (void)OsalMutexLock(&drvData->mutex);
    if (!HdfSbufReadUint32(data, &lightId)) {
        HDF_LOGE("%s: sbuf read lightId failed", __func__);
        (void)OsalMutexUnlock(&drvData->mutex);
        return HDF_ERR_INVALID_PARAM;
    }

    if (lightId >= LIGHT_ID_BUTT) {
        HDF_LOGE("%s: light id invalid para", __func__);
        (void)OsalMutexUnlock(&drvData->mutex);
        return HDF_FAILURE;
    }

    ret = DispatchCmdHandle(lightId, data, reply);
    (void)OsalMutexUnlock(&drvData->mutex);

    return ret;
}

static int32_t GetLightBaseConfigData(const struct DeviceResourceNode *node, const struct DeviceResourceIface *parser,
    uint32_t lightId)
{
    int32_t ret;
    uint32_t *defaultBrightness = NULL;
    struct LightDriverData *drvData = NULL;
    const char *name = NULL;

    drvData = GetLightDrvData();
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(node, HDF_ERR_INVALID_PARAM);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(parser, HDF_ERR_INVALID_PARAM);

    drvData->info[lightId] = (struct LightDeviceInfo *)OsalMemCalloc(sizeof(struct LightDeviceInfo));
    if (drvData->info[lightId] == NULL) {
        HDF_LOGE("%s: malloc fail", __func__);
        return HDF_FAILURE;
    }

    ret = parser->GetUint32(node, "busRNum", (uint32_t *)&drvData->info[lightId]->busRNum, 0);
    CHECK_LIGHT_PARSER_NUM_RETURN_VALUE(ret, drvData->info[lightId]->busRNum);

    ret = parser->GetUint32(node, "busGNum", (uint32_t *)&drvData->info[lightId]->busGNum, 0);
    CHECK_LIGHT_PARSER_NUM_RETURN_VALUE(ret, drvData->info[lightId]->busGNum);

    ret = parser->GetUint32(node, "busBNum", (uint32_t *)&drvData->info[lightId]->busBNum, 0);
    CHECK_LIGHT_PARSER_NUM_RETURN_VALUE(ret, drvData->info[lightId]->busBNum);

    ret = parser->GetString(node, "lightName", &name, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s:get lightName failed!", __func__);
        return HDF_FAILURE;
    }

    if (strcpy_s(drvData->info[lightId]->lightInfo.lightName, NAME_MAX_LEN, name) != EOK) {
        HDF_LOGE("%s:copy lightName failed!", __func__);
        return HDF_FAILURE;
    }

    ret = parser->GetUint32(node, "lightNumber", (uint32_t *)&drvData->info[lightId]->lightInfo.lightNumber, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s:get lightNumber failed!", __func__);
        return HDF_FAILURE;
    }

    defaultBrightness = (uint32_t *)&drvData->info[lightId]->defaultBrightness;
    ret = parser->GetUint32(node, "defaultBrightness", defaultBrightness, 0);
    CHECK_LIGHT_PARSER_RESULT_RETURN_VALUE(ret, "defaultBrightness");
    ret = parser->GetUint32(node, "onTime", &drvData->info[lightId]->onTime, 0);
    CHECK_LIGHT_PARSER_RESULT_RETURN_VALUE(ret, "onTime");
    ret = parser->GetUint32(node, "offTime", &drvData->info[lightId]->offTime, 0);
    CHECK_LIGHT_PARSER_RESULT_RETURN_VALUE(ret, "offTime");

    drvData->info[lightId]->lightBrightness = 0;
    drvData->info[lightId]->lightState = LIGHT_STATE_STOP;

    return HDF_SUCCESS;
}

static int32_t ParseLightInfo(const struct DeviceResourceNode *node, const struct DeviceResourceIface *parser)
{
    int32_t ret;
    uint32_t i;
    uint32_t temp;
    struct LightDriverData *drvData = NULL;

    drvData = GetLightDrvData();
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(node, HDF_ERR_INVALID_PARAM);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(parser, HDF_ERR_INVALID_PARAM);

    drvData->lightNum = (uint32_t)parser->GetElemNum(node, "lightId");
    if (drvData->lightNum > LIGHT_ID_NUM) {
        HDF_LOGE("%s: lightNum cross the border", __func__);
        return HDF_FAILURE;
    }

    ret = memset_s(drvData->info, sizeof(drvData->info[LIGHT_ID_NONE]) * LIGHT_ID_BUTT, 0,
        sizeof(drvData->info[LIGHT_ID_NONE]) * LIGHT_ID_BUTT);
    CHECK_LIGHT_PARSER_RESULT_RETURN_VALUE(ret, "memset_s");

    for (i = 0; i < drvData->lightNum; ++i) {
        ret = parser->GetUint32ArrayElem(node, "lightId", i, &temp, 0);
        CHECK_LIGHT_PARSER_RESULT_RETURN_VALUE(ret, "lightId");

        if (temp >= LIGHT_ID_BUTT) {
            HDF_LOGE("%s: light id invalid para", __func__);
            return HDF_FAILURE;
        }

        ret = GetLightBaseConfigData(node, parser, temp);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: get light base config fail", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t GetLightConfigData(const struct DeviceResourceNode *node)
{
    struct DeviceResourceIface *parser = NULL;
    const struct DeviceResourceNode *light = NULL;
    const struct DeviceResourceNode *childNode = NULL;

    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(node, HDF_ERR_INVALID_PARAM);

    parser = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(parser, HDF_ERR_INVALID_PARAM);
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(parser->GetChildNode, HDF_ERR_INVALID_PARAM);

    childNode = parser->GetChildNode(node, "lightAttr");
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(childNode, HDF_ERR_INVALID_PARAM);
    light = parser->GetChildNode(childNode, "light01");
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(light, HDF_ERR_INVALID_PARAM);

    if (ParseLightInfo(light, parser) != HDF_SUCCESS) {
        HDF_LOGE("%s: ParseLightInfo  is failed!", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetLightGpioDir(const struct LightDriverData *drvData)
{
    int32_t i;
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_FAILURE);

    for (i = 0; i < LIGHT_ID_BUTT; ++i) {
        if (drvData->info[i] == NULL) {
            continue;
        }

        if (drvData->info[i]->busRNum >= 0) {
            if (GpioSetDir(drvData->info[i]->busRNum, GPIO_DIR_OUT) != HDF_SUCCESS) {
                HDF_LOGE("%s: set red light's gpio failed", __func__);
                return HDF_FAILURE;
            }
        }
        if (drvData->info[i]->busGNum >= 0) {
            if (GpioSetDir(drvData->info[i]->busGNum, GPIO_DIR_OUT) != HDF_SUCCESS) {
                HDF_LOGE("%s: set green light's gpio failed", __func__);
                return HDF_FAILURE;
            }
        }
        if (drvData->info[i]->busBNum >= 0) {
            if (GpioSetDir(drvData->info[i]->busBNum, GPIO_DIR_OUT) != HDF_SUCCESS) {
                HDF_LOGE("%s: set blue light's gpio failed", __func__);
                return HDF_FAILURE;
            }
        }
    }

    return HDF_SUCCESS;
}

int32_t BindLightDriver(struct HdfDeviceObject *device)
{
    struct LightDriverData *drvData = NULL;

    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(device, HDF_FAILURE);

    drvData = (struct LightDriverData *)OsalMemCalloc(sizeof(*drvData));
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_MALLOC_FAIL);

    drvData->ioService.Dispatch = DispatchLight;
    drvData->device = device;
    device->service = &drvData->ioService;
    g_lightDrvData = drvData;

    return HDF_SUCCESS;
}

static void LightWorkEntry(void *para)
{
    uint32_t lightId;
    struct LightDriverData *drvData = (struct LightDriverData *)para;
    CHECK_LIGHT_NULL_PTR_RETURN(drvData);
    lightId = drvData->lightId;

    if (drvData->info[lightId] == NULL) {
        HDF_LOGE("%s: lightId info is NULL!", __func__);
        return;
    }

    if (drvData->info[lightId]->lightState == LIGHT_STATE_START) {
        if (StopLight(lightId) != HDF_SUCCESS) {
        HDF_LOGE("%s: add light work fail! device state[%d]!", __func__, drvData->info[lightId]->lightState);
        }
        drvData->info[lightId]->lightState = LIGHT_STATE_STOP;
        return;
    }

    if (drvData->info[lightId]->lightState == LIGHT_STATE_STOP) {
        if (StartLight(lightId) != HDF_SUCCESS) {
        HDF_LOGE("%s: add light work fail! device state[%d]!", __func__, drvData->info[lightId]->lightState);
        }
        drvData->info[lightId]->lightState = LIGHT_STATE_START;
        return;
    }
}

int32_t InitLightDriver(struct HdfDeviceObject *device)
{
    struct LightDriverData *drvData = NULL;

    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(device, HDF_FAILURE);
    drvData = (struct LightDriverData *)device->service;
    CHECK_LIGHT_NULL_PTR_RETURN_VALUE(drvData, HDF_FAILURE);

    if (OsalMutexInit(&drvData->mutex) != HDF_SUCCESS) {
        HDF_LOGE("%s: init mutex fail!", __func__);
        return HDF_FAILURE;
    }

    if (HdfWorkQueueInit(&drvData->workQueue, LIGHT_WORK_QUEUE_NAME) != HDF_SUCCESS) {
        HDF_LOGE("%s: init workQueue fail!", __func__);
        return HDF_FAILURE;
    }

    if (HdfWorkInit(&drvData->work, LightWorkEntry, (void*)drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: init work fail!", __func__);
        return HDF_FAILURE;
    }

    if (GetLightConfigData(device->property) != HDF_SUCCESS) {
        HDF_LOGE("%s: get light config fail!", __func__);
        return HDF_FAILURE;
    }

    if (SetLightGpioDir(drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: set light gpio dir fail!", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

void ReleaseLightDriver(struct HdfDeviceObject *device)
{
    int32_t i;
    struct LightDriverData *drvData = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return;
    }

    drvData = (struct LightDriverData *)device->service;
    if (drvData == NULL) {
        HDF_LOGE("%s: drvData is null", __func__);
        return;
    }

    for (i = LIGHT_ID_NONE; i < LIGHT_ID_BUTT; ++i) {
        if (drvData->info[i] != NULL) {
            OsalMemFree(drvData->info[i]);
            drvData->info[i] = NULL;
        }
    }

    HdfWorkDestroy(&drvData->work);
    HdfWorkQueueDestroy(&drvData->workQueue);
    (void)OsalMutexDestroy(&drvData->mutex);
    OsalMemFree(drvData);
    g_lightDrvData = NULL;
}

struct HdfDriverEntry g_lightDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_LIGHT",
    .Bind = BindLightDriver,
    .Init = InitLightDriver,
    .Release = ReleaseLightDriver,
};

HDF_INIT(g_lightDriverEntry);

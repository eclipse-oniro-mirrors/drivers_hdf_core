/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "sensor_gas_driver.h"
#include <securec.h>
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "osal_math.h"
#include "osal_mem.h"
#include "sensor_config_controller.h"
#include "sensor_device_manager.h"
#include "sensor_platform_if.h"

#define HDF_LOG_TAG    khdf_sensor_gas_driver

#define HDF_GAS_WORK_QUEUE_NAME    "hdf_gas_work_queue"

static struct GasDrvData *g_gasDrvData = NULL;

static struct GasDrvData *GasGetDrvData(void)
{
    return g_gasDrvData;
}

static struct SensorRegCfgGroupNode *g_regCfgGroup[SENSOR_GROUP_MAX] = { NULL };

int32_t GasRegisterChipOps(const struct GasOpsCall *ops)
{
    struct GasDrvData *drvData = GasGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(ops, HDF_ERR_INVALID_PARAM);

    drvData->ops.Init = ops->Init;
    drvData->ops.ReadData = ops->ReadData;

    return HDF_SUCCESS;
}

static void GasDataWorkEntry(void *arg)
{
    struct GasDrvData *drvData = NULL;
    struct SensorReportEvent event;

    drvData = (struct GasDrvData *)arg;
    CHECK_NULL_PTR_RETURN(drvData);

    if (drvData->ops.ReadData == NULL) {
        HDF_LOGI("%s: Gas ReadData function NULl", __func__);
        return;
    }

    if (drvData->ops.ReadData(drvData->gasCfg, &event) != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas read data failed", __func__);
        return;
    }

    if (ReportSensorEvent(&event) != HDF_SUCCESS) {
        HDF_LOGE("%s: report gas data failed", __func__);
        return;
    }
}

static void GasTimerEntry(uintptr_t arg)
{
    int64_t interval;
    int32_t ret;
    struct GasDrvData *drvData = (struct GasDrvData *)arg;
    CHECK_NULL_PTR_RETURN(drvData);

    if (!HdfAddWork(&drvData->gasWorkQueue, &drvData->gasWork)) {
        HDF_LOGE("%s: Gas add work queue failed", __func__);
    }

    interval = OsalDivS64(drvData->interval, (SENSOR_CONVERT_UNIT * SENSOR_CONVERT_UNIT));
    interval = (interval < SENSOR_TIMER_MIN_TIME) ? SENSOR_TIMER_MIN_TIME : interval;
    ret = OsalTimerSetTimeout(&drvData->gasTimer, interval);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas modify time failed", __func__);
    }
}

static int32_t InitGasData(struct GasDrvData *drvData)
{
    if (HdfWorkQueueInit(&drvData->gasWorkQueue, HDF_GAS_WORK_QUEUE_NAME) != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas init work queue failed", __func__);
        return HDF_FAILURE;
    }

    if (HdfWorkInit(&drvData->gasWork, GasDataWorkEntry, drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas create thread failed", __func__);
        return HDF_FAILURE;
    }

    drvData->interval = SENSOR_TIMER_MIN_TIME;
    drvData->enable = false;
    drvData->detectFlag = false;

    return HDF_SUCCESS;
}

static int32_t SetGasEnable(void)
{
    int32_t ret;
    struct GasDrvData *drvData = GasGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(drvData->gasCfg, HDF_ERR_INVALID_PARAM);

    if (drvData->enable) {
        HDF_LOGE("%s: Gas sensor is enabled", __func__);
        return HDF_SUCCESS;
    }

    ret = SetSensorRegCfgArray(&drvData->gasCfg->busCfg, drvData->gasCfg->regCfgGroup[SENSOR_ENABLE_GROUP]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas sensor enable config failed", __func__);
        return ret;
    }

    ret = OsalTimerCreate(&drvData->gasTimer, SENSOR_TIMER_MIN_TIME, GasTimerEntry, (uintptr_t)drvData);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas create timer failed[%d]", __func__, ret);
        return ret;
    }

    ret = OsalTimerStartLoop(&drvData->gasTimer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas start timer failed[%d]", __func__, ret);
        return ret;
    }
    drvData->enable = true;

    return HDF_SUCCESS;
}

static int32_t SetGasDisable(void)
{
    int32_t ret;
    struct GasDrvData *drvData = GasGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(drvData->gasCfg, HDF_ERR_INVALID_PARAM);

    if (!drvData->enable) {
        HDF_LOGE("%s: Gas sensor had disable", __func__);
        return HDF_SUCCESS;
    }

    ret = SetSensorRegCfgArray(&drvData->gasCfg->busCfg, drvData->gasCfg->regCfgGroup[SENSOR_DISABLE_GROUP]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas sensor disable config failed", __func__);
        return ret;
    }

    ret = OsalTimerDelete(&drvData->gasTimer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas delete timer failed", __func__);
        return ret;
    }
    drvData->enable = false;

    return HDF_SUCCESS;
}

static int32_t SetGasBatch(int64_t samplingInterval, int64_t interval)
{
    (void)interval;

    struct GasDrvData *drvData = NULL;

    drvData = GasGetDrvData();
    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    drvData->interval = samplingInterval;

    return HDF_SUCCESS;
}

static int32_t SetGasMode(int32_t mode)
{
    if (mode <= SENSOR_WORK_MODE_DEFAULT || mode >= SENSOR_WORK_MODE_MAX) {
        HDF_LOGE("%s: The current mode is not supported", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetGasOption(uint32_t option)
{
    (void)option;
    return HDF_SUCCESS;
}

static int32_t DispatchGas(struct HdfDeviceIoClient *client,
    int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)cmd;
    (void)data;
    (void)reply;

    return HDF_SUCCESS;
}

int32_t GasBindDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN_VALUE(device, HDF_ERR_INVALID_PARAM);

    struct GasDrvData *drvData = (struct GasDrvData *)OsalMemCalloc(sizeof(*drvData));
    if (drvData == NULL) {
        HDF_LOGE("%s: Malloc gas drv data fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    drvData->ioService.Dispatch = DispatchGas;
    drvData->device = device;
    device->service = &drvData->ioService;
    g_gasDrvData = drvData;

    return HDF_SUCCESS;
}

static int32_t InitGasOps(struct SensorCfgData *config, struct SensorDeviceInfo *deviceInfo)
{
    CHECK_NULL_PTR_RETURN_VALUE(config, HDF_ERR_INVALID_PARAM);

    HDF_LOGI("%s: Sensor name [%s]about this Device", __func__, deviceInfo->sensorInfo.sensorName);

    deviceInfo->ops.Enable = SetGasEnable;
    deviceInfo->ops.Disable = SetGasDisable;
    deviceInfo->ops.SetBatch = SetGasBatch;
    deviceInfo->ops.SetMode = SetGasMode;
    deviceInfo->ops.SetOption = SetGasOption;

    if (memcpy_s(&deviceInfo->sensorInfo, sizeof(deviceInfo->sensorInfo),
        &config->sensorInfo, sizeof(config->sensorInfo)) != EOK) {
        HDF_LOGE("%s: Copy sensor info failed", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t InitGasAfterDetected(struct SensorCfgData *config)
{
    struct SensorDeviceInfo deviceInfo;
    CHECK_NULL_PTR_RETURN_VALUE(config, HDF_ERR_INVALID_PARAM);

    if (InitGasOps(config, &deviceInfo) != HDF_SUCCESS) {
        HDF_LOGE("%s: Init gas ops failed", __func__);
        return HDF_FAILURE;
    }

    if (AddSensorDevice(&deviceInfo) != HDF_SUCCESS) {
        HDF_LOGE("%s: Add gas device failed", __func__);
        return HDF_FAILURE;
    }

    if (ParseSensorDirection(config) != HDF_SUCCESS) {
        HDF_LOGE("%s: Parse gas direction failed", __func__);
        (void)DeleteSensorDevice(&config->sensorInfo);
        return HDF_FAILURE;
    }

    if (ParseSensorRegConfig(config) != HDF_SUCCESS) {
        HDF_LOGE("%s: Parse sensor register failed", __func__);
        (void)DeleteSensorDevice(&config->sensorInfo);
        ReleaseSensorAllRegConfig(config);
        ReleaseSensorDirectionConfig(config);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

struct SensorCfgData *GasCreateCfgData(const struct DeviceResourceNode *node)
{
    struct GasDrvData *drvData = GasGetDrvData();

    if (drvData == NULL || node == NULL) {
        HDF_LOGE("%s: Gas node pointer NULL", __func__);
        return NULL;
    }

    if (drvData->detectFlag) {
        HDF_LOGE("%s: Gas sensor have detected", __func__);
        return NULL;
    }

    if (drvData->gasCfg == NULL) {
        HDF_LOGE("%s: Gas gasCfg pointer NULL", __func__);
        return NULL;
    }

    if (GetSensorBaseConfigData(node, drvData->gasCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: Get sensor base config failed", __func__);
        goto BASE_CONFIG_EXIT;
    }

    if (DetectSensorDevice(drvData->gasCfg) != HDF_SUCCESS) {
        HDF_LOGI("%s: Gas sensor detect device no exist", __func__);
        drvData->detectFlag = false;
        goto BASE_CONFIG_EXIT;
    }

    drvData->detectFlag = true;
    if (InitGasAfterDetected(drvData->gasCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: Gas sensor detect device no exist", __func__);
        goto INIT_EXIT;
    }

    return drvData->gasCfg;

INIT_EXIT:
    (void)ReleaseSensorBusHandle(&drvData->gasCfg->busCfg);
BASE_CONFIG_EXIT:
    drvData->gasCfg->root = NULL;
    (void)memset_s(&drvData->gasCfg->sensorInfo, sizeof(struct SensorBasicInfo), 0, sizeof(struct SensorBasicInfo));
    (void)memset_s(&drvData->gasCfg->busCfg, sizeof(struct SensorBusCfg), 0, sizeof(struct SensorBusCfg));
    (void)memset_s(&drvData->gasCfg->sensorAttr, sizeof(struct SensorAttr), 0, sizeof(struct SensorAttr));

    return drvData->gasCfg;
}

void GasReleaseCfgData(struct SensorCfgData *gasCfg)
{
    CHECK_NULL_PTR_RETURN(gasCfg);

    (void)DeleteSensorDevice(&gasCfg->sensorInfo);
    ReleaseSensorAllRegConfig(gasCfg);
    (void)ReleaseSensorBusHandle(&gasCfg->busCfg);
    ReleaseSensorDirectionConfig(gasCfg);

    gasCfg->root = NULL;
    (void)memset_s(&gasCfg->sensorInfo, sizeof(struct SensorBasicInfo), 0, sizeof(struct SensorBasicInfo));
    (void)memset_s(&gasCfg->busCfg, sizeof(struct SensorBusCfg), 0, sizeof(struct SensorBusCfg));
    (void)memset_s(&gasCfg->sensorAttr, sizeof(struct SensorAttr), 0, sizeof(struct SensorAttr));
}

int32_t GasInitDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN_VALUE(device, HDF_ERR_INVALID_PARAM);
    struct GasDrvData *drvData = (struct GasDrvData *)device->service;
    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    if (InitGasData(drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: Init gas config failed", __func__);
        return HDF_FAILURE;
    }

    drvData->gasCfg = (struct SensorCfgData *)OsalMemCalloc(sizeof(*drvData->gasCfg));
    if (drvData->gasCfg == NULL) {
        HDF_LOGE("%s: Malloc gas config data failed", __func__);
        return HDF_FAILURE;
    }

    drvData->gasCfg->regCfgGroup = &g_regCfgGroup[0];

    HDF_LOGI("%s: Init gas driver success", __func__);

    return HDF_SUCCESS;
}

void GasReleaseDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN(device);

    struct GasDrvData *drvData = (struct GasDrvData *)device->service;
    CHECK_NULL_PTR_RETURN(drvData);

    if (drvData->detectFlag && drvData->gasCfg != NULL) {
        GasReleaseCfgData(drvData->gasCfg);
    }

    OsalMemFree(drvData->gasCfg);
    drvData->gasCfg = NULL;

    HdfWorkDestroy(&drvData->gasWork);
    HdfWorkQueueDestroy(&drvData->gasWorkQueue);
    OsalMemFree(drvData);
}

struct HdfDriverEntry g_sensorGasDevEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_SENSOR_GAS",
    .Bind = GasBindDriver,
    .Init = GasInitDriver,
    .Release = GasReleaseDriver,
};

HDF_INIT(g_sensorGasDevEntry);
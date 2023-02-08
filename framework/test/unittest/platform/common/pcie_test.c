/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pcie_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"

#define HDF_LOG_TAG pcie_test_c

#define USER_LEM_MAX    8192
#define DMA_ALIGN_SIZE  256
#define DMA_TEST_LEN    256
#define PCIE_TEST_DISABLE_ADDR 0xB7
#define PCIE_TEST_UPPER_ADDR 0x28
#define PCIE_TEST_CMD_ADDR 0x04

struct PcieTestFunc {
    int cmd;
    int32_t (*func)(struct PcieTester *tester);
};

static int32_t PcieTestGetConfig(struct PcieTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;

    service = HdfIoServiceBind("PCIE_TEST");
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("%s: bind service failed", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("%s: failed to obtain reply", __func__);
        HdfIoServiceRecycle(service);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: remote dispatch failed", __func__);
        HdfSbufRecycle(reply);
        HdfIoServiceRecycle(service);
        return ret;
    }

    if (!HdfSbufReadUint32(reply, &config->busNum)) {
        HDF_LOGE("%s: read busNum failed", __func__);
        HdfSbufRecycle(reply);
        HdfIoServiceRecycle(service);
        return HDF_ERR_IO;
    }
    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return ret;
}

static DevHandle PcieTesterGet(void)
{
    int32_t ret;
    static struct PcieTester tester = {0};

    ret = PcieTestGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read config failed:%d", __func__, ret);
        return NULL;
    }
    tester.handle = PcieOpen(tester.config.busNum);
    if (tester.handle == NULL) {
        HDF_LOGE("%s: open pcie %u failed", __func__, tester.config.busNum);
        return NULL;
    }

    return &tester;
}

static void PcieTesterPut(struct PcieTester *tester)
{
    if (tester == NULL) {
        HDF_LOGE("%s: tester is NULL", __func__);
        return;
    }
    PcieClose(tester->handle);
    tester->handle = NULL;
}

static int32_t TestPcieReadAndWrite(struct PcieTester *tester)
{
    int32_t ret;
    uint8_t disable;
    uint32_t upper;
    uint16_t cmd;

    ret = PcieRead(tester->handle, PCIE_CONFIG, PCIE_TEST_DISABLE_ADDR, &disable, sizeof(disable));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PcieRead failed ret = %d.", __func__, ret);
        return ret;
    }
    HDF_LOGD("%s: disable is %d", __func__, disable);
    ret = PcieWrite(tester->handle, PCIE_CONFIG, PCIE_TEST_DISABLE_ADDR, &disable, sizeof(disable));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PcieWrite failed ret = %d.", __func__, ret);
        return ret;
    }

    ret = PcieRead(tester->handle, PCIE_CONFIG, PCIE_TEST_UPPER_ADDR, (uint8_t *)&upper, sizeof(upper));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PcieRead failed ret = %d.", __func__, ret);
        return ret;
    }
    HDF_LOGD("%s: upper is 0x%x", __func__, upper);
    ret = PcieWrite(tester->handle, PCIE_CONFIG, PCIE_TEST_UPPER_ADDR, (uint8_t *)&upper, sizeof(upper));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PcieWrite failed ret = %d.", __func__, ret);
        return ret;
    }

    ret = PcieRead(tester->handle, PCIE_CONFIG, PCIE_TEST_CMD_ADDR, (uint8_t *)&cmd, sizeof(cmd));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PcieRead failed ret = %d.", __func__, ret);
        return ret;
    }
    HDF_LOGD("%s: cmd is 0x%x", __func__, cmd);
    ret = PcieWrite(tester->handle, PCIE_CONFIG, PCIE_TEST_CMD_ADDR, (uint8_t *)&cmd, sizeof(cmd));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: PcieWrite failed ret = %d.", __func__, ret);
    }
    return ret;
}

int32_t TestPcieDmaCb(DevHandle handle)
{
    (void)handle;
    HDF_LOGI("TestPcieDmaCb: trigger");
    return HDF_SUCCESS;
}

static int32_t TestPcieDmaMapAndUnmap(struct PcieTester *tester)
{
    int32_t ret;
    uintptr_t buf = 0;

    if (tester->handle == NULL) {
        HDF_LOGE("%s: invalid tester", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    buf = (uintptr_t)OsalMemAllocAlign(DMA_ALIGN_SIZE, DMA_TEST_LEN);
    if (buf == 0) {
        HDF_LOGE("%s: malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    /* dma to device */
    ret = PcieDmaMap(tester->handle, TestPcieDmaCb, buf, DMA_TEST_LEN, PCIE_DMA_TO_DEVICE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed ret = %d", __func__, ret);
        OsalMemFree((void *)buf);
        return ret;
    }
    PcieDmaUnmap(tester->handle, buf, DMA_TEST_LEN, PCIE_DMA_TO_DEVICE);

    /* device to dma */
    ret = PcieDmaMap(tester->handle, TestPcieDmaCb, buf, DMA_TEST_LEN, PCIE_DMA_FROM_DEVICE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed ret = %d", __func__, ret);
        OsalMemFree((void *)buf);
        return ret;
    }
    PcieDmaUnmap(tester->handle, buf, DMA_TEST_LEN, PCIE_DMA_FROM_DEVICE);
    OsalMemFree((void *)buf);

    return HDF_SUCCESS;
}

int32_t TestPcieIrqCb(DevHandle handle)
{
    (void)handle;
    HDF_LOGI("TestPcieIrqCb: trigger");
    return HDF_SUCCESS;
}

static int32_t TestPcieRegAndUnregIrq(struct PcieTester *tester)
{
    int32_t ret;
    ret = PcieRegisterIrq(tester->handle, TestPcieIrqCb);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: register irq fail", __func__);
        return ret;
    }

    PcieUnregisterIrq(tester->handle);
    return HDF_SUCCESS;
}

static struct PcieTestFunc g_entry[] = {
    { PCIE_READ_AND_WRITE_01, TestPcieReadAndWrite },
    { PCIE_DMA_MAP_AND_UNMAP_01, TestPcieDmaMapAndUnmap },
    { PCIE_REG_AND_UNREG_IRQ_01, TestPcieRegAndUnregIrq },
};

int32_t PcieTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;
    struct PcieTester *tester = NULL;

    if (cmd > PCIE_TEST_MAX) {
        HDF_LOGE("%s: invalid cmd:%d", __func__, cmd);
        return ret;
    }

    tester = PcieTesterGet();
    if (tester == NULL) {
        HDF_LOGE("%s: get tester failed", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd == cmd && g_entry[i].func != NULL) {
            ret = g_entry[i].func(tester);
            break;
        }
    }

    HDF_LOGI("[%s][======cmd:%d====ret:%d======]", __func__, cmd, ret);
    PcieTesterPut(tester);
    return ret;
}

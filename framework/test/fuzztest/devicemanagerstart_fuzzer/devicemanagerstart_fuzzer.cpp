/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "devicemanagerstart_fuzzer.h"

#include "hdf_base.h"
#include "hdf_core_log.h"
#include "hdf_sbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "devmgr_dump.h"
#include "devmgr_service.h"
#include "devmgr_service_full.h"
#include "devmgr_uevent.h"

#ifdef __cplusplus
} // extern "C"
#endif

#define HDF_LOG_TAG devicemanagerstart_fuzzer

static int DeviceManagerStartServiceFuzzTest(const uint8_t *data, size_t size)
{
    HDF_LOGI("start hdf device manager fuzzer");
    int status = HDF_FAILURE;
    struct IDevmgrService* instance = DevmgrServiceGetInstance();
    if (instance == NULL) {
        HDF_LOGE("Getting DevmgrService instance failed");
        return status;
    }

    if (instance->StartService != NULL) {
        status = instance->StartService(instance);
        HDF_LOGI("device manager fuzzer start service success");
        status = instance->StartService(nullptr);
        if (status == HDF_ERR_INVALID_PARAM) {
            HDF_LOGI("device manager fuzzer start service FAILED");
        }
    }

    (void)DevMgrUeventReceiveStart();
    DevMgrRegisterDumpFunc();
    if (status == HDF_SUCCESS) {
        struct DevmgrServiceFull *fullService = reinterpret_cast<struct DevmgrServiceFull *>(instance);
        struct HdfMessageLooper *looper = &fullService->looper;
        if ((looper != NULL) && (looper->Start != NULL)) {
            HDF_LOGI("device manager fuzzer skip loop");
        }
    }
    struct HdfSBuf *dataBuf = HdfSbufTypedObtain(SBUF_IPC);
    if (dataBuf == nullptr) {
        HDF_LOGE("%{public}s:%{public}d: failed to create data buf", __func__, __LINE__);
        return false;
    }

    if (!HdfSbufWriteBuffer(dataBuf, data, size)) {
        HDF_LOGE("%{public}s:%{public}d: failed to write data", __func__, __LINE__);
        HdfSbufRecycle(dataBuf);
        return false;
    }

    HDF_LOGI("end of hdf device manager fuzzer, status is %{public}d", status);
    return status;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int ret = HDF_FAILURE;
    if (ret == HDF_SUCCESS) {
        return ret;
    }

    ret = DeviceManagerStartServiceFuzzTest(data, size);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DeviceManagerStartServiceFuzzTest failed, ret is %{public}d", ret);
        return ret;
    }
    return HDF_SUCCESS;
}
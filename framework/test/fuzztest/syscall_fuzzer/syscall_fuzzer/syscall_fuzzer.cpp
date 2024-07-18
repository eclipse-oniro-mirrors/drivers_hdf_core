/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include "hdf_base.h"
#include "hdf_log.h"
#include <string>
#include "hdf_io_service_if.h"
#include "syscall_fuzzer.h"
#include "parcel.h"
#include "hdf_io_service.h"
#include <cstddef>
#include <cstdint>
#include "osal_mem.h"

namespace OHOS {
static void SyscallFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        HDF_LOGE("%{public}s: data is nullptr!", __func__);
    }

    struct HdfSBuf *reply = HdfSbufObtainDefaultSize();
    HdfListAllDevice(reply);
    HdfListAllService(reply);

    Parcel parcel;
    parcel.WriteBuffer(data, size);
    struct HdfDevEventlistener *listener = nullptr;
    listener = (struct HdfDevEventlistener *)OsalMemCalloc(sizeof(struct HdfDevEventlistener));
    auto servicename = parcel.ReadCString();
    struct HdfIoService *serv = HdfIoServiceBind(servicename);
    if (serv == nullptr) {
        HDF_LOGE("%{public}s: HdfIoServiceBind failed!", __func__);
    }
    if (HdfDeviceRegisterEventListener(serv, listener) == HDF_SUCCESS) {
        int ret = HdfDeviceUnregisterEventListener(serv, listener);
        if (ret != HDF_SUCCESS) {
            HdfIoServiceRecycle(serv);
        }
    }
    OsalMemFree(listener);
    HdfIoServiceRecycle(serv);
    void *para = static_cast<void *>(data);
    HdfDevEventListenTask(para);
    struct HdfDevListenerThread *thread = reinterpret_cast<struct HdfDevListenerThread *>(data);
    HdfDevEventReadAndDispatch(thread);
    GetListenerCount(thread);
    HdfAdapterStopListenIoctl(0);
    HdfAdapterExitListenIoctl(0);
    struct HdfSyscallAdapterGroup *group = reinterpret_cast<struct HdfSyscallAdapterGroup *>(data);
    HdfIoServiceGroupThreadStart(group, 0);
    HdfIoserviceGroupGetListenerCount(group);
    HdfIoserviceGroupGetServiceCount(group);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SyscallFuzzTest(data, size);
    return HDF_SUCCESS;
}
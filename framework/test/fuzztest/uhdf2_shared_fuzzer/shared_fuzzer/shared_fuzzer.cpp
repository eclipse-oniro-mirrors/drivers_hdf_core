/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include "shared_fuzzer.h"
#include "dev_attribute_serialize.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include <string>

namespace OHOS {
static void SharedFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        HDF_LOGE("%{public}s: data is nullptr!", __func__);
    }

    struct HdfDeviceInfo *attribute = (struct HdfDeviceInfo *)data;
    struct HdfSBuf *sbuf = (struct HdfSBuf *)data;
    DeviceAttributeSerialize(attribute, sbuf);
    DeviceAttributeDeserialize(sbuf);
    DeviceSerializedAttributeRelease(attribute);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{

    /* Run your code on data */
    OHOS::SharedFuzzTest(data, size);
    return HDF_SUCCESS;
}
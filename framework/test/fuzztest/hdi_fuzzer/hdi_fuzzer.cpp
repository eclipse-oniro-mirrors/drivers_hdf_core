/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdi_fuzzer.h"
#include <cstddef>
#include <cstdint>
#include "hdf_log.h"
#include "iservstat_listener_hdi.h"

using namespace OHOS::HDI::ServiceManager::V1_0;

namespace OHOS {
namespace HDI {
namespace ServiceManager {
namespace V1_0 {
void ServStatListenerFuzzer::OnReceive(const ServiceStatus &status)
{
    (void)status;
}
} // namespace V1_0
} // namespace ServiceManager
} // namespace HDI
} // namespace OHOS

namespace OHOS {
constexpr size_t THRESHOLD = 10;
constexpr size_t ZEROTH_DIGIT_OFFSET = 24;
constexpr size_t FIRST_DIGIT_OFFSET = 16;
constexpr size_t SECOND_DIGIT_OFFSET = 8;

constexpr int32_t OFFSET = 4;
const std::u16string DECLARE_INTERFACE_DESCRIPTOR = u"HDI.IServiceStatusListener.V1_0";

uint32_t Convert2Uint32(const uint8_t *ptr)
{
    if (ptr == nullptr) {
        return HDF_SUCCESS;
    }
    /*
     * Move the 0th digit 24 to the left, the first digit 16 to the left, the second digit 8 to the left,
     * and the third digit no left
     */
    return (ptr[0] << ZEROTH_DIGIT_OFFSET) | (ptr[1] << FIRST_DIGIT_OFFSET) | (ptr[2] << SECOND_DIGIT_OFFSET) | ptr[3];
}

bool HdiFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr) {
        return false;
    }
    uint32_t code = Convert2Uint32(rawData);
    rawData = rawData + OFFSET;
    size = size - OFFSET;

    MessageParcel data;
    data.WriteInterfaceToken(DECLARE_INTERFACE_DESCRIPTOR);
    data.WriteBuffer(rawData, size);
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    sptr<ServStatListenerStub> servStatListener = new ServStatListenerFuzzer();
    if (servStatListener == nullptr) {
        HDF_LOGE("%{public}s:new ServStatListenerFuzzer failed.", __func__);
        return false;
    }

    servStatListener->OnRemoteRequest(code, data, reply, option);

    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return HDF_SUCCESS;
    }

    /* Run your code on data */
    OHOS::HdiFuzzTest(data, size);
    return HDF_SUCCESS;
}

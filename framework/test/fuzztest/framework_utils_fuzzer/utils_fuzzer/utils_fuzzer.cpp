/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include "utils_fuzzer.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "parcel.h"
#include <string>
#include "osal_mem.h"
#include "hdi_support.h"
#include <string_ex.h>
#include "hdf_map.h"
#include "hdf_sbuf.h"
#include "hdf_sbuf_impl.h"

namespace OHOS {
    const int NUM = 23;
    const static char *g_hdfSecStrArray[NUM] = {
    "i2c",
    "spi",
    "gpio",
    "pinctl",
    "clock",
    "regulator",
    "mipi",
    "uart",
    "sdio",
    "mdio",
    "apb",
    "pcie",
    "pcm",
    "i2s",
    "pwm",
    "dma",
    "efuse",
    "flash",
    "emmc",
    "rtc",
    "adc",
    "wdt",
    "i3c",
    };
static void HdfUtilsFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        HDF_LOGE("%{public}s: data is nullptr!", __func__);
    }
    Parcel parcel;
    parcel.WriteBuffer(data, size);
    const std::u16string INTERFACE_NAME = OHOS::Str8ToStr16(parcel.ReadString());
    Map testMap;
    Map testMap1;
    MapInit(&testMap);
    MapDelete(&testMap);
    for (int i = 0; i < NUM; ++i) {
        MapSet(&testMap1, g_hdfSecStrArray[i], &i, sizeof(int *));
    }
    const char *name = "i2c";
    MapGet(&testMap1, "");
    MapGet(&testMap1, name);
    MapErase(&testMap1, "");
    MapErase(&testMap1, name);
    MapDelete(&testMap1);

    const uint64_t value = 1;
    const uint32_t value1 = 2;
    const uint16_t value2 = 3;
    const uint8_t value3 = 4;
    uint64_t value4 = 1;
    uint32_t value5 = 1;
    uint16_t value6 = 1;
    uint8_t value7 = 1;
    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    struct HdfSBuf *copy = HdfSbufCopy(sbuf);
    struct HdfSBufImpl *impl = HdfSbufGetImpl(sbuf);
    struct HdfSBufImpl *impl1 = HdfSbufGetImpl(copy);
    HdfSbufTypedObtainInplace(value1, impl1);
    HdfSbufSetDataSize(copy, size);
    HdfSbufGetDataSize(copy);
    HdfSbufReadUnpadBuffer(copy, size);
    HdfSbufWriteUint64(copy, value);
    HdfSbufWriteUint32(copy, value1);
    HdfSbufWriteUint16(copy, value2);
    HdfSbufWriteUint8(copy, value3);
    HdfSbufReadUint64(copy, &value4);
    HdfSbufReadUint32(copy, &value5);
    HdfSbufReadUint16(copy, &value6);
    HdfSbufReadUint8(copy, &value7);

    HdfSbufGetData(sbuf);
    HdfSbufFlush(sbuf);
    HdfSbufGetCapacity(sbuf);
    impl->writeBuffer(impl, data, value1);
    impl->writeUnpadBuffer(impl, data, value1);
    impl->writeUint64(impl, value);
    impl->writeUint32(impl, value1);
    impl->writeUint16(impl, value2);
    impl->writeUint8(impl, value3);
    impl->readUnpadBuffer(impl, size);
    impl->readUint64(impl, &value4);
    impl->readUint32(impl, &value5);
    impl->readUint16(impl, &value6);
    impl->readUint8(impl, &value7);
    struct HdfSBufImpl *implCopy = impl->copy(impl);
    impl->setDataSize(implCopy, size);
    impl->getDataSize(implCopy);
    impl->getCapacity(implCopy);
    impl->transDataOwnership(implCopy);
    impl->move(implCopy);
    impl->recycle(implCopy);
    impl->recycle(impl);
    HdfSbufMove(copy);
    HdfSbufTransDataOwnership(copy);
    HdfSbufRecycle(sbuf);
    HdfSbufRecycle(copy);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::HdfUtilsFuzzTest(data, size);
    return HDF_SUCCESS;
}
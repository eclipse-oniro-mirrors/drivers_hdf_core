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
#include "hdf_cstring.h"
#include "hdf_thread_ex.h"
#include "osal_message.h"

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

    struct HdfMessage *message = HdfMessageObtain(size);
    HdfMessageDelete(reinterpret_cast<HdfSListNode *>(message));

    const char *str = "123";
    struct HdfCString *cstr = HdfCStringObtain(str);
    HdfCStringRecycle(cstr);
    struct HdfThread *thread = HdfThreadNewInstance();
    HdfThreadFreeInstance(thread);

    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    struct HdfSBuf *copy = HdfSbufCopy(sbuf);
    HdfSbufSetDataSize(copy, size);
    HdfSbufGetDataSize(copy);
    HdfSbufGetData(copy);
    struct HdfSBufImpl *impl = HdfSbufGetImpl(copy);
    if (impl == nullptr) {
        impl = (struct HdfSBufImpl *)OsalMemCalloc(sizeof(struct HdfSBufImpl));
    }
    HdfSbufMove(copy);
    HdfSbufTransDataOwnership(copy);
    HdfSbufTypedObtainInplace(static_cast<uint32_t>(size), impl);
    HdfSbufFlush(copy);
    HdfSbufGetCapacity(copy);
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
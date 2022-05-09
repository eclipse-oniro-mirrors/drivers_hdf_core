/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_platform_core_entry_test.h"
#include "hdf_log.h"
#include "platform_event_test.h"
#if defined(LOSCFG_DRIVERS_HDF_PLATFORM_TRACE) || defined(CONFIG_DRIVERS_HDF_PLATFORM_TRACE)
#include "platform_trace_test.h"
#endif
#include "platform_queue_test.h"

#define HDF_LOG_TAG hdf_platform_core_entry_test

int32_t HdfPlatformEventTestEntry(HdfTestMsg *msg)
{
    if (msg != NULL) {
        msg->result = PlatformEventTestExecute(msg->subCmd);
    }
    return HDF_SUCCESS;
}

int32_t HdfPlatformQueueTestEntry(HdfTestMsg *msg)
{
    if (msg != NULL) {
        msg->result = PlatformQueueTestExecute(msg->subCmd);
    }
    return HDF_SUCCESS;
}

#if defined(LOSCFG_DRIVERS_HDF_PLATFORM_TRACE) || defined(CONFIG_DRIVERS_HDF_PLATFORM_TRACE)
int32_t HdfPlatformTraceTestEntry(HdfTestMsg *msg)
{
    HDF_LOGD("HdfPlatformTraceTestEntry enter:");
    if (msg != NULL) {
        msg->result = PlatformTraceTestExecute(msg->subCmd);
    }
    return HDF_SUCCESS;
}
#endif

int32_t HdfPlatformDeviceTestEntry(HdfTestMsg *msg)
{
    msg->result = HDF_SUCCESS;
    return HDF_SUCCESS;
}

int32_t HdfPlatformManagerTestEntry(HdfTestMsg *msg)
{
    msg->result = HDF_SUCCESS;
    return HDF_SUCCESS;
}


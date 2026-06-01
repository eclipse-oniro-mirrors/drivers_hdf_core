/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "osal_time.h"
#include "hdf_core_log.h"
#include "osal_math.h"

#include <libhmsrv_sys/hm_timer.h>
#include <udk/delay.h>
#include <udk/timer.h>

#define HDF_LOG_TAG osal_time

void OsalMSleep(uint32_t ms)
{
    struct timespec ts = {
        .tv_sec = ms / MSEC_PER_SEC,
        .tv_nsec = (ms % MSEC_PER_SEC) * NSEC_PER_MSEC,
    };

    udk_nanosleep(&ts, NULL);
}

void OsalSleep(uint32_t sec)
{
    OsalMSleep(sec * HDF_KILO_UNIT);
}

void OsalUSleep(uint32_t us)
{
    struct timespec ts = {
        .tv_sec = (long)(unsigned long)us / USEC_PER_SEC,
        .tv_nsec = ((long)(unsigned long)us % USEC_PER_SEC) * NSEC_PER_USEC,
    };

    udk_nanosleep(&ts, NULL);
}

int32_t OsalGetTime(OsalTimespec *time)
{
    struct timespec ts;
    int ret;

    if (time == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    memset_s(&ts, sizeof(ts), 0, sizeof(ts));
    ret = udk_clock_gettime(CLOCK_REALTIME, &ts);
    if (ret < 0) {
        HDF_LOGE("get ktime_t failed, ret=%d", ret);
        return HDF_FAILURE;
    }

    time->sec = ts.tv_sec;
    time->usec = ts.tv_nsec / HDF_KILO_UNIT;

    return HDF_SUCCESS;
}

int32_t OsalDiffTime(const OsalTimespec *start, const OsalTimespec *end, OsalTimespec *diff)
{
    uint32_t usec = 0;
    uint32_t sec = 0;
    if (start == NULL || end == NULL || diff == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (start->sec > end->sec) {
        HDF_LOGE("%s start time later than end time", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (end->usec < start->usec) {
        usec = (HDF_KILO_UNIT * HDF_KILO_UNIT);
        sec = 1;
    }
    diff->usec = usec + end->usec - start->usec;
    diff->sec = end->sec - start->sec - sec;

    return HDF_SUCCESS;
}

uint64_t OsalGetSysTimeMs(void)
{
    OsalTimespec time;
    memset_s(&time, sizeof(time), 0, sizeof(time));
    OsalGetTime(&time);
    return (time.sec * HDF_KILO_UNIT + OsalDivS64(time.usec, HDF_KILO_UNIT));
}

void OsalMDelay(uint32_t ms)
{
    udk_mdelay(ms);
}

void OsalUDelay(uint32_t us)
{
    udk_udelay(us);
}
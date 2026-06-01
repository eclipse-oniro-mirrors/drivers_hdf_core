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

#include "osal_timer.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include "osal_mutex.h"

#include <udk/timer.h>

#define HDF_LOG_TAG osal_timer
#define HDF_OSAL_TIMER_PRECISION 10

typedef enum {
    OSAL_TIMER_ONCE,
    OSAL_TIMER_LOOP
} OsalTimerMode;

int32_t OsalTimerCreate(OsalTimer *timer, uint32_t interval, OsalTimerFunc func, uintptr_t arg)
{
    int ret;
    struct udk_timer *udkTimer = NULL;

    if (func == NULL || timer == NULL || interval == 0U) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkTimer = (struct udk_timer *)OsalMemCalloc(sizeof(*udkTimer));
    if (udkTimer == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        timer->realTimer = NULL;
        return HDF_ERR_MALLOC_FAIL;
    }

    udk_timer_init(udkTimer, func, (unsigned long)arg, interval);
    ret = udk_timer_setprecision(udkTimer, HDF_OSAL_TIMER_PRECISION);
    if (ret < 0) {
        HDF_LOGE("%s set timer precision fail, ret=%d", __func__, ret);
        OsalMemFree(udkTimer);
        return HDF_FAILURE;
    }
    timer->realTimer = udkTimer;

    return HDF_SUCCESS;
}

static int32_t OsalTimerStart(OsalTimer *timer, OsalTimerMode mode)
{
    int ret;
    struct udk_timer *udkTimer = NULL;

    if (timer == NULL || timer->realTimer == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkTimer = (struct udk_timer *)timer->realTimer;

    switch (mode) {
        case OSAL_TIMER_LOOP:
            udk_timer_setflags(udkTimer, UDK_TIMER_FLAGS_PERIODIC);
            break;
        case OSAL_TIMER_ONCE:
            /* fall through */
        default:
            /* do nothing */
            HDF_LOGD("%s default timer mode", __func__);
    }

    ret = udk_timer_add(udkTimer);
    if (ret < 0) {
        HDF_LOGE("udk timer mod fail, ret=%d", ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t OsalTimerStartOnce(OsalTimer *timer)
{
    return OsalTimerStart(timer, OSAL_TIMER_ONCE);
}

int32_t OsalTimerStartLoop(OsalTimer *timer)
{
    return OsalTimerStart(timer, OSAL_TIMER_LOOP);
}

int32_t OsalTimerSetTimeout(OsalTimer *timer, uint32_t interval)
{
    int ret;
    struct udk_timer *udkTimer = NULL;

    if (timer == NULL || timer->realTimer == NULL || interval == 0) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkTimer = timer->realTimer;
    ret = udk_timer_mod(udkTimer, interval);
    if (ret < 0) {
        HDF_LOGE("udk timer mod fail, ret=%d", ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t OsalTimerDelete(OsalTimer *timer)
{
    struct udk_timer *udkTimer = NULL;

    if (timer == NULL || timer->realTimer == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkTimer = timer->realTimer;
    udk_timer_del(udkTimer);
    timer->realTimer = NULL;

    return HDF_SUCCESS;
}
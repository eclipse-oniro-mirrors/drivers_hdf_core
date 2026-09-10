/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "osal_timer.h"
#include "cmsis_os2.h"
#include "hdf_core_log.h"
#include "osal_cmsis_tick.h"
#include "osal_mem.h"

#define HDF_LOG_TAG osal_timer

struct OsalCmsisTimer {
    uintptr_t arg;
    OsalTimerFunc func;
    uint32_t interval;
    osTimerId_t timerId;
    osTimerType_t type;
};

/* CMSIS timer callback adapter. */
static void OsalTimerCallback(void *argument)
{
    struct OsalCmsisTimer *timer = (struct OsalCmsisTimer *)argument;
    if (timer != NULL && timer->func != NULL) {
        timer->func(timer->arg);
    }
}

int32_t OsalTimerCreate(OsalTimer *timer, uint32_t interval, OsalTimerFunc func, uintptr_t arg)
{
    struct OsalCmsisTimer *cmsisTimer = NULL;
    if (func == NULL || timer == NULL || interval == 0) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    timer->realTimer = NULL;
    cmsisTimer = (struct OsalCmsisTimer *)OsalMemCalloc(sizeof(*cmsisTimer));
    if (cmsisTimer == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    cmsisTimer->arg = arg;
    cmsisTimer->func = func;
    cmsisTimer->interval = interval;
    cmsisTimer->timerId = NULL;
    /* Default type; set by StartOnce/StartLoop. */
    cmsisTimer->type = osTimerPeriodic;
    timer->realTimer = (void *)cmsisTimer;
    return HDF_SUCCESS;
}

static int32_t OsalStartTimer(OsalTimer *timer, osTimerType_t type)
{
    struct OsalCmsisTimer *cmsisTimer = NULL;
    uint32_t ticks;

    if (timer == NULL || timer->realTimer == NULL) {
        HDF_LOGE("%s invalid para %d", __func__, __LINE__);
        return HDF_ERR_INVALID_PARAM;
    }
    cmsisTimer = (struct OsalCmsisTimer *)timer->realTimer;
    if (cmsisTimer->interval == 0 || cmsisTimer->func == NULL) {
        HDF_LOGE("%s invalid para %d", __func__, __LINE__);
        return HDF_ERR_INVALID_PARAM;
    }
    /* Stop/delete existing timer if re-starting. */
    if (cmsisTimer->timerId != NULL) {
        (void)osTimerStop(cmsisTimer->timerId);
        (void)osTimerDelete(cmsisTimer->timerId);
        cmsisTimer->timerId = NULL;
    }
    cmsisTimer->type = type;
    cmsisTimer->timerId = osTimerNew(OsalTimerCallback, type, cmsisTimer, NULL);
    if (cmsisTimer->timerId == NULL) {
        HDF_LOGE("%s osTimerNew fail", __func__);
        return HDF_FAILURE;
    }
    ticks = OsalCmsisMsToTick(cmsisTimer->interval);
    if (osTimerStart(cmsisTimer->timerId, ticks) != osOK) {
        HDF_LOGE("%s osTimerStart fail", __func__);
        (void)osTimerDelete(cmsisTimer->timerId);
        cmsisTimer->timerId = NULL;
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalTimerStartLoop(OsalTimer *timer)
{
    return OsalStartTimer(timer, osTimerPeriodic);
}

int32_t OsalTimerStartOnce(OsalTimer *timer)
{
    return OsalStartTimer(timer, osTimerOnce);
}

int32_t OsalTimerSetTimeout(OsalTimer *timer, uint32_t interval)
{
    struct OsalCmsisTimer *cmsisTimer = NULL;
    if (timer == NULL || timer->realTimer == NULL || interval == 0) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    cmsisTimer = (struct OsalCmsisTimer *)timer->realTimer;
    if (cmsisTimer->interval == interval) {
        return HDF_SUCCESS;
    }
    cmsisTimer->interval = interval;
    /* Re-create with new interval using the timer's existing type. */
    return OsalStartTimer(timer, cmsisTimer->type);
}

int32_t OsalTimerDelete(OsalTimer *timer)
{
    struct OsalCmsisTimer *cmsisTimer = NULL;
    if (timer == NULL || timer->realTimer == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    cmsisTimer = (struct OsalCmsisTimer *)timer->realTimer;
    if (cmsisTimer->timerId != NULL) {
        osStatus_t ret = osTimerDelete(cmsisTimer->timerId);
        if (ret != osOK) {
            HDF_LOGE("%s osTimerDelete fail %d", __func__, ret);
            return HDF_FAILURE;
        }
        cmsisTimer->timerId = NULL;
    }
    OsalMemFree(timer->realTimer);
    timer->realTimer = NULL;
    return HDF_SUCCESS;
}

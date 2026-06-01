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

#include "osal_sem.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include <libhmsrv_sys/hm_timer.h>
#include <udk/sem.h>
#include <udk/time.h>

#define HDF_LOG_TAG osal_sem

int32_t OsalSemInit(struct OsalSem *sem, uint32_t value)
{
    struct udk_sem *udkSem = NULL;

    if (sem == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkSem = (struct udk_sem *)OsalMemCalloc(sizeof(*udkSem));
    if (udkSem == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (udk_sem_init(udkSem, (int)value) != 0) {
        HDF_LOGE("%s udk_sem_init fail", __func__);
        OsalMemFree(udkSem);
        return HDF_FAILURE;
    }

    sem->realSemaphore = (void *)udkSem;
    return HDF_SUCCESS;
}

int32_t OsalSemWait(struct OsalSem *sem, uint32_t ms)
{
    int ret;
    struct udk_sem *udkSem = NULL;
    struct timespec ts;

    if (sem == NULL || sem->realSemaphore == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkSem = (struct udk_sem *)sem->realSemaphore;
    if (ms == OSAL_WAIT_FOREVER) {
        ret = udk_sem_wait(udkSem);
        if (ret != 0) {
            HDF_LOGE("%s udk_sem_wait fail, ret=%d", __func__, ret);
            return HDF_FAILURE;
        }
    } else {
        ret = udk_clock_gettime(CLOCK_MONOTONIC, &ts);
        if (ret != 0) {
            HDF_LOGE("%s udk_clock_gettime fail, ret=%d", __func__, ret);
            return HDF_FAILURE;
        }

        ret = udk_time_add(&ts, ((unsigned long)(ms)) * USEC_PER_MSEC);
        if (ret != 0) {
            HDF_LOGE("%s udk_time_add fail %d", __func__, ret);
            return HDF_FAILURE;
        }

        ret = udk_sem_timedwait(udkSem, &ts);
        if (ret != 0) {
            if (ret == -ETIMEDOUT) {
                return HDF_ERR_TIMEOUT;
            } else {
                HDF_LOGE("%s time_out %u %d", __func__, ms, ret);
                return HDF_FAILURE;
            }
        }
    }

    return HDF_SUCCESS;
}

int32_t OsalSemPost(struct OsalSem *sem)
{
    if (sem == NULL || sem->realSemaphore == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (udk_sem_post((struct udk_sem *)sem->realSemaphore) != 0) {
        HDF_LOGE("%s udk_sem_post fail", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t OsalSemDestroy(struct OsalSem *sem)
{
    if (sem == NULL || sem->realSemaphore == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    /* UDK sem does not require kill, just free memory */
    OsalMemFree(sem->realSemaphore);
    sem->realSemaphore = NULL;

    return HDF_SUCCESS;
}
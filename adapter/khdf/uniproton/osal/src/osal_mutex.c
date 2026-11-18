/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of the conditions and the following disclaimer in the documentation and/or other materials
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
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND IN ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "osal_mutex.h"
#include "hdf_core_log.h"
#include "prt_config.h"
#include "prt_sem.h"

#define HDF_LOG_TAG osal_mutex

#define HDF_INVALID_MUX_ID UINT32_MAX

int32_t OsalMutexInit(struct OsalMutex *mutex)
{
    SemHandle semId;
    uint32_t ret;

    if (mutex == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemCreate(1, &semId);
    if (ret != OS_OK) {
        HDF_LOGE("%s PRT_SemCreate fail %u", __func__, ret);
        mutex->realMutex = (void *)(uintptr_t)HDF_INVALID_MUX_ID;
        return HDF_FAILURE;
    }
    mutex->realMutex = (void *)(uintptr_t)semId;
    return HDF_SUCCESS;
}

int32_t OsalMutexDestroy(struct OsalMutex *mutex)
{
    uint32_t ret;

    if (mutex == NULL || mutex->realMutex == (void *)(uintptr_t)HDF_INVALID_MUX_ID) {
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemDelete((SemHandle)(uintptr_t)mutex->realMutex);
    if (ret != OS_OK) {
        HDF_LOGE("%s PRT_SemDelete fail %u", __func__, ret);
        return HDF_FAILURE;
    }
    mutex->realMutex = (void *)(uintptr_t)HDF_INVALID_MUX_ID;
    return HDF_SUCCESS;
}

int32_t OsalMutexLock(struct OsalMutex *mutex)
{
    uint32_t ret;

    if (mutex == NULL || mutex->realMutex == (void *)(uintptr_t)HDF_INVALID_MUX_ID) {
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemPend((SemHandle)(uintptr_t)mutex->realMutex, OS_TICK_FOREVER);
    if (ret != OS_OK) {
        HDF_LOGE("%s PRT_SemPend fail %u", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalMutexTimedLock(struct OsalMutex *mutex, uint32_t ms)
{
    uint32_t ret;
    uint32_t ticks = (uint32_t)(((uint64_t)ms * OS_TICK_PER_SECOND) / 1000);

    if (mutex == NULL || mutex->realMutex == (void *)(uintptr_t)HDF_INVALID_MUX_ID) {
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemPend((SemHandle)(uintptr_t)mutex->realMutex, ticks);
    if (ret == OS_OK) {
        return HDF_SUCCESS;
    } else {
        if (ret == OS_ERRNO_SEM_TIMEOUT) {
            return HDF_ERR_TIMEOUT;
        }
        HDF_LOGE("%s PRT_SemPend fail %u", __func__, ret);
        return HDF_FAILURE;
    }
}

int32_t OsalMutexUnlock(struct OsalMutex *mutex)
{
    uint32_t ret;

    if (mutex == NULL || mutex->realMutex == (void *)(uintptr_t)HDF_INVALID_MUX_ID) {
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemPost((SemHandle)(uintptr_t)mutex->realMutex);
    if (ret != OS_OK) {
        HDF_LOGE("%s PRT_SemPost fail %u", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

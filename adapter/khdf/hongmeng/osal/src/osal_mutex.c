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

#include "osal_mutex.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include <udk/mutex.h>

#define HDF_LOG_TAG osal_mutex

int32_t OsalMutexInit(struct OsalMutex *mutex)
{
    struct udk_mutex *mutexImpl = NULL;

    if (mutex == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    mutexImpl = (struct udk_mutex *)OsalMemCalloc(sizeof(*mutexImpl));
    if (mutexImpl == NULL) {
        HDF_LOGE("%s OsalMemCalloc failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    udk_mutex_init(mutexImpl);
    mutex->realMutex = (void *)mutexImpl;
    return HDF_SUCCESS;
}

int32_t OsalMutexDestroy(struct OsalMutex *mutex)
{
    if (mutex == NULL || mutex->realMutex == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    /* UDK mutex does not require destroy, just free memory */
    OsalMemFree(mutex->realMutex);
    mutex->realMutex = NULL;
    return HDF_SUCCESS;
}

int32_t OsalMutexLock(struct OsalMutex *mutex)
{
    struct udk_mutex *mutexImpl = NULL;

    if (mutex == NULL || mutex->realMutex == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    mutexImpl = (struct udk_mutex *)mutex->realMutex;
    udk_mutex_lock(mutexImpl);
    return HDF_SUCCESS;
}

int32_t OsalMutexTimedLock(struct OsalMutex *mutex, uint32_t ms)
{
    HDF_LOGW("hongmeng does not support mutex timed lock");
    return HDF_ERR_NOT_SUPPORT;
}

int32_t OsalMutexUnlock(struct OsalMutex *mutex)
{
    struct udk_mutex *mutexImpl = NULL;

    if (mutex == NULL || mutex->realMutex == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    mutexImpl = (struct udk_mutex *)mutex->realMutex;
    udk_mutex_unlock(mutexImpl);
    return HDF_SUCCESS;
}
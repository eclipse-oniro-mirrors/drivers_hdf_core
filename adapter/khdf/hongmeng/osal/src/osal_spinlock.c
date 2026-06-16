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

#include "osal_spinlock.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include <udk/spinlock.h>

#define HDF_LOG_TAG osal_spinlock

int32_t OsalSpinInit(OsalSpinlock *spinlock)
{
    struct udk_spinlock *spinImpl = NULL;

    if (spinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    spinImpl = (struct udk_spinlock *)OsalMemCalloc(sizeof(*spinImpl));
    if (spinImpl == NULL) {
        HDF_LOGE("%s OsalMemCalloc failed", __func__);
        spinlock->realSpinlock = NULL;
        return HDF_ERR_MALLOC_FAIL;
    }

    udk_spinlock_init(spinImpl);
    spinlock->realSpinlock = (void *)spinImpl;
    return HDF_SUCCESS;
}

int32_t OsalSpinDestroy(OsalSpinlock *spinlock)
{
    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    OsalMemFree(spinlock->realSpinlock);
    spinlock->realSpinlock = NULL;
    return HDF_SUCCESS;
}

int32_t OsalSpinLock(OsalSpinlock *spinlock)
{
    struct udk_spinlock *spinImpl = NULL;
    int ret;

    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    spinImpl = (struct udk_spinlock *)spinlock->realSpinlock;
    ret = udk_spinlock_lock(spinImpl);
    if (ret != 0) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t OsalSpinUnlock(OsalSpinlock *spinlock)
{
    struct udk_spinlock *spinImpl = NULL;

    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    spinImpl = (struct udk_spinlock *)spinlock->realSpinlock;
    udk_spinlock_unlock(spinImpl);
    return HDF_SUCCESS;
}

int32_t OsalSpinLockIrq(OsalSpinlock *spinlock)
{
    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return OsalSpinLock(spinlock);
}

int32_t OsalSpinUnlockIrq(OsalSpinlock *spinlock)
{
    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return OsalSpinUnlock(spinlock);
}

int32_t OsalSpinLockIrqSave(OsalSpinlock *spinlock, uint32_t *flags)
{
    if (spinlock == NULL || spinlock->realSpinlock == NULL || flags == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return OsalSpinLock(spinlock);
}

int32_t OsalSpinUnlockIrqRestore(OsalSpinlock *spinlock, uint32_t *flags)
{
    if (spinlock == NULL || spinlock->realSpinlock == NULL || flags == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return OsalSpinUnlock(spinlock);
}
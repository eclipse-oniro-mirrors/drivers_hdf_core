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

#include "osal_spinlock.h"
#include "hdf_core_log.h"
#include "prt_hwi.h"

#define HDF_LOG_TAG osal_spinlock

int32_t OsalSpinInit(OsalSpinlock *spinlock)
{
    if (spinlock == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    spinlock->realSpinlock = NULL;
    return HDF_SUCCESS;
}

int32_t OsalSpinDestroy(OsalSpinlock *spinlock)
{
    if (spinlock == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    spinlock->realSpinlock = NULL;
    return HDF_SUCCESS;
}

int32_t OsalSpinLock(OsalSpinlock *spinlock)
{
    uint32_t intSave;

    if (spinlock == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    intSave = PRT_IntLock();
    spinlock->realSpinlock = (void *)(uintptr_t)intSave;
    return HDF_SUCCESS;
}

int32_t OsalSpinUnlock(OsalSpinlock *spinlock)
{
    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    PRT_IntRestore((uint32_t)(uintptr_t)spinlock->realSpinlock);
    spinlock->realSpinlock = NULL;
    return HDF_SUCCESS;
}

int32_t OsalSpinLockIrq(OsalSpinlock *spinlock)
{
    return OsalSpinLock(spinlock);
}

int32_t OsalSpinUnlockIrq(OsalSpinlock *spinlock)
{
    return OsalSpinUnlock(spinlock);
}

int32_t OsalSpinLockIrqSave(OsalSpinlock *spinlock, uint32_t *flags)
{
    uint32_t intSave;

    if (spinlock == NULL || flags == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    intSave = PRT_IntLock();
    *flags = intSave;
    spinlock->realSpinlock = (void *)(uintptr_t)intSave;
    return HDF_SUCCESS;
}

int32_t OsalSpinUnlockIrqRestore(OsalSpinlock *spinlock, uint32_t *flags)
{
    if (spinlock == NULL || flags == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    PRT_IntRestore(*flags);
    spinlock->realSpinlock = NULL;
    return HDF_SUCCESS;
}

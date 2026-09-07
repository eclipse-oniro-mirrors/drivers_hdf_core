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
 *    of the following conditions and the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
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

#include "osal_spinlock.h"
#include "cmsis_os2.h"
#include "hdf_core_log.h"
#include "osal_extend.h"

#define HDF_LOG_TAG osal_spinlock

int32_t OsalSpinInit(OsalSpinlock *spinlock)
{
    if (spinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    spinlock->realSpinlock = osMutexNew(NULL);
    if (spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s osMutexNew fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalSpinDestroy(OsalSpinlock *spinlock)
{
    osStatus_t ret;
    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = osMutexDelete(spinlock->realSpinlock);
    if (ret != osOK) {
        HDF_LOGE("%s osMutexDelete fail %d", __func__, ret);
        return HDF_FAILURE;
    }
    spinlock->realSpinlock = NULL;
    return HDF_SUCCESS;
}

int32_t OsalSpinLock(OsalSpinlock *spinlock)
{
    osStatus_t ret;
    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = osMutexAcquire(spinlock->realSpinlock, osWaitForever);
    if (ret != osOK) {
        HDF_LOGE("%s fail %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalSpinUnlock(OsalSpinlock *spinlock)
{
    osStatus_t ret;
    if (spinlock == NULL || spinlock->realSpinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = osMutexRelease(spinlock->realSpinlock);
    if (ret != osOK) {
        HDF_LOGE("%s fail %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalSpinLockIrq(OsalSpinlock *spinlock)
{
    return OsalSpinLockIrqSave(spinlock, NULL);
}

int32_t OsalSpinUnlockIrq(OsalSpinlock *spinlock)
{
    return OsalSpinUnlockIrqRestore(spinlock, NULL);
}

int32_t OsalSpinLockIrqSave(OsalSpinlock *spinlock, uint32_t *flags)
{
    uint32_t irqFlags;
    if (spinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    /* D2: disable IRQ FIRST, then acquire lock. flags is honored. */
    irqFlags = OsalExtendIrqSave();
    if (flags != NULL) {
        *flags = irqFlags;
    }
    if (OsalSpinLock(spinlock) != HDF_SUCCESS) {
        OsalExtendIrqRestore(irqFlags);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalSpinUnlockIrqRestore(OsalSpinlock *spinlock, uint32_t *flags)
{
    uint32_t irqFlags = 0;
    if (spinlock == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    /* D2: release lock FIRST, then restore IRQ. */
    if (OsalSpinUnlock(spinlock) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    if (flags != NULL) {
        irqFlags = *flags;
    }
    OsalExtendIrqRestore(irqFlags);
    return HDF_SUCCESS;
}

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

#include "osal_sem.h"
#include "cmsis_os2.h"
#include "hdf_core_log.h"
#include "osal_cmsis_tick.h"

#define HDF_LOG_TAG osal_sem

int32_t OsalSemInit(struct OsalSem *sem, uint32_t value)
{
    if (sem == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    sem->realSemaphore = osSemaphoreNew(value, value, NULL);
    if (sem->realSemaphore == NULL) {
        HDF_LOGE("%s osSemaphoreNew fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalSemWait(struct OsalSem *sem, uint32_t ms)
{
    if (sem == NULL || sem->realSemaphore == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    uint32_t timeout = (ms == OSAL_WAIT_FOREVER) ? osWaitForever : OsalCmsisMsToTick(ms);
    osStatus_t ret = osSemaphoreAcquire(sem->realSemaphore, timeout);
    if (ret == osErrorTimeout) {
        return HDF_ERR_TIMEOUT;
    }
    if (ret != osOK) {
        HDF_LOGE("%s osSemaphoreAcquire fail %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalSemPost(struct OsalSem *sem)
{
    if (sem == NULL || sem->realSemaphore == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    osStatus_t ret = osSemaphoreRelease(sem->realSemaphore);
    if (ret != osOK) {
        HDF_LOGE("%s osSemaphoreRelease fail %d", __func__, ret);
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
    osStatus_t ret = osSemaphoreDelete(sem->realSemaphore);
    if (ret != osOK) {
        HDF_LOGE("%s osSemaphoreDelete fail %d", __func__, ret);
        return HDF_FAILURE;
    }
    sem->realSemaphore = NULL;
    return HDF_SUCCESS;
}

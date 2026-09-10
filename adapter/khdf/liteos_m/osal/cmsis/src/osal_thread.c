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

#include "osal_thread.h"
#include "cmsis_os2.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG osal_thread

#define OSAL_THREAD_NAME "hdf_thread"
#define OSAL_INVALID_CPU_ID 0xFFFFFFFFU

struct ThreadWrapper {
    OsalThreadEntry threadEntry;
    void *entryPara;
    unsigned int cpuID;
    osThreadId_t threadId;
};

/* D3: HDF 4-level enum → CMSIS enum. No numeric conversion to native prio. */
osPriority_t OsalThreadCmsisPriority(OSAL_THREAD_PRIORITY priority)
{
    switch (priority) {
        case OSAL_THREAD_PRI_LOW:
            return osPriorityLow;
        case OSAL_THREAD_PRI_DEFAULT:
            return osPriorityNormal;
        case OSAL_THREAD_PRI_HIGH:
            return osPriorityHigh;
        case OSAL_THREAD_PRI_HIGHEST:
            return osPriorityRealtime;   /* NOT osPriorityISR */
        default:
            return osPriorityNormal;
    }
}

/* CMSIS thread func adapter: unwraps entry/para stored in ThreadWrapper. */
static void OsalThreadEntryAdapter(void *argument)
{
    struct ThreadWrapper *wrapper = (struct ThreadWrapper *)argument;
    if (wrapper != NULL && wrapper->threadEntry != NULL) {
        (void)wrapper->threadEntry(wrapper->entryPara);
    }
}

int32_t OsalThreadCreate(struct OsalThread *thread, OsalThreadEntry threadEntry, void *entryPara)
{
    struct ThreadWrapper *para = NULL;

    if (thread == NULL || threadEntry == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    thread->realThread = NULL;
    para = (struct ThreadWrapper *)OsalMemCalloc(sizeof(*para));
    if (para == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    para->entryPara = entryPara;
    para->threadEntry = threadEntry;
    para->threadId = NULL;
    para->cpuID = OSAL_INVALID_CPU_ID;
    thread->realThread = para;

    return HDF_SUCCESS;
}

int32_t OsalThreadBind(struct OsalThread *thread, unsigned int cpuID)
{
    struct ThreadWrapper *para = NULL;

    if (thread == NULL || thread->realThread == NULL) {
        HDF_LOGE("%s invalid parameter %d", __func__, __LINE__);
        return HDF_ERR_INVALID_PARAM;
    }
    para = (struct ThreadWrapper *)thread->realThread;
    para->cpuID = cpuID;
    return HDF_SUCCESS;
}

int32_t OsalThreadStart(struct OsalThread *thread, const struct OsalThreadParam *param)
{
    struct ThreadWrapper *para = NULL;
    osThreadAttr_t attr;

    if (thread == NULL || thread->realThread == NULL || param == NULL ||
        param->priority > OSAL_THREAD_PRI_HIGHEST) {
        HDF_LOGE("%s invalid parameter %d", __func__, __LINE__);
        return HDF_ERR_INVALID_PARAM;
    }

    para = (struct ThreadWrapper *)thread->realThread;
    (void)memset_s(&attr, sizeof(attr), 0, sizeof(attr));
    attr.name = (param->name != NULL) ? param->name : OSAL_THREAD_NAME;
    attr.priority = OsalThreadCmsisPriority(param->priority);
    if (param->stackSize != 0) {
        attr.stack_size = param->stackSize;
    }
    /* affinity_mask for cpu binding (SMP); OSAL_INVALID_CPU_ID = not bound. */
    if (para->cpuID != OSAL_INVALID_CPU_ID) {
        attr.affinity_mask = (1U << para->cpuID);
    }

    para->threadId = osThreadNew(OsalThreadEntryAdapter, para, &attr);
    if (para->threadId == NULL) {
        HDF_LOGE("%s osThreadNew fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static bool OsalCheckPara(struct OsalThread *thread)
{
    struct ThreadWrapper *para = NULL;

    if (thread == NULL || thread->realThread == NULL) {
        HDF_LOGE("%s invalid parameter %d", __func__, __LINE__);
        return false;
    }
    para = (struct ThreadWrapper *)thread->realThread;
    if (para->threadId == NULL) {
        HDF_LOGE("%s invalid parameter %d", __func__, __LINE__);
        return false;
    }
    return true;
}

int32_t OsalThreadSuspend(struct OsalThread *thread)
{
    struct ThreadWrapper *para = NULL;

    if (!OsalCheckPara(thread)) {
        return HDF_ERR_INVALID_PARAM;
    }
    para = (struct ThreadWrapper *)thread->realThread;
    osStatus_t ret = osThreadSuspend(para->threadId);
    if (ret != osOK) {
        HDF_LOGE("%s osThreadSuspend failed %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalThreadResume(struct OsalThread *thread)
{
    struct ThreadWrapper *para = NULL;

    if (!OsalCheckPara(thread)) {
        return HDF_ERR_INVALID_PARAM;
    }
    para = (struct ThreadWrapper *)thread->realThread;
    osStatus_t ret = osThreadResume(para->threadId);
    if (ret != osOK) {
        HDF_LOGE("%s osThreadResume failed %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t OsalThreadDestroy(struct OsalThread *thread)
{
    struct ThreadWrapper *para = NULL;

    if (!OsalCheckPara(thread)) {
        return HDF_ERR_INVALID_PARAM;
    }
    para = (struct ThreadWrapper *)thread->realThread;
    /* Per the public contract the caller must have already terminated the
     * thread before calling OsalThreadDestroy; this only frees the wrapper.
     * Matching the liteos_m reference, which does not call LOS_TaskDelete
     * here either — calling osThreadTerminate would risk a use-after-free
     * of the wrapper if termination is asynchronous on the target RTOS. */
    (void)para;
    OsalMemFree(thread->realThread);
    thread->realThread = NULL;
    return HDF_SUCCESS;
}

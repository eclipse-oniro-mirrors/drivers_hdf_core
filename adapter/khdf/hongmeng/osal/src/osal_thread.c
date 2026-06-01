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

#include "osal_thread.h"
#include "osal_mem.h"
#include <hdf_core_log.h>
#include <hmkit/types_utils.h>
#include <udk/atomic.h>
#include <udk/cpuset.h>
#include <udk/thread.h>
#include <udk/thread_cond.h>

#define HDF_LOG_TAG osal_thread
#define OSAL_INVALID_CPU_ID UINT_MAX
#define THREAD_NAME_LEN 16

struct ThreadWrapper {
    OsalThreadEntry threadEntry;
    void *entryPara;
    int threadRet;

    udk_thread_t thread;
    struct udk_mutex lock;
    struct udk_thread_cond cond;
    struct udk_atomic_int *state;
    uint32_t cpuId;
};

enum {
    THREAD_STATE_INITIALIZED = 0x1,
    THREAD_STATE_RUNNING = 0x2,
    THREAD_STATE_STOPPING = 0x3,
};

int32_t OsalThreadCreate(struct OsalThread *thread, OsalThreadEntry threadEntry, void *entryPara)
{
    int ret;
    struct ThreadWrapper *wrapper = NULL;

    if (thread == NULL || threadEntry == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    wrapper = (struct ThreadWrapper *)OsalMemCalloc(sizeof(*wrapper));
    if (wrapper == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    wrapper->entryPara = entryPara;
    wrapper->threadEntry = threadEntry;
    /* return HDF_FAILURE if OsalThreadStart is never called */
    wrapper->threadRet = -HDF_FAILURE;
    wrapper->cpuId = OSAL_INVALID_CPU_ID;

    udk_mutex_init(&wrapper->lock);
    ret = udk_thread_cond_init(&wrapper->cond, NULL);
    if (ret < 0) {
        OsalMemFree(wrapper);
        return HDF_FAILURE;
    }

    wrapper->state = udk_atomic_int_alloc(THREAD_STATE_INITIALIZED);
    if (wrapper->state == NULL) {
        OsalMemFree(wrapper);
        return HDF_FAILURE;
    }
    thread->realThread = wrapper;

    return HDF_SUCCESS;
}

int32_t OsalThreadBind(struct OsalThread *thread, unsigned int cpuId)
{
    struct ThreadWrapper *wrapper = NULL;

    if (thread == NULL || thread->realThread == NULL) {
        HDF_LOGE("%s invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    wrapper = (struct ThreadWrapper *)thread->realThread;
    wrapper->cpuId = cpuId;
    return HDF_SUCCESS;
}

static int OsalWakeUpProcess(struct ThreadWrapper *wrapper)
{
    int ret;

    if (wrapper == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    udk_mutex_lock(&wrapper->lock);
    udk_atomic_int_set(wrapper->state, THREAD_STATE_RUNNING);
    ret = udk_thread_cond_signal(&wrapper->cond);
    udk_mutex_unlock(&wrapper->lock);
    if (ret != 0) {
        HDF_LOGE("Failed to signal condition");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void *OsalThreadFn(void *args)
{
    int state = THREAD_STATE_INITIALIZED;
    struct ThreadWrapper *wrapper = (struct ThreadWrapper *)args;

    if (wrapper->threadEntry == NULL) {
        return NULL;
    }

    udk_mutex_lock(&wrapper->lock);
    while (state != THREAD_STATE_RUNNING) {
        state = udk_atomic_int_read(wrapper->state);
        if ((state == THREAD_STATE_RUNNING) ||
            (state == THREAD_STATE_STOPPING)) {
            break;
        }
        udk_thread_cond_wait(&wrapper->cond, &wrapper->lock);
    }
    udk_mutex_unlock(&wrapper->lock);
    wrapper->threadRet = wrapper->threadEntry(wrapper->entryPara);

    return NULL;
}

static void OsalThreadBindCpu(struct ThreadWrapper *wrapper)
{
    int ret;
    bool emptyset;
    udk_cpuset_t cpuset = udk_cpuset_empty();

    if (wrapper == NULL) {
        HDF_LOGW("thread wrapper is NULL");
        return;
    }

    cpuset = udk_cpuset_add(cpuset, wrapper->cpuId);
    emptyset = udk_cpuset_equal(udk_cpuset_empty(), cpuset);
    if (emptyset) {
        HDF_LOGW("bind to invalid cpu: %u, ignored", wrapper->cpuId);
        return;
    }

    ret = udk_thread_setaffinity(wrapper->thread, cpuset);
    if (ret < 0) {
        HDF_LOGW("bind failed, ret=%d", ret);
    }
}

int32_t OsalThreadStart(struct OsalThread *thread, const struct OsalThreadParam *param)
{
    int ret;
    char name[THREAD_NAME_LEN];
    struct ThreadWrapper *wrapper = NULL;

    if (thread == NULL || thread->realThread == NULL || param == NULL || param->name == NULL) {
        HDF_LOGE("%s invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    wrapper = thread->realThread;
    ret = udk_thread_create(&wrapper->thread, NULL, OsalThreadFn, (void *)wrapper);
    if (ret != 0) {
        HDF_LOGE("%s failed to start thread, ret=%d", __func__, ret);
        return HDF_FAILURE;
    }

    memset_s(name, sizeof(name), 0, sizeof(name));
    ret = memcpy_s(name, THREAD_NAME_LEN, param->name, strlen(param->name));
    if (ret != 0) {
        HDF_LOGE("%s failed to copy thread name, ret=%d", __func__, ret);
        return HDF_FAILURE;
    }

    ret = udk_thread_setname(wrapper->thread, name, NULL);
    if (ret < 0) {
        HDF_LOGE("set thread name failed, ret=%d", ret);
        return HDF_FAILURE;
    }

    OsalThreadBindCpu(wrapper);
    ret = OsalWakeUpProcess(wrapper);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t OsalThreadSuspend(struct OsalThread *thread)
{
    UNUSED(thread);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t OsalThreadDestroy(struct OsalThread *thread)
{
    struct ThreadWrapper *wrapper = NULL;
    if (thread == NULL || thread->realThread == NULL) {
        HDF_LOGE("%s invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    wrapper = (struct ThreadWrapper *)thread->realThread;
    udk_atomic_int_destroy(wrapper->state);
    OsalMemFree(wrapper);
    thread->realThread = NULL;

    return HDF_SUCCESS;
}

int32_t OsalThreadResume(struct OsalThread *thread)
{
    UNUSED(thread);
    return HDF_ERR_NOT_SUPPORT;
}
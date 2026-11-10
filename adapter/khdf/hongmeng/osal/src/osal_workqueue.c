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

#include "hdf_workqueue.h"
#include "hdf_core_log.h"
#include "osal_mem.h"

#include <libhmsrv_sys/hm_timer.h>
#include <udk/delay.h>
#include <udk/timer.h>
#include <udk/workqueue.h>

#define HDF_LOG_TAG hdf_workqueue

struct OsalHdfWork {
    struct udk_work work;
    HdfWorkFunc func;
    void *para;
};

struct OsalHdfDelayedWork {
    struct udk_delayed_work dwork;
    HdfWorkFunc func;
    void *para;
};

int32_t HdfWorkQueueInit(HdfWorkQueue *queue, char *name)
{
    struct udk_workqueue_attr attr;
    struct udk_workqueue *udkWq = NULL;

    HDF_LOGD("%s entry", __func__);
    if (queue == NULL || name == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    memset_s(&attr, sizeof(attr), 0, sizeof(attr));
    attr.nr_workers = 1;
    attr.nr_queues = UDK_WORK_PRIO_DEFAULT;

    udkWq = udk_workqueue_alloc(&attr);
    if (!udkWq) {
        HDF_LOGE("%s udk wq alloc failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    queue->realWorkQueue = udkWq;

    return HDF_SUCCESS;
}

static int HdfWorkHandler(struct udk_work *work)
{
    struct OsalHdfWork *hdfWork = NULL;

    if (work == NULL) {
        HDF_LOGE("%s invalid work", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    hdfWork = container_of(work, struct OsalHdfWork, work);
    udk_work_destroy(work);

    if (hdfWork->func != NULL) {
        hdfWork->func(hdfWork->para);
    } else {
        HDF_LOGW("%s no handler function", __func__);
    }

    return HDF_SUCCESS;
}

int32_t HdfWorkInit(HdfWork *work, HdfWorkFunc func, void *para)
{
    int ret;
    struct OsalHdfWork *hdfWork = NULL;

    if (work == NULL || func == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    work->realWork = NULL;

    hdfWork = OsalMemCalloc(sizeof(*hdfWork));
    if (hdfWork == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    hdfWork->func = func;
    hdfWork->para = para;
    ret = udk_work_init(&hdfWork->work, HdfWorkHandler, 0);
    if (ret < 0) {
        HDF_LOGE("%s udk work init fail,ret=%d", __func__, ret);
        OsalMemFree(hdfWork);
        return HDF_FAILURE;
    }

    work->realWork = hdfWork;
    return HDF_SUCCESS;
}

static int HdfDelayedWorkHandler(struct udk_work *work)
{
    struct udk_delayed_work *dwork = NULL;
    struct OsalHdfDelayedWork *hdfDwork = NULL;

    if (work == NULL) {
        HDF_LOGE("%s invalid work", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    dwork = container_of(work, struct udk_delayed_work, work);
    hdfDwork = container_of(dwork, struct OsalHdfDelayedWork, dwork);
    udk_delayed_work_destroy(dwork);

    if (hdfDwork->func != NULL) {
        hdfDwork->func(hdfDwork->para);
    } else {
        HDF_LOGW("%s no handler function", __func__);
    }

    return HDF_SUCCESS;
}

int32_t HdfDelayedWorkInit(HdfWork *work, HdfWorkFunc func, void *para)
{
    int ret;
    struct OsalHdfDelayedWork *hdfDwork = NULL;

    if (work == NULL || func == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    work->realWork = NULL;

    hdfDwork = OsalMemCalloc(sizeof(*hdfDwork));
    if (hdfDwork == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    hdfDwork->func = func;
    hdfDwork->para = para;
    ret = udk_delayed_work_init(&hdfDwork->dwork, HdfDelayedWorkHandler, 0);
    if (ret < 0) {
        HDF_LOGE("%s udk work init fail, ret=%d", __func__, ret);
        OsalMemFree(hdfDwork);
        return HDF_FAILURE;
    }

    work->realWork = hdfDwork;
    return HDF_SUCCESS;
}

void HdfWorkDestroy(HdfWork *work)
{
    if (work == NULL || work->realWork == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return;
    }

    OsalMemFree(work->realWork);
    work->realWork = NULL;
    return;
}

void HdfDelayedWorkDestroy(HdfWork *work)
{
    if (work == NULL || work->realWork == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return;
    }

    return HdfWorkDestroy(work);
}

void HdfWorkQueueDestroy(HdfWorkQueue *queue)
{
    if (queue == NULL || queue->realWorkQueue == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return;
    }

    udk_workqueue_destroy(queue->realWorkQueue);
    queue->realWorkQueue = NULL;
    return;
}

bool HdfAddWork(HdfWorkQueue *queue, HdfWork *work)
{
    int ret;

    if (queue == NULL || queue->realWorkQueue == NULL || work == NULL || work->realWork == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return false;
    }

    ret = udk_workqueue_add_work(queue->realWorkQueue, work->realWork);
    if (ret < 0) {
        HDF_LOGE("%s add work fail, ret=%d", __func__, ret);
        return false;
    }

    return true;
}

bool HdfAddDelayedWork(HdfWorkQueue *queue, HdfWork *work, uint32_t ms)
{
    int ret;

    if (queue == NULL || queue->realWorkQueue == NULL || work == NULL || work->realWork == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return false;
    }

    ret = udk_workqueue_add_delayed_work(queue->realWorkQueue, work->realWork, ms);
    if (ret < 0) {
        HDF_LOGE("%s add work fail, ret=%d", __func__, ret);
        return false;
    }

    return true;
}

unsigned int HdfWorkBusy(HdfWork *work)
{
    HDF_LOGW("hongmeng does not support work busy check");
    return 0;
}

bool HdfCancelWorkSync(HdfWork *work)
{
    int ret;
    struct udk_work *udkWork;

    if (work == NULL || work->realWork == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return false;
    }

    udkWork = work->realWork;
    ret = udk_workqueue_cancel_work(udkWork->wq, udkWork);
    if (ret < 0) {
        HDF_LOGE("%s cancel work fail, ret=%d", __func__, ret);
        return false;
    }

    return true;
}

bool HdfCancelDelayedWorkSync(HdfWork *work)
{
    int ret;
    struct udk_delayed_work *udkDwork;

    if (work == NULL || work->realWork == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return false;
    }

    udkDwork = work->realWork;
    ret = udk_workqueue_cancel_delayed_work(udkDwork->work.wq, udkDwork);
    if (ret < 0) {
        HDF_LOGE("%s cancel work fail, ret=%d", __func__, ret);
        return false;
    }

    return true;
}
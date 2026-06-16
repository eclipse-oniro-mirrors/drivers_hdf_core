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

#ifndef HDF_WORKQUEUE_H
#define HDF_WORKQUEUE_H

#include "hdf_types.h"

typedef void (*HdfWorkFunc)(void *);

typedef struct {
    void *realWork;
} HdfWork;

typedef struct {
    void *realWorkQueue;
} HdfWorkQueue;

int32_t HdfWorkQueueInit(HdfWorkQueue *queue, char *name);
int32_t HdfWorkInit(HdfWork *work, HdfWorkFunc func, void *para);
int32_t HdfDelayedWorkInit(HdfWork *work, HdfWorkFunc func, void *para);
void HdfWorkDestroy(HdfWork *work);
void HdfWorkQueueDestroy(HdfWorkQueue *queue);
void HdfDelayedWorkDestroy(HdfWork *work);
bool HdfAddWork(HdfWorkQueue *queue, HdfWork *work);
bool HdfAddDelayedWork(HdfWorkQueue *queue, HdfWork *work, uint32_t ms);
unsigned int HdfWorkBusy(HdfWork *work);
bool HdfCancelWorkSync(HdfWork *work);
bool HdfCancelDelayedWorkSync(HdfWork *work);

#endif /* HDF_WORKQUEUE_H */
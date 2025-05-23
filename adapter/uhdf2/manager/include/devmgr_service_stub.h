/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef DEVMGR_SERVICE_STUB_H
#define DEVMGR_SERVICE_STUB_H

#include "hdf_remote_service.h"
#include "devmgr_service_full.h"
#include "osal_mutex.h"

#define DEVICE_MANAGER_SERVICE "hdf_device_manager"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct DevmgrServiceStub {
    struct DevmgrServiceFull super;
    struct HdfRemoteService *remote;
    struct OsalMutex devmgrStubMutx;
};

enum {
    DEVMGR_SERVICE_ATTACH_DEVICE_HOST = 1,
    DEVMGR_SERVICE_ATTACH_DEVICE,
    DEVMGR_SERVICE_DETACH_DEVICE,
    DEVMGR_SERVICE_LOAD_DEVICE,
    DEVMGR_SERVICE_UNLOAD_DEVICE,
    DEVMGR_SERVICE_QUERY_DEVICE,
    DEVMGR_SERVICE_LIST_ALL_DEVICE,
    DEVMGR_SERVICE_LIST_ALL_HOST,
};

struct HdfObject *DevmgrServiceStubCreate(void);
void DevmgrServiceStubRelease(struct HdfObject *object);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DEVMGR_SERVICE_STUB_H */

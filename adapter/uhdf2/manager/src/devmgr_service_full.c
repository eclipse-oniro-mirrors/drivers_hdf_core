/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "devmgr_service_full.h"
#include "devhost_service_clnt.h"
#include "devhost_service_proxy.h"
#include "device_token_clnt.h"
#include "hdf_driver_installer.h"
#include "hdf_log.h"
#include "hdf_map.h"
#include "hdf_message_looper.h"
#include "osal_message.h"

#define HDF_LOG_TAG devmgr_service_full
#define INVALID_PID (-1)

static Map g_hostMap = {0};
#define HOST_INIT_DIE_NUM 1
#define HOST_MAX_DIE_NUM  3

static void CleanupDiedHostResources(struct DevHostServiceClnt *hostClnt, struct HdfRemoteService *service)
{
    OsalMutexLock(&hostClnt->hostLock);
    struct DevHostServiceProxy *hostProxy = (struct DevHostServiceProxy *)hostClnt->hostService;
    if (hostProxy != NULL) {
        if ((hostProxy->remote != NULL) && ((uintptr_t)hostProxy->remote == (uintptr_t)service)) {
            HDF_LOGI("%{public}s hostId: %{public}u remove current hostService", __func__, hostClnt->hostId);
            hostClnt->hostPid = INVALID_PID;
            DevHostServiceProxyRecycle(hostProxy);
            hostClnt->hostService = NULL;
            HdfSListFlush(&hostClnt->devices, DeviceTokenClntDelete);
        } else {
            hostProxy = (struct DevHostServiceProxy *)service->target;
            HDF_LOGI("%{public}s hostId: %{public}u remove old hostService", __func__, hostClnt->hostId);
            DevHostServiceProxyRecycle(hostProxy);
        }
    } else {
        hostProxy = (struct DevHostServiceProxy *)service->target;
        HDF_LOGI("%{public}s hostId: %{public}u remove old hostService, and current hostService is null",
            __func__, hostClnt->hostId);
        DevHostServiceProxyRecycle(hostProxy);
    }

    OsalMutexUnlock(&hostClnt->hostLock);
}

static int32_t DevmgrServiceFullHandleDeviceHostDied(struct DevHostServiceClnt *hostClnt,
    struct HdfRemoteService *service)
{
    bool isHostEmpty = HdfSListIsEmpty(&hostClnt->devices);

    CleanupDiedHostResources(hostClnt, service);
    if (isHostEmpty || hostClnt->stopFlag) {
        return 0;
    }

    if (g_hostMap.nodeSize == 0) {
        MapInit(&g_hostMap);
    }
    // host is started at init phase by init module, and it respawn by init module
    if (!HdfSListIsEmpty(&hostClnt->unloadDevInfos)) {
        return 0;
    }
    int *hostDieValue = (int *)MapGet(&g_hostMap, hostClnt->hostName);
    if (hostDieValue == NULL) {
        int hostDieNum = HOST_INIT_DIE_NUM;
        MapSet(&g_hostMap, hostClnt->hostName, &hostDieNum, sizeof(int));
    } else {
        if (*hostDieValue > HOST_MAX_DIE_NUM) {
            HDF_LOGE("host %{public}s die 4 times, remove it", hostClnt->hostName);
            *hostDieValue = 0;
            hostClnt->stopFlag = false;
            return INVALID_PID;
        }
        (*hostDieValue)++;
    }
    HDF_LOGI("%{public}s:%{public}d", __func__, __LINE__);
    struct IDriverInstaller *installer = DriverInstallerGetInstance();
    if (installer != NULL && installer->StartDeviceHost != NULL) {
        HDF_LOGI("%{public}s:%{public}d", __func__, __LINE__);
        int pid = installer->StartDeviceHost(hostClnt->hostId, hostClnt->hostName, true);
        OsalMutexLock(&hostClnt->hostLock);
        hostClnt->hostPid = pid;
        OsalMutexUnlock(&hostClnt->hostLock);
        return hostClnt->hostPid;
    }
    return INVALID_PID;
}

static void DevmgrServiceFullOnDeviceHostDied(struct DevmgrServiceFull *inst, uint32_t hostId,
    struct HdfRemoteService *service)
{
    (void)hostId;
    struct DevHostServiceClnt *hostClnt = NULL;
    struct DevHostServiceClnt *hostClntTmp = NULL;
    if (inst == NULL) {
        return;
    }
    OsalMutexLock(&inst->super.devMgrMutex);
    DLIST_FOR_EACH_ENTRY_SAFE(hostClnt, hostClntTmp, &inst->super.hosts, struct DevHostServiceClnt, node) {
        if (hostClnt->hostId == hostId) {
            int32_t ret = DevmgrServiceFullHandleDeviceHostDied(hostClnt, service);
            if (ret == INVALID_PID) {
                HDF_LOGE("%{public}s: failed to respawn host %{public}s", __func__, hostClnt->hostName);
            }
            break;
        }
    }
    OsalMutexUnlock(&inst->super.devMgrMutex);
}

int32_t DevmgrServiceFullDispatchMessage(struct HdfMessageTask *task, struct HdfMessage *msg)
{
    (void)task;
    struct DevmgrServiceFull *fullService = (struct DevmgrServiceFull *)DevmgrServiceGetInstance();
    if (msg == NULL) {
        HDF_LOGE("Input msg is null");
        return HDF_ERR_INVALID_PARAM;
    }
    switch (msg->messageId) {
        case DEVMGR_MESSAGE_DEVHOST_DIED: {
            int hostId = (int)(uintptr_t)msg->data[0];
            struct HdfRemoteService *service = (struct HdfRemoteService *)msg->data[1];
            DevmgrServiceFullOnDeviceHostDied(fullService, hostId, service);
            break;
        }
        default: {
            HDF_LOGE("wrong message(%{public}u)", msg->messageId);
        }
    }

    return HDF_SUCCESS;
}

struct HdfMessageTask *DevmgrServiceFullGetMessageTask(void)
{
    struct DevmgrServiceFull *fullService = (struct DevmgrServiceFull *)DevmgrServiceGetInstance();
    if (fullService != NULL) {
        return &fullService->task;
    }
    HDF_LOGE("Get message task failed, fullService is null");
    return NULL;
}

void DevmgrServiceFullConstruct(struct DevmgrServiceFull *inst)
{
    static struct IHdfMessageHandler handler = {.Dispatch = DevmgrServiceFullDispatchMessage};
    if (inst != NULL) {
        HdfMessageLooperConstruct(&inst->looper);
        DevmgrServiceConstruct(&inst->super);
        HdfMessageTaskConstruct(&inst->task, &inst->looper, &handler);
    }
}

struct HdfObject *DevmgrServiceFullCreate(void)
{
    static struct DevmgrServiceFull *instance = NULL;
    if (instance == NULL) {
        static struct DevmgrServiceFull fullInstance;
        DevmgrServiceFullConstruct(&fullInstance);
        instance = &fullInstance;
    }
    return (struct HdfObject *)instance;
}

int DeviceManagerIsQuickLoad(void)
{
    return false;
}

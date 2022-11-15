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

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/prctl.h>

#include "securec.h"
#include "parameter.h"
#include "devhost_service.h"
#include "devhost_service_full.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "hdf_power_manager.h"

#define HDF_LOG_TAG                    hdf_device_host
#define DEVHOST_INPUT_PARAM_NUM        3
#define DEVHOST_INPUT_PARAM_HOSTID_POS 1
#define PARAM_BUF_LEN 128

static void StartMemoryHook(const char* processName)
{
    const char defaultValue[PARAM_BUF_LEN] = { 0 };
    const char targetPrefix[] = "startup:";
    const int targetPrefixLen = strlen(targetPrefix);
    char paramValue[PARAM_BUF_LEN] = { 0 };
    int retParam = GetParameter("libc.hook_mode", defaultValue, paramValue, sizeof(paramValue));
    if (retParam <= 0 || strncmp(paramValue, targetPrefix, targetPrefixLen) != 0) {
        return;
    }
    if (strstr(paramValue + targetPrefixLen, processName) != NULL) {
        const int hookSignal = 36; // 36: native memory hooked signal
        HDF_LOGE("raise hook signal %{public}d to %{public}s", hookSignal, processName);
        raise(hookSignal);
    }
}

bool HdfStringToInt(const char *str, int *value)
{
    if (str == NULL || value == NULL) {
        return false;
    }

    char *end = NULL;
    errno = 0;
    const int base = 10;
    long result = strtol(str, &end, base);
    if (end == str || end[0] != '\0' || errno == ERANGE || result > INT_MAX || result < INT_MIN) {
        return false;
    }

    *value = (int)result;
    return true;
}

static void SetProcTitle(char **argv, const char *newTitle)
{
    size_t len = strlen(argv[0]);
    if (strlen(newTitle) > len) {
        HDF_LOGE("failed to set new process title because the '%{public}s' is too long", newTitle);
        return;
    }
    (void)memset_s(argv[0], len, 0, len);
    if (strcpy_s(argv[0], len + 1, newTitle) != EOK) {
        HDF_LOGE("failed to set new process title");
        return;
    }
    prctl(PR_SET_NAME, newTitle);
}

int main(int argc, char **argv)
{
    if (argc != DEVHOST_INPUT_PARAM_NUM) {
        HDF_LOGE("Devhost main parameter error, argc: %{public}d", argc);
        return HDF_ERR_INVALID_PARAM;
    }

    prctl(PR_SET_PDEATHSIG, SIGKILL); // host process should exit with devmgr process

    int hostId = 0;
    if (!HdfStringToInt(argv[DEVHOST_INPUT_PARAM_HOSTID_POS], &hostId)) {
        HDF_LOGE("Devhost main parameter error, argv[1]: %{public}s", argv[DEVHOST_INPUT_PARAM_HOSTID_POS]);
        return HDF_ERR_INVALID_PARAM;
    }
    const char *hostName = argv[argc - 1];
    HDF_LOGI("hdf device host %{public}s %{public}d start", hostName, hostId);
    SetProcTitle(argv, hostName);
    StartMemoryHook(hostName);

    struct IDevHostService *instance = DevHostServiceNewInstance(hostId, hostName);
    if (instance == NULL || instance->StartService == NULL) {
        HDF_LOGE("DevHostServiceGetInstance fail");
        return HDF_ERR_INVALID_OBJECT;
    }
    int status = instance->StartService(instance);
    if (status != HDF_SUCCESS) {
        HDF_LOGE("Devhost StartService fail, return: %{public}d", status);
        DevHostServiceFreeInstance(instance);
        return status;
    }
    HdfPowerManagerInit();
    struct DevHostServiceFull *fullService = (struct DevHostServiceFull *)instance;
    struct HdfMessageLooper *looper = &fullService->looper;
    if ((looper != NULL) && (looper->Start != NULL)) {
        looper->Start(looper);
    }

    DevHostServiceFreeInstance(instance);
    HdfPowerManagerExit();
    HDF_LOGI("hdf device host %{public}s %{public}d exit", hostName, hostId);
    return status;
}

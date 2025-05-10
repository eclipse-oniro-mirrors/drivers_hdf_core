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
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <getopt.h>

#include "securec.h"
#include "parameter.h"
#include "malloc.h"
#include "devhost_dump.h"
#include "devhost_service.h"
#include "devhost_service_full.h"
#include "hdf_base.h"
#include "hdf_core_log.h"
#include "hdf_power_manager.h"

#define HDF_LOG_TAG                    hdf_device_host
#define DEVHOST_MIN_INPUT_PARAM_NUM    5
#define DEVHOST_INPUT_PARAM_HOSTID_POS 1
#define DEVHOST_HOST_NAME_POS          2
#define DEVHOST_PROCESS_PRI_POS        3
#define DEVHOST_THREAD_PRI_POS         4
#define PARAM_BUF_LEN 128
#define MALLOPT_PARA_CNT 2
#define INVALID_PRIORITY "0"

typedef struct {
    int hostId;
    char *hostName;
    int schedPriority;
    int processPriority;
    int malloptKey;
    int malloptValue;
} HostConfig;

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

static void HdfSetProcPriority(int32_t procPriority, int32_t schedPriority)
{
//    int32_t schedPriority = 0;
//    int32_t procPriority = 0;
//
//    if (!HdfStringToInt(argv[DEVHOST_PROCESS_PRI_POS], &procPriority)) {
//        HDF_LOGE("procPriority parameter error: %{public}s", argv[DEVHOST_PROCESS_PRI_POS]);
//        return;
//    }
//    if (!HdfStringToInt(argv[DEVHOST_THREAD_PRI_POS], &schedPriority)) {
//        HDF_LOGE("schedPriority parameter error: %{public}s", argv[DEVHOST_THREAD_PRI_POS]);
//        return;
//    }

    if (setpriority(PRIO_PROCESS, 0, procPriority) != 0) {
        HDF_LOGE("host setpriority failed: %{public}d", errno);
    }

    struct sched_param param = {0};
    param.sched_priority = schedPriority;
    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        HDF_LOGE("host sched_setscheduler failed: %{public}d", errno);
    } else {
        HDF_LOGI("host sched_setscheduler succeed:%{public}d %{public}d", procPriority, schedPriority);
    }
}

static void SetMallopt(int32_t malloptKey, int32_t malloptValue)
{
    int ret = mallopt(malloptKey, malloptValue);
    if (ret != 1) {
        HDF_LOGE("mallopt failed, malloptKey:%{public}d, malloptValue:%{public}d, ret:%{public}d",
            malloptKey, malloptValue, ret);
    }
    HDF_LOGI("host set mallopt succeed, mallopt:%{public}d %{public}d", malloptKey, malloptValue);
}

static int ParseCommandLineArgs(int argc, char **argv, HostConfig *config)
{
    int opt;
    while ((opt = getopt(argc, argv, "i:n:p:s:m:v:")) != -1) {
        switch (opt) {
            case 'i':
                if (!HdfStringToInt(optarg, &config->hostId)) {
                    HDF_LOGE("Invalid process ID: %{public}s", optarg);
                    return HDF_ERR_INVALID_PARAM;
                }
                break;
            case 'n':
                config->hostName = optarg;
                break;
            case 'p':
                if (!HdfStringToInt(optarg, &config->schedPriority)) {
                    HDF_LOGE("Invalid process priority: %{public}s", optarg);
                    return HDF_ERR_INVALID_PARAM;
                }
                break;
            case 's':
                if (!HdfStringToInt(optarg, &config->processPriority)) {
                    HDF_LOGE("Invalid process priority: %{public}s", optarg);
                    return HDF_ERR_INVALID_PARAM;
                }
                break;
            case 'm':
                if (!HdfStringToInt(optarg, &config->malloptKey)) {
                    HDF_LOGE("Invalid mallopt key: %{public}s", optarg);
                    return HDF_ERR_INVALID_PARAM;
                }
                break;
            case 'v':
                if (!HdfStringToInt(optarg, &config->malloptValue)) {
                    HDF_LOGE("Invalid mallopt value: %{public}s", optarg);
                    return HDF_ERR_INVALID_PARAM;
                }
                break;
            default:
                HDF_LOGE("Unknown option: -%c", opt);
                return HDF_ERR_INVALID_PARAM;
        }
    }
    return HDF_SUCCESS;
}

int main(int argc, char **argv)
{
    HostConfig config = {0};
    if (ParseCommandLineArgs(argc, argv, &config) != HDF_SUCCESS) {
        printf("ParseCommandLineArgs(argc, argv, &config) != HDF_SUCCESS");
        return HDF_ERR_INVALID_PARAM;
    }

    prctl(PR_SET_PDEATHSIG, SIGKILL); // host process should exit with devmgr process

    int hostId = config.hostId;

    const char *hostName = config.hostName;
    HDF_LOGI("hdf device host %{public}s %{public}d start", hostName, hostId);
    SetProcTitle(argv, hostName);
    StartMemoryHook(hostName);
    if ((config.processPriority != 0) && (config.schedPriority != 0)) {
        HdfSetProcPriority(config.processPriority, config.schedPriority);
    }
    if ((config.malloptKey != 0) && (config.malloptValue != 0)) {
        SetMallopt(config.malloptKey, config.malloptValue);
    }

    struct IDevHostService *instance = DevHostServiceNewInstance(hostId, hostName);
    if (instance == NULL || instance->StartService == NULL) {
        HDF_LOGE("DevHostServiceGetInstance fail");
        return HDF_ERR_INVALID_OBJECT;
    }
    HDF_LOGD("create IDevHostService of %{public}s success", hostName);

    DevHostDumpInit();
    HDF_LOGD("%{public}s start device service begin", hostName);
    int status = instance->StartService(instance);
    if (status != HDF_SUCCESS) {
        HDF_LOGE("Devhost StartService fail, return: %{public}d", status);
        DevHostServiceFreeInstance(instance);
        DevHostDumpDeInit();
        return status;
    }

    HdfPowerManagerInit();
    struct DevHostServiceFull *fullService = (struct DevHostServiceFull *)instance;
    struct HdfMessageLooper *looper = &fullService->looper;
    if ((looper != NULL) && (looper->Start != NULL)) {
        HDF_LOGI("%{public}s start loop", hostName);
        looper->Start(looper);
    }

    DevHostServiceFreeInstance(instance);
    HdfPowerManagerExit();
    DevHostDumpDeInit();
    HDF_LOGI("hdf device host %{public}s %{public}d exit", hostName, hostId);
    return status;
}

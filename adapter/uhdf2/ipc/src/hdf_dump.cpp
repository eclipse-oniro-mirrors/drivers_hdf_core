/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hdf_dump.h"

#include "ipc_payload_statistics.h"

#include "file_ex.h"
#include "string_ex.h"
#include "unistd.h"

#include "hdf_base.h"
#include "hdf_dump_reg.h"
#include "hdf_log.h"
#include "hdf_sbuf.h"
#include "securec.h"

#define HDF_LOG_TAG hdf_dump

using namespace OHOS;

static DevHostDumpFunc g_dump = nullptr;

const char *HDF_DUMP_SUCCESS_STR = " success\n";
const char *HDF_DUMP_FAIL_STR = " fail\n";

// The maximum parameter is the parameter sent to the host, including public(num=2) and private(mux_num=20) parameters
static constexpr int32_t MAX_PARA_NUM = 22;

static bool HdfDumpIpcStatStart(std::string& result)
{
    result = std::string("HdfDumperIpcStatStart pid:") + std::to_string(getpid());
    bool ret = IPCPayloadStatistics::StartStatistics();
    result += ret ? HDF_DUMP_SUCCESS_STR : HDF_DUMP_FAIL_STR;
    return ret;
}

static int32_t HdfDumpIpcStatStop(std::string& result)
{
    result = std::string("HdfDumperIpcStatStop pid:") + std::to_string(getpid());
    bool ret = IPCPayloadStatistics::StopStatistics();
    result += ret ? HDF_DUMP_SUCCESS_STR : HDF_DUMP_FAIL_STR;
    return ret;
}

static int32_t HdfDumpIpcStatGet(std::string& result)
{
    result += "********************************GlobalStatisticsInfo********************************";
    result += "\nCurrentPid:";
    result += std::to_string(getpid());
    result += "\nTotalCount:";
    result += std::to_string(IPCPayloadStatistics::GetTotalCount());
    result += "\nTotalTimeCost:";
    result += std::to_string(IPCPayloadStatistics::GetTotalCost());
    std::vector<int32_t> pids;
    pids = IPCPayloadStatistics::GetPids();
    for (unsigned int i = 0; i < pids.size(); i++) {
        result += "\n--------------------------------ProcessStatisticsInfo-------------------------------";
        result += "\nCallingPid:";
        result += std::to_string(pids[i]);
        result += "\nCallingPidTotalCount:";
        result += std::to_string(IPCPayloadStatistics::GetCount(pids[i]));
        result += "\nCallingPidTotalTimeCost:";
        result += std::to_string(IPCPayloadStatistics::GetCost(pids[i]));
        std::vector<IPCInterfaceInfo> intfs;
        intfs = IPCPayloadStatistics::GetDescriptorCodes(pids[i]);
        for (unsigned int j = 0; j < intfs.size(); j++) {
            result += "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~InterfaceStatisticsInfo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
            result += "\nDescriptorCode:";
            result += Str16ToStr8(intfs[j].desc) + std::string("_") + std::to_string(intfs[j].code);
            result += "\nDescriptorCodeCount:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCount(pids[i], intfs[j].desc, intfs[j].code));
            result += "\nDescriptorCodeTimeCost:";
            result += "\nTotal:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pids[i], intfs[j].desc, intfs[j].code).totalCost);
            result += " | Max:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pids[i], intfs[j].desc, intfs[j].code).maxCost);
            result += " | Min:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pids[i], intfs[j].desc, intfs[j].code).minCost);
            result += " | Avg:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pids[i], intfs[j].desc, intfs[j].code).averCost);
            result += "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
        }
        result += "\n------------------------------------------------------------------------------------";
    }
    result += "\n************************************************************************************\n";

    return true;
}

bool HdfDumpIpcStat(int32_t fd, const char *cmd)
{
    if (cmd == NULL) {
        HDF_LOGE("%{public}s cmd is null", __func__);
        return HDF_FAILURE;
    }

    bool ret = false;
    std::string result;
	HDF_LOGI("%{public}s %{public}d", cmd, fd);
    if (strcmp(cmd, "--start-stat") == 0) {
        ret = HdfDumpIpcStatStart(result);
    } else if (strcmp(cmd, "--stop-stat") == 0) {
        ret = HdfDumpIpcStatStop(result);
    } else if (strcmp(cmd, "--stat") == 0) {
        ret = HdfDumpIpcStatGet(result);
    } else {
        return ret;
    }

    if (!OHOS::SaveStringToFd(fd, result)) {
        ret = false;
    }

    return ret;
}

void HdfRegisterDumpFunc(DevHostDumpFunc dump)
{
    g_dump = dump;
}

int32_t HdfDump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (g_dump == nullptr) {
        HDF_LOGE("%{public}s g_dump is null",  __func__);
        return HDF_FAILURE;
    }

    uint32_t argv = args.size();
    HDF_LOGI("%{public}s argv:%{public}u", __func__, argv);
 
    if (argv > MAX_PARA_NUM) {
        HDF_LOGE("%{public}s argv %{public}u is invalid",  __func__, argv);
        return HDF_FAILURE;
    }

    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    if (data == nullptr) {
        return HDF_FAILURE;
    }

    int32_t ret = HDF_SUCCESS;
    std::string result;
    const char *value = nullptr;

    struct HdfSBuf *reply = HdfSbufTypedObtain(SBUF_IPC);
    if (reply == nullptr) {
        goto FINISHED;
    }

    if (!HdfSbufWriteInt32(data, fd)) {
        goto FINISHED;
    }

    if (!HdfSbufWriteUint32(data, argv)) {
        goto FINISHED;
    }

    // Convert vector to sbuf structure
    for (uint32_t i = 0; i < argv; i++) {
        HDF_LOGI("%{public}s para:%{public}s", __func__, OHOS::Str16ToStr8(args.at(i)).data());
        if (!HdfSbufWriteString(data, OHOS::Str16ToStr8(args.at(i)).data())) {
            goto FINISHED;
        }
    }

    (void)g_dump(data, reply);

    value = HdfSbufReadString(reply);
    while (value != nullptr) {
        HDF_LOGI("%{public}s reply:%{public}s", __func__, value);
        result.append(value);
        value = HdfSbufReadString(reply);
    }

    if (!OHOS::SaveStringToFd(fd, result)) {
        ret = HDF_FAILURE;
    }

FINISHED:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}
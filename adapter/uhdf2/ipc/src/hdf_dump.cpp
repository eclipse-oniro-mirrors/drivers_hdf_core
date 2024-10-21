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
#include "hdf_core_log.h"
#include "hdf_sbuf.h"
#include "securec.h"

#define HDF_LOG_TAG hdf_dump

using namespace OHOS;

static DevHostDumpFunc g_dump = nullptr;

const char *HDF_DUMP_SUCCESS_STR = " success\n";
const char *HDF_DUMP_FAIL_STR = " fail\n";

// The maximum parameter is the parameter sent to the host, including public(num=2) and private(mux_num=20) parameters
static constexpr int32_t MAX_PARA_NUM = 22;

static void HdfDumpIpcStatStart(std::string& result)
{
    result = std::string("HdfDumpIpcStatStart pid:") + std::to_string(getpid());
    bool ret = IPCPayloadStatistics::StartStatistics();
    result += ret ? HDF_DUMP_SUCCESS_STR : HDF_DUMP_FAIL_STR;
    return;
}

static void HdfDumpIpcStatStop(std::string& result)
{
    result = std::string("HdfDumpIpcStatStop pid:") + std::to_string(getpid());
    bool ret = IPCPayloadStatistics::StopStatistics();
    result += ret ? HDF_DUMP_SUCCESS_STR : HDF_DUMP_FAIL_STR;
    return;
}

static void HdfDumpIpcStatGet(std::string& result)
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
        int32_t pid = pids[i];
        result += "\n--------------------------------ProcessStatisticsInfo-------------------------------";
        result += "\nCallingPid:";
        result += std::to_string(pid);
        result += "\nCallingPidTotalCount:";
        result += std::to_string(IPCPayloadStatistics::GetCount(pid));
        result += "\nCallingPidTotalTimeCost:";
        result += std::to_string(IPCPayloadStatistics::GetCost(pid));

        std::vector<IPCInterfaceInfo> intfs;
        intfs = IPCPayloadStatistics::GetDescriptorCodes(pid);
        for (unsigned int j = 0; j < intfs.size(); j++) {
            IPCInterfaceInfo intf = intfs[j];
            result += "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~InterfaceStatisticsInfo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
            result += "\nDescriptorCode:";
            result += Str16ToStr8(intf.desc) + std::string("_") + std::to_string(intf.code);
            result += "\nDescriptorCodeCount:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCount(pid, intf.desc, intf.code));

            result += "\nDescriptorCodeTimeCost:";
            result += "\nTotal:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pid, intf.desc, intf.code).totalCost);
            result += " | Max:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pid, intf.desc, intf.code).maxCost);
            result += " | Min:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pid, intf.desc, intf.code).minCost);
            result += " | Avg:";
            result += std::to_string(
                IPCPayloadStatistics::GetDescriptorCodeCost(pid, intf.desc, intf.code).averCost);
            result += "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
        }
        result += "\n------------------------------------------------------------------------------------";
    }
    result += "\n************************************************************************************\n";

    return;
}

void HdfDumpIpcStat(int32_t fd, const char *cmd)
{
    if (fd < 0) {
        HDF_LOGE("invalid fd %{public}d", fd);
        return;
    }

    if (cmd == nullptr) {
        HDF_LOGE("%{public}s cmd is null", __func__);
        return;
    }

    std::string result;
    HDF_LOGI("%{public}s %{public}d", cmd, fd);
    if (strcmp(cmd, "--start-stat") == 0) {
        HdfDumpIpcStatStart(result);
    } else if (strcmp(cmd, "--stop-stat") == 0) {
        HdfDumpIpcStatStop(result);
    } else if (strcmp(cmd, "--stat") == 0) {
        HdfDumpIpcStatGet(result);
    } else {
        HDF_LOGE("%{public}s invalid param", __func__);
        return;
    }

    if (!OHOS::SaveStringToFd(fd, result)) {
        HDF_LOGE("%{public}s SaveStringToFd failed", __func__);
    }

    return;
}

void HdfRegisterDumpFunc(DevHostDumpFunc dump)
{
    g_dump = dump;
}

int32_t HdfDump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (g_dump == nullptr || fd < 0) {
        HDF_LOGE("%{public}s g_dump fd:%{public}d",  __func__, fd);
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

    if (!HdfSbufWriteFileDescriptor(data, fd)) {
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
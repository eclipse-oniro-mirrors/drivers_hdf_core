/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include <benchmark/benchmark.h>
#include <string>
#include <vector>
#include <idevmgr_hdi.h>
#include <iservmgr_hdi.h>

#define HDF_LOG_TAG idevmgr_client_benchmark_test

namespace OHOS {
using OHOS::IRemoteObject;
using OHOS::sptr;
using OHOS::HDI::DeviceManager::V1_0::IDeviceManager;
using OHOS::HDI::DeviceManager::V1_0::HdiDevHostInfo;
using OHOS::HDI::ServiceManager::V1_0::IServiceManager;

namespace {
class DeviceManagerProxyBenchmarkTest : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State &state)
    {
        devmgr_ = IDeviceManager::Get();
    }

    void TearDown(const ::benchmark::State &state) {}

    sptr<IDeviceManager> devmgr_;
};

BENCHMARK_F(DeviceManagerProxyBenchmarkTest, LoadDeviceTest)(benchmark::State &state)
{
    for (auto _ : state) {
        int32_t ret = devmgr_->LoadDevice("sample_driver_service");
        (void)ret;
        ret = devmgr_->UnloadDevice("sample_driver_service");
        (void)ret;
    }
}
BENCHMARK_REGISTER_F(DeviceManagerProxyBenchmarkTest, LoadDeviceTest)
    ->Iterations(20)->Repetitions(3)->ReportAggregatesOnly();

BENCHMARK_F(DeviceManagerProxyBenchmarkTest, UnloadDeviceTest)(benchmark::State &state)
{
    for (auto _ : state) {
        int32_t ret = devmgr_->UnloadDevice("sample_driver_service");
        (void)ret;
    }
}
BENCHMARK_REGISTER_F(DeviceManagerProxyBenchmarkTest, UnloadDeviceTest)
    ->Iterations(20)->Repetitions(3)->ReportAggregatesOnly();

BENCHMARK_F(DeviceManagerProxyBenchmarkTest, ListAllDeviceTest)(benchmark::State &state)
{
    for (auto _ : state) {
        std::vector<HdiDevHostInfo> deviceInfos;
        int32_t ret = devmgr_->ListAllDevice(deviceInfos);
        (void)ret;
    }
}
BENCHMARK_REGISTER_F(DeviceManagerProxyBenchmarkTest, ListAllDeviceTest)
    ->Iterations(50)->Repetitions(3)->ReportAggregatesOnly();

BENCHMARK_F(DeviceManagerProxyBenchmarkTest, ListAllHostTest)(benchmark::State &state)
{
    for (auto _ : state) {
        std::vector<int> pidList;
        int32_t ret = devmgr_->ListAllHost(pidList);
        (void)ret;
    }
}
BENCHMARK_REGISTER_F(DeviceManagerProxyBenchmarkTest, ListAllHostTest)
    ->Iterations(50)->Repetitions(3)->ReportAggregatesOnly();
} // namespace
} // namespace OHOS

BENCHMARK_MAIN();

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
#include <iservmgr_hdi.h>
#include <iservstat_listener_hdi.h>
#include <hdf_device_class.h>

#define HDF_LOG_TAG servmgr_client_benchmark_test

namespace OHOS {
using OHOS::IRemoteObject;
using OHOS::sptr;
using OHOS::HDI::ServiceManager::V1_0::IServiceManager;
using OHOS::HDI::ServiceManager::V1_0::IServStatListener;
using OHOS::HDI::ServiceManager::V1_0::ServiceStatus;
using OHOS::HDI::ServiceManager::V1_0::ServStatListenerStub;
using OHOS::HDI::ServiceManager::V1_0::HdiServiceInfo;

namespace {
class ServStatListener : public ServStatListenerStub {
public:
    using StatusCallback = std::function<void(const ServiceStatus &)>;
    explicit ServStatListener(StatusCallback callback) : callback_(std::move(callback)) {}
    ~ServStatListener() = default;
    void OnReceive(const ServiceStatus &status) override
    {
        callback_(status);
    }

private:
    StatusCallback callback_;
};

class ServiceManagerProxyBenchmarkTest : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State &state)
    {
        servmgr_ = IServiceManager::Get();
    }

    void TearDown(const ::benchmark::State &state) {}

    sptr<IServiceManager> servmgr_;
};

BENCHMARK_F(ServiceManagerProxyBenchmarkTest, GetServiceTest)(benchmark::State &state)
{
    for (auto _ : state) {
        auto service = servmgr_->GetService("hdf_device_manager");
        (void)service;
    }
}
BENCHMARK_REGISTER_F(ServiceManagerProxyBenchmarkTest, GetServiceTest)
    ->Iterations(100)->Repetitions(3)->ReportAggregatesOnly();

BENCHMARK_F(ServiceManagerProxyBenchmarkTest, ListAllServiceTest)(benchmark::State &state)
{
    for (auto _ : state) {
        std::vector<HdiServiceInfo> serviceInfos;
        int32_t ret = servmgr_->ListAllService(serviceInfos);
        (void)ret;
    }
}
BENCHMARK_REGISTER_F(ServiceManagerProxyBenchmarkTest, ListAllServiceTest)
    ->Iterations(50)->Repetitions(3)->ReportAggregatesOnly();

BENCHMARK_F(ServiceManagerProxyBenchmarkTest, ListServiceByInterfaceDescTest)(benchmark::State &state)
{
    for (auto _ : state) {
        std::vector<std::string> serviceNames;
        int32_t ret = servmgr_->ListServiceByInterfaceDesc(serviceNames, "hdf_device_manager");
        (void)ret;
    }
}
BENCHMARK_REGISTER_F(ServiceManagerProxyBenchmarkTest, ListServiceByInterfaceDescTest)
    ->Iterations(50)->Repetitions(3)->ReportAggregatesOnly();

BENCHMARK_F(ServiceManagerProxyBenchmarkTest, RegisterServiceStatusListenerTest)(benchmark::State &state)
{
    for (auto _ : state) {
        sptr<IServStatListener> listener = new ServStatListener(
            ServStatListener::StatusCallback([](const ServiceStatus &status) {}));
        int32_t ret = servmgr_->RegisterServiceStatusListener(listener, DEVICE_CLASS_DEFAULT);
        (void)ret;
        ret = servmgr_->UnregisterServiceStatusListener(listener);
        (void)ret;
    }
}
BENCHMARK_REGISTER_F(ServiceManagerProxyBenchmarkTest, RegisterServiceStatusListenerTest)
    ->Iterations(30)->Repetitions(3)->ReportAggregatesOnly();
} // namespace
} // namespace OHOS

BENCHMARK_MAIN();

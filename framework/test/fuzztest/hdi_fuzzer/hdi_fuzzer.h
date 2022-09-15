/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef HDI_FUZZER_H
#define HDI_FUZZER_H

#include "iservstat_listener_hdi.h"

#define FUZZ_PROJECT_NAME "hdi_fuzzer"

namespace OHOS {
namespace HDI {
namespace ServiceManager {
namespace V1_0 {
class ServStatListenerFuzzer : public ServStatListenerStub {
public:
    virtual ~ServStatListenerFuzzer() {}

    void OnReceive(const ServiceStatus &status) override;
};
} // namespace V1_0
} // namespace ServiceManager
} // namespace HDI
} // namespace OHOS

#endif // HDI_FUZZER_H
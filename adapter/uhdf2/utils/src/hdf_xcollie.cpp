/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <unistd.h>
#include "hdf_xcollie.h"
#include "hdf_core_log.h"

namespace OHOS {
#ifdef HDFHICOLLIE_ENABLE
HdfXCollie::HdfXCollie(const std::string& tag, uint32_t timeoutSeconds,
    std::function<void(void *)> func, void* arg, uint32_t flag)
{
    tag_ = tag;
    id_ = HiviewDFX::XCollie::GetInstance().SetTimer(tag_, timeoutSeconds, func, arg, flag);
    isCanceled_ = false;
    HDF_LOGD("start HdfXCollie, tag:%{public}s,timeout:%{public}u,flag:%{public}u,id:%{public}d",
        tag_.c_str(), timeoutSeconds, flag, id_);
}

HdfXCollie::~HdfXCollie()
{
    CancelHdfXCollie();
}

void HdfXCollie::CancelHdfXCollie()
{
    if (!isCanceled_) {
        HiviewDFX::XCollie::GetInstance().CancelTimer(id_);
        isCanceled_ = true;
        HDF_LOGD("cancel HdfXCollie, tag:%{public}s,id:%{public}d", tag_.c_str(), id_);
    }
}
#else

HdfXCollie::HdfXCollie(const std::string& tag, uint32_t timeoutSeconds,
    std::function<void(void *)> func, void* arg, uint32_t flag)
{
}

HdfXCollie::~HdfXCollie()
{
}
#endif
}

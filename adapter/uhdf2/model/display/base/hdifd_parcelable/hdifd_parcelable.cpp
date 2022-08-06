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

#include "hdifd_parcelable.h"
#include <sstream>
#include "securec.h"
#include "hdf_log.h"
#include "ipc_file_descriptor.h"

namespace OHOS {
namespace HDI {
namespace Display {
HdifdParcelable::HdifdParcelable()
    : init_(false), hdiFd_(-1)
{
}

HdifdParcelable::HdifdParcelable(int32_t fd)
    : init_(true), hdiFd_(fd)
{
}

HdifdParcelable::~HdifdParcelable()
{
    if ((init_ != false) && (hdiFd_ < 0)) {
        close(hdiFd_);
    }
}

bool HdifdParcelable::Init(int32_t fd)
{
    bool ret = true;

    if (init_ == true) {
        HDF_LOGI("%{public}s: fd parcelable have been initialized", __func__);
        ret = false;
    } else {
        if (fd < 0) {
            hdiFd_ = -1;
        } else {
            hdiFd_ = dup(fd);
            ret = (hdiFd_ < 0) ? false : true;
        }
        if (ret == false) {
            return ret;
        }
        init_ = true;
    }
    return ret;
}

bool HdifdParcelable::WriteFileDescriptor(const int fd, Parcel& parcel)
{
    if (fd < 0) {
        return false;
    }
    int dupFd = dup(fd);
    if (dupFd < 0) {
        return false;
    }

    sptr<IPCFileDescriptor> descriptor = new (std::nothrow) IPCFileDescriptor(dupFd);
    if (descriptor == nullptr) {
        HDF_LOGE("%{public}s: create IPCFileDescriptor object failed", __func__);
        return false;
    }
    return parcel.WriteObject<IPCFileDescriptor>(descriptor);
}

int HdifdParcelable::ReadFileDescriptor(Parcel& parcel)
{
    sptr<IPCFileDescriptor> descriptor = parcel.ReadObject<IPCFileDescriptor>();
    if (descriptor == nullptr) {
        return -1;
    }
    int fd = descriptor->GetFd();
    if (fd < 0) {
        return -1;
    }
    return dup(fd);
}

bool HdifdParcelable::Marshalling(Parcel& parcel) const
{
    bool validFlag = (hdiFd_ >= 0);
    if (!parcel.WriteBool(validFlag)) {
        HDF_LOGE("%{public}s: parcel.WriteBool failed", __func__);
        return false;
    }
    if (validFlag && !WriteFileDescriptor(hdiFd_, parcel)) {
        HDF_LOGE("%{public}s: parcel.WriteFileDescriptor fd failed", __func__);
        return false;
    }
    return true;
}

sptr<HdifdParcelable> HdifdParcelable::Unmarshalling(Parcel& parcel)
{
    bool validFlag = false;
    if (!parcel.ReadBool(validFlag)) {
        HDF_LOGE("%{public}s: ReadBool validFlag failed", __func__);
        return nullptr;
    }
    int32_t fd = -1;
    if (validFlag) {
        fd = ReadFileDescriptor(parcel);
        if (fd < 0) {
            HDF_LOGE("%{public}s: ReadFileDescriptor fd failed", __func__);
            return nullptr;
        }
    }
    sptr<HdifdParcelable> newParcelable = new HdifdParcelable(fd);
    newParcelable->init_ = true;
    return newParcelable;
}

int32_t HdifdParcelable::GetFd()
{
    return hdiFd_;
}

int32_t HdifdParcelable::Move()
{
    init_ = false;
    return hdiFd_;
}

std::string HdifdParcelable::Dump() const
{
    std::stringstream os;
    os << "fd: {" << hdiFd_ << "}\n";
    return os.str();
}
} // Display
} // HDI
} // OHOS

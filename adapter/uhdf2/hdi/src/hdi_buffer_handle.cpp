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

#include "hdi_buffer_handle.h"
#include "hdf_log.h"
#include "hdi_buffer_handle_util.h"
#include <sstream>

#define HDF_LOG_TAG hdi_buffer_handle

namespace OHOS {
namespace HDI {
namespace Base {
HdiBufferHandle::HdiBufferHandle() : handle_(nullptr) {}

HdiBufferHandle::~HdiBufferHandle()
{
    if (handle_ != nullptr) {
        FreeBufferHandle(handle_);
    }
}

HdiBufferHandle::HdiBufferHandle(BufferHandle &handle) : HdiBufferHandle()
{
    handle_ = CloneBufferHandle(&handle);
}

HdiBufferHandle::HdiBufferHandle(const HdiBufferHandle &other) : HdiBufferHandle()
{
    if (other.handle_ == nullptr) {
        return;
    }
    handle_ = CloneBufferHandle(other.handle_);
}

HdiBufferHandle::HdiBufferHandle(HdiBufferHandle &&other) noexcept : HdiBufferHandle()
{
    handle_ = other.handle_;
    other.handle_ = nullptr;
}

HdiBufferHandle &HdiBufferHandle::operator=(const HdiBufferHandle &other)
{
    if (this != &other) {
        if (handle_ != nullptr) {
            FreeBufferHandle(handle_);
        }
        handle_ = CloneBufferHandle(other.handle_);
    }
    return *this;
}

HdiBufferHandle &HdiBufferHandle::operator=(HdiBufferHandle &&other) noexcept
{
    if (this != &other) {
        if (handle_ != nullptr) {
            FreeBufferHandle(handle_);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

bool HdiBufferHandle::Marshalling(Parcel &parcel) const
{
    MessageParcel &messageParcel = static_cast<MessageParcel &>(parcel);
    bool isValid = handle_ != nullptr ? true : false;
    if (!messageParcel.WriteBool(isValid)) {
        HDF_LOGI("%{public}s: failed to write valid flag of buffer handle", __func__);
        return false;
    }

    if (!isValid) {
        HDF_LOGD("%{public}s: write valid buffer handle", __func__);
        return true;
    }

    if (!messageParcel.WriteUint32(handle_->reserveFds) || !messageParcel.WriteUint32(handle_->reserveInts) ||
        !messageParcel.WriteInt32(handle_->width) || !messageParcel.WriteInt32(handle_->stride) ||
        !messageParcel.WriteInt32(handle_->height) || !messageParcel.WriteInt32(handle_->size) ||
        !messageParcel.WriteInt32(handle_->format) || !messageParcel.WriteInt64(handle_->usage) ||
        !messageParcel.WriteUint64(handle_->phyAddr) || !messageParcel.WriteInt32(handle_->key)) {
        HDF_LOGE("%{public}s: a lot failed", __func__);
        return false;
    }

    bool validFd = (handle_->fd >= 0);
    if (!messageParcel.WriteBool(validFd)) {
        HDF_LOGE("%{public}s: failed to write valid flag of fd", __func__);
        return false;
    }
    if (validFd && !messageParcel.WriteFileDescriptor(handle_->fd)) {
        HDF_LOGE("%{public}s: failed to write fd", __func__);
        return false;
    }

    if (!WriteReserveData(messageParcel, *handle_)) {
        return false;
    }

    return true;
}

sptr<HdiBufferHandle> HdiBufferHandle::Unmarshalling(Parcel &parcel)
{
    sptr<HdiBufferHandle> newParcelable = new HdiBufferHandle();
    if (!newParcelable->ExtractFromParcel(parcel)) {
        return nullptr;
    }
    return newParcelable;
}

BufferHandle *HdiBufferHandle::Move()
{
    BufferHandle *handlePtr = handle_;
    handle_ = nullptr;
    return handlePtr;
}

std::string HdiBufferHandle::Dump() const
{
    std::stringstream os;
    os << "{";
    if (handle_ != nullptr) {
        os << "fd:" << handle_->fd << ", ";
        os << "width:" << handle_->width << ", ";
        os << "stride:" << handle_->stride << ", ";
        os << "height:" << handle_->height << ", ";
        os << "size:" << handle_->size << ", ";
        os << "format:" << handle_->format << ", ";
        os << "usage:" << handle_->usage << ", ";
        os << "phyAddr:" << handle_->phyAddr << ", ";
        os << "key:" << handle_->key << ", ";
        os << "}\nreserveFds[" << handle_->reserveFds << "]:{";

        uint32_t i = 0;
        for (; i < handle_->reserveFds; i++) {
            os << handle_->reserve[i] << ", ";
        }
        os << "},\nreserveInts[" << handle_->reserveInts << "]:{";

        uint32_t n = 0;
        for (; n < handle_->reserveInts; n++) {
            os << handle_->reserve[i + n] << ", ";
        }
    }
    os << "}\n";
    return os.str();
}

bool HdiBufferHandle::ExtractFromParcel(Parcel &parcel)
{
    MessageParcel &messageParcel = static_cast<MessageParcel &>(parcel);
    if (!messageParcel.ReadBool()) {
        HDF_LOGD("%{public}s: read invalid buffer handle", __func__);
        return true;
    }
    uint32_t reserveFds = 0;
    uint32_t reserveInts = 0;
    if (!messageParcel.ReadUint32(reserveFds) || !messageParcel.ReadUint32(reserveInts)) {
        HDF_LOGE("%{public}s: failed to read reserveFds or reserveInts", __func__);
        return false;
    }
    if ((handle_ = AllocateBufferHandle(reserveFds, reserveInts)) == NULL) {
        HDF_LOGE("%{public}s: failed to malloc BufferHandle", __func__);
        return false;
    }
    bool validFd = false;
    if (!messageParcel.ReadInt32(handle_->width) || !messageParcel.ReadInt32(handle_->stride) ||
        !messageParcel.ReadInt32(handle_->height) || !messageParcel.ReadInt32(handle_->size) ||
        !messageParcel.ReadInt32(handle_->format) || !messageParcel.ReadUint64(handle_->usage) ||
        !messageParcel.ReadUint64(handle_->phyAddr) || !messageParcel.ReadInt32(handle_->key) ||
        !messageParcel.ReadBool(validFd)) {
        HDF_LOGE("%{public}s: failed to parcel read", __func__);
        return false;
    }
    if (validFd) {
        handle_->fd = messageParcel.ReadFileDescriptor();
        if (handle_->fd == -1) {
            FreeBufferHandle(handle_);
            HDF_LOGE("%{public}s: failed to read fd", __func__);
            return false;
        }
    }

    if (!ReadReserveData(messageParcel, *handle_)) {
        FreeBufferHandle(handle_);
        return false;
    }
    return true;
}

bool HdiBufferHandle::WriteReserveData(MessageParcel &messageParcel, const BufferHandle &handle)
{
    for (uint32_t i = 0; i < handle.reserveFds; i++) {
        if (!messageParcel.WriteFileDescriptor(handle.reserve[i])) {
            HDF_LOGE("%{public}s: failed to write reserved fd value", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle.reserveInts; j++) {
        if (!messageParcel.WriteInt32(handle.reserve[handle.reserveFds + j])) {
            HDF_LOGE("%{public}s: failed to write reserved integer value", __func__);
            return false;
        }
    }
    return true;
}

bool HdiBufferHandle::ReadReserveData(MessageParcel &messageParcel, BufferHandle &handle)
{
    for (uint32_t i = 0; i < handle.reserveFds; i++) {
        handle.reserve[i] = messageParcel.ReadFileDescriptor();
        if (handle.reserve[i] == -1) {
            HDF_LOGE("%{public}s: failed to read reserved fd value", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle.reserveInts; j++) {
        if (!messageParcel.ReadInt32(handle.reserve[handle.reserveFds + j])) {
            HDF_LOGE("%{public}s: failed to read reserved integer value", __func__);
            return false;
        }
    }
    return true;
}
} // namespace Base
} // namespace HDI
} // namespace OHOS
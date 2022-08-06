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

#include "buffer_handle_parcelable.h"
#include <sstream>
#include "hdf_log.h"
#include "ipc_file_descriptor.h"
#include "securec.h"

namespace OHOS {
namespace HDI {
namespace Display {
BufferHandleParcelable::BufferHandleParcelable() : init_(false), handle_(nullptr) {}

BufferHandleParcelable::BufferHandleParcelable(BufferHandle &handle) : init_(true), handle_(&handle) {}

BufferHandleParcelable::~BufferHandleParcelable()
{
    if (init_ == false || handle_ == nullptr) {
        return;
    }
    FreeBufferHandle(handle_);
}

bool BufferHandleParcelable::Init(const BufferHandle &handle)
{
    if (init_ == true) {
        HDF_LOGW("%{public}s: bufferhandle parcel have been initialized", __func__);
        return false;
    }

    size_t handleSize = sizeof(BufferHandle) + (sizeof(int32_t) * (handle.reserveFds + handle.reserveInts));
    handle_ = static_cast<BufferHandle *>(malloc(handleSize));
    if (handle_ == nullptr) {
        HDF_LOGE("%{public}s: InitBufferHandle malloc %zu failed", __func__, handleSize);
        return false;
    }

    if (memcpy_s(handle_, handleSize, &handle, handleSize) != EOK) {
        HDF_LOGE("%{public}s: clone bufferhandle failed", __func__);
        return false;
    }

    if (handle.fd == -1) {
        handle_->fd = handle.fd;
    } else {
        handle_->fd = dup(handle.fd);
        if (handle_->fd == -1) {
            HDF_LOGE("%{public}s: bufferhandle dup fd failed", __func__);
            free(handle_);
            handle_ = nullptr;
            return false;
        }
    }

    for (uint32_t i = 0; i < handle_->reserveFds; i++) {
        handle_->reserve[i] = dup(handle.reserve[i]);
        if (handle_->reserve[i] == -1) {
            FreeBufferHandle(handle_);
            HDF_LOGE("%{public}s ReadFileDescriptor reserve failed", __func__);
            return false;
        }
    }
    init_ = true;

    return true;
}

bool BufferHandleParcelable::Marshalling(Parcel &parcel) const
{
    if (handle_ == nullptr) {
        parcel.WriteBool(false);
        return true;
    }
    if (!parcel.WriteUint32(handle_->reserveFds) || !parcel.WriteUint32(handle_->reserveInts) ||
        !parcel.WriteInt32(handle_->width) || !parcel.WriteInt32(handle_->stride) ||
        !parcel.WriteInt32(handle_->height) || !parcel.WriteInt32(handle_->size) ||
        !parcel.WriteInt32(handle_->format) || !parcel.WriteInt64(handle_->usage) ||
        !parcel.WriteUint64(handle_->phyAddr) || !parcel.WriteInt32(handle_->key)) {
        HDF_LOGE("%{public}s: a lot failed", __func__);
        return false;
    }
    bool validFd = (handle_->fd >= 0);
    if (!parcel.WriteBool(validFd)) {
        HDF_LOGE("%{public}s parcel.WriteBool failed", __func__);
        return false;
    }
    if (validFd && !WriteFileDescriptor(handle_->fd, parcel)) {
        HDF_LOGE("%{public}s parcel.WriteFileDescriptor fd failed", __func__);
        return false;
    }

    for (uint32_t i = 0; i < handle_->reserveFds; i++) {
        if (!WriteFileDescriptor(handle_->reserve[i], parcel)) {
            HDF_LOGE("%{public}s parcel.WriteFileDescriptor reserveFds failed", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle_->reserveInts; j++) {
        if (!parcel.WriteInt32(handle_->reserve[handle_->reserveFds + j])) {
            HDF_LOGE("%{public}s parcel.WriteInt32 reserve failed", __func__);
            return false;
        }
    }
    return true;
}

bool BufferHandleParcelable::ExtractFromParcel(Parcel &parcel)
{
    if (init_ == true) {
        HDF_LOGI("%{public}s: bufferhandle parcel have been initialized", __func__);
        return false;
    }
    auto hasVal = parcel.ReadBool();
    if (!hasVal) {
        return true;
    }

    uint32_t reserveFds = 0;
    uint32_t reserveInts = 0;
    if (!parcel.ReadUint32(reserveFds) || !parcel.ReadUint32(reserveInts)) {
        HDF_LOGE("%{public}s: parcel.ReadUint32 reserveFds failed", __func__);
        return false;
    }
    size_t handleSize = sizeof(BufferHandle) + (sizeof(int32_t) * (reserveFds + reserveInts));
    handle_ = static_cast<BufferHandle *>(malloc(handleSize));
    if (handle_ == nullptr) {
        HDF_LOGE("%{public}s: BufferHandle malloc %zu failed", __func__, handleSize);
        return false;
    }
    handle_->reserveFds = reserveFds;
    handle_->reserveInts = reserveInts;
    bool validFd = false;
    if (!parcel.ReadInt32(handle_->width) || !parcel.ReadInt32(handle_->stride) || !parcel.ReadInt32(handle_->height) ||
        !parcel.ReadInt32(handle_->size) || !parcel.ReadInt32(handle_->format) || !parcel.ReadUint64(handle_->usage) ||
        !parcel.ReadUint64(handle_->phyAddr) || !parcel.ReadInt32(handle_->key) || !parcel.ReadBool(validFd)) {
        HDF_LOGE("%{public}s: parcel read failed", __func__);
        return false;
    }
    handle_->virAddr = 0;
    if (validFd) {
        handle_->fd = ReadFileDescriptor(parcel);
        if (handle_->fd == -1) {
            HDF_LOGE("%{public}s: ReadFileDescriptor fd failed", __func__);
            return false;
        }
    }
    for (uint32_t i = 0; i < handle_->reserveFds; i++) {
        handle_->reserve[i] = ReadFileDescriptor(parcel);
        if (handle_->reserve[i] == -1) {
            FreeBufferHandle(handle_);
            HDF_LOGE("%{public}s: ReadFileDescriptor reserve failed", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle_->reserveInts; j++) {
        if (!parcel.ReadInt32(handle_->reserve[reserveFds + j])) {
            FreeBufferHandle(handle_);
            HDF_LOGE("%{public}s: ReadInt32 reserve failed", __func__);
            return false;
        }
    }
    return true;
}

sptr<BufferHandleParcelable> BufferHandleParcelable::Unmarshalling(Parcel &parcel)
{
    sptr<BufferHandleParcelable> newParcelable = new BufferHandleParcelable();
    bool ret = newParcelable->ExtractFromParcel(parcel);
    if (!ret) {
        return nullptr;
    } else {
        newParcelable->init_ = true;
    }
    return newParcelable;
}

BufferHandle *BufferHandleParcelable::GetBufferHandle()
{
    return handle_;
}

BufferHandle *BufferHandleParcelable::Move()
{
    BufferHandle *handlePtr = handle_;
    handle_ = nullptr;
    init_ = false;
    return handlePtr;
}

bool BufferHandleParcelable::WriteFileDescriptor(const int fd, Parcel &parcel)
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

int BufferHandleParcelable::ReadFileDescriptor(Parcel &parcel)
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

void BufferHandleParcelable::FreeBufferHandle(BufferHandle *handle)
{
    if (handle->fd != -1) {
        close(handle->fd);
    }
    for (uint32_t i = 0; i < handle->reserveFds; i++) {
        if (handle->reserve[i] != -1) {
            close(handle->reserve[i]);
        }
    }
    free(handle);
}

std::string BufferHandleParcelable::Dump() const
{
    std::stringstream os;

    os << "{";
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

    os << "}\n";
    return os.str();
}
} // namespace Display
} // namespace HDI
} // namespace OHOS

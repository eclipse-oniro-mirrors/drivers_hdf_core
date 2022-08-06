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

#include "display_buffer_hdi_impl.h"
#include "hdf_log.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {
DisplayBufferHdiImpl::DisplayBufferHdiImpl(bool isAllocLocal)
{
    allocator_ = IAllocatorInterface::Get(isAllocLocal);
    if (allocator_ == nullptr) {
        HDF_LOGE("%{public}s: get IAllocatorInterface failure", __func__);
        return;
    }
    mapper_ = IMapperInterface::Get(true);
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: get IMapperInterface failure", __func__);
    }
}

int32_t DisplayBufferHdiImpl::AllocMem(const AllocInfo &info, BufferHandle *&handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer;
    if (allocator_ == nullptr) {
        HDF_LOGE("%{public}s: allocator_ is nullptr", __func__);
        return -1;
    }
    int32_t ret = allocator_->AllocMem(info, hdiBuffer);
    if (ret == HDF_SUCCESS) {
        handle = hdiBuffer->Move();
    } else {
        handle = nullptr;
    }
    return ret;
}

void DisplayBufferHdiImpl::FreeMem(const BufferHandle &handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer = new BufferHandleParcelable(const_cast<BufferHandle &>(handle));
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: mapper_ is nullptr", __func__);
        return -1;
    }
    mapper_->FreeMem(hdiBuffer);
}

void *DisplayBufferHdiImpl::Mmap(const BufferHandle &handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer = new BufferHandleParcelable(const_cast<BufferHandle &>(handle));
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: mapper_ is nullptr", __func__);
        return nullptr;
    }
    int32_t ret = mapper_->Mmap(hdiBuffer);
    (void)hdiBuffer->Move();
    void *virAddr = (ret == HDF_SUCCESS ? handle.virAddr : nullptr);
    return virAddr;
}

void *DisplayBufferHdiImpl::MmapCache(const BufferHandle &handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer = new BufferHandleParcelable(const_cast<BufferHandle &>(handle));
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: mapper_ is nullptr", __func__);
        return nullptr;
    }
    int32_t ret = mapper_->MmapCache(hdiBuffer);
    (void)hdiBuffer->Move();
    void *virAddr = (ret == HDF_SUCCESS ? handle.virAddr : nullptr);
    return virAddr;
}

int32_t DisplayBufferHdiImpl::Unmap(const BufferHandle &handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer = new BufferHandleParcelable(const_cast<BufferHandle &>(handle));
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: mapper_ is nullptr", __func__);
        return nullptr;
    }
    int32_t ret = mapper_->Unmap(hdiBuffer);
    (void)hdiBuffer->Move();
    return ret;
}

int32_t DisplayBufferHdiImpl::FlushCache(const BufferHandle &handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer = new BufferHandleParcelable(const_cast<BufferHandle &>(handle));
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: mapper_ is nullptr", __func__);
        return nullptr;
    }
    int32_t ret = mapper_->FlushCache(hdiBuffer);
    (void)hdiBuffer->Move();
    return ret;
}

int32_t DisplayBufferHdiImpl::FlushMCache(const BufferHandle &handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer = new BufferHandleParcelable(const_cast<BufferHandle &>(handle));
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: mapper_ is nullptr", __func__);
        return nullptr;
    }
    int32_t ret = mapper_->FlushMCache(hdiBuffer);
    (void)hdiBuffer->Move();
    return ret;
}

int32_t DisplayBufferHdiImpl::InvalidateCache(const BufferHandle &handle) const
{
    sptr<BufferHandleParcelable> hdiBuffer = new BufferHandleParcelable(const_cast<BufferHandle &>(handle));
    if (mapper_ == nullptr) {
        HDF_LOGE("%{public}s: mapper_ is nullptr", __func__);
        return nullptr;
    }
    int32_t ret = mapper_->InvalidateCache(hdiBuffer);
    (void)hdiBuffer->Move();
    return ret;
}

int32_t DisplayBufferHdiImpl::IsSupportedAlloc(
    const std::vector<VerifyAllocInfo> &infos, std::vector<bool> &supporteds) const
{
    (void)infos;
    (void)supporteds;
    return HDF_ERR_NOT_SUPPORT;
}
} // namespace V1_0
} // namespace Buffer
} // namespace Display
} // namespace HDI
} // namespace OHOS

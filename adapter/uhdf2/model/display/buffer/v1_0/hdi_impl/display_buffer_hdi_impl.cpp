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

#include "v1_0/hdi_impl/display_buffer_hdi_impl.h"
#include "hilog/log.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {

#ifndef BUFFER_HDI_IMPL_LOGE
#define BUFFER_HDI_IMPL_LOGE(format, ...)              \
    do {                                               \
        HILOG_ERROR(LOG_CORE,                          \
            "\033[0;32;31m"                            \
            "[%{public}s:%{public}d] " format "\033[m" \
            "\n",                                      \
            __FUNCTION__, __LINE__, ##__VA_ARGS__);    \
    } while (0)
#endif

#ifndef CHECK_NULLPOINTER_RETURN_VALUE
#define CHECK_NULLPOINTER_RETURN_VALUE(pointer, ret)                  \
    do {                                                              \
        if ((pointer) == NULL) {                                      \
            BUFFER_HDI_IMPL_LOGE("pointer is null and return ret\n"); \
            return (ret);                                             \
        }                                                             \
    } while (0)
#endif

#ifndef CHECK_NULLPOINTER_RETURN
#define CHECK_NULLPOINTER_RETURN(pointer)                             \
    do {                                                              \
        if ((pointer) == NULL) {                                      \
            BUFFER_HDI_IMPL_LOGE("pointer is null and return\n");     \
            return;                                                   \
        }                                                             \
    } while (0)
#endif

IDisplayBuffer *IDisplayBuffer::Get()
{
    IDisplayBuffer *instance = new DisplayBufferHdiImpl();
    if (instance == nullptr) {
        return nullptr;
    }
    return instance;
}

DisplayBufferHdiImpl::DisplayBufferHdiImpl(bool isAllocLocal)
{
    allocator_ = IAllocator::Get(isAllocLocal);
    CHECK_NULLPOINTER_RETURN(allocator_);
    mapper_ = IMapper::Get(true);
    CHECK_NULLPOINTER_RETURN(mapper_);
}

int32_t DisplayBufferHdiImpl::AllocMem(const AllocInfo &info, BufferHandle *&handle) const
{
    CHECK_NULLPOINTER_RETURN_VALUE(allocator_, HDF_FAILURE);
    sptr<NativeBuffer> hdiBuffer;
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
    CHECK_NULLPOINTER_RETURN(mapper_);
    sptr<NativeBuffer> hdiBuffer = new NativeBuffer();
    hdiBuffer->SetBufferHandle(const_cast<BufferHandle*>(&handle), true);
    mapper_->FreeMem(hdiBuffer);
}

void *DisplayBufferHdiImpl::Mmap(const BufferHandle &handle) const
{
    CHECK_NULLPOINTER_RETURN_VALUE(mapper_, nullptr);
    sptr<NativeBuffer> hdiBuffer = new NativeBuffer();
    hdiBuffer->SetBufferHandle(const_cast<BufferHandle*>(&handle));
    int32_t ret = mapper_->Mmap(hdiBuffer);
    void *virAddr = (ret == HDF_SUCCESS ? handle.virAddr : nullptr);
    return virAddr;
}

void *DisplayBufferHdiImpl::MmapCache(const BufferHandle &handle) const
{
    CHECK_NULLPOINTER_RETURN_VALUE(mapper_, nullptr);
    sptr<NativeBuffer> hdiBuffer = new NativeBuffer();
    hdiBuffer->SetBufferHandle(const_cast<BufferHandle*>(&handle));
    int32_t ret = mapper_->MmapCache(hdiBuffer);
    void *virAddr = (ret == HDF_SUCCESS ? handle.virAddr : nullptr);
    return virAddr;
}

int32_t DisplayBufferHdiImpl::Unmap(const BufferHandle &handle) const
{
    CHECK_NULLPOINTER_RETURN_VALUE(mapper_, HDF_FAILURE);
    sptr<NativeBuffer> hdiBuffer = new NativeBuffer();
    hdiBuffer->SetBufferHandle(const_cast<BufferHandle*>(&handle));
    int32_t ret = mapper_->Unmap(hdiBuffer);
    return ret;
}

int32_t DisplayBufferHdiImpl::FlushCache(const BufferHandle &handle) const
{
    CHECK_NULLPOINTER_RETURN_VALUE(mapper_, HDF_FAILURE);
    sptr<NativeBuffer> hdiBuffer = new NativeBuffer();
    hdiBuffer->SetBufferHandle(const_cast<BufferHandle*>(&handle));
    int32_t ret = mapper_->FlushCache(hdiBuffer);
    return ret;
}

int32_t DisplayBufferHdiImpl::FlushMCache(const BufferHandle &handle) const
{
    CHECK_NULLPOINTER_RETURN_VALUE(mapper_, HDF_FAILURE);
    sptr<NativeBuffer> hdiBuffer = new NativeBuffer();
    hdiBuffer->SetBufferHandle(const_cast<BufferHandle*>(&handle));
    int32_t ret = mapper_->FlushMCache(hdiBuffer);
    return ret;
}

int32_t DisplayBufferHdiImpl::InvalidateCache(const BufferHandle &handle) const
{
    CHECK_NULLPOINTER_RETURN_VALUE(mapper_, HDF_FAILURE);
    sptr<NativeBuffer> hdiBuffer = new NativeBuffer();
    hdiBuffer->SetBufferHandle(const_cast<BufferHandle*>(&handle));
    int32_t ret = mapper_->InvalidateCache(hdiBuffer);
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

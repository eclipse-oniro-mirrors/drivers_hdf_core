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

#ifndef OHOS_HDI_DISPLAY_V1_0_DISPLAY_BUFFER_HDI_IMPL_H
#define OHOS_HDI_DISPLAY_V1_0_DISPLAY_BUFFER_HDI_IMPL_H

#include "buffer_handle.h"
#include "v1_0/display_buffer_type.h"
#include "v1_0/iallocator_interface.h"
#include "v1_0/imapper_interface.h"
#include "v1_0/include/idisplay_buffer.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {
class DisplayBufferHdiImpl : public IDisplayBuffer {
public:
    explicit DisplayBufferHdiImpl(bool isAllocLocal = false);
    virtual ~DisplayBufferHdiImpl() = default;

    virtual int32_t AllocMem(const AllocInfo &info, BufferHandle *&handle) const override;
    virtual void FreeMem(const BufferHandle &handle) const override;
    virtual void *Mmap(const BufferHandle &handle) const override;
    virtual void *MmapCache(const BufferHandle &handle) const override;
    virtual int32_t Unmap(const BufferHandle &handle) const override;
    virtual int32_t FlushCache(const BufferHandle &handle) const override;
    virtual int32_t FlushMCache(const BufferHandle &handle) const override;
    virtual int32_t InvalidateCache(const BufferHandle &handle) const override;
    virtual int32_t IsSupportedAlloc(
        const std::vector<VerifyAllocInfo> &infos, std::vector<bool> &supporteds) const override;

private:
    sptr<IAllocatorInterface> allocator_;
    sptr<IMapperInterface> mapper_;
};
} // namespace V1_0
} // namespace Buffer
} // namespace Display
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_DISPLAY_V1_0_DISPLAY_BUFFER_HDI_IMPL_H

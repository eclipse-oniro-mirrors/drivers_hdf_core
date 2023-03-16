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

#ifndef HDI_BUFFER_UTIL_H
#define HDI_BUFFER_UTIL_H

#include "buffer_handle.h"
#include "hdf_sbuf.h"

#define MAX_RESERVE_FDS 1024
#define MAX_RESERVE_INTS 1024

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BufferHandle *AllocateNativeBufferHandle(uint32_t reserveFds, uint32_t reserveInts);
BufferHandle *CloneNativeBufferHandle(const BufferHandle *other);
void FreeNativeBufferHandle(BufferHandle *handle);

bool HdfSbufWriteNativeBufferHandle(struct HdfSBuf *data, const BufferHandle *handle);
BufferHandle *HdfSbufReadNativeBufferHandle(struct HdfSBuf *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HDI_BUFFER_UTIL_H
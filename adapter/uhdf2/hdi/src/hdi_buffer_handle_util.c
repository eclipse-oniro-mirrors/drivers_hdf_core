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

#include "hdi_buffer_handle_util.h"
#include <unistd.h>
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"

BufferHandle *AllocateBufferHandle(uint32_t reserveFds, uint32_t reserveInts)
{
    size_t handleSize = sizeof(BufferHandle) + (sizeof(int32_t) * (reserveFds + reserveInts));
    BufferHandle *handle = (BufferHandle *)(OsalMemCalloc(handleSize));
    if (handle != NULL) {
        handle->fd = -1;
        handle->reserveFds = reserveFds;
        handle->reserveInts = reserveInts;
        for (uint32_t i = 0; i < reserveFds; i++) {
            handle->reserve[i] = -1;
        }
    } else {
        HDF_LOGE("BufferHandle malloc %zu failed", handleSize);
    }
    return handle;
}

BufferHandle *CloneBufferHandle(const BufferHandle *other)
{
    if (other == NULL) {
        HDF_LOGW("%{public}s handle is NULL", __func__);
        return NULL;
    }

    BufferHandle *handle = AllocateBufferHandle(other->reserveFds, other->reserveInts);
    if (handle == NULL) {
        HDF_LOGW("%{public}s AllocateBufferHandle failed, handle is NULL", __func__);
        return NULL;
    }

    if (other->fd == -1) {
        handle->fd = other->fd;
    } else {
        handle->fd = dup(other->fd);
        if (handle->fd == -1) {
            HDF_LOGE("CloneBufferHandle dup failed");
            FreeBufferHandle(handle);
            return NULL;
        }
    }
    handle->width = other->width;
    handle->stride = other->stride;
    handle->height = other->height;
    handle->size = other->size;
    handle->format = other->format;
    handle->usage = other->usage;
    handle->phyAddr = other->phyAddr;
    handle->key = other->key;

    for (uint32_t i = 0; i < handle->reserveFds; i++) {
        handle->reserve[i] = dup(other->reserve[i]);
        if (handle->reserve[i] == -1) {
            HDF_LOGE("CloneBufferHandle dup reserveFds failed");
            FreeBufferHandle(handle);
            return NULL;
        }
    }

    if (other->reserveInts == 0) {
        return handle;
    }

    if (memcpy_s(&handle->reserve[handle->reserveFds], sizeof(int32_t) * handle->reserveInts,
            &other->reserve[other->reserveFds], sizeof(int32_t) * other->reserveInts) != EOK) {
        HDF_LOGE("CloneBufferHandle memcpy_s failed");
        FreeBufferHandle(handle);
        return NULL;
    }
    return handle;
}

void FreeBufferHandle(BufferHandle *handle)
{
    if (handle->fd != -1) {
        close(handle->fd);
    }
    for (uint32_t i = 0; i < handle->reserveFds; i++) {
        if (handle->reserve[i] != -1) {
            close(handle->reserve[i]);
        }
    }
    OsalMemFree(handle);
}

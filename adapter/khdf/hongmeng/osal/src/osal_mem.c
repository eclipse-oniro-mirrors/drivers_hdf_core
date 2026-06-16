/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "osal_mem.h"
#include "hdf_core_log.h"
#include <udk/iolib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HDF_LOG_TAG osal_mem

struct MemHdr {
    uint32_t type;
    uint32_t offset;
};

struct MemBlock {
    struct MemHdr hdr;
    uint8_t mem[0];
};

#define TYPE_MALLOC 0xa5a5a5a1

static void *MemAlloc(size_t size, uint32_t *type)
{
    char *base = NULL;
    const uint32_t mngSize = sizeof(struct MemBlock);

    if (size > (SIZE_MAX - mngSize)) {
        HDF_LOGE("%s invalid param %zu", __func__, size);
        return NULL;
    }

    base = (char *)udk_malloc(size + mngSize);
    if (base == NULL) {
        HDF_LOGE("%s malloc fail %zu", __func__, size);
        return NULL;
    }
    *type = TYPE_MALLOC;

    return base;
}

void *OsalMemAlloc(size_t size)
{
    void *buf = NULL;
    char *base = NULL;
    struct MemBlock *block = NULL;
    const uint32_t mngSize = sizeof(*block);
    uint32_t type;

    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }

    base = MemAlloc(size, &type);
    if (base == NULL) {
        HDF_LOGE("%s malloc fail %zu", __func__, size);
        return NULL;
    }

    block = (struct MemBlock *)base;
    block->hdr.type = type;
    block->hdr.offset = 0U;

    buf = (void *)(base + mngSize);
    return buf;
}

void *OsalMemCalloc(size_t size)
{
    int ret;
    void *buf = NULL;

    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }

    buf = OsalMemAlloc(size);
    if (buf == NULL) {
        return NULL;
    }

    ret = memset_s(buf, size, 0, size);
    if (ret != 0) {
        HDF_LOGE("memset buf fail, ret=%d", ret);
        OsalMemFree(buf);
        return NULL;
    }

    return buf;
}

void *OsalMemAllocAlign(size_t alignment, size_t size)
{
    char *base = NULL;
    char *buf = NULL;
    struct MemBlock *block = NULL;
    const uint32_t mngSize = sizeof(*block);
    uint32_t type;
    uint32_t offset;
    char *alignedPtr = NULL;

    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0 ||
        (alignment % sizeof(void *)) != 0 || size > (SIZE_MAX - alignment)) {
        HDF_LOGE("%s invalid param align %zu size %zu", __func__, alignment, size);
        return NULL;
    }

    base = (char *)MemAlloc(size + alignment, &type);
    if (base == NULL) {
        HDF_LOGE("%s malloc fail align %zu, size %zu", __func__, alignment, size);
        return NULL;
    }

    buf = base + mngSize;
    alignedPtr = (char *)(((uintptr_t)(buf + alignment - 1)) & ~((uintptr_t)(alignment - 1)));
    offset = (uint32_t)(alignedPtr - buf);
    block = (struct MemBlock *)(base + offset);
    block->hdr.type = type;
    block->hdr.offset = offset;

    return alignedPtr;
}

void OsalMemFree(void *mem)
{
    uint32_t type;
    char *base = NULL;
    struct MemBlock *block = NULL;

    if (mem == NULL) {
        return;
    }

    block = (struct MemBlock *)((char *)mem - (char *)&((struct MemBlock *)0)->mem);
    type = block->hdr.type;
    base = (char *)block - block->hdr.offset;

    if (type == TYPE_MALLOC) {
        udk_free(base);
    } else {
        HDF_LOGE("%s block type %u fail", __func__, type);
    }
}
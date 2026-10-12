/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of the following conditions and the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "osal_mem.h"
#include "hdf_core_log.h"
#include "osal_extend.h"
#include "securec.h"

#define HDF_LOG_TAG osal_mem

void *OsalMemAlloc(size_t size)
{
    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }
    return OsalExtendMemAlloc(size);
}

void *OsalMemCalloc(size_t size)
{
    void *buf = NULL;
    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }
    buf = OsalMemAlloc(size);
    if (buf != NULL) {
        (void)memset_s(buf, size, 0, size);
    }
    return buf;
}

void *OsalMemAllocAlign(size_t alignment, size_t size)
{
    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }
    /* D4: extend layer returns NULL if it has no aligned allocator.
     * No generic fallback here to keep free() semantics unambiguous.
     *
     * A fallback could allocate size + alignment + header bytes, store the
     * raw pointer just below the returned aligned address, and recover it at
     * free() time. But that would make OsalMemFree() unable to distinguish a
     * extend-direct pointer from a fallback pointer, since the header slot
     * preceding a extend-direct pointer may not hold a valid raw pointer.
     * To avoid this ambiguity, aligned allocation is delegated entirely to
     * the extend; callers must handle NULL if the extend lacks support. */
    return OsalExtendMemAllocAlign(alignment, size);
}

void OsalMemFree(void *mem)
{
    if (mem != NULL) {
        OsalExtendMemFree(mem);
    }
}

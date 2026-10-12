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

#include "osal_extend.h"
#include "hdf_core_log.h"
#include "los_interrupt.h"
#include "los_memory.h"
#include "los_task.h"

#define HDF_LOG_TAG osal_extend

void *OsalExtendMemAlloc(size_t size)
{
    return LOS_MemAlloc((void *)OS_SYS_MEM_ADDR, size);
}

void OsalExtendMemFree(void *mem)
{
    if (mem != NULL) {
        (void)LOS_MemFree((void *)OS_SYS_MEM_ADDR, mem);
    }
}

void *OsalExtendMemAllocAlign(size_t alignment, size_t size)
{
    return LOS_MemAllocAlign((void *)OS_SYS_MEM_ADDR, size, alignment);
}

uint32_t OsalExtendIrqSave(void)
{
    return LOS_IntLock();
}

void OsalExtendIrqRestore(uint32_t flags)
{
    LOS_IntRestore(flags);
}

void OsalExtendUSleep(uint32_t us)
{
    /* LOS_UDelay is busy-wait; for us-level sleep use LOS_UDelay(us). */
    LOS_UDelay(us);
}

void OsalExtendUDelay(uint32_t us)
{
    LOS_UDelay(us);
}

void OsalExtendMDelay(uint32_t ms)
{
    LOS_MDelay(ms);
}

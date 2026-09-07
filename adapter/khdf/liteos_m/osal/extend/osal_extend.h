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

#ifndef OSAL_EXTEND_H
#define OSAL_EXTEND_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Extend extension interface contract for CMSIS-RTOS2 OSAL.
 *
 * CMSIS-RTOS2 does not cover: general malloc, spinlock IRQ control,
 * atomic ops, sub-tick microsecond delay, char device. These are
 * implemented by the target RTOS/BSP via the OsalExtendXxx hooks
 * below. A reference implementation for liteos_m is provided in
 * osal/extend/liteos_m/osal_extend_impl.c.
 *
 * These are STRONG declarations: each target kernel MUST supply an
 * osal_extend_impl.c that defines all eight symbols (see the liteos_m
 * reference). There are no default weak stubs — a kernel that fails to
 * provide an implementation gets a link error rather than silent NULL/0
 * returns, since NULL memory or no-op IRQ disable would be unsafe in
 * production. osal/cmsis/BUILD.gn compiles the liteos_m implementation
 * only when ohos_kernel_type == "liteos_m"; other kernels add their own
 * extend/<kernel>/osal_extend_impl.c and a matching BUILD.gn branch.
 */

/* ---- Dynamic memory (CMSIS has only fixed-block osMemoryPool*) ---- */

/**
 * @brief Allocates size bytes from the extend heap.
 * @param size Bytes to allocate.
 * @return Pointer on success, NULL on failure.
 */
void *OsalExtendMemAlloc(size_t size);

/**
 * @brief Frees memory previously allocated by OsalExtendMemAlloc.
 * @param mem Pointer to free; NULL is a no-op.
 */
void OsalExtendMemFree(void *mem);

/**
 * @brief Allocates aligned memory. Per D4 there is NO fallback: if the
 *        extend has no aligned allocator, return NULL and the caller
 *        (osal_mem.c OsalMemAllocAlign) propagates the failure.
 * @param alignment Boundary, must be power of 2.
 * @param size Bytes to allocate.
 * @return Aligned pointer on success, NULL if unsupported/failed.
 */
void *OsalExtendMemAllocAlign(size_t alignment, size_t size);

/* ---- IRQ control (CMSIS has no interrupt disable API) ---- */

/**
 * @brief Disables interrupts and returns the saved IRQ state.
 * @return Saved interrupt state to pass to OsalExtendIrqRestore.
 */
uint32_t OsalExtendIrqSave(void);

/**
 * @brief Restores interrupt state saved by OsalExtendIrqSave.
 * @param flags Saved state returned by OsalExtendIrqSave.
 */
void OsalExtendIrqRestore(uint32_t flags);

/* ---- Sub-tick microsecond delay (osDelay granularity is a tick) ---- */

/**
 * @brief Sleeps us microseconds (may yield CPU).
 */
void OsalExtendUSleep(uint32_t us);

/**
 * @brief Busy-waits us microseconds (does NOT yield).
 */
void OsalExtendUDelay(uint32_t us);

/**
 * @brief Busy-waits ms milliseconds (does NOT yield).
 */
void OsalExtendMDelay(uint32_t ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OSAL_EXTEND_H */

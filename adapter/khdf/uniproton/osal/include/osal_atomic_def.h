/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
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

#ifndef OSAL_ATOMIC_DEF_H
#define OSAL_ATOMIC_DEF_H

#include "prt_hwi.h"
#include "hdf_base.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline int32_t OsalAtomicReadWrapperInline(volatile int32_t *counter)
{
    uint32_t intSave = PRT_IntLock();
    int32_t val = *counter;
    PRT_IntRestore(intSave);
    return val;
}

static inline void OsalAtomicSetWrapperInline(volatile int32_t *counter, int32_t value)
{
    uint32_t intSave = PRT_IntLock();
    *counter = value;
    PRT_IntRestore(intSave);
}

static inline void OsalAtomicIncWrapperInline(volatile int32_t *counter)
{
    uint32_t intSave = PRT_IntLock();
    (*counter)++;
    PRT_IntRestore(intSave);
}

static inline int32_t OsalAtomicIncRetWrapperInline(volatile int32_t *counter)
{
    uint32_t intSave = PRT_IntLock();
    int32_t val = ++(*counter);
    PRT_IntRestore(intSave);
    return val;
}

static inline void OsalAtomicDecWrapperInline(volatile int32_t *counter)
{
    uint32_t intSave = PRT_IntLock();
    (*counter)--;
    PRT_IntRestore(intSave);
}

static inline int32_t OsalAtomicDecRetWrapperInline(volatile int32_t *counter)
{
    uint32_t intSave = PRT_IntLock();
    int32_t val = --(*counter);
    PRT_IntRestore(intSave);
    return val;
}

#define OsalAtomicReadWrapper(v)        OsalAtomicReadWrapperInline(&(v)->counter)
#define OsalAtomicSetWrapper(v, value)  OsalAtomicSetWrapperInline(&(v)->counter, (value))
#define OsalAtomicIncWrapper(v)         OsalAtomicIncWrapperInline(&(v)->counter)
#define OsalAtomicIncRetWrapper(v)      OsalAtomicIncRetWrapperInline(&(v)->counter)
#define OsalAtomicDecWrapper(v)         OsalAtomicDecWrapperInline(&(v)->counter)
#define OsalAtomicDecRetWrapper(v)      OsalAtomicDecRetWrapperInline(&(v)->counter)

#define OSAL_BITS_PER_LONG 32
#define OSAL_BIT_MASK(nr) (1UL << ((nr) % OSAL_BITS_PER_LONG))
#define OSAL_BIT_WORD(nr) ((nr) / OSAL_BITS_PER_LONG)
#define OSAL_BITOP_WORD(nr) ((nr) / OSAL_BITS_PER_LONG)

static inline int32_t OsalTestBitWrapper(unsigned long nr, const volatile unsigned long *addr)
{
    return (1UL & (addr[OSAL_BIT_WORD(nr)] >> (nr & (OSAL_BITS_PER_LONG - 1))));
}

static inline int32_t OsalTestSetBitWrapper(unsigned long nr, volatile unsigned long *addr)
{
    uint32_t intSave = PRT_IntLock();

    const unsigned long mask = OSAL_BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + OSAL_BIT_WORD(nr);
    unsigned long old = *p;
    *p = old | mask;

    PRT_IntRestore(intSave);
    return ((old & mask) != 0);
}

static inline int32_t OsalTestClearBitWrapper(unsigned long nr, volatile unsigned long *addr)
{
    uint32_t intSave = PRT_IntLock();

    const unsigned long mask = OSAL_BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + OSAL_BIT_WORD(nr);
    unsigned long old = *p;
    *p = old & ~mask;

    PRT_IntRestore(intSave);
    return ((old & mask) != 0);
}

static inline void OsalClearBitWrapper(unsigned long nr, volatile unsigned long *addr)
{
    uint32_t intSave = PRT_IntLock();

    const unsigned long mask = OSAL_BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + OSAL_BIT_WORD(nr);
    *p &= ~mask;

    PRT_IntRestore(intSave);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OSAL_ATOMIC_DEF_H */

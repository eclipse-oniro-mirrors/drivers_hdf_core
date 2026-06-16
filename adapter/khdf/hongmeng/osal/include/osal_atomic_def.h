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

#ifndef OSAL_ATOMIC_DEF_H
#define OSAL_ATOMIC_DEF_H

#include "hdf_base.h"
#include <udk/bitops.h>
#include <libhmsync/atomic.h>

#define OsalAtomicReadWrapper(v)          raw_atomic_int_read((const raw_atomic_int_t *)(v))
#define OsalAtomicSetWrapper(v, value)    raw_atomic_int_set((raw_atomic_int_t *)(v), value)
#define OsalAtomicIncWrapper(v)           raw_atomic_int_inc((raw_atomic_int_t *)(v))
#define OsalAtomicIncRetWrapper(v)        raw_atomic_int_inc((raw_atomic_int_t *)(v))
#define OsalAtomicDecWrapper(v)           raw_atomic_int_dec((raw_atomic_int_t *)(v))
#define OsalAtomicDecRetWrapper(v)        raw_atomic_int_dec((raw_atomic_int_t *)(v))

#define OsalTestBitWrapper(nr, addr)      udk_bit_test(addr, nr)
#define OsalTestSetBitWrapper(nr, addr)   udk_test_and_set_bit(addr, nr)
#define OsalTestClearBitWrapper(nr, addr) udk_test_and_clear_bit(addr, nr)
#define OsalClearBitWrapper(nr, addr)     udk_bit_clear(addr, nr)

#endif /* OSAL_ATOMIC_DEF_H */
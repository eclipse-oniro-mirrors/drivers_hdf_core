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

#ifndef OSAL_UACCESS_DEF_H
#define OSAL_UACCESS_DEF_H

#include <stddef.h>
#include <udk/mm.h>

static inline size_t CopyToUser(void *to, const void *from, size_t len)
{
    return (size_t)udk_copy_to_user(to, from, (unsigned long)len);
}

static inline size_t CopyFromUser(void *to, const void *from, size_t len)
{
    return (size_t)udk_copy_from_user(to, from, (unsigned long)len);
}

#endif /* OSAL_UACCESS_DEF_H */
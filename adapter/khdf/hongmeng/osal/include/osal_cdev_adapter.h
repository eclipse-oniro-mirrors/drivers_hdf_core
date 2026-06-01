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

#ifndef OSAL_CDEV_ADAPTER_DEF_H
#define OSAL_CDEV_ADAPTER_DEF_H

#include "hdf_types.h"
#include <stddef.h>
#include <stdint.h>
#include <poll.h>
#include <udk/char.h>
#include <udk/poll.h>

typedef struct udk_poll_wqueue wait_queue_head_t;

typedef struct udk_poll_wevent poll_table;

struct file {
    void *private_data;
};

static inline void poll_wait(struct file *filp, wait_queue_head_t *queue, poll_table *p)
{
    udk_poll_wqueue_add(queue, p);
}

#define wake_up_interruptible(q) \
do { \
    udk_poll_wqueue_wakeup(q, 0U); \
} while (0)

#define init_waitqueue_head(q) \
do { \
    udk_poll_wqueue_init(q); \
} while (0)

#endif /* OSAL_CDEV_ADAPTER_DEF_H */
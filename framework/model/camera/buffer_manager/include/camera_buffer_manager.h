/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_BUFFER_MANAGER_H
#define CAMERA_BUFFER_MANAGER_H

#include <osal/osal_atomic.h>
#include <osal/osal_spinlock.h>
#include <osal/osal_mutex.h>
#include <camera/camera_product.h>
#include "hdf_dlist.h"
#include "camera_buffer_manager_adapter.h"

struct CameraQueueOps {
    int32_t (*queueSetup)(struct BufferQueue *queue, uint32_t *bufferCount, uint32_t *planeCount, uint32_t sizes[]);
    int32_t (*prepareBuffer)(struct CameraBuffer *buffer);
    void (*queueBuffer)(struct CameraBuffer *buffer);
    int32_t (*startStreaming)(struct BufferQueue *queue);
    void (*stopStreaming)(struct BufferQueue *queue);
};

enum BufferState {
    BUFFER_STATE_DEQUEUED,
    BUFFER_STATE_PREPARING,
    BUFFER_STATE_QUEUED,
    BUFFER_STATE_ACTIVE,
    BUFFER_STATE_DONE,
    BUFFER_STATE_ERROR,
};

struct CameraBuffer {
    struct BufferQueue *bufferQueue;
    uint32_t id;
    uint32_t memType;
    uint64_t timeStamp;
    enum BufferState state;
    uint32_t flags;
    uint32_t numPlanes;
    struct CameraPlane planes[MAX_PLANES];
    struct DListHead queueEntry;
    struct DListHead doneEntry;
    uint32_t field;
    uint32_t sequence;
};

/* BufferQueue flags */
enum QueueState {
    QUEUE_STATE_STREAMING = (1 << 0),               /**< set bit: queue is streaming */
    QUEUE_STATE_WAITING_DEQUEUE = (1 << 1),         /**< set bit: queue is waiting buffer dequeue */
    QUEUE_STATE_STREAMING_CALLED = (1 << 2),        /**< set bit: start streaming is called */
    QUEUE_STATE_ERROR = (1 << 3),                   /**< set bit: error happened */
    QUEUE_STATE_WAITING_BUFFERS = (1 << 4),         /**< set bit: queue is waiting for buffers */
    QUEUE_STATE_LAST_BUFFER_DEQUEUED = (1 << 5),    /**< set bit: last buffer has been dequeued */
    QUEUE_STATE_ALLOW_CACHE_HINTS = (1 << 6),       /**< set bit:  queue allow cache hints*/
    QUEUE_STATE_BIDIRECTIONAL = (1 << 7),           /**< set bit:  queue is bidirectional*/
};

/* CameraBuffer flags */
enum BufferFlags {
    BUFFER_SYNCED = (1 << 0),
    BUFFER_NEED_FINISH_SYNC = (1 << 1),
    BUFFER_NEED_PREPARE_SYNC = (1 << 2),
    BUFFER_PREPARED = (1 << 3),
};

#endif  // CAMERA_BUFFER_MANAGER_H
/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <utils/hdf_log.h>
#include <osal/osal_mem.h>
#include "osal_uaccess.h"
#include "camera_buffer_manager.h"

#define HDF_LOG_TAG HDF_CAMERA_BUFFER_MANAGER

static void CameraBufferFinishMem(struct CameraBuffer *buffer)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(buffer->bufferQueue, struct CameraQueueImp, queue);
    uint32_t plane;

    if ((buffer->flags & BUFFER_SYNCED) == 0) {
        return;
    }

    if ((buffer->flags & BUFFER_NEED_FINISH_SYNC) != 0) {
        for (plane = 0; plane < buffer->numPlanes; ++plane) {
            if (queueImp->memOps->finishMem != NULL) {
                queueImp->memOps->finishMem(buffer->planes[plane].memPriv);
            }
        }
    }
    buffer->flags &= ~BUFFER_SYNCED;
}

static void CameraQueueClose(struct BufferQueue *queue)
{
    uint32_t i;

    if ((queue->flags & QUEUE_STATE_STREAMING_CALLED) != 0) {
        if ((queue->queueOps->stopStreaming) != NULL) {
            queue->queueOps->stopStreaming(queue);
        }
    }

    if (OsalAtomicRead(&queue->driverOwnCount) != 0) {
        for (i = 0; i < queue->numBuffers; ++i) {
            if (queue->buffers[i]->state == BUFFER_STATE_ACTIVE) {
                CameraBufferDone(queue->buffers[i], BUFFER_STATE_ERROR);
            }
        }
        HDF_LOGI("%s: Expect zero, driverOwnCount = %{public}d", __func__, OsalAtomicRead(&queue->driverOwnCount));
    }

    queue->flags &= ~QUEUE_STATE_STREAMING;
    queue->flags &= ~QUEUE_STATE_STREAMING_CALLED;
    queue->queuedCount = 0;
    queue->flags &= ~QUEUE_STATE_ERROR;

    DListHeadInit(&queue->queuedList);
    DListHeadInit(&queue->doneList);
    OsalAtomicSet(&queue->driverOwnCount, 0);
    wake_up_interruptible(&queue->doneWait);

    for (i = 0; i < queue->numBuffers; ++i) {
        struct CameraBuffer *buffer = queue->buffers[i];
        CameraBufferFinishMem(buffer);
        buffer->flags &= ~BUFFER_PREPARED;
        buffer->state = BUFFER_STATE_DEQUEUED;
    }
}

static int32_t CameraQueueCheckMmapOps(struct BufferQueue *queue)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(queue, struct CameraQueueImp, queue);

    if (((queue->ioModes & MEMTYPE_MMAP) == 0) || (queueImp->memOps->alloc == NULL) ||
        (queueImp->memOps->free == NULL) || (queueImp->memOps->mmap == NULL)) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraQueueCheckUserPtrOps(struct BufferQueue *queue)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(queue, struct CameraQueueImp, queue);

    if (((queue->ioModes & MEMTYPE_USERPTR) == 0) || (queueImp->memOps->getUserPtr == NULL) ||
        (queueImp->memOps->freeUserPtr == NULL)) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraQueueCheckDmaBufOps(struct BufferQueue *queue)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(queue, struct CameraQueueImp, queue);

    if (((queue->ioModes & MEMTYPE_DMABUF) == 0) || (queueImp->memOps->attachDmaBuf == NULL) ||
        (queueImp->memOps->detachDmaBuf == NULL) || (queueImp->memOps->mapDmaBuf == NULL) ||
        (queueImp->memOps->unmapDmaBuf == NULL)) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraQueueCheckMemOps(struct BufferQueue *queue, enum CameraMemType memType)
{
    if (memType != MEMTYPE_MMAP && memType != MEMTYPE_USERPTR && memType != MEMTYPE_DMABUF) {
        HDF_LOGE("%s: memType is fault!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (memType == MEMTYPE_MMAP && CameraQueueCheckMmapOps(queue) != HDF_SUCCESS) {
        HDF_LOGE("%s: MMAP for current setup unsupported!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (memType == MEMTYPE_USERPTR && CameraQueueCheckUserPtrOps(queue) != HDF_SUCCESS) {
        HDF_LOGE("%s: USERPTR for current setup unsupported!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (memType == MEMTYPE_DMABUF && CameraQueueCheckDmaBufOps(queue) != HDF_SUCCESS) {
        HDF_LOGE("%s: DMABUF for current setup unsupported!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

static void CameraBufferFreeMmap(struct CameraBuffer *buffer)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(buffer->bufferQueue, struct CameraQueueImp, queue);
    uint32_t plane;

    for (plane = 0; plane < buffer->numPlanes; ++plane) {
        if ((queueImp->memOps->free != NULL) && (buffer->planes[plane].memPriv != NULL)) {
            queueImp->memOps->free(buffer->planes[plane].memPriv);
            buffer->planes[plane].memPriv = NULL;
        }
    }
}

static void CameraBufferFreeUserPtr(struct CameraBuffer *buffer)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(buffer->bufferQueue, struct CameraQueueImp, queue);
    uint32_t plane;

    for (plane = 0; plane < buffer->numPlanes; ++plane) {
        if ((queueImp->memOps->freeUserPtr != NULL) && (buffer->planes[plane].memPriv != NULL)) {
            queueImp->memOps->freeUserPtr(buffer->planes[plane].memPriv);
            buffer->planes[plane].memPriv = NULL;
        }
    }
}

static void CameraBufferFreeDmaPlane(struct CameraBuffer *buffer, struct CameraPlane *plane)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(buffer->bufferQueue, struct CameraQueueImp, queue);

    if (plane->dmaMapped != 0) {
        if (queueImp->memOps->unmapDmaBuf != NULL) {
            queueImp->memOps->unmapDmaBuf(plane->memPriv);
        }
    }
    if (queueImp->memOps->detachDmaBuf != NULL) {
        queueImp->memOps->detachDmaBuf(plane->memPriv);
    }

    MemoryAdapterFreeDmaBuf(plane->dmaBuf);
    plane->memPriv = NULL;
    plane->dmaBuf = NULL;
    plane->dmaMapped = 0;
}

static void CameraBuffeFreeDma(struct CameraBuffer *buffer)
{
    uint32_t planeId;

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        CameraBufferFreeDmaPlane(buffer, &buffer->planes[planeId]);
    }
}

static int32_t CameraQueueAttachDmaBuf(struct BufferQueue *queue,
    struct CameraBuffer *buffer, struct CameraPlane planes[])
{
    uint32_t planeNum;
    void *memPriv;
    int32_t ret;
    struct CameraQueueImp *queueImp = CONTAINER_OF(queue, struct CameraQueueImp, queue);

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        void *dmaBuf = MemoryAdapterDmaBufGet(planes[planeNum].memory.fd);
        if (MemoryAdapterIsErrOrNullPtr(dmaBuf)) {
            CameraBuffeFreeDma(buffer);
            HDF_LOGE("%s: dmaBuf ptr error!", __func__);
            return HDF_FAILURE;
        }
        if (planes[planeNum].length == 0) {
            planes[planeNum].length = MemoryAdapterDmaBufSize(dmaBuf);
        }
        if (planes[planeNum].length < buffer->planes[planeNum].minLength) {
            MemoryAdapterFreeDmaBuf(dmaBuf);
            CameraBuffeFreeDma(buffer);
            HDF_LOGE("%s: not enough dmabuf length!", __func__);
            return HDF_FAILURE;
        }
        if (dmaBuf == buffer->planes[planeNum].dmaBuf && buffer->planes[planeNum].length == planes[planeNum].length) {
            MemoryAdapterFreeDmaBuf(dmaBuf);
            continue;
        }
        CameraBufferFreeDmaPlane(buffer, &buffer->planes[planeNum]);
        buffer->planes[planeNum].bytesUsed = 0;
        buffer->planes[planeNum].length = 0;
        buffer->planes[planeNum].memory.fd = 0;
        buffer->planes[planeNum].dataOffset = 0;
        if (queueImp->memOps->attachDmaBuf != NULL) {
            memPriv = MemoryAdapterAttachDmaBuf(queueImp, dmaBuf, planes[planeNum].length, planeNum);
        }
        if (MemoryAdapterIsErrPtr(memPriv)) {
            ret = MemoryAdapterPtrErr(memPriv);
            MemoryAdapterFreeDmaBuf(dmaBuf);
            CameraBuffeFreeDma(buffer);
            return ret;
        }
        buffer->planes[planeNum].dmaBuf = dmaBuf;
        buffer->planes[planeNum].memPriv = memPriv;
    }

    return HDF_SUCCESS;
}

static int32_t CameraBufferMapDmaBuf(struct CameraBuffer *buffer)
{
    int32_t planeNum;
    int32_t ret;
    struct CameraQueueImp *queueImp = CONTAINER_OF(buffer->bufferQueue, struct CameraQueueImp, queue);

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        if (buffer->planes[planeNum].dmaMapped != 0) {
            continue;
        }
        if (queueImp->memOps->mapDmaBuf == NULL) {
            HDF_LOGE("%s: mapDmaBuf function is NULL!", __func__);
            return HDF_FAILURE;
        }
        ret = queueImp->memOps->mapDmaBuf(buffer->planes[planeNum].memPriv);
        if (ret != HDF_SUCCESS) {
            CameraBuffeFreeDma(buffer);
            HDF_LOGE("%s: mapDmaBuf failed!", __func__);
            return ret;
        }
        buffer->planes[planeNum].dmaMapped = 1;
    }

    return HDF_SUCCESS;
}

static void CameraBufferFree(struct BufferQueue *queue, struct CameraBuffer *buffer)
{
    if (queue->memType == MEMTYPE_MMAP) {
        CameraBufferFreeMmap(buffer);
    } else if (queue->memType == MEMTYPE_DMABUF) {
        CameraBuffeFreeDma(buffer);
    } else {
        CameraBufferFreeUserPtr(buffer);
    }
}

static int32_t CameraQueueFree(struct BufferQueue *queue, uint32_t count)
{
    uint32_t i;
    struct CameraBuffer *buffer = NULL;

    if (queue->numBuffers < count) {
        HDF_LOGE("%s: count out of range!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    for (i = queue->numBuffers - count; i < queue->numBuffers; ++i) {
        buffer = queue->buffers[i];
        if (buffer == NULL) {
            continue;
        }
        if (buffer->state == BUFFER_STATE_PREPARING) {
            HDF_LOGE("%s: Can free buffer. Buffer is preparing!", __func__);
            return HDF_FAILURE;
        }
        CameraBufferFree(queue, buffer);
        OsalMemFree(buffer);
        queue->buffers[i] = NULL;
    }

    queue->numBuffers -= count;
    if (queue->numBuffers == 0) {
        queue->memType = MEMTYPE_UNKNOWN;
        DListHeadInit(&queue->queuedList);
    }
    return HDF_SUCCESS;
}

static int32_t CameraQueueRelease(struct BufferQueue *queue, struct UserCameraReq *userReq)
{
    int32_t ret;

    ret = CameraQueueCheckMemOps(queue, userReq->memType);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraQueueCheckMemOps failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    userReq->capabilities = queue->ioModes;
    if ((queue->flags & QUEUE_STATE_STREAMING) != 0) {
        HDF_LOGE("%s: Queue is streaming", __func__);
        return HDF_FAILURE;
    }
    if (((queue->flags & QUEUE_STATE_WAITING_DEQUEUE) != 0) && userReq->count != 0) {
        HDF_LOGE("%s: Queue is waiting dequeue", __func__);
        return HDF_FAILURE;
    }

    if (userReq->count == 0 || queue->numBuffers != 0 ||
        (queue->memType != MEMTYPE_UNKNOWN && queue->memType != userReq->memType)) {
        OsalMutexLock(&queue->mmapLock);
        CameraQueueClose(queue);
        ret = CameraQueueFree(queue, queue->numBuffers);
        OsalMutexUnlock(&queue->mmapLock);
    }
    return ret;
}

static int32_t CameraBufferAllocMmap(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    struct CameraQueueImp *queueImp = CONTAINER_OF(queue, struct CameraQueueImp, queue);
    void *memPriv = NULL;
    int32_t plane;
    int32_t ret = HDF_FAILURE;

    for (plane = 0; plane < buffer->numPlanes; ++plane) {
        unsigned long size = MemoryAdapterPageAlign(buffer->planes[plane].length);
        if (size < buffer->planes[plane].length) {
            goto FREE;
        }
        
        memPriv = MemoryAdapterAlloc(queueImp, plane, size);
        if (MemoryAdapterIsErrOrNullPtr(memPriv)) {
            if (memPriv != NULL) {
                ret = MemoryAdapterPtrErr(memPriv);
            }
            goto FREE;
        }
        buffer->planes[plane].memPriv = memPriv;
    }
    return HDF_SUCCESS;

FREE:
    for (; plane > 0; --plane) {
        if ((queueImp->memOps->free != NULL) && (buffer->planes[plane - 1].memPriv != NULL)) {
            queueImp->memOps->free(buffer->planes[plane - 1].memPriv);
            buffer->planes[plane - 1].memPriv = NULL;
        }
    }
    return ret;
}

static void CameraBufferSetupOffsets(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    uint32_t planeId;
    unsigned long off;

    if (buffer->id != 0) {
        struct CameraBuffer *prev = queue->buffers[buffer->id - 1];
        struct CameraPlane *p = &prev->planes[prev->numPlanes - 1];
        off = MemoryAdapterPageAlign(p->memory.offset + p->length);
    }

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        buffer->planes[planeId].memory.offset = off;
        off += buffer->planes[planeId].length;
        off = MemoryAdapterPageAlign(off);
    }
}

static int32_t CameraQueueAlloc(struct BufferQueue *queue, uint32_t memType,
    uint32_t numBuffers, uint32_t numPlanes, const uint32_t planeSizes[])
{
    uint32_t bufferId;
    uint32_t planeId;
    struct CameraBuffer *buffer = NULL;
    int32_t ret;

    numBuffers = numBuffers < (MAX_FRAME - queue->numBuffers) ? numBuffers : (MAX_FRAME - queue->numBuffers);
    for (bufferId = 0; bufferId < numBuffers; ++bufferId) {
        buffer = OsalMemCalloc(queue->bufferSize);
        if (buffer == NULL) {
            HDF_LOGE("%s: Memory alloc failed", __func__);
            break;
        }
        buffer->state = BUFFER_STATE_DEQUEUED;
        buffer->bufferQueue = queue;
        buffer->numPlanes = numPlanes;
        buffer->id = queue->numBuffers + bufferId;
        buffer->memType = memType;
        if (queue->memType != MEMTYPE_DMABUF) {
            buffer->flags |= BUFFER_NEED_FINISH_SYNC;
            buffer->flags |= BUFFER_NEED_PREPARE_SYNC;
        }
        for (planeId = 0; planeId < numPlanes; ++planeId) {
            buffer->planes[planeId].length = planeSizes[planeId];
            buffer->planes[planeId].minLength = planeSizes[planeId];
        }
        queue->buffers[buffer->id] = buffer;
        if (memType == MEMTYPE_MMAP) {
            ret = CameraBufferAllocMmap(buffer);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%s: Mmap alloc failed", __func__);
                queue->buffers[buffer->id] = NULL;
                OsalMemFree(buffer);
                break;
            }
            CameraBufferSetupOffsets(buffer);
        }
    }
    HDF_LOGI("%s: allocated %{public}d buffers and %{public}d planes", __func__, bufferId, planeId);

    return bufferId;
}

static int32_t CameraQueueRequestBufs(struct BufferQueue *queue, struct UserCameraReq *userReq,
    uint32_t numBuffers, uint32_t numPlanes, uint32_t planeSizes[])
{
    int32_t ret;
    uint32_t allocatedBuffers;

    allocatedBuffers = CameraQueueAlloc(queue, userReq->memType, numBuffers, numPlanes, planeSizes);
    ret = allocatedBuffers < queue->minBuffersNeeded ? HDF_FAILURE : HDF_SUCCESS;
    if (allocatedBuffers == 0) {
        HDF_LOGE("%s: allocatedBuffers wrong", __func__);
        return HDF_FAILURE;
    }
    if (ret == HDF_SUCCESS && allocatedBuffers < numBuffers) {
        numBuffers = allocatedBuffers;
        numPlanes = 0;
        ret = queue->queueOps->queueSetup ? queue->queueOps->queueSetup(queue, &numBuffers,
            &numPlanes, planeSizes) : HDF_SUCCESS;
        if (ret == HDF_SUCCESS && allocatedBuffers < numBuffers) {
            ret = HDF_FAILURE;
        }
    }

    OsalMutexLock(&queue->mmapLock);
    queue->numBuffers = allocatedBuffers;
    if (ret != HDF_SUCCESS) {
        CameraQueueFree(queue, queue->numBuffers);
        OsalMutexUnlock(&queue->mmapLock);
        return ret;
    }
    OsalMutexUnlock(&queue->mmapLock);
    userReq->count = allocatedBuffers;
    queue->flags |= QUEUE_STATE_WAITING_BUFFERS;
    return HDF_SUCCESS;
}

static int32_t CameraBufferCheckPlanes(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    if (userBuffer->planes == NULL) {
        HDF_LOGE("%s: user buffer's plane is null!", __func__);
        return HDF_FAILURE;
    }
    if (userBuffer->planeCount < buffer->numPlanes || userBuffer->planeCount > MAX_PLANES) {
        HDF_LOGE("%s: incorrect plane count!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void CameraBufferFillUserBuffer(struct CameraBuffer *buffer, struct UserCameraBuffer *userBuffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    uint32_t plane;

    userBuffer->id = buffer->id;
    userBuffer->memType = buffer->memType;
    userBuffer->field = buffer->field;
    userBuffer->timeStamp = buffer->timeStamp;
    userBuffer->sequence = buffer->sequence;
    userBuffer->planeCount = buffer->numPlanes;

    for (plane = 0; plane < buffer->numPlanes; ++plane) {
        struct UserCameraPlane *dst = &userBuffer->planes[plane];
        struct CameraPlane *src = &buffer->planes[plane];
        dst->bytesUsed = src->bytesUsed;
        dst->length = src->length;
        if (queue->memType == MEMTYPE_MMAP) {
            dst->memory.offset = src->memory.offset;
        } else if (queue->memType == MEMTYPE_USERPTR) {
            dst->memory.userPtr = src->memory.userPtr;
        } else if (queue->memType == MEMTYPE_DMABUF) {
            dst->memory.fd = src->memory.fd;
        }
        dst->dataOffset = src->dataOffset;
    }

    switch (buffer->state) {
        case BUFFER_STATE_QUEUED:
        case BUFFER_STATE_ACTIVE:
            userBuffer->flags |= USER_BUFFER_QUEUED;
            break;
        case BUFFER_STATE_ERROR:
            userBuffer->flags |= USER_BUFFER_ERROR;
            break;
        case BUFFER_STATE_DONE:
            userBuffer->flags |= USER_BUFFER_DONE;
            break;
        default:
            break;
    }
}

static void CameraBufferEnqueue(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;

    buffer->state = BUFFER_STATE_ACTIVE;
    OsalAtomicInc(&queue->driverOwnCount);
    if (queue->queueOps->queueBuffer != NULL) {
        queue->queueOps->queueBuffer(buffer);
    }
    return;
}

static int32_t CameraQueueStartStreaming(struct BufferQueue *queue)
{
    struct CameraBuffer *buffer = NULL;
    int32_t ret;
    uint32_t i;

    DLIST_FOR_EACH_ENTRY(buffer, &queue->queuedList, struct CameraBuffer, queueEntry) {
        CameraBufferEnqueue(buffer);
    }
    queue->flags |= QUEUE_STATE_STREAMING_CALLED;
    if (queue->queueOps->startStreaming == NULL) {
        return HDF_FAILURE;
    }

    ret = queue->queueOps->startStreaming(queue);
    if (ret == HDF_SUCCESS) {
        return ret;
    }
    queue->flags &= ~QUEUE_STATE_STREAMING_CALLED;

    if (OsalAtomicRead(&queue->driverOwnCount) != 0) {
        for (i = 0; i < queue->numBuffers; ++i) {
            buffer = queue->buffers[i];
            if (buffer->state == BUFFER_STATE_ACTIVE) {
                CameraBufferDone(buffer, BUFFER_STATE_QUEUED);
            }
        }
        HDF_LOGW("%s: driver count must be zero: %{public}d", __func__, OsalAtomicRead(&queue->driverOwnCount));
    }
    if (!DListIsEmpty(&queue->doneList)) {
        HDF_LOGW("%s: doneList is not empty", __func__);
    }
    return ret;
}

static void CameraBufferSetCacheSync(struct BufferQueue *queue,
    struct CameraBuffer *buffer, struct UserCameraBuffer *userBuffer)
{
    if (queue->memType == MEMTYPE_DMABUF) {
        buffer->flags &= ~BUFFER_NEED_PREPARE_SYNC;
        buffer->flags &= ~BUFFER_NEED_FINISH_SYNC;
        return;
    }
    buffer->flags |= BUFFER_NEED_PREPARE_SYNC;
    buffer->flags |= BUFFER_NEED_FINISH_SYNC;
    return;
}

static int32_t CameraBufferCheckLength(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    uint32_t length;
    uint32_t bytesUsed;
    uint32_t planeId;

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        if (buffer->memType == MEMTYPE_USERPTR || buffer->memType == MEMTYPE_DMABUF) {
            length = userBuffer->planes[planeId].length;
        } else {
            length = buffer->planes[planeId].length;
        }
        bytesUsed = userBuffer->planes[planeId].bytesUsed ? userBuffer->planes[planeId].bytesUsed : length;
        if (userBuffer->planes[planeId].bytesUsed > length) {
            return HDF_ERR_INVALID_PARAM;
        }
        if (userBuffer->planes[planeId].dataOffset > 0 && userBuffer->planes[planeId].dataOffset >= bytesUsed) {
            return HDF_ERR_INVALID_PARAM;
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraQueuePrepare(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer)
{
    struct CameraBuffer *buffer = NULL;
    int32_t ret;

    if (userBuffer->id >= queue->numBuffers) {
        HDF_LOGE("%s: buffer index out of range", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (queue->buffers[userBuffer->id] == NULL) {
        HDF_LOGE("%s: buffer is NULL", __func__);
        return HDF_FAILURE;
    }
    if (userBuffer->memType != queue->memType) {
        HDF_LOGE("%s: invalid memory type: userBuffer memType = %{public}u, queue memType = %{public}u",
            __func__, userBuffer->memType, queue->memType);
        return HDF_FAILURE;
    }

    buffer = queue->buffers[userBuffer->id];
    ret = CameraBufferCheckPlanes(buffer, userBuffer);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    if (buffer->state != BUFFER_STATE_DEQUEUED) {
        HDF_LOGE("%s: buffer is not in dequeued state", __func__);
        return HDF_FAILURE;
    }

    if ((buffer->flags & BUFFER_PREPARED) == 0) {
        CameraBufferSetCacheSync(queue, buffer, userBuffer);
        ret = CameraBufferCheckLength(buffer, userBuffer);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
    }
    return ret;
}

static void CameraBufferPrepareMem(struct CameraBuffer *buffer)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(buffer->bufferQueue, struct CameraQueueImp, queue);
    uint32_t plane;

    if ((buffer->flags & BUFFER_SYNCED) != 0) {
        return;
    }

    if ((buffer->flags & BUFFER_NEED_PREPARE_SYNC) != 0) {
        for (plane = 0; plane < buffer->numPlanes; ++plane) {
            if (queueImp->memOps->prepareMem != NULL) {
                queueImp->memOps->prepareMem(buffer->planes[plane].memPriv);
            }
        }
    }
    buffer->flags |= BUFFER_SYNCED;
}

static int32_t CameraBufferFillBuffer(struct CameraBuffer *buffer,
    struct CameraPlane *planes, const struct UserCameraBuffer *userBuffer)
{
    uint32_t planeId;

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        switch (buffer->memType) {
            case MEMTYPE_USERPTR:
                planes[planeId].memory.userPtr = userBuffer->planes[planeId].memory.userPtr;
                planes[planeId].length = userBuffer->planes[planeId].length;
                break;
            case MEMTYPE_DMABUF:
                planes[planeId].memory.fd = userBuffer->planes[planeId].memory.fd;
                planes[planeId].length = userBuffer->planes[planeId].length;
                break;
            default:
                break;
        }
        planes[planeId].bytesUsed = userBuffer->planes[planeId].bytesUsed;
        planes[planeId].dataOffset = userBuffer->planes[planeId].dataOffset;
    }

    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepareMmap(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    int32_t ret;

    ret = CameraBufferFillBuffer(buffer, buffer->planes, userBuffer);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    if (buffer->bufferQueue->queueOps->prepareBuffer != NULL) {
        ret = buffer->bufferQueue->queueOps->prepareBuffer(buffer);
    }
    return ret;
}

static void CameraBufferReleaseUserPtr(struct CameraBuffer *buffer)
{
    struct CameraQueueImp *queueImp = CONTAINER_OF(buffer->bufferQueue, struct CameraQueueImp, queue);
    uint32_t planeNum;

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        if (buffer->planes[planeNum].memPriv != NULL) {
            if (queueImp->memOps->freeUserPtr != NULL) {
                queueImp->memOps->freeUserPtr(buffer->planes[planeNum].memPriv);
            } else {
                HDF_LOGW("%s: no freeUserPtr function!", __func__);
            }
        }
        buffer->planes[planeNum].memPriv = NULL;
        buffer->planes[planeNum].memory.userPtr = 0;
        buffer->planes[planeNum].length = 0;
    }
}

static int32_t CameraQueuePrepareUserPtrMem(struct BufferQueue *queue,
    struct CameraBuffer *buffer, struct CameraPlane planes[])
{
    uint32_t planeNum;
    void *memPriv;
    int32_t ret;
    struct CameraQueueImp *queueImp = CONTAINER_OF(queue, struct CameraQueueImp, queue);

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        if ((buffer->planes[planeNum].memory.userPtr != 0) &&
            (buffer->planes[planeNum].memory.userPtr == planes[planeNum].memory.userPtr) &&
            (buffer->planes[planeNum].length == planes[planeNum].length)) {
            continue;
        }
        if (planes[planeNum].length < buffer->planes[planeNum].minLength) {
            CameraBufferReleaseUserPtr(buffer);
            return HDF_FAILURE;
        }
        if (buffer->planes[planeNum].memPriv != NULL) {
            if (queueImp->memOps->freeUserPtr != NULL) {
                queueImp->memOps->freeUserPtr(buffer->planes[planeNum].memPriv);
            } else {
                HDF_LOGW("%s: no freeUserPtr function!", __func__);
            }
        }

        buffer->planes[planeNum].memPriv = NULL;
        buffer->planes[planeNum].bytesUsed = 0;
        buffer->planes[planeNum].length = 0;
        buffer->planes[planeNum].memory.userPtr = 0;
        buffer->planes[planeNum].dataOffset = 0;
        memPriv = MemoryAdapterGetUserPtr(queue, planes, planeNum);
        if (MemoryAdapterIsErrPtr(memPriv)) {
            ret = MemoryAdapterPtrErr(memPriv);
            CameraBufferReleaseUserPtr(buffer);
            return ret;
        }
        buffer->planes[planeNum].memPriv = memPriv;
    }
    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepareUserPtr(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    struct CameraPlane planes[MAX_PLANES];
    struct BufferQueue *queue = buffer->bufferQueue;
    int32_t ret;

    (void)memset_s(planes, sizeof(planes), 0, sizeof(planes[0]) * buffer->numPlanes);
    ret = CameraBufferFillBuffer(buffer, planes, userBuffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraBufferFillBuffer failed", __func__);
        return ret;
    }

    ret = CameraQueuePrepareUserPtrMem(queue, buffer, planes);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: user ptr mem prepare failed", __func__);
        return ret;
    }

    if (queue->queueOps->prepareBuffer != NULL) {
        ret = queue->queueOps->prepareBuffer(buffer);
        if (ret != HDF_SUCCESS) {
            CameraBufferReleaseUserPtr(buffer);
            return ret;
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepareDmaBuf(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    struct CameraPlane planes[MAX_PLANES];
    struct BufferQueue *queue = buffer->bufferQueue;
    uint32_t planeNum;
    int32_t ret;

    (void)memset_s(planes, sizeof(planes), 0, sizeof(planes[0]) * buffer->numPlanes);
    ret = CameraBufferFillBuffer(buffer, planes, userBuffer);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    ret = CameraQueueAttachDmaBuf(queue, buffer, planes);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: dma buffer attach failed", __func__);
        return ret;
    }
    ret = CameraBufferMapDmaBuf(buffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: dma buffer map failed", __func__);
        return ret;
    }

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        buffer->planes[planeNum].bytesUsed = planes[planeNum].bytesUsed;
        buffer->planes[planeNum].length = planes[planeNum].length;
        buffer->planes[planeNum].memory.fd = planes[planeNum].memory.fd;
        buffer->planes[planeNum].dataOffset = planes[planeNum].dataOffset;
    }
    if (queue->queueOps->prepareBuffer != NULL) {
        ret = queue->queueOps->prepareBuffer(buffer);
        if (ret != HDF_SUCCESS) {
            CameraBuffeFreeDma(buffer);
            return ret;
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepare(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    enum BufferState origState = buffer->state;
    int32_t ret;

    if ((queue->flags & QUEUE_STATE_ERROR) != 0) {
        return HDF_FAILURE;
    }
    if ((buffer->flags & BUFFER_PREPARED) != 0) {
        return HDF_SUCCESS;
    }

    buffer->state = BUFFER_STATE_PREPARING;
    switch (queue->memType) {
        case MEMTYPE_MMAP:
            ret = CameraBufferPrepareMmap(buffer, userBuffer);
            break;
        case MEMTYPE_USERPTR:
            ret = CameraBufferPrepareUserPtr(buffer, userBuffer);
            break;
        case MEMTYPE_DMABUF:
            ret = CameraBufferPrepareDmaBuf(buffer, userBuffer);
            break;
        default:
            HDF_LOGE("%s: invalid queue type, type=%{public}u", __func__, queue->memType);
            ret = HDF_FAILURE;
            break;
    }
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: buffer prepare is wrong", __func__);
        return ret;
    }

    CameraBufferPrepareMem(buffer);
    buffer->flags |= BUFFER_PREPARED;
    buffer->state = origState;
    return ret;
}

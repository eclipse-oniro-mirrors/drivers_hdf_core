/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_service_observer.h"
#include "hdf_base.h"
#include "hdf_cstring.h"
#include "hdf_core_log.h"
#include "hdf_observer_record.h"
#include "hdf_service_subscriber.h"

#define HDF_LOG_TAG service_observer
#define MAX_NOTIFY_SUBSCRIBERS 32

bool HdfServiceObserverConstruct(struct HdfServiceObserver *observer)
{
    if (observer == NULL) {
        HDF_LOGE("observer is null");
        return false;
    }
    if (OsalMutexInit(&observer->observerMutex) != HDF_SUCCESS) {
        return false;
    }
    HdfSListInit(&observer->services);
    return true;
}

void HdfServiceObserverDestruct(struct HdfServiceObserver *observer)
{
    if (observer != NULL) {
        HdfSListFlush(&observer->services, HdfServiceObserverRecordDelete);
        OsalMutexDestroy(&observer->observerMutex);
    }
}

int HdfServiceObserverSubscribeService(
    struct HdfServiceObserver *observer, const char *svcName, devid_t deviceId, struct SubscriberCallback callback)
{
    struct HdfServiceObserverRecord *serviceRecord = NULL;
    struct HdfServiceSubscriber *subscriber = NULL;
    void *publisher = NULL;
    bool needNotify = false;
    struct SubscriberCallback notifyCallback = {0};
    uint32_t serviceKey = HdfStringMakeHashKey(svcName, 0);
    if ((observer == NULL) || (svcName == NULL)) {
        HDF_LOGE("observer or svcName or callback.OnServiceConnected is null");
        return HDF_FAILURE;
    }
    OsalMutexLock(&observer->observerMutex);
    serviceRecord = (struct HdfServiceObserverRecord *)HdfSListSearch(
        &observer->services, serviceKey, HdfServiceObserverRecordCompare);
    if (serviceRecord == NULL) {
        serviceRecord = HdfServiceObserverRecordObtain(serviceKey);
        if (serviceRecord == NULL) {
            HDF_LOGE("failed to subscribe service, serviceRecord is null");
            OsalMutexUnlock(&observer->observerMutex);
            return HDF_FAILURE;
        }
        subscriber = HdfServiceSubscriberObtain(callback, deviceId);
        if (subscriber == NULL) {
            HDF_LOGE("failed to subscribe service, subscriber is null");
            HdfServiceObserverRecordRecycle(serviceRecord);
            OsalMutexUnlock(&observer->observerMutex);
            return HDF_FAILURE;
        }
        HdfSListAdd(&observer->services, &serviceRecord->entry);
    } else {
        subscriber = HdfServiceSubscriberObtain(callback, deviceId);
        if (subscriber == NULL) {
            HDF_LOGE("failed to subscribe service, subscriber obtain null");
            OsalMutexUnlock(&observer->observerMutex);
            return HDF_FAILURE;
        }
    }
    if ((serviceRecord->publisher != NULL) && (subscriber->callback.OnServiceConnected != NULL) &&
        ((serviceRecord->policy != SERVICE_POLICY_PRIVATE) || (serviceRecord->devId == deviceId))) {
        subscriber->state = HDF_SUBSCRIBER_STATE_READY;
        publisher = serviceRecord->publisher;
        notifyCallback = subscriber->callback;
        needNotify = true;
    }
    OsalMutexLock(&serviceRecord->obsRecMutex);
    HdfSListAdd(&serviceRecord->subscribers, &subscriber->entry);
    OsalMutexUnlock(&serviceRecord->obsRecMutex);
    OsalMutexUnlock(&observer->observerMutex);

    if (needNotify) {
        notifyCallback.OnServiceConnected(notifyCallback.deviceObject, publisher);
    }
    return HDF_SUCCESS;
}

int HdfServiceObserverPublishService(struct HdfServiceObserver *observer, const char *svcName, devid_t deviceId,
    uint16_t policy, struct HdfObject *service)
{
    struct HdfServiceObserverRecord *serviceRecord = NULL;
    uint32_t serviceKey = HdfStringMakeHashKey(svcName, 0);
    if ((observer == NULL) || (svcName == NULL)) {
        HDF_LOGE("observer or svcName is null");
        return HDF_FAILURE;
    }
    OsalMutexLock(&observer->observerMutex);
    serviceRecord = (struct HdfServiceObserverRecord *)HdfSListSearch(
        &observer->services, serviceKey, HdfServiceObserverRecordCompare);
    if (serviceRecord == NULL) {
        serviceRecord = HdfServiceObserverRecordObtain(serviceKey);
        if (serviceRecord == NULL) {
            HDF_LOGE("PublishService failed, serviceRecord is null");
            OsalMutexUnlock(&observer->observerMutex);
            return HDF_FAILURE;
        }
        serviceRecord->publisher = service;
        serviceRecord->devId = deviceId;
        serviceRecord->policy = policy;
        HdfSListAdd(&observer->services, &serviceRecord->entry);
        OsalMutexUnlock(&observer->observerMutex);
    } else {
        struct SubscriberCallback callbacks[MAX_NOTIFY_SUBSCRIBERS];
        void *publisher = NULL;
        int count = 0;
        int i;
        struct HdfSListIterator it;

        serviceRecord->publisher = service;
        OsalMutexLock(&serviceRecord->obsRecMutex);
        publisher = serviceRecord->publisher;
        HdfSListIteratorInit(&it, &serviceRecord->subscribers);
        while (HdfSListIteratorHasNext(&it)) {
            struct HdfServiceSubscriber *subscriber =
                (struct HdfServiceSubscriber *)HdfSListIteratorNext(&it);
            if (deviceId == subscriber->devId || policy != SERVICE_POLICY_PRIVATE) {
                subscriber->state = HDF_SUBSCRIBER_STATE_READY;
                if (subscriber->callback.OnServiceConnected != NULL && count < MAX_NOTIFY_SUBSCRIBERS) {
                    callbacks[count++] = subscriber->callback;
                }
            }
        }
        OsalMutexUnlock(&serviceRecord->obsRecMutex);
        OsalMutexUnlock(&observer->observerMutex);

        for (i = 0; i < count; i++) {
            callbacks[i].OnServiceConnected(callbacks[i].deviceObject, publisher);
        }
        return HDF_SUCCESS;
    }
    return HDF_SUCCESS;
}

void HdfServiceObserverRemoveRecord(struct HdfServiceObserver *observer, const char *svcName)
{
    struct HdfServiceObserverRecord *serviceRecord = NULL;
    uint32_t serviceKey = HdfStringMakeHashKey(svcName, 0);
    if ((observer == NULL) || (svcName == NULL)) {
        HDF_LOGW("observer or svcName is null");
        return;
    }
    OsalMutexLock(&observer->observerMutex);
    serviceRecord = (struct HdfServiceObserverRecord *)HdfSListSearch(
        &observer->services, serviceKey, HdfServiceObserverRecordCompare);
    if (serviceRecord != NULL) {
        HdfSListRemove(&observer->services, &serviceRecord->entry);
        HdfServiceObserverRecordRecycle(serviceRecord);
    }
    OsalMutexUnlock(&observer->observerMutex);
}

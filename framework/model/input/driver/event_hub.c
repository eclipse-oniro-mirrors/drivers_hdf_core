/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "osal_time.h"
#include "hdf_log.h"
#include "event_hub.h"

#define SEC_TO_USEC    1000000

static void EventQueueWorkEntry(void *arg)
{
    InputDevice *inputDev = (InputDevice *)arg;
    if (inputDev == NULL) {
        HDF_LOGE("%s: inputDev is NULL", __func__);
        return;
    }
    HdfDeviceSendEvent(inputDev->hdfDevObj, 0, inputDev->pkgBuf);
    HdfSbufFlush(inputDev->pkgBuf);
}

void PushOnePackage(InputDevice *inputDev, uint32_t type, uint32_t code, int32_t value)
{
    OsalTimespec time;
    EventPackage package = {0};
    uint32_t flag;
    InputManager *inputManager = GetInputManager();

    if (inputDev == NULL) {
        HDF_LOGE("%s: parm is null", __func__);
        return;
    }
    OsalSpinLockIrqSave(&inputManager->lock, &flag);
    package.type = type;
    package.code = code;
    package.value = value;
    OsalGetTime(&time);
    package.time = time.sec * SEC_TO_USEC + time.usec;

    if (!HdfSbufWriteBuffer(inputDev->pkgBuf, &package, sizeof(EventPackage))) {
        HDF_LOGE("%s: sbuf write pkg failed, clear sbuf", __func__);
        HdfSbufFlush(inputDev->pkgBuf);
        inputDev->errFrameFlag = true;
    }
    inputDev->pkgCount++;

    if (inputDev->pkgCount >= inputDev->pkgNum) {
        HDF_LOGE("%s: current pkgs num beyond the sbuf limit", __func__);
        inputDev->errFrameFlag = true;
    }

    if (type == EV_SYN && code == SYN_REPORT) {
        if (!HdfSbufWriteBuffer(inputDev->pkgBuf, NULL, 0)) {
            HDF_LOGE("%s: sbuf write null pkg failed, clear sbuf", __func__);
            HdfSbufFlush(inputDev->pkgBuf);
            inputDev->errFrameFlag = true;
        }

        if (!inputDev->errFrameFlag) {
            if (HdfWorkInit(inputDev->eventWork, EventQueueWorkEntry, inputDev) != HDF_SUCCESS) {
                HDF_LOGE("%s: create event thread failed", __func__);
                OsalSpinUnlockIrqRestore(&inputManager->lock, &flag);
                return;
            }
            if (!HdfAddWork(&inputDev->eventWorkQueue, inputDev->eventWork)) {
                HDF_LOGE("%s: Add event work queue failed", __func__);
                OsalSpinUnlockIrqRestore(&inputManager->lock, &flag);
                return;
            }
        }

        inputDev->pkgCount = 0;
        inputDev->errFrameFlag = false;
    }
    OsalSpinUnlockIrqRestore(&inputManager->lock, &flag);
}
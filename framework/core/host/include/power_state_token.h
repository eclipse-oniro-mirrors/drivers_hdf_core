/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef POWER_STATE_TOKEN_H
#define POWER_STATE_TOKEN_H

#include "hdf_sref.h"
#include "power_state_token_if.h"
#include "hdf_pm.h"
#include "osal_atomic.h"

#ifdef __cplusplus
extern "C" {
#endif
struct PowerStateToken {
    struct IPowerStateToken super;
    const struct IPowerEventListener *listener;
    struct HdfDeviceObject *deviceObject;
    struct HdfSRef wakeRef;
    HdfPsmState psmState;
    uint32_t mode;
    struct OsalAtomic taskRefCount;
};

struct PowerStateToken *PowerStateTokenNewInstance(
    struct HdfDeviceObject *deviceObject, const struct IPowerEventListener *listener);
void PowerStateTokenFreeInstance(struct PowerStateToken *stateToken);
int PowerStateChange(struct PowerStateToken *stateToken, uint32_t pEvent);

#ifdef __cplusplus
}
#endif

#endif /* POWER_STATE_TOKEN_H */
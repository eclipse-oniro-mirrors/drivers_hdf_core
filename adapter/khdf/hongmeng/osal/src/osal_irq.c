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

#include "osal_irq.h"
#include "hdf_core_log.h"
#include <udk/irq.h>

#define HDF_LOG_TAG osal_irq

int32_t OsalRegisterIrq(uint32_t irqId, uint32_t config, OsalIRQHandle handle,
                        const char *name, void *data)
{
    uint32_t ret;
    const char *irqName = NULL;

    irqName = (name != NULL) ? name : "hdf_irq";

    ret = udk_request_irq(irqId, (irq_handler_t)handle, 0ULL, irqName, data);
    if (ret != 0) {
        HDF_LOGE("%s fail %u", __func__, ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t OsalUnregisterIrq(uint32_t irqId, void *dev)
{
    UNUSED(dev);
    udk_disable_irq(irqId);
    udk_free_irq(irqId);

    return HDF_SUCCESS;
}

int32_t OsalEnableIrq(uint32_t irqId)
{
    udk_enable_irq(irqId);
    return HDF_SUCCESS;
}

int32_t OsalDisableIrq(uint32_t irqId)
{
    udk_disable_irq(irqId);
    return HDF_SUCCESS;
}
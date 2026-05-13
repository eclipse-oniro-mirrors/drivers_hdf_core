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

#include <udk/device.h>
#include <udk/module.h>
#include "devmgr_service_start.h"
#include "hdf_base.h"
#include "hdf_core_log.h"

#define HDF_LOG_TAG devmgr_load

int32_t HdfDeviceManagerInit(void)
{
    int32_t ret;
    
    HDF_LOGI("%s in", __func__);
    ret = DeviceManagerStart();
    if (ret < 0) {
        HDF_LOGE("%s start failed %d", __func__, ret);
        return HDF_FAILURE;
    }

    HDF_LOGI("%s start success", __func__);
    return HDF_SUCCESS;
}

udk_module_init(HdfDdeviceManagerInit);

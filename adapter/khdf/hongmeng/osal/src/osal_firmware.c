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

#include "osal_firmware.h"
#include "hdf_core_log.h"

#define HDF_LOG_TAG osal_fw

int32_t OsalRequestFirmware(struct OsalFirmware *fwPara, const char *fwName, void *device)
{
    UNUSED(fwPara);
    UNUSED(fwName);
    UNUSED(device);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t OsalSeekFirmware(struct OsalFirmware *fwPara, uint32_t offset)
{
    UNUSED(fwPara);
    UNUSED(offset);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t OsalReadFirmware(struct OsalFirmware *fwPara, struct OsalFwBlock *block)
{
    UNUSED(fwPara);
    UNUSED(block);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t OsalReleaseFirmware(struct OsalFirmware *fwPara)
{
    UNUSED(fwPara);
    return HDF_ERR_NOT_SUPPORT;
}
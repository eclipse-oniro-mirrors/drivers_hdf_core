/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <dlfcn.h>
#include <unistd.h>
#include "hdf_load_vdi.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG dev_load_vdi

#ifdef __ARCH64__
#define VDI_PATH HDF_LIBRARY_DIR"64/"
#else
#define VDI_PATH HDF_LIBRARY_DIR"/"
#endif

struct HdfVdiObject *HdfLoadVdi(const char *soName, uint32_t version)
{
    char path[PATH_MAX + 1] = {0};
    char resolvedPath[PATH_MAX + 1] = {0};

    if (soName == NULL) {
        HDF_LOGE("name is NULL");
        return NULL;
    }

    if (snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s/%s", VDI_PATH, soName) < 0) {
        HDF_LOGE("%{public}s %{public}s snprintf_s failed", __func__, soName);
        return NULL;
    }

    if (realpath(path, resolvedPath) == NULL || strncmp(resolvedPath, VDI_PATH, strlen(VDI_PATH)) != 0) {
        HDF_LOGE("%{public}s %{public}s %{public}s realpath file name failed", __func__, path, resolvedPath);
        return NULL;
    }

    struct HdfVdiObject *vdi = (struct HdfVdiObject *)OsalMemCalloc(sizeof(*vdi));
    if (vdi == NULL) {
        HDF_LOGE("%{public}s malloc failed", __func__);
        return NULL;
    }

    void *handler = dlopen(resolvedPath, RTLD_LAZY);
    if (handler == NULL) {
        HDF_LOGE("%{public}s dlopen failed %{public}s", __func__, dlerror());
        OsalMemFree(vdi);
        return NULL;
    }

    struct HdfVdiBase *vdiBase = *(struct HdfVdiBase **)dlsym(handler, "hdfVdiDesc");
    if (vdiBase == NULL) {
        HDF_LOGE("%{public}s dlsym hdfVdiDesc failed %{public}s", __func__, dlerror());
        dlclose(handler);
        OsalMemFree(vdi);
        return NULL;
    }

    if (version != vdiBase->moduleVersion) {
        HDF_LOGE("%{public}s check version failed %{public}d %{public}d", __func__, version, vdiBase->moduleVersion);
        dlclose(handler);
        OsalMemFree(vdi);
        return NULL;    
    }

    if (vdiBase->OpenVdi) {
        vdiBase->OpenVdi(vdiBase);
    }

    vdi->dlHandler = (uintptr_t)handler;
    vdi->vdiBase = vdiBase;

    return vdi;
}

void HdfCloseVdi(struct HdfVdiObject *vdi)
{
    if (vdi == NULL || vdi->dlHandler == 0 || vdi->vdiBase == NULL) {
        HDF_LOGE("%{public}s para invalid", __func__);
        return;
    }

    struct HdfVdiBase *vdiBase = vdi->vdiBase;
    if (vdiBase->CloseVdi) {
        vdiBase->CloseVdi(vdiBase);
    }

    dlclose((void *)vdi->dlHandler);
    vdi->dlHandler = 0;
    vdi->vdiBase = NULL;
    OsalMemFree(vdi);
}


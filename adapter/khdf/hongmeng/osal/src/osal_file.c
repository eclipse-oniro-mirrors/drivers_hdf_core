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

#include "osal_file.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include <udk/iolib.h>
#include <udk/vfs.h>

#define HDF_LOG_TAG osal_file

struct udk_file {
    int fd;
    int64_t pos;
};

int32_t OsalFileOpen(OsalFile *file, const char *path, int flags, uint32_t rights)
{
    struct udk_file *udkFp = NULL;

    if (file == NULL || path == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkFp = OsalMemCalloc(sizeof(struct udk_file));
    if (udkFp == NULL) {
        HDF_LOGE("%s malloc failed", __func__);
        return HDF_FAILURE;
    }

    udkFp->fd = udk_vfs_open(path, flags, (mode_t)rights);
    if (udkFp->fd < 0) {
        HDF_LOGE("%s open file fail %d", __func__, udkFp->fd);
        OsalMemFree(udkFp);
        return HDF_FAILURE;
    }

    file->realFile = (void *)udkFp;
    return HDF_SUCCESS;
}

ssize_t OsalFileWrite(OsalFile *file, const void *string, uint32_t length)
{
    ssize_t ret;
    struct udk_file *udkFp = NULL;

    if (file == NULL || file->realFile == NULL || string == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udkFp = (struct udk_file *)file->realFile;
    ret = udk_vfs_pwrite(udkFp->fd, udkFp->pos, string, length);
    if (ret < 0) {
        HDF_LOGE("%s write file length %u fail %d", __func__, length, ret);
        return HDF_FAILURE;
    }

    return ret;
}

void OsalFileClose(OsalFile *file)
{
    struct udk_file *udkFp = NULL;

    if (file == NULL || file->realFile == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return;
    }
    udkFp = (struct udk_file *)file->realFile;
    udk_vfs_close(udkFp->fd);
    OsalMemFree(udkFp);
}

ssize_t OsalFileRead(OsalFile *file, void *buf, uint32_t length)
{
    ssize_t ret;
    struct udk_file *udkFp = NULL;

    if (file == NULL || file->realFile == NULL || buf == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    udkFp = (struct udk_file *)file->realFile;

    ret = udk_vfs_pread(udkFp->fd, udkFp->pos, buf, length);
    if (ret < 0) {
        HDF_LOGE("%s read file length %u fail %d", __func__, length, ret);
        return HDF_FAILURE;
    }

    return ret;
}

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

off_t OsalFileLseek(OsalFile *file, off_t offset, int32_t whence)
{
    struct udk_file *udkFp = NULL;

    if (file == NULL || file->realFile == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    /* hongmeng does not support lseek other than SEEK_SET */
    if (whence != SEEK_SET) {
        HDF_LOGE("%s unsupport whence %d", __func__, whence);
        return HDF_ERR_INVALID_PARAM;
    }

    udkFp = (struct udk_file *)file->realFile;
    udkFp->pos = offset;

    return HDF_SUCCESS;
}

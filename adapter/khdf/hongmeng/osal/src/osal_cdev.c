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

#include "osal_cdev.h"

#include "hdf_core_log.h"
#include "osal_file.h"
#include "osal_mem.h"
#include "osal_uaccess.h"
#include <udk/char.h>
#include <udk/lib/idr.h>
#include <udk/sysfs.h>
#include <udk/uevent.h>

#define HDF_LOG_TAG osal_cdev

#define HDF_MINOR_START 0
#define HDF_MAX_CHAR_DEVICES 1024
static devnum_t g_hdfDevt;
static bool g_charRegionInited = false;
static DEFINE_IDR(g_hdfDevIdr);

struct OsalCdev {
    const struct OsalCdevOps *opsImpl;
    struct udk_char_operations udkFops;
    struct udk_char_device *udkCdev;
    struct file filep;
    void *priv;
};

static const char *StringRfindChar(const char *str, char chr)
{
    const char *p = NULL;
    if (str == NULL) {
        return NULL;
    }

    p = str + strlen(str);
    while (p >= str) {
        if (*p == chr) {
            return p;
        }
        p--;
    }

    return NULL;
}

static int OsalCdevOpen(struct udk_char_context *ctx)
{
    struct OsalCdev *cdev;

    cdev = container_of(ctx->chrdev->ops, struct OsalCdev, udkFops);
    return cdev->opsImpl->open(cdev, &cdev->filep);
}

static int OsalCdevRelease(struct udk_char_context *ctx)
{
    struct OsalCdev *cdev;

    cdev = container_of(ctx->chrdev->ops, struct OsalCdev, udkFops);
    return cdev->opsImpl->release(cdev, &cdev->filep);
}

static int OsalCdevRead(struct udk_char_context *ctx,
                        void *buf, unsigned long count, unsigned long long *off)
{
    struct OsalCdev *cdev;

    cdev = container_of(ctx->chrdev->ops, struct OsalCdev, udkFops);
    return (int)cdev->opsImpl->read(&cdev->filep, buf, count, (int64_t *)off);
}

static int OsalCdevWrite(struct udk_char_context *ctx,
                         const void *buf, unsigned long count, unsigned long long *off)
{
    struct OsalCdev *cdev;

    cdev = container_of(ctx->chrdev->ops, struct OsalCdev, udkFops);
    return cdev->opsImpl->write(&cdev->filep, buf, count, (int64_t *)off);
}

static int OsalCdevIoctl(struct udk_char_context *ctx, unsigned int cmd, unsigned long arg)
{
    struct OsalCdev *cdev;

    cdev = container_of(ctx->chrdev->ops, struct OsalCdev, udkFops);
    return (int)cdev->opsImpl->ioctl(&cdev->filep, cmd, arg);
}

static long long OsalCdevSeek(struct udk_char_context *ctx, long long offset, int whence)
{
    struct OsalCdev *cdev;

    cdev = container_of(ctx->chrdev->ops, struct OsalCdev, udkFops);
    return cdev->opsImpl->seek(&cdev->filep, offset, whence);
}

static int OsalCdevPoll(struct udk_char_context *ctx,
                        struct udk_poll_wevent *pevent, unsigned int *revent)
{
    struct OsalCdev *cdev;

    cdev = container_of(ctx->chrdev->ops, struct OsalCdev, udkFops);
    *revent = cdev->opsImpl->poll(&cdev->filep, pevent);
    return 0;
}

static void FillFops(struct udk_char_operations *udkFops, const struct OsalCdevOps *fops)
{
    if (fops->open != NULL) {
        udkFops->open = OsalCdevOpen;
    }
    if (fops->release != NULL) {
        udkFops->close = OsalCdevRelease;
    }
    if (fops->read != NULL) {
        udkFops->read = OsalCdevRead;
    }
    if (fops->write != NULL) {
        udkFops->write = OsalCdevWrite;
    }
    if (fops->ioctl != NULL) {
        udkFops->ioctl = OsalCdevIoctl;
    }
    if (fops->poll != NULL) {
        udkFops->poll = OsalCdevPoll;
    }
    if (fops->seek != NULL) {
        udkFops->llseek = OsalCdevSeek;
    }
}

static int UdkHdfUeventRead(void *ctx, void *dst, size_t pos, size_t size, size_t *rsize)
{
    int ret;
    struct udk_device *udkDev = NULL;
    struct udk_uevent_env *env = NULL;

    if (ctx == NULL || dst == NULL || size == 0 || rsize == NULL) {
        return -EINVAL;
    }

    udkDev = (struct udk_device *)ctx;
    env = udk_uevent_env_alloc();
    if (env == NULL) {
        return -ENOMEM;
    }

    ret = udk_uevent_env_add(env, UDK_UEVENT_ENV_BUF, UEVENT_READ_FORMAT,
                             MAJOR(udkDev->devt), MINOR(udkDev->devt), udkDev->name);
    if (ret < 0) {
        udk_uevent_env_free(env);
        return ret;
    }

    ret = udk_uevent_env_buf_read(env, dst, pos, size, rsize);
    if (ret < 0) {
        HDF_LOGW("udk uevent read buf failed, ret=%d", ret);
        udk_uevent_env_free(env);
        return ret;
    }

    udk_uevent_env_free(env);
    return 0;
}

static int UdkHdfUeventWrite(void *ctx, void *dst, size_t pos, size_t size, size_t *rsize)
{
    int ret;
    struct udk_device *udkDev = NULL;
    struct udk_uevent_env *env = NULL;
    char action_str[UEVENT_ACTION_SIZE];

    if (ctx == NULL || dst == NULL || size == 0 || rsize == NULL) {
        return -EINVAL;
    }

    udkDev = (struct udk_device *)ctx;
    memset_s(action_str, sizeof(action_str), 0, sizeof(action_str));
    ret = udk_uevent_action(dst, size, action_str, sizeof(action_str) - 1, rsize);
    if (ret < 0) {
        return ret;
    }

    env = udk_uevent_env_alloc();
    if (env == NULL) {
        return -ENOMEM;
    }

    ret = udk_uevent_env_add(env, UDK_UEVENT_ENV_ACTION, "%s%c", action_str, '\0');
    if (ret < 0) {
        udk_uevent_env_free(env);
        return ret;
    }

    ret = udk_uevent_env_add(env, UDK_UEVENT_ENV_DEVPATH, "%s/%s%c",
                             "/devices/virtual/hdf_udk", udkDev->name, '\0');
    if (ret < 0) {
        udk_uevent_env_free(env);
        return ret;
    }

    ret = udk_uevent_env_add(env, UDK_UEVENT_ENV_BUF, UEVENT_WRITE_FORMAT,
                             action_str, '\0', "/devices/virtual/hdf_udk", udkDev->name, '\0',
                             "hdf_udk", '\0', MAJOR(udkDev->devt), '\0', MINOR(udkDev->devt), '\0',
                             udkDev->name, '\0');
    if (ret < 0) {
        udk_uevent_env_free(env);
        return ret;
    }

    ret = udk_uevent_net_broadcast(env);
    if (ret < 0) {
        HDF_LOGW("device %s uevent notify failed, ret=%d", udkDev->name, ret);
        udk_uevent_env_free(env);
        return ret;
    }

    udk_uevent_env_free(env);
    return 0;
}

static const struct udk_sysfs_fops hdf_uevent_fops = {
    .read = UdkHdfUeventRead,
    .write = UdkHdfUeventWrite,
};

static void UdkHdfSysfsAdd(struct udk_char_device *cdev)
{
    int ret;
    struct udk_device *dev = NULL;
    char path[UDK_SYSFS_PATH_NAME_MAX];

    if (cdev == NULL) {
        return;
    }

    ret = udk_sysfs_dir_create("devices/virtual/hdf_udk");
    if (ret < 0) {
        HDF_LOGE("create default directory failed, ret=%d", ret);
        return;
    }

    ret = udk_sysfs_dir_create("class/hdf_udk");
    if (ret < 0) {
        HDF_LOGE("create default directory failed, ret=%d", ret);
        return;
    }

    dev = &cdev->udkdev;
    ret = udk_sysfs_device_dir_create(dev, "hdf_udk");
    if (ret < 0) {
        HDF_LOGE("create default directory failed, ret=%d", ret);
        udk_sysfs_dir_destroy("devices/virtual/hdf_udk");
        udk_sysfs_dir_destroy("class/hdf_udk");
        return;
    }

    memset_s(path, sizeof(path), 0, sizeof(path));
    ret = snprintf_s(path, UDK_SYSFS_PATH_NAME_MAX, UDK_SYSFS_PATH_NAME_MAX - 1,
                     "%s/%s", "devices/virtual/hdf_udk", dev->name);
    if (ret < 0) {
        HDF_LOGE("obtain path devices/virtual/hdf_udk/%s failed, ret=%d", dev->name, ret);
        udk_sysfs_device_dir_destroy(dev, "hdf_udk");
        udk_sysfs_dir_destroy("devices/virtual/hdf_udk");
        udk_sysfs_dir_destroy("class/hdf_udk");
        return;
    }

    /* create /sys/devices/virtual/hdf_udk/dev->name/uevent node */
    ret = udk_sysfs_file_create(path, "uevent", &hdf_uevent_fops, dev);
    if (ret < 0) {
        HDF_LOGW("create file node failed, ret=%d", ret);
        udk_sysfs_device_dir_destroy(dev, "hdf_udk");
        udk_sysfs_dir_destroy("devices/virtual/hdf_udk");
        udk_sysfs_dir_destroy("class/hdf_udk");
        return;
    }
}

static void UdkHdfSysfsDelete(struct udk_char_device *cdev)
{
    int ret;
    struct udk_device *dev = NULL;
    char path[UDK_SYSFS_PATH_NAME_MAX];

    if (cdev == NULL) {
        return;
    }
    dev = &cdev->udkdev;

    memset_s(path, sizeof(path), 0, sizeof(path));
    ret = snprintf_s(path, UDK_SYSFS_PATH_NAME_MAX, UDK_SYSFS_PATH_NAME_MAX - 1,
                     "%s/%s", "devices/virtual/hdf_udk", dev->name);
    if (ret < 0) {
        HDF_LOGE("obtain path devices/virtual/hdf_udk/%s failed, ret=%d", dev->name, ret);
        return;
    }

    /* destroy /sys/devices/virtual/hdf_udk/dev->name/uevent node */
    ret = udk_sysfs_file_destroy(path, "uevent");
    if (ret < 0) {
        HDF_LOGW("destroy file node failed, ret=%d", ret);
        return;
    }

    ret = udk_sysfs_device_dir_destroy(dev, "hdf_udk");
    if (ret < 0) {
        HDF_LOGW("destroy default directory failed, ret=%d", ret);
        return;
    }
}

static int InitHdfCharDevRegion(void)
{
    int ret = 0;

    if (g_charRegionInited) {
        HDF_LOGD("Hdf chardev region already initialized");
        return 0;
    }

    ret = udk_alloc_chrdev_region(&g_hdfDevt, HDF_MINOR_START,
                                  HDF_MAX_CHAR_DEVICES, "hdf_udk");
    if (ret < 0) {
        HDF_LOGE("Failed to allocate chrdev region, ret=%d", ret);
        return HDF_FAILURE;
    }

    g_charRegionInited = true;
    return 0;
}

static int RegisterDev(struct OsalCdev *cdev, const char *devName)
{
    int ret;
    int id;
    devnum_t devt;

    ret = InitHdfCharDevRegion();
    if (ret < 0) {
        return ret;
    }

    id = idr_alloc(&g_hdfDevIdr, cdev);
    if (id < 0) {
        HDF_LOGE("Failed to allocate id for OsalCdev, ret=%d", id);
        return HDF_FAILURE;
    }

    devt = MKDEV(MAJOR(g_hdfDevt), id);
    cdev->udkCdev = udk_chrdev_create(devName, devt, &cdev->udkFops);
    if (cdev->udkCdev == NULL) {
        HDF_LOGE("Failed to create character device");
        idr_remove(&g_hdfDevIdr, id);
        return HDF_FAILURE;
    }

    UdkHdfSysfsAdd(cdev->udkCdev);
    return 0;
}

struct OsalCdev *OsalAllocCdev(const struct OsalCdevOps *fops)
{
    struct OsalCdev *cdev = NULL;

    if (fops == NULL) {
        HDF_LOGE("Invalid param");
        return NULL;
    }

    cdev = OsalMemCalloc(sizeof(struct OsalCdev));
    if (cdev == NULL) {
        HDF_LOGE("Failed to allocate memory for OsalCdev");
        return NULL;
    }

    FillFops(&cdev->udkFops, fops);
    cdev->opsImpl = fops;

    return cdev;
}

int OsalRegisterCdev(struct OsalCdev *cdev, const char *name, unsigned int mode, void *priv)
{
    UNUSED(mode);
    const char *lastSlash;
    int ret;

    if (cdev == NULL || name == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    lastSlash = StringRfindChar(name, '/');
    ret = RegisterDev(cdev, (lastSlash == NULL) ? name : (lastSlash + 1));
    if (ret == HDF_SUCCESS) {
        cdev->priv = priv;
    }

    return ret;
}

void OsalUnregisterCdev(struct OsalCdev *cdev)
{
    if (cdev != NULL) {
        UdkHdfSysfsDelete(cdev->udkCdev);
        udk_chrdev_destroy(cdev->udkCdev);
        idr_remove(&g_hdfDevIdr, MINOR(cdev->udkCdev->udkdev.devt));
    }
}

void OsalFreeCdev(struct OsalCdev *cdev)
{
    if (cdev != NULL) {
        OsalMemFree(cdev);
    }
}

void *OsalGetCdevPriv(struct OsalCdev *cdev)
{
    if (cdev != NULL && cdev->priv != NULL) {
        return cdev->priv;
    }
    return NULL;
}

void OsalSetFilePriv(struct file *filep, void *priv)
{
    if (filep != NULL) {
        filep->private_data = priv;
    }
}

void *OsalGetFilePriv(struct file *filep)
{
    if (filep != NULL && filep->private_data != NULL) {
        return filep->private_data;
    }
    return NULL;
}
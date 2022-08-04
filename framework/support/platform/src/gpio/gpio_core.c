/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "gpio/gpio_core.h"
#ifdef __LITEOS_M__
#include "los_interrupt.h"
#endif
#include "osal_mem.h"
#include "platform_core.h"

#define HDF_LOG_TAG gpio_core

#define GPIO_IRQ_STACK_SIZE        10000

static inline void GpioInfoLock(struct GpioInfo *ginfo)
{
#ifndef __LITEOS_M__
    (void)OsalSpinLockIrqSave(&ginfo->spin, &ginfo->irqSave);
#else
    ginfo->irqSave = LOS_IntLock();
#endif
}

static inline void GpioInfoUnlock(struct GpioInfo *ginfo)
{
#ifndef __LITEOS_M__
    (void)OsalSpinUnlockIrqRestore(&ginfo->spin, &ginfo->irqSave);
#else
    LOS_IntRestore(ginfo->irqSave);
#endif
}

int32_t GpioCntlrWrite(struct GpioCntlr *cntlr, uint16_t local, uint16_t val)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->write == NULL) {
        HDF_LOGE("%s: ops or write is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->write(cntlr, local, val);
}

int32_t GpioCntlrRead(struct GpioCntlr *cntlr, uint16_t local, uint16_t *val)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->read == NULL) {
        HDF_LOGE("%s: ops or read is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (val == NULL) {
        HDF_LOGE("%s: val is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return cntlr->ops->read(cntlr, local, val);
}

int32_t GpioCntlrSetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t dir)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->setDir == NULL) {
        HDF_LOGE("%s: ops or setDir is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->setDir(cntlr, local, dir);
}

int32_t GpioCntlrGetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t *dir)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->getDir == NULL) {
        HDF_LOGE("%s: ops or getDir is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (dir == NULL) {
        HDF_LOGE("%s: dir is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return cntlr->ops->getDir(cntlr, local, dir);
}

int32_t GpioCntlrToIrq(struct GpioCntlr *cntlr, uint16_t local, uint16_t *irq)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->toIrq == NULL) {
        HDF_LOGE("%s: ops or toIrq is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->toIrq(cntlr, local, irq);
}

void GpioCntlrIrqCallback(struct GpioCntlr *cntlr, uint16_t local)
{
    struct GpioInfo *ginfo = NULL;
    struct GpioIrqRecord *irqRecord = NULL;

    if (cntlr == NULL || cntlr->ginfos == NULL || local >= cntlr->count) {
        HDF_LOGW("%s: invalid cntlr(ginfos) or loal num:%hu!", __func__, local);
        return;
    }
    ginfo = &cntlr->ginfos[local];
    if (ginfo == NULL) {
        HDF_LOGW("%s: ginfo null(start:%hu, local:%hu)", __func__, cntlr->start, local);
        return;
    }

    GpioInfoLock(ginfo);
    irqRecord = ginfo->irqRecord;

    if (irqRecord == NULL) {
        HDF_LOGW("%s: irq not set (start:%hu, local:%hu)", __func__, cntlr->start, local);
        GpioInfoUnlock(ginfo);
        return;
    }
    GpioIrqRecordTrigger(irqRecord);
    GpioInfoUnlock(ginfo);
}

static int32_t GpioCntlrIrqThreadHandler(void *data)
{
    int32_t ret;
    uint32_t irqSave;
    struct GpioIrqRecord *irqRecord = (struct GpioIrqRecord *)data;

    while (true) {
        ret = OsalSemWait(&irqRecord->sem, HDF_WAIT_FOREVER);
        if (irqRecord->removed) {
            break;
        }
        if (ret != HDF_SUCCESS) {
            continue;
        }
        if (irqRecord->btmFunc != NULL) {
            (void)irqRecord->btmFunc(irqRecord->global, irqRecord->irqData);
        }
    }
    (void)OsalSpinLockIrqSave(&irqRecord->spin, &irqSave);
    (void)OsalSpinUnlockIrqRestore(&irqRecord->spin, &irqSave); // last post done
    HDF_LOGI("%s: gpio(%hu) thread exit!", __func__, irqRecord->global);
    (void)OsalSemDestroy(&irqRecord->sem);
    (void)OsalSpinDestroy(&irqRecord->spin);
    (void)OsalThreadDestroy(&irqRecord->thread);
    OsalMemFree(irqRecord);
    return HDF_SUCCESS;
}

static int32_t GpioCntlrSetIrqInner(struct GpioInfo *ginfo, struct GpioIrqRecord *irqRecord)
{
    int32_t ret;
    uint16_t local = GpioInfoToLocal(ginfo);
    struct GpioCntlr *cntlr = ginfo->cntlr;

    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    GpioInfoLock(ginfo);

    if (ginfo->irqRecord != NULL) {
        GpioInfoUnlock(ginfo);
        HDF_LOGE("%s: gpio(%hu+%hu) irq already set!", __func__, cntlr->start, local);
        return HDF_ERR_NOT_SUPPORT;
    }

    ginfo->irqRecord = irqRecord;
    GpioInfoUnlock(ginfo);
    ret = cntlr->ops->setIrq(cntlr, local, irqRecord->mode);
    if (ret != HDF_SUCCESS) {
        GpioInfoLock(ginfo);
        HDF_LOGE("%s: gpio failed to setIrq!", __func__);
        ginfo->irqRecord = NULL;
        GpioInfoUnlock(ginfo);
    }
    return ret;
}

static int32_t GpioIrqRecordCreate(struct GpioInfo *ginfo, uint16_t mode, GpioIrqFunc func, void *arg,
    struct GpioIrqRecord **new)
{
    int32_t ret;
    struct GpioIrqRecord *irqRecord = NULL;
    struct OsalThreadParam cfg;

    irqRecord = (struct GpioIrqRecord *)OsalMemCalloc(sizeof(*irqRecord));
    if (irqRecord == NULL) {
        HDF_LOGE("%s: failed to calloc for irqRecord!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    irqRecord->removed = false;
    irqRecord->mode = mode;
    irqRecord->irqFunc = ((mode & GPIO_IRQ_USING_THREAD) == 0) ? func : NULL;
    irqRecord->btmFunc = ((mode & GPIO_IRQ_USING_THREAD) != 0) ? func : NULL;
    irqRecord->irqData = arg;
    irqRecord->global = GpioInfoToGlobal(ginfo);
    if (irqRecord->btmFunc != NULL) {
        ret = OsalThreadCreate(&irqRecord->thread, GpioCntlrIrqThreadHandler, irqRecord);
        if (ret != HDF_SUCCESS) {
            OsalMemFree(irqRecord);
            HDF_LOGE("%s: failed to create irq thread:%d!", __func__, ret);
            return ret;
        }
        (void)OsalSpinInit(&irqRecord->spin);
        (void)OsalSemInit(&irqRecord->sem, 0);
        cfg.name = (char *)ginfo->name;
        cfg.priority = OSAL_THREAD_PRI_HIGHEST;
        cfg.stackSize = GPIO_IRQ_STACK_SIZE;
        ret = OsalThreadStart(&irqRecord->thread, &cfg);
        if (ret != HDF_SUCCESS) {
            (void)OsalSemDestroy(&irqRecord->sem);
            (void)OsalSpinDestroy(&irqRecord->spin);
            (void)OsalThreadDestroy(&irqRecord->thread);
            OsalMemFree(irqRecord);
            HDF_LOGE("%s: failed to start irq thread:%d!", __func__, ret);
            return ret;
        }
        HDF_LOGI("%s: gpio(%hu) thread started!", __func__, GpioInfoToGlobal(ginfo));
    }

    *new = irqRecord;
    return HDF_SUCCESS;
}

int32_t GpioCntlrSetIrq(struct GpioCntlr *cntlr, uint16_t local, uint16_t mode, GpioIrqFunc func, void *arg)
{
    int32_t ret;
    struct GpioInfo *ginfo = NULL;
    struct GpioIrqRecord *irqRecord = NULL;

    if (cntlr == NULL || cntlr->ginfos == NULL) {
        HDF_LOGE("%s: cntlr or ginfos is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (local >= cntlr->count || func == NULL) {
        HDF_LOGE("%s: local is invalid or func is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->ops == NULL || cntlr->ops->setIrq == NULL) {
        HDF_LOGE("%s: ops or setIrq is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    ginfo = &cntlr->ginfos[local];
    ret = GpioIrqRecordCreate(ginfo, mode, func, arg, &irqRecord);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to create irq record:%d!", __func__, ret);
        return ret;
    }

    ret = GpioCntlrSetIrqInner(ginfo, irqRecord);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed to set irq inner:%d!", __func__, ret);
        GpioIrqRecordDestroy(irqRecord);
    }
    return ret;
}

int32_t GpioCntlrUnsetIrq(struct GpioCntlr *cntlr, uint16_t local, void *arg)
{
    int32_t ret;
    struct GpioInfo *ginfo = NULL;
    struct GpioIrqRecord *irqRecord = NULL;

    if (cntlr == NULL || cntlr->ginfos == NULL) {
        HDF_LOGE("%s: cntlr or ginfos is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (local >= cntlr->count) {
        HDF_LOGE("%s: local is invalid!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->ops == NULL || cntlr->ops->unsetIrq == NULL) {
        HDF_LOGE("%s: ops or unsetIrq is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    ginfo = &cntlr->ginfos[local];
    GpioInfoLock(ginfo);
    if (ginfo->irqRecord == NULL) {
        GpioInfoUnlock(ginfo);
        HDF_LOGE("%s: gpio(%hu+%hu) irq not set yet!", __func__, cntlr->start, local);
        return HDF_ERR_NOT_SUPPORT;
    }
    irqRecord = ginfo->irqRecord;
    if (arg != irqRecord->irqData) {
        GpioInfoUnlock(ginfo);
        HDF_LOGE("%s: gpio(%hu+%hu) arg not match!", __func__, cntlr->start, local);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = cntlr->ops->unsetIrq(cntlr, local);
    if (ret == HDF_SUCCESS) {
        ginfo->irqRecord = NULL;
    }
    GpioInfoUnlock(ginfo);

    if (ret == HDF_SUCCESS) {
        GpioIrqRecordDestroy(irqRecord);
    }
    return ret;
}

int32_t GpioCntlrEnableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->enableIrq == NULL) {
        HDF_LOGE("%s: ops or enableIrq is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->enableIrq(cntlr, local);
}

int32_t GpioCntlrDisableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->disableIrq == NULL) {
        HDF_LOGE("%s: ops or disableIrq is NULL!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->disableIrq(cntlr, local);
}

int32_t GpioCntlrGetNumByGpioName(struct GpioCntlr *cntlr, const char *gpioName)
{
    uint32_t index;

    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (gpioName == NULL || strlen(gpioName) > GPIO_NAME_LEN) {
        HDF_LOGE("%s: gpioName is NULL or gpioName len out of range!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    for (index = 0; index < cntlr->count; index++) {
        if (strcmp(cntlr->ginfos[index].name, gpioName) == 0) {
            HDF_LOGD("%s: gpioName:%s get gpio number: %u success!", __func__, gpioName, index);
            return cntlr->start + index;
        }
    }
    HDF_LOGE("%s: gpioName:%s failed to get gpio number!", __func__, gpioName);
    return HDF_FAILURE;
}

/*
 * i2c_msg.c
 *
 * i2c message utils
 *
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "i2c_msg.h"
#include <linux/dma-buf.h>

#include "i2c_service.h"
#include "osal_mem.h"

struct I2cMsgDma {
    struct I2cMsg msg;
    struct dma_buf *buf;
};

int32_t AssignReplayBuffer(uint32_t lenReply, uint8_t **bufReply, struct I2cMsg *msgs, int16_t count)
{
    int16_t i;
    uint8_t *buf = NULL;

    *bufReply = OsalMemCalloc(lenReply);
    if (*bufReply == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }
    for (i = 0, buf = *bufReply; i < count && buf < (*bufReply + lenReply); i++) {
        if ((msgs[i].flags & I2C_FLAG_READ) != 0) {
            msgs[i].buf = buf;
            buf += msgs[i].len;
        }
    }

    return HDF_SUCCESS;
}

static int32_t RebuildMsgs(struct HdfSBuf *data, struct I2cMsg **outMsgs, int16_t count)
{
    int16_t i;
    uint32_t len;
    uint32_t lenReply = 0;
    uint8_t **bufReply = NULL;
    struct I2cUserMsg *userMsgs = NULL;
    struct I2cMsg *msgs = NULL;
    size_t msgSize = sizeof(struct I2cMsg) * count;
    uint8_t *buf = NULL;

    msgs = OsalMemAlloc(msgSize + sizeof(void *));
    bufReply = (uint8_t **)((uint8_t *)msgs + msgSize);

    for (i = 0; i < count; i++) {
        if (!HdfSbufReadBuffer(data, (const void **)&userMsgs, &len) || (userMsgs == NULL) ||
            (len != sizeof(struct I2cUserMsg))) {
            HDF_LOGE("%s: read userMsgs fail", __func__);
            OsalMemFree(msgs);
            return HDF_ERR_IO;
        }
        msgs[i].addr = userMsgs->addr;
        msgs[i].len = userMsgs->len;
        msgs[i].buf = NULL;
        msgs[i].flags = userMsgs->flags;
        if ((msgs[i].flags & I2C_FLAG_READ) != 0) {
            lenReply += msgs[i].len;
        } else if ((!HdfSbufReadBuffer(data, (const void **)&buf, &len)) || (buf == NULL)) {
            HDF_LOGE("%s: read msg[%d] buf fail", i, __func__);
        } else {
            msgs[i].len = len;
            msgs[i].buf = buf;
        }
    }

    if (lenReply > 0 && AssignReplayBuffer(lenReply, bufReply, msgs, count) != HDF_SUCCESS) {
        OsalMemFree(msgs);
        return HDF_FAILURE;
    }

    *outMsgs = msgs;
    return HDF_SUCCESS;
}

int32_t I2cMsgsRebuildFromSbuf(struct HdfSBuf *data, struct I2cMsg **msgs, int16_t *count)
{
    int16_t msgCount = 0;
    struct I2cMsg *builtMsgs = NULL;
    int32_t ret;

    if (!HdfSbufReadInt16(data, &msgCount)) {
        HDF_LOGE("%s: read count fail", __func__);
        return HDF_ERR_IO;
    }

    if (msgCount <= 0) {
        HDF_LOGE("%s: count %d out of range", __func__, msgCount);
        return HDF_ERR_OUT_OF_RANGE;
    }

    ret = RebuildMsgs(data, &builtMsgs, msgCount);

    *count = msgCount;
    *msgs = builtMsgs;
    return ret;
}

int32_t I2cMsgsWriteToSbuf(struct I2cMsg *msgs, int16_t count, struct HdfSBuf *reply)
{
    int16_t i;
    for (i = 0; i < count; i++) {
        if ((msgs[i].flags & I2C_FLAG_READ) == 0) {
            continue;
        }
        if (!HdfSbufWriteBuffer(reply, msgs[i].buf, msgs[i].len)) {
            HDF_LOGE("I2cTransferWriteBackMsgs: write msg[%hd] reply fail!", i);
            return HDF_ERR_IO;
        }
    }

    return HDF_SUCCESS;
}

void I2cMsgsFree(struct I2cMsg *msgs, int16_t count)
{
    uint8_t **bufReply = NULL;
    if (msgs == NULL) {
        return;
    }

    bufReply = (uint8_t **)((uint8_t *)msgs + sizeof(struct I2cMsg) * count);
    OsalMemFree(*bufReply);
    OsalMemFree(msgs);
}

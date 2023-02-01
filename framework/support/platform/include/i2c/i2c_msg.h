/*
 * i2c_msg.h
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
#ifndef HDF_I2C_MSG_H
#define HDF_I2C_MSG_H

#include "i2c_core.h"

int32_t I2cMsgsRebuildFromSbuf(struct HdfSBuf *data, struct I2cMsg **msgs, int16_t *count);

void I2cMsgsFree(struct I2cMsg *msgs, int16_t count);

int32_t I2cMsgsWriteToSbuf(struct I2cMsg *msgs, int16_t count, struct HdfSBuf *reply);

#endif // HDF_I2C_MSG_H
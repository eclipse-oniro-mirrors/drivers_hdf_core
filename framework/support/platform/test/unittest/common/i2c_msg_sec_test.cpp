/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include <cstring>
#include "securec.h"
#include "hdf_sbuf.h"
#include "i2c_if.h"
#include "i2c_service.h"
#include "osal_mem.h"

extern "C" {
int32_t I2cMsgsRebuildFromSbuf(struct HdfSBuf *data, struct I2cMsg **msgs, int16_t *count);
void I2cMsgsFree(struct I2cMsg *msgs, int16_t count);
int32_t I2cMsgsWriteToSbuf(struct I2cMsg *msgs, int16_t count, struct HdfSBuf *reply);
}

using namespace testing::ext;

class I2cMsgsRebuildTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

static struct HdfSBuf *CreateI2cSbuf(struct I2cMsg *msgs, int16_t count)
{
    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    if (sbuf == nullptr) {
        return nullptr;
    }
    if (!HdfSbufWriteInt16(sbuf, count)) {
        HdfSbufRecycle(sbuf);
        return nullptr;
    }
    for (int16_t i = 0; i < count; i++) {
        struct I2cUserMsg userMsg;
        userMsg.addr = msgs[i].addr;
        userMsg.len = msgs[i].len;
        userMsg.flags = msgs[i].flags;
        if (!HdfSbufWriteBuffer(sbuf, &userMsg, sizeof(userMsg))) {
            HdfSbufRecycle(sbuf);
            return nullptr;
        }
        if ((msgs[i].flags & I2C_FLAG_READ) == 0 && msgs[i].buf != nullptr) {
            if (!HdfSbufWriteBuffer(sbuf, msgs[i].buf, msgs[i].len)) {
                HdfSbufRecycle(sbuf);
                return nullptr;
            }
        }
    }
    return sbuf;
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildNormalTest001, TestSize.Level1)
{
    struct I2cMsg msgs[3];
    memset_s(msgs, sizeof(msgs), 0, sizeof(msgs));
    for (int i = 0; i < 3; i++) {
        msgs[i].addr = 0x50;
        msgs[i].len = 4;
        msgs[i].flags = I2C_FLAG_READ;
        msgs[i].buf = nullptr;
    }
    struct HdfSBuf *sbuf = CreateI2cSbuf(msgs, 3);
    ASSERT_NE(sbuf, nullptr);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_EQ(ret, HDF_SUCCESS);
    EXPECT_EQ(outCount, 3);
    if (outMsgs != nullptr) {
        I2cMsgsFree(outMsgs, outCount);
    }
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildNullSbufTest001, TestSize.Level1)
{
    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(nullptr, &outMsgs, &outCount);
    EXPECT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildNullMsgsPtrTest001, TestSize.Level1)
{
    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    ASSERT_NE(sbuf, nullptr);
    HdfSbufWriteInt16(sbuf, 1);

    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, nullptr, &outCount);
    EXPECT_NE(ret, HDF_SUCCESS);
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildZeroCountTest001, TestSize.Level1)
{
    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    ASSERT_NE(sbuf, nullptr);
    HdfSbufWriteInt16(sbuf, 0);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_NE(ret, HDF_SUCCESS);
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildNegativeCountTest001, TestSize.Level1)
{
    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    ASSERT_NE(sbuf, nullptr);
    HdfSbufWriteInt16(sbuf, -1);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_NE(ret, HDF_SUCCESS);
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildUpperBoundTest001, TestSize.Level1)
{
    const int16_t count = 20;
    struct I2cMsg msgs[count];
    memset_s(msgs, sizeof(msgs), 0, sizeof(msgs));
    for (int i = 0; i < count; i++) {
        msgs[i].addr = 0x50;
        msgs[i].len = 0xFFFF;
        msgs[i].flags = I2C_FLAG_READ;
        msgs[i].buf = nullptr;
    }
    struct HdfSBuf *sbuf = CreateI2cSbuf(msgs, count);
    ASSERT_NE(sbuf, nullptr);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_EQ(ret, HDF_ERR_INVALID_PARAM);
    if (outMsgs != nullptr) {
        I2cMsgsFree(outMsgs, outCount);
    }
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildSingleLargeMsgTest001, TestSize.Level1)
{
    struct I2cMsg msgs[1];
    memset_s(msgs, sizeof(msgs), 0, sizeof(msgs));
    msgs[0].addr = 0x50;
    msgs[0].len = 0xFFFF;
    msgs[0].flags = I2C_FLAG_READ;
    msgs[0].buf = nullptr;

    struct HdfSBuf *sbuf = CreateI2cSbuf(msgs, 1);
    ASSERT_NE(sbuf, nullptr);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_EQ(ret, HDF_SUCCESS);
    if (outMsgs != nullptr) {
        I2cMsgsFree(outMsgs, outCount);
    }
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildBoundaryMsgCountTest001, TestSize.Level1)
{
    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    ASSERT_NE(sbuf, nullptr);
    int16_t badCount = 10000;
    HdfSbufWriteInt16(sbuf, badCount);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_NE(ret, HDF_SUCCESS);
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildEmptySbufTest001, TestSize.Level1)
{
    struct HdfSBuf *sbuf = HdfSbufObtainDefaultSize();
    ASSERT_NE(sbuf, nullptr);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_NE(ret, HDF_SUCCESS);
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildWriteMsgNormalTest001, TestSize.Level1)
{
    struct I2cMsg msgs[1];
    uint8_t buf[4] = {0x01, 0x02, 0x03, 0x04};
    memset_s(msgs, sizeof(msgs), 0, sizeof(msgs));
    msgs[0].addr = 0x50;
    msgs[0].len = sizeof(buf);
    msgs[0].flags = 0;
    msgs[0].buf = buf;

    struct HdfSBuf *sbuf = CreateI2cSbuf(msgs, 1);
    ASSERT_NE(sbuf, nullptr);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_EQ(ret, HDF_SUCCESS);
    if (outMsgs != nullptr) {
        I2cMsgsFree(outMsgs, outCount);
    }
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildMixedMsgsNormalTest001, TestSize.Level1)
{
    struct I2cMsg msgs[2];
    uint8_t writeBuf[4] = {0x01, 0x02, 0x03, 0x04};
    memset_s(msgs, sizeof(msgs), 0, sizeof(msgs));
    msgs[0].addr = 0x50;
    msgs[0].len = sizeof(writeBuf);
    msgs[0].flags = 0;
    msgs[0].buf = writeBuf;
    msgs[1].addr = 0x50;
    msgs[1].len = 8;
    msgs[1].flags = I2C_FLAG_READ;
    msgs[1].buf = nullptr;

    struct HdfSBuf *sbuf = CreateI2cSbuf(msgs, 2);
    ASSERT_NE(sbuf, nullptr);

    struct I2cMsg *outMsgs = nullptr;
    int16_t outCount = 0;
    int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
    EXPECT_EQ(ret, HDF_SUCCESS);
    if (outMsgs != nullptr) {
        I2cMsgsFree(outMsgs, outCount);
    }
    HdfSbufRecycle(sbuf);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildReliabilityTest001, TestSize.Level1)
{
    int32_t ret = I2cMsgsRebuildFromSbuf(nullptr, nullptr, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(I2cMsgsRebuildTest, I2cMsgsRebuildStressTest001, TestSize.Level1)
{
    for (int iter = 0; iter < 50; iter++) {
        struct I2cMsg msgs[2];
        uint8_t writeBuf[4] = {0x01, 0x02, 0x03, 0x04};
        memset_s(msgs, sizeof(msgs), 0, sizeof(msgs));
        msgs[0].addr = 0x50;
        msgs[0].len = sizeof(writeBuf);
        msgs[0].flags = 0;
        msgs[0].buf = writeBuf;
        msgs[1].addr = 0x50;
        msgs[1].len = 4;
        msgs[1].flags = I2C_FLAG_READ;
        msgs[1].buf = nullptr;

        struct HdfSBuf *sbuf = CreateI2cSbuf(msgs, 2);
        if (sbuf == nullptr) {
            continue;
        }
        struct I2cMsg *outMsgs = nullptr;
        int16_t outCount = 0;
        int32_t ret = I2cMsgsRebuildFromSbuf(sbuf, &outMsgs, &outCount);
        EXPECT_EQ(ret, HDF_SUCCESS);
        if (outMsgs != nullptr) {
            I2cMsgsFree(outMsgs, outCount);
        }
        HdfSbufRecycle(sbuf);
    }
}

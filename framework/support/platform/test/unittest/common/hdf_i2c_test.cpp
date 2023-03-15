/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include "hdf_uhdf_test.h"
#include "i2c_test.h"
#include "hdf_io_service_if.h"

using namespace testing::ext;

class HdfLiteI2cTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfLiteI2cTest::SetUpTestCase()
{
    int32_t ret;
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_SETUP_ALL, -1};
    HdfTestOpenService();
    HdfTestSendMsgToService(&msg);

    ret = I2cTestExecute(I2C_TEST_CMD_SETUP_ALL);
    if (ret != 0) {
        printf("SetUpTestCase: User SetUp FAIL:%d\n\r", ret);
    }
    printf("SetUpTestCase: exit!\n");
}

void HdfLiteI2cTest::TearDownTestCase()
{
    int32_t ret;
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_TEARDOWN_ALL, -1};
    HdfTestSendMsgToService(&msg);
    HdfTestCloseService();

    ret = I2cTestExecute(I2C_TEST_CMD_TEARDOWN_ALL);
    if (ret != 0) {
        printf("TearDownTestCase: User TearDown FAIL:%d\n\r", ret);
    }
    printf("TearDownTestCase: exit!\n");
}

void HdfLiteI2cTest::SetUp()
{
}

void HdfLiteI2cTest::TearDown()
{
}

/**
  * @tc.name: HdfLiteI2cTestTransfer001
  * @tc.desc: i2c transfer test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfLiteI2cTest, HdfLiteI2cTestTransfer001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_TRANSFER, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_TRANSFER));
}

/**
  * @tc.name: HdfLiteI2cTestWriteRead001
  * @tc.desc: i2c write and read test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfLiteI2cTest, HdfLiteI2cTestWriteRead001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_WRITE_READ, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_WRITE_READ));
}

/**
  * @tc.name: HdfLiteI2cTestMultiThread001
  * @tc.desc: i2c multithread test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfLiteI2cTest, HdfLiteI2cTestMultiThread001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_MULTI_THREAD, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_MULTI_THREAD));
}

/**
  * @tc.name: HdfLiteI2cTestReliability001
  * @tc.desc: i2c reliability test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfLiteI2cTest, HdfLiteI2cTestReliability001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_RELIABILITY, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_RELIABILITY));
}

/**
  * @tc.name: HdfLiteI2cTestPeformance001
  * @tc.desc: i2c reliability test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfLiteI2cTest, HdfLiteI2cTestPeformance001, TestSize.Level1)
{
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_PERFORMANCE));
}

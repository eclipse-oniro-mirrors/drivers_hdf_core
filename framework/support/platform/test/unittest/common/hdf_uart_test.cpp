/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <gtest/gtest.h>
#include "hdf_uhdf_test.h"
#include "hdf_io_service_if.h"
#include "uart_test.h"

using namespace testing::ext;

class HdfUartTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfUartTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfUartTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfUartTest::SetUp()
{
}

void HdfUartTest::TearDown()
{
}

/**
 * @tc.name: UartSetTransModeTest001
 * @tc.desc: uart function test
 * @tc.type: FUNC
 * @tc.require: AR000F8689
 */
HWTEST_F(HdfUartTest, UartSetTransModeTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_SET_TRANSMODE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_SET_TRANSMODE));
}

/**
  * @tc.name: UartReadTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartReadTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = { TEST_PAL_UART_TYPE, UART_TEST_CMD_READ, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_READ));
}

/**
  * @tc.name: UartReliabilityTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartReliabilityTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_RELIABILITY, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_RELIABILITY));
}

/**
  * @tc.name: UartIfPerformanceTest001
  * @tc.desc: uart user if performance test
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfUartTest, UartIfPerformanceTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_PERFORMANCE));
}

/**
 * @tc.name: UartMiniBlockWriteTest001
 * @tc.desc: uart mini block write test only for the mini platform
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartMiniBlockWriteTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_MINI_BLOCK_WRITE_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: UartVminVtimeAttrTest001
 * @tc.desc: set vmin/vtime via attribute and read back, then verify set baud keeps them
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartVminVtimeAttrTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_VMIN_VTIME_ATTR));
}

/**
 * @tc.name: UartTransModeSwitchTest001
 * @tc.desc: switch between block and non-block modes and check read timing on an idle line
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartTransModeSwitchTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_TRANS_MODE_SWITCH));
}

/**
 * @tc.name: UartSelfLoopDataTest001
 * @tc.desc: write and read back the pattern on a loopback-wired port and verify data integrity
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartSelfLoopDataTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_SELF_LOOP_DATA));
}

/**
 * @tc.name: UartVminWakeTest001
 * @tc.desc: blocking read is woken up once vmin bytes have been written to the loopback port
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartVminWakeTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_VMIN_WAKE));
}

/**
 * @tc.name: UartVtimeInterByteTest001
 * @tc.desc: blocking read returns the partial bytes after the inter-byte vtime expires
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartVtimeInterByteTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_VTIME_INTERBYTE));
}

/**
 * @tc.name: UartBatchReadPerfTest001
 * @tc.desc: transfer the pattern in chunks on the loopback port and verify integrity and call count
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartBatchReadPerfTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_BATCH_READ_PERF));
}

/**
 * @tc.name: UartDualPortDataTest001
 * @tc.desc: cross verify data between two board uart ports wired to each other
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfUartTest, UartDualPortDataTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_DUAL_PORT_DATA));
}

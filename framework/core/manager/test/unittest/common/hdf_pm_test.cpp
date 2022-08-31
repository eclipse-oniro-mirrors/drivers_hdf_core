/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include "hdf_io_service.h"
#include "hdf_pm_driver_test.h"

using namespace testing::ext;

class HdfPmTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    static struct HdfIoService *ioService;
};

struct HdfIoService *HdfPmTest::ioService = nullptr;

void HdfPmTest::SetUpTestCase()
{
    ioService = HdfIoServiceBind("pm_ut_service");
    if (ioService == nullptr) {
        return;
    }
    ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_BEGEN, nullptr, nullptr);
}

void HdfPmTest::TearDownTestCase()
{
    if (ioService == nullptr) {
        return;
    }
    ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_END, nullptr, nullptr);
    HdfIoServiceRecycle(ioService);
}

void HdfPmTest::SetUp()
{
}

void HdfPmTest::TearDown()
{
}

/**
  * @tc.name: HdfPmTestOneDriverOnce
  * @tc.desc: test one driver
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestOneDriverOnce, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_ONE_DRIVER_ONCE, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestOneDriverTwice
  * @tc.desc: test one driver
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestOneDriverTwice, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_ONE_DRIVER_TWICE, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestOneDriverTen
  * @tc.desc: test one driver
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestOneDriverTen, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_ONE_DRIVER_TEN, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestOneDriverHundred
  * @tc.desc: test one driver
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestOneDriverHundred, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_ONE_DRIVER_HUNDRED, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestOneDriverThousand
  * @tc.desc: test one driver
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestOneDriverThousand, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_ONE_DRIVER_THOUSAND, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestTwoDriverOnce
  * @tc.desc: test two drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestTwoDriverOnce, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_TWO_DRIVER_ONCE, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestTwoDriverTwice
  * @tc.desc: test two drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestTwoDriverTwice, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_TWO_DRIVER_TWICE, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestTwoDriverTen
  * @tc.desc: test two drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestTwoDriverTen, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_TWO_DRIVER_TEN, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestTwoDriverHundred
  * @tc.desc: test two drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestTwoDriverHundred, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_TWO_DRIVER_HUNDRED, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestTwoDriverThousand
  * @tc.desc: test two drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestTwoDriverThousand, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_TWO_DRIVER_THOUSAND, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestThreeDriverOnce
  * @tc.desc: test three drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestThreeDriverOnce, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_THREE_DRIVER_ONCE, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestThreeDriverTwice
  * @tc.desc: test three drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestThreeDriverTwice, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_THREE_DRIVER_TWICE, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestThreeDriverTen
  * @tc.desc: test three drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestThreeDriverTen, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(&ioService->object, HDF_PM_TEST_THREE_DRIVER_TEN, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestThreeDriverHundred
  * @tc.desc: test three drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestThreeDriverHundred, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_THREE_DRIVER_HUNDRED, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestThreeDriverThousand
  * @tc.desc: test three drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestThreeDriverThousand, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_THREE_DRIVER_THOUSAND, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestThreeDriverSeqHundred
  * @tc.desc: test three drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestThreeDriverSeqHundred, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_THREE_DRIVER_SEQ_HUNDRED, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

/**
  * @tc.name: HdfPmTestThreeDriverHundredWithSync
  * @tc.desc: test three drivers
  * @tc.type: FUNC
  * @tc.require: SR000GGTQF
  */
HWTEST_F(HdfPmTest, HdfPmTestThreeDriverHundredWithSync, TestSize.Level3)
{
    if (ioService == nullptr) {
        ASSERT_TRUE(true);
        return;
    }

    int32_t ret = ioService->dispatcher->Dispatch(
        &ioService->object, HDF_PM_TEST_THREE_DRIVER_HUNDRED_WITH_SYNC, nullptr, nullptr);
    EXPECT_TRUE(ret == HDF_SUCCESS);
}

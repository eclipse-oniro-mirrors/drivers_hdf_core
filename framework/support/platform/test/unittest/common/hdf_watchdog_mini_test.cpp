/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include "hdf_log.h"
#include "hdf_uhdf_test.h"
#include "watchdog_if.h"

using namespace testing::ext;

class HdfWatchdogMiniTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfWatchdogMiniTest::SetUpTestCase()
{
}

void HdfWatchdogMiniTest::TearDownTestCase()
{
}

void HdfWatchdogMiniTest::SetUp()
{
}

void HdfWatchdogMiniTest::TearDown()
{
}

static int32_t WatchdogMiniEnableGetEnableTest(void)
{
    DevHandle handle = nullptr;
    int32_t ret;
    static int16_t wdtId = 7;
    bool enable = true;
  
    ret = WatchdogOpen(wdtId, &handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogMiniEnableGetEnableTest: open watchdog_%{public}hd fail, ret:%{public}d\n", wdtId, ret);
        return ret;
    }

    ret = WatchdogEnable(handle, enable);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogMiniEnableGetEnableTest: watchdog enable fail, ret: %{public}d\n", ret);
        WatchdogClose(handle);
        return ret;
    }

    HDF_LOGI("WatchdogMiniEnableGetEnableTest: watchdog enable test done, then test watchdog get enable!\n");

    ret = WatchdogGetEnable(handle, &enable);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogMiniEnableGetEnableTest: watchdog get enable fail, ret: %{public}d!\n", ret);
        WatchdogClose(handle);
        return ret;
    }
    WatchdogClose(handle);
    HDF_LOGI("WatchdogMiniEnableGetEnableTest: all test done!\n");
    return HDF_SUCCESS;
}

static int32_t WatchdogMiniBarkTest(void)
{
    DevHandle handle = nullptr;
    int32_t ret;
    static int16_t wdtId = 7;
  
    ret = WatchdogOpen(wdtId, &handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogMiniBarkTest: open watchdog_%{public}hd fail, ret:%{public}d\n", wdtId, ret);
        return ret;
    }

    ret = WatchdogBark(handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogMiniBarkTest: watchdog bark fail, ret: %{public}d!\n", ret);
        WatchdogClose(handle);
        return ret;
    }
    WatchdogClose(handle);
    HDF_LOGI("WatchdogMiniBarkTest: all test done!\n");
    return HDF_SUCCESS;
}

/**
  * @tc.name: WatchdogMiniEnableGetEnableTest001
  * @tc.desc: watchdog mini enable and get enable test.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfWatchdogMiniTest, WatchdogMiniEnableGetEnableTest001, TestSize.Level1)
{
    int32_t ret = WatchdogMiniEnableGetEnableTest();
    ASSERT_EQ(HDF_SUCCESS, ret);
}

/**
  * @tc.name: WatchdogMiniBarkTest001
  * @tc.desc: watchdog mini bark test.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfWatchdogMiniTest, WatchdogMiniBarkTest001, TestSize.Level1)
{
    int32_t ret = WatchdogMiniBarkTest();
    ASSERT_EQ(HDF_SUCCESS, ret);
}

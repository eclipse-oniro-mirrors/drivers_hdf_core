/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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


#include <gtest/gtest.h>

#include <devmgr_hdi.h>
#include <osal_time.h>
#include <servmgr_hdi.h>
#include "hdf_dump.h"
#include "hdf_dump_reg.h"
#include "hcs_tree_if.h"
#include "hcs_dm_parser.h"

#define HDF_LOG_TAG   driver_manager

namespace OHOS {
using namespace testing::ext;

static constexpr const char *TEST_SERVICE_NAME = "sample_driver_service";

class DevMgrTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    static const uint32_t waitTime = 30;
    static const uint32_t timeout = 200;
    static struct HDIServiceManager *servmgr;
    static struct HDIDeviceManager *devmgr;
};

struct HDIServiceManager *DevMgrTest::servmgr = nullptr;
struct HDIDeviceManager *DevMgrTest::devmgr = nullptr;

void DevMgrTest::SetUpTestCase()
{
    servmgr = HDIServiceManagerGet();
    devmgr = HDIDeviceManagerGet();
}

void DevMgrTest::TearDownTestCase()
{
}

void DevMgrTest::SetUp()
{
}

void DevMgrTest::TearDown()
{
}

#ifdef SAMPLE_DRIVER
/*
* @tc.name: DriverLoaderTest
* @tc.desc: driver load test
* @tc.type: FUNC
* @tc.require: AR000DT1TK
*/
HWTEST_F(DevMgrTest, DriverLoaderTest, TestSize.Level1)
{
    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);

    int ret = devmgr->LoadDevice(devmgr, TEST_SERVICE_NAME);
    ASSERT_EQ(ret, HDF_SUCCESS);

    struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    uint32_t cnt = 0;
    while (sampleService == nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    ASSERT_TRUE(sampleService != nullptr);
}
#endif

/*
* @tc.name: DriverUnLoaderTest
* @tc.desc: driver unload test
* @tc.type: FUNC
* @tc.require: AR000DT1TK
*/
HWTEST_F(DevMgrTest, DriverUnLoaderTest, TestSize.Level1)
{
    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);

    int ret = devmgr->UnloadDevice(devmgr, TEST_SERVICE_NAME);
    ASSERT_EQ(ret, HDF_SUCCESS);

    uint32_t cnt = 0;
    struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    while (sampleService != nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    ASSERT_TRUE(sampleService == nullptr);
}

#ifdef SAMPLE_DRIVER
HWTEST_F(DevMgrTest, DriverTest, TestSize.Level1)
{
    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);
    int ret;
    constexpr int loop = 100;

    for (int i = 0; i < loop; i++) {
        ret = devmgr->LoadDevice(devmgr, TEST_SERVICE_NAME);
        ASSERT_EQ(ret, HDF_SUCCESS);
        uint32_t cnt = 0;
        struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        while (sampleService == nullptr && cnt < timeout) {
            OsalMSleep(waitTime);
            sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
            cnt++;
        }
        ASSERT_TRUE(sampleService != nullptr);

        ret = devmgr->UnloadDevice(devmgr, TEST_SERVICE_NAME);
        ASSERT_EQ(ret, HDF_SUCCESS);
        cnt = 0;
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        while (sampleService != nullptr && cnt < timeout) {
            OsalMSleep(waitTime);
            sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
            cnt++;
        }
        ASSERT_TRUE(sampleService == nullptr);
    }
}
#endif

static int TestDump(struct HdfSBuf *, struct HdfSBuf *)
{
    return HDF_SUCCESS;
}

HWTEST_F(DevMgrTest, DevMgrDumpErrorTest, TestSize.Level1)
{
    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);
    HdfRegisterDumpFunc(nullptr);
    int32_t fd = 0;
    HdfRegisterDumpFunc(TestDump);
    const std::vector<std::u16string> vcr = {u"123", u"456"};
    const std::vector<std::u16string> vcr1 = {
        u"1", u"2", u"3", u"4", u"5", u"6", u"7", u"8", u"9", u"10",
        u"11", u"12", u"13", u"14", u"15", u"16", u"17", u"18", u"19", u"20",
        u"21", u"22", u"23", u"24"
    };
    int ret = HdfDump(fd, vcr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    fd = 1;
    ret = HdfDump(fd, vcr1);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    int32_t illegalFd = -1;
    std::vector<std::u16string> illegalArgs = {};
    ret = HdfDump(illegalFd, illegalArgs);
    ASSERT_TRUE(ret != HDF_SUCCESS);
}

HWTEST_F(DevMgrTest, HdfUtilsTest, TestSize.Level1)
{
    std::string CONFIGPATH = "/system/etc/hdfconfig/default";
    std::string CONFIGPATH1 = "/system/etc/hdfconfig/default.hcb";
    SetHcsBlobPath(CONFIGPATH.c_str());
    const struct DeviceResourceNode *node = HcsGetRootNode();
    ASSERT_TRUE(node == nullptr);
    SetHcsBlobPath(CONFIGPATH1.c_str());
    const struct DeviceResourceNode *node1 = HcsGetRootNode();
    ASSERT_TRUE(node1 == nullptr);
}
} // namespace OHOS

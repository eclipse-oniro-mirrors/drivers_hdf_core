/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <cstring>

#include "hdf_device_object.h"
#include "hdf_device_desc.h"
#include "hdf_pm.h"

extern "C" int HdfDeviceObjectRemoveService(struct HdfDeviceObject *dev);

#include "hdf_base.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include "securec.h"

#ifndef SERVICE_INFO_LEN_MAX
#define SERVICE_INFO_LEN_MAX 256
#endif

namespace OHOS {
using namespace testing::ext;

class HdfDeviceObjectTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSetClassNullTest, TestSize.Level1)
{
    bool ret = HdfDeviceSetClass(nullptr, DEVICE_CLASS_DEFAULT);
    ASSERT_FALSE(ret);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSetClassInvalidClassTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    bool ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_MAX);
    ASSERT_FALSE(ret);
    ret = HdfDeviceSetClass(&dev, (DeviceClass)(DEVICE_CLASS_MAX + 1));
    ASSERT_FALSE(ret);
    ret = HdfDeviceSetClass(&dev, (DeviceClass)0xFFFF);
    ASSERT_FALSE(ret);
    ret = HdfDeviceSetClass(&dev, (DeviceClass)0x10000);
    ASSERT_FALSE(ret);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSetClassValidTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    bool ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_DEFAULT);
    ASSERT_TRUE(ret);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_DEFAULT);
    ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_AUDIO);
    ASSERT_TRUE(ret);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_AUDIO);
    ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_DISPLAY);
    ASSERT_TRUE(ret);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_DISPLAY);
    ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_INPUT);
    ASSERT_TRUE(ret);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_INPUT);
    ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_USB);
    ASSERT_TRUE(ret);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_USB);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectConstructNullTest, TestSize.Level1)
{
    HdfDeviceObjectConstruct(nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectConstructValidTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0xFF, sizeof(dev));
    HdfDeviceObjectConstruct(&dev);
    ASSERT_EQ(dev.service, nullptr);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_DEFAULT);
#ifdef LOSCFG_DRIVERS_HDF_CONFIG_MACRO
    ASSERT_EQ(dev.deviceMatchAttr, nullptr);
#else
    ASSERT_EQ(dev.property, nullptr);
#endif
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectConstructRepeatedTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0xFF, sizeof(dev));
    HdfDeviceObjectConstruct(&dev);
    ASSERT_EQ(dev.service, nullptr);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_DEFAULT);
    dev.service = (struct IDeviceIoService *)0x1234;
    dev.deviceClass = DEVICE_CLASS_USB;
    HdfDeviceObjectConstruct(&dev);
    ASSERT_EQ(dev.service, nullptr);
    ASSERT_EQ(dev.deviceClass, DEVICE_CLASS_DEFAULT);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectConstructMultipleTest, TestSize.Level1)
{
    struct HdfDeviceObject dev1;
    struct HdfDeviceObject dev2;
    struct HdfDeviceObject dev3;
    (void)memset_s(&dev1, sizeof(dev1), 0xFF, sizeof(dev1));
    (void)memset_s(&dev2, sizeof(dev2), 0xFF, sizeof(dev2));
    (void)memset_s(&dev3, sizeof(dev3), 0xFF, sizeof(dev3));
    HdfDeviceObjectConstruct(&dev1);
    HdfDeviceObjectConstruct(&dev2);
    HdfDeviceObjectConstruct(&dev3);
    ASSERT_EQ(dev1.service, nullptr);
    ASSERT_EQ(dev2.service, nullptr);
    ASSERT_EQ(dev3.service, nullptr);
    ASSERT_EQ(dev1.deviceClass, DEVICE_CLASS_DEFAULT);
    ASSERT_EQ(dev2.deviceClass, DEVICE_CLASS_DEFAULT);
    ASSERT_EQ(dev3.deviceClass, DEVICE_CLASS_DEFAULT);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectReleaseNullTest, TestSize.Level1)
{
    HdfDeviceObjectRelease(nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectAllocNullParentTest, TestSize.Level1)
{
    struct HdfDeviceObject *dev = HdfDeviceObjectAlloc(nullptr, "test_driver");
    ASSERT_EQ(dev, nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmAcquireDeviceNullTest, TestSize.Level1)
{
    HdfPmAcquireDevice(nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmReleaseDeviceNullTest, TestSize.Level1)
{
    HdfPmReleaseDevice(nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmAcquireDeviceAsyncNullTest, TestSize.Level1)
{
    HdfPmAcquireDeviceAsync(nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmReleaseDeviceAsyncNullTest, TestSize.Level1)
{
    HdfPmReleaseDeviceAsync(nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmSetModeNullTest, TestSize.Level1)
{
    HdfPmSetMode(nullptr, HDF_POWER_SYS_CTRL);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmSetModeInvalidModeTest, TestSize.Level1)
{
    HdfPmSetMode(nullptr, HDF_POWER_MODE_MAX + 1);
    HdfPmSetMode(nullptr, (uint32_t)0xFFFF);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmRegisterPowerListenerNullTest, TestSize.Level1)
{
    struct IPowerEventListener listener;
    (void)memset_s(&listener, sizeof(listener), 0, sizeof(listener));
    int32_t ret = HdfPmRegisterPowerListener(nullptr, &listener);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfPmUnregisterPowerListenerNullTest, TestSize.Level1)
{
    struct IPowerEventListener listener;
    (void)memset_s(&listener, sizeof(listener), 0, sizeof(listener));
    HdfPmUnregisterPowerListener(nullptr, &listener);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSubscribeServiceNullDevTest, TestSize.Level1)
{
    struct SubscriberCallback cb;
    (void)memset_s(&cb, sizeof(cb), 0, sizeof(cb));
    int32_t ret = HdfDeviceSubscribeService(nullptr, "test_service", cb);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSubscribeServiceNullNameTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    struct SubscriberCallback cb;
    (void)memset_s(&cb, sizeof(cb), 0, sizeof(cb));
    int32_t ret = HdfDeviceSubscribeService(&dev, nullptr, cb);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSubscribeServiceNullAllTest, TestSize.Level1)
{
    struct SubscriberCallback cb = {0};
    int32_t ret = HdfDeviceSubscribeService(nullptr, nullptr, cb);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceGetServiceNameNullTest, TestSize.Level1)
{
    const char *name = HdfDeviceGetServiceName(nullptr);
    ASSERT_EQ(name, nullptr);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectRegisterNullTest, TestSize.Level1)
{
    int ret = HdfDeviceObjectRegister(nullptr);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectUnRegisterNullTest, TestSize.Level1)
{
    int ret = HdfDeviceObjectUnRegister(nullptr);
    ASSERT_EQ(ret, HDF_ERR_INVALID_OBJECT);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectPublishServiceNullDevTest, TestSize.Level1)
{
    int ret = HdfDeviceObjectPublishService(nullptr, "test_svc", SERVICE_POLICY_CAPACITY, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectPublishServiceNullNameTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    int ret = HdfDeviceObjectPublishService(&dev, nullptr, SERVICE_POLICY_CAPACITY, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectPublishServiceInvalidPolicyTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    int ret = HdfDeviceObjectPublishService(&dev, "test_svc", SERVICE_POLICY_NONE, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HdfDeviceObjectPublishService(&dev, "test_svc", SERVICE_POLICY_INVALID, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectSetServInfoTooLongTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    char longInfo[SERVICE_INFO_LEN_MAX + 50];
    (void)memset_s(longInfo, sizeof(longInfo), 'A', sizeof(longInfo));
    longInfo[sizeof(longInfo) - 1] = '\0';
    int ret = HdfDeviceObjectSetServInfo(&dev, longInfo);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectUpdateNullTest, TestSize.Level1)
{
    int ret = HdfDeviceObjectUpdate(nullptr);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectSetInterfaceDescNullDevTest, TestSize.Level1)
{
    int ret = HdfDeviceObjectSetInterfaceDesc(nullptr, "desc");
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectSetInterfaceDescNullDescTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    int ret = HdfDeviceObjectSetInterfaceDesc(&dev, nullptr);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectCheckInterfaceDescNullTest, TestSize.Level1)
{
    bool ret = HdfDeviceObjectCheckInterfaceDesc(nullptr, nullptr);
    ASSERT_FALSE(ret);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSetClassAllClassesTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    DeviceClass classes[] = {
        DEVICE_CLASS_DEFAULT,
        DEVICE_CLASS_PLAT,
        DEVICE_CLASS_SENSOR,
        DEVICE_CLASS_INPUT,
        DEVICE_CLASS_DISPLAY,
        DEVICE_CLASS_AUDIO,
        DEVICE_CLASS_CAMERA,
        DEVICE_CLASS_USB,
        DEVICE_CLASS_USERAUTH,
        DEVICE_CLASS_HIMEDIACOMM,
    };
    for (size_t i = 0; i < sizeof(classes) / sizeof(classes[0]); i++) {
        bool ret = HdfDeviceSetClass(&dev, classes[i]);
        ASSERT_TRUE(ret) << "Failed for class index " << i;
        ASSERT_EQ(dev.deviceClass, classes[i]);
    }
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceSetClassBoundaryTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    bool ret = HdfDeviceSetClass(&dev, (DeviceClass)0);
    ASSERT_TRUE(ret);
    ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_HIMEDIACOMM);
    ASSERT_TRUE(ret);
    ret = HdfDeviceSetClass(&dev, (DeviceClass)(DEVICE_CLASS_MAX - 1));
    ASSERT_TRUE(ret);
    ret = HdfDeviceSetClass(&dev, DEVICE_CLASS_MAX);
    ASSERT_FALSE(ret);
    ret = HdfDeviceSetClass(&dev, (DeviceClass)(DEVICE_CLASS_MAX + 1));
    ASSERT_FALSE(ret);
    ret = HdfDeviceSetClass(&dev, (DeviceClass)0xFFFF);
    ASSERT_FALSE(ret);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectPublishServiceAllPoliciesNullDevTest, TestSize.Level1)
{
    int ret = HdfDeviceObjectPublishService(nullptr, "svc", SERVICE_POLICY_NONE, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(nullptr, "svc", SERVICE_POLICY_PUBLIC, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(nullptr, "svc", SERVICE_POLICY_CAPACITY, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(nullptr, "svc", SERVICE_POLICY_FRIENDLY, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(nullptr, "svc", SERVICE_POLICY_PRIVATE, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(nullptr, "svc", SERVICE_POLICY_INVALID, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfDeviceObjectTest, HdfDeviceObjectPublishServiceAllPoliciesNullNameTest, TestSize.Level1)
{
    struct HdfDeviceObject dev;
    (void)memset_s(&dev, sizeof(dev), 0, sizeof(dev));
    int ret = HdfDeviceObjectPublishService(&dev, nullptr, SERVICE_POLICY_NONE, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(&dev, nullptr, SERVICE_POLICY_PUBLIC, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(&dev, nullptr, SERVICE_POLICY_CAPACITY, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(&dev, nullptr, SERVICE_POLICY_FRIENDLY, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(&dev, nullptr, SERVICE_POLICY_PRIVATE, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ret = HdfDeviceObjectPublishService(&dev, nullptr, SERVICE_POLICY_INVALID, 0);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
}
} // namespace OHOS

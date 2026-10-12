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
#include <string>

#include "hdi_support.h"
#include "hdf_base.h"
#include "hdf_core_log.h"

namespace OHOS {
using namespace testing::ext;

static const char *HDI_TEST_INVALID_DESC = "invalid_descriptor";
static const char *HDI_TEST_VALID_DESC = "ohos.hdi.sample.v1_0.IFoo";
static const char *HDI_TEST_VALID_SERVICE = "sample_service";
static const char *HDI_TEST_EMPTY_DESC = "";
static const char *HDI_TEST_EMPTY_SERVICE = "";
static const char *HDI_TEST_NO_VERSION_DESC = "ohos.hdi.sample.IFoo";
static const char *HDI_TEST_BAD_VERSION_DESC = "ohos.hdi.sample.vX_Y.IFoo";
static const char *HDI_TEST_MINIMAL_DESC = "a.v1_0.IB";
static const char *HDI_TEST_LONG_DESC = "ohos.hdi.aaa.bbb.ccc.ddd.v1_2.IClassName";
static const char *HDI_TEST_LONG_SERVICE = "long_service_name_for_test";
static const char *HDI_TEST_DESC_NO_NAME = "ohos.hdi.v1_0.";
static const char *HDI_TEST_DESC_DOUBLE_DOT = "ohos..hdi.v1_0.IFoo";
static const char *HDI_TEST_DESC_UPPER_VERSION = "ohos.hdi.sample.V1_0.IFoo";
static const char *HDI_TEST_DESC_ZERO_MINOR = "ohos.hdi.sample.v1_0.IFoo";
static const char *HDI_TEST_DESC_LARGE_VERSION = "ohos.hdi.sample.v999_888.IFoo";

class HdiSupportTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(HdiSupportTest, LoadHdiImplNullDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(nullptr, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNullServiceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, nullptr);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNullAllTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(nullptr, nullptr);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplEmptyDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_EMPTY_DESC, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplEmptyServiceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_EMPTY_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplInvalidDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_INVALID_DESC, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNoVersionDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_NO_VERSION_DESC, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplBadVersionDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_BAD_VERSION_DESC, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplDescNoNameTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_DESC_NO_NAME, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplDescDoubleDotTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_DESC_DOUBLE_DOT, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplUpperVersionTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_DESC_UPPER_VERSION, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplValidDescNonexistentLibTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplMinimalDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_MINIMAL_DESC, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplLongDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_LONG_DESC, HDI_TEST_LONG_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplLargeVersionTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_DESC_LARGE_VERSION, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplZeroMinorTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_DESC_ZERO_MINOR, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplNullDescTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0x1234);
    UnloadHdiImpl(nullptr, HDI_TEST_VALID_SERVICE, fakeImpl);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplNullImplTest, TestSize.Level1)
{
    UnloadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE, nullptr);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplNullAllTest, TestSize.Level1)
{
    UnloadHdiImpl(nullptr, nullptr, nullptr);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplInvalidDescTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0x1234);
    UnloadHdiImpl(HDI_TEST_INVALID_DESC, HDI_TEST_VALID_SERVICE, fakeImpl);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplNoVersionDescTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0x5678);
    UnloadHdiImpl(HDI_TEST_NO_VERSION_DESC, HDI_TEST_VALID_SERVICE, fakeImpl);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplEmptyDescTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0x9ABC);
    UnloadHdiImpl(HDI_TEST_EMPTY_DESC, HDI_TEST_VALID_SERVICE, fakeImpl);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplValidDescNonexistentTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0xDEF0);
    UnloadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE, fakeImpl);
}

HWTEST_F(HdiSupportTest, LoadUnloadCombinedTest001, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE);
    ASSERT_EQ(impl, nullptr);
    UnloadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE, impl);
}

HWTEST_F(HdiSupportTest, LoadUnloadCombinedTest002, TestSize.Level1)
{
    void *impl1 = LoadHdiImpl(HDI_TEST_LONG_DESC, HDI_TEST_LONG_SERVICE);
    ASSERT_EQ(impl1, nullptr);
    void *impl2 = LoadHdiImpl(HDI_TEST_LONG_DESC, HDI_TEST_LONG_SERVICE);
    ASSERT_EQ(impl2, nullptr);
    UnloadHdiImpl(HDI_TEST_LONG_DESC, HDI_TEST_LONG_SERVICE, impl1);
    UnloadHdiImpl(HDI_TEST_LONG_DESC, HDI_TEST_LONG_SERVICE, impl2);
}

HWTEST_F(HdiSupportTest, LoadUnloadCombinedTest003, TestSize.Level1)
{
    const char *descs[] = {
        "ohos.hdi.audio.v1_0.IAudioManager",
        "ohos.hdi.camera.v1_0.ICameraHost",
        "ohos.hdi.display.v1_0.IDisplayComposer",
        "ohos.hdi.sensor.v1_0.ISensorInterface",
        "ohos.hdi.usb.v1_0.IUsbPnpNotify",
    };
    const char *services[] = {
        "audio_service",
        "camera_host",
        "display_composer",
        "sensor_interface",
        "usb_pnp_notify",
    };
    for (size_t i = 0; i < sizeof(descs) / sizeof(descs[0]); i++) {
        void *impl = LoadHdiImpl(descs[i], services[i]);
        ASSERT_EQ(impl, nullptr);
        UnloadHdiImpl(descs[i], services[i], impl);
    }
}

HWTEST_F(HdiSupportTest, LoadHdiImplMultipleCallsTest, TestSize.Level1)
{
    for (int i = 0; i < 10; i++) {
        void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE);
        ASSERT_EQ(impl, nullptr);
    }
    UnloadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplStressTest, TestSize.Level1)
{
    const char *testDescs[] = {
        "ohos.hdi.sample.v1_0.IFoo",
        "ohos.hdi.sample.v1_1.IFoo",
        "ohos.hdi.sample.v2_0.IFoo",
        "ohos.hdi.test.v1_5.ITest",
        "a.b.c.v1_0.ID",
    };
    for (size_t i = 0; i < sizeof(testDescs) / sizeof(testDescs[0]); i++) {
        void *impl = LoadHdiImpl(testDescs[i], "svc");
        ASSERT_EQ(impl, nullptr);
        UnloadHdiImpl(testDescs[i], "svc", impl);
    }
}

HWTEST_F(HdiSupportTest, UnloadHdiImplMultipleCallsTest, TestSize.Level1)
{
    for (int i = 0; i < 10; i++) {
        UnloadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_VALID_SERVICE, reinterpret_cast<void *>(0x1000));
    }
}

HWTEST_F(HdiSupportTest, LoadHdiImplSpecialCharsDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.sample.v1_0.I_Foo_Bar", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNumericServiceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, "123456");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNumericDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0.I123", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplSingleCharDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("a.v1_0.IX", "s");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplUnderscorePrefixTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("_ohos.hdi.v1_0.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplDotStartDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(".ohos.hdi.v1_0.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplDotEndDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0.IFoo.", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplVersionStartDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("v1_0.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNoInterfaceNameTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0.", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplLeadingZeroVersionTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v01_00.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNegativeVersionTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v-1_0.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplFloatVersionTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1.0.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplSpaceInDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0.I Foo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplTabInDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0.\tIFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplNewlineInDescTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0.\nIFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplVeryLongDescTest, TestSize.Level1)
{
    std::string longDesc = "ohos.hdi.";
    for (int i = 0; i < 100; i++) {
        longDesc += "verylongname.";
    }
    longDesc += "v1_0.IFoo";
    void *impl = LoadHdiImpl(longDesc.c_str(), "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplVeryLongServiceTest, TestSize.Level1)
{
    std::string longService = "svc";
    for (int i = 0; i < 200; i++) {
        longService += "_name_part";
    }
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, longService.c_str());
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplMultiDotInterfaceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0.IFoo.Bar", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplSingleVersionTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplUnderscoreVersionTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi.v1_0_1.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplHyphenInNameTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl("ohos.hdi-sample.v1_0.IFoo", "svc");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplUpperCaseServiceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, "SAMPLE_SERVICE");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplMixedCaseServiceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, "SampleService");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplDotInServiceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, "sample.service");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, LoadHdiImplSlashInServiceTest, TestSize.Level1)
{
    void *impl = LoadHdiImpl(HDI_TEST_VALID_DESC, "sample/service");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplSpecialCharsDescTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0x1234);
    UnloadHdiImpl("ohos.hdi.v1_0.I_Foo_Bar", "svc", fakeImpl);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplLongDescTest, TestSize.Level1)
{
    std::string longDesc = "ohos.hdi.";
    for (int i = 0; i < 100; i++) {
        longDesc += "verylongname.";
    }
    longDesc += "v1_0.IFoo";
    void *fakeImpl = reinterpret_cast<void *>(0x5678);
    UnloadHdiImpl(longDesc.c_str(), "svc", fakeImpl);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplValidDescNullServiceTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0x9ABC);
    UnloadHdiImpl(HDI_TEST_VALID_DESC, nullptr, fakeImpl);
}

HWTEST_F(HdiSupportTest, UnloadHdiImplEmptyServiceTest, TestSize.Level1)
{
    void *fakeImpl = reinterpret_cast<void *>(0xDEF0);
    UnloadHdiImpl(HDI_TEST_VALID_DESC, HDI_TEST_EMPTY_SERVICE, fakeImpl);
}

static void LoadHdiImplMassiveStressHelper(void)
{
    const char *descriptors[] = {
        "ohos.hdi.audio.v1_0.IAudioManager",
        "ohos.hdi.audio.v2_0.IAudioManager",
        "ohos.hdi.camera.v1_0.ICameraHost",
        "ohos.hdi.camera.v1_1.ICameraHost",
        "ohos.hdi.camera.v2_0.ICameraHost",
        "ohos.hdi.display.v1_0.IDisplayComposer",
        "ohos.hdi.display.v1_1.IDisplayComposer",
        "ohos.hdi.sensor.v1_0.ISensorInterface",
        "ohos.hdi.sensor.v2_0.ISensorInterface",
        "ohos.hdi.usb.v1_0.IUsbPnpNotify",
    };
    const char *services[] = {
        "audio_svc", "audio_svc", "camera_svc", "camera_svc", "camera_svc",
        "display_svc", "display_svc", "sensor_svc", "sensor_svc", "usb_svc",
    };
    size_t count = sizeof(descriptors) / sizeof(descriptors[0]);
    for (size_t i = 0; i < count; i++) {
        void *impl = LoadHdiImpl(descriptors[i], services[i]);
        ASSERT_EQ(impl, nullptr) << "Failed at index " << i;
        UnloadHdiImpl(descriptors[i], services[i], impl);
    }
}

HWTEST_F(HdiSupportTest, LoadHdiImplMassiveStressTest, TestSize.Level1)
{
    LoadHdiImplMassiveStressHelper();
}

} // namespace OHOS

/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include "hdf_io_service.h"
#include "vdi_sample1_driver.h"
#include "vdi_sample2_driver.h"
#include "hdf_load_vdi.h"

namespace OHOS {
using namespace testing::ext;

class HdfVdiTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfVdiTest::SetUpTestCase()
{
}

void HdfVdiTest::TearDownTestCase()
{
}

void HdfVdiTest::SetUp()
{
}

void HdfVdiTest::TearDown()
{
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleABase, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample1_driver.z.so", 1);
    ASSERT_TRUE(vdi != nullptr);
    ASSERT_TRUE(vdi->vdiBase != nullptr);

    struct VdiSampleA *sampleA = reinterpret_cast<struct VdiSampleA *>(vdi->vdiBase);
    ASSERT_TRUE(sampleA->module != nullptr);
    struct ModuleA *modA = reinterpret_cast<struct ModuleA *>(sampleA->module);
    int ret = modA->ServiceA();
    ASSERT_TRUE(ret == HDF_SUCCESS);
    ret = modA->ServiceB(modA);
    ASSERT_TRUE(ret == HDF_SUCCESS);

    HdfCloseVdi(vdi);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleAErrorSo, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample1_driver_error.z.so", 1);
    ASSERT_TRUE(vdi == nullptr);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleAErrorVersion, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample1_driver.z.so", 0);
    ASSERT_TRUE(vdi == nullptr);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleBBase, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample2_driver.z.so", 1);
    ASSERT_TRUE(vdi != nullptr);
    ASSERT_TRUE(vdi->vdiBase != nullptr);

    struct VdiSampleB *sampleB = reinterpret_cast<struct VdiSampleB *>(vdi->vdiBase);
    ASSERT_TRUE(sampleB->module != nullptr);
    VdiSample *vdiSample = reinterpret_cast<VdiSample *>(sampleB->module);
    int ret = vdiSample->ServiceA();
    ASSERT_TRUE(ret == HDF_SUCCESS);
    ret = vdiSample->ServiceB(vdiSample);
    ASSERT_TRUE(ret == HDF_SUCCESS);

    HdfCloseVdi(vdi);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleBErrorSo, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample2_driver_error.z.so", 1);
    ASSERT_TRUE(vdi == nullptr);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleBErrorVersion, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample2_driver.z.so", 0);
    ASSERT_TRUE(vdi == nullptr);
}
} // namespace OHOS

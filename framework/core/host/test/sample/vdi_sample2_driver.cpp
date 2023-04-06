/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "hdf_base.h"
#include "hdf_load_vdi.h"
#include "vdi_sample2_driver.h"

#define HDF_LOG_TAG vdi_sample2

VdiSample::VdiSample(int para)
{
    priData = para;
}

int VdiSample::ServiceA()
{
    HDF_LOGD("%{public}s", __func__);
    return HDF_SUCCESS;
}

int VdiSample::ServiceB(IVdiSample *vdi)
{
    VdiSample *sample = reinterpret_cast<VdiSample *>(vdi);
    HDF_LOGD("%{public}s %{public}d", __func__, sample->priData);
    return HDF_SUCCESS;
}

static int SampleAOpen(struct HdfVdiBase *vdiBase)
{
    HDF_LOGD("%{public}s", __func__);
    struct VdiSampleB *sampleB = reinterpret_cast<struct VdiSampleB *>(vdiBase);
    sampleB->module = new VdiSample(1);
    return HDF_SUCCESS;
}

static int SampleAClose(struct HdfVdiBase *vdiBase)
{
    HDF_LOGD("%{public}s", __func__);
    struct VdiSampleB *sampleB = reinterpret_cast<struct VdiSampleB *>(vdiBase);
    VdiSample *sample = reinterpret_cast<VdiSample *>(sampleB->module);
    delete sample;
    sampleB->module = nullptr;
    return HDF_SUCCESS;
}

static struct VdiSampleB g_sampleB = {
    .base = {
        .moduleVersion = 1,
        .moduleName = "SampleServiceB",
        .OpenVdi = SampleAOpen,
        .CloseVdi = SampleAClose,
    },
    .module = nullptr,
};

HDF_VDI_INIT(g_sampleB);

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
#include "vdi_sample1_driver.h"

#define HDF_LOG_TAG vdi_sample1

static int ServiceA()
{
    HDF_LOGD("%{public}s", __func__);
    return HDF_SUCCESS;
}

static int ServiceB(struct ModuleA *modA)
{
    HDF_LOGD("%{public}s %{public}d", __func__, modA->priData);
    return HDF_SUCCESS;
}

static int SampleAOpen(struct HdfVdiBase *vdiBase)
{
    (void)vdiBase;
    HDF_LOGD("%{public}s", __func__);
    return HDF_SUCCESS;
}

static int SampleAClose(struct HdfVdiBase *vdiBase)
{
    (void)vdiBase;
    HDF_LOGD("%{public}s", __func__);
    return HDF_SUCCESS;
}

static struct ModuleA g_modA = {
    .ServiceA = ServiceA,
    .ServiceB = ServiceB,
    .priData = 1,
};

static struct VdiSampleA g_sampleA = {
    .base = {
        .moduleVersion = 1,
        .moduleName = "SampleServiceA",
        .OpenVdi = SampleAOpen,
        .CloseVdi = SampleAClose,
    },
    .module = &g_modA,
};

HDF_VDI_INIT(g_sampleA);

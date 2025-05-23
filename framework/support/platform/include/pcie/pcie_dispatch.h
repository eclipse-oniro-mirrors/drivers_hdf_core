/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef PCIE_DISPATCH_H
#define PCIE_DISPATCH_H

#include "hdf_base.h"

#ifndef __USER__
#include "devsvc_manager_clnt.h"
#else
#include "hdf_io_service_if.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#ifndef __USER__
int32_t PcieIoDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* PCIE_DISPATCH_H */

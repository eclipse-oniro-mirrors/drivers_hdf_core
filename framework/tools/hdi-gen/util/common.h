/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_COMMON_H
#define OHOS_HDI_COMMON_H

namespace OHOS {
namespace HDI {
    constexpr const char *TAB = "    ";
    constexpr const char *TAG = "HDI-GEN";

#ifndef __MINGW32__
    constexpr char SEPARATOR = '/';
#else
    constexpr char SEPARATOR = '\\';
#endif
}
}

#endif // OHOS_HDI_COMMON_H

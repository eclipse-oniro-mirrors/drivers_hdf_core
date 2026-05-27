/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HDF_LOG_ADAPTER_H
#define HDF_LOG_ADAPTER_H

#include <stdio.h>
#include "hdf_types.h"
#include "udk/log.h"

/* NOTE: HDF_LOG_TAG is defined by each source file before including this
 * header (or via -DHDF_LOG_TAG compile flag). Do NOT define it here. */

#define HDF_FMT_TAG_CAT(TAG, LEVEL) "[" #LEVEL "/" #TAG "] "
#define HDF_FMT_TAG(TAG, LEVEL) HDF_FMT_TAG_CAT(TAG, LEVEL)
#define HDF_LOG_BUF_SIZE 256

bool DealFormat(const char *fmt, char *dest, size_t size);

#define HDF_LOG_WRAPPER(level, fmt, args...) \
    do { \
        char tmp_fmt[HDF_LOG_BUF_SIZE] = {0}; \
        if (DealFormat(fmt, tmp_fmt, sizeof(tmp_fmt))) \
            udk_log(level, tmp_fmt, ##args); \
    } while (0)

#define HDF_LOGV_WRAPPER(fmt, args...) HDF_LOG_WRAPPER(UDKLOG_DEBUG, HDF_FMT_TAG(HDF_LOG_TAG, V) fmt "\n", ## args)
#define HDF_LOGD_WRAPPER(fmt, args...) HDF_LOG_WRAPPER(UDKLOG_DEBUG, HDF_FMT_TAG(HDF_LOG_TAG, D) fmt "\n", ## args)
#define HDF_LOGI_WRAPPER(fmt, args...) HDF_LOG_WRAPPER(UDKLOG_INFO, HDF_FMT_TAG(HDF_LOG_TAG, I) fmt "\n", ## args)
#define HDF_LOGW_WRAPPER(fmt, args...) HDF_LOG_WRAPPER(UDKLOG_WARN, HDF_FMT_TAG(HDF_LOG_TAG, W) fmt "\n", ## args)
#define HDF_LOGE_WRAPPER(fmt, args...) HDF_LOG_WRAPPER(UDKLOG_ERROR, HDF_FMT_TAG(HDF_LOG_TAG, E) fmt "\n", ## args)

#endif /* HDF_LOG_ADAPTER_H */
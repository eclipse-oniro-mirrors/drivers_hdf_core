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

#include "hdf_types.h"
#include "udk/log.h"

#define NUMBER 2
static const char *g_property[NUMBER] = { "%{private}", "%{public}" };
static size_t g_property_len[NUMBER] = { 10, 9 };

bool DealFormat(const char *fmt, char *dest, size_t size)
{
    const char *ptr = fmt;
    const char *ptrCur = ptr;
    errno_t ret;
    size_t index = 0;
    size_t i;

    if (fmt == NULL || dest == NULL || size == 0 || strlen(fmt) >= (size - 1)) {
        udk_error("%s invalid para", __func__);
        return false;
    }

    while (ptrCur != NULL && *ptrCur != '\0') {
        if (*ptrCur != '%') {
            ptrCur++;
            continue;
        }
        for (i = 0; i < NUMBER; i++) {
            if (strncmp(ptrCur, g_property[i], g_property_len[i]) != 0) {
                continue;
            }

            /* add 1 is for to copy char '%' */
            ret = strncpy_s(&dest[index], size - index, ptr, ptrCur - ptr + 1);
            if (ret != EOK) {
                udk_error("%s strncpy_s error %d", __func__, ret);
                return false;
            }
            index += (ptrCur - ptr + 1);
            ptr = ptrCur + g_property_len[i];
            ptrCur = ptr;
            break;
        }
        if (i == NUMBER) {
            ptrCur++;
        }
    }
    ret = strcat_s(&dest[index], size - index, ptr);
    if (ret != EOK) {
        udk_error("%s strcat_s error %d", __func__, ret);
        return false;
    }

    return true;
}
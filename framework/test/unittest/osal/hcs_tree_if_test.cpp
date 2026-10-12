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
#include <cstdint>
#include <string>

#include "hcs_tree_if.h"
#include "hcs_blob_if.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include "securec.h"

namespace OHOS {
using namespace testing::ext;

#define HCS_TEST_ATTR_NAME_BOOL    "flag"
#define HCS_TEST_ATTR_NAME_U8      "u8Val"
#define HCS_TEST_ATTR_NAME_U16     "u16Val"
#define HCS_TEST_ATTR_NAME_U32     "u32Val"
#define HCS_TEST_ATTR_NAME_U64     "u64Val"
#define HCS_TEST_ATTR_NAME_STR     "strVal"
#define HCS_TEST_ATTR_NAME_ARR     "arrVal"
#define HCS_TEST_ATTR_NAME_MISSING "nonexistent"
#define HCS_TEST_STRING_VALUE      "hello_hdf"
#define HCS_TEST_U8_VALUE          42
#define HCS_TEST_U16_VALUE         1000
#define HCS_TEST_U32_VALUE         0x12345678
#define HCS_TEST_U64_VALUE         0x123456789ABCDEF0ULL
#define HCS_TEST_ARRAY_COUNT       3
#define HCS_TEST_ARRAY_ELEM0       10
#define HCS_TEST_ARRAY_ELEM1       20
#define HCS_TEST_ARRAY_ELEM2       30
#define HCS_VAL_UINT8_SIZE          1
#define HCS_VAL_UINT16_SIZE         2
#define HCS_VAL_UINT32_SIZE         4
#define HCS_VAL_UINT64_SIZE         8
#define HCS_BITS_PER_BYTE            8
#define HCS_BYTE_MASK                0xFF
#define HCS_SHIFT_1BYTE              (HCS_VAL_UINT8_SIZE * HCS_BITS_PER_BYTE)
#define HCS_SHIFT_2BYTE              (HCS_VAL_UINT16_SIZE * HCS_BITS_PER_BYTE)
#define HCS_SHIFT_3BYTE              ((HCS_VAL_UINT32_SIZE - 1) * HCS_BITS_PER_BYTE)
#define HCS_NULL_TERM_SIZE           1
#define HCS_PREFIX_AND_U8_LEN       (HCS_PREFIX_LENGTH + HCS_VAL_UINT8_SIZE)
#define HCS_PREFIX_AND_U16_LEN     (HCS_PREFIX_LENGTH + HCS_VAL_UINT16_SIZE)
#define HCS_PREFIX_AND_U32_LEN     (HCS_PREFIX_LENGTH + HCS_VAL_UINT32_SIZE)
#define HCS_PREFIX_AND_U64_LEN     (HCS_PREFIX_LENGTH + HCS_VAL_UINT64_SIZE)
#define HCS_STRING_PREFIX_AND_TERM  (HCS_PREFIX_LENGTH + 1)
#define HCS_ARRAY_HEADER_SIZE      (HCS_PREFIX_LENGTH + HCS_VAL_UINT16_SIZE)

struct HcsTestAttr {
    struct DeviceResourceAttr attr;
    char nameBuf[32];
    char valueBuf[64];
};

static void HcsTestInitAttr(struct HcsTestAttr *ta, const char *name, const char *valueData, size_t valueLen)
{
    (void)memset_s(ta, sizeof(*ta), 0, sizeof(*ta));
    (void)snprintf_s(ta->nameBuf, sizeof(ta->nameBuf), sizeof(ta->nameBuf) - 1, "%s", name);
    if (valueData != nullptr && valueLen > 0 && valueLen <= sizeof(ta->valueBuf)) {
        (void)memcpy_s(ta->valueBuf, sizeof(ta->valueBuf), valueData, valueLen);
    }
    ta->attr.name = ta->nameBuf;
    ta->attr.value = ta->valueBuf;
    ta->attr.next = nullptr;
}

static void HcsTestMakeUint8Value(char *buf, size_t bufLen, uint8_t val)
{
    (void)memset_s(buf, bufLen, 0, bufLen);
    buf[0] = static_cast<char>(CONFIG_BYTE);
    buf[1] = static_cast<char>(val);
}

static void HcsTestMakeUint16Value(char *buf, size_t bufLen, uint16_t val)
{
    (void)memset_s(buf, bufLen, 0, bufLen);
    buf[0] = static_cast<char>(CONFIG_WORD);
    buf[1] = static_cast<char>(val & HCS_BYTE_MASK);
    buf[HCS_VAL_UINT16_SIZE] = static_cast<char>((val >> HCS_SHIFT_1BYTE) & HCS_BYTE_MASK);
}

static void HcsTestMakeUint32Value(char *buf, size_t bufLen, uint32_t val)
{
    (void)memset_s(buf, bufLen, 0, bufLen);
    buf[0] = static_cast<char>(CONFIG_DWORD);
    buf[1] = static_cast<char>(val & HCS_BYTE_MASK);
    buf[HCS_VAL_UINT16_SIZE] = static_cast<char>((val >> HCS_SHIFT_1BYTE) & HCS_BYTE_MASK);
    buf[HCS_VAL_UINT32_SIZE - 1] = static_cast<char>((val >> HCS_SHIFT_2BYTE) & HCS_BYTE_MASK);
    buf[HCS_VAL_UINT32_SIZE] = static_cast<char>((val >> HCS_SHIFT_3BYTE) & HCS_BYTE_MASK);
}

static void HcsTestMakeUint64Value(char *buf, size_t bufLen, uint64_t val)
{
    (void)memset_s(buf, bufLen, 0, bufLen);
    buf[0] = static_cast<char>(CONFIG_QWORD);
    for (size_t i = 0; i < HCS_VAL_UINT64_SIZE; i++) {
        buf[i + 1] = static_cast<char>((val >> (i * HCS_BITS_PER_BYTE)) & HCS_BYTE_MASK);
    }
}

static void HcsTestMakeStringValue(char *buf, size_t bufLen, const char *str)
{
    (void)memset_s(buf, bufLen, 0, bufLen);
    buf[0] = static_cast<char>(CONFIG_STRING);
    size_t len = strlen(str);
    if (len + HCS_STRING_PREFIX_AND_TERM > bufLen) {
        len = bufLen - HCS_STRING_PREFIX_AND_TERM;
    }
    if (memcpy_s(buf + 1, bufLen - 1, str, len) != EOK) {
        return;
    }
}

static void HcsTestMakeUint8ArrayValue(char *buf, size_t bufLen, const uint8_t *vals, uint16_t count)
{
    (void)memset_s(buf, bufLen, 0, bufLen);
    size_t offset = 0;
    buf[offset++] = static_cast<char>(CONFIG_ARRAY);
    buf[offset++] = static_cast<char>(count & HCS_BYTE_MASK);
    buf[offset++] = static_cast<char>((count >> HCS_SHIFT_1BYTE) & HCS_BYTE_MASK);
    for (uint16_t i = 0; i < count && offset + HCS_PREFIX_AND_U8_LEN < bufLen; i++) {
        buf[offset++] = static_cast<char>(CONFIG_BYTE);
        buf[offset++] = static_cast<char>(vals[i]);
    }
}

static void HcsTestMakeStringArrayValue(char *buf, size_t bufLen, const char **strs, uint16_t count)
{
    (void)memset_s(buf, bufLen, 0, bufLen);
    size_t offset = 0;
    buf[offset++] = static_cast<char>(CONFIG_ARRAY);
    buf[offset++] = static_cast<char>(count & HCS_BYTE_MASK);
    buf[offset++] = static_cast<char>((count >> HCS_SHIFT_1BYTE) & HCS_BYTE_MASK);
    for (uint16_t i = 0; i < count; i++) {
        buf[offset++] = static_cast<char>(CONFIG_STRING);
        size_t slen = strlen(strs[i]);
        if (offset + slen + HCS_NULL_TERM_SIZE > bufLen) {
            break;
        }
        if (memcpy_s(buf + offset, bufLen - offset, strs[i], slen) != EOK) {
            return;
        }
        offset += slen;
        buf[offset++] = '\0';
    }
}

static void HcsTestMakeRefValue(char *buf, size_t bufLen, uint32_t refHash)
{
    HcsTestMakeUint32Value(buf, bufLen, refHash);
    buf[0] = static_cast<char>(CONFIG_REFERENCE);
}

static struct HcsTestAttr g_boolAttr;
static struct HcsTestAttr g_u8Attr;
static struct HcsTestAttr g_u16Attr;
static struct HcsTestAttr g_u32Attr;
static struct HcsTestAttr g_u64Attr;
static struct HcsTestAttr g_strAttr;
static struct HcsTestAttr g_arrAttr;
static struct DeviceResourceNode g_testNode;

static void HcsTestBuildNode(void)
{
    (void)memset_s(&g_testNode, sizeof(g_testNode), 0, sizeof(g_testNode));
    g_testNode.name = "test_node";

    char vbuf[64];
    HcsTestMakeUint8Value(vbuf, sizeof(vbuf), 1);
    HcsTestInitAttr(&g_boolAttr, HCS_TEST_ATTR_NAME_BOOL, vbuf, HCS_PREFIX_AND_U8_LEN);

    HcsTestMakeUint8Value(vbuf, sizeof(vbuf), HCS_TEST_U8_VALUE);
    HcsTestInitAttr(&g_u8Attr, HCS_TEST_ATTR_NAME_U8, vbuf, HCS_PREFIX_AND_U8_LEN);

    HcsTestMakeUint16Value(vbuf, sizeof(vbuf), HCS_TEST_U16_VALUE);
    HcsTestInitAttr(&g_u16Attr, HCS_TEST_ATTR_NAME_U16, vbuf, HCS_PREFIX_AND_U16_LEN);

    HcsTestMakeUint32Value(vbuf, sizeof(vbuf), HCS_TEST_U32_VALUE);
    HcsTestInitAttr(&g_u32Attr, HCS_TEST_ATTR_NAME_U32, vbuf, HCS_PREFIX_AND_U32_LEN);

    HcsTestMakeUint64Value(vbuf, sizeof(vbuf), HCS_TEST_U64_VALUE);
    HcsTestInitAttr(&g_u64Attr, HCS_TEST_ATTR_NAME_U64, vbuf, HCS_PREFIX_AND_U64_LEN);

    HcsTestMakeStringValue(vbuf, sizeof(vbuf), HCS_TEST_STRING_VALUE);
    size_t strLen = strlen(HCS_TEST_STRING_VALUE) + HCS_STRING_PREFIX_AND_TERM;
    HcsTestInitAttr(&g_strAttr, HCS_TEST_ATTR_NAME_STR, vbuf, strLen);

    uint8_t arrVals[HCS_TEST_ARRAY_COUNT] = {HCS_TEST_ARRAY_ELEM0, HCS_TEST_ARRAY_ELEM1, HCS_TEST_ARRAY_ELEM2};
    HcsTestMakeUint8ArrayValue(vbuf, sizeof(vbuf), arrVals, HCS_TEST_ARRAY_COUNT);
    size_t arrLen = HCS_ARRAY_HEADER_SIZE + HCS_TEST_ARRAY_COUNT * HCS_PREFIX_AND_U8_LEN;
    HcsTestInitAttr(&g_arrAttr, HCS_TEST_ATTR_NAME_ARR, vbuf, arrLen);

    g_boolAttr.attr.next = &g_u8Attr.attr;
    g_u8Attr.attr.next = &g_u16Attr.attr;
    g_u16Attr.attr.next = &g_u32Attr.attr;
    g_u32Attr.attr.next = &g_u64Attr.attr;
    g_u64Attr.attr.next = &g_strAttr.attr;
    g_strAttr.attr.next = &g_arrAttr.attr;
    g_arrAttr.attr.next = nullptr;

    g_testNode.attrData = &g_boolAttr.attr;
}

class HcsTreeIfTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HcsTestBuildNode();
    }
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(HcsTreeIfTest, HcsGetBoolTest001, TestSize.Level1)
{
    bool result = HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_BOOL);
    ASSERT_TRUE(result);
}

HWTEST_F(HcsTreeIfTest, HcsGetBoolTest002, TestSize.Level1)
{
    bool result = HcsGetBool(nullptr, HCS_TEST_ATTR_NAME_BOOL);
    ASSERT_FALSE(result);
    result = HcsGetBool(&g_testNode, nullptr);
    ASSERT_FALSE(result);
    result = HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_MISSING);
    ASSERT_FALSE(result);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8Test001, TestSize.Level1)
{
    uint8_t value = 0;
    int32_t ret = HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_U8, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, HCS_TEST_U8_VALUE);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8Test002, TestSize.Level1)
{
    uint8_t value = 99;
    int32_t ret = HcsGetUint8(nullptr, HCS_TEST_ATTR_NAME_U8, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8(&g_testNode, nullptr, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_U8, nullptr, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16Test001, TestSize.Level1)
{
    uint16_t value = 0;
    int32_t ret = HcsGetUint16(&g_testNode, HCS_TEST_ATTR_NAME_U16, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, HCS_TEST_U16_VALUE);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16Test002, TestSize.Level1)
{
    uint16_t value = 99;
    int32_t ret = HcsGetUint16(nullptr, HCS_TEST_ATTR_NAME_U16, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint16(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint16(&g_testNode, HCS_TEST_ATTR_NAME_U16, nullptr, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32Test001, TestSize.Level1)
{
    uint32_t value = 0;
    int32_t ret = HcsGetUint32(&g_testNode, HCS_TEST_ATTR_NAME_U32, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, HCS_TEST_U32_VALUE);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32Test002, TestSize.Level1)
{
    uint32_t value = 99;
    int32_t ret = HcsGetUint32(nullptr, HCS_TEST_ATTR_NAME_U32, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint32(&g_testNode, nullptr, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint32(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64Test001, TestSize.Level1)
{
    uint64_t value = 0;
    int32_t ret = HcsGetUint64(&g_testNode, HCS_TEST_ATTR_NAME_U64, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, HCS_TEST_U64_VALUE);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64Test002, TestSize.Level1)
{
    uint64_t value = 99;
    int32_t ret = HcsGetUint64(nullptr, HCS_TEST_ATTR_NAME_U64, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint64(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint64(&g_testNode, HCS_TEST_ATTR_NAME_U64, nullptr, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8ArrayElemTest001, TestSize.Level1)
{
    uint8_t value = 0;
    int32_t ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 0, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, HCS_TEST_ARRAY_ELEM0);
    ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 1, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, HCS_TEST_ARRAY_ELEM1);
    ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 2, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, HCS_TEST_ARRAY_ELEM2);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8ArrayElemTest002, TestSize.Level1)
{
    uint8_t value = 99;
    int32_t ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 100, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8ArrayElem(nullptr, HCS_TEST_ATTR_NAME_ARR, 0, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8ArrayElem(&g_testNode, nullptr, 0, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, 0, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 0, nullptr, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16ArrayElemTest001, TestSize.Level1)
{
    uint16_t value = 0;
    int32_t ret = HcsGetUint16ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 0, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, static_cast<uint16_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16ArrayElemTest002, TestSize.Level1)
{
    uint16_t value = 99;
    int32_t ret = HcsGetUint16ArrayElem(nullptr, HCS_TEST_ATTR_NAME_ARR, 0, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint16ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 100, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32ArrayElemTest001, TestSize.Level1)
{
    uint32_t value = 0;
    int32_t ret = HcsGetUint32ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 0, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, static_cast<uint32_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32ArrayElemTest002, TestSize.Level1)
{
    uint32_t value = 99;
    int32_t ret = HcsGetUint32ArrayElem(nullptr, HCS_TEST_ATTR_NAME_ARR, 0, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint32ArrayElem(&g_testNode, nullptr, 0, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64ArrayElemTest001, TestSize.Level1)
{
    uint64_t value = 0;
    int32_t ret = HcsGetUint64ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 0, &value, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(value, static_cast<uint64_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64ArrayElemTest002, TestSize.Level1)
{
    uint64_t value = 99;
    int32_t ret = HcsGetUint64ArrayElem(nullptr, HCS_TEST_ATTR_NAME_ARR, 0, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint64ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 100, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8ArrayTest001, TestSize.Level1)
{
    uint8_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], HCS_TEST_ARRAY_ELEM0);
    ASSERT_EQ(values[1], HCS_TEST_ARRAY_ELEM1);
    ASSERT_EQ(values[2], HCS_TEST_ARRAY_ELEM2);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8ArrayTest002, TestSize.Level1)
{
    uint8_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint8Array(nullptr, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, nullptr, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 0, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16ArrayTest001, TestSize.Level1)
{
    uint16_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint16Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], static_cast<uint16_t>(HCS_TEST_ARRAY_ELEM0));
    ASSERT_EQ(values[1], static_cast<uint16_t>(HCS_TEST_ARRAY_ELEM1));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16ArrayTest002, TestSize.Level1)
{
    uint16_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint16Array(nullptr, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint16Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, nullptr, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint16Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 0, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32ArrayTest001, TestSize.Level1)
{
    uint32_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint32Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], static_cast<uint32_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32ArrayTest002, TestSize.Level1)
{
    uint32_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint32Array(nullptr, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint32Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, nullptr, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint32Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 0, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64ArrayTest001, TestSize.Level1)
{
    uint64_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint64Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], static_cast<uint64_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64ArrayTest002, TestSize.Level1)
{
    uint64_t values[HCS_TEST_ARRAY_COUNT] = {0};
    int32_t ret = HcsGetUint64Array(nullptr, HCS_TEST_ATTR_NAME_ARR, values, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint64Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, nullptr, HCS_TEST_ARRAY_COUNT, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint64Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 0, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetStringTest001, TestSize.Level1)
{
    const char *value = nullptr;
    int32_t ret = HcsGetString(&g_testNode, HCS_TEST_ATTR_NAME_STR, &value, "default");
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_NE(value, nullptr);
    ASSERT_STREQ(value, HCS_TEST_STRING_VALUE);
}

HWTEST_F(HcsTreeIfTest, HcsGetStringTest002, TestSize.Level1)
{
    const char *value = nullptr;
    int32_t ret = HcsGetString(nullptr, HCS_TEST_ATTR_NAME_STR, &value, "default");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "default");
    ret = HcsGetString(&g_testNode, nullptr, &value, "default");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "default");
    ret = HcsGetString(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, &value, "default");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "default");
    ret = HcsGetString(&g_testNode, HCS_TEST_ATTR_NAME_STR, nullptr, "default");
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetStringArrayElemTest001, TestSize.Level1)
{
    struct HcsTestAttr strArrAttr;
    char vbuf[64];
    const char *strs[2] = {"aaa", "bbb"};
    HcsTestMakeStringArrayValue(vbuf, sizeof(vbuf), strs, 2);
    HcsTestInitAttr(&strArrAttr, "strArr", vbuf, strlen(vbuf));
    strArrAttr.attr.next = g_testNode.attrData;
    g_testNode.attrData = &strArrAttr.attr;

    const char *value = nullptr;
    int32_t ret = HcsGetStringArrayElem(&g_testNode, "strArr", 0, &value, "def");
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "aaa");
    ret = HcsGetStringArrayElem(&g_testNode, "strArr", 1, &value, "def");
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "bbb");

    g_testNode.attrData = strArrAttr.attr.next;
}

HWTEST_F(HcsTreeIfTest, HcsGetStringArrayElemTest002, TestSize.Level1)
{
    const char *value = nullptr;
    int32_t ret = HcsGetStringArrayElem(nullptr, "strArr", 0, &value, "def");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "def");
    ret = HcsGetStringArrayElem(&g_testNode, nullptr, 0, &value, "def");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "def");
    ret = HcsGetStringArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_MISSING, 0, &value, "def");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "def");
    ret = HcsGetStringArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, 0, &value, "def");
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetElemNumTest001, TestSize.Level1)
{
    int32_t num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_ARR);
    ASSERT_EQ(num, HCS_TEST_ARRAY_COUNT);
}

HWTEST_F(HcsTreeIfTest, HcsGetElemNumTest002, TestSize.Level1)
{
    int32_t num = HcsGetElemNum(nullptr, HCS_TEST_ATTR_NAME_ARR);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, nullptr);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_MISSING);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_STR);
    ASSERT_EQ(num, HDF_FAILURE);
}

HWTEST_F(HcsTreeIfTest, HcsGetChildNodeTest001, TestSize.Level1)
{
    struct DeviceResourceNode child1;
    struct DeviceResourceNode child2;
    (void)memset_s(&child1, sizeof(child1), 0, sizeof(child1));
    (void)memset_s(&child2, sizeof(child2), 0, sizeof(child2));
    child1.name = "child1";
    child2.name = "child2";
    child1.sibling = &child2;
    child2.sibling = nullptr;
    g_testNode.child = &child1;

    const struct DeviceResourceNode *result = HcsGetChildNode(&g_testNode, "child1");
    ASSERT_EQ(result, &child1);
    result = HcsGetChildNode(&g_testNode, "child2");
    ASSERT_EQ(result, &child2);

    g_testNode.child = nullptr;
}

HWTEST_F(HcsTreeIfTest, HcsGetChildNodeTest002, TestSize.Level1)
{
    const struct DeviceResourceNode *result = HcsGetChildNode(nullptr, "child1");
    ASSERT_EQ(result, nullptr);
    result = HcsGetChildNode(&g_testNode, nullptr);
    ASSERT_EQ(result, nullptr);
    result = HcsGetChildNode(&g_testNode, "nonexistent_child");
    ASSERT_EQ(result, nullptr);
}

HWTEST_F(HcsTreeIfTest, HcsGetNodeByMatchAttrTest001, TestSize.Level1)
{
    struct DeviceResourceNode matchNode;
    struct HcsTestAttr matchAttr;
    char vbuf[64];
    (void)memset_s(&matchNode, sizeof(matchNode), 0, sizeof(matchNode));
    matchNode.name = "match_target";
    HcsTestMakeStringValue(vbuf, sizeof(vbuf), "target_value");
    HcsTestInitAttr(&matchAttr, HCS_MATCH_ATTR, vbuf, strlen(vbuf));
    matchNode.attrData = &matchAttr.attr;
    matchNode.parent = &g_testNode;
    matchNode.child = nullptr;
    matchNode.sibling = nullptr;
    g_testNode.child = &matchNode;
    g_testNode.sibling = nullptr;

    const struct DeviceResourceNode *result = HcsGetNodeByMatchAttr(&g_testNode, "target_value");
    ASSERT_EQ(result, &matchNode);

    g_testNode.child = nullptr;
}

HWTEST_F(HcsTreeIfTest, HcsGetNodeByMatchAttrTest002, TestSize.Level1)
{
    const struct DeviceResourceNode *result = HcsGetNodeByMatchAttr(nullptr, "value");
    ASSERT_EQ(result, nullptr);
    result = HcsGetNodeByMatchAttr(&g_testNode, "nonexistent_value");
    ASSERT_EQ(result, nullptr);
    result = HcsGetNodeByMatchAttr(&g_testNode, nullptr);
    ASSERT_EQ(result, nullptr);
}

HWTEST_F(HcsTreeIfTest, HcsGetNodeByRefAttrTest001, TestSize.Level1)
{
    struct HcsTestAttr refAttr;
    char vbuf[64];
    HcsTestMakeRefValue(vbuf, sizeof(vbuf), 0xABCD);
    HcsTestInitAttr(&refAttr, "refAttr", vbuf, 5);
    refAttr.attr.next = g_testNode.attrData;
    g_testNode.attrData = &refAttr.attr;

    g_testNode.hashValue = 0xABCD;

    const struct DeviceResourceNode *result = HcsGetNodeByRefAttr(&g_testNode, "refAttr");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->hashValue, 0xABCD);

    g_testNode.attrData = refAttr.attr.next;
    g_testNode.hashValue = 0;
}

HWTEST_F(HcsTreeIfTest, HcsGetNodeByRefAttrTest002, TestSize.Level1)
{
    const struct DeviceResourceNode *result = HcsGetNodeByRefAttr(nullptr, "refAttr");
    ASSERT_EQ(result, nullptr);
    result = HcsGetNodeByRefAttr(&g_testNode, nullptr);
    ASSERT_EQ(result, nullptr);
    result = HcsGetNodeByRefAttr(&g_testNode, HCS_TEST_ATTR_NAME_MISSING);
    ASSERT_EQ(result, nullptr);
    result = HcsGetNodeByRefAttr(&g_testNode, HCS_TEST_ATTR_NAME_STR);
    ASSERT_EQ(result, nullptr);
}

HWTEST_F(HcsTreeIfTest, HcsTreeIfCombinedTest001, TestSize.Level1)
{
    uint8_t u8v = 0;
    uint16_t u16v = 0;
    uint32_t u32v = 0;
    uint64_t u64v = 0;
    const char *strv = nullptr;

    ASSERT_EQ(HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_U8, &u8v, 0), HDF_SUCCESS);
    ASSERT_EQ(HcsGetUint16(&g_testNode, HCS_TEST_ATTR_NAME_U16, &u16v, 0), HDF_SUCCESS);
    ASSERT_EQ(HcsGetUint32(&g_testNode, HCS_TEST_ATTR_NAME_U32, &u32v, 0), HDF_SUCCESS);
    ASSERT_EQ(HcsGetUint64(&g_testNode, HCS_TEST_ATTR_NAME_U64, &u64v, 0), HDF_SUCCESS);
    ASSERT_EQ(HcsGetString(&g_testNode, HCS_TEST_ATTR_NAME_STR, &strv, ""), HDF_SUCCESS);
    ASSERT_TRUE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_BOOL));
    ASSERT_EQ(u8v, HCS_TEST_U8_VALUE);
    ASSERT_EQ(u16v, HCS_TEST_U16_VALUE);
    ASSERT_EQ(u32v, HCS_TEST_U32_VALUE);
    ASSERT_EQ(u64v, HCS_TEST_U64_VALUE);
    ASSERT_STREQ(strv, HCS_TEST_STRING_VALUE);
}

HWTEST_F(HcsTreeIfTest, HcsTreeIfCombinedTest002, TestSize.Level1)
{
    uint8_t arr[HCS_TEST_ARRAY_COUNT] = {0};
    ASSERT_EQ(HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, arr, HCS_TEST_ARRAY_COUNT, 0), HDF_SUCCESS);
    ASSERT_EQ(HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_ARR), HCS_TEST_ARRAY_COUNT);
    for (int i = 0; i < HCS_TEST_ARRAY_COUNT; i++) {
        uint8_t elem = 0;
        ASSERT_EQ(HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_ARR, i, &elem, 0), HDF_SUCCESS);
        ASSERT_EQ(elem, arr[i]);
    }
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8WrongTypeTest, TestSize.Level1)
{
    uint8_t value = 0;
    int32_t ret = HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_STR, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_ARR, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16WrongTypeTest, TestSize.Level1)
{
    uint16_t value = 0;
    int32_t ret = HcsGetUint16(&g_testNode, HCS_TEST_ATTR_NAME_STR, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint16(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32WrongTypeTest, TestSize.Level1)
{
    uint32_t value = 0;
    int32_t ret = HcsGetUint32(&g_testNode, HCS_TEST_ATTR_NAME_STR, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint32(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64WrongTypeTest, TestSize.Level1)
{
    uint64_t value = 0;
    int32_t ret = HcsGetUint64(&g_testNode, HCS_TEST_ATTR_NAME_STR, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
    ret = HcsGetUint64(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, &value, 99);
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_EQ(value, 99);
}

HWTEST_F(HcsTreeIfTest, HcsGetStringWrongTypeTest, TestSize.Level1)
{
    const char *value = nullptr;
    int32_t ret = HcsGetString(&g_testNode, HCS_TEST_ATTR_NAME_U8, &value, "default");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "default");
    ret = HcsGetString(&g_testNode, HCS_TEST_ATTR_NAME_U32, &value, "default");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "default");
    ret = HcsGetString(&g_testNode, HCS_TEST_ATTR_NAME_ARR, &value, "default");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "default");
}

HWTEST_F(HcsTreeIfTest, HcsGetBoolWrongTypeTest, TestSize.Level1)
{
    bool result = HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_STR);
    ASSERT_FALSE(result);
    result = HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_U32);
    ASSERT_FALSE(result);
    result = HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_ARR);
    ASSERT_FALSE(result);
}

HWTEST_F(HcsTreeIfTest, HcsGetElemNumWrongTypeTest, TestSize.Level1)
{
    int32_t num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_STR);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_BOOL);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_U8);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_U16);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_U32);
    ASSERT_EQ(num, HDF_FAILURE);
    num = HcsGetElemNum(&g_testNode, HCS_TEST_ATTR_NAME_U64);
    ASSERT_EQ(num, HDF_FAILURE);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8ArrayWrongTypeTest, TestSize.Level1)
{
    uint8_t values[3] = {0};
    int32_t ret = HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_STR, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16ArrayWrongTypeTest, TestSize.Level1)
{
    uint16_t values[3] = {0};
    int32_t ret = HcsGetUint16Array(&g_testNode, HCS_TEST_ATTR_NAME_STR, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint16Array(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32ArrayWrongTypeTest, TestSize.Level1)
{
    uint32_t values[3] = {0};
    int32_t ret = HcsGetUint32Array(&g_testNode, HCS_TEST_ATTR_NAME_STR, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint32Array(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64ArrayWrongTypeTest, TestSize.Level1)
{
    uint64_t values[3] = {0};
    int32_t ret = HcsGetUint64Array(&g_testNode, HCS_TEST_ATTR_NAME_STR, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint64Array(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, values, 3, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8ArrayElemWrongTypeTest, TestSize.Level1)
{
    uint8_t value = 0;
    int32_t ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_STR, 0, &value, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint8ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, 0, &value, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16ArrayElemWrongTypeTest, TestSize.Level1)
{
    uint16_t value = 0;
    int32_t ret = HcsGetUint16ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_STR, 0, &value, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
    ret = HcsGetUint16ArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, 0, &value, 0);
    ASSERT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(HcsTreeIfTest, HcsGetStringArrayElemWrongTypeTest, TestSize.Level1)
{
    const char *value = nullptr;
    int32_t ret = HcsGetStringArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_U8, 0, &value, "def");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "def");
    ret = HcsGetStringArrayElem(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, 0, &value, "def");
    ASSERT_NE(ret, HDF_SUCCESS);
    ASSERT_STREQ(value, "def");
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8ArrayPartialLenTest, TestSize.Level1)
{
    uint8_t values[2] = {0};
    int32_t ret = HcsGetUint8Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 2, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], HCS_TEST_ARRAY_ELEM0);
    ASSERT_EQ(values[1], HCS_TEST_ARRAY_ELEM1);
}

HWTEST_F(HcsTreeIfTest, HcsGetUint16ArrayPartialLenTest, TestSize.Level1)
{
    uint16_t values[1] = {0};
    int32_t ret = HcsGetUint16Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 1, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], static_cast<uint16_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint32ArrayPartialLenTest, TestSize.Level1)
{
    uint32_t values[2] = {0};
    int32_t ret = HcsGetUint32Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 2, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], static_cast<uint32_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint64ArrayPartialLenTest, TestSize.Level1)
{
    uint64_t values[1] = {0};
    int32_t ret = HcsGetUint64Array(&g_testNode, HCS_TEST_ATTR_NAME_ARR, values, 1, 0);
    ASSERT_EQ(ret, HDF_SUCCESS);
    ASSERT_EQ(values[0], static_cast<uint64_t>(HCS_TEST_ARRAY_ELEM0));
}

HWTEST_F(HcsTreeIfTest, HcsGetChildNodeMultipleChildrenTest, TestSize.Level1)
{
    struct DeviceResourceNode children[5];
    const char *names[] = {"c0", "c1", "c2", "c3", "c4"};
    for (int i = 0; i < 5; i++) {
        (void)memset_s(&children[i], sizeof(children[i]), 0, sizeof(children[i]));
        children[i].name = names[i];
        if (i < 4) {
            children[i].sibling = &children[i + 1];
        } else {
            children[i].sibling = nullptr;
        }
    }
    g_testNode.child = &children[0];
    for (int i = 0; i < 5; i++) {
        const struct DeviceResourceNode *result = HcsGetChildNode(&g_testNode, names[i]);
        ASSERT_EQ(result, &children[i]);
    }
    const struct DeviceResourceNode *result = HcsGetChildNode(&g_testNode, "nonexistent");
    ASSERT_EQ(result, nullptr);
    g_testNode.child = nullptr;
}

HWTEST_F(HcsTreeIfTest, HcsGetChildNodeNullNameChildrenTest, TestSize.Level1)
{
    struct DeviceResourceNode child;
    (void)memset_s(&child, sizeof(child), 0, sizeof(child));
    child.name = nullptr;
    child.sibling = nullptr;
    g_testNode.child = &child;
    const struct DeviceResourceNode *result = HcsGetChildNode(&g_testNode, "any_name");
    ASSERT_EQ(result, nullptr);
    g_testNode.child = nullptr;
}

HWTEST_F(HcsTreeIfTest, HcsAllApisNullNodeTest, TestSize.Level1)
{
    uint8_t u8v = 1;
    uint16_t u16v = 1;
    uint32_t u32v = 1;
    uint64_t u64v = 1;
    const char *strv = "init";
    ASSERT_FALSE(HcsGetBool(nullptr, "attr"));
    ASSERT_NE(HcsGetUint8(nullptr, "attr", &u8v, 0), HDF_SUCCESS);
    ASSERT_EQ(u8v, 0);
    ASSERT_NE(HcsGetUint16(nullptr, "attr", &u16v, 0), HDF_SUCCESS);
    ASSERT_EQ(u16v, 0);
    ASSERT_NE(HcsGetUint32(nullptr, "attr", &u32v, 0), HDF_SUCCESS);
    ASSERT_EQ(u32v, 0);
    ASSERT_NE(HcsGetUint64(nullptr, "attr", &u64v, 0), HDF_SUCCESS);
    ASSERT_EQ(u64v, 0);
    ASSERT_NE(HcsGetString(nullptr, "attr", &strv, ""), HDF_SUCCESS);
    ASSERT_STREQ(strv, "");
    ASSERT_EQ(HcsGetElemNum(nullptr, "attr"), HDF_FAILURE);
    ASSERT_EQ(HcsGetChildNode(nullptr, "child"), nullptr);
    ASSERT_EQ(HcsGetNodeByMatchAttr(nullptr, "value"), nullptr);
    ASSERT_EQ(HcsGetNodeByRefAttr(nullptr, "attr"), nullptr);
    uint8_t arr8[3] = {0};
    uint16_t arr16[3] = {0};
    uint32_t arr32[3] = {0};
    uint64_t arr64[3] = {0};
    ASSERT_NE(HcsGetUint8Array(nullptr, "attr", arr8, 3, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint16Array(nullptr, "attr", arr16, 3, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint32Array(nullptr, "attr", arr32, 3, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint64Array(nullptr, "attr", arr64, 3, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint8ArrayElem(nullptr, "attr", 0, &u8v, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint16ArrayElem(nullptr, "attr", 0, &u16v, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint32ArrayElem(nullptr, "attr", 0, &u32v, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint64ArrayElem(nullptr, "attr", 0, &u64v, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetStringArrayElem(nullptr, "attr", 0, &strv, ""), HDF_SUCCESS);
    ASSERT_STREQ(strv, "");
}

HWTEST_F(HcsTreeIfTest, HcsGetBoolFromAllAttrTypesTest, TestSize.Level1)
{
    ASSERT_TRUE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_BOOL));
    ASSERT_FALSE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_U8));
    ASSERT_FALSE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_U16));
    ASSERT_FALSE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_U32));
    ASSERT_FALSE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_U64));
    ASSERT_FALSE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_STR));
    ASSERT_FALSE(HcsGetBool(&g_testNode, HCS_TEST_ATTR_NAME_ARR));
}

HWTEST_F(HcsTreeIfTest, HcsGetUint8FromAllAttrTypesTest, TestSize.Level1)
{
    uint8_t value = 0;
    ASSERT_EQ(HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_U8, &value, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_STR, &value, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_ARR, &value, 0), HDF_SUCCESS);
    ASSERT_NE(HcsGetUint8(&g_testNode, HCS_TEST_ATTR_NAME_BOOL, &value, 0), HDF_SUCCESS);
}
} // namespace OHOS

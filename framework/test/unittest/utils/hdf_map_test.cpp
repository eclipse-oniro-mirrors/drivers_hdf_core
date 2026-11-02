/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <algorithm>
#include "hdf_map.h"
#include "hdf_cstring.h"

using namespace testing::ext;

#define HDF_LOG_TAG map_test

class HdfMapTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override
    {
        MapInit(&map);
    }
    void TearDown() override
    {
        MapDelete(&map);
    }
    Map map;
};

/* ==================== MapInit ==================== */

HWTEST_F(HdfMapTest, MapInit001, TestSize.Level1)
{
    EXPECT_EQ(map.nodes, nullptr);
    EXPECT_EQ(map.nodeSize, 0u);
    EXPECT_EQ(map.bucketSize, 0u);
}

HWTEST_F(HdfMapTest, MapInitNull001, TestSize.Level1)
{
    MapInit(nullptr);
    EXPECT_TRUE(true);
}

/* ==================== MapSet and MapGet ==================== */

HWTEST_F(HdfMapTest, MapSetGet001, TestSize.Level1)
{
    int value = 42;
    EXPECT_EQ(MapSet(&map, "key1", &value, sizeof(value)), HDF_SUCCESS);
    void *result = MapGet(&map, "key1");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*(int *)result, 42);
}

HWTEST_F(HdfMapTest, MapSetGetMultiple001, TestSize.Level1)
{
    int v1 = 10, v2 = 20, v3 = 30;
    EXPECT_EQ(MapSet(&map, "a", &v1, sizeof(v1)), HDF_SUCCESS);
    EXPECT_EQ(MapSet(&map, "b", &v2, sizeof(v2)), HDF_SUCCESS);
    EXPECT_EQ(MapSet(&map, "c", &v3, sizeof(v3)), HDF_SUCCESS);
    EXPECT_EQ(map.nodeSize, 3u);
    EXPECT_EQ(*(int *)MapGet(&map, "a"), 10);
    EXPECT_EQ(*(int *)MapGet(&map, "b"), 20);
    EXPECT_EQ(*(int *)MapGet(&map, "c"), 30);
}

HWTEST_F(HdfMapTest, MapSetUpdate001, TestSize.Level1)
{
    int v1 = 10;
    int v2 = 99;
    EXPECT_EQ(MapSet(&map, "key", &v1, sizeof(v1)), HDF_SUCCESS);
    EXPECT_EQ(*(int *)MapGet(&map, "key"), 10);
    /* Update with same size */
    EXPECT_EQ(MapSet(&map, "key", &v2, sizeof(v2)), HDF_SUCCESS);
    EXPECT_EQ(*(int *)MapGet(&map, "key"), 99);
    EXPECT_EQ(map.nodeSize, 1u);
}

HWTEST_F(HdfMapTest, MapSetUpdateSizeMismatch001, TestSize.Level1)
{
    int v1 = 10;
    int64_t v2 = 99;
    EXPECT_EQ(MapSet(&map, "key", &v1, sizeof(v1)), HDF_SUCCESS);
    /* Update with different size should fail */
    EXPECT_EQ(MapSet(&map, "key", &v2, sizeof(v2)), HDF_ERR_INVALID_OBJECT);
}

HWTEST_F(HdfMapTest, MapSetNullParams001, TestSize.Level1)
{
    int v = 10;
    EXPECT_EQ(MapSet(nullptr, "key", &v, sizeof(v)), HDF_ERR_INVALID_PARAM);
    EXPECT_EQ(MapSet(&map, nullptr, &v, sizeof(v)), HDF_ERR_INVALID_PARAM);
    EXPECT_EQ(MapSet(&map, "key", nullptr, sizeof(v)), HDF_ERR_INVALID_PARAM);
    EXPECT_EQ(MapSet(&map, "key", &v, 0), HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfMapTest, MapSetOversizedValue001, TestSize.Level1)
{
    char bigBuf[1001];
    std::fill(bigBuf, bigBuf + sizeof(bigBuf), 'x');
    EXPECT_EQ(MapSet(&map, "key", bigBuf, sizeof(bigBuf)), HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfMapTest, MapSetOversizedKey001, TestSize.Level1)
{
    char bigKey[1002];
    std::fill(bigKey, bigKey + 1001, 'k');
    bigKey[1001] = '\0';
    int v = 10;
    EXPECT_EQ(MapSet(&map, bigKey, &v, sizeof(v)), HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfMapTest, MapGetNotFound001, TestSize.Level1)
{
    int v = 10;
    MapSet(&map, "key", &v, sizeof(v));
    EXPECT_EQ(MapGet(&map, "nonexistent"), nullptr);
}

HWTEST_F(HdfMapTest, MapGetEmpty001, TestSize.Level1)
{
    EXPECT_EQ(MapGet(&map, "key"), nullptr);
}

HWTEST_F(HdfMapTest, MapGetNull001, TestSize.Level1)
{
    EXPECT_EQ(MapGet(nullptr, "key"), nullptr);
    int v = 10;
    MapSet(&map, "key", &v, sizeof(v));
    EXPECT_EQ(MapGet(&map, nullptr), nullptr);
}

/* ==================== MapErase ==================== */

HWTEST_F(HdfMapTest, MapErase001, TestSize.Level1)
{
    int v = 10;
    MapSet(&map, "key", &v, sizeof(v));
    EXPECT_EQ(map.nodeSize, 1u);
    EXPECT_EQ(MapErase(&map, "key"), HDF_SUCCESS);
    EXPECT_EQ(map.nodeSize, 0u);
    EXPECT_EQ(MapGet(&map, "key"), nullptr);
}

HWTEST_F(HdfMapTest, MapEraseNotFound001, TestSize.Level1)
{
    int v = 10;
    MapSet(&map, "key", &v, sizeof(v));
    EXPECT_EQ(MapErase(&map, "nonexistent"), HDF_FAILURE);
    EXPECT_EQ(map.nodeSize, 1u);
}

HWTEST_F(HdfMapTest, MapEraseEmpty001, TestSize.Level1)
{
    EXPECT_EQ(MapErase(&map, "key"), HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfMapTest, MapEraseNull001, TestSize.Level1)
{
    EXPECT_EQ(MapErase(nullptr, "key"), HDF_ERR_INVALID_PARAM);
    int v = 10;
    MapSet(&map, "key", &v, sizeof(v));
    EXPECT_EQ(MapErase(&map, nullptr), HDF_ERR_INVALID_PARAM);
}

HWTEST_F(HdfMapTest, MapEraseAndReinsert001, TestSize.Level1)
{
    int v1 = 10, v2 = 20;
    MapSet(&map, "key", &v1, sizeof(v1));
    MapErase(&map, "key");
    EXPECT_EQ(MapSet(&map, "key", &v2, sizeof(v2)), HDF_SUCCESS);
    EXPECT_EQ(*(int *)MapGet(&map, "key"), 20);
    EXPECT_EQ(map.nodeSize, 1u);
}

/* ==================== MapDelete ==================== */

HWTEST_F(HdfMapTest, MapDelete001, TestSize.Level1)
{
    int v1 = 10, v2 = 20, v3 = 30;
    MapSet(&map, "a", &v1, sizeof(v1));
    MapSet(&map, "b", &v2, sizeof(v2));
    MapSet(&map, "c", &v3, sizeof(v3));
    MapDelete(&map);
    EXPECT_EQ(map.nodes, nullptr);
    EXPECT_EQ(map.nodeSize, 0u);
    EXPECT_EQ(map.bucketSize, 0u);
}

HWTEST_F(HdfMapTest, MapDeleteEmpty001, TestSize.Level1)
{
    MapDelete(&map);
    EXPECT_EQ(map.nodes, nullptr);
}

HWTEST_F(HdfMapTest, MapDeleteNull001, TestSize.Level1)
{
    MapDelete(nullptr);
    EXPECT_TRUE(true);
}

/* ==================== Stress / Bucket Growth ==================== */

HWTEST_F(HdfMapTest, MapStressManyEntries001, TestSize.Level0)
{
    const int count = 100;
    int values[100];
    for (int i = 0; i < count; i++) {
        values[i] = i;
        std::string key = "key_" + std::to_string(i);
        EXPECT_EQ(MapSet(&map, key.c_str(), &values[i], sizeof(int)), HDF_SUCCESS);
    }
    EXPECT_EQ(map.nodeSize, (uint32_t)count);
    EXPECT_GT(map.bucketSize, (uint32_t)count);

    /* Verify all entries */
    for (int i = 0; i < count; i++) {
        std::string key = "key_" + std::to_string(i);
        void *result = MapGet(&map, key.c_str());
        ASSERT_NE(result, nullptr) << "Failed for key " << key;
        EXPECT_EQ(*(int *)result, i);
    }
}

HWTEST_F(HdfMapTest, MapStressEraseHalf001, TestSize.Level0)
{
    const int count = 50;
    int values[50];
    for (int i = 0; i < count; i++) {
        values[i] = i;
        std::string key = "k" + std::to_string(i);
        MapSet(&map, key.c_str(), &values[i], sizeof(int));
    }
    EXPECT_EQ(map.nodeSize, (uint32_t)count);

    /* Erase even-indexed entries */
    for (int i = 0; i < count; i += 2) {
        std::string key = "k" + std::to_string(i);
        EXPECT_EQ(MapErase(&map, key.c_str()), HDF_SUCCESS);
    }
    EXPECT_EQ(map.nodeSize, (uint32_t)(count / 2));

    /* Verify odd entries still exist */
    for (int i = 1; i < count; i += 2) {
        std::string key = "k" + std::to_string(i);
        void *result = MapGet(&map, key.c_str());
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*(int *)result, i);
    }
}

HWTEST_F(HdfMapTest, MapStressOverwrite001, TestSize.Level1)
{
    int v = 0;
    for (int i = 0; i < 20; i++) {
        v = i;
        EXPECT_EQ(MapSet(&map, "same_key", &v, sizeof(v)), HDF_SUCCESS);
    }
    EXPECT_EQ(map.nodeSize, 1u);
    EXPECT_EQ(*(int *)MapGet(&map, "same_key"), 19);
}

HWTEST_F(HdfMapTest, MapStressCollisions001, TestSize.Level1)
{
    /* Insert entries that might hash to the same bucket */
    int v = 100;
    const char *keys[] = {"aa", "bb", "ab", "ba", "aaa", "bbb", "aba", "bab"};
    int numKeys = static_cast<int>(sizeof(keys) / sizeof(keys[0]));
    for (int i = 0; i < numKeys; i++) {
        EXPECT_EQ(MapSet(&map, keys[i], &v, sizeof(v)), HDF_SUCCESS);
    }
    EXPECT_EQ(map.nodeSize, (uint32_t)numKeys);
    for (int i = 0; i < numKeys; i++) {
        void *result = MapGet(&map, keys[i]);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*(int *)result, 100);
    }
}

/* ==================== CString tests ==================== */

class HdfCStringTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(HdfCStringTest, HdfStringMakeHashKey001, TestSize.Level1)
{
    uint32_t hash1 = HdfStringMakeHashKey("hello", 0);
    uint32_t hash2 = HdfStringMakeHashKey("hello", 0);
    EXPECT_EQ(hash1, hash2);
}

HWTEST_F(HdfCStringTest, HdfStringMakeHashKeyDifferent001, TestSize.Level1)
{
    uint32_t hash1 = HdfStringMakeHashKey("hello", 0);
    uint32_t hash2 = HdfStringMakeHashKey("world", 0);
    EXPECT_NE(hash1, hash2);
}

HWTEST_F(HdfCStringTest, HdfStringMakeHashKeyMask001, TestSize.Level1)
{
    uint32_t hash = HdfStringMakeHashKey("test", 0);
    uint32_t hashMasked = HdfStringMakeHashKey("test", 0x80000000);
    EXPECT_EQ(hash | 0x80000000, hashMasked);
}

HWTEST_F(HdfCStringTest, HdfStringMakeHashKeyNull001, TestSize.Level1)
{
    uint32_t hash = HdfStringMakeHashKey(nullptr, 0);
    EXPECT_EQ(hash, 0u);
}

HWTEST_F(HdfCStringTest, HdfCStringObtain001, TestSize.Level1)
{
    const char *str = "hello world";
    struct HdfCString *cs = HdfCStringObtain(str);
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->size, static_cast<int>(strlen(str)));
    EXPECT_STREQ(cs->value, str);
    HdfCStringRecycle(cs);
}

HWTEST_F(HdfCStringTest, HdfCStringObtainEmpty001, TestSize.Level1)
{
    struct HdfCString *cs = HdfCStringObtain("");
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->size, 0);
    EXPECT_STREQ(cs->value, "");
    HdfCStringRecycle(cs);
}

HWTEST_F(HdfCStringTest, HdfCStringObtainNull001, TestSize.Level1)
{
    struct HdfCString *cs = HdfCStringObtain(nullptr);
    EXPECT_EQ(cs, nullptr);
}

HWTEST_F(HdfCStringTest, HdfCStringObtainMaxLen001, TestSize.Level1)
{
    /* CSTRING_MAX is 1024 — string of exactly 1024 chars should succeed */
    char str1024[1025];
    std::fill(str1024, str1024 + 1024, 'a');
    str1024[1024] = '\0';
    struct HdfCString *cs = HdfCStringObtain(str1024);
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->size, 1024);
    HdfCStringRecycle(cs);
}

HWTEST_F(HdfCStringTest, HdfCStringObtainOverMaxLen001, TestSize.Level1)
{
    /* String of 1025 chars should fail */
    char str1025[1026];
    std::fill(str1025, str1025 + 1025, 'a');
    str1025[1025] = '\0';
    struct HdfCString *cs = HdfCStringObtain(str1025);
    EXPECT_EQ(cs, nullptr);
}

HWTEST_F(HdfCStringTest, HdfCStringRecycleNull001, TestSize.Level1)
{
    HdfCStringRecycle(nullptr);
    EXPECT_TRUE(true);
}

HWTEST_F(HdfCStringTest, HdfStringCopy001, TestSize.Level1)
{
    const char *src = "copy me";
    char *dst = HdfStringCopy(src);
    ASSERT_NE(dst, nullptr);
    EXPECT_STREQ(dst, src);
    EXPECT_NE(dst, src);
    free(dst);
}

HWTEST_F(HdfCStringTest, HdfStringCopyEmpty001, TestSize.Level1)
{
    char *dst = HdfStringCopy("");
    ASSERT_NE(dst, nullptr);
    EXPECT_STREQ(dst, "");
    free(dst);
}

HWTEST_F(HdfCStringTest, HdfStringCopyNull001, TestSize.Level1)
{
    char *dst = HdfStringCopy(nullptr);
    EXPECT_EQ(dst, nullptr);
}

HWTEST_F(HdfCStringTest, HdfStringCopyLong001, TestSize.Level1)
{
    std::string longStr(500, 'x');
    char *dst = HdfStringCopy(longStr.c_str());
    ASSERT_NE(dst, nullptr);
    EXPECT_STREQ(dst, longStr.c_str());
    free(dst);
}

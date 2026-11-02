/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include <cstring>
#include "hdf_slist.h"

using namespace testing::ext;

#define HDF_LOG_TAG slist_test

/* Test container that embeds an HdfSListNode */
struct TestNode {
    int value;
    struct HdfSListNode node;
};

#define NODE_TO_TEST(ptr) HDF_SLIST_CONTAINER_OF(struct HdfSListNode, (ptr), struct TestNode, node)

static struct TestNode *CreateTestNode(int val)
{
    struct TestNode *node = new TestNode;
    node->value = val;
    node->node.next = nullptr;
    return node;
}

static void DeleteTestNode(struct HdfSListNode *link)
{
    if (link != nullptr) {
        struct TestNode *node = NODE_TO_TEST(link);
        delete node;
    }
}

static bool SearchByValue(struct HdfSListNode *link, uint32_t key)
{
    if (link == nullptr) {
        return false;
    }
    struct TestNode *node = NODE_TO_TEST(link);
    return (uint32_t)node->value == key;
}

static bool CompareAscending(struct HdfSListNode *current, struct HdfSListNode *newNode)
{
    if (current == nullptr || newNode == nullptr) {
        return false;
    }
    struct TestNode *cur = NODE_TO_TEST(current);
    struct TestNode *nw = NODE_TO_TEST(newNode);
    return cur->value <= nw->value;
}

class HdfSListTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override
    {
        HdfSListInit(&list);
    }
    void TearDown() override
    {
        HdfSListFlush(&list, DeleteTestNode);
    }
    struct HdfSList list;
};

/* ==================== Init and IsEmpty ==================== */

HWTEST_F(HdfSListTest, HdfSListInit001, TestSize.Level1)
{
    struct HdfSList l;
    HdfSListInit(&l);
    ASSERT_TRUE(l.root == nullptr);
    EXPECT_TRUE(HdfSListIsEmpty(&l));
}

HWTEST_F(HdfSListTest, HdfSListInit002, TestSize.Level1)
{
    EXPECT_TRUE(HdfSListIsEmpty(&list));
}

HWTEST_F(HdfSListTest, HdfSListInitNull001, TestSize.Level1)
{
    HdfSListInit(nullptr);
    EXPECT_TRUE(true);
}

HWTEST_F(HdfSListTest, HdfSListIsEmptyNull001, TestSize.Level1)
{
    EXPECT_TRUE(HdfSListIsEmpty(nullptr));
}

/* ==================== Add (head) ==================== */

HWTEST_F(HdfSListTest, HdfSListAdd001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    HdfSListAdd(&list, &n1->node);
    EXPECT_FALSE(HdfSListIsEmpty(&list));
    EXPECT_EQ(HdfSListCount(&list), 1);
    struct HdfSListNode *peek = HdfSListPeek(&list);
    ASSERT_NE(peek, nullptr);
    EXPECT_EQ(NODE_TO_TEST(peek)->value, 10);
}

HWTEST_F(HdfSListTest, HdfSListAdd002, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    struct TestNode *n3 = CreateTestNode(30);
    HdfSListAdd(&list, &n1->node);
    HdfSListAdd(&list, &n2->node);
    HdfSListAdd(&list, &n3->node);
    EXPECT_EQ(HdfSListCount(&list), 3);
    /* Add prepends — order should be 30, 20, 10 */
    struct HdfSListNode *iter = HdfSListPeek(&list);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 30);
    iter = HdfSListNext(iter);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 20);
    iter = HdfSListNext(iter);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 10);
}

HWTEST_F(HdfSListTest, HdfSListAddNull001, TestSize.Level1)
{
    HdfSListAdd(&list, nullptr);
    EXPECT_TRUE(HdfSListIsEmpty(&list));
    struct TestNode *n1 = CreateTestNode(10);
    HdfSListAdd(nullptr, &n1->node);
    delete n1;
    EXPECT_TRUE(HdfSListIsEmpty(&list));
}

/* ==================== AddTail (append) ==================== */

HWTEST_F(HdfSListTest, HdfSListAddTail001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    struct TestNode *n3 = CreateTestNode(30);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n2->node);
    HdfSListAddTail(&list, &n3->node);
    EXPECT_EQ(HdfSListCount(&list), 3);
    /* AddTail appends — order should be 10, 20, 30 */
    struct HdfSListNode *iter = HdfSListPeek(&list);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 10);
    iter = HdfSListNext(iter);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 20);
    iter = HdfSListNext(iter);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 30);
}

HWTEST_F(HdfSListTest, HdfSListAddTailDedup001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n1->node);
    EXPECT_EQ(HdfSListCount(&list), 1);
}

HWTEST_F(HdfSListTest, HdfSListAddTailNull001, TestSize.Level1)
{
    HdfSListAddTail(&list, nullptr);
    EXPECT_TRUE(HdfSListIsEmpty(&list));
}

/* ==================== AddOrder (ordered insert) ==================== */

HWTEST_F(HdfSListTest, HdfSListAddOrder001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    struct TestNode *n3 = CreateTestNode(30);
    /* Insert out of order */
    EXPECT_TRUE(HdfSListAddOrder(&list, &n2->node, CompareAscending));
    EXPECT_TRUE(HdfSListAddOrder(&list, &n3->node, CompareAscending));
    EXPECT_TRUE(HdfSListAddOrder(&list, &n1->node, CompareAscending));
    EXPECT_EQ(HdfSListCount(&list), 3);
    /* Verify ascending order */
    struct HdfSListNode *iter = HdfSListPeek(&list);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 10);
    iter = HdfSListNext(iter);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 20);
    iter = HdfSListNext(iter);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 30);
}

HWTEST_F(HdfSListTest, HdfSListAddOrderEmpty001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    EXPECT_TRUE(HdfSListAddOrder(&list, &n1->node, CompareAscending));
    EXPECT_EQ(HdfSListCount(&list), 1);
    EXPECT_EQ(NODE_TO_TEST(HdfSListPeek(&list))->value, 10);
}

HWTEST_F(HdfSListTest, HdfSListAddOrderDuplicate001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    EXPECT_TRUE(HdfSListAddOrder(&list, &n1->node, CompareAscending));
    EXPECT_FALSE(HdfSListAddOrder(&list, &n1->node, CompareAscending));
    EXPECT_EQ(HdfSListCount(&list), 1);
}

HWTEST_F(HdfSListTest, HdfSListAddOrderNull001, TestSize.Level1)
{
    EXPECT_FALSE(HdfSListAddOrder(&list, nullptr, CompareAscending));
    struct TestNode *n1 = CreateTestNode(10);
    EXPECT_FALSE(HdfSListAddOrder(&list, &n1->node, nullptr));
    delete n1;
    EXPECT_FALSE(HdfSListAddOrder(nullptr, nullptr, nullptr));
    EXPECT_TRUE(HdfSListIsEmpty(&list));
}

/* ==================== Remove ==================== */

HWTEST_F(HdfSListTest, HdfSListRemove001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    struct TestNode *n3 = CreateTestNode(30);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n2->node);
    HdfSListAddTail(&list, &n3->node);
    HdfSListRemove(&list, &n2->node);
    EXPECT_EQ(HdfSListCount(&list), 2);
    struct HdfSListNode *iter = HdfSListPeek(&list);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 10);
    iter = HdfSListNext(iter);
    EXPECT_EQ(NODE_TO_TEST(iter)->value, 30);
    delete n2;
}

HWTEST_F(HdfSListTest, HdfSListRemoveHead001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n2->node);
    HdfSListRemove(&list, &n1->node);
    EXPECT_EQ(HdfSListCount(&list), 1);
    EXPECT_EQ(NODE_TO_TEST(HdfSListPeek(&list))->value, 20);
    delete n1;
}

HWTEST_F(HdfSListTest, HdfSListRemoveNotFound001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    HdfSListAddTail(&list, &n1->node);
    HdfSListRemove(&list, &n2->node);
    EXPECT_EQ(HdfSListCount(&list), 1);
    delete n2;
}

HWTEST_F(HdfSListTest, HdfSListRemoveNull001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    HdfSListAddTail(&list, &n1->node);
    HdfSListRemove(&list, nullptr);
    HdfSListRemove(nullptr, &n1->node);
    EXPECT_EQ(HdfSListCount(&list), 1);
}

/* ==================== Count ==================== */

HWTEST_F(HdfSListTest, HdfSListCount001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListCount(&list), 0);
    for (int i = 0; i < 10; i++) {
        struct TestNode *n = CreateTestNode(i);
        HdfSListAddTail(&list, &n->node);
    }
    EXPECT_EQ(HdfSListCount(&list), 10);
}

HWTEST_F(HdfSListTest, HdfSListCountNull001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListCount(nullptr), 0);
}

/* ==================== Peek and Next ==================== */

HWTEST_F(HdfSListTest, HdfSListPeek001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListPeek(&list), nullptr);
    struct TestNode *n1 = CreateTestNode(10);
    HdfSListAdd(&list, &n1->node);
    ASSERT_NE(HdfSListPeek(&list), nullptr);
    EXPECT_EQ(NODE_TO_TEST(HdfSListPeek(&list))->value, 10);
}

HWTEST_F(HdfSListTest, HdfSListPeekNull001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListPeek(nullptr), nullptr);
}

HWTEST_F(HdfSListTest, HdfSListNext001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n2->node);
    struct HdfSListNode *first = HdfSListPeek(&list);
    struct HdfSListNode *second = HdfSListNext(first);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(NODE_TO_TEST(second)->value, 20);
    EXPECT_EQ(HdfSListNext(second), nullptr);
}

HWTEST_F(HdfSListTest, HdfSListNextNull001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListNext(nullptr), nullptr);
}

/* ==================== Pop ==================== */

HWTEST_F(HdfSListTest, HdfSListPop001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n2->node);
    struct HdfSListNode *popped = HdfSListPop(&list);
    ASSERT_NE(popped, nullptr);
    EXPECT_EQ(NODE_TO_TEST(popped)->value, 10);
    EXPECT_EQ(HdfSListCount(&list), 1);
    delete n1;
}

HWTEST_F(HdfSListTest, HdfSListPopEmpty001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListPop(&list), nullptr);
}

HWTEST_F(HdfSListTest, HdfSListPopNull001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListPop(nullptr), nullptr);
}

/* ==================== GetLast ==================== */

HWTEST_F(HdfSListTest, HdfSListGetLast001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(20);
    struct TestNode *n3 = CreateTestNode(30);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n2->node);
    HdfSListAddTail(&list, &n3->node);
    struct HdfSListNode *last = HdfSListGetLast(&list);
    ASSERT_NE(last, nullptr);
    EXPECT_EQ(NODE_TO_TEST(last)->value, 30);
}

HWTEST_F(HdfSListTest, HdfSListGetLastEmpty001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListGetLast(&list), nullptr);
}

HWTEST_F(HdfSListTest, HdfSListGetLastNull001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListGetLast(nullptr), nullptr);
}

/* ==================== Search ==================== */

HWTEST_F(HdfSListTest, HdfSListSearch001, TestSize.Level1)
{
    for (int i = 0; i < 5; i++) {
        struct TestNode *n = CreateTestNode(i * 10);
        HdfSListAddTail(&list, &n->node);
    }
    struct HdfSListNode *found = HdfSListSearch(&list, 30, SearchByValue);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(NODE_TO_TEST(found)->value, 30);
}

HWTEST_F(HdfSListTest, HdfSListSearchNotFound001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    HdfSListAddTail(&list, &n1->node);
    EXPECT_EQ(HdfSListSearch(&list, 999, SearchByValue), nullptr);
}

HWTEST_F(HdfSListTest, HdfSListSearchNull001, TestSize.Level1)
{
    EXPECT_EQ(HdfSListSearch(&list, 10, nullptr), nullptr);
    EXPECT_EQ(HdfSListSearch(nullptr, 10, SearchByValue), nullptr);
}

/* ==================== Flush ==================== */

HWTEST_F(HdfSListTest, HdfSListFlush001, TestSize.Level1)
{
    for (int i = 0; i < 5; i++) {
        struct TestNode *n = CreateTestNode(i);
        HdfSListAddTail(&list, &n->node);
    }
    EXPECT_EQ(HdfSListCount(&list), 5);
    HdfSListFlush(&list, DeleteTestNode);
    EXPECT_TRUE(HdfSListIsEmpty(&list));
    EXPECT_EQ(HdfSListCount(&list), 0);
}

HWTEST_F(HdfSListTest, HdfSListFlushNullDeleter001, TestSize.Level1)
{
    for (int i = 0; i < 3; i++) {
        struct TestNode *n = CreateTestNode(i);
        HdfSListAddTail(&list, &n->node);
    }
    HdfSListFlush(&list, nullptr);
    EXPECT_TRUE(HdfSListIsEmpty(&list));
    /* Nodes are not freed — need manual cleanup to avoid leak */
}

HWTEST_F(HdfSListTest, HdfSListFlushNull001, TestSize.Level1)
{
    HdfSListFlush(nullptr, DeleteTestNode);
    EXPECT_TRUE(true);
}

/* ==================== Iterator ==================== */

HWTEST_F(HdfSListTest, HdfSListIteratorBasic001, TestSize.Level1)
{
    for (int i = 0; i < 5; i++) {
        struct TestNode *n = CreateTestNode(i * 10);
        HdfSListAddTail(&list, &n->node);
    }
    struct HdfSListIterator iter;
    HdfSListIteratorInit(&iter, &list);
    int count = 0;
    while (HdfSListIteratorHasNext(&iter)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&iter);
        ASSERT_NE(node, nullptr);
        EXPECT_EQ(NODE_TO_TEST(node)->value, count * 10);
        count++;
    }
    EXPECT_EQ(count, 5);
}

HWTEST_F(HdfSListTest, HdfSListIteratorEmpty001, TestSize.Level1)
{
    struct HdfSListIterator iter;
    HdfSListIteratorInit(&iter, &list);
    EXPECT_FALSE(HdfSListIteratorHasNext(&iter));
}

HWTEST_F(HdfSListTest, HdfSListIteratorNull001, TestSize.Level1)
{
    struct HdfSListIterator iter;
    HdfSListIteratorInit(&iter, nullptr);
    EXPECT_FALSE(HdfSListIteratorHasNext(&iter));
    EXPECT_EQ(HdfSListIteratorNext(&iter), nullptr);
}

HWTEST_F(HdfSListTest, HdfSListIteratorRemove001, TestSize.Level1)
{
    for (int i = 0; i < 5; i++) {
        struct TestNode *n = CreateTestNode(i * 10);
        HdfSListAddTail(&list, &n->node);
    }
    struct HdfSListIterator iter;
    HdfSListIteratorInit(&iter, &list);
    int removedCount = 0;
    while (HdfSListIteratorHasNext(&iter)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&iter);
        ASSERT_NE(node, nullptr);
        if (NODE_TO_TEST(node)->value == 20) {
            HdfSListIteratorRemove(&iter);
            DeleteTestNode(node);
            removedCount++;
        }
    }
    EXPECT_EQ(removedCount, 1);
    EXPECT_EQ(HdfSListCount(&list), 4);
    /* Verify 20 is gone */
    EXPECT_EQ(HdfSListSearch(&list, 20, SearchByValue), nullptr);
}

HWTEST_F(HdfSListTest, HdfSListIteratorInsert001, TestSize.Level1)
{
    struct TestNode *n1 = CreateTestNode(10);
    struct TestNode *n2 = CreateTestNode(30);
    HdfSListAddTail(&list, &n1->node);
    HdfSListAddTail(&list, &n2->node);
    struct HdfSListIterator iter;
    HdfSListIteratorInit(&iter, &list);
    while (HdfSListIteratorHasNext(&iter)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&iter);
        ASSERT_NE(node, nullptr);
        if (NODE_TO_TEST(node)->value == 10) {
            struct TestNode *newNode = CreateTestNode(20);
            HdfSListIteratorInsert(&iter, &newNode->node);
            break;
        }
    }
    EXPECT_EQ(HdfSListCount(&list), 3);
    /* HdfSListIteratorInsert inserts BEFORE current node.
     * List was 10->30, insert 20 before 10 gives: 20->10->30 */
    struct HdfSListNode *ptr = HdfSListPeek(&list);
    EXPECT_EQ(NODE_TO_TEST(ptr)->value, 20);
    ptr = HdfSListNext(ptr);
    EXPECT_EQ(NODE_TO_TEST(ptr)->value, 10);
    ptr = HdfSListNext(ptr);
    EXPECT_EQ(NODE_TO_TEST(ptr)->value, 30);
}

HWTEST_F(HdfSListTest, HdfSListIteratorRemoveAll001, TestSize.Level1)
{
    for (int i = 0; i < 10; i++) {
        struct TestNode *n = CreateTestNode(i);
        HdfSListAddTail(&list, &n->node);
    }
    struct HdfSListIterator iter;
    HdfSListIteratorInit(&iter, &list);
    while (HdfSListIteratorHasNext(&iter)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&iter);
        HdfSListIteratorRemove(&iter);
        DeleteTestNode(node);
    }
    EXPECT_TRUE(HdfSListIsEmpty(&list));
    EXPECT_EQ(HdfSListCount(&list), 0);
}

/* ==================== Macros ==================== */

HWTEST_F(HdfSListTest, OffsetOfMacro001, TestSize.Level1)
{
    EXPECT_EQ(OFFSET_OF(struct TestNode, value), 0u);
    /* node is at offset sizeof(int) = 4 on most platforms */
    EXPECT_EQ(OFFSET_OF(struct TestNode, node), sizeof(int));
}

HWTEST_F(HdfSListTest, ContainerOfMacro001, TestSize.Level1)
{
    struct TestNode node;
    node.value = 42;
    struct HdfSListNode *link = &node.node;
    struct TestNode *recovered = HDF_SLIST_CONTAINER_OF(struct HdfSListNode, link, struct TestNode, node);
    EXPECT_EQ(recovered->value, 42);
    EXPECT_EQ(recovered, &node);
}

/* ==================== Stress / Large-scale ==================== */

HWTEST_F(HdfSListTest, HdfSListStressAddRemove001, TestSize.Level0)
{
    const int count = 100;
    /* Add 100 nodes */
    for (int i = 0; i < count; i++) {
        struct TestNode *n = CreateTestNode(i);
        HdfSListAddTail(&list, &n->node);
    }
    EXPECT_EQ(HdfSListCount(&list), count);
    /* Remove even-valued nodes */
    struct HdfSListIterator iter;
    HdfSListIteratorInit(&iter, &list);
    int removed = 0;
    while (HdfSListIteratorHasNext(&iter)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&iter);
        struct TestNode *tn = NODE_TO_TEST(node);
        if (tn->value % 2 == 0) {
            HdfSListIteratorRemove(&iter);
            DeleteTestNode(node);
            removed++;
        }
    }
    EXPECT_EQ(removed, count / 2);
    EXPECT_EQ(HdfSListCount(&list), count / 2);
    /* Verify only odd values remain */
    HdfSListIteratorInit(&iter, &list);
    while (HdfSListIteratorHasNext(&iter)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&iter);
        struct TestNode *tn = NODE_TO_TEST(node);
        EXPECT_NE(tn->value % 2, 0);
    }
}

HWTEST_F(HdfSListTest, HdfSListStressOrderInsert001, TestSize.Level0)
{
    int values[] = {47, 3, 19, 88, 1, 55, 22, 66, 13, 99};
    int numValues = (int)(sizeof(values) / sizeof(values[0]));
    /* Insert in random order */
    for (int i = 0; i < numValues; i++) {
        struct TestNode *n = CreateTestNode(values[i]);
        EXPECT_TRUE(HdfSListAddOrder(&list, &n->node, CompareAscending));
    }
    EXPECT_EQ(HdfSListCount(&list), numValues);
    /* Verify ascending order */
    struct HdfSListNode *iter = HdfSListPeek(&list);
    int prevVal = -1;
    while (iter != nullptr) {
        int val = NODE_TO_TEST(iter)->value;
        EXPECT_GE(val, prevVal);
        prevVal = val;
        iter = HdfSListNext(iter);
    }
}

HWTEST_F(HdfSListTest, HdfSListPopAll001, TestSize.Level1)
{
    for (int i = 0; i < 10; i++) {
        struct TestNode *n = CreateTestNode(i);
        HdfSListAddTail(&list, &n->node);
    }
    int count = 0;
    while (!HdfSListIsEmpty(&list)) {
        struct HdfSListNode *node = HdfSListPop(&list);
        ASSERT_NE(node, nullptr);
        EXPECT_EQ(NODE_TO_TEST(node)->value, count);
        DeleteTestNode(node);
        count++;
    }
    EXPECT_EQ(count, 10);
    EXPECT_TRUE(HdfSListIsEmpty(&list));
}

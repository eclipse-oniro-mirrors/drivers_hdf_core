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

#include <base/hdi_smq.h>
#include <base/hdi_smq_meta.h>
#include <hdf_base.h>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#define HDF_LOG_TAG smq_guard_test

namespace {
using namespace testing::ext;
using OHOS::HDI::Base::SharedMemQueue;
using OHOS::HDI::Base::SharedMemQueueMeta;
using OHOS::HDI::Base::SmqType;
using OHOS::HDI::Base::SharedMemQueueSyncer;

/*
 * 以下用例覆盖 fix-9-bug-0909 分支 commit 2e464fe9 对公共库 hdi_smq / hdi_smq_meta 的加固：
 *   1) SharedMemQueueMeta 构造函数：elementCount==0、elementCount 溢出 elementSize、
 *      (elementCount+1)*elementSize 32 位回绕（CVE size-lie）三处边界分支；
 *   2) SharedMemQueue(meta) 构造函数：无效 meta（size==0 / elementCount==0）拒绝分支；
 *   3) ReadNonBlocking / WriteNonBlocking / GetAvalidReadSize 未初始化与越界 offset 拒绝分支；
 *   4) SYNCED_SMQ 单生产者单消费者并发读写，验证加固不误伤正常路径。
 */

class SmqGuardTest : public testing::Test {
public:
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
    void SetUp() {};
    void TearDown() {};
};

/*
 * 用例组一：SharedMemQueueMeta 构造边界（对应 hdi_smq_meta.h 修改）
 */

// elementCount == 0：应被拒绝，size_ 保持 0
HWTEST_F(SmqGuardTest, SmqMetaElementCountZeroTest, TestSize.Level1)
{
    SharedMemQueueMeta<int32_t> meta(-1, 0, SmqType::SYNCED_SMQ);
    ASSERT_EQ(meta.GetSize(), 0u);
    ASSERT_EQ(meta.GetElementCount(), 0u);
}

// elementCount > UINT32_MAX / elementSize_：应被拒绝，size_ 保持 0
HWTEST_F(SmqGuardTest, SmqMetaElementCountOverflowElementSizeTest, TestSize.Level1)
{
    // sizeof(int32_t) == 4, UINT32_MAX / 4 == 0x3FFFFFFF
    constexpr size_t overFlowCount = 0x40000000UL; // just beyond UINT32_MAX/4
    SharedMemQueueMeta<int32_t> meta(-1, overFlowCount, SmqType::SYNCED_SMQ);
    ASSERT_EQ(meta.GetSize(), 0u);
    ASSERT_EQ(meta.GetElementCount(), overFlowCount);
}

// (elementCount+1)*elementSize 在 32 位回绕：应被 64 位中间量拦截
// 该值正是漏洞报告 PoC 中的 elementCount=0x3FFFFFFF
HWTEST_F(SmqGuardTest, SmqMetaDataSizeWrapTest, TestSize.Level1)
{
    constexpr size_t wrapCount = 0x3FFFFFFFUL; // (0x3FFFFFFF+1)*4 wraps to 0 on 32-bit
    SharedMemQueueMeta<int32_t> meta(-1, wrapCount, SmqType::SYNCED_SMQ);
    ASSERT_EQ(meta.GetSize(), 0u);
    ASSERT_EQ(meta.GetElementCount(), wrapCount);
}

// 正常构造：size_ 有效，且 data zone size 正确
HWTEST_F(SmqGuardTest, SmqMetaNormalTest, TestSize.Level1)
{
    constexpr size_t normalCount = 16;
    SharedMemQueueMeta<int32_t> meta(-1, normalCount, SmqType::SYNCED_SMQ);
    ASSERT_GT(meta.GetSize(), 0u);
    ASSERT_EQ(meta.GetElementCount(), normalCount);

    auto dataZone = meta.GetMemZone(SharedMemQueueMeta<int32_t>::MEMZONE_DATA);
    ASSERT_NE(dataZone, nullptr);
    // data zone size == (count + 1) * sizeof(int32_t)
    ASSERT_EQ(dataZone->size, (normalCount + 1) * sizeof(int32_t));
}

/*
 * 用例组二：SharedMemQueue(meta) 构造拒绝无效 meta（对应 hdi_smq.h 修改）
 */

// 用 elementCount==0 的 meta 构造：IsGood() 应为 false（拒绝分支，不 Init）
HWTEST_F(SmqGuardTest, SmqQueueCtorInvalidMetaZeroTest, TestSize.Level1)
{
    SharedMemQueueMeta<int32_t> meta(-1, 0, SmqType::SYNCED_SMQ);
    SharedMemQueue<int32_t> sq(meta);
    EXPECT_FALSE(sq.IsGood());
}

// 用 size-lie（回绕）的 meta 构造：IsGood() 应为 false
HWTEST_F(SmqGuardTest, SmqQueueCtorInvalidMetaWrapTest, TestSize.Level1)
{
    SharedMemQueueMeta<int32_t> meta(-1, 0x3FFFFFFFUL, SmqType::SYNCED_SMQ);
    SharedMemQueue<int32_t> sq(meta);
    EXPECT_FALSE(sq.IsGood());
}

// 用有效 meta 构造：IsGood() 应为 true，且可正常读写
HWTEST_F(SmqGuardTest, SmqQueueCtorValidMetaTest, TestSize.Level1)
{
    SharedMemQueue<int32_t> src(16, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(src.IsGood());
    auto metaPtr = src.GetMeta();
    ASSERT_NE(metaPtr, nullptr);

    SharedMemQueue<int32_t> sq(*metaPtr);
    ASSERT_TRUE(sq.IsGood());
    ASSERT_EQ(sq.GetSize(), src.GetSize());
}

/*
 * 用例组三：ReadNonBlocking / WriteNonBlocking / GetAvalidReadSize 未初始化与越界 offset
 */

// 未初始化（无效 meta 拒绝后指针为 nullptr）时读写应返回 HDF_ERR_INVALID_OBJECT，不崩溃
HWTEST_F(SmqGuardTest, SmqReadWriteUninitializedTest, TestSize.Level1)
{
    SharedMemQueueMeta<int32_t> badMeta(-1, 0, SmqType::SYNCED_SMQ);
    SharedMemQueue<int32_t> sq(badMeta);
    ASSERT_FALSE(sq.IsGood());

    int32_t val = 0;
    EXPECT_EQ(sq.ReadNonBlocking(&val, 1), HDF_ERR_INVALID_OBJECT);
    EXPECT_EQ(sq.WriteNonBlocking(&val, 1), HDF_ERR_INVALID_OBJECT);
    EXPECT_EQ(sq.GetAvalidReadSize(), 0u);
}

// WriteNonBlocking 越界 wOffset（栽植共享内存 write offset > elementCount）应被拒绝
HWTEST_F(SmqGuardTest, SmqWriteOutOfRangeOffsetTest, TestSize.Level1)
{
    constexpr uint32_t queueSize = 16;
    SharedMemQueue<uint32_t> sq(queueSize, SmqType::UNSYNC_SMQ);
    ASSERT_TRUE(sq.IsGood());
    auto meta = sq.GetMeta();
    ASSERT_NE(meta, nullptr);
    int fd = meta->GetFd();
    ASSERT_GE(fd, 0);

    auto wZone = meta->GetMemZone(SharedMemQueueMeta<uint32_t>::MEMZONE_WPTR);
    ASSERT_NE(wZone, nullptr);

    int pageOff = (static_cast<int>(wZone->offset) / PAGE_SIZE) * PAGE_SIZE;
    int length = static_cast<int>(wZone->offset) - pageOff + static_cast<int>(wZone->size);
    void *ptr = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pageOff);
    ASSERT_NE(ptr, MAP_FAILED);

    auto *wPtr = reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(ptr) +
        (static_cast<int>(wZone->offset) - pageOff));
    // 栽植一个越界的 write offset，模拟恶意客户端写共享内存
    *wPtr = static_cast<uint64_t>(queueSize) + 1;

    uint32_t val = 42;
    int ret = sq.WriteNonBlocking(&val, 1);
    EXPECT_EQ(ret, HDF_ERR_INVALID_OBJECT);

    munmap(ptr, length);
}

/*
 * 用例组四：并发单生产者单消费者读写（对应加固不误伤正常并发路径）
 */

HWTEST_F(SmqGuardTest, SmqConcurrentSingleProducerSingleConsumerTest, TestSize.Level1)
{
    constexpr uint32_t queueSize = 512;
    constexpr uint32_t total = 10000;
    std::shared_ptr<SharedMemQueue<uint32_t>> sq =
        std::make_shared<SharedMemQueue<uint32_t>>(queueSize, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(sq->IsGood());

    std::atomic<bool> start{false};
    std::atomic<uint64_t> produced{0};
    std::atomic<uint64_t> consumed{0};
    std::vector<uint32_t> received;
    received.reserve(total);

    std::thread producer([&start, &produced, &sq]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (uint32_t i = 0; i < total;) {
            int ret = sq->WriteNonBlocking(&i, 1);
            if (ret == 0) {
                produced.fetch_add(1, std::memory_order_relaxed);
                ++i;
            } else {
                // SYNCED_SMQ 队列满返回 -E2BIG，忙等重试
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&start, &consumed, &sq, &received]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        while (consumed.load(std::memory_order_relaxed) < total) {
            uint32_t v = 0;
            int ret = sq->ReadNonBlocking(&v, 1);
            if (ret == 0) {
                received.push_back(v);
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                // 队列空返回 -ENODATA，忙等重试
                std::this_thread::yield();
            }
        }
    });

    start.store(true, std::memory_order_release);
    producer.join();
    consumer.join();

    ASSERT_EQ(produced.load(), total);
    ASSERT_EQ(consumed.load(), total);
    ASSERT_EQ(received.size(), total);
    // 单生产者按序写入，单消费者 FIFO 读出，顺序应严格递增
    for (uint32_t i = 0; i < total; ++i) {
        ASSERT_EQ(received[i], i);
    }
}
} // namespace
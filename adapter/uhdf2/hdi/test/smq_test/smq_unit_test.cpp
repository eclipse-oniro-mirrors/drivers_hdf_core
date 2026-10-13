/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <base/hdi_smq.h>
#include <base/hdi_smq_meta.h>
#include <base/hdi_smq_syncer.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <memory>

#define HDF_LOG_TAG smq_unit_test

namespace {
using namespace testing::ext;
using OHOS::HDI::Base::SharedMemQueue;
using OHOS::HDI::Base::SharedMemQueueMeta;
using OHOS::HDI::Base::SmqType;

static constexpr uint32_t TEST_QUEUE_SIZE = 10;
static constexpr uint32_t SMALL_QUEUE_SIZE = 3;

class SmqUnitTest : public testing::Test {
public:
    void SetUp() {}
    void TearDown() {}
};

/*
 * @tc.name: SmqConstructTest001
 * @tc.desc: Verify SYNCED_SMQ construction and IsGood
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqConstructTest001, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
}

/*
 * @tc.name: SmqConstructTest002
 * @tc.desc: Verify UNSYNC_SMQ construction and IsGood
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqConstructTest002, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::UNSYNC_SMQ);
    ASSERT_TRUE(smq->IsGood());
}

/*
 * @tc.name: SmqConstructTest003
 * @tc.desc: Verify construction with zero element count (should be invalid)
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqConstructTest003, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(0, SmqType::SYNCED_SMQ);
    ASSERT_FALSE(smq->IsGood());
}

/*
 * @tc.name: SmqConstructTest004
 * @tc.desc: Verify construction with oversized element count (should be invalid)
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqConstructTest004, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(UINT16_MAX + 1, SmqType::SYNCED_SMQ);
    ASSERT_FALSE(smq->IsGood());
}

/*
 * @tc.name: SmqGetSizeTest
 * @tc.desc: Verify GetSize returns a positive value for a valid SMQ
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqGetSizeTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    size_t size = smq->GetSize();
    ASSERT_GT(size, 0);
}

/*
 * @tc.name: SmqGetMetaTest
 * @tc.desc: Verify GetMeta returns a valid metadata object
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqGetMetaTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    auto meta = smq->GetMeta();
    ASSERT_NE(meta, nullptr);
    ASSERT_EQ(meta->GetElementCount(), TEST_QUEUE_SIZE);
    ASSERT_EQ(meta->GetType(), static_cast<uint32_t>(SmqType::SYNCED_SMQ));
}

/*
 * @tc.name: SmqGetAvalidSizeTest
 * @tc.desc: Verify GetAvalidWriteSize and GetAvalidReadSize on a fresh queue
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqGetAvalidSizeTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    ASSERT_EQ(smq->GetAvalidReadSize(), 0);
    ASSERT_EQ(smq->GetAvalidWriteSize(), TEST_QUEUE_SIZE);
}

/*
 * @tc.name: SmqWriteNonBlockingSingleTest
 * @tc.desc: Verify WriteNonBlocking with a single element
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqWriteNonBlockingSingleTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    uint32_t value = 42;
    ASSERT_EQ(smq->WriteNonBlocking(&value), 0);
    ASSERT_EQ(smq->GetAvalidReadSize(), 1);
    ASSERT_EQ(smq->GetAvalidWriteSize(), TEST_QUEUE_SIZE - 1);
}

/*
 * @tc.name: SmqReadNonBlockingSingleTest
 * @tc.desc: Verify ReadNonBlocking with a single element
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqReadNonBlockingSingleTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    uint32_t writeVal = 99;
    ASSERT_EQ(smq->WriteNonBlocking(&writeVal), 0);

    uint32_t readVal = 0;
    ASSERT_EQ(smq->ReadNonBlocking(&readVal), 0);
    ASSERT_EQ(readVal, 99);
    ASSERT_EQ(smq->GetAvalidReadSize(), 0);
}

/*
 * @tc.name: SmqWriteNonBlockingArrayTest
 * @tc.desc: Verify WriteNonBlocking with an array of elements (no wrap)
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqWriteNonBlockingArrayTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    std::vector<uint32_t> data = {1, 2, 3, 4, 5};
    ASSERT_EQ(smq->WriteNonBlocking(data.data(), data.size()), 0);
    ASSERT_EQ(smq->GetAvalidReadSize(), data.size());
}

/*
 * @tc.name: SmqReadNonBlockingArrayTest
 * @tc.desc: Verify ReadNonBlocking with an array of elements
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqReadNonBlockingArrayTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    std::vector<uint32_t> writeData = {10, 20, 30};
    ASSERT_EQ(smq->WriteNonBlocking(writeData.data(), writeData.size()), 0);

    std::vector<uint32_t> readData(3, 0);
    ASSERT_EQ(smq->ReadNonBlocking(readData.data(), readData.size()), 0);
    ASSERT_EQ(readData[0], 10);
    ASSERT_EQ(readData[1], 20);
    ASSERT_EQ(readData[2], 30);
}

/*
 * @tc.name: SmqWriteNonBlockingOverflowTest
 * @tc.desc: Verify WriteNonBlocking returns error when exceeding capacity (SYNCED_SMQ)
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqWriteNonBlockingOverflowTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(SMALL_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    std::vector<uint32_t> data = {1, 2, 3, 4};
    int ret = smq->WriteNonBlocking(data.data(), data.size());
    ASSERT_NE(ret, 0);
}

/*
 * @tc.name: SmqReadNonBlockingEmptyTest
 * @tc.desc: Verify ReadNonBlocking returns error when queue is empty
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqReadNonBlockingEmptyTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    uint32_t val = 0;
    int ret = smq->ReadNonBlocking(&val);
    ASSERT_NE(ret, 0);
}

/*
 * @tc.name: SmqWriteReadNonBlockingWrapTest
 * @tc.desc: Verify ring-buffer wrap-around behavior
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqWriteReadNonBlockingWrapTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(SMALL_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());

    // Fill partially, then drain, then write again to trigger wrap
    std::vector<uint32_t> first = {1, 2};
    ASSERT_EQ(smq->WriteNonBlocking(first.data(), first.size()), 0);
    uint32_t val = 0;
    ASSERT_EQ(smq->ReadNonBlocking(&val), 0);
    ASSERT_EQ(val, 1);
    ASSERT_EQ(smq->ReadNonBlocking(&val), 0);
    ASSERT_EQ(val, 2);

    std::vector<uint32_t> second = {3, 4};
    ASSERT_EQ(smq->WriteNonBlocking(second.data(), second.size()), 0);
    ASSERT_EQ(smq->ReadNonBlocking(&val), 0);
    ASSERT_EQ(val, 3);
    ASSERT_EQ(smq->ReadNonBlocking(&val), 0);
    ASSERT_EQ(val, 4);
}

/*
 * @tc.name: SmqWriteBlockingTimeoutTest
 * @tc.desc: Verify Write with timeout on a full SYNCED_SMQ
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqWriteBlockingTimeoutTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(SMALL_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    std::vector<uint32_t> data(SMALL_QUEUE_SIZE, 1);
    ASSERT_EQ(smq->WriteNonBlocking(data.data(), data.size()), 0);
    // Queue is full; blocking write with zero timeout should fail
    uint32_t extra = 5;
    int ret = smq->Write(&extra, 1, 1);
    ASSERT_NE(ret, 0);
}

/*
 * @tc.name: SmqReadBlockingTimeoutTest
 * @tc.desc: Verify Read with timeout on an empty SYNCED_SMQ
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqReadBlockingTimeoutTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(SMALL_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    uint32_t val = 0;
    int ret = smq->Read(&val, 1, 1);
    ASSERT_NE(ret, 0);
}

/*
 * @tc.name: SmqWriteBlockingSuccessTest
 * @tc.desc: Verify Write/Read blocking round-trip in the same thread
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqWriteBlockingSuccessTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());
    uint32_t writeVal = 77;
    ASSERT_EQ(smq->Write(&writeVal), 0);
    uint32_t readVal = 0;
    ASSERT_EQ(smq->Read(&readVal), 0);
    ASSERT_EQ(readVal, 77);
}

/*
 * @tc.name: SmqUnsyncWriteReadNonBlockingTest
 * @tc.desc: Verify UNSYNC_SMQ WriteNonBlocking/ReadNonBlocking round-trip
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqUnsyncWriteReadNonBlockingTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::UNSYNC_SMQ);
    ASSERT_TRUE(smq->IsGood());
    for (uint32_t i = 0; i < TEST_QUEUE_SIZE; i++) {
        ASSERT_EQ(smq->WriteNonBlocking(&i), 0);
    }
    for (uint32_t i = 0; i < TEST_QUEUE_SIZE; i++) {
        uint32_t val = 0;
        ASSERT_EQ(smq->ReadNonBlocking(&val), 0);
        ASSERT_EQ(val, i);
    }
}

/*
 * @tc.name: SmqUnsyncBlockingNotSupportedTest
 * @tc.desc: Verify blocking Write/Read on UNSYNC_SMQ returns not-supported
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqUnsyncBlockingNotSupportedTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::UNSYNC_SMQ);
    ASSERT_TRUE(smq->IsGood());
    uint32_t val = 1;
    ASSERT_NE(smq->Write(&val, 1, 0), 0);
    ASSERT_NE(smq->Read(&val, 1, 0), 0);
}

/*
 * @tc.name: SmqResetTest
 * @tc.desc: Verify Reset clears the queue (UNSYNC_SMQ)
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqResetTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::UNSYNC_SMQ);
    ASSERT_TRUE(smq->IsGood());
    std::vector<uint32_t> data = {1, 2, 3};
    ASSERT_EQ(smq->WriteNonBlocking(data.data(), data.size()), 0);
    ASSERT_EQ(smq->GetAvalidReadSize(), data.size());

    ASSERT_EQ(smq->Reset(), HDF_SUCCESS);
    ASSERT_EQ(smq->GetAvalidReadSize(), 0);
    ASSERT_EQ(smq->GetAvalidWriteSize(), TEST_QUEUE_SIZE);
}

/*
 * @tc.name: SmqMetaMarshallingTest
 * @tc.desc: Verify SharedMemQueueMeta Marshalling/UnMarshalling round-trip
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqMetaMarshallingTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());

    OHOS::MessageParcel parcel;
    ASSERT_TRUE(smq->GetMeta()->Marshalling(parcel));
    auto recoveredMeta = SharedMemQueueMeta<uint32_t>::UnMarshalling(parcel);
    ASSERT_NE(recoveredMeta, nullptr);
    ASSERT_EQ(recoveredMeta->GetElementCount(), TEST_QUEUE_SIZE);
    ASSERT_EQ(recoveredMeta->GetType(), static_cast<uint32_t>(SmqType::SYNCED_SMQ));

    auto smq2 = std::make_shared<SharedMemQueue<uint32_t>>(*recoveredMeta);
    ASSERT_TRUE(smq2->IsGood());
}

/*
 * @tc.name: SmqConstructByMetaTest
 * @tc.desc: Verify constructing a SharedMemQueue from an existing meta (shared memory)
 * @tc.type: FUNC
 */
HWTEST_F(SmqUnitTest, SmqConstructByMetaTest, TestSize.Level1)
{
    auto smq = std::make_unique<SharedMemQueue<uint32_t>>(TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
    ASSERT_TRUE(smq->IsGood());

    uint32_t writeVal = 123;
    ASSERT_EQ(smq->WriteNonBlocking(&writeVal), 0);

    auto meta = smq->GetMeta();
    ASSERT_NE(meta, nullptr);
    auto smq2 = std::make_shared<SharedMemQueue<uint32_t>>(*meta);
    ASSERT_TRUE(smq2->IsGood());

    uint32_t readVal = 0;
    ASSERT_EQ(smq2->ReadNonBlocking(&readVal), 0);
    ASSERT_EQ(readVal, 123);
}
} // namespace

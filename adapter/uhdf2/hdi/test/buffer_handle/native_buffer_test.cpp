/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <sys/stat.h>
#include <ostream>
#include <gtest/gtest.h>
#include <message_parcel.h>
#include "buffer_util.h"
#include "native_buffer.h"

using namespace testing::ext;
using OHOS::MessageParcel;
using OHOS::sptr;
using OHOS::HDI::Base::NativeBuffer;

class NativeBufferTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    static BufferHandle *CreateBufferHandle();

    void SetUp() {};
    void TearDown() {};
};

BufferHandle *NativeBufferTest::CreateBufferHandle()
{
    BufferHandle *handle = AllocateNativeBufferHandle(1, 0);
    if (handle == nullptr) {
        std::cout << "failed to allocate buffer handle" << std::endl;
        return nullptr;
    }

    handle->fd = open("/dev/null", O_WRONLY);
    if (handle->fd == -1) {
        std::cout << "failed to open '/dev/null'" << std::endl;
        FreeNativeBufferHandle(handle);
        return nullptr;
    }
    handle->reserve[0] = dup(handle->fd);
    return handle;
}

// test nullptr native buffer
HWTEST_F(NativeBufferTest, NativeBufferTest001, TestSize.Level1)
{
    MessageParcel data;
    sptr<NativeBuffer> srcBuffer = new NativeBuffer(nullptr);
    std::cout << "srcBuffer:\n" << srcBuffer->Dump() << std::endl;
    bool ret = data.WriteStrongParcelable(srcBuffer);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = data.ReadStrongParcelable<NativeBuffer>();
    ASSERT_NE(destBuffer, nullptr);
    std::cout << "destBuffer:\n" << destBuffer->Dump() << std::endl;

    BufferHandle *destHandle = destBuffer->Move();
    ASSERT_EQ(destHandle, nullptr);
}

// test available native buffer
HWTEST_F(NativeBufferTest, NativeBufferTest002, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    MessageParcel data;
    sptr<NativeBuffer> srcBuffer = new NativeBuffer(srcHandle);
    std::cout << "srcBuffer:\n" << srcBuffer->Dump() << std::endl;
    bool ret = data.WriteStrongParcelable(srcBuffer);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = data.ReadStrongParcelable<NativeBuffer>();
    ASSERT_NE(destBuffer, nullptr);
    std::cout << "destBuffer:\n" << destBuffer->Dump() << std::endl;

    BufferHandle *destHandle = destBuffer->Move();
    ASSERT_NE(destHandle, nullptr);

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
}
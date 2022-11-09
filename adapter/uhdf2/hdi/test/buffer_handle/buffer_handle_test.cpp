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

#include <ostream>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>

#include <gtest/gtest.h>

#include "buffer_util.h"

using namespace testing::ext;

class BufferHandleTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    static BufferHandle *CreateBufferHandle();
    static std::string BufferHandleDump(const BufferHandle *handle);

    void SetUp() {};
    void TearDown() {};
};

BufferHandle *BufferHandleTest::CreateBufferHandle()
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

std::string BufferHandleTest::BufferHandleDump(const BufferHandle *handle)
{
    std::stringstream os;
    os << "{";
    if (handle == nullptr) {
        os << "}\n";
        return os.str();
    }

    os << "fd:" << handle->fd << ", ";
    os << "width:" << handle->width << ", ";
    os << "stride:" << handle->stride << ", ";
    os << "height:" << handle->height << ", ";
    os << "size:" << handle->size << ", ";
    os << "format:" << handle->format << ", ";
    os << "key:" << handle->key << ", ";
    os << "reserveFds:" << handle->reserveFds << ", ";
    os << "reserveInts:" << handle->reserveInts << ", ";
    os << "reserve: [";
    if (UINT32_MAX - handle->reserveFds >= handle->reserveInts) {
        uint32_t reserveSize = handle->reserveFds + handle->reserveInts;
        for (uint32_t i = 0; i < reserveSize; ++i) {
            os << handle->reserve[i];
            if (i + 1 < reserveSize) {
                os << ", ";
            }
        }
    }
    os << "]";
    os << "}\n";
    return os.str();
}

// allocate illegal buffer
HWTEST_F(BufferHandleTest, BufferHandleTest001, TestSize.Level1)
{
    BufferHandle *handle = AllocateNativeBufferHandle(UINT32_MAX, UINT32_MAX);
    ASSERT_EQ(handle, nullptr);
    std::cout << "handle:\n" << BufferHandleDump(handle) << std::endl;
    FreeNativeBufferHandle(handle);
}

// allocate zero size buffer
HWTEST_F(BufferHandleTest, BufferHandleTest002, TestSize.Level1)
{
    BufferHandle *handle = AllocateNativeBufferHandle(0, 0);
    ASSERT_NE(handle, nullptr);
    std::cout << "handle:\n" << BufferHandleDump(handle) << std::endl;
    FreeNativeBufferHandle(handle);
}

// test CloneNativeBufferHandle method
HWTEST_F(BufferHandleTest, BufferHandleTest003, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);
    std::cout << "srcHandle:\n" << BufferHandleDump(srcHandle) << std::endl;

    BufferHandle *destHandle = CloneNativeBufferHandle(srcHandle);
    ASSERT_NE(destHandle, nullptr);
    std::cout << "destHandle:\n" << BufferHandleDump(destHandle) << std::endl;

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
}

// test available native buffer
HWTEST_F(BufferHandleTest, BufferHandleTest004, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);
    std::cout << "srcHandle:\n" << BufferHandleDump(srcHandle) << std::endl;

    bool ret = HdfSbufWriteNativeBufferHandle(data, srcHandle);
    ASSERT_TRUE(ret);

    BufferHandle *destHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_NE(destHandle, nullptr);
    std::cout << "destHandle:\n" << BufferHandleDump(destHandle) << std::endl;

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
    HdfSbufRecycle(data);
}
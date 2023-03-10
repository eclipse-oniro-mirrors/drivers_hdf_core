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

#ifndef HDI_NATIVE_BUFFER_H
#define HDI_NATIVE_BUFFER_H

#include <message_parcel.h>
#include "buffer_handle.h"

namespace OHOS {
namespace HDI {
namespace Base {
using OHOS::MessageParcel;
using OHOS::Parcelable;

class NativeBuffer : public Parcelable {
public:
    NativeBuffer();
    virtual ~NativeBuffer();
    explicit NativeBuffer(const BufferHandle *handle);

    NativeBuffer(const NativeBuffer &other);
    NativeBuffer(NativeBuffer &&other) noexcept;

    NativeBuffer &operator=(const NativeBuffer &other);
    NativeBuffer &operator=(NativeBuffer &&other) noexcept;

    bool Marshalling(Parcel &parcel) const override;
    static sptr<NativeBuffer> Unmarshalling(Parcel &parcel);

    // clone a new BufferHandle
    BufferHandle *Clone();
    // move own BufferHandle
    BufferHandle *Move() noexcept;
    // Set BufferHandle, No ownership by NativeBuffer
    void SetBufferHandle(BufferHandle *handle, bool isOwner = false,
        std::function<void(BufferHandle *)> destructor = nullptr);
    // Get BufferHandle from NativeBuffer
    BufferHandle *GetBufferHandle() noexcept;

    std::string Dump() const;
private:
    bool ExtractFromParcel(Parcel &parcel);
    static bool WriteReserveData(MessageParcel &messageParcel, const BufferHandle &handle);
    static bool ReadReserveData(MessageParcel &messageParcel, BufferHandle &handle);
    void DestroyBuffer();
    BufferHandle *handle_;
    bool isOwner_;
    std::function<void(BufferHandle *)> bufferDestructor_;
};
} // namespace Base
} // namespace HDI
} // namespace OHOS

#endif // HDI_NATIVE_BUFFER_H

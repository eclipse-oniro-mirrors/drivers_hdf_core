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

#ifndef OHOS_HDI_BUFFER_HANDLE_PARCELABLE_H
#define OHOS_HDI_BUFFER_HANDLE_PARCELABLE_H

#include <string>
#include <vector>
#include <message_parcel.h>
#include "buffer_handle.h"

namespace OHOS {
namespace HDI {
namespace Display {
using namespace OHOS;

class BufferHandleParcelable : public Parcelable {
public:
    BufferHandleParcelable();
    explicit BufferHandleParcelable(BufferHandle &handle);
    virtual ~BufferHandleParcelable();

    bool Init(const BufferHandle &handle);
    bool ExtractFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static sptr<BufferHandleParcelable> Unmarshalling(Parcel &parcel);
    BufferHandle *GetBufferHandle();
    BufferHandle *Move();
    std::string Dump() const;

private:
    static bool WriteFileDescriptor(const int fd, Parcel &parcel);
    static int ReadFileDescriptor(Parcel &parcel);
    static void FreeBufferHandle(BufferHandle *handle);

private:
    bool init_;
    BufferHandle *handle_;
};
} // namespace Display
} // namespace HDI
} // namespace OHOS
#endif // OHOS_HDI_BUFFER_HANDLE_PARCELABLE_H

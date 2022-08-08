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

#ifndef OHOS_HDI_DISPLAY_V1_0_DISPLAY_CMD_UTILS_H
#define OHOS_HDI_DISPLAY_V1_0_DISPLAY_CMD_UTILS_H

#include "buffer_handle_utils.h"
#include "command_data_packer.h"
#include "command_data_unpacker.h"
#include "v1_0/display_composer_type.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {
using namespace OHOS::HDI::Display::Composer::V1_0;

class DisplayCmdUtils {
public:
    static constexpr int32_t MAX_INT = 0x7fffffff;
    static constexpr int32_t MIN_INT = 0x80000000;
    static constexpr uint32_t ELEMENT_SIZE = sizeof(int32_t);
    static constexpr uint32_t TRANSFER_WAIT_TIME = 100000000; // ms
    static constexpr uint32_t INIT_ELEMENT_COUNT = 32 * 1024;

    #define SWITCHCASE(x) case (x): {return #x;}
    #define PARCEL_OPS_CHECK_RET(fn, arg)                                                        \
    do {                                                                                         \
        bool ret = fn(arg);                                                                      \
        if (ret == false) {                                                                      \
            HDF_LOG("%{public}s: parcel ops failed, line = %{public}d", __func__, __LINE__);     \
            return HDF_FAILURE;                                                                  \
        }                                                                                        \
    } while (0)

    #define PARCEL_OPS_CHECK_WITHOUT_RET(fn, arg, ret)                                           \
    do {                                                                                         \
        if (ret) {                                                                               \
            ret = fn(arg);                                                                       \
        } else {                                                                                 \
            HDF_LOG("%{public}s: parcel ops failed, line = %{public}d", __func__, __LINE__);     \
        }                                                                                        \
    } while (0)

    static const char *CommandToString(int32_t cmdId)
    {
        switch (cmdId) {
            /* request cmd */
            SWITCHCASE(REQUEST_CMD_PREPAREDISPLAYLAYERS);
            SWITCHCASE(REQUEST_CMD_SETDISPLAYCLIENTBUFFER);
            SWITCHCASE(REQUEST_CMD_SETDISPLAYCLIENTDAMAGE);
            SWITCHCASE(REQUEST_CMD_COMMIT);
            SWITCHCASE(REQUEST_CMD_SETLAYERALPHA);
            SWITCHCASE(REQUEST_CMD_SETLAYERPOSITION);
            SWITCHCASE(REQUEST_CMD_SETLAYERCROP);
            SWITCHCASE(REQUEST_CMD_SETLAYERZORDER);
            SWITCHCASE(REQUEST_CMD_SETLAYERPREMULTI);
            SWITCHCASE(REQUEST_CMD_SETTRANSFORMMODE);
            SWITCHCASE(REQUEST_CMD_SETLAYERDIRTYREGION);
            SWITCHCASE(REQUEST_CMD_SETLAYERVISIBLEREGION);
            SWITCHCASE(REQUEST_CMD_SETLAYERBUFFER);
            SWITCHCASE(REQUEST_CMD_SETLAYERCOMPOSITIONTYPE);
            SWITCHCASE(REQUEST_CMD_SETLAYERBLENDTYPE);
            SWITCHCASE(REQUEST_CMD_SETLAYERVISIBLE);
            /* reply cmd */
            SWITCHCASE(REPLY_CMD_SETERROR);
            SWITCHCASE(REPLY_CMD_PREPAREDISPLAYLAYERS);
            SWITCHCASE(REPLY_CMD_COMMIT);
            /* pack control cmd */
            SWITCHCASE(CONTROL_CMD_REQUEST_BEGIN);
            SWITCHCASE(CONTROL_CMD_REPLY_BEGIN);
            SWITCHCASE(CONTROL_CMD_REQUEST_END);
            SWITCHCASE(CONTROL_CMD_REPLY_END);
            default:
                return "unknow command id.";
        }
    }

    static int32_t StartPack(int32_t cmdId, std::shared_ptr<CommandDataPacker> packer)
    {
        return packer->PackBegin(cmdId) ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t EndPack(std::shared_ptr<CommandDataPacker> packer)
    {
        return packer->PackEnd(CONTROL_CMD_REQUEST_END) ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t StartSection(int32_t cmdId, std::shared_ptr<CommandDataPacker> packer)
    {
        return packer->BeginSection(cmdId) ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t SetupDevice(uint32_t devId, uint32_t layerId, std::shared_ptr<CommandDataPacker> packer)
    {
        bool retVal = packer->WriteUint32(devId);
        if (retVal) {
            retVal = packer->WriteUint32(layerId);
        }
        return retVal ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t EndSection(std::shared_ptr<CommandDataPacker> packer)
    {
        return packer->EndSection() ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t GenerateHdifdSeqid()
    {
        static int32_t HdifdSeqidCursor = 0;

        if (HdifdSeqidCursor <= MAX_INT) {
            ++HdifdSeqidCursor;
            return HdifdSeqidCursor;
        } else {
            return 0;
        }
    }

    static bool MatchHdiFd(int32_t id, std::vector<HdifdInfo> hdiFds, int32_t &fd)
    {
        for (uint32_t i = 0; i < hdiFds.size(); ++i) {
            if (hdiFds[i].id == id) {
                fd = hdiFds[i].hdiFd->Move();
                return true;
            }
        }
        return false;
    }

    static int32_t RectPack(const IRect &rect, std::shared_ptr<CommandDataPacker> packer)
    {
        bool retVal = packer->WriteInt32(rect.x);
        if (retVal) {
            retVal = packer->WriteInt32(rect.y);
        }
        if (retVal) {
            retVal = packer->WriteInt32(rect.w);
        }
        if (retVal) {
            retVal = packer->WriteInt32(rect.h);
        }
        return retVal ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t FileDescriptorPack(
        const int32_t fd, std::shared_ptr<CommandDataPacker> packer, std::vector<HdifdInfo> &hdiFds)
    {
        int32_t ec = HDF_SUCCESS;

        HdifdInfo hdifdInfo;
        hdifdInfo.id = GenerateHdifdSeqid();
        hdifdInfo.hdiFd = new HdifdParcelable();
        if (hdifdInfo.hdiFd == nullptr) {
            HDF_LOGE("%{public}s: new HdifdParcelable failed", __func__);
            ec = HDF_FAILURE;
        } else {
            if (fd >= 0) {
                // A normal fd is transfered by binder, here just write id for unpacking to match fd.
                if (ec == HDF_SUCCESS && hdifdInfo.hdiFd->Init(fd)) {
                    hdiFds.push_back(hdifdInfo);
                    ec = packer->WriteInt32(hdifdInfo.id) ? HDF_SUCCESS : HDF_FAILURE;
                } else {
                    HDF_LOGE("%{public}s: WriteInt32 failed, line=%{public}d", __func__, __LINE__);
                    ec = HDF_FAILURE;
                }
            } else {
                // A illegal fd is transfered by smq directly.
                ec = packer->WriteInt32(fd);
                if (ec != HDF_SUCCESS {
                    HDF_LOGE("%{public}s: WriteInt32 failed, line=%{public}d", __func__, __LINE__);
                }
            }
        }
        return ec;
    }

    static int32_t BufferHandlePack(const BufferHandle &buffer, std::shared_ptr<CommandDataPacker> packer,
        std::vector<HdifdInfo> &hdiFds)
    {
        PARCEL_OPS_CHECK_RET(packer->WriteUint32, buffer.reserveFds);
        PARCEL_OPS_CHECK_RET(packer->WriteUint32, buffer.reserveInts);
        int32_t ec = FileDescriptorPack(buffer.fd, packer, hdiFds);
        if (ec != HDF_SUCCESS) {
            return ec;
        }
        PARCEL_OPS_CHECK_RET(packer->WriteInt32, buffer.width);
        PARCEL_OPS_CHECK_RET(packer->WriteInt32, buffer.stride);
        PARCEL_OPS_CHECK_RET(packer->WriteInt32, buffer.height);
        PARCEL_OPS_CHECK_RET(packer->WriteInt32, buffer.size);
        PARCEL_OPS_CHECK_RET(packer->WriteInt32, buffer.format);
        PARCEL_OPS_CHECK_RET(packer->WriteUint64, buffer.usage);
        PARCEL_OPS_CHECK_RET(packer->WriteUint64, buffer.phyAddr);
        PARCEL_OPS_CHECK_RET(packer->WriteInt32, buffer.key);

        bool retVal = true;
        int32_t i = 0;
        for (i = 0; i < buffer.reserveFds; i++) {
            ec = FileDescriptorPack(buffer.reserve[i], packer, hdiFds);
            if (ec != HDF_SUCCESS) {
                retVal = false;
                break;
            }
        }
        for (int32_t j = 0; j < buffer.reserveInts; j++) {
            retVal = packer->WriteInt32(buffer.reserve[i++]);
            if (!retVal) {
                break;
            }
        }
        return retVal ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t SetupDeviceUnpack(std::shared_ptr<CommandDataUnpacker> unpacker, uint32_t &devId, uint32_t &layerId)
    {
        bool retVal = unpacker->ReadUint32(devId);
        if (retVal) {
            retVal = unpacker->ReadUint32(layerId);
        }
        return retVal ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t RectUnpack(std::shared_ptr<CommandDataUnpacker> unpacker, IRect &rect)
    {
        bool retVal = unpacker->ReadInt32(rect.x);
        if (retVal) {
            retVal = unpacker->ReadInt32(rect.y);
        }
        if (retVal) {
            retVal = unpacker->ReadInt32(rect.w);
        }
        if (retVal) {
            retVal = unpacker->ReadInt32(rect.h);
        }
        return retVal ? HDF_SUCCESS : HDF_FAILURE;
    }

    static int32_t FileDescriptorUnpack(
        std::shared_ptr<CommandDataUnpacker> unpacker, const std::vector<HdifdInfo> &hdiFds, int32_t &fd)
    {
        int32_t fdId = -1;
        int32_t ec = unpacker->ReadInt32(fdId) ? HDF_SUCCESS : HDF_FAILURE;
        if (ec == HDF_SUCCESS && fdId >= 0) {
            ec = MatchHdiFd(fdId, hdiFds, fd) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_FAILURE) {
            // If matching failure, the illegal fd is transfered by smq directly, not by binder IPC.
            fd = fdId;
        }
        return ec;
    }

    static int32_t BufferHandleUnpack(std::shared_ptr<CommandDataUnpacker> unpacker,
        const std::vector<HdifdInfo> &hdiFds, BufferHandle *&buffer)
    {
        uint32_t fdsNum = 0;
        uint32_t intsNum = 0;
        PARCEL_OPS_CHECK_RET(unpacker->ReadUint32, fdsNum);
        PARCEL_OPS_CHECK_RET(unpacker->ReadUint32, intsNum);
        BufferHandle *handle = AllocateBufferHandle(fdsNum, intsNum);
        if (handle == nullptr) {
            return HDF_FAILURE;
        }
        handle->reserveFds = fdsNum;
        handle->reserveInts = intsNum;
        int32_t ec = FileDescriptorUnpack(unpacker, hdiFds, handle->fd);
        bool retVal = (ec == HDF_SUCCESS ? true : false);
        PARCEL_OPS_CHECK_WITHOUT_RET(unpacker->ReadInt32, handle->width, retVal);
        PARCEL_OPS_CHECK_WITHOUT_RET(unpacker->ReadInt32, handle->stride, retVal);
        PARCEL_OPS_CHECK_WITHOUT_RET(unpacker->ReadInt32, handle->height, retVal);
        PARCEL_OPS_CHECK_WITHOUT_RET(unpacker->ReadInt32, handle->size, retVal);
        PARCEL_OPS_CHECK_WITHOUT_RET(unpacker->ReadUint64, handle->usage, retVal);
        PARCEL_OPS_CHECK_WITHOUT_RET(unpacker->ReadUint64, handle->phyAddr, retVal);
        PARCEL_OPS_CHECK_WITHOUT_RET(unpacker->ReadInt32, handle->key, retVal);
        if (retVal) {
            int32_t i = 0;
            for (i = 0; i < handle->reserveFds; i++) {
                ec = FileDescriptorUnpack(unpacker, hdiFds, handle->reserve[i]);
                if (ec != HDF_SUCCESS) {
                    retVal = false;
                    break;
                }
            }
            for (int32_t j = 0; j < handle->reserveInts; j++) {
                retVal = unpacker->ReadInt32(handle->reserve[i++]);
                if (!retVal) {
                    break;
                }
            }
        }
        if (!retVal) {
            if (handle != nullptr) {
                FreeBufferHandle(handle);
                handle = nullptr;
            }
            HDF_LOGE("%{public}s: buffer handle unpack failed", __func__);
        }
        buffer = handle;
        return retVal ? HDF_SUCCESS : HDF_FAILURE;
    }
};
using CmdUtils = DisplayCmdUtils;
} // namespace V1_0
} // namespace Composer
} // namespace Display
} // namespace HDI
} // namespace OHOS
#endif // OHOS_HDI_DISPLAY_V1_0_DISPLAY_CMD_UTILS_H
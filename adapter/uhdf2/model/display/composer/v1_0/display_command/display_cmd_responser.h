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

#ifndef OHOS_HDI_DISPLAY_V1_0_DISPLAY_CMD_REQUESTER_H
#define OHOS_HDI_DISPLAY_V1_0_DISPLAY_CMD_REQUESTER_H

#include <unordered_map>
#include "hdf_base.h"
#include "hdf_log.h"
#include "hdi_smq.h"
#include "buffer_handle_parcelable.h"
#include "buffer_handle_utils.h"
#include "command_data_packer.h"
#include "command_data_unpacker.h"
#include "display_cmd_utils.h"
#include "hdifd_parcelable.h"
#include "idisplay_composer_hwi.h"
#include "v1_0/display_composer_type.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {
using namespace OHOS::HDI::Base;
using namespace OHOS::HDI::Display::Composer::V1_0;
using HdifdSet = std::vector<std::shared_ptr<HdifdParcelable>>;

template <typename Transfer, typename HalImpl>
class DisplayCmdResponser {
public:
    static std::unique_ptr<DisplayCmdResponser> Create(std::shared_ptr<HalImpl> impl)
    {
        return std::make_unique<DisplayCmdResponser>(impl);
    }

    DisplayCmdResponser(std::shared_ptr<HalImpl> halImpl)
        : impl_(halImpl),
        request_(nullptr),
        isReplyUpdated_(false),
        reply_(nullptr),
        replyCommandCnt_(0),
        replyPacker_(nullptr)
    {
    }

    ~DisplayCmdResponser() {}

    int32_t InitCmdRequest(const std::shared_ptr<Transfer> &request)
    {
        if (request_ != nullptr) {
            request_.reset();
        }
        request_ = request;

        return HDF_SUCCESS;
    }

    int32_t GetCmdReply(std::shared_ptr<Transfer> &reply)
    {
        int32_t ec = HDF_SUCCESS;
        if (isReplyUpdated_ == false) {
            ec = InitReply(CmdUtils::INIT_ELEMENT_COUNT);
        }
        if (ec == HDF_SUCCESS) {
            if (reply_ != nullptr) {
                reply = reply_;
            } else {
                ec = HDF_FAILURE;
            }
        }
        isReplyUpdated_ = false;
        if (ec != HDF_SUCCESS) {
            HDF_LOGE("error: GetCmdReply failure");
        }
        return ec;
    }

    void ProcessRequestCmd(std::shared_ptr<CommandDataUnpacker> unpacker, int32_t cmd,
        const std::vector<HdifdInfo> &inFds, std::vector<HdifdInfo> &outFds)
    {
        HDF_LOGD("PackSection, cmd-[%{public}d] = %{public}s", cmd, CmdUtils::CommandToString(cmd));
        switch (cmd) {
            case REQUEST_CMD_PREPAREDISPLAYLAYERS:
                OnPrepareDisplayLayers(unpacker);
                break;
            case REQUEST_CMD_SETDISPLAYCLIENTBUFFER:
                OnSetDisplayClientBuffer(unpacker, inFds);
                break;
            case REQUEST_CMD_SETDISPLAYCLIENTDAMAGE:
                OnSetDisplayClientDamage(unpacker);
                break;
            case REQUEST_CMD_COMMIT:
                OnCommit(unpacker, outFds);
                break;
            case REQUEST_CMD_SETLAYERALPHA:
                OnSetLayerAlpha(unpacker);
                break;
            case REQUEST_CMD_SETLAYERPOSITION:
                OnSetLayerPosition(unpacker);
                break;
            case REQUEST_CMD_SETLAYERCROP:
                OnSetLayerCrop(unpacker);
                break;
            case REQUEST_CMD_SETLAYERZORDER:
                OnSetLayerZorder(unpacker);
                break;
            case REQUEST_CMD_SETLAYERPREMULTI:
                OnSetLayerPreMulti(unpacker);
                break;
            case REQUEST_CMD_SETTRANSFORMMODE:
                OnSetTransformMode(unpacker);
                break;
            case REQUEST_CMD_SETLAYERDIRTYREGION:
                OnSetLayerDirtyRegion(unpacker);
                break;
            case REQUEST_CMD_SETLAYERVISIBLEREGION:
                OnSetLayerVisibleRegion(unpacker);
                break;
            case REQUEST_CMD_SETLAYERBUFFER:
                OnSetLayerBuffer(unpacker, inFds);
                break;
            case REQUEST_CMD_SETLAYERCOMPOSITIONTYPE:
                OnSetLayerCompositionType(unpacker);
                break;
            case REQUEST_CMD_SETLAYERBLENDTYPE:
                OnSetLayerBlendType(unpacker);
                break;
            case REQUEST_CMD_SETLAYERVISIBLE:
                OnSetLayerVisible(unpacker);
                break;
            case CONTROL_CMD_REQUEST_END:
                OnRequestEnd(unpacker);
                break;
            default:
                HDF_LOGE("error: not support display command, unpacked cmd = %{public}d", cmd);
                ec = HDF_FAILURE;
                break;
        }
    }

    int32_t CmdRequest(uint32_t inEleCnt, const std::vector<HdifdInfo> &inFds, uint32_t &outEleCnt,
        std::vector<HdifdInfo> &outFds)
    {
        std::shared_ptr<char[]> requestData(new char[inEleCnt * CmdUtils::ELEMENT_SIZE], std::default_delete<char[]>());
        int32_t ec = request_->Read(reinterpret_cast<int32_t *>(requestData.get()), inEleCnt,
            CmdUtils::TRANSFER_WAIT_TIME);

        std::shared_ptr<CommandDataUnpacker> unpacker = std::make_shared<CommandDataUnpacker>();
        if (ec == HDF_SUCCESS) {
            unpacker->Init(requestData.get(), inEleCnt * CmdUtils::ELEMENT_SIZE);
            unpacker->Dump();
        } else {
            ec = HDF_FAILURE;
        }
        int32_t unpackCmd = -1;
        if (ec != HDF_SUCCESS || unpacker->PackBegin(unpackCmd) == false) {
            HDF_LOGE("error: Check RequestBegin failed.");
            ec = HDF_FAILURE;
        }
        if (unpackCmd != CONTROL_CMD_REQUEST_BEGIN) {
            HDF_LOGI("error: unpacker PackBegin cmd not match, cmd(%{public}d)=%{public}s.", unpackCmd,
                CmdUtils::CommandToString(unpackCmd));
            ec = HDF_FAILURE;
        }

        while (ec == HDF_SUCCESS && unpacker->NextSection()) {
            if (!unpacker->BeginSection(unpackCmd)) {
                HDF_LOGE("error: PackSection failed, unpackCmd=%{public}s.", CmdUtils::CommandToString(unpackCmd));
                ec = HDF_FAILURE;
            }
            ProcessRequestCmd(unpacker, unpackCmd, inFds, outFds);
        }
        /* pack request end commands */
        replyPacker_->PackEnd(CONTROL_CMD_REPLY_END);
        HDF_LOGE("CmdReply command cnt=%{public}d", replyCommandCnt_);
        /*  Write reply pack */
        replyPacker_->Dump();
        outEleCnt = replyPacker_->ValidSize() / CmdUtils::ELEMENT_SIZE;
        ec = reply_->Write(reinterpret_cast<int32_t *>(replyPacker_->GetDataPtr()), outEleCnt,
            CmdUtils::TRANSFER_WAIT_TIME);
        if (ec != HDF_SUCCESS) {
            HDF_LOGE("Reply write failure, ec=%{public}d", ec);
            outEleCnt = 0;
        }
        int32_t ec2 = PeriodDataReset();
        return (ec == HDF_SUCCESS ? ec2 : ec);
    }

private:
    int32_t InitReply(uint32_t size)
    {
        reply_ = std::make_shared<Transfer>(size, SmqType::SYNCED_SMQ);
        if (reply_ == nullptr) {
            HDF_LOGE("nullptr error");
            return HDF_FAILURE;
        }
        replyPacker_ = std::make_shared<CommandDataPacker>();
        if (replyPacker_ == nullptr || replyPacker_->Init(reply_->GetSize() * CmdUtils::ELEMENT_SIZE) == false) {
            HDF_LOGE("replyPacker init failure");
            return HDF_FAILURE;
        }
        return CmdUtils::StartPack(CONTROL_CMD_REPLY_BEGIN, replyPacker_);
    }

    void OnRequestEnd(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        size_t errCnt = errMaps_.size();
        if (errCnt >= 0) {
            int32_t ec = CmdUtils::StartSection(REPLY_CMD_SETERROR, replyPacker_);
            bool retVal = (ec == HDF_SUCCESS) ? true : false;
            if (retVal) {
                retVal = replyPacker_->WriteUint32(errCnt);
            }
            for (auto it = errMaps_.begin(); it != errMaps_.end(); ++it) {
                if (retVal) {
                    break;
                }
                if (retVal) {
                    retVal = replyPacker_->WriteInt32(it->first);
                }
                if (retVal) {
                    retVal = replyPacker_->WriteInt32(it->second);
                }
                HDF_LOGE("Call display cmd failed, Id:%{public}s, ec=%{public}d", CmdUtils::CommandToString(it->first),
                    it->second);
            }
            if (retVal) {
                CmdUtils::EndSection(replyPacker_);
            } else {
                HDF_LOGE("OnRequestEnd failed");
            }
            replyCommandCnt_++;
        }
        return;
    }

    void OnPrepareDisplayLayers(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        int32_t ec = unpacker->ReadUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;

        bool needFlush = false;
        if (ec == HDF_SUCCESS) {
            ec = impl_->PrepareDisplayLayers(devId, needFlush);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::StartSection(REPLY_CMD_PREPAREDISPLAYLAYERS, replyPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = replyPacker_->WriteBool(needFlush) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(replyPacker_);
            replyCommandCnt_++;
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_PREPAREDISPLAYLAYERS, ec);
        }
        return;
    }

    void OnSetDisplayClientBuffer(std::shared_ptr<CommandDataUnpacker> unpacker, const std::vector<HdifdInfo> &inFds)
    {
        uint32_t devId;
        int32_t ec = unpacker->ReadUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;

        BufferHandle *buffer = nullptr;
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::BufferHandleUnpack(unpacker, inFds, buffer);
        }

        int32_t fence = -1;
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::FileDescriptorUnpack(unpacker, inFds, fence);
        }
        HdifdParcelable fdParcel(fence);

        if (ec == HDF_SUCCESS) {
            ec = impl_->SetDisplayClientBuffer(devId, *buffer, fdParcel.GetFd());
        }
        FreeBufferHandle(buffer);
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETDISPLAYCLIENTBUFFER, ec);
        }
        return;
    }

    void OnSetDisplayClientDamage(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t vectSize = 0;
        int32_t ec = unpacker->ReadUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;
        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadUint32(vectSize) ? HDF_SUCCESS : HDF_FAILURE;
        }

        std::vector<IRect> rects;
        rects.reserve(vectSize);
        for (int32_t i = 0; i < vectSize; i++) {
            if (ec == HDF_FAILURE) {
                break;
            }
            ec = CmdUtils::RectUnpack(unpacker, rects[i]);
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetDisplayClientDamage(devId, rects);
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETDISPLAYCLIENTDAMAGE, ec);
        }
        return;
    }

    void OnCommit(std::shared_ptr<CommandDataUnpacker> unpacker, std::vector<HdifdInfo> &outFds)
    {
        uint32_t devId = -1;
        int32_t fence = -1;

        int32_t ec = unpacker->ReadUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;
        if (ec == HDF_SUCCESS) {
            ec = impl_->Commit(devId, fence);
        }
        HdifdParcelable fdParcel(fence);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::StartSection(REPLY_CMD_COMMIT, replyPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::FileDescriptorPack(fdParcel.GetFd(), replyPacker_, outFds);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(replyPacker_);
            replyCommandCnt_++;
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_COMMIT, ec);
        }

        return;
    }

    void OnSetLayerAlpha(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        LayerAlpha alpha = {0};

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        bool retVal = (ec == HDF_SUCCESS) ? true : false;
        if (retVal) {
            retVal = unpacker->ReadBool(alpha.enGlobalAlpha);
        }
        if (retVal) {
            retVal = unpacker->ReadBool(alpha.enPixelAlpha);
        }
        if (retVal) {
            retVal = unpacker->ReadUint8(alpha.alpha0);
        }
        if (retVal) {
            retVal = unpacker->ReadUint8(alpha.alpha1);
        }
        if (retVal) {
            retVal = unpacker->ReadUint8(alpha.gAlpha);
        }
        if (retVal) {
            ec = impl_->SetLayerAlpha(devId, layerId, alpha);
        } else {
            ec = retVal ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERALPHA, ec);
        }
        return;
    }

    void OnSetLayerPosition(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        IRect rect = {0}

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::RectUnpack(unpacker, rect);
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerPosition(devId, layerId, rect);
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERPOSITION, ec);
        }
        return;
    }

    void OnSetLayerCrop(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        IRect rect = {0};

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::RectUnpack(unpacker, rect);
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerCrop(devId, layerId, rect);
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERCROP, ec);
        }
        return;
    }

    void OnSetLayerZorder(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        uint32_t zorder = 0;

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadUint32(zorder) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerZorder(devId, layerId, zorder);
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERZORDER, ec);
        }
        return;
    }

    void OnSetLayerPreMulti(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        bool preMulti = false;

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadBool(preMulti) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerPreMulti(devId, layerId, preMulti);
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERPREMULTI, ec);
        }
        return;
    }

    void OnSetTransformMode(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        int32_t type = 0;

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadInt32(type) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetTransformMode(devId, layerId, static_cast<TransformType>(type));
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETTRANSFORMMODE, ec);
        }
        return;
    }

    void OnSetLayerDirtyRegion(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        IRect rect = {0};

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::RectUnpack(unpacker, rect);
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerDirtyRegion(devId, layerId, rect);
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERDIRTYREGION, ec);
        }
        return;
    }

    void OnSetLayerVisibleRegion(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        uint32_t vectSize = 0;

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadUint32(vectSize) ? HDF_SUCCESS : HDF_FAILURE;
        }
        std::vector<IRect> rects;
        for (int32_t i = 0; i < vectSize; i++) {
            if (ec == HDF_FAILURE) {
                break;
            }
            IRect rect;
            ec = CmdUtils::RectUnpack(unpacker, rect);
            rects.push_back(rect);
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerVisibleRegion(devId, layerId, rects);
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERVISIBLEREGION, ec);
        }
        return;
    }

    void OnSetLayerBuffer(std::shared_ptr<CommandDataUnpacker> unpacker, const std::vector<HdifdInfo> &inFds)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        BufferHandle *buffer = nullptr;

        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::BufferHandleUnpack(unpacker, inFds, buffer);
        }
        int32_t fence = -1;
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::FileDescriptorUnpack(unpacker, inFds, fence);
        }
        HdifdParcelable fdParcel(fence);
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerBuffer(devId, layerId, *buffer, fdParcel.GetFd());
        }
        FreeBufferHandle(buffer);
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERBUFFER, ec);
        }
        return;
    }

    void OnSetLayerCompositionType(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        int32_t type;

        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadInt32(type) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerCompositionType(devId, layerId, static_cast<CompositionType>(type));
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERCOMPOSITIONTYPE, ec);
        }
        return;
    }

    void OnSetLayerBlendType(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        int32_t type;
        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadInt32(type) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerBlendType(devId, layerId, static_cast<BlendType>(type));
        }
        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERBLENDTYPE, ec);
        }
        return;
    }

    void OnSetLayerVisible(std::shared_ptr<CommandDataUnpacker> unpacker)
    {
        uint32_t devId = -1;
        uint32_t layerId = -1;
        int32_t ec = CmdUtils::SetupDeviceUnpack(unpacker, devId, layerId);
        bool visible = false;
        if (ec == HDF_SUCCESS) {
            ec = unpacker->ReadBool(visible) ? HDF_SUCCESS : HDF_FAILURE;
        }

        if (ec == HDF_SUCCESS) {
            ec = impl_->SetLayerVisible(devId, layerId, visible);
        }

        if (ec != HDF_SUCCESS) {
            errMaps_.emplace(REQUEST_CMD_SETLAYERPREMULTI, ec);
        }
        return;
    }

    int32_t PeriodDataReset()
    {
        replyCommandCnt_ = 0;
        errMaps_.clear();

        int32_t ec = CmdUtils::StartPack(CONTROL_CMD_REPLY_BEGIN, replyPacker_);
        if (ec != HDF_SUCCESS) {
            HDF_LOGE("PackBegin failure, ec=%{public}d", ec);
        }
        return ec;
    }

private:
    std::shared_ptr<HalImpl> impl_;
    std::shared_ptr<Transfer> request_;
    bool isReplyUpdated_;
    std::shared_ptr<Transfer> reply_;
    /* period data */
    uint32_t replyCommandCnt_;
    std::shared_ptr<CommandDataPacker> replyPacker_;
    std::unordered_map<int32_t, int32_t> errMaps_;
};
using HdiDisplayCmdResponser = DisplayCmdResponser<SharedMemQueue<int32_t>, IDisplayComposerHwi>;
} // namespace V1_0
} // namespace Composer
} // namespace Display
} // namespace HDI
} // namespace OHOS
#endif // OHOS_HDI_DISPLAY_V1_0_DISPLAY_CMD_REQUESTER_H
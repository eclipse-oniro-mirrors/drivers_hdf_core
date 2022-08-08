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
#include "hdf_log.h"
#include "hdi_smq.h"
#include "hdifd_parcelable.h"
#include "buffer_handle_parcelable.h"
#include "command_data_packer.h"
#include "command_data_unpacker.h"
#include "display_cmd_utils.h"
#include "v1_0/display_composer_type.h"
#include "v1_0/idisplay_composer.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {
using namespace OHOS::HDI::Base;
using namespace OHOS::HDI::Display::Composer::V1_0;

template <typename Transfer, typename CompHdi>
class DisplayCmdRequester {
public:
    DisplayCmdRequester(sptr<CompHdi> hdi)
        : initFlag_(false),
        hdi_(hdi),
        request_(nullptr),
        reply_(nullptr),
        requestPacker_(nullptr)
    {
    }

    static std::unique_ptr<DisplayCmdRequester> Create(sptr<CompHdi> hdi)
    {
        auto requester = std::make_unique<DisplayCmdRequester>(hdi);
        auto ret = requester->Init(CmdUtils::INIT_ELEMENT_COUNT);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("DisplayCmdRequester init failed");
            return nullptr;
        }
        return requester;
    }

    int32_t Init(uint32_t eleCnt)
    {
        int32_t ec = HDF_FAILURE;
        request_ = std::make_shared<Transfer>(eleCnt, SmqType::SYNCED_SMQ);
        if (request_ == nullptr) {
            HDF_LOGE("nullptr failure.");
            return HDF_FAILURE;
        } else {
            ec = hdi_->InitCmdRequest(request_);
            if (ec != HDF_SUCCESS) {
                HDF_LOGE("InitCmdRequest failure, ec=%{public}d", ec);
                return HDF_FAILURE;
            }
            requestPacker_ = std::make_shared<CommandDataPacker>();
            if (requestPacker_ == nullptr ||
                requestPacker_->Init(request_->GetSize() * CmdUtils::ELEMENT_SIZE) == false) {
                HDF_LOGE("requestPacker init failure.");
                return HDF_FAILURE;
            }
        }

        ec = hdi_->GetCmdReply(reply_);
        if (ec != HDF_SUCCESS) {
            HDF_LOGE("GetCmdReply failure, ec=%{public}d", ec);
            return ec;
        }
        initFlag_ = true;
        return CmdUtils::StartPack(CONTROL_CMD_REQUEST_BEGIN, requestPacker_);
    }

    int32_t PrepareDisplayLayers(uint32_t devId, bool &needFlushFb)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_PREPAREDISPLAYLAYERS, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndPack(requestPacker_);
        }
        size_t replyEleCnt;
        std::vector<HdifdInfo> outFds;
        std::shared_ptr<char[]> replyData;
        if (ec == HDF_SUCCESS) {
            ec = DoRequest(replyEleCnt, outFds, replyData);
        }

        if (ec == HDF_SUCCESS) {
            ec = DoReplyResults(replyEleCnt, outFds, replyData, [&](void *data) -> int32_t {
                needFlushFb = *(reinterpret_cast<bool *>(data));
                return HDF_SUCCESS;
            });
        }

        if (ec != HDF_SUCCESS) {
            HDF_LOGE("PrepareDisplayLayers failure, ec=%{public}d", ec);
        }
        return PeriodDataReset();
    }

    int32_t SetDisplayClientBuffer(uint32_t devId, const BufferHandle &buffer, int32_t fence)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETDISPLAYCLIENTBUFFER, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::BufferHandlePack(buffer, requestPacker_, requestHdiFds_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::FileDescriptorPack(fence, requestPacker_, requestHdiFds_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, ec=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetDisplayClientDamage(uint32_t devId, std::vector<IRect> &rects)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETDISPLAYCLIENTDAMAGE, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;
        }
        uint32_t vectSize = static_cast<uint32_t>(rects.size());
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteUint32(vectSize) ? HDF_SUCCESS : HDF_FAILURE;
        }
        for (int32_t i = 0; i < vectSize; i++) {
            if (ec == HDF_FAILURE) {
                break;
            }
            ec = CmdUtils::RectPack(rects[i], requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, ec=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t Commit(uint32_t devId, int32_t &fence)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_COMMIT, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteUint32(devId) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndPack(requestPacker_);
        }
        size_t replyEleCnt = 0;
        std::vector<HdifdInfo> outFds;
        std::shared_ptr<char[]> replyData;
        if (ec == HDF_SUCCESS) {
            ec = DoRequest(replyEleCnt, outFds, replyData);
        }

        if (ec == HDF_SUCCESS) {
            ec = DoReplyResults(replyEleCnt, outFds, replyData, [&](void *data) -> int32_t {
                fence = *(reinterpret_cast<int32_t *>(data));
                return HDF_SUCCESS;
            });
        }

        if (ec != HDF_SUCCESS) {
            HDF_LOGE("Commit failure, ec=%{public}d", ec);
        }
        return PeriodDataReset();
    }

    int32_t SetLayerAlpha(uint32_t devId, uint32_t layerId, const LayerAlpha &alpha)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERALPHA, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        bool retVal = (ec == HDF_SUCCESS) ? true : false;
        if (retVal) {
            retVal = requestPacker_->WriteBool(alpha.enGlobalAlpha);
        }
        if (retVal) {
            retVal = requestPacker_->WriteBool(alpha.enPixelAlpha);
        }
        if (retVal) {
            retVal = requestPacker_->WriteUint8(alpha.alpha0);
        }
        if (retVal) {
            retVal = requestPacker_->WriteUint8(alpha.alpha1);
        }
        if (retVal) {
            retVal = requestPacker_->WriteUint8(alpha.gAlpha);
        }
        if (retVal) {
            retVal = CmdUtils::EndSection(requestPacker_) == HDF_SUCCESS ? true : false;
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, retVal);
        }
        return retVal ? HDF_SUCCESS : HDF_FAILURE;
    }

    int32_t SetLayerPosition(uint32_t devId, uint32_t layerId, const IRect &rect)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERPOSITION, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::RectPack(rect, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerCrop(uint32_t devId, uint32_t layerId, const IRect &rect)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERCROP, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::RectPack(rect, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERZORDER, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteUint32(zorder) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERPREMULTI, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteBool(preMul) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetTransformMode(uint32_t devId, uint32_t layerId, TransformType type)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETTRANSFORMMODE, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteInt32(type) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, const IRect &region)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERDIRTYREGION, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::RectPack(region, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, std::vector<IRect> &rects)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERVISIBLEREGION, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        uint32_t vSize = rects.size();
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteUint32(vSize) ? HDF_SUCCESS : HDF_FAILURE;
        }
        for (uint32_t i = 0; i < vSize; i++) {
            if (ec == HDF_FAILURE) {
                break;
            }
            ec = CmdUtils::RectPack(rects[i], requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerBuffer(uint32_t devId, uint32_t layerId, BufferHandle buffer, int32_t fence)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERBUFFER, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::BufferHandlePack(buffer, requestPacker_, requestHdiFds_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::FileDescriptorPack(fence, requestPacker_, requestHdiFds_);
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, ec=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerCompositionType(uint32_t devId, uint32_t layerId, CompositionType type)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERCOMPOSITIONTYPE, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteInt32(type) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerBlendType(uint32_t devId, uint32_t layerId, BlendType type)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERBLENDTYPE, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteInt32(type) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

    int32_t SetLayerVisible(uint32_t devId, uint32_t layerId, bool visible)
    {
        int32_t ec = CmdUtils::StartSection(REQUEST_CMD_SETLAYERVISIBLE, requestPacker_);
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::SetupDevice(devId, layerId, requestPacker_);
        }
        if (ec == HDF_SUCCESS) {
            ec = requestPacker_->WriteBool(visible) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (ec == HDF_SUCCESS) {
            ec = CmdUtils::EndSection(requestPacker_);
        } else {
            HDF_LOGE("error: %{public}d@%{public}s failed, retVal=%{public}d", __LINE__, __func__, ec);
        }
        return ec;
    }

private:
    int32_t OnReplySetError(
        std::shared_ptr<CommandDataUnpacker> replyUnpacker, std::unordered_map<int32_t, int32_t> &errMaps)
    {
        int32_t ec = HDF_FAILURE;
        uint32_t errCnt = 0;
        auto retVal = replyUnpacker->ReadUint32(errCnt);
        if (retVal) {
            int32_t errCmd = -1;
            int32_t errCode = -1;
            for (; errCnt > 0; errCnt--) {
                retVal = replyUnpacker->ReadInt32(errCmd);
                if (retVal) {
                    retVal = replyUnpacker->ReadInt32(errCode);
                }
                HDF_LOGI("cmd:%{public}s, err:%{public}d", CmdUtils::CommandToString(errCmd), errCode);
                errMaps.emplace(errCmd, errCode);
            }
            ec = retVal ? HDF_SUCCESS : HDF_FAILURE;
        }

        return ec;
    }

    int32_t OnReplyPrepareDisplayLayers(std::shared_ptr<CommandDataUnpacker> replyUnpacker, bool &needFlushFb)
    {
        return replyUnpacker->ReadBool(needFlushFb) ? HDF_SUCCESS : HDF_FAILURE;
    }

    int32_t OnReplyCommit(
        std::shared_ptr<CommandDataUnpacker> replyUnpacker, std::vector<HdifdInfo> replyFds, int32_t &fenceFd)
    {
        return CmdUtils::FileDescriptorUnpack(replyUnpacker, replyFds, fenceFd);
    }

    int32_t DoReplyResults(size_t replyEleCnt, std::vector<HdifdInfo> replyFds, std::shared_ptr<char[]> replyData,
        std::function<int32_t(void *)> fn)
    {
        std::shared_ptr<CommandDataUnpacker> replyUnpacker = std::make_shared<CommandDataUnpacker>();
        replyUnpacker->Init(replyData.get(), replyEleCnt * CmdUtils::ELEMENT_SIZE);
        replyUnpacker->Dump();
        int32_t unpackCmd = -1;
        int32_t ec = HDF_SUCCESS;
        if (!replyUnpacker->PackBegin(unpackCmd)) {
            HDF_LOGE("error: PackBegin failed, unpackCmd=%{public}d.", unpackCmd);
            ec = HDF_FAILURE;
        } else {
            if (unpackCmd != CONTROL_CMD_REPLY_BEGIN) {
                HDF_LOGI("error: PackBegin cmd not match, unpackCmd=%{public}d.", unpackCmd);
                ec = HDF_FAILURE;
            }
        }
        while (ec == HDF_SUCCESS && replyUnpacker->NextSection()) {
            if (!replyUnpacker->BeginSection(unpackCmd)) {
                HDF_LOGE("error: PackSection failed, unpackCmd=%{public}d.", unpackCmd);
                ec = HDF_FAILURE;
            }
            switch (unpackCmd) {
                case REPLY_CMD_PREPAREDISPLAYLAYERS:
                    bool needFlushFb;
                    ec = OnReplyPrepareDisplayLayers(replyUnpacker, needFlushFb);
                    if (ec == HDF_SUCCESS) {
                        ec = fn(&needFlushFb);
                    }
                    if (ec != HDF_SUCCESS) {
                        HDF_LOGI("ReadBool failed, unpackCmd=%{public}s", CmdUtils::CommandToString(unpackCmd));
                    }
                    break;
                case REPLY_CMD_COMMIT:
                    int32_t fenceFd = -1;
                    ec = OnReplyCommit(replyUnpacker, replyFds, fenceFd);
                    if (ec == HDF_SUCCESS) {
                        ec = fn(&fenceFd);
                    }
                    if (ec != HDF_SUCCESS) {
                        HDF_LOGI("error: return fence fd error, unpackCmd=%{public}s",
                            CmdUtils::CommandToString(unpackCmd));
                    }
                    break;
                case REPLY_CMD_SETERROR:
                    std::unordered_map<int32_t, int32_t> errMaps;
                    ec = OnReplySetError(replyUnpacker, errMaps);
                    if (ec == HDF_SUCCESS && errMaps.size() > 0) {
                        HDF_LOGI("error: server return errs, size=%{public}d", errMaps.size());
                        ec = HDF_FAILURE;
                    }
                    break;
                default:
                    HDF_LOGE("Unpack command failure");
                    ec = HDF_FAILURE;
                    break;
            }
        }
        if (ec == HDF_SUCCESS) {
            ec = replyUnpacker->PackEnd(unpackCmd) ? HDF_SUCCESS : HDF_FAILURE;
        }
        if (unpackCmd != CONTROL_CMD_REPLY_END) {
            HDF_LOGE("error: PackEnd failed, endCmd = %{public}s", CmdUtils::CommandToString(unpackCmd));
        }
        return ec;
    }

    int32_t DoRequest(size_t &replyEleCnt, std::vector<HdifdInfo> &outFds, std::shared_ptr<char[]> &replyData)
    {
        requestPacker_->Dump();
        int32_t eleCnt = requestPacker_->ValidSize() / CmdUtils::ELEMENT_SIZE;
        int32_t ec = request_->Write(
            reinterpret_cast<int32_t *>(requestPacker_->GetDataPtr()), eleCnt, CmdUtils::TRANSFER_WAIT_TIME);
        if (ec != HDF_SUCCESS) {
            HDF_LOGE("CmdRequest write failure, ec=%{public}d", ec);
            return ec;
        }
        ec = hdi_->CmdRequest(eleCnt, requestHdiFds_, replyEleCnt, outFds);
        if (ec != HDF_SUCCESS) {
            HDF_LOGE("CmdRequest failure, ec=%{public}d", ec);
            return ec;
        }
        if (replyEleCnt != 0) {
            replyData.reset(new char[replyEleCnt * CmdUtils::ELEMENT_SIZE], std::default_delete<char[]>());
            ec = reply_->Read(reinterpret_cast<int32_t *>(replyData.get()), replyEleCnt, CmdUtils::TRANSFER_WAIT_TIME);
            if (ec != HDF_SUCCESS) {
                HDF_LOGE("reply read data failure, ec=%{public}d", ec);
            }
        }
        return ec;
    }

    int32_t PeriodDataReset()
    {
        for (uint32_t i = 0; i < requestHdiFds_.size(); ++i) {
            int32_t fd = requestHdiFds_[i].hdiFd->Move();
            if (fd != -1) {
                close(fd);
            }
        }
        requestHdiFds_.clear();
        return CmdUtils::StartPack(CONTROL_CMD_REQUEST_BEGIN, requestPacker_);
    }

private:
    bool initFlag_;
    sptr<CompHdi> hdi_;
    std::shared_ptr<Transfer> request_;
    std::shared_ptr<Transfer> reply_;
    /* Period data */
    std::shared_ptr<CommandDataPacker> requestPacker_;
    std::vector<HdifdInfo> requestHdiFds_;
};
using HdiDisplayCmdRequester = DisplayCmdRequester<SharedMemQueue<int32_t>, IDisplayComposer>;
} // namespace V1_0
} // namespace Composer
} // namespace Display
} // namespace HDI
} // namespace OHOS
#endif // OHOS_HDI_DISPLAY_V1_0_DISPLAY_CMD_REQUESTER_H
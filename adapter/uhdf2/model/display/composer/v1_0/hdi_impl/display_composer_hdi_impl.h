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

#ifndef OHOS_HDI_DISPLAY_V1_0_DISPLAY_COMPOSER_HDI_IMPL_H
#define OHOS_HDI_DISPLAY_V1_0_DISPLAY_COMPOSER_HDI_IMPL_H

#include "hdf_log.h"
#include "v1_0/display_command/display_cmd_requester.h"
#include "v1_0/display_composer_type.h"
#include "v1_0/idisplay_composer.h"
#include "v1_0/include/idisplay_composer_interface.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {
using namespace OHOS::HDI::Display::Composer::V1_0;

template <typename Interface, typename CompHdi, typename CmdReq>
class DisplayComposerHdiImpl : public Interface, public IHotPlugCallback, public IVBlankCallback {
public:
    static std::unique_ptr<IDisplayComposerInterface> create()
    {
        sptr<CompHdi> hdi = CompHdi::Get();
        std::shared_ptr<CmdReq> req = CmdReq::Create(hdi);
        return std::make_unique<DisplayComposerHdiImpl>(hdi, req);
    }

    DisplayComposerHdiImpl(sptr<Hdi> hdi, std::shared_ptr<CmdReq> req)
        : hdi_(hdi),
        req_(req),
        hotPlugCb_(nullptr),
        vBlankCb_(nullptr),
        hotPlugCbData_(nullptr),
        vBlankCbData_(nullptr)
    {
    }

    virtual ~DisplayComposerHdiImpl() {}

    // device func
    virtual int32_t RegHotPlugCallback(HotPlugCallback cb, void *data) override
    {
        hotPlugCb_ = cb;
        hotPlugCbData_ = data;
        return hdi_->RegHotPlugCallback(this);
    }

    virtual int32_t GetDisplayCapability(uint32_t devId, DisplayCapability &info) override
    {
        return hdi_->GetDisplayCapability(devId, info);
    }

    virtual int32_t GetDisplaySupportedModes(uint32_t devId, std::vector<DisplayModeInfo> &modes) override
    {
        return hdi_->GetDisplaySupportedModes(devId, modes);
    }

    virtual int32_t GetDisplayMode(uint32_t devId, uint32_t &modeId) override
    {
        return hdi_->GetDisplayMode(devId, modeId);
    }

    virtual int32_t SetDisplayMode(uint32_t devId, uint32_t modeId) override
    {
        return hdi_->SetDisplayMode(devId, modeId);
    }

    virtual int32_t GetDisplayPowerStatus(uint32_t devId, DispPowerStatus &status) override
    {
        return hdi_->GetDisplayPowerStatus(devId, status);
    }

    virtual int32_t SetDisplayPowerStatus(uint32_t devId, DispPowerStatus status) override
    {
        return hdi_->SetDisplayPowerStatus(devId, status);
    }

    virtual int32_t GetDisplayBacklight(uint32_t devId, uint32_t &level) override
    {
        return hdi_->GetDisplayBacklight(devId, level);
    }

    virtual int32_t SetDisplayBacklight(uint32_t devId, uint32_t level) override
    {
        return hdi_->SetDisplayBacklight(devId, level);
    }

    virtual int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t &value) override
    {
        return hdi_->GetDisplayProperty(devId, id, value);
    }

    virtual int32_t GetDisplayCompChange(
        uint32_t devId, std::vector<uint32_t> &layers, std::vector<int32_t> &types) override
    {
        return hdi_->GetDisplayCompChange(devId, layers, types);
    }

    virtual int32_t SetDisplayClientCrop(uint32_t devId, const IRect &rect) override
    {
        return hdi_->SetDisplayClientCrop(devId, rect);
    }

    virtual int32_t SetDisplayClientDestRect(uint32_t devId, const IRect &rect) override
    {
        return hdi_->SetDisplayClientDestRect(devId, rect);
    }

    virtual int32_t SetDisplayClientBuffer(uint32_t devId, const BufferHandle &buffer, int32_t fence) override
    {
        return req_->SetDisplayClientBuffer(devId, buffer, fence);
    }

    virtual int32_t SetDisplayClientDamage(uint32_t devId, std::vector<IRect> &rects) override
    {
        return req_->SetDisplayClientDamage(devId, rects);
    }

    virtual int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled) override
    {
        return hdi_->SetDisplayVsyncEnabled(devId, enabled);
    }

    virtual int32_t RegDisplayVBlankCallback(uint32_t devId, VBlankCallback cb, void *data) override
    {
        vBlankCb_ = cb;
        vBlankCbData_ = data;
        return hdi_->RegDisplayVBlankCallback(devId, this);
    }

    virtual int32_t GetDisplayReleaseFence(
        uint32_t devId, std::vector<uint32_t> &layers, std::vector<int32_t> &fences) override
    {
        std::vector<sptr<HdifdParcelable>> hdiFences;
        int32_t ret = hdi_->GetDisplayReleaseFence(devId, layers, hdiFences);
        if (ret == HDF_SUCCESS) {
            for (int i = 0; i < hdiFences.size(); i++) {
                fences.push_back(hdiFences[i]->Move());
            }
        }
        return ret;
    }

    virtual int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t &format, uint32_t &devId) override
    {
        return hdi_->CreateVirtualDisplay(width, height, format, devId);
    }

    virtual int32_t DestroyVirtualDisplay(uint32_t devId) override
    {
        return hdi_->DestroyVirtualDisplay(devId);
    }

    virtual int32_t SetVirtualDisplayBuffer(uint32_t devId, const BufferHandle &buffer, const int32_t fence) override
    {
        int32_t ret = HDF_SUCCESS;

        sptr<BufferHandleParcelable> hdiBuffer(new BufferHandleParcelable());
        bool bRet = hdiBuffer->Init(buffer);
        if (bRet == false) {
            ret = HDF_FAILURE;
        } else {
            sptr<HdifdParcelable> hdiFence(new HdifdParcelable);
            hdiFence->Init(fence);
            ret = hdi_->SetVirtualDisplayBuffer(devId, hdiBuffer, hdiFence);
        }

        return ret;
    }

    virtual int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value) override
    {
        return hdi_->SetDisplayProperty(devId, id, value);
    }

    virtual int32_t Commit(uint32_t devId, int32_t &fence) override
    {
        return req_->Commit(devId, fence);
    }

    // layer func
    virtual int32_t CreateLayer(uint32_t devId, const LayerInfo &layerInfo, uint32_t &layerId) override
    {
        return hdi_->CreateLayer(devId, layerInfo, layerId);
    }

    virtual int32_t DestroyLayer(uint32_t devId, uint32_t layerId) override
    {
        return hdi_->DestroyLayer(devId, layerId);
    }

    virtual int32_t PrepareDisplayLayers(uint32_t devId, bool &needFlushFb) override
    {
        return req_->PrepareDisplayLayers(devId, needFlushFb);
    }

    virtual int32_t SetLayerAlpha(uint32_t devId, uint32_t layerId, const LayerAlpha &alpha) override
    {
        return req_->SetLayerAlpha(devId, layerId, alpha);
    }

    virtual int32_t SetLayerPosition(uint32_t devId, uint32_t layerId, const IRect &rect) override
    {
        return req_->SetLayerPosition(devId, layerId, rect);
    }

    virtual int32_t SetLayerCrop(uint32_t devId, uint32_t layerId, const IRect &rect) override
    {
        return req_->SetLayerCrop(devId, layerId, rect);
    }

    virtual int32_t SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder) override
    {
        return req_->SetLayerZorder(devId, layerId, zorder);
    }

    virtual int32_t SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul) override
    {
        return req_->SetLayerPreMulti(devId, layerId, preMul);
    }

    virtual int32_t SetTransformMode(uint32_t devId, uint32_t layerId, TransformType type) override
    {
        return req_->SetTransformMode(devId, layerId, type);
    }

    virtual int32_t SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, const IRect &region) override
    {
        return req_->SetLayerDirtyRegion(devId, layerId, region);
    }

    virtual int32_t SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, std::vector<IRect> &rects) override
    {
        return req_->SetLayerVisibleRegion(devId, layerId, rects);
    }

    virtual int32_t SetLayerBuffer(uint32_t devId, uint32_t layerId, BufferHandle &buffer, int32_t fence) override
    {
        return req_->SetLayerBuffer(devId, layerId, buffer, fence);
    }

    virtual int32_t SetLayerCompositionType(uint32_t devId, uint32_t layerId, CompositionType type) override
    {
        return req_->SetLayerCompositionType(devId, layerId, type);
    }

    virtual int32_t SetLayerBlendType(uint32_t devId, uint32_t layerId, BlendType type) override
    {
        return req_->SetLayerBlendType(devId, layerId, type);
    }

    virtual int32_t SetLayerVisible(uint32_t devId, uint32_t layerId, bool visible) override
    {
        return req_->SetLayerVisible(devId, layerId, visible);
    }

    // Callback implement
    virtual int32_t OnHotPlug(uint32_t outputId, bool connected) override
    {
        int32_t ret = HDF_SUCCESS;
        if (hotPlugCb_ != nullptr) {
            hotPlugCb_(outputId, connected, hotPlugCbData_);
        } else {
            HDF_LOGE("erroe: hot plug callback fn is nullptr");
            ret = HDF_FAILURE;
        }

        return ret;
    }

    virtual int32_t OnVBlank(uint32_t sequence, uint64_t ns) override
    {
        int32_t ret = HDF_SUCCESS;
        if (vBlankCb_ != nullptr) {
            vBlankCb_(sequence, ns, vBlankCbData_);
        } else {
            HDF_LOGE("erroe: vblank callback fn is nullptr");
            ret = HDF_FAILURE;
        }

        return ret;
    }

private:
    sptr<CompHdi> hdi_;
    std::shared_ptr<CmdReq> req_;
    HotPlugCallback hotPlugCb_;
    VBlankCallback vBlankCb_;
    void *hotPlugCbData_;
    void *vBlankCbData_;
};
using HdiDisplayComposer = DisplayComposerHdiImpl<IDisplayComposerInterface, IDisplayComposer, HdiDisplayCmdRequester>;
} // namespace V1_0
} // namespace Composer
} // namespace Display
} // namespace HDI
} // namespace OHOS
#endif // OHOS_HDI_DISPLAY_V1_0_DISPLAY_COMPOSER_HDI_IMPL_H

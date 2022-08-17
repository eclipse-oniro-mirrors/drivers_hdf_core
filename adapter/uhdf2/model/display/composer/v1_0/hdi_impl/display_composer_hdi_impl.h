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

#include "hilog/log.h"
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

#define COMPOSER_CHECK_NULLPTR(ptr)                                                         \
    if (ptr == nullptr) {                                                                   \
        HILOG_ERROR(LOG_CORE, "%{public}d@%{public}s nullptr error", __LINE__, __func__);   \
        return DISPLAY_NULL_PTR;                                                            \
    }

template <typename Interface, typename CompHdi, typename CmdReq>
class DisplayComposerHdiImpl : public Interface, public IHotPlugCallback, public IVBlankCallback {
public:
    static IDisplayComposerInterface* Create()
    {
        sptr<CompHdi> hdi = CompHdi::Get();
        if (hdi == nullptr) {
            return nullptr;
        }
        std::shared_ptr<CmdReq> req = CmdReq::Create(hdi);
        if (req == nullptr) {
            return nullptr;
        }
        return new DisplayComposerHdiImpl(hdi, req);
    }

    DisplayComposerHdiImpl(sptr<CompHdi> hdi, std::shared_ptr<CmdReq> req)
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
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->RegHotPlugCallback(this));
    }

    virtual int32_t GetDisplayCapability(uint32_t devId, DisplayCapability &info) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->GetDisplayCapability(devId, info));
    }

    virtual int32_t GetDisplaySupportedModes(uint32_t devId, std::vector<DisplayModeInfo> &modes) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->GetDisplaySupportedModes(devId, modes));
    }

    virtual int32_t GetDisplayMode(uint32_t devId, uint32_t &modeId) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->GetDisplayMode(devId, modeId));
    }

    virtual int32_t SetDisplayMode(uint32_t devId, uint32_t modeId) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->SetDisplayMode(devId, modeId));
    }

    virtual int32_t GetDisplayPowerStatus(uint32_t devId, DispPowerStatus &status) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->GetDisplayPowerStatus(devId, status));
    }

    virtual int32_t SetDisplayPowerStatus(uint32_t devId, DispPowerStatus status) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->SetDisplayPowerStatus(devId, status));
    }

    virtual int32_t GetDisplayBacklight(uint32_t devId, uint32_t &level) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->GetDisplayBacklight(devId, level));
    }

    virtual int32_t SetDisplayBacklight(uint32_t devId, uint32_t level) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->SetDisplayBacklight(devId, level));
    }

    virtual int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t &value) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->GetDisplayProperty(devId, id, value));
    }

    virtual int32_t GetDisplayCompChange(
        uint32_t devId, std::vector<uint32_t> &layers, std::vector<int32_t> &types) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->GetDisplayCompChange(devId, layers, types));
    }

    virtual int32_t SetDisplayClientCrop(uint32_t devId, const IRect &rect) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->SetDisplayClientCrop(devId, rect));
    }

    virtual int32_t SetDisplayClientDestRect(uint32_t devId, const IRect &rect) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->SetDisplayClientDestRect(devId, rect));
    }

    virtual int32_t SetDisplayClientBuffer(uint32_t devId, const BufferHandle &buffer, int32_t fence) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetDisplayClientBuffer(devId, buffer, fence));
    }

    virtual int32_t SetDisplayClientDamage(uint32_t devId, std::vector<IRect> &rects) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetDisplayClientDamage(devId, rects));
    }

    virtual int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->SetDisplayVsyncEnabled(devId, enabled));
    }

    virtual int32_t RegDisplayVBlankCallback(uint32_t devId, VBlankCallback cb, void *data) override
    {
        vBlankCb_ = cb;
        vBlankCbData_ = data;
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->RegDisplayVBlankCallback(devId, this));
    }

    virtual int32_t GetDisplayReleaseFence(
        uint32_t devId, std::vector<uint32_t> &layers, std::vector<int32_t> &fences) override
    {
        std::vector<sptr<HdifdParcelable>> hdiFences;
        COMPOSER_CHECK_NULLPTR(hdi_);
        int32_t ret = ToDispErrCode(hdi_->GetDisplayReleaseFence(devId, layers, hdiFences));
        if (ret == DISPLAY_SUCCESS) {
            for (int i = 0; i < hdiFences.size(); i++) {
                fences.push_back(hdiFences[i]->Move());
            }
        }
        return ret;
    }

    virtual int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t &format, uint32_t &devId) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->CreateVirtualDisplay(width, height, format, devId));
    }

    virtual int32_t DestroyVirtualDisplay(uint32_t devId) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->DestroyVirtualDisplay(devId));
    }

    virtual int32_t SetVirtualDisplayBuffer(uint32_t devId, const BufferHandle &buffer, const int32_t fence) override
    {
        int32_t ret = DISPLAY_SUCCESS;

        sptr<BufferHandleParcelable> hdiBuffer(new BufferHandleParcelable());
        bool bRet = hdiBuffer->Init(buffer);
        if (bRet == false) {
            ret = DISPLAY_FAILURE;
        } else {
            sptr<HdifdParcelable> hdiFence(new HdifdParcelable);
            hdiFence->Init(fence);
            ret = ToDispErrCode(hdi_->SetVirtualDisplayBuffer(devId, hdiBuffer, hdiFence));
        }

        return ret;
    }

    virtual int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->SetDisplayProperty(devId, id, value));
    }

    virtual int32_t Commit(uint32_t devId, int32_t &fence) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->Commit(devId, fence));
    }

    // layer func
    virtual int32_t CreateLayer(uint32_t devId, const LayerInfo &layerInfo, uint32_t &layerId) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->CreateLayer(devId, layerInfo, layerId));
    }

    virtual int32_t DestroyLayer(uint32_t devId, uint32_t layerId) override
    {
        COMPOSER_CHECK_NULLPTR(hdi_);
        return ToDispErrCode(hdi_->DestroyLayer(devId, layerId));
    }

    virtual int32_t PrepareDisplayLayers(uint32_t devId, bool &needFlushFb) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->PrepareDisplayLayers(devId, needFlushFb));
    }

    virtual int32_t SetLayerAlpha(uint32_t devId, uint32_t layerId, const LayerAlpha &alpha) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerAlpha(devId, layerId, alpha));
    }

    virtual int32_t SetLayerPosition(uint32_t devId, uint32_t layerId, const IRect &rect) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerPosition(devId, layerId, rect));
    }

    virtual int32_t SetLayerCrop(uint32_t devId, uint32_t layerId, const IRect &rect) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerCrop(devId, layerId, rect));
    }

    virtual int32_t SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerZorder(devId, layerId, zorder));
    }

    virtual int32_t SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerPreMulti(devId, layerId, preMul));
    }

    virtual int32_t SetTransformMode(uint32_t devId, uint32_t layerId, TransformType type) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetTransformMode(devId, layerId, type));
    }

    virtual int32_t SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, const IRect &region) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerDirtyRegion(devId, layerId, region));
    }

    virtual int32_t SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, std::vector<IRect> &rects) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerVisibleRegion(devId, layerId, rects));
    }

    virtual int32_t SetLayerBuffer(uint32_t devId, uint32_t layerId, const BufferHandle &buffer, int32_t fence) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerBuffer(devId, layerId, buffer, fence));
    }

    virtual int32_t SetLayerCompositionType(uint32_t devId, uint32_t layerId, CompositionType type) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerCompositionType(devId, layerId, type));
    }

    virtual int32_t SetLayerBlendType(uint32_t devId, uint32_t layerId, BlendType type) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerBlendType(devId, layerId, type));
    }

    virtual int32_t SetLayerVisible(uint32_t devId, uint32_t layerId, bool visible) override
    {
        COMPOSER_CHECK_NULLPTR(req_);
        return ToDispErrCode(req_->SetLayerVisible(devId, layerId, visible));
    }

    // Callback implement
    virtual int32_t OnHotPlug(uint32_t outputId, bool connected) override
    {
        int32_t ret = HDF_SUCCESS;
        if (hotPlugCb_ != nullptr) {
            hotPlugCb_(outputId, connected, hotPlugCbData_);
        } else {
            HILOG_ERROR(LOG_CORE, "erroe: hot plug callback fn is nullptr");
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
            HILOG_ERROR(LOG_CORE, "erroe: vblank callback fn is nullptr");
            ret = HDF_FAILURE;
        }

        return ret;
    }

private:
    int32_t ToDispErrCode(int32_t hdfStatus)
    {
        int32_t ec = DISPLAY_FAILURE;
        switch (hdfStatus) {
            case HDF_SUCCESS:
                ec = DISPLAY_SUCCESS;
                break;
            case HDF_FAILURE:
                ec = DISPLAY_FAILURE;
                break;
            case HDF_ERR_NOT_SUPPORT:
                ec = DISPLAY_NOT_SUPPORT;
                break;
            case HDF_ERR_INVALID_PARAM:
                ec = DISPLAY_PARAM_ERR;
                break;
            case HDF_ERR_DEVICE_BUSY:
                ec = DISPLAY_SYS_BUSY;
                break;
            case HDF_ERR_BAD_FD:
                ec = DISPLAY_FD_ERR;
                break;
            case HDF_ERR_NOPERM:
                ec = DISPLAY_NOT_PERM;
                break;
            case HDF_DEV_ERR_NO_MEMORY:
                ec = DISPLAY_NOMEM;
                break;
            case HDF_ERR_IO:
            case HDF_ERR_INVALID_OBJECT:
            case HDF_ERR_MALLOC_FAIL:
            case HDF_ERR_TIMEOUT:
            case HDF_ERR_THREAD_CREATE_FAIL:
            case HDF_ERR_QUEUE_FULL:
            case HDF_BSP_ERR_OP:
            case HDF_ERR_BSP_PLT_API_ERR:
            case HDF_PAL_ERR_DEV_CREATE:
            case HDF_PAL_ERR_INNER:
            case HDF_DEV_ERR_NO_DEVICE:
            case HDF_DEV_ERR_NO_DEVICE_SERVICE:
            case HDF_DEV_ERR_DEV_INIT_FAIL:
            case HDF_DEV_ERR_PUBLISH_FAIL:
            case HDF_DEV_ERR_ATTACHDEV_FAIL:
            case HDF_DEV_ERR_NODATA:
            case HDF_DEV_ERR_NORANGE:
            case HDF_DEV_ERR_OP:
            default:
                break;
        }
        return ec;
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

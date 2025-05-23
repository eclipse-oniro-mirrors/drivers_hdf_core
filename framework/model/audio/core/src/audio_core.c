/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_core.h"
#include "audio_driver_log.h"
#include "audio_sapm.h"
#include "devmgr_service_clnt.h"
#include "osal_io.h"

#define HDF_LOG_TAG HDF_AUDIO_KADM

#define CHANNEL_MAX_NUM 2
#define CHANNEL_MIN_NUM 1
#define AUDIO_DAI_LINK_COMPLETE 1
#define AUDIO_DAI_LINK_UNCOMPLETE 0

typedef enum {
    AUDIO_CTL_ELEM_TYPE_NONE = 0, /**< Invalid type */
    AUDIO_CTL_ELEM_TYPE_BOOLEAN,
    AUDIO_CTL_ELEM_TYPE_INTEGER,
    AUDIO_CTL_ELEM_TYPE_ENUMERATED,
    AUDIO_CTL_ELEM_TYPE_BYTES,
    AUDIO_CTL_ELEM_TYPE_LAST = AUDIO_CTL_ELEM_TYPE_BYTES
} AudioCtlElemValueType;

AUDIO_LIST_HEAD(daiController);
AUDIO_LIST_HEAD(platformController);
AUDIO_LIST_HEAD(codecController);
AUDIO_LIST_HEAD(dspController);

int32_t AudioSocRegisterPlatform(struct HdfDeviceObject *device, struct PlatformData *platformData)
{
    struct PlatformDevice *platformDevice = NULL;

    if (device == NULL) {
        ADM_LOG_ERR("Input params check error: device is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (platformData == NULL) {
        ADM_LOG_ERR("Input params check error: platformData is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    platformDevice = (struct PlatformDevice *)OsalMemCalloc(sizeof(*platformDevice));
    if (platformDevice == NULL) {
        ADM_LOG_ERR("Malloc platformDevice device fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    platformDevice->devPlatformName = platformData->drvPlatformName;
    platformDevice->devData = platformData;
    platformDevice->device = device;
    DListInsertHead(&platformDevice->list, &platformController);

    ADM_LOG_INFO("Register [%s] success.", platformDevice->devPlatformName);
    return HDF_SUCCESS;
}

int32_t AudioSocRegisterDai(struct HdfDeviceObject *device, struct DaiData *daiData)
{
    struct DaiDevice *dai = NULL;

    if (device == NULL) {
        ADM_LOG_ERR("Input params check error: device is NULL");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (daiData == NULL) {
        ADM_LOG_ERR("Input params check error: daiData is NULL");
        return HDF_ERR_INVALID_OBJECT;
    }

    dai = (struct DaiDevice *)OsalMemCalloc(sizeof(*dai));
    if (dai == NULL) {
        ADM_LOG_ERR("Malloc dai device fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    dai->devDaiName = daiData->drvDaiName;
    dai->devData = daiData;
    dai->device = device;
    DListInsertHead(&dai->list, &daiController);
    ADM_LOG_INFO("Register [%s] success.", dai->devDaiName);

    return HDF_SUCCESS;
}

int32_t AudioRegisterCodec(struct HdfDeviceObject *device, struct CodecData *codecData, struct DaiData *daiData)
{
    struct CodecDevice *codec = NULL;
    int32_t ret;

    if (device == NULL) {
        ADM_LOG_ERR("Input params check error: device is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (codecData == NULL) {
        ADM_LOG_ERR("Input params check error: codecData is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (daiData == NULL) {
        ADM_LOG_ERR("Input params check error: daiData is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    codec = (struct CodecDevice *)OsalMemCalloc(sizeof(*codec));
    if (codec == NULL) {
        ADM_LOG_ERR("Malloc codec device fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    codec->devCodecName = codecData->drvCodecName;
    codec->devData = codecData;
    codec->device = device;

    ret = AudioSocRegisterDai(device, daiData);
    if (ret != HDF_SUCCESS) {
        OsalIoUnmap((void *)((uintptr_t)(codec->devData->virtualAddress)));
        OsalMemFree(codec);
        ADM_LOG_ERR("Register dai device fail ret=%d", ret);
        return HDF_ERR_IO;
    }
    DListInsertHead(&codec->list, &codecController);
    ADM_LOG_INFO("Register [%s] success.", codec->devCodecName);

    return HDF_SUCCESS;
}

int32_t AudioRegisterDsp(struct HdfDeviceObject *device, struct DspData *dspData, struct DaiData *DaiData)
{
    struct DspDevice *dspDev = NULL;
    int32_t ret;

    if (device == NULL) {
        ADM_LOG_ERR("Input params check error: device is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (dspData == NULL) {
        ADM_LOG_ERR("Input params check error: dspData is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (DaiData == NULL) {
        ADM_LOG_ERR("Input params check error: daiData is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    dspDev = (struct DspDevice *)OsalMemCalloc(sizeof(*dspDev));
    if (dspDev == NULL) {
        ADM_LOG_ERR("Malloc codec device fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    dspDev->devDspName = dspData->drvDspName;
    dspDev->devData = dspData;
    dspDev->device = device;

    ret = AudioSocRegisterDai(device, DaiData);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(dspDev);
        ADM_LOG_ERR("Register dai device fail ret=%d", ret);
        return HDF_ERR_IO;
    }
    DListInsertHead(&dspDev->list, &dspController);
    ADM_LOG_INFO("Register [%s] success.", dspDev->devDspName);

    return HDF_SUCCESS;
}

static int32_t AudioSeekPlatformDevice(struct AudioRuntimeDeivces *rtd, const struct AudioConfigData *configData)
{
    struct PlatformDevice *platform = NULL;
    if (rtd == NULL || configData == NULL) {
        ADM_LOG_ERR("Input params check error");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (configData->platformName == NULL) {
        ADM_LOG_ERR("Input devicesName check error: configData->platformName is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    ADM_LOG_DEBUG("Seek platformName[%s]", configData->platformName);
    DLIST_FOR_EACH_ENTRY(platform, &platformController, struct PlatformDevice, list) {
        if (platform->devPlatformName != NULL &&
            strcmp(platform->devPlatformName, configData->platformName) == 0) {
            rtd->platform = platform;
            break;
        }
    }

    return HDF_SUCCESS;
}

static int32_t AudioSeekCpuDaiDevice(struct AudioRuntimeDeivces *rtd, const struct AudioConfigData *configData)
{
    struct DaiDevice *cpuDai = NULL;
    if (rtd == NULL || configData == NULL) {
        ADM_LOG_ERR("Input params check error");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (configData->cpuDaiName == NULL) {
        ADM_LOG_ERR("Input cpuDaiName check error: configData->cpuDaiName is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (DListIsEmpty(&daiController)) {
        ADM_LOG_ERR("daiController is empty.");
        return HDF_FAILURE;
    }
    ADM_LOG_DEBUG("Seek cpuDaiName[%s]", configData->cpuDaiName);
    DLIST_FOR_EACH_ENTRY(cpuDai, &daiController, struct DaiDevice, list) {
        if (cpuDai == NULL) {
            return HDF_ERR_INVALID_OBJECT;
        }
        if (cpuDai->devDaiName != NULL &&
            strcmp(cpuDai->devDaiName, configData->cpuDaiName) == 0) {
            rtd->cpuDai = cpuDai;
            break;
        }
    }

    return HDF_SUCCESS;
}

static int32_t AudioSeekCodecDevice(struct AudioRuntimeDeivces *rtd, const struct AudioConfigData *configData)
{
    struct CodecDevice *codec = NULL;
    struct DaiDevice *codecDai = NULL;

    if ((rtd == NULL) || (configData == NULL)) {
        ADM_LOG_ERR("Input params check error");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (configData->codecName == NULL) {
        ADM_LOG_ERR("Input devicesName check error: configData->codecName is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (configData->codecDaiName == NULL) {
        ADM_LOG_ERR("Input devicesName check error: configData->codecDaiName is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    ADM_LOG_DEBUG ("Seek codecName[%s] codecDaiName[%s]", configData->codecName, configData->codecDaiName);
    DLIST_FOR_EACH_ENTRY(codec, &codecController, struct CodecDevice, list) {
        if (codec->devCodecName != NULL && strcmp(codec->devCodecName, configData->codecName) == 0) {
            rtd->codec = codec;
            DLIST_FOR_EACH_ENTRY(codecDai, &daiController, struct DaiDevice, list) {
                if (codecDai == NULL) {
                    return HDF_ERR_INVALID_OBJECT;
                }
                if (codecDai->device != NULL && codec->device == codecDai->device &&
                    codecDai->devDaiName != NULL && strcmp(codecDai->devDaiName, configData->codecDaiName) == 0) {
                    rtd->codecDai = codecDai;
                    break;
                }
            }
            break;
        }
    }
    return HDF_SUCCESS;
}

static int32_t AudioSeekDspDevice(struct AudioRuntimeDeivces *rtd, const struct AudioConfigData *configData)
{
    struct DspDevice *dsp = NULL;
    struct DaiDevice *dspDai = NULL;

    if ((rtd == NULL) || (configData == NULL)) {
        ADM_LOG_ERR("Input params check error");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (configData->dspName == NULL) {
        ADM_LOG_ERR("Input devicesName check error: configData->dspName is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (configData->dspDaiName == NULL) {
        ADM_LOG_ERR("Input devicesName check error: configData->dspDaiName is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    ADM_LOG_DEBUG("Seek dspName[%s] dspDaiName[%s]", configData->dspName, configData->dspDaiName);
    DLIST_FOR_EACH_ENTRY(dsp, &dspController, struct DspDevice, list) {
        if (dsp == NULL) {
            return HDF_ERR_INVALID_OBJECT;
        }
        if (dsp->devDspName != NULL && strcmp(dsp->devDspName, configData->dspName) == 0) {
            rtd->dsp = dsp;
            DLIST_FOR_EACH_ENTRY(dspDai, &daiController, struct DaiDevice, list) {
                if (dspDai == NULL) {
                    return HDF_ERR_INVALID_OBJECT;
                }
                if (dspDai->device != NULL && dsp->device == dspDai->device &&
                    dspDai->devDaiName != NULL && strcmp(dspDai->devDaiName, configData->dspDaiName) == 0) {
                    rtd->dspDai = dspDai;
                    break;
                }
            }
            break;
        }
    }

    return HDF_SUCCESS;
}

int32_t AudioBindDaiLink(struct AudioCard *audioCard, const struct AudioConfigData *configData)
{
    if (audioCard == NULL) {
        ADM_LOG_ERR("Input params check error: audioCard is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (configData == NULL) {
        ADM_LOG_ERR("Input params check error: configData is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    audioCard->rtd = (struct AudioRuntimeDeivces *)OsalMemCalloc(sizeof(struct AudioRuntimeDeivces));
    if (audioCard->rtd == NULL) {
        ADM_LOG_ERR("Malloc audioCard->rtd fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    audioCard->rtd->complete = AUDIO_DAI_LINK_UNCOMPLETE;
    if (AudioSeekPlatformDevice(audioCard->rtd, configData) == HDF_SUCCESS) {
        ADM_LOG_INFO("PLATFORM [%s] is registered!", configData->platformName);
    }
    if (AudioSeekCpuDaiDevice(audioCard->rtd, configData) == HDF_SUCCESS) {
        ADM_LOG_INFO("CPUDAI [%s] is registered!", configData->cpuDaiName);
    }
    if (AudioSeekCodecDevice(audioCard->rtd, configData) == HDF_SUCCESS) {
        ADM_LOG_INFO("CODEC [%s] is registered!", configData->codecName);
        ADM_LOG_INFO("CODECDAI [%s] is registered!", configData->codecDaiName);
    }
    if (AudioSeekDspDevice(audioCard->rtd, configData) == HDF_SUCCESS) {
        ADM_LOG_INFO("DSP [%s] is registered!", configData->dspName);
        ADM_LOG_INFO("DSPDAI [%s] is registered!", configData->dspDaiName);
    }
    audioCard->rtd->complete = AUDIO_DAI_LINK_COMPLETE;
    ADM_LOG_DEBUG("All devices register complete!");

    return HDF_SUCCESS;
}

int32_t AudioDaiReadReg(const struct DaiDevice *dai, uint32_t reg, uint32_t *val)
{
    int32_t ret;

    if (dai == NULL || dai->devData == NULL || dai->devData->Read == NULL || val == NULL) {
        ADM_LOG_ERR("Input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = dai->devData->Read(dai, reg, val);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("dai device read fail.");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioDaiWriteReg(const struct DaiDevice *dai, uint32_t reg, uint32_t val)
{
    int32_t ret;
    if (dai == NULL || dai->devData == NULL || dai->devData->Write == NULL) {
        ADM_LOG_ERR("Input param codec is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = dai->devData->Write(dai, reg, val);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("dai device write fail.");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioCodecReadReg(const struct CodecDevice *codec, uint32_t reg, uint32_t *val)
{
    int32_t ret;

    if (codec == NULL || codec->devData == NULL || codec->devData->Read == NULL || val == NULL) {
        ADM_LOG_ERR("Input param codec is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = codec->devData->Read(codec, reg, val);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("Codec device read fail.");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioCodecWriteReg(const struct CodecDevice *codec, uint32_t reg, uint32_t val)
{
    int32_t ret;

    if (codec == NULL || codec->devData == NULL || codec->devData->Write == NULL) {
        ADM_LOG_ERR("Input param codec is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = codec->devData->Write(codec, reg, val);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("Codec device write fail.");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioUpdateCodecRegBits(struct CodecDevice *codec, uint32_t reg,
    const uint32_t mask, const uint32_t shift, uint32_t value)
{
    int32_t ret;
    uint32_t curValue = 0;
    uint32_t controlMask;
    uint32_t tempVal;
    if (codec == NULL || codec->devData == NULL) {
        ADM_LOG_ERR("Invalid input param.");
        return HDF_ERR_INVALID_OBJECT;
    }

    value = value << shift;
    controlMask = mask << shift;

    OsalMutexLock(&codec->devData->mutex);
    ret = AudioCodecReadReg(codec, reg, &curValue);
    if (ret != HDF_SUCCESS) {
        OsalMutexUnlock(&codec->devData->mutex);
        ADM_LOG_ERR("Read reg fail ret=%d.", ret);
        return HDF_FAILURE;
    }

    tempVal = curValue & controlMask;
    if (tempVal == value) {
        OsalMutexUnlock(&codec->devData->mutex);
        ADM_LOG_DEBUG("Success.");
        return HDF_SUCCESS;
    }
    curValue = (curValue & ~controlMask) | (value & controlMask);
    ret = AudioCodecWriteReg(codec, reg, curValue);
    if (ret != HDF_SUCCESS) {
        OsalMutexUnlock(&codec->devData->mutex);
        ADM_LOG_ERR("Write reg fail ret=%d", ret);
        return HDF_FAILURE;
    }
    OsalMutexUnlock(&codec->devData->mutex);

    ADM_LOG_DEBUG("Success.");
    return HDF_SUCCESS;
}

int32_t AudioUpdateDaiRegBits(const struct DaiDevice *dai, uint32_t reg,
    const uint32_t mask, const uint32_t shift, uint32_t value)
{
    int32_t ret;
    uint32_t curValue = 0;
    uint32_t mixerControlMask;
    uint32_t tempVal;
    struct DaiData *data = NULL;

    if (dai == NULL || dai->devData == NULL) {
        ADM_LOG_ERR("Invalid input param.");
        return HDF_ERR_INVALID_OBJECT;
    }
    data = dai->devData;
    value = value << shift;
    mixerControlMask = mask << shift;

    OsalMutexLock(&data->mutex);
    ret = AudioDaiReadReg(dai, reg, &curValue);
    if (ret != HDF_SUCCESS) {
        OsalMutexUnlock(&data->mutex);
        ADM_LOG_ERR("Read reg fail ret=%d.", ret);
        return HDF_FAILURE;
    }

    tempVal = curValue & mixerControlMask;
    if (tempVal == value) {
        OsalMutexUnlock(&data->mutex);
        ADM_LOG_DEBUG("Success.");
        return HDF_SUCCESS;
    }
    curValue = (curValue & ~mixerControlMask) | (value & mixerControlMask);
    ret = AudioDaiWriteReg(dai, reg, curValue);
    if (ret != HDF_SUCCESS) {
        OsalMutexUnlock(&data->mutex);
        ADM_LOG_ERR("Write reg fail ret=%d", ret);
        return HDF_FAILURE;
    }
    OsalMutexUnlock(&data->mutex);

    ADM_LOG_DEBUG("Success.");
    return HDF_SUCCESS;
}

int32_t AudioCodecRegUpdate(struct CodecDevice *codec, struct AudioMixerControl *mixerCtrl)
{
    uint32_t mixerValue;
    if (codec == NULL || mixerCtrl == NULL) {
        ADM_LOG_ERR("AudioKcontrolGetCodec is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerValue = mixerCtrl->value;
    if (mixerValue < mixerCtrl->min || mixerValue > mixerCtrl->max) {
        ADM_LOG_ERR("Audio codec invalid value=%u", mixerValue);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (mixerCtrl->invert) {
        mixerValue = mixerCtrl->max - mixerCtrl->value;
    }
    if (AudioUpdateCodecRegBits(codec, mixerCtrl->reg, mixerCtrl->mask, mixerCtrl->shift, mixerValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
        return HDF_FAILURE;
    }

    if (mixerCtrl->reg != mixerCtrl->rreg || mixerCtrl->shift != mixerCtrl->rshift) {
        if (AudioUpdateCodecRegBits(codec, mixerCtrl->rreg, mixerCtrl->mask,
            mixerCtrl->rshift, mixerValue) != HDF_SUCCESS) {
            ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

int32_t AudioCodecMuxRegUpdate(struct CodecDevice *codec, struct AudioEnumKcontrol *enumCtrl, const uint32_t *value)
{
    uint32_t val[2];
    int32_t ret;

    if (codec == NULL || enumCtrl == NULL || value == NULL) {
        ADM_LOG_ERR("input para is null.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (enumCtrl->values != NULL) {
        val[0] = enumCtrl->values[value[0]];
        val[1] = enumCtrl->values[value[1]];
    } else {
        val[0] = value[0];
        val[1] = value[1];
    }

    if (val[0] > enumCtrl->max) {
        ADM_LOG_ERR("Audio invalid value=%u", val[0]);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = AudioUpdateCodecRegBits(codec, enumCtrl->reg, enumCtrl->mask, enumCtrl->shiftLeft, val[0]);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("update left reg bits fail!");
        return ret;
    }

    if (enumCtrl->reg != enumCtrl->reg2 || enumCtrl->shiftLeft != enumCtrl->shiftRight) {
        if (val[1] > enumCtrl->max) {
            ADM_LOG_ERR("Audio invalid value=%u", val[1]);
            return HDF_ERR_INVALID_OBJECT;
        }
        ret = AudioUpdateCodecRegBits(codec, enumCtrl->reg2, enumCtrl->mask, enumCtrl->shiftRight, val[1]);
        if (ret != HDF_SUCCESS) {
            ADM_LOG_ERR("update right reg bits fail!");
            return ret;
        }
    }
    return HDF_SUCCESS;
}

int32_t AudioDaiRegUpdate(const struct DaiDevice *dai, struct AudioMixerControl *mixerCtrl)
{
    uint32_t value;

    if (dai == NULL || mixerCtrl == NULL) {
        ADM_LOG_ERR("AudioKcontrolGetCodec is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }

    value = mixerCtrl->value;
    if (value < mixerCtrl->min || value > mixerCtrl->max) {
        ADM_LOG_ERR("Audio dai invalid value=%u", value);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (mixerCtrl->invert) {
        value = mixerCtrl->max - mixerCtrl->value;
    }

    if (AudioUpdateDaiRegBits(dai, mixerCtrl->reg, mixerCtrl->mask, mixerCtrl->shift, value) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
        return HDF_FAILURE;
    }

    if (mixerCtrl->reg != mixerCtrl->rreg || mixerCtrl->shift != mixerCtrl->rshift) {
        if (AudioUpdateDaiRegBits(dai, mixerCtrl->rreg, mixerCtrl->mask,
            mixerCtrl->rshift, value) != HDF_SUCCESS) {
            ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

struct CodecDevice *AudioKcontrolGetCodec(const struct AudioKcontrol *kcontrol)
{
    struct AudioCard *audioCard = NULL;
    if (kcontrol == NULL || kcontrol->pri == NULL) {
        ADM_LOG_ERR("Get Codec input param kcontrol is NULL.");
        return NULL;
    }

    audioCard = (struct AudioCard *)((volatile uintptr_t)(kcontrol->pri));
    if (audioCard->rtd == NULL) {
        ADM_LOG_ERR("Get audio card or rtd fail.");
        return NULL;
    }

    return audioCard->rtd->codec;
}

struct DaiDevice *AudioKcontrolGetCpuDai(const struct AudioKcontrol *kcontrol)
{
    struct AudioCard *audioCard = NULL;
    if (kcontrol == NULL || kcontrol->pri == NULL) {
        ADM_LOG_ERR("Get CpuDai input param kcontrol is NULL.");
        return NULL;
    }

    audioCard = (struct AudioCard *)((volatile uintptr_t)(kcontrol->pri));
    if (audioCard->rtd == NULL) {
        ADM_LOG_ERR("Get audio card or rtd fail.");
        return NULL;
    }

    return audioCard->rtd->cpuDai;
}

struct AudioKcontrol *AudioAddControl(const struct AudioCard *audioCard, const struct AudioKcontrol *ctrl)
{
    struct AudioKcontrol *control = NULL;

    if ((audioCard == NULL) || (ctrl == NULL)) {
        ADM_LOG_ERR("Input params check error");
        return NULL;
    }

    control = (struct AudioKcontrol *)OsalMemCalloc(sizeof(*control));
    if (control == NULL) {
        ADM_LOG_ERR("Malloc control fail!");
        return NULL;
    }
    DListHeadInit(&control->list);
    control->name = ctrl->name;
    control->iface = ctrl->iface;
    control->Info = ctrl->Info;
    control->Get = ctrl->Get;
    control->Set = ctrl->Set;
    control->pri = (void *)audioCard;
    control->privateValue = ctrl->privateValue;

    return control;
}

int32_t AudioAddControls(struct AudioCard *audioCard, const struct AudioKcontrol *controls, int32_t controlMaxNum)
{
    struct AudioKcontrol *control = NULL;
    int32_t i;
    ADM_LOG_DEBUG("Entry.");

    if (audioCard == NULL) {
        ADM_LOG_ERR("Input params check error: audioCard is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (controls == NULL) {
        ADM_LOG_ERR("Input params check error: controls is NULL.");
        return HDF_FAILURE;
    }
    if (controlMaxNum <= 0) {
        ADM_LOG_ERR("Input params check error: controlMaxNum=%d.", controlMaxNum);
        return HDF_FAILURE;
    }

    for (i = 0; i < controlMaxNum; i++) {
        control = AudioAddControl(audioCard, &controls[i]);
        if (control == NULL) {
            ADM_LOG_ERR("Add control fail!");
            return HDF_FAILURE;
        }
        DListInsertHead(&control->list, &audioCard->controls);
    }
    ADM_LOG_DEBUG("Success.");
    return HDF_SUCCESS;
}

int32_t AudioInfoCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemInfo *elemInfo)
{
    struct AudioMixerControl *mixerCtrl = NULL;

    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemInfo == NULL) {
        ADM_LOG_ERR("Input param kcontrol is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)(kcontrol->privateValue));
    if (mixerCtrl->reg != mixerCtrl->rreg || mixerCtrl->shift != mixerCtrl->rshift) {
        /* stereo */
        elemInfo->count = CHANNEL_MAX_NUM;
    } else {
        elemInfo->count = CHANNEL_MIN_NUM;
    }
    elemInfo->type = AUDIO_CTL_ELEM_TYPE_INTEGER;
    elemInfo->min = mixerCtrl->min;
    elemInfo->max = mixerCtrl->max;

    return HDF_SUCCESS;
}

int32_t AudioInfoEnumCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemInfo *elemInfo)
{
    struct AudioEnumKcontrol *enumCtrl = NULL;

    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemInfo == NULL) {
        ADM_LOG_ERR("Input param kcontrol is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    enumCtrl = (struct AudioEnumKcontrol *)((volatile uintptr_t)(kcontrol->privateValue));
    if (enumCtrl->reg != enumCtrl->reg2 || enumCtrl->shiftLeft != enumCtrl->shiftRight) {
        /* stereo */
        elemInfo->count = CHANNEL_MAX_NUM;
    } else {
        elemInfo->count = CHANNEL_MIN_NUM;
    }
    elemInfo->type = AUDIO_CTL_ELEM_TYPE_ENUMERATED;
    elemInfo->min = 0;
    elemInfo->max = enumCtrl->max;

    return HDF_SUCCESS;
}

int32_t AudioGetCtrlOpsRReg(struct AudioCtrlElemValue *elemValue,
    const struct AudioMixerControl *mixerCtrl, uint32_t rcurValue)
{
    if (elemValue == NULL || mixerCtrl == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (mixerCtrl->reg != mixerCtrl->rreg || mixerCtrl->shift != mixerCtrl->rshift) {
        if (mixerCtrl->reg == mixerCtrl->rreg) {
            rcurValue = (rcurValue >> mixerCtrl->rshift) & mixerCtrl->mask;
        } else {
            rcurValue = (rcurValue >> mixerCtrl->shift) & mixerCtrl->mask;
        }
        if (rcurValue > mixerCtrl->max || rcurValue < mixerCtrl->min) {
            ADM_LOG_ERR("Audio invalid rcurValue=%u", rcurValue);
            return HDF_FAILURE;
        }
        if (mixerCtrl->invert) {
            rcurValue = mixerCtrl->max - rcurValue;
        }
        elemValue->value[1] = rcurValue;
    }

    return HDF_SUCCESS;
}

int32_t AudioGetCtrlOpsReg(struct AudioCtrlElemValue *elemValue,
    const struct AudioMixerControl *mixerCtrl, uint32_t curValue)
{
    if (elemValue == NULL || mixerCtrl == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    curValue = (curValue >> mixerCtrl->shift) & mixerCtrl->mask;
    if (curValue > mixerCtrl->max || curValue < mixerCtrl->min) {
        ADM_LOG_ERR("Audio invalid curValue=%u", curValue);
        return HDF_FAILURE;
    }
    if (mixerCtrl->invert) {
        curValue = mixerCtrl->max - curValue;
    }
    elemValue->value[0] = curValue;

    return HDF_SUCCESS;
}

static int32_t AudioGetEnumCtrlOpsReg(struct AudioCtrlElemValue *elemValue,
    const struct AudioEnumKcontrol *enumCtrl, uint32_t curValue)
{
    if (elemValue == NULL || enumCtrl == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    curValue = (curValue >> enumCtrl->shiftLeft) & enumCtrl->mask;
    if (curValue > enumCtrl->max) {
        ADM_LOG_ERR("Audio invalid curValue=%u", curValue);
        return HDF_FAILURE;
    }

    elemValue->value[0] = curValue;

    return HDF_SUCCESS;
}

static int32_t AudioGetEnumCtrlOpsRReg(struct AudioCtrlElemValue *elemValue,
    const struct AudioEnumKcontrol *enumCtrl, uint32_t rcurValue)
{
    if (elemValue == NULL || enumCtrl == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (enumCtrl->reg != enumCtrl->reg2 || enumCtrl->shiftLeft != enumCtrl->shiftRight) {
        rcurValue = (rcurValue >> enumCtrl->shiftLeft) & enumCtrl->mask;

        if (rcurValue > enumCtrl->max) {
            ADM_LOG_ERR("Audio invalid rcurValue=%u", rcurValue);
            return HDF_FAILURE;
        }

        elemValue->value[1] = rcurValue;
    }

    return HDF_SUCCESS;
}

int32_t AudioCodecGetCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    uint32_t curValue = 0;
    uint32_t rcurValue = 0;
    struct AudioMixerControl *mixerCtrl = NULL;
    struct CodecDevice *codec = NULL;
    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemValue == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    codec = AudioKcontrolGetCodec(kcontrol);
    if (codec == NULL) {
        ADM_LOG_ERR("mixerCtrl and codec is NULL.");
        return HDF_FAILURE;
    }
    if (AudioCodecReadReg(codec, mixerCtrl->reg, &curValue) != HDF_SUCCESS ||
        AudioCodecReadReg(codec, mixerCtrl->rreg, &rcurValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Read Codec Reg fail.");
        return HDF_FAILURE;
    }
    if (AudioGetCtrlOpsReg(elemValue, mixerCtrl, curValue) != HDF_SUCCESS ||
        AudioGetCtrlOpsRReg(elemValue, mixerCtrl, rcurValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec get kcontrol reg and rreg fail.");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioCodecGetEnumCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    uint32_t curValue = 0;
    uint32_t rcurValue = 0;
    struct AudioEnumKcontrol *enumCtrl = NULL;
    struct CodecDevice *codec = NULL;
    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemValue == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    enumCtrl = (struct AudioEnumKcontrol *)((volatile uintptr_t)kcontrol->privateValue);
    codec = AudioKcontrolGetCodec(kcontrol);
    if (codec == NULL) {
        ADM_LOG_ERR("mixerCtrl and codec is NULL.");
        return HDF_FAILURE;
    }
    if (AudioCodecReadReg(codec, enumCtrl->reg, &curValue) != HDF_SUCCESS ||
        AudioCodecReadReg(codec, enumCtrl->reg2, &rcurValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Read Reg fail.");
        return HDF_FAILURE;
    }

    if (AudioGetEnumCtrlOpsReg(elemValue, enumCtrl, curValue) != HDF_SUCCESS ||
        AudioGetEnumCtrlOpsRReg(elemValue, enumCtrl, rcurValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec get kcontrol reg and rreg fail.");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioSetCtrlOpsReg(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue,
    const struct AudioMixerControl *mixerCtrl, uint32_t *value)
{
    uint32_t val;

    if (kcontrol == NULL || (kcontrol->privateValue <= 0) || elemValue == NULL ||
        mixerCtrl == NULL || value == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    val = elemValue->value[0];
    if (val < mixerCtrl->min || val > mixerCtrl->max) {
        ADM_LOG_ERR("Audio invalid value=%u", val);
        return HDF_ERR_INVALID_OBJECT;
    }
    *value = mixerCtrl->invert == 0 ? val : mixerCtrl->max - val;

    return HDF_SUCCESS;
}

int32_t AudioSetCtrlOpsRReg(const struct AudioCtrlElemValue *elemValue,
    struct AudioMixerControl *mixerCtrl, uint32_t *rvalue, bool *updateRReg)
{
    uint32_t val;

    if (elemValue == NULL || mixerCtrl == NULL || rvalue == NULL || updateRReg == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (mixerCtrl->reg != mixerCtrl->rreg || mixerCtrl->shift != mixerCtrl->rshift) {
        val = elemValue->value[1];
        if (val < mixerCtrl->min || val > mixerCtrl->max) {
            ADM_LOG_ERR("Audio invalid fail.");
            return HDF_FAILURE;
        }

        if (mixerCtrl->reg == mixerCtrl->rreg) {
            mixerCtrl->shift = mixerCtrl->rshift;
        }
        *rvalue = mixerCtrl->invert == 0 ? val : mixerCtrl->max - val;
        *updateRReg = true;
    }

    return HDF_SUCCESS;
}

int32_t AudioCodecSetCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue)
{
    uint32_t value = 0;
    uint32_t rvalue = 0;
    bool updateRReg = false;
    struct CodecDevice *codec = NULL;
    struct AudioMixerControl *mixerCtrl = NULL;

    if (kcontrol == NULL || (kcontrol->privateValue <= 0) || elemValue == NULL) {
        ADM_LOG_ERR("Audio codec set control register input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    codec = AudioKcontrolGetCodec(kcontrol);
    if (codec == NULL) {
        ADM_LOG_ERR("AudioKcontrolGetCodec is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    if (AudioSetCtrlOpsReg(kcontrol, elemValue, mixerCtrl, &value) != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioSetCtrlOpsReg is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (AudioUpdateCodecRegBits(codec, mixerCtrl->reg, mixerCtrl->mask, mixerCtrl->shift, value) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
        return HDF_FAILURE;
    }

    if (AudioSetCtrlOpsRReg(elemValue, mixerCtrl, &rvalue, &updateRReg) != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioSetCtrlOpsRReg is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (updateRReg) {
        if (AudioUpdateCodecRegBits(codec, mixerCtrl->rreg, mixerCtrl->mask,
            mixerCtrl->rshift, rvalue) != HDF_SUCCESS) {
            ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t AudioCodecSetEnumRegUpdate(struct CodecDevice *codec, const struct AudioEnumKcontrol *enumCtrl,
    const uint32_t *value)
{
    uint32_t setVal[2];
    int32_t ret;

    if (codec == NULL || enumCtrl == NULL || value == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (enumCtrl->values != NULL) {
        setVal[0] = enumCtrl->values[value[0]];
        setVal[1] = enumCtrl->values[value[1]];
    } else {
        setVal[0] = value[0];
        setVal[1] = value[1];
    }

    if (setVal[0] > enumCtrl->max) {
        ADM_LOG_ERR("Audio invalid value=%u", setVal[0]);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = AudioUpdateCodecRegBits(codec, enumCtrl->reg, enumCtrl->mask, enumCtrl->shiftLeft, setVal[0]);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
        return HDF_FAILURE;
    }

    if (enumCtrl->reg != enumCtrl->reg2 || enumCtrl->shiftLeft != enumCtrl->shiftRight) {
        if (setVal[1] > enumCtrl->max) {
            ADM_LOG_ERR("Audio invalid value=%u", setVal[1]);
            return HDF_ERR_INVALID_OBJECT;
        }
        ret = AudioUpdateCodecRegBits(codec, enumCtrl->reg2, enumCtrl->mask, enumCtrl->shiftRight, setVal[1]);
        if (ret != HDF_SUCCESS) {
            ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

int32_t AudioCodecSetEnumCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue)
{
    struct CodecDevice *codec = NULL;
    struct AudioEnumKcontrol *enumCtrl = NULL;
    int32_t ret;
    if (kcontrol == NULL || (kcontrol->privateValue <= 0) || elemValue == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    codec = AudioKcontrolGetCodec(kcontrol);
    if (codec == NULL) {
        ADM_LOG_ERR("AudioKcontrolGetCodec is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }
    enumCtrl = (struct AudioEnumKcontrol *)((volatile uintptr_t)kcontrol->privateValue);
    ret = AudioCodecSetEnumRegUpdate(codec, enumCtrl, (uint32_t *)elemValue->value);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioCodecSetEnumRegUpdate fail.");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioCpuDaiSetCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue)
{
    uint32_t value;
    uint32_t rvalue;
    bool updateRReg = false;
    struct DaiDevice *dai = NULL;
    struct AudioMixerControl *mixerCtrl = NULL;
    if (kcontrol == NULL || (kcontrol->privateValue <= 0) || elemValue == NULL) {
        ADM_LOG_ERR("Audio cpu dai set control register input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    if (AudioSetCtrlOpsReg(kcontrol, elemValue, mixerCtrl, &value) != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioSetCtrlOpsReg is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }
    dai = AudioKcontrolGetCpuDai(kcontrol);
    if (dai == NULL) {
        ADM_LOG_ERR("AudioKcontrolGetCodec is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (AudioUpdateDaiRegBits(dai, mixerCtrl->reg, mixerCtrl->mask, mixerCtrl->shift, value) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
        return HDF_FAILURE;
    }

    if (AudioSetCtrlOpsRReg(elemValue, mixerCtrl, &rvalue, &updateRReg) != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioSetCtrlOpsRReg is fail.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (updateRReg) {
        if (AudioUpdateDaiRegBits(dai, mixerCtrl->rreg, mixerCtrl->mask, mixerCtrl->rshift, rvalue) != HDF_SUCCESS) {
            ADM_LOG_ERR("Audio codec stereo update reg bits fail.");
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

int32_t AudioCpuDaiGetCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    uint32_t curValue = 0;
    uint32_t rcurValue = 0;
    struct AudioMixerControl *mixerCtrl = NULL;
    struct DaiDevice *dai = NULL;
    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemValue == NULL) {
        ADM_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    dai = AudioKcontrolGetCpuDai(kcontrol);
    if (dai == NULL) {
        ADM_LOG_ERR("mixerCtrl and codec is NULL.");
        return HDF_FAILURE;
    }
    if (AudioDaiReadReg(dai, mixerCtrl->reg, &curValue) != HDF_SUCCESS ||
        AudioDaiReadReg(dai, mixerCtrl->rreg, &rcurValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Read dai Reg fail.");
        return HDF_FAILURE;
    }
    if (AudioGetCtrlOpsReg(elemValue, mixerCtrl, curValue) != HDF_SUCCESS ||
        AudioGetCtrlOpsRReg(elemValue, mixerCtrl, rcurValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio dai get kcontrol reg and rreg fail.");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}


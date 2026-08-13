/*
 * Copyright (c) 2026 Talkweb Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "usb_virtual_device.h"

#include <string.h>

#include "hdf_base.h"
#include "hdf_core_log.h"
#include "hdf_device_desc.h"
#include "hdf_sbuf.h"
#include "osal_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Driver identity & constants --------------------------------------- */
#define HDF_LOG_TAG                     usb_virtual_dev

/*
 * Cap on the number of synthetic interface records we ever advertise. The
 * value matches the maximum the framework already tolerates for USB device
 * callbacks; staying well below it keeps the descriptor size bounded.
 */
#define HDF_USB_VIRTUAL_MAX_INTERFACES 4

/*
 * Static, deterministic descriptor block. No host-writable fields; this is
 * the only mutable-state surface the driver exposes and it is read-only by
 * design (we never publish a setter through the service interface).
 */
typedef struct {
    uint16_t vendorId;
    uint16_t productId;
    uint8_t  deviceClass;
    uint8_t  deviceSubClass;
    uint8_t  deviceProtocol;
    uint8_t  interfaceCount;
    uint8_t  interfaceClass[HDF_USB_VIRTUAL_MAX_INTERFACES];
    uint8_t  interfaceSubClass[HDF_USB_VIRTUAL_MAX_INTERFACES];
    uint8_t  interfaceProtocol[HDF_USB_VIRTUAL_MAX_INTERFACES];
} UsbVirtualDescriptor;

static UsbVirtualDescriptor g_usbVirtualDescriptor = {
    .vendorId        = HDF_USB_VIRTUAL_VID,
    .productId       = HDF_USB_VIRTUAL_PID,
    .deviceClass     = HDF_USB_VIRTUAL_DEV_CLASS,
    .deviceSubClass  = HDF_USB_VIRTUAL_DEV_SUBCLASS,
    .deviceProtocol  = USB_USB_VIRTUAL_DEV_PROTOCOL,
    .interfaceCount  = 1,
    .interfaceClass    = { 0xFF, 0x00, 0x00, 0x00 },
    .interfaceSubClass = { 0x00, 0x00, 0x00, 0x00 },
    .interfaceProtocol = { 0x00, 0x00, 0x00, 0x00 },
};

/* ---- Per-device private context ---------------------------------------- */
/*
 * The private context stored in the HdfDeviceObject. Holds the started flag
 * so Init()/Release() are idempotent and the device registration is
 * observable for diagnostics.
 */
typedef struct {
    bool registered;
} UsbVirtualPriv;

/* ---- Synthetic descriptor service command ------------------------------ */
/*
 * Command id used by the synthetic service to report the descriptor to a
 * userspace client. The value is large enough to never collide with the
 * framework-defined USB PnP command ids in hdf_usb_pnp_manage.h (which
 * stop at 50 + a few).
 */
#define USB_VIRTUAL_CMD_GET_DESCRIPTOR     0x1000

static int32_t UsbVirtualDispatch(struct HdfDeviceIoClient *client, int32_t id,
                                  struct HdfSBuf *data, struct HdfSBuf *reply)
{
    UsbVirtualPriv *priv = NULL;

    if (client == NULL || client->device == NULL || client->device->priv == NULL) {
        HDF_LOGE("UsbVirtualDispatch: invalid client!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (reply == NULL) {
        HDF_LOGE("UsbVirtualDispatch: reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    (void)data;

    priv = (UsbVirtualPriv *)client->device->priv;

    if (id != USB_VIRTUAL_CMD_GET_DESCRIPTOR) {
        HDF_LOGW("UsbVirtualDispatch: unsupported cmd id=0x%08x.", id);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (!priv->registered) {
        HDF_LOGE("UsbVirtualDispatch: device not registered yet!");
        return HDF_ERR_DEVICE_BUSY;
    }

    /*
     * Pack the descriptor fields into the reply SBuf. HdfSbufWriteUint16/Uint8
     * return false on any failure; we treat the first failure as fatal and
     * return a generic error to the caller rather than a truncated payload.
     */
    if (!HdfSbufWriteUint16(reply, g_usbVirtualDescriptor.vendorId) ||
        !HdfSbufWriteUint16(reply, g_usbVirtualDescriptor.productId) ||
        !HdfSbufWriteUint8(reply, g_usbVirtualDescriptor.deviceClass) ||
        !HdfSbufWriteUint8(reply, g_usbVirtualDescriptor.deviceSubClass) ||
        !HdfSbufWriteUint8(reply, g_usbVirtualDescriptor.deviceProtocol) ||
        !HdfSbufWriteUint8(reply, g_usbVirtualDescriptor.interfaceCount)) {
        HDF_LOGE("UsbVirtualDispatch: write descriptor header fail!");
        return HDF_FAILURE;
    }

    for (uint8_t i = 0; i < g_usbVirtualDescriptor.interfaceCount; i++) {
        if (!HdfSbufWriteUint8(reply, g_usbVirtualDescriptor.interfaceClass[i]) ||
            !HdfSbufWriteUint8(reply, g_usbVirtualDescriptor.interfaceSubClass[i]) ||
            !HdfSbufWriteUint8(reply, g_usbVirtualDescriptor.interfaceProtocol[i])) {
            HDF_LOGE("UsbVirtualDispatch: write interface[%u] fail!", i);
            return HDF_FAILURE;
        }
    }

    HDF_LOGI("UsbVirtualDispatch: descriptor reported, ifCount=%u.",
             g_usbVirtualDescriptor.interfaceCount);
    return HDF_SUCCESS;
}

/* ---- HdfDriverEntry: Bind / Init / Release ----------------------------- */

static int32_t UsbVirtualDriverBind(struct HdfDeviceObject *device)
{
    struct HdfDeviceService *service = NULL;
    UsbVirtualPriv *priv = NULL;

    if (device == NULL) {
        HDF_LOGE("UsbVirtualDriverBind: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    priv = (UsbVirtualPriv *)OsalMemCalloc(sizeof(*priv));
    if (priv == NULL) {
        HDF_LOGE("UsbVirtualDriverBind: calloc priv fail!");
        return HDF_ERR_MALLOC_FAIL;
    }
    priv->registered = false;

    /*
     * Allocate the service object through OsalMemCalloc so Bind can free it
     * cleanly on the failure path; the framework does not own this memory.
     */
    service = (struct HdfDeviceService *)OsalMemCalloc(sizeof(*service));
    if (service == NULL) {
        HDF_LOGE("UsbVirtualDriverBind: calloc service fail!");
        OsalMemFree(priv);
        return HDF_ERR_MALLOC_FAIL;
    }
    service->Dispatch = UsbVirtualDispatch;

    device->service = &service->super;
    device->priv = priv;

    HDF_LOGI("UsbVirtualDriverBind: %s bind success.", HDF_USB_VIRTUAL_MODULE_NAME);
    return HDF_SUCCESS;
}

static int32_t UsbVirtualDriverInit(struct HdfDeviceObject *device)
{
    UsbVirtualPriv *priv = NULL;

    if (device == NULL || device->priv == NULL) {
        HDF_LOGE("UsbVirtualDriverInit: invalid device!");
        return HDF_ERR_INVALID_OBJECT;
    }

    priv = (UsbVirtualPriv *)device->priv;

    /*
     * Mark the device as registered. In a real USB device-class driver this
     * is where we would push the descriptor to the device-mode controller and
     * wait for SET_CONFIGURATION; the virtual driver has no controller and
     * only needs the flag so the service callback can report a sane state.
     */
    if (priv->registered) {
        HDF_LOGI("UsbVirtualDriverInit: already initialized, skip.");
        return HDF_SUCCESS;
    }
    priv->registered = true;

    HDF_LOGI("UsbVirtualDriverInit: %s init success (vid=0x%04X pid=0x%04X).",
             HDF_USB_VIRTUAL_MODULE_NAME,
             g_usbVirtualDescriptor.vendorId,
             g_usbVirtualDescriptor.productId);
    return HDF_SUCCESS;
}

static void UsbVirtualDriverRelease(struct HdfDeviceObject *device)
{
    UsbVirtualPriv *priv = NULL;
    struct HdfDeviceService *service = NULL;

    if (device == NULL) {
        return;
    }

    priv = (UsbVirtualPriv *)device->priv;
    if (priv != NULL) {
        priv->registered = false;
        OsalMemFree(priv);
        device->priv = NULL;
    }

    /*
     * Free the service object allocated in Bind. Cast back from the super
     * field; the framework never aliases this pointer, so this is safe.
     */
    if (device->service != NULL) {
        service = (struct HdfDeviceService *)device->service;
        OsalMemFree(service);
        device->service = NULL;
    }

    HDF_LOGI("UsbVirtualDriverRelease: %s released.", HDF_USB_VIRTUAL_MODULE_NAME);
}

struct HdfDriverEntry g_usbVirtualDriverEntry = {
    .moduleVersion = 1,
    .moduleName = HDF_USB_VIRTUAL_MODULE_NAME,
    .Bind = UsbVirtualDriverBind,
    .Init = UsbVirtualDriverInit,
    .Release = UsbVirtualDriverRelease,
};

HDF_INIT(g_usbVirtualDriverEntry);

#ifdef __cplusplus
}
#endif

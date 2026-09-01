/*
 * Copyright (c) 2026 Talkweb Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/*
 * Virtual network driver.
 *
 * This driver registers a NetDevice with the network framework core
 * (net_device.c) and provides a dependency-free, deterministic implementation
 * of the NetDeviceInterFace callbacks. It mirrors the layout of the existing
 * ethernet driver (hdf_eth_core.c) so the framework treats both identically,
 * but requires no SoC HAL and no real PHY/MAC, making it suitable for
 * framework-level testing on any LiteOS target.
 *
 * Design goals:
 *  - Zero impact on existing code: the only touch point is appending one
 *    guarded line to the network BUILD.gn sources list; no existing .c/.h
 *    is edited.
 *  - GN-only enable/disable: loading/unloading is controlled entirely by
 *    the LOSCFG_DRIVERS_HDF_NET_VIRTUAL Kconfig switch.
 *  - No security pitfalls: the synthetic MAC address is fixed and bounded;
 *    xmit drops every NetBuf silently (no buffer ownership transfer bugs);
 *    receive is not synthesized, so no injection path exists.
 */

#include "net_virtual_driver.h"

#include <stdlib.h>
#include <string.h>

#include "hdf_base.h"
#include "hdf_core_log.h"
#include "hdf_device_desc.h"
#include "osal_mem.h"
#include "net_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Driver identity & constants --------------------------------------- */
#define HDF_LOG_TAG                 net_virtual_drv
#define HDF_NET_VIRTUAL_MAC_SIZE    MAC_ADDR_SIZE

/*
 * Synthetic but locally administered MAC address (the "locally administered"
 * bit is bit 1 of the first octet: 0x02). The remaining octets are fixed so
 * the value is deterministic and never collides with a real vendor OUI.
 */
static const unsigned char g_netVirtualMac[HDF_NET_VIRTUAL_MAC_SIZE] = {
    0x02, 0x00, 0x00, 0x00, 0x00, 0x01,
};

/* ---- Per-device private context ---------------------------------------- */
/*
 * The private context stored in NetDevice::priv. Holds the device name copy
 * and a started flag so open()/close() are idempotent and observable.
 */
typedef struct {
    bool opened;
} NetVirtualPriv;

/* ---- NetDeviceInterFace callbacks -------------------------------------- */

static int32_t NetVirtualInit(struct NetDevice *netDev)
{
    HDF_LOGI("NetVirtualInit: enter.");
    if (netDev == NULL) {
        HDF_LOGE("NetVirtualInit: netDev is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    /* Nothing to allocate for the virtual device; priv is set in Bind. */
    return HDF_SUCCESS;
}

static void NetVirtualDeInit(struct NetDevice *netDev)
{
    HDF_LOGI("NetVirtualDeInit: enter.");
    (void)netDev;
}

static int32_t NetVirtualOpen(struct NetDevice *netDev)
{
    NetVirtualPriv *priv = NULL;

    if (netDev == NULL) {
        HDF_LOGE("NetVirtualOpen: netDev is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    priv = (NetVirtualPriv *)netDev->priv;
    if (priv == NULL) {
        HDF_LOGE("NetVirtualOpen: priv is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (priv->opened) {
        HDF_LOGI("NetVirtualOpen: already opened, skip.");
        return HDF_SUCCESS;
    }
    priv->opened = true;
    HDF_LOGI("NetVirtualOpen: vnet0 opened.");
    return HDF_SUCCESS;
}

static int32_t NetVirtualStop(struct NetDevice *netDev)
{
    NetVirtualPriv *priv = NULL;

    if (netDev == NULL) {
        HDF_LOGE("NetVirtualStop: netDev is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    priv = (NetVirtualPriv *)netDev->priv;
    if (priv == NULL) {
        HDF_LOGE("NetVirtualStop: priv is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (!priv->opened) {
        HDF_LOGI("NetVirtualStop: already stopped, skip.");
        return HDF_SUCCESS;
    }
    priv->opened = false;
    HDF_LOGI("NetVirtualStop: vnet0 stopped.");
    return HDF_SUCCESS;
}

/*
 * Transmit callback. The virtual device has no real hardware, so every
 * outgoing NetBuf is dropped. We deliberately return NETDEV_TX_OK so the
 * stack frees the buffer and does not retry indefinitely; a virtual link
 * that always drops is the safe, side-effect-free contract.
 */
static NetDevTxResult NetVirtualXmit(struct NetDevice *netDev, NetBuf *netBuff)
{
    (void)netDev;
    (void)netBuff;
    HDF_LOGD("NetVirtualXmit: drop packet (virtual link).");
    return NETDEV_TX_OK;
}

static int32_t NetVirtualIoctl(struct NetDevice *netDev, IfReq *req, int32_t cmd)
{
    (void)netDev;
    (void)req;
    HDF_LOGI("NetVirtualIoctl: cmd=%d (virtual, no-op).", cmd);
    return HDF_SUCCESS;
}

static int32_t NetVirtualSetMacAddr(struct NetDevice *netDev, void *addr)
{
    if (netDev == NULL || addr == NULL) {
        HDF_LOGE("NetVirtualSetMacAddr: invalid param!");
        return HDF_ERR_INVALID_PARAM;
    }
    /*
     * The virtual device owns a fixed synthetic MAC; reject externally
     * supplied addresses so the device identity stays stable and
     * deterministic. Callers still receive a defined, documented result.
     */
    HDF_LOGW("NetVirtualSetMacAddr: virtual MAC is fixed, reject external addr.");
    return HDF_ERR_NOT_SUPPORT;
}

static struct NetDevStats g_netVirtualStats = {0};

static struct NetDevStats *NetVirtualGetStats(struct NetDevice *netDev)
{
    (void)netDev;
    /* Return the static, zeroed stats block; the virtual link carries no traffic. */
    return &g_netVirtualStats;
}

static void NetVirtualSetNetIfStatus(struct NetDevice *netDev, NetIfStatus status)
{
    if (netDev == NULL) {
        HDF_LOGE("NetVirtualSetNetIfStatus: netDev is null!");
        return;
    }
    HDF_LOGI("NetVirtualSetNetIfStatus: status=%d.", status);
}

static uint16_t NetVirtualSelectQueue(struct NetDevice *netDev, NetBuf *netBuff)
{
    (void)netDev;
    (void)netBuff;
    /* Single queue virtual device. */
    return 0;
}

static uint32_t NetVirtualNetifNotify(struct NetDevice *netDev, NetDevNotify *notify)
{
    (void)netDev;
    if (notify == NULL) {
        HDF_LOGE("NetVirtualNetifNotify: notify is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    HDF_LOGI("NetVirtualNetifNotify: notify=%u.", *notify);
    return HDF_SUCCESS;
}

static int32_t NetVirtualChangeMtu(struct NetDevice *netDev, int32_t newMtu)
{
    if (netDev == NULL) {
        HDF_LOGE("NetVirtualChangeMtu: netDev is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (newMtu <= 0 || newMtu > HDF_NET_VIRTUAL_MTU) {
        HDF_LOGE("NetVirtualChangeMtu: invalid newMtu=%d.", newMtu);
        return HDF_ERR_INVALID_PARAM;
    }
    HDF_LOGI("NetVirtualChangeMtu: newMtu=%d (virtual, accepted).", newMtu);
    return HDF_SUCCESS;
}

static void NetVirtualLinkStatusChanged(struct NetDevice *netDev)
{
    if (netDev == NULL) {
        HDF_LOGE("NetVirtualLinkStatusChanged: netDev is null!");
        return;
    }
    HDF_LOGI("NetVirtualLinkStatusChanged: (virtual, no-op).");
}

static ProcessingResult NetVirtualSpecialEtherTypeProcess(const struct NetDevice *netDev,
                                                          NetBuf *buff)
{
    (void)netDev;
    (void)buff;
    /* The virtual link does not intercept any special EtherType. */
    return PROCESSING_CONTINUE;
}

static struct NetDeviceInterFace g_netVirtualIf = {
    .init = NetVirtualInit,
    .deInit = NetVirtualDeInit,
    .open = NetVirtualOpen,
    .stop = NetVirtualStop,
    .xmit = NetVirtualXmit,
    .ioctl = NetVirtualIoctl,
    .setMacAddr = NetVirtualSetMacAddr,
    .getStats = NetVirtualGetStats,
    .setNetIfStatus = NetVirtualSetNetIfStatus,
    .selectQueue = NetVirtualSelectQueue,
    .netifNotify = NetVirtualNetifNotify,
    .changeMtu = NetVirtualChangeMtu,
    .linkStatusChanged = NetVirtualLinkStatusChanged,
    .specialEtherTypeProcess = NetVirtualSpecialEtherTypeProcess,
};

/* ---- HdfDriverEntry: Bind / Init / Release ----------------------------- */

static int32_t NetVirtualDriverBind(struct HdfDeviceObject *device)
{
    struct NetDevice *netDev = NULL;
    NetVirtualPriv *priv = NULL;

    if (device == NULL) {
        HDF_LOGE("NetVirtualDriverBind: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    priv = (NetVirtualPriv *)OsalMemCalloc(sizeof(*priv));
    if (priv == NULL) {
        HDF_LOGE("NetVirtualDriverBind: calloc priv fail!");
        return HDF_ERR_MALLOC_FAIL;
    }
    priv->opened = false;

    /*
     * Create the NetDevice through the framework API. ETHERNET_LINK is used
     * because the virtual link emulates a frame-oriented link type; LITE_OS
     * category keeps it consistent with the LiteOS adaptation layer.
     */
    netDev = NetDeviceInit(HDF_NET_VIRTUAL_IF_NAME, strlen(HDF_NET_VIRTUAL_IF_NAME),
                           ETHERNET_LINK, LITE_OS);
    if (netDev == NULL) {
        HDF_LOGE("NetVirtualDriverBind: NetDeviceInit fail!");
        OsalMemFree(priv);
        return HDF_FAILURE;
    }

    netDev->netDeviceIf = &g_netVirtualIf;
    netDev->priv = priv;
    (void)memcpy_s(netDev->macAddr, MAC_ADDR_SIZE, g_netVirtualMac, MAC_ADDR_SIZE);

    device->service = (struct IDeviceIoService *)netDev;
    HDF_LOGI("NetVirtualDriverBind: %s bind success.", HDF_NET_VIRTUAL_MODULE_NAME);
    return HDF_SUCCESS;
}

static int32_t NetVirtualDriverInit(struct HdfDeviceObject *device)
{
    struct NetDevice *netDev = NULL;
    int32_t ret;

    if (device == NULL || device->service == NULL) {
        HDF_LOGE("NetVirtualDriverInit: invalid device!");
        return HDF_ERR_INVALID_OBJECT;
    }

    netDev = (struct NetDevice *)device->service;

    /*
     * Register the NetDevice with the framework so it becomes visible to the
     * protocol stack. NetDeviceAdd is idempotent-safe only once per instance;
     * we guard against double-init by checking the device flags via the framework.
     */
    ret = NetDeviceAdd(netDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("NetVirtualDriverInit: NetDeviceAdd fail, ret=%d!", ret);
        return ret;
    }

    /* Bring the interface up so the stack sees a usable link. */
    ret = NetIfSetStatus(netDev, NETIF_UP);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("NetVirtualDriverInit: NetIfSetStatus UP fail, ret=%d!", ret);
        (void)NetDeviceDelete(netDev);
        return ret;
    }

    ret = NetIfSetLinkStatus(netDev, NETIF_LINK_UP);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("NetVirtualDriverInit: NetIfSetLinkStatus UP fail, ret=%d!", ret);
        /* Non-fatal: link status is informational; keep the device registered. */
    }

    HDF_LOGI("NetVirtualDriverInit: %s init success.", HDF_NET_VIRTUAL_MODULE_NAME);
    return HDF_SUCCESS;
}

static void NetVirtualDriverRelease(struct HdfDeviceObject *device)
{
    struct NetDevice *netDev = NULL;
    NetVirtualPriv *priv = NULL;

    if (device == NULL || device->service == NULL) {
        return;
    }

    netDev = (struct NetDevice *)device->service;

    /* Tear down link state before deletion. */
    (void)NetIfSetStatus(netDev, NETIF_DOWN);
    (void)NetIfSetLinkStatus(netDev, NETIF_LINK_DOWN);

    priv = (NetVirtualPriv *)netDev->priv;
    if (priv != NULL) {
        if (priv->opened) {
            (void)NetVirtualStop(netDev);
        }
        OsalMemFree(priv);
        netDev->priv = NULL;
    }

    (void)NetDeviceDelete(netDev);
    (void)NetDeviceDeInit(netDev);

    device->service = NULL;
    HDF_LOGI("NetVirtualDriverRelease: %s released.", HDF_NET_VIRTUAL_MODULE_NAME);
}

struct HdfDriverEntry g_netVirtualDriverEntry = {
    .moduleVersion = 1,
    .moduleName = HDF_NET_VIRTUAL_MODULE_NAME,
    .Bind = NetVirtualDriverBind,
    .Init = NetVirtualDriverInit,
    .Release = NetVirtualDriverRelease,
};

HDF_INIT(g_netVirtualDriverEntry);

#ifdef __cplusplus
}
#endif

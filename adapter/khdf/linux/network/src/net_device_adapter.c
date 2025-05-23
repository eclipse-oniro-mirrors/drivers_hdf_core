/*
 * net_device_adapter.c
 *
 * net device adapter of linux
 *
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "net_device_adapter.h"
#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>
#include <linux/version.h>
#if defined(CONFIG_DRIVERS_HDF_IMX8MM_ETHERNET)
#include <linux/phy.h>
#include <linux/of_mdio.h>
#endif
#include "net_device.h"
#include "net_device_impl.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG NetDeviceFull

static int32_t NetDevXmitCheck(const struct sk_buff *skb, struct net_device *dev)
{
    struct FullNetDevicePriv *priv = NULL;

    if (dev == NULL || skb == NULL) {
        HDF_LOGE("%s : dev = NUll or skb = NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    priv = (struct FullNetDevicePriv *)netdev_priv(dev);
    if (priv == NULL || priv->dev == NULL || priv->impl == NULL) {
        HDF_LOGE("%s fail : priv NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

static netdev_tx_t NetDevXmit(struct sk_buff *skb, struct net_device *dev)
{
    struct FullNetDevicePriv *priv = NULL;
    struct NetDevice *netDev = NULL;
    struct NetDeviceInterFace *netDevIf = NULL;

    if (NetDevXmitCheck(skb, dev) != HDF_SUCCESS) {
        NetBufFree(skb);
        return NETDEV_TX_OK;
    }
    priv = (struct FullNetDevicePriv *)netdev_priv(dev);
    netDev = priv->impl->netDevice;

    skb->dev = dev;
    netDevIf = netDev->netDeviceIf;
    if (netDevIf != NULL && netDevIf->xmit != NULL) {
        netDevIf->xmit(netDev, skb);
    } else {
        HDF_LOGE("%s fail : netdevIf = null or xmit = null!", __func__);
        NetBufFree(skb);
    }
    return NETDEV_TX_OK;
}

static struct net_device_stats* NetDevGetStats(struct net_device *dev)
{
    struct FullNetDevicePriv *priv = NULL;
    struct NetDevice *netDev = NULL;
    struct NetDeviceInterFace *netDevIf = NULL;
    struct NetDevStats *stats;
    static struct net_device_stats netStats;

    (void)memset_s(&netStats, sizeof(struct net_device_stats), 0, sizeof(struct net_device_stats));
    priv = (struct FullNetDevicePriv *)netdev_priv(dev);
    if (priv == NULL || priv->dev == NULL || priv->impl == NULL || priv->impl->netDevice == NULL) {
        HDF_LOGE("%s fail : priv NULL!", __func__);
        return &netStats;
    }
    netDev = priv->impl->netDevice;
    netDevIf = netDev->netDeviceIf;
    if (netDevIf != NULL && netDevIf->getStats != NULL) {
        stats = netDevIf->getStats(netDev);
        if (stats != NULL) {
            netStats.rx_packets = stats->rxPackets;
            netStats.tx_packets = stats->txPackets;
            netStats.rx_bytes = stats->rxBytes;
            netStats.tx_bytes = stats->txBytes;
            netStats.rx_errors = stats->rxErrors;
            netStats.tx_errors = stats->txErrors;
            netStats.rx_dropped = stats->rxDropped;
            netStats.tx_dropped = stats->txDropped;
        }
    }
    return &netStats;
}

static int NetDevChangeMtu(struct net_device *dev, int mtu)
{
    if (mtu > WLAN_MAX_MTU || mtu < WLAN_MIN_MTU || dev == NULL) {
        return -EINVAL;
    }

    dev->mtu = (uint32_t)mtu;
    return HDF_SUCCESS;
}

static int NetDevOpen(struct net_device *dev)
{
    if (dev == NULL) {
        return -EINVAL;
    }

    netif_start_queue(dev);
    return HDF_SUCCESS;
}

static int NetDevStop(struct net_device *dev)
{
    if (dev == NULL) {
        return -EINVAL;
    }

    netif_stop_queue(dev);
    return HDF_SUCCESS;
}

static struct net_device_ops g_netDeviceOps = {
    .ndo_start_xmit = NetDevXmit,
    .ndo_change_mtu = NetDevChangeMtu,
    .ndo_get_stats = NetDevGetStats,
    .ndo_open = NetDevOpen,
    .ndo_stop = NetDevStop
};

static struct net_device *CreateNetDevice(struct NetDevice *hdfDev)
{
    struct net_device *dev = NULL;

    dev = alloc_etherdev(sizeof(struct FullNetDevicePriv));
    if (dev == NULL) {
        return NULL;
    }

    if (memcpy_s(dev->name, IFNAMSIZ, hdfDev->name, IFNAMSIZ) != EOK) {
        free_netdev(dev);
        return NULL;
    }
    dev->mtu = DEFAULT_MTU;
    dev->netdev_ops = &g_netDeviceOps;

    return dev;
}

static void DestroyNetDevice(struct net_device *dev)
{
    free_netdev(dev);
}

static struct net_device *GetDevFromDevImpl(const struct NetDeviceImpl *impl)
{
    struct FullNetDevicePriv *priv = NULL;

    if (impl == NULL || impl->osPrivate == NULL) {
        return NULL;
    }
    priv = (struct FullNetDevicePriv *)impl->osPrivate;
    return priv->dev;
}

static int32_t NetDevInit(struct NetDeviceImpl *impl)
{
    struct FullNetDevicePriv *priv = NULL;
    struct net_device *dev = NULL;

    if (impl == NULL || impl->netDevice == NULL) {
        HDF_LOGE("%s fail: impl null , netDevice null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    dev = CreateNetDevice(impl->netDevice);
    if (dev == NULL) {
        HDF_LOGE("%s fail : CreateNetDevice fail!", __func__);
        return HDF_FAILURE;
    }

    priv = (struct FullNetDevicePriv *)netdev_priv(dev);
    priv->dev = dev;
    priv->impl = impl;
    impl->osPrivate = (void *)priv;
    HDF_LOGI("%s Success!", __func__);

    return HDF_SUCCESS;
}

static int32_t NetDevDeInit(struct NetDeviceImpl *impl)
{
    struct FullNetDevicePriv *priv = NULL;

    if (impl == NULL) {
        HDF_LOGE("netdevice linux deinit already free.");
        return HDF_SUCCESS;
    }
    if (impl->osPrivate != NULL) {
        priv = (struct FullNetDevicePriv *)impl->osPrivate;
        DestroyNetDevice(priv->dev);
        impl->osPrivate = NULL;
    }
    HDF_LOGI("net device linux deinit success!");

    return HDF_SUCCESS;
}

static int32_t NetDevAdd(struct NetDeviceImpl *impl)
{
    struct net_device *dev = NULL;
    int ret;

    if ((dev = GetDevFromDevImpl(impl)) == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (impl->netDevice == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    if (memcpy_s(dev->dev_addr, ETH_ALEN, impl->netDevice->macAddr,
        MAC_ADDR_SIZE) != EOK) {
        HDF_LOGE("%s : memcpy fail!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    dev->needed_headroom = impl->netDevice->neededHeadRoom;
    dev->needed_tailroom = impl->netDevice->neededTailRoom;

    if ((ret = register_netdev(dev)) < 0) {
        HDF_LOGE("%s : register_netdev fail!,ret=%d", __func__, ret);
        return HDF_FAILURE;
    }

    HDF_LOGI("%s success!!", __func__);
    return HDF_SUCCESS;
}

static int32_t NetDevDelete(struct NetDeviceImpl *impl)
{
    struct FullNetDevicePriv *priv = NULL;
    struct net_device *dev = NULL;

    if (impl == NULL || impl->osPrivate == NULL) {
        HDF_LOGE("%s fail : impl = null or osPrivate = null!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    priv = (struct FullNetDevicePriv *)impl->osPrivate;

    dev = priv->dev;
    if (dev != NULL) {
        unregister_netdev(dev);
    }
    HDF_LOGI("%s success!", __func__);
    return HDF_SUCCESS;
}

#if defined(CONFIG_DRIVERS_HDF_IMX8MM_ETHERNET)
#define NAPI_ADD_NUM   (64)
static void NetDevApiAdd(struct NetDeviceImpl *impl, struct napi_struct *napi,
    int (*poll)(struct napi_struct *, int), int weight)
{
    struct net_device *dev = GetDevFromDevImpl(impl);

    netif_napi_add(dev, napi, poll, NAPI_ADD_NUM);
}

static struct netdev_queue *NetDevGetTxQueue(struct NetDeviceImpl *impl, unsigned int queue)
{
    struct net_device *dev = GetDevFromDevImpl(impl);
    struct netdev_queue *nq;

    nq = netdev_get_tx_queue(dev, queue);

    return nq;
}

static __be16 NetDevTypeTrans(struct NetDeviceImpl *impl, struct sk_buff *skb)
{
    __be16 ret;
    struct net_device *dev = GetDevFromDevImpl(impl);

    ret = eth_type_trans(skb, dev);

    return ret;
}

static struct sk_buff *NetDevAllocBuf(struct NetDeviceImpl *impl, uint32_t length)
{
    struct sk_buff *skb = NULL;
    struct net_device *dev = GetDevFromDevImpl(impl);

    skb = netdev_alloc_skb(dev, length);
    return skb;
}

static void NetDevStartQueue(struct NetDeviceImpl *impl)
{
    struct net_device *dev = GetDevFromDevImpl(impl);
    struct netdev_queue *nq;

    nq = netdev_get_tx_queue(dev, 1);
    netif_tx_start_queue(nq);
}

static void NetDevDisableTx(struct NetDeviceImpl *impl)
{
    struct net_device *dev = GetDevFromDevImpl(impl);

    netif_tx_disable(dev);
}

static void NetDevSetDev(struct NetDeviceImpl *impl, struct device *dev)
{
    struct net_device *ndev = GetDevFromDevImpl(impl);

    SET_NETDEV_DEV(ndev, dev);
}

static void NetDevWakeQueue(struct NetDeviceImpl *impl)
{
    struct net_device *dev = GetDevFromDevImpl(impl);
    struct netdev_queue *nq;

    nq = netdev_get_tx_queue(dev, 1);
    netif_tx_wake_queue(nq);
}

static struct phy_device *NetDevOfPhyConnect(struct NetDeviceImpl *impl, struct device_node *phy_np,
    void (*hndlr)(struct net_device *), u32 flags, phy_interface_t iface)
{
    struct phy_device *phy = NULL;
    struct net_device *ndev = GetDevFromDevImpl(impl);
    phy = of_phy_connect(ndev, phy_np, hndlr, 0, iface);
    return phy;
}
#endif

static int32_t NetDevSetStatus(struct NetDeviceImpl *impl,
    NetIfStatus status)
{
    struct net_device *dev = GetDevFromDevImpl(impl);
    int32_t ret;

    if (dev == NULL) {
        HDF_LOGE("%s fail : net dev null!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    rtnl_lock();
    if (status == NETIF_DOWN) {
        dev_close(dev);
        ret = HDF_SUCCESS;
    } else if (status == NETIF_UP) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
        ret = dev_open(dev, NULL);
#else
        ret = dev_open(dev);
#endif
    } else {
        HDF_LOGE("%s fail : status error!", __func__);
        rtnl_unlock();
        return HDF_ERR_INVALID_PARAM;
    }
    rtnl_unlock();

    if (!ret) {
        return HDF_SUCCESS;
    }
    HDF_LOGE("%s fail ret = %d!", __func__, ret);
    return HDF_FAILURE;
}

static int32_t NetDevReceive(struct NetDeviceImpl *impl,
    NetBuf *buff, ReceiveFlag flag)
{
    struct net_device *dev = GetDevFromDevImpl(impl);

    if (dev == NULL) {
        HDF_LOGE("%s fail : dev = null!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (flag >= MAX_RECEIVE_FLAG || buff == NULL) {
        HDF_LOGE("%s fail : flag = %d or buff = null!", __func__, flag);
        return HDF_ERR_INVALID_PARAM;
    }

    buff->dev = dev;
    buff->protocol = eth_type_trans(buff, dev);
    if (flag & IN_INTERRUPT) {
        netif_rx(buff);
    } else {
        netif_rx_ni(buff);
    }
    return HDF_SUCCESS;
}

static int32_t NetDevChangeMacAddr(struct NetDeviceImpl *impl)
{
    struct net_device *dev = NULL;

    if ((dev = GetDevFromDevImpl(impl)) == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (impl->netDevice == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    if (memcpy_s(dev->dev_addr, ETH_ALEN, impl->netDevice->macAddr, MAC_ADDR_SIZE) != EOK) {
        HDF_LOGE("%s : memcpy fail!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

static struct NetDeviceImplOp g_ndImplOps = {
    .init = NetDevInit,
    .deInit = NetDevDeInit,
    .add = NetDevAdd,
    .delete = NetDevDelete,
    .setStatus = NetDevSetStatus,
    .receive = NetDevReceive,
    .changeMacAddr = NetDevChangeMacAddr,
#if defined(CONFIG_DRIVERS_HDF_IMX8MM_ETHERNET)
    .netif_napi_add = NetDevApiAdd,
    .get_tx_queue = NetDevGetTxQueue,
    .type_trans = NetDevTypeTrans,
    .alloc_buf = NetDevAllocBuf,
    .start_queue = NetDevStartQueue,
    .disable_tx = NetDevDisableTx,
    .set_dev = NetDevSetDev,
    .wake_queue = NetDevWakeQueue,
    .of_phyconnect = NetDevOfPhyConnect,
#endif
};

int32_t RegisterNetDeviceImpl(struct NetDeviceImpl *ndImpl)
{
    if (ndImpl == NULL) {
        HDF_LOGE("%s fail : impl = null!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ndImpl->interFace = &g_ndImplOps;
    HDF_LOGI("register full netdevice impl success.");
    return HDF_SUCCESS;
}

int32_t UnRegisterNetDeviceImpl(struct NetDeviceImpl *ndImpl)
{
    if (ndImpl == NULL) {
        HDF_LOGI("%s already unregister!", __func__);
        return HDF_SUCCESS;
    }
    ndImpl->interFace = NULL;
    HDF_LOGI("%s success.", __func__);
    return HDF_SUCCESS;
}

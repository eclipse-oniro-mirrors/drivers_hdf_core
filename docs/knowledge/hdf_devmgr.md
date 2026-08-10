# 驱动管理进程 hdf_devmgr

本文记录 `hdf_devmgr` 的设计意图、核心架构、改动规则与代码路径。

## 进程定位

`hdf_devmgr` 是 HDF 子系统的**中央管理守护进程**，运行为独立系统服务：

- **Host 生命周期管理**：启动/管理 `devhost_*` 子进程，每个 hostId 一个独立进程
- **设备驱动加载/卸载**：根据 HCS 配置启动驱动，支持按需 LoadDevice/UnloadDevice
- **服务注册与发现**：维护全局 DevSvcManager 注册表（SA-ID 5100）
- **IPC 桥接**：IDeviceManager + IServiceManager HDI 接口 Binder stub 端点
- **电源状态传播**：接收系统电源事件向所有 Host 传播
- **PnP uevent 处理**：Netlink 监听内核 uevent，按 hdf_pnp.cfg 规则触发动态加载/卸载
- **Debug dump**：CLI 查询设备/Host/服务状态

就绪信号：`bootevent.hdf_devmgr.ready=true`

## 分层架构

```text
IDevmgrService (虚表: AttachDeviceHost, AttachDevice, DetachDevice,
  LoadDevice, UnloadDevice, StartService, PowerStateChange, ListAll)
  |
  DevmgrService   (框架层: hosts DList, devMgrMutex)
  |
  DevmgrServiceFull  (适配层: MessageLooper + MessageTask, 异步Host死亡处理)
  |
  DevmgrServiceStub  (IPC层: HdfRemoteService* + devmgrStubMutex)
```

对象工厂映射（`devmgr_object_config.c`）：

| Object ID | 创建函数 | 说明 |
|---|---|---|
| DEVMGR_SERVICE | DevmgrServiceStubCreate | Manager 核心服务 |
| DEVSVC_MANAGER | DevSvcManagerStubCreate | 服务注册管理 |
| DRIVER_INSTALLER | DriverInstallerFullCreate | init系统启动devhost |

## 核心数据结构

| 结构 | 关键成员 | 说明 |
|---|---|---|
| DevmgrService | hosts(DList), devMgrMutex | Manager 主对象 |
| DevHostServiceClnt | hostService(代理), devices/unloadDevInfos/dynamicDevInfos, hostId/hostName/hostPid | Host客户端记录 |
| DevSvcManager | services(DList), svcstatListeners(DList), observer, mutex | 服务注册表 |
| DeviceTokenClnt | tokenIf(devid/servName/deviceName), deviceInfo | 设备令牌 |
| ServStatListenerHolder | index, listenClass, NotifyStatus回调 | 服务状态监听器 |

## IDevmgrService 接口

| 方法 | 功能 |
|---|---|
| AttachDeviceHost | devhost注册自身 |
| AttachDevice / DetachDevice | 注册/注销设备令牌 |
| LoadDevice / UnloadDevice | 按需动态加载/卸载（按服务名） |
| StartService | 解析HCS+启动hosts+启动svc manager |
| PowerStateChange | 向所有host传播电源状态 |
| ListAllDevice / ListAllHost | 列举设备/host |

## 进程启动流程

```text
main() [device_manager.c]
  -> DevmgrServiceGetInstance() [Stub->Full->DevmgrService 三层构造]
  -> StartService:
     -> 创建 DevSvcManagerStub, 注册"hdf_device_manager"服务
     -> DriverModuleLoadHelperInit()
     -> DevmgrServiceStartDeviceHosts(): HcsGetHostList -> 每个host -> DriverInstallerStartDeviceHost()
     -> DevSvcManagerStartService(): 注册SA-ID 5100
  -> DevMgrUeventReceiveStart() [uevent线程]
  -> SetParameter("bootevent.hdf_devmgr.ready", "true")
  -> looper->Start() [主事件循环]
```

## 设备生命周期

**AttachDeviceHost**：devhost启动后IPC注册 → Manager查找DevHostServiceClnt → InstallDriver加载配置设备

**LoadDevice**：搜索dynamicDevInfos匹配svcName → 如host未运行则启动(同步等2秒) → IPC AddDevice → status=USABLE

**UnloadDevice**：搜索dynamicDevInfos → IPC DelDevice → status=UNUSABLE → 无剩余设备则停止host

**Host死亡处理**：HdfDeathRecipient → 异步MessageTask → CleanupDiedHostResources(清hostPid/回收代理/清设备令牌)

**LoadLeftDriver**：启动后加载 DEVICE_PRELOAD_ENABLE_STEP2 设备

## 服务注册与发现

**注册**：devhost IPC DEVSVC_MANAGER_ADD_SERVICE → SELinux检查 → HdfDeviceObjectHolder + DevSvcRecord → 通知监听器

**发现**：客户端SA-ID 5100 → GetService IPC → SELinux检查 → 返回HdfRemoteService

**监听注册**：REGISTER_SVCLISTENER(类掩码+远程监听器) → ServStatListenerHolder → 立即+后续通知

## IPC Stub/Proxy

**DevmgrServiceStub**：接口令牌"HDI.IDeviceManager.V1_0"，8命令分发(Attach/Detach/Load/Unload/ListAll)

**DevSvcManagerStub**：SA-ID 5100，接口"HDI.IServiceManager.V1_0"，8命令+SELinux+devObjHolderList有效性追踪

**DevHostServiceProxy**：Manager到devhost的AddDevice/DelDevice/Dump IPC + 死亡检测异步MessageTask

**ServStatListenerHolder双实现**：框架层(kernel: HdfDeviceIoClient) vs uhdf2层(IPC: HdfRemoteService+DispatchAsync)

## 电源管理

唤醒(resume)正向遍历hosts → PmNotify；休眠(suspend)反向遍历确保依赖host后休眠。

## Uevent处理

配置`/system/etc/hdfconfig/hdf_pnp.cfg` → Netlink socket监听 → 匹配规则 → LoadDevice/UnloadDevice

## 代码路径映射

| 层 | 目录 | 关键文件 |
|---|---|---|
| 框架层 | `framework/core/manager/` | devmgr_service.c(核心), devhost_service_clnt.c(Host客户端), devsvc_manager.c(服务注册) |
| 接口层 | `framework/core/shared/include/` | devmgr/devhost/devsvc_service_if.h, device_token_if.h, hdf_object_manager.h |
| HDI层 | `interfaces/inner_api/hdi/` | devmgr_hdi.h(C HDI), idevmgr_hdi.h(C++ HDI), servmgr_hdi.h |
| 适配层 | `adapter/uhdf2/manager/` | device_manager.c(main), devmgr_service_stub.c(IPC stub), devsvc_manager_stub.c, devhost_service_proxy.c, devmgr_uevent.c, devmgr_dump.c, devmgr_object_config.c(工厂配置) |

## 改动规则

- Manager代码分布在 `framework/core/manager/` + `adapter/uhdf2/manager/`，**不要只改一侧**
- 新增Manager-Host交互先在 `framework/core/shared/include/` 定义接口契约
- IPC桥接层 `adapter/uhdf2/ipc/` 不承载业务逻辑
- 新增对象类型在 HdfObjectManager Creator映射表注册
- Host死亡处理通过MessageTask异步处理，避免Binder回调死锁
- DevSvcManagerStub含SELinux检查+devObjHolderList有效性追踪

## 相关文档

- [hdf_devhost.md](hdf_devhost.md)

# 驱动孵化进程 hdf_devhost

本文记录 `hdf_devhost` 的设计意图、核心架构、改动规则与代码路径。

## 进程定位

`DevHostService`（devhost）是标准系统**用户空间驱动孵化进程**：

- **进程隔离**：每种host类型独立进程（`devhost_*`），驱动崩溃不影响Manager或其他Host
- **驱动动态加载**：dlopen_ns/dlsym加载驱动.so，基于Manager下发的HCS配置指令
- **设备生命周期**：创建→Bind→Init→发布服务→Release驱动实例
- **服务发布**：IPC注册驱动服务到servmgr(SA-ID 5100)
- **电源管理**：接收电源事件，通过PowerStateToken向各驱动传播
- **IPC桥接**：Stub接收Manager命令；DeviceServiceStub转发客户端IO请求到驱动Dispatch
- **本地服务观察**：HdfServiceObserver支持FRIENDLY/PRIVATE策略host内互相订阅

host设置 `PR_SET_PDEATHSIG(SIGKILL)` 以在devmgr死亡时自动退出，servmgr死亡时 `_exit(0)`。

## 分层架构

```text
IDevHostService (虚表: AddDevice, DelDevice, StartService, PmNotify, Dump)
  |
  DevHostService   (框架层: hostId, hostName, devices DList, observer)
  |
  DevHostServiceFull  (适配层: MessageTask + MessageLooper, 异步消息分发)
  |
  DevHostServiceStub  (IPC层: HdfRemoteService* + hostSvcMutex)
```

对象工厂映射（`devhost_object_config.c`）：

| Object ID | 创建函数 | 说明 |
|---|---|---|
| DEVMGR_SERVICE | DevmgrServiceProxyCreate | Manager代理(IPC客户端) |
| DEVSVC_MANAGER | DevSvcManagerProxyCreate | servmgr代理 |
| DEVHOST_SERVICE | DevHostServiceStubCreate | Host核心服务 |
| DRIVER_LOADER | HdfDriverLoaderFullCreate | dlopen加载器 |
| DEVICE_TOKEN | DeviceTokenStubCreate | 令牌IPC包装 |
| DEVICE_SERVICE | DeviceServiceStubCreate | 设备服务IPC包装 |

## 核心数据结构

| 结构 | 关键成员 | 说明 |
|---|---|---|
| DevHostService | hostId, hostName, devices(DList), observer | Host主对象 |
| HdfDeviceNode | deviceObject, driver, token, powerToken, policy, servName, devStatus, interfaceDesc | 设备节点核心结构 |
| HdfDriverEntry | moduleVersion, moduleName, Bind/Init/Release | 驱动入口(HDF_INIT注册) |
| HdfDriver | entry(HdfDriverEntry*), priv(dlopen handle) | 运行时驱动对象 |
| HdfDeviceInfo | deviceId, policy, preload, moduleName, svcName | HCS设备配置 |
| HdfDevice | devNodes(DList), devidIndex(动态ID从129起) | 逻辑设备(含多个节点) |

设备ID编码：`MK_DEVID(hostId, deviceId, nodeId)`，动态节点ID从129分配。

ServicePolicy：NONE(0) / PUBLIC(1) / CAPACITY(2) / FRIENDLY(3 host内订阅) / PRIVATE(4 同devId订阅)

VDI：HdfVdiBase(CreateVdiInstance/DestoryVdiInstance) + HdfVdiObject(dlHandler+vdiBase)，声明 `HDF_VDI_INIT`

## 进程启动流程

```text
main() [devhost.c / drivers_peripheral/devhost/host/devhost.c]
  -> ParseCommandLineArgs: -i hostId -n hostName -p procPri -s schedPri -m mallopt
  -> InitializeHost: PDEATHSIG(SIGKILL), SetProcTitle, SetPriority, SetMallopt
  -> StartHostService:
     -> DevHostServiceNewInstance: Stub->Full->DevHostService 三层构造
     -> DevHostDumpInit()
     -> StartService:
        -> DevmgrServiceClntAttachDeviceHost: IPC(重试100次×20ms)注册到devmgr
        -> HdfSysEventNotifyRegister(POWER事件)
        -> HdfPowerManagerInit()
        -> looper->Start() [主事件循环]
  [退出]: FreeInstance, PowerManagerExit, DumpDeInit
```

## 驱动加载机制

**dlopen动态加载**：`dlopen_ns(moduleName, RTLD_LAZY)` → `dlsym(handle, "driverDesc")` → HdfDriverEntry

驱动.so通过 `HDF_INIT(g_xxxEntry)` 导出全局 `driverDesc` 指针。

**内核静态加载**：链接器段 `HDF_DRIVER_BEGIN()/END()` 遍历（内核/LiteOS模式）。

**VDI二级加载**：`HdfLoadVdi(libName)` → realpath安全验证 → dlopen_ns + dlsym("hdfVdiDesc") → CreateVdiInstance

## 设备生命周期

### AddDevice流程

IPC ADD_DEVICE → 反序列化HdfDeviceInfo → async MessageTask → DevHostServiceAddDevice:

1. QueryOrAddDevice(查找/创建HdfDevice)
2. driverLoader->GetDriver(moduleName) (dlopen加载)
3. HdfDeviceNodeNewInstance(创建DeviceServiceStub+设置字段+读HCS属性)
4. device->Attach: 动态devId分配(129起) → LaunchNode

**LaunchNode序列**：Bind(设置deviceObject.service) → Init(硬件初始化) → PublishService(IPC注册servmgr+本地observer) → AttachDevice(IPC通知devmgr)

### DelDevice流程

IPC DEL_DEVICE → async → DevHostServiceDelDevice → HdfDeviceDetach → UnlaunchNode:

**UnlaunchNode序列**：Release(_rwlock保护) → RemoveService(IPC注销) → DetachDevice(IPC通知devmgr) → ReclaimDriver(MemFree不dlclose)

## 服务观察/订阅

HdfServiceObserver：hash key查找Record → Publish(设置publisher+NotifySubscribers) / Subscribe(创建Subscriber+如publisher存在立即回调)

驱动通过 `HdfDeviceSubscribeService()` → deviceObject→devNode→hostService→observer 调用。

## IPC Stub/Proxy

| 组件 | 方向 | 功能 |
|---|---|---|
| DevHostServiceStub | devmgr→host | 3命令: AddDevice/DelDevice/Dump, mutex序列化 |
| DevmgrServiceProxy | host→devmgr | AttachDeviceHost(重试100次), Attach/DetachDevice |
| DevSvcManagerProxy | host→servmgr | Add/Update/Remove Service, 死亡检测→_exit(0) |
| DeviceServiceStub | 客户端→驱动 | rwlock保护下ioService->Dispatch(code,data,reply) |
| DeviceTokenStub | — | 最小IPC stub(dispatch返回HDF_SUCCESS)，Binder身份对象 |

接口令牌检查：DeviceServiceStub强版本 `HdfDeviceObjectCheckInterfaceDesc` → HdfRemoteServiceCheckInterfaceToken

## 电源管理

系统事件映射：SUSPEND→POWER_STATE_SUSPEND, DISPLAY_OFF→DOZE_SUSPEND, RESUME→POWER_STATE_RESUME, DISPLAY_ON→DOZE_RESUME

传播：唤醒反向遍历(后设备先醒)，休眠正向遍历(先设备先睡)

PowerStateToken两种模式：
- **SYS_CTRL**：系统事件推送，直接调用listener->Suspend/Resume/Doze
- **DYNAMIC_CTRL**：HdfSRef引用计数wake lock，Acquire(首次→Resume)/Release(末次→Suspend)

PSM状态：IDLE→ACTIVE(Resume)→INACTIVE(Suspend)

异步PM：`HdfPmAcquire/ReleaseDeviceAsync` → PM任务队列专用线程处理

## Peripheral Devhost

可执行文件入口在 `drivers_peripheral/devhost/host/devhost.c`（同adapter/uhdf2/host/devhost.c）。

构建关系：可执行文件hdf_devhost(仅main入口, peripheral repo) + 共享库libhdf_host(框架+适配逻辑, core repo)，运行时链接。

## 代码路径映射

| 层 | 目录 | 关键文件 |
|---|---|---|
| 框架层 | `framework/core/host/` | devhost_service.c(核心), hdf_device_node.c(设备生命周期), hdf_device_object.c(驱动API), hdf_driver_loader.c/ hdf_load_vdi.c(加载), hdf_service_observer.c(本地服务) |
| 接口层 | `framework/core/shared/include/` | devhost_service_if.h, power_state_token_if.h |
| Inner API | `interfaces/inner_api/host/` | hdf_device_desc.h(驱动API+HDF_INIT), hdf_device_node.h, hdf_device_info.h, hdf_pm.h, hdf_load_vdi.h |
| 适配层 | `adapter/uhdf2/host/` | devhost.c(main入口), devhost_service_stub.c(IPC stub), devmgr_service_proxy.c/devsvc_manager_proxy.c(IPC proxy), driver_loader_full.c(dlopen), device_service_stub.c(设备IO dispatch), devhost_object_config.c(工厂配置) |
| Peripheral | `drivers_peripheral/devhost/` | host/devhost.c(main入口), host/BUILD.gn(链接libhdf_host) |

## 改动规则

- Host代码分布在 `framework/core/host/` + `adapter/uhdf2/host/`，**不要只改一侧**
- 可执行文件入口在peripheral repo，框架逻辑在core repo libhdf_host
- 新增Manager-Host交互先在 `framework/core/shared/include/` 定义接口
- 驱动.so必须导出 `driverDesc` 符号(HDF_INIT)；VDI导出 `hdfVdiDesc`(HDF_VDI_INIT)
- 服务发布分IPC(PUBLIC/CAPACITY→servmgr)和本地(FRIENDLY/PRIVATE→observer)
- DeviceServiceStub Dispatch在pthread_rwlock保护下，改IO分发需注意读写锁
- AddDevice/DelDevice通过MessageTask异步处理，不要改为同步(避免IPC死锁)
- PM有SYS_CTRL和DYNAMIC_CTRL两种模式，改PM逻辑先确认目标模式
- 新增对象类型在devhost_object_config.c Creator映射表注册

## 相关文档

- [hdf_devmgr.md](hdf_devmgr.md)

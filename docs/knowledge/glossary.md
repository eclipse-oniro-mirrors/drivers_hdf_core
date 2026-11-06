# 术语表

## 驱动管理进程（hdf_devmgr / DevmgrService）
HDF 子系统的中央管理守护进程，运行为独立系统服务（SA-ID 5100 的 IServiceManager 端点）。负责 Host 生命周期管理（启动/管理 `devhost_*` 子进程）、设备驱动加载/卸载、服务注册与发现、电源状态传播、PnP uevent 处理、Debug dump。入口为 `adapter/uhdf2/manager/device_manager.c`，就绪信号 `bootevent.hdf_devmgr.ready=true`。与"Host 进程"的区别：Manager 管理设备生命周期，不直接加载驱动；Host 孵化驱动实例但不管理其他 Host。详见 `docs/knowledge/hdf_devmgr.md`。

## 驱动孵化进程（hdf_devhost / DevHostService）
标准系统用户空间驱动孵化进程，每种 host 类型独立进程（`devhost_*`）。负责 dlopen 动态加载驱动 .so、执行 Bind/Init/Release 生命周期、发布驱动服务到 servmgr、接收电源事件传播。设置 `PR_SET_PDEATHSIG(SIGKILL)` 在 Manager 死亡时自动退出。注意：可执行文件入口在 `drivers/peripheral/devhost/host/devhost.c`，链接本仓的 `libhdf_host` 共享库；本仓 `adapter/uhdf2/host/devhost.c` 含 `main()` 但本仓 BUILD.gn 不构建它。详见 `docs/knowledge/hdf_devhost.md`。

## Manager-Host-Shared 三层架构
HDF 框架核心的组织模式：`framework/core/shared/` 定义接口契约（虚表 + 头文件，不承载业务）；`framework/core/manager/` + `adapter/uhdf2/manager/` 实现 Manager 侧；`framework/core/host/` + `adapter/uhdf2/host/` 实现 Host 侧。三层对应同一套接口契约的不同实现，支持跨进程复用。

## 对象工厂（HdfObjectManager）
HDF 核心对象的创建机制。`HDF_OBJECT_ID_*`（8 类，定义于 `framework/core/shared/include/hdf_object_manager.h:18-29`）通过 `HdfObjectManagerGetCreators(objectId)` 查 Creator 表创建实例。不同进程环境（kernel/uhdf2/uhdf）注册不同 Creator 表，使同一框架代码在不同环境实例化不同实现（如 Host 中 `DEVMGR_SERVICE` 创建 Proxy，Manager 中创建 Stub）。配置在 `adapter/uhdf2/manager/devmgr_object_config.c` 与 `adapter/uhdf2/host/devhost_object_config.c`。

## SBUF（HdfSBuf）
HDF 的核心序列化机制，用于 Manager-Host、客户端-驱动之间的数据传递。`enum HdfSbufType`（`interfaces/inner_api/utils/hdf_sbuf.h:50-55`）有 4 成员：`SBUF_RAW`（用户-内核直通，无 DriverBinder）、`SBUF_IPC`（进程间 IPC，构建于 `libhdf_ipc_adapter.z.so`）、`SBUF_IPC_HW`（保留位，未启用）、`SBUF_TYPE_MAX`（sentinel）。提供 `HdfSbufWriteUint32/ReadUint32/WriteBuffer` 等类型化读写接口。与 Binder parcel 的区别：SBUF 是 C 接口、类型自描述、跨内核/用户态通用。

## HCS（Hardware Configuration Source）
HDF 的设备配置文本格式，以 `.hcs` 为后缀。采用 `root { device_info { host { device { deviceNode {...} } } } }` 树形结构描述 host/device/deviceNode 及其属性。经 `hc-gen` 编译为 `.hcb` 二进制，运行时由 `device_resource_if`（`interfaces/inner_api/utils/device_resource_if.h`）解析。与 Linux Device Tree 的区别：HCS 是 HDF 专属配置，描述驱动设备拓扑与匹配属性（`deviceMatchAttr`），不描述硬件寄存器。

## hc-gen
HCS 编译器，位于 `framework/tools/hc-gen/`。将 `.hcs` 文本编译为 `.hcb` 二进制 blob，供运行时 `GenerateCfgTree`（`framework/utils/src/hcs_parser/hcs_parser.c`）解析。新增 HCS 语法必须先扩展 hc-gen。

## hdi-gen / idl-gen
HDI 代码生成工具，位于 `framework/tools/hdi-gen/` 与 `framework/tools/idl-gen/`。从 IDL（`.idl`）描述自动生成 HDI proxy/stub/service/driver 代码。生成物不可手改。业务 IDL 位于 `drivers/interface/` 仓；本仓仅含 hdi-gen 自身的测试 fixtures（`framework/tools/hdi-gen/test/**/*.idl`）。

## HDI（Hardware Device Interface）
OpenHarmony 硬件设备接口，由 IDL 描述经 hdi-gen 生成 proxy/stub/service/driver。客户端经 `libhdi` 库调用 proxy，proxy 经 IPC 到达 stub，stub 转发到 service 实现。本仓提供 `interfaces/inner_api/hdi/` 头文件（`idevmgr_hdi.h`、`iservmgr_hdi.h` 等）与 `libhdi` 库。

## OSAL（Operating System Abstraction Layer）
操作系统抽象层，统一封装 mutex/sem/spinlock/atomic/thread/timer/mem/time/irq/io/cdev/firmware。接口定义于 `interfaces/inner_api/osal/shared/`（`osal_mem.h`、`osal_mutex.h`、`osal_sem.h` 等）。实现按内核分散：Linux → `adapter/khdf/linux/osal/`，LiteOS-A → `adapter/khdf/liteos/osal/`，LiteOS-M → `adapter/khdf/liteos_m/osal/`，UniProton → `adapter/khdf/uniproton/osal/`，HongMeng → `adapter/khdf/hongmeng/osal/`；用户态 POSIX → `framework/support/posix/` 与 `adapter/uhdf/posix/`。

## HdfDriverEntry
驱动入口结构体（`interfaces/inner_api/host/shared/hdf_device_desc.h:202-240`），含 `moduleVersion`、`moduleName`、`Bind`/`Init`/`Release` 三函数指针。驱动通过 `HDF_INIT(g_xxxEntry)` 宏注册，用户态导出 `driverDesc` 符号（`adapter/uhdf2/host/src/driver_loader_full.c:28`），内核态落入 `HDF_DRIVER_BEGIN()/END()` 链接器段。HCS `deviceNode.moduleName` 匹配 `HdfDriverEntry.moduleName` 触发加载。

## HdfDeviceObject
设备对象（`interfaces/inner_api/host/shared/hdf_device_desc.h:95-113`），是驱动与框架交互的核心载体。含 `service`（`IDeviceIoService*`，驱动 Bind 时设置）、`property`（HCS 属性节点）、`deviceClass`、`priv`（驱动私有数据）。用户态额外含 `pthread_rwlock_t mutex` 保护 service 指针。框架创建后传给驱动的 Bind/Init/Release。

## HdfDeviceNode
设备节点（`interfaces/inner_api/host/shared/hdf_device_node.h:34-52`），是 Host 进程中设备实例的核心结构。含 `deviceObject`、`driver`（`HdfDriver*`）、`token`（`IHdfDeviceToken*`）、`powerToken`、`policy`、`servName`、`devStatus`（NONE/INITED/LAUNCHED）、`servStatus`。生命周期由 `HdfDeviceLaunchNode`（Bind→Init→PublishService→AttachDevice）和 `HdfDeviceUnlaunchNode`（Release→RemoveService→DetachDevice→ReclaimDriver）管理。

## HdfDevice
逻辑设备（`interfaces/inner_api/host/shared/hdf_device.h:51-57`），含 `devNodes`（DList）与 `devidIndex`。一个 `HdfDevice` 可含多个 `HdfDeviceNode`（同一 deviceId 不同 nodeId）。动态节点 ID 从 129 起分配（`framework/core/host/src/hdf_device.c:20`）。设备 ID 编码 `MK_DEVID(hostId, deviceId, nodeId)`：8 位 hostId + 16 位 deviceId + 8 位 nodeId（`hdf_device.h:28-39`）。

## ServicePolicy（服务策略）
驱动服务发布策略（`interfaces/inner_api/host/shared/hdf_device_desc.h:56-70`）：`NONE`(0) 不提供服务；`PUBLIC`(1) 内核态应用可用；`CAPACITY`(2) 内核+用户态可用（经 servmgr IPC 注册）；`FRIENDLY`(3) host 内订阅，不经 IPC；`PRIVATE`(4) 同 devId 订阅。PUBLIC/CAPACITY 经 `DevSvcManagerClntAddService` 注册到 servmgr，FRIENDLY/PRIVATE 经 `HdfServiceObserver` 本地发布。

## DevicePreload（预加载策略）
驱动加载策略（`interfaces/inner_api/host/shared/hdf_device_desc.h:80-85`）：`ENABLE`(0) 系统启动时加载；`ENABLE_STEP2`(1) OS 启动后加载（快速启动模式）；`DISABLE`(2) 不自动加载，按需 LoadDevice。Manager 启动后 `LoadLeftDriver` 加载 STEP2 设备。

## PowerStateToken
电源状态令牌（`framework/core/host/src/power_state_token.c`），每个注册了 `IPowerEventListener` 的 `HdfDeviceNode` 持有一个。两种模式（`interfaces/inner_api/host/shared/hdf_pm.h:14-18`）：`SYS_CTRL` 系统事件推送（直接调 listener Suspend/Resume/Doze）；`DYNAMIC_CTRL` 引用计数 wake lock（`HdfSRef`，Acquire 首次→Resume，Release 末次→Suspend）。异步 PM 经 `HdfPmTaskQueue` 专用线程处理（`framework/core/host/src/hdf_power_manager.c`）。

## HdfServiceObserver
本地服务观察者（`framework/core/host/src/hdf_service_observer.c`），实现 host 内 FRIENDLY/PRIVATE 策略的服务订阅。以 `HdfStringMakeHashKey(svcName)` 为 key 查找 `HdfServiceObserverRecord`。`PublishService` 设置 publisher 并通知已有 subscribers；`SubscribeService` 创建 subscriber，如 publisher 已存在则立即回调 `OnServiceConnected`。驱动通过 `HdfDeviceSubscribeService()` → deviceObject→devNode→hostService→observer 调用。

## HdfIoService
驱动服务 IO 对象（`interfaces/inner_api/core/hdf_io_service_if.h:108-117`），客户端经 `HdfIoServiceBind(serviceName)` 获取。含 `dispatcher`（`HdfIoDispatcher*`），其 `Dispatch(service, cmdId, data, reply)` 方法是客户端调用驱动 IO 的入口。平台设备用户态 API（`xxx_if_u.c`）封装此接口。

## DevHostServiceClnt
Manager 侧的 Host 客户端记录（`framework/core/manager/src/devhost_service_clnt.c`），含 `hostService`（proxy 代理）、`devices`/`unloadDevInfos`/`dynamicDevInfos`（HCS 配置与动态设备信息 DList/SList）、`hostId`/`hostName`/`hostPid`。Manager 通过它管理各 devhost 进程。

## DevSvcManager
服务注册管理器（`framework/core/manager/src/devsvc_manager.c`），维护全局服务注册表（`services` DList）与服务状态监听器（`svcstatListeners`）。devhost 通过 `DevSvcManagerClntAddService` 注册服务，客户端通过 SA-ID 5100 查询。含 SELinux 检查与 `devObjHolderList` 有效性追踪（`adapter/uhdf2/manager/devsvc_manager_stub.c`）。

## VDI（Vendor Driver Interface）
厂商驱动接口二级加载机制（`interfaces/inner_api/host/uhdf/hdf_load_vdi.h`）。驱动通过 `HDF_VDI_INIT(module)` 导出 `hdfVdiDesc` 符号（`HdfVdiBase*`）。`HdfLoadVdi(libName)` 经 `realpath` 安全验证路径后 `dlopen_ns` + `dlsym("hdfVdiDesc")` 加载，调用 `CreateVdiInstance` 创建实例。与普通驱动加载（`driverDesc`）的区别：VDI 用于厂商接口层，版本需调用方校验（`HdfGetVdiVersion`）。

## 平台设备三层结构（core/if/if_u）
平台外设（I2C/SPI/GPIO 等）的归一化代码组织：`xxx_core.c`（核心抽象 + `PlatformManager` 管理）、`xxx_if.c`（内核态 API，直接调 core）、`xxx_if_u.c`（用户态 API，基于 `HdfIoServiceBind("HDF_PLATFORM_XXX_MANAGER")` + SBUF）。同一 core 实现服务两种调用环境。位于 `framework/support/platform/<设备>/`。

## MessageLooper / MessageTask
HDF 的异步消息机制（`framework/utils/include/hdf_message_looper.h`、`hdf_message_task.h`）。`HdfMessageLooper` 持有 `HdfMessageQueue`，`Start()` 进入循环取消息。`HdfMessageTask` 绑定 looper 与 handler，提供 `SendMessage`/`SendMessageLater`/`RemoveMessage`。Manager 与 devhost 的主事件循环均基于此，AddDevice/DelDevice 异步处理也经 MessageTask。

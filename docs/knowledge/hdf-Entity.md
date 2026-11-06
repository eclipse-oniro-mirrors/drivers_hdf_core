# 业务实体知识

HDF Core 模块核心业务实体的完整知识定义，涵盖静态概念、动态运行、规则约束与代码映射。

## 驱动入口（HdfDriverEntry）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 驱动入口（HdfDriverEntry） |
| 实体定义 | 驱动向 HDF 框架注册自身的入口结构，是框架识别并加载驱动的唯一契约。一个驱动对应一个 `HdfDriverEntry` 全局实例。 |
| 核心特征 | `moduleVersion`（int32 版本）、`moduleName`（const char*，与 HCS `deviceNode.moduleName` 匹配）、`Bind`/`Init`/`Release` 三函数指针。定义于 `interfaces/inner_api/host/shared/hdf_device_desc.h:202-240`。 |
| 类型/分类 | 按 `moduleName` 命名空间区分；按注册方式分用户态（`HDF_INIT` 导出 `driverDesc` 符号）与内核态（`HDF_DRIVER_BEGIN()/END()` 链接器段）。 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 驱动源码中 `HDF_INIT(g_xxxEntry)` → 宏展开注册全局指针。Host 进程 `driverLoader->GetDriver(moduleName)` 时：用户态 `dlopen_ns(moduleName)` + `dlsym("driverDesc")`（`adapter/uhdf2/host/src/driver_loader_full.c:33-73`）；内核态遍历 `HDF_DRIVER_BEGIN()`~`HDF_DRIVER_END()` 段（`framework/core/host/src/hdf_driver_loader.c:16-38`）。框架调用 `Bind` → `Init` → （生命周期结束时）`Release`。 |
| 状态流转 | 注册（编译/链接期）→ 已加载（dlopen/dlsym 成功）→ 已绑定（Bind 调用成功，`deviceObject.service` 设置）→ 已初始化（Init 成功）→ 已释放（Release 调用后，驱动 .so 不 dlclose）。 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | `moduleName` 必须与 HCS `deviceNode.moduleName` 完全匹配，否则驱动不会被加载。`Bind` 仅在 `policy == PUBLIC || CAPACITY` 时强制要求（`framework/core/host/src/hdf_device_node.c:75`）；`Init` 必须实现。`Release` 在驱动卸载或加载失败时调用。 |
| 并发约束 | 用户态 `Release` 在 `pthread_rwlock_wrlock(&deviceObject.mutex)` 写锁保护下调用，调用后立即置空 `deviceObject.service`（`framework/core/host/src/hdf_device_node.c:196-205`），防止 Dispatch 并发访问已释放的 service。 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 被 `HdfDriver`（运行时驱动对象，`entry` + `priv` 即 dlopen handle）持有；`HdfDriver` 被 `HdfDeviceNode` 持有。 |
| 下游影响 | `Bind` 设置的 `deviceObject.service`（`IDeviceIoService*`）被 `DeviceServiceStub` 用于 IPC Dispatch；`Init` 操作硬件后通过 `PublishService` 注册到 servmgr。 |
| 平级关联 | 与 `HdfVdiBase`（VDI 入口）平行：VDI 经 `HDF_VDI_INIT` 导出 `hdfVdiDesc`，由 `HdfLoadVdi` 加载，版本需调用方校验。 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `struct HdfDriverEntry`（`interfaces/inner_api/host/shared/hdf_device_desc.h:202`）、`struct HdfDriver`（`interfaces/inner_api/host/shared/hdf_driver.h`）。 |
| 核心方法 | `HdfDriverLoaderGetDriver(moduleName)`（用户态 `adapter/uhdf2/host/src/driver_loader_full.c:33`；内核态 `framework/core/host/src/hdf_driver_loader.c:40`）、`HdfDriverEntryConstruct()`（内核态段遍历，`framework/core/host/src/hdf_driver_loader.c:16`）、`DeviceDriverBind(devNode)`（`framework/core/host/src/hdf_device_node.c:65`）。 |

## 设备节点（HdfDeviceNode）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 设备节点（HdfDeviceNode） |
| 实体定义 | Host 进程中一个驱动实例的运行时载体，绑定 `HdfDriverEntry` 与 `HdfDeviceObject`，承载设备生命周期与 IO 服务。一个 `HdfDevice` 可含多个 `HdfDeviceNode`（同 deviceId 不同 nodeId）。 |
| 核心特征 | `deviceObject`（HdfDeviceObject，含 service/property/priv）、`driver`（HdfDriver*）、`token`（IHdfDeviceToken*）、`powerToken`（PowerStateToken*）、`policy`（ServicePolicy）、`servName`、`devStatus`（DEVNODE_NONE/INITED/LAUNCHED，`interfaces/inner_api/host/shared/hdf_device_node.h:54-58`）、`servStatus`（bool，是否已发布到 servmgr）、`interfaceDesc`。定义于 `hdf_device_node.h:34-52`。 |
| 类型/分类 | 按 `policy` 区分服务发布路径；按 `preload` 区分加载时机；动态节点 nodeId 从 129 分配（`framework/core/host/src/hdf_device.c:20`）。 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | Manager IPC `AddDevice` → devhost `MessageTask` 异步 → `DevHostServiceAddDevice`（`framework/core/host/src/devhost_service.c:63`）→ `QueryOrAddDevice` + `GetDriver` + `HdfDeviceNodeNewInstance` + `device->Attach`。`Attach` 内调 `LaunchNode`：`Bind` → `Init` → `PublishService`（PUBLIC/CAPACITY 到 servmgr + 本地 observer） → `AttachDevice`（IPC 通知 Manager）。卸载经 `UnlaunchNode`：`Release` → `RemoveService` → `DetachDevice` → `ReclaimDriver`。 |
| 状态流转 | `DEVNODE_NONE`(0) → `DEVNODE_INITED`(1)（`HdfDeviceNodeNewInstance` 后，`hdf_device_node.c:304`）→ `DEVNODE_LAUNCHED`(2)（`HdfDeviceLaunchNode` 开始时设置，`hdf_device_node.c:106`）→ `DEVNODE_INITED`（`UnlaunchNode` 结束，`hdf_device_node.c:221`）。`DEVNODE_LAUNCHED` 状态的节点在析构时先调 `UnlaunchNode`（`hdf_device_node.c:244`）。 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 同一 `deviceId + nodeId` 的 devNode 不可重复添加（`devhost_service.c:83` 返回 `HDF_ERR_DEVICE_BUSY`）。`policy == NONE` 或 `servName` 为空时不发布服务（`hdf_device_node.c:48`）。PUBLIC/CAPACITY 同时注册 servmgr 与本地 observer；FRIENDLY/PRIVATE 仅注册本地 observer。 |
| 并发约束 | `Release` 在 `deviceObject.mutex` 写锁下；`DeviceServiceStubDispatch` 在读锁下（`adapter/uhdf2/host/src/device_service_stub.c:37-57`）。AddDevice/DelDevice 经 `DevHostServiceStub` 的 `hostSvcMutex` 串行化（`devhost_service_stub.c:90`）。 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 被 `HdfDevice`（逻辑设备）的 `devNodes` DList 持有；`HdfDevice` 被 `DevHostService` 的 `devices` DList 持有。 |
| 下游影响 | `deviceObject.service`（`IDeviceIoService*`）被 `DeviceServiceStub` 包装为 `HdfRemoteService` 注册到 servmgr；`powerToken` 被 `PowerStateChange` 调用；`token` 被 `DevmgrServiceClntAttachDevice` 注册到 Manager。 |
| 平级关联 | 同一 `HdfDevice` 内的 devNodes 共享 `deviceId` 但 nodeId 递增；通过 `HdfServiceObserver` 实现 host 内服务订阅（FRIENDLY/PRIVATE）。 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `struct HdfDeviceNode`（`interfaces/inner_api/host/shared/hdf_device_node.h:34`）、`struct IDeviceNode`（虚表，`hdf_device_node.h:26-32`）。 |
| 核心方法 | `HdfDeviceNodeNewInstance(deviceInfo, driver)`（`framework/core/host/src/hdf_device_node.c:266`）、`HdfDeviceLaunchNode(devNode)`（`hdf_device_node.c:91`）、`HdfDeviceUnlaunchNode(devNode)`（`hdf_device_node.c:183`）、`HdfDeviceNodeConstruct/Destruct`（`hdf_device_node.c:224/237`）。 |

## 逻辑设备（HdfDevice）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 逻辑设备（HdfDevice） |
| 实体定义 | 承载同一 `deviceId` 下多个 `HdfDeviceNode` 的容器，管理动态节点 ID 分配。一个 HdfDevice 对应 HCS 中一个 `device` 节点。 |
| 核心特征 | `devNodes`（DListHead）、`deviceId`（devid_t，含 hostId/deviceId/nodeId 编码）、`devidIndex`（uint16，动态 nodeId 分配计数器）。定义于 `interfaces/inner_api/host/shared/hdf_device.h:51-57`。 |
| 类型/分类 | 不区分类型；按创建方式分配置加载（`deviceId` 来自 HCS）与动态添加（`deviceId` 由 Manager 分配）。 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `DevHostServiceAddDevice` 时 `DevHostServiceQueryOrAddDevice`（`devhost_service.c:46`）查找或创建 HdfDevice → `device->Attach(&device->super, devNode)`（`hdf_device.c:83`）分配动态 nodeId 并调 `LaunchNode`。`DelDevice` 时 `device->Detach` 调 `UnlaunchNode`，若 `devNodes` 空则 `DevHostServiceFreeDevice` 释放。 |
| 状态流转 | 创建（`HdfDeviceConstruct`，`DListHeadInit`）→ 挂载节点（`DListInsertTail`）→ 释放节点（`DListRemove`）→ 销毁（`HdfDeviceDestruct` 遍历释放剩余节点，`hdf_device.c:172`）。 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 动态 nodeId 从 129 起分配（`DYNAMIC_ALLOC_ID`，`hdf_device.c:20`），上限 `DEVNODEID_MASK`（255，8 位）。`FindUsableDevNodeId` 优先复用已释放的 nodeId（`hdf_device.c:29`）。`devidIndex` 超过 `DEVNODEID_MASK` 时返回 `HDF_FAILURE`（`hdf_device.c:57`）。 |
| 并发约束 | devNodes 的增删由 `DevHostServiceStub` 的 `hostSvcMutex` 串行保护；无独立锁。 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 被 `DevHostService.devices` DList 持有（`framework/core/host/src/devhost_service.c:237`）。 |
| 下游影响 | 管理 `HdfDeviceNode` 生命周期；devNode 全部释放后 HdfDevice 自身被释放。 |
| 平级关联 | 同一 Host 内多个 HdfDevice 通过 `DevHostService.devices` 组织；电源传播按 devices 正/反遍历（`devhost_service.c:210-222`）。 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `struct HdfDevice`（`interfaces/inner_api/host/shared/hdf_device.h:51`）、`struct IHdfDevice`（虚表，`hdf_device.h:43-49`）。 |
| 核心方法 | `HdfDeviceAttach`（`hdf_device.c:83`）、`HdfDeviceDetach`（`hdf_device.c:113`）、`AcquireNodeDeivceId`（`hdf_device.c:49`）、`HdfDeviceNewInstance`（`hdf_device.c:200`）。 |

## 设备信息（HdfDeviceInfo）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 设备信息（HdfDeviceInfo） |
| 实体定义 | 从 HCS 配置或 Manager IPC 下发的设备描述信息，是驱动加载的输入参数。Manager 与 devhost 都持有此结构。 |
| 核心特征 | `deviceId`、`policy`（ServicePolicy）、`priority`、`preload`（DevicePreload）、`permission`、`moduleName`、`svcName`、`deviceMatchAttr`、`deviceName`、`status`（HDF_SERVICE_USABLE/UNUSABLE）、`isDynamic`（bool）。定义于 `interfaces/inner_api/host/shared/hdf_device_info.h:28-42`。 |
| 类型/分类 | 配置设备（`isDynamic=false`，来自 HCS）与动态设备（`isDynamic=true`，由 `LoadDevice` 添加到 `dynamicDevInfos`）。 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | Manager 解析 HCS 得到 `HdfDeviceInfo` 列表 → 按 host 分发 → devhost `AddDevice` IPC 携带序列化的 `HdfDeviceInfo`（`adapter/uhdf2/host/src/devhost_service_stub.c:39` `DeviceAttributeDeserialize`）。devhost 据此 `GetDriver(moduleName)` + `HdfDeviceNodeNewInstance(deviceInfo, driver)`。 |
| 状态流转 | `HDF_SERVICE_UNUSABLE` → `HDF_SERVICE_USABLE`（LoadDevice 成功后 Manager 设置）。 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | `LoadDevice` 按 `svcName` 匹配 `dynamicDevInfos`（`framework/core/manager/src/devmgr_service.c:31`）。`preload == DISABLE` 的设备不自动加载，等待动态 LoadDevice。`priority` 决定同 host 内加载顺序（`MAX_PRIORITY_NUM = 200`，`hdf_device_desc.h:46`）。 |
| 并发约束 | Manager 侧 `dynamicDevInfos` 在 `g_hostsLock` 读锁下遍历查找（`devmgr_service.c:47`）。 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | Manager 侧被 `DevHostServiceClnt.devices`/`unloadDevInfos`/`dynamicDevInfos` 持有。 |
| 下游影响 | 驱动 `HdfDeviceNode` 的 `policy`/`servName`/`devId` 均源自此结构；`deviceMatchAttr` 用于 `HcsGetNodeByMatchAttr` 查找 HCS 属性节点（`hdf_device_node.c:297`）。 |
| 平级关联 | 同 host 内多个 `HdfDeviceInfo` 按 `priority` 排序。 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `struct HdfDeviceInfo`（`interfaces/inner_api/host/shared/hdf_device_info.h:28`）。 |
| 核心方法 | `HdfDeviceInfoNewInstance`/`Construct`/`FreeInstance`（`hdf_device_info.h:49-51`）、`DeviceAttributeDeserialize(data)`（`adapter/uhdf2/host/src/devhost_service_stub.c:39`，IPC 反序列化）。 |

## Host 客户端（DevHostServiceClnt）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | Host 客户端（DevHostServiceClnt） |
| 实体定义 | Manager 侧对一个 devhost 进程的代理记录，持有该 host 的代理对象、设备配置、PID 等运行时信息。 |
| 核心特征 | `hostService`（IDevHostService* proxy）、`devices`（DList，配置设备）、`unloadDevInfos`（SList，未加载设备）、`dynamicDevInfos`（SList，动态设备）、`hostId`、`hostName`、`hostPid`。定义于 `framework/core/manager/src/devhost_service_clnt.c`。 |
| 类型/分类 | 不区分类型；按 hostId 唯一标识。 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | Manager `DevmgrServiceStartDeviceHosts` → `DriverInstallerStartDeviceHost`（fork+exec devhost）→ devhost 回调 `AttachDeviceHost` → Manager 查找/创建 `DevHostServiceClnt` → `InstallDriver` 加载配置设备。Host 死亡时 `HdfDeathRecipient` → 异步 `MessageTask` → `CleanupDiedHostResources`（清 `hostPid`、回收代理、清设备令牌）。 |
| 状态流转 | 创建（host 启动）→ 已 AttachDeviceHost → 运行中 → 死亡（hostPid 失效）→ 清理回收。 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | `LoadDevice` 时若 `hostPid == INVALID_PID` 则同步等待 host 启动（`WAIT_HOST_SLEEP_CNT=1000` × `WAIT_HOST_SLEEP_TIME=2ms`，`devmgr_service.c:72-73`）。Host 死亡后所有设备令牌需异步清理，不能在 Binder 回调中同步操作（避免死锁）。 |
| 并发约束 | `hosts` DList 遍历在 `g_hostsLock`（`pthread_rwlock_t`）保护下。 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 被 `DevmgrService.hosts` DList 持有。 |
| 下游影响 | 持有 `DevHostServiceProxy`（IPC 代理），向 devhost 发 `AddDevice`/`DelDevice`/`Dump`；持有 `HdfDeviceInfo` 列表驱动设备加载。 |
| 平级关联 | 多个 `DevHostServiceClnt` 按 hostId 在 `hosts` DList 中组织；电源传播按 hosts 正/反遍历（`docs/knowledge/hdf_devmgr.md` 电源管理）。 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `struct DevHostServiceClnt`（`framework/core/manager/src/devhost_service_clnt.c`）。 |
| 核心方法 | `DevHostServiceClntNewInstance`、`DevHostServiceClntInstallDriver`、`DevHostServiceClntDetachHost`、`CleanupDiedHostResources`。 |

## 服务注册记录（DevSvcRecord / HdfServiceObserverRecord）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 服务注册记录（DevSvcRecord / HdfServiceObserverRecord） |
| 实体定义 | 两条平行的服务记录实体：`DevSvcRecord`（Manager 侧 servmgr 全局注册表项）与 `HdfServiceObserverRecord`（devhost 本地观察者记录，用于 FRIENDLY/PRIVATE 订阅）。 |
| 核心特征 | `HdfServiceObserverRecord`：`serviceKey`（hash）、`publisher`（HdfObject*）、`subscribers`（SList）、`policy`、`devId`、`obsRecMutex`（`framework/core/host/src/hdf_service_observer.c`）。`DevSvcRecord`：serviceName、`HdfDeviceObjectHolder`、`HdfRemoteService*`（Manager 侧）。 |
| 类型/分类 | 按 `serviceKey = HdfStringMakeHashKey(svcName, 0)` 索引。 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | devhost `PublishService`：PUBLIC/CAPACITY → `DevSvcManagerClntAddService` IPC 到 Manager `DevSvcManagerStub`（含 SELinux 检查）→ 写入 `DevSvcRecord`；FRIENDLY/PRIVATE → `HdfServiceObserverPublishService`（`hdf_service_observer.c:84`）设置 publisher 并通知 subscribers。客户端 `SubscribeService` → `HdfServiceObserverSubscribeService`（`hdf_service_observer.c:39`），如 publisher 已存在则立即回调 `OnServiceConnected`。 |
| 状态流转 | observer record：创建（首次 publish 或 subscribe）→ publisher 设置 → 通知 subscribers → 移除（`HdfServiceObserverRemoveRecord`，`hdf_service_observer.c:114`）。 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | PRIVATE 策略仅同 `devId` 的 subscriber 被通知（`hdf_service_observer.c:74`）。`DevSvcManagerStub` 含 SELinux 检查 + `devObjHolderList` 有效性追踪（`adapter/uhdf2/manager/devsvc_manager_stub.c`）。 |
| 并发约束 | observer `services` SList 增删在 `observerMutex` 下；record 内 `subscribers` 在 `obsRecMutex` 下（`hdf_service_observer.c:63/78`）。 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | observer record 被 `DevHostService.observer` 持有；DevSvcRecord 被 `DevSvcManager.services` 持有。 |
| 下游影响 | subscriber 的 `OnServiceConnected` 回调驱动业务逻辑；`DevSvcRecord` 的 `HdfRemoteService*` 被客户端 `HdfIoServiceBind` 获取。 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `struct HdfServiceObserverRecord`（`framework/core/host/include/hdf_service_observer.h`）、`struct DevSvcManager`（`framework/core/manager/src/devsvc_manager.c`）。 |
| 核心方法 | `HdfServiceObserverPublishService`/`SubscribeService`/`RemoveRecord`（`framework/core/host/src/hdf_service_observer.c`）、`DevSvcManagerClntAddService`/`RemoveService`（`framework/core/host/src/devsvc_manager_clnt.c`）。 |

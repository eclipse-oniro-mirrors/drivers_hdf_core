# 业务背景

## 应用定位

### 核心职责

OpenHarmony 驱动子系统核心（`@ohos/hdf_core`，简称 HDF Core）为 OpenHarmony 不同系统形态（standard/small/mini）提供**统一的驱动开发与部署框架**，目标是"一次开发，多系统部署"。本组件直接处理以下业务问题：

- **驱动生命周期管理**：通过中央管理进程 `hdf_devmgr` 按 HCS 配置启动/停止 `devhost_*` 子进程，完成驱动 Bind/Init/Release 流程，并支持按需动态加载/卸载（LoadDevice/UnloadDevice）。
- **驱动服务发布与发现**：统一通过 `DevSvcManager`（SA-ID 5100）注册驱动对外服务，客户端经 `HdfIoServiceBind(serviceName)` 获取 `HdfIoService` 并通过 `Dispatch(cmdId, data, reply)` 发起 IO 调用。
- **驱动入口与配置匹配**：驱动以 `HdfDriverEntry`（Bind/Init/Release 三接口）通过 `HDF_INIT` 注册，HCS `device_info` 段 `host → device → deviceNode` 树通过 `moduleName` 匹配驱动。
- **跨进程/跨态通信**：基于 SBUF 序列化 + IPC/Binder 桥接（标准系统）或用户-内核直通（`/dev/hdf/*` vnode）实现 Manager-Host、客户端-驱动、驱动-驱动之间的数据传递。
- **平台设备归一化 API**：I2C/SPI/GPIO/UART/PWM/ADC/DAC/RTC/Watchdog 等 20+ 类平台外设统一 core/if/if_u 三层结构，提供内核态与用户态两种调用路径。
- **OSAL 抽象层**：统一封装 mutex/sem/spinlock/atomic/thread/timer/mem/time/irq/io/cdev/firmware，按 5 个内核（Linux/LiteOS-A/LiteOS-M/UniProton/HongMeng）+ POSIX（用户态）分目录实现。
- **HCS 配置与工具链**：`hc-gen` 编译 `.hcs` 文本为 `.hcb` 二进制，运行时 `device_resource_if` 解析；`hdi-gen`/`idl-gen` 从 IDL 生成 HDI proxy/stub/service/driver 代码。
- **电源管理**：接收系统电源事件（suspend/resume/display off/on），按 SYS_CTRL 或 DYNAMIC_CTRL 模式向各驱动传播。
- **PnP 热插拔**：Netlink 监听内核 uevent，按 `hdf_pnp.cfg` 规则触发动态加载/卸载。
- **驱动通用框架模型**：为 Audio/Camera/Display/Input/Sensor/Network(WLAN)/USB/Storage/Misc 每类硬件提供归一化驱动模型。

### 职责边界（本组件不负责）

- **业务 HDI 接口定义与外设具体实现**：业务 IDL 位于 `drivers/interface/` 仓，具体外设驱动实现位于 `drivers/peripheral/` 与 `vendor/**`。本仓只提供框架、平台中间层、工具链与适配层。
- `hdf_devhost` **可执行文件入口**：本仓 `adapter/uhdf2/host/devhost.c` 含 `main()` 但本仓 BUILD.gn 不构建它；真正产出 `hdf_devhost` 可执行文件的是 `drivers/peripheral/devhost/host/BUILD.gn`，它链接本仓的 `libhdf_host` 共享库。本仓只产出 `adapter/uhdf2/host:libhdf_host`。
- **SELinux `.te` 策略**：由 `security_selinux_adapter` 仓维护，本仓依赖其组件但不包含 `.te` 文件。
- **内核源码与构建**：`adapter/khdf/<内核>/` 只做内核态适配，不引入内核构建系统变更。
- **厂商 SoC 板级平台驱动**：`adapter/platform/` 仅作 vendor 参考实现，不属产品链正式依赖。
- **应用层调用方**：上层系统服务（如 `samgr`、`account`、`camera_service`）作为 HDF 客户端使用本组件产出，但不由本组件管理。

## 系统上下文

```plantuml
@startuml
title HDF Core 系统上下文
left to right direction

actor "应用/系统服务" as App

package "OpenHarmony 系统" {
  component "samgr\n(SystemAbilityManager)" as Samgr
  component "init\n(系统启动)" as Init
  component "security_selinux_adapter\n(.te 策略)" as Selinux
  component "hilog\n(日志)" as Hilog
  component "ipc\n(Binder)" as Ipc
  component "bounds_checking_function\n(securec)" as Securec

  package "HDF Core (本仓)" {
    component "hdf_devmgr\n(管理守护进程)" as Devmgr
    component "devhost_*\n(驱动孵化进程\n链接 libhdf_host)" as Devhost
    component "libhdi\n(HDI 客户端库)" as Libhdi
    component "libhdf_platform\n(平台设备 API)" as Libplat
    component "hc-gen/hdi-gen\n(工具链)" as Tools
    component "HCS 解析器\n(device_resource_if)" as HcsParser
  }

  component "drivers/interface\n(业务 IDL)" as Idl
  component "drivers/peripheral\n(外设实现\n含 hdf_devhost main)" asPeriph
  component "vendor/**\n(厂商驱动)" as Vendor
  component "Linux/LiteOS/...\n内核" as Kernel
}

App --> Samgr : 按 SA-ID 获取服务
Samgr ..> Devmgr : SA-ID 5100\nIServiceManager
Init --> Devmgr : 启动 hdf_devmgr
Devmgr --> Devhost : fork+exec\nAttachDeviceHost/AddDevice
Devhost --> Samgr : 注册驱动服务\n(SA-ID 5100)
Devhost --> Periph : dlopen_ns 驱动.so\n(dlsym "driverDesc")
Devhost --> Idl : HDI proxy/stub\n(hdi-gen 生成)
Libhdi --> Samgr : 客户端获取 HDI 服务
Libplat --> Devhost : HdfIoServiceBind\n+ Dispatch(SBUF)
HcsParser --> Devmgr : 解析 .hcb
HcsParser --> Devhost : 解析 .hcb
Tools --> Idl : 从 .idl 生成代码
Devhost --> Kernel : /dev/hdf/* vnode\n+ ioctl(HDF_WRITE_READ)
Devmgr --> Kernel : Netlink uevent
Devmgr --> Selinux : 依赖 .te 策略
Devmgr --> Hilog : 日志
Devhost --> Hilog : 日志
Devhost --> Securec : snprintf_s 等
Devhost --> Ipc : Binder IPC
Vendor --> Devhost : 提供驱动 .so
@enduml
```

## 核心输入

| 输入类型 | 来源 | 主要参数/触发时机 |
|---|---|---|
| HCS 配置（`.hcb`） | `hc-gen` 编译产物，部署于 `/vendor/etc/hdfconfig/` | Manager 启动时 `HcsGetHostList` 解析 host 列表；devhost AddDevice 时 `HcsGetNodeByMatchAttr` 解析设备属性 |
| `AttachDeviceHost` IPC | devhost 启动后主动调用 Manager | hostId、hostName、`HdfRemoteService*`（stub 句柄） |
| `AddDevice`/`DelDevice` IPC | Manager → devhost | 序列化 `HdfDeviceInfo`（deviceId/policy/preload/moduleName/svcName/deviceMatchAttr） |
| `LoadDevice`/`UnloadDevice` HDI | 客户端经 `IDeviceManager` 调用 | serviceName 字符串，匹配 `dynamicDevInfos` |
| `Dispatch(cmdId, data, reply)` | 客户端经 `HdfIoServiceBind` 后调用 | cmdId 命令字、`HdfSBuf*` 数据/回复（SBUF_RAW 或 SBUF_IPC） |
| uevent | 内核 Netlink | action、subsystem、devpath 等字段，匹配 `hdf_pnp.cfg` 规则 |
| 电源事件 | `OsalSysEvent`（`HDF_POWER_EVENT` 等） | SUSPEND/RESUME/DISPLAY_OFF/DISPLAY_ON |
| 驱动 `HDF_INIT(g_xxxEntry)` | 驱动 .so 静态注册或链接器段 | 导出 `driverDesc` 符号（用户态）或 `HDF_DRIVER_BEGIN()/END()` 段（内核态） |

## 核心输出

| 输出类型 | 接收方 | 业务作用 |
|---|---|---|
| 就绪参数 `bootevent.hdf_devmgr.ready=true` | `init` | 通知系统 HDF 管理进程就绪，可继续启动依赖服务 |
| devhost 子进程 | 系统 | 每个 hostId 一个独立进程，承载驱动实例 |
| 注册的系统服务（SA-ID 5100 及各驱动服务） | `samgr` | 供客户端按 SA-ID 或 serviceName 发现并调用 |
| `HdfRemoteService*` 句柄 | 客户端 | 作为 `HdfIoService.dispatcher->Dispatch` 的目标，承载 IO 调用 |
| 驱动事件（`HdfDeviceSendEvent`） | 注册了 `HdfDevEventlistener` 的应用/驱动 | 驱动主动上报事件 |
| 电源状态传播 | 各 devhost → 各 `PowerStateToken` | 驱动 Suspend/Resume/Idle |
| Dump 输出 | CLI（`hdf_devmgr` / `devhost` Dump 命令） | 调试诊断设备/host/服务状态 |

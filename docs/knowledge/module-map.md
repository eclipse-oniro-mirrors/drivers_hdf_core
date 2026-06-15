# 模块路由

本文是改动归属速查。目录里能直接看出来的文件清单不在这里重复。

| 要改什么 | 优先看哪里 |
|---|---|
| 驱动框架核心逻辑、设备生命周期管理 | `framework/core/` |
| Manager 进程（设备管理守护进程） | `framework/core/manager/`、`adapter/uhdf2/manager/` |
| Host 进程（驱动加载与设备服务） | `framework/core/host/`、`adapter/uhdf2/host/` |
| Manager 与 Host 共享的接口契约 | `framework/core/shared/` |
| 驱动框架对外 C 语言头文件 | `framework/include/` |
| OSAL 抽象接口定义 | `framework/include/osal/`、`interfaces/inner_api/osal/` |
| 平台设备对外 API 头文件 | `framework/include/platform/` |
| 平台设备中间层实现 | `framework/support/platform/` |
| 平台设备内核态 API | `framework/support/platform/xxx_if.c` |
| 平台设备用户态 API | `framework/support/platform/xxx_if_u.c` |
| POSIX OSAL 实现 | `framework/support/posix/` |
| 基础数据结构（SBUF、Map、SList） | `framework/utils/` |
| HCS 解析器、device_resource_if | `framework/utils/hcs_parser/` |
| Audio 驱动模型 | `framework/model/audio/` |
| Display/LCD 驱动模型 | `framework/model/display/` |
| Input/Touchscreen 驱动模型 | `framework/model/input/` |
| Sensor 驱动模型 | `framework/model/sensor/` |
| WLAN 驱动模型 | `framework/model/network/` |
| USB 驱动模型 | `framework/model/usb/` |
| Storage/MMC/MTD 驱动模型 | `framework/model/storage/` |
| Camera 驱动模型 | `framework/model/camera/` |
| Misc（Vibrator/Light/DSoftBus） | `framework/model/misc/` |
| HCS 编译器 | `framework/tools/hc-gen/` |
| HDI 代码生成器 | `framework/tools/hdi-gen/` |
| IDL 生成器 | `framework/tools/idl-gen/` |
| HDF 调试工具 | `framework/tools/hdf_dbg/` |
| 驱动生态开发工具 | `framework/tools/hdf_dev_eco_tool/` |
| 标准系统用户态适配 | `adapter/uhdf2/` |
| 内核态 Linux 适配 | `adapter/khdf/linux/` |
| 内核态 LiteOS-A 适配 | `adapter/khdf/liteos/` |
| 内核态 LiteOS-M 适配 | `adapter/khdf/liteos_m/` |
| 内核态 UniProton 适配 | `adapter/khdf/uniproton/` |
| 内核态 HongMeng 适配 | `adapter/khdf/hongmeng/` |
| 轻量系统用户态适配 | `adapter/uhdf/` |
| 平台设备 SoC 板级适配 | `adapter/platform/` |
| HDF 内部稳定 API（跨模块边界） | `interfaces/inner_api/` |
| HDI 客户端接口 | `interfaces/inner_api/hdi/` |
| IPC 适配接口 | `interfaces/inner_api/ipc/` |
| SBUF 序列化接口 | `interfaces/inner_api/utils/hdf_sbuf.h` |

## 容易放错的位置

- 内核特定代码不要直接写在 `framework/` 中，应放在 `adapter/khdf/<kernel>/` 对应目录。
- 驱动业务逻辑不要散落在 `core` 或 `support`，应优先在对应 `model/` 子目录实现。
- 平台设备用户态 API 修改时先确认目标：内核态改 `xxx_if.c`，用户态改 `xxx_if_u.c`，核心抽象改 `xxx_core.c`。
- HDI 生成物（proxy/stub/service）不要手改，应通过 `hdi-gen` 工具从 IDL 重新生成。
- HCS 配置文件修改后需确认 `hc-gen` 是否支持新增语法，并同步检查 `.hcb` 编译流程。
- Manager 进程逻辑改 `framework/core/manager/` 和 `adapter/uhdf2/manager/`，Host 进程逻辑改 `framework/core/host/` 和 `adapter/uhdf2/host/`，不要混淆。
- `adapter/platform/` 是 SoC 板级平台驱动适配实现，不是平台框架中间层；框架中间层在 `framework/support/platform/`。
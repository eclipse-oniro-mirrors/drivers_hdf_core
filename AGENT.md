# OpenHarmony HDF Core 驱动框架指引

## 项目定位

本仓库是 OpenHarmony 驱动子系统的核心源码，包含驱动框架（HDF）、配置管理（HCS）、配置解析、驱动通用框架模型、硬件通用平台能力接口等。核心职责是为不同系统形态（standard/small/mini）提供统一的驱动开发与部署环境，力求做到一次开发，多系统部署。

仓库代码最终编译为 **hdf_devmgr**（设备管理守护进程）、**libhdf_host**（驱动宿主共享库）、**libhdi**（HDI 客户端库）、**libhdf_platform**（平台设备用户态 API）等产物，由系统服务加载使用。

## 快速路由

- `framework/core/`：驱动框架核心实现，采用 Manager-Host-Shared 三层架构（对应子目录 `manager/`、`host/`、`shared/`、`common/`、`adapter/`、`sec/`）；Manager 负责设备生命周期管理，Host 负责驱动加载与设备服务分发，Shared 定义跨进程接口契约。
- `framework/include/`：驱动框架对外 C 语言头文件，子域覆盖 `osal/`（+ 根级 `osal.h`）、`platform/`、`utils/`、`wifi/`、`audio/`、`bluetooth/`、`camera/`、`net/`、`ethernet/`。
- `framework/model/`：驱动通用框架模型，按硬件类别组织：`audio/`、`camera/`、`display/`、`input/`、`misc/`、`network/`、`sensor/`、`storage/`、`usb/`。
- `framework/support/platform/`：平台设备驱动框架中间层（`include/`、`src/`、`test/`），每种外设（I2C、SPI、GPIO 等）均有 core/if/if_u 三层实现。
- `framework/support/posix/`：POSIX OSAL 实现，用于轻量系统或用户态环境。
- `framework/utils/`：基础数据结构（SBUF、Map、SList、DList）和 HCS 解析器实现。目录结构 `include/`、`src/`；HCS 解析器位于 `src/hcs_parser/`（含 `hcs_parser.c`、`hcs_blob_if.c`、`hcs_tree_if.c`、`device_resource_if.c` 等）。
- `framework/tools/`：开发工具链——`hc-gen/`（HCS 编译器）、`hcs-view/`（HCS 可视化）、`hdi-gen/`（HDI 代码生成器）、`idl-gen/`（IDL 生成器）、`hdf_dbg/`（调试工具）、`hdf_dev_eco_tool/`（驱动生态工具）。
- `framework/sample/`：驱动示例（`config/`、`platform/`），仅供参考，不参与产品编译。
- `framework/test/`：框架测试——`unittest/`（GTest 单测）、`coretest/`、`fuzztest/`（Fuzz 用例）。
- `adapter/uhdf2/`：标准系统（standard）用户态适配层，含 `manager/`、`host/`、`ipc/`、`hdi/`、`hcs/`、`platform/`、`osal/`、`shared/`、`utils/`、`security/`、`pub_utils/`、`include/`、`test/`。
- `adapter/uhdf/`：小型系统（small）用户态适配层，含 `manager/`、`platform/`、`posix/`、`test/`；不要与 `uhdf2/` 混改。
- `adapter/khdf/`：内核态适配层，子目录 `linux/`、`liteos/`、`liteos_m/`、`uniproton/`、`hongmeng/`。
- `adapter/platform/`：厂商侧平台驱动适配（`can/gpio/i2c/mipi_dsi/pwm/spi/uart/watchdog`），非框架代码，供 vendor 参考。
- `adapter/build/`：构建期公共产物（如 `test_common/`）。
- `interfaces/inner_api/`：HDF 对外稳定内部 API，子域 `core/`、`hdi/`、`host/`、`ipc/`、`osal/`、`utils/`；跨模块使用，属最高兼容性等级。
- `docs/knowledge/`：稳定背景知识文档（`module-map.md`、`hdf_devmgr.md`、`hdf_devhost.md`）。
- `figures/`：设计文档配图，仅供 docs 引用。

构建入口是 `adapter/BUILD.gn`（`uhdf_entry` 子组件）。提交使用 `git commit -s`（DCO 签名），commit subject 遵循 `<scope>: <summary>` 格式（scope 使用如 `hdf_core`、`hdi-gen`、`uhdf2/host`、`khdf/linux` 等本仓已有前缀）。

> 高频变更路径：`framework/core/`（核心架构变更）、`adapter/uhdf2/`（适配层变更）、`framework/model/`（新增驱动模型）、`framework/support/platform/`（平台设备驱动）。
> 高风险路径：`framework/include/`（对外 API 头文件，修改需兼容性审查）、`interfaces/inner_api/`（跨模块边界 API）。

### Where to look（任务 → 起点路径）

| 任务类型 | 起点路径 | 备注 |
|---|---|---|
| 新增/修改驱动模型 | `framework/model/<类别>/` | 每类硬件已有归一化框架，不要散落到 core/support |
| 新增平台外设 API | `framework/support/platform/<设备>/` | 分 core/if/if_u 三层；用户态改 `xxx_if_u.c`（`HdfIoService` + SBUF），内核态改 `xxx_if.c`（直接调 `xxx_core.c`） |
| 修改 Manager-Host 交互 | 先 `framework/core/shared/` 定义契约，再 `framework/core/manager/` 与 `framework/core/host/` 双侧实现 | 契约不承载业务 |
| 标准系统用户态适配 | `adapter/uhdf2/<manager\|host\|ipc\|hdi\|hcs\|platform\|osal\|shared\|security\|utils\|pub_utils\|include\|test>/` | 只做适配，不写业务；子目录清单与仓库 13 个 `adapter/uhdf2/*` 一一对应 |
| 小型系统用户态适配 | `adapter/uhdf/<manager\|platform\|posix\|test>/` | 与 uhdf2 平行，不要交叉修改 |
| 内核态适配 | `adapter/khdf/<linux\|liteos\|liteos_m\|uniproton\|hongmeng>/` | 内核相关代码必须在此，不进 `framework/` |
| 厂商平台适配（vendor） | `adapter/platform/<设备>/` | 参考实现，正式产品需 vendor 自维护 |
| HCS 语法/编译 | `framework/tools/hc-gen/` | 新增语法必须先扩 hc-gen |
| HCS 可视化 | `framework/tools/hcs-view/` | 独立子工程，改动不影响 hc-gen |
| HDI/IDL 生成 | `framework/tools/hdi-gen/`、`framework/tools/idl-gen/` | 生成物不可手改，改 IDL 后重跑工具 |
| 对外 C 头文件 | `framework/include/<子域>/` | 修改需 API 兼容性审查 |
| 跨模块稳定 API | `interfaces/inner_api/<core\|hdi\|host\|ipc\|osal\|utils>/` | 修改需跨模块兼容评估，视为最高兼容性等级 |
| 单元测试 | `framework/test/unittest/` | GTest 用例 |
| Fuzz 测试 | `framework/test/fuzztest/` | 新增/修改公开接口后建议补 fuzz 用例 |
| 示例参考 | `framework/sample/` | 只读参考，不参与产品编译 |

### 嵌套指南

若子目录下存在自身的 `AGENT.md`、`README.md` 或 `docs/` 说明，以子目录指南为准；本文件仅提供仓根级路由与约束。开工前可用以下命令一次性枚举所有子级指南：

```bash
# 注意：
# 1) git pathspec 的 `**` 不匹配仓根（不含斜杠的路径），需同时列出根级和递归两种 pattern
# 2) 本仓部分子目录还提供中文 README（README_zh.md），改动前建议一并检视
git ls-files \
  "AGENT.md" "AGENTS.md" "CLAUDE.md" ".cursorrules" "GEMINI.md" "README.md" "README_zh.md" \
  "**/AGENT.md" "**/AGENTS.md" "**/CLAUDE.md" "**/.cursorrules" "**/GEMINI.md" "**/README.md" "**/README_zh.md"
```

当前仓库实际存在的嵌套指南清单（改动对应目录前必读）：

| 路径 | 适用范围 |
|---|---|
| `README.md` / `README_zh.md` | 仓根简介、目录结构、License 概览 |
| `adapter/README.md` | `adapter/**` 适配层总览 |
| `adapter/uhdf/README.md` | 小型系统用户态适配层 |
| `adapter/uhdf2/README.md` | 标准系统用户态适配层 |
| `adapter/khdf/linux/README.md` | Linux 内核适配 |
| `adapter/khdf/liteos/README.md` | LiteOS-A 内核适配 |
| `adapter/khdf/liteos_m/README.md` | LiteOS-M 内核适配 |
| `adapter/khdf/uniproton/README.md` | UniProton 内核适配 |
| `adapter/khdf/hongmeng/README.md` | HongMeng 内核适配 |
| `framework/README.md` | 框架整体说明 |
| `framework/tools/hcs-view/README_zh.md` | HCS 可视化工具总览（仅中文版） |
| `framework/tools/hcs-view/hcsVSCode/README.md` | HCS VSCode 插件 |
| `framework/tools/hcs-view/hcsWebView/test/README.md` | HCS WebView 测试 |
| `framework/tools/idl-gen/docs/README.md` | idl-gen 使用文档 |

> 若清单与实际不一致，以上述 `git ls-files` 输出为准并同步更新本表。

## 知识索引

稳定背景知识放在 `docs/knowledge/`。改动前先按目录定位，再读取对应文档：

| 场景 | 先读 |
|---|---|
| 顶层目录职责、模块边界、术语约定 | `docs/knowledge/module-map.md` |
| 驱动管理进程 hdf_devmgr 架构、启动流程、设备生命周期、IPC 机制 | `docs/knowledge/hdf_devmgr.md` |
| 驱动孵化进程 hdf_devhost 架构、驱动加载、设备服务发布、电源管理 | `docs/knowledge/hdf_devhost.md` |
| 任务中出现基础术语：SBUF / HCS / hc-gen / hdi-gen / IDL / OSAL / Manager-Host-Shared / DevHostService / DevmgrService | 先读 `docs/knowledge/module-map.md#模块路由` 对应条目 |
| 任务中出现 C 类型/结构体：`HdfDeviceObject` / `HdfDriverEntry` / `HdfSBuf` / `HdfIoService` / `HdfDeviceIoClient` / `IServStatListener` / `HDF_STATUS`（枚举，定义于 `interfaces/inner_api/utils/hdf_base.h:42-77`） | 先读 `docs/knowledge/module-map.md#模块路由`，若涉及跨进程再读 `docs/knowledge/hdf_devmgr.md` 或 `hdf_devhost.md` |
| 任务中出现标识符/字段名：`hostId`（DevHost 编号）、`serviceName`（HDF 服务名）、`device_resource_if.h`（HCS 解析入口，见 `interfaces/inner_api/utils/device_resource_if.h`） | 先读 `docs/knowledge/module-map.md#模块路由`（服务命名/HCS 段） |
| 任务中出现 OSAL 内部术语（均定义于 `interfaces/inner_api/osal/shared/`）：`OsalMutex` / `OsalSem` / `OsalSpinlock` / `OsalAtomic` / `OsalThread` / `OsalTimer`、内存分配 `OsalMemAlloc` / `OsalMemCalloc` / `OsalMemAllocAlign`（见 `osal_mem.h`）、时间 `OsalSleep`（见 `osal_time.h`）；实现按内核分散在 `adapter/khdf/<内核>/osal/`、`adapter/uhdf/posix/`、`framework/support/posix/` | 先读 `docs/knowledge/module-map.md#模块路由`（OSAL 抽象接口定义 / POSIX OSAL 实现 / 对应内核适配）+ `#容易放错的位置`（"内核特定代码不要写在 framework/"条）；改动接口签名同步走 API 兼容性评审 |
| 任务中出现 model 层专有术语：`AudioCard`/`CodecDevice`/`DaiDevice`/`SapmCtrlConfig`（audio）、`PanelInfo`/`MipiDsiDesc`/`BacklightDev`(display)、`TouchChipDevice`/`TouchChipOps`/`KeyDriverInfo`（input）、`SensorDevInfoNode`/`SensorReportEvent`（sensor）、`NetDevice`/`HdfWifiNetDeviceData`/`FlowControlModule`/`PriorityQueue`（network/wifi）、`BufferQueue`/`CameraBuffer`/`SubDevice`（camera）、`UsbnetTransInfo`/`UsbPnpNotifyServiceInfo`/`UsbPnpNotifyInterfaceInfo`（usb）、`MmcCntlr`/`MmcDevice`/`MmcBlock`（storage/mmc） | 先读对应 `framework/model/<类别>/` 下 header 注释或 `docs/knowledge/module-map.md#模块路由` 中对应硬件类行；若需跨进程理解再读 `docs/knowledge/hdf_devmgr.md` 或 `hdf_devhost.md` |
| 任务中直接提及 hdf_devmgr 管理进程、设备生命周期、Manager-Host IPC | 先读 `docs/knowledge/hdf_devmgr.md` |
| 任务中直接提及 devhost 驱动加载、设备服务发布、电源管理 | 先读 `docs/knowledge/hdf_devhost.md` |

路径级触发（改动这些目录前必读；`module-map.md` 实际只有两个章节：`# 模块路由`（H1）与 `## 容易放错的位置`（H2），以下条目均指向这两节内对应的**表格行**或**条目**）：

| 改动路径 | 必读章节 · 具体条目 |
|---|---|
| `framework/core/manager/` 或 `adapter/uhdf2/manager/` 或 `adapter/uhdf/manager/` | `docs/knowledge/hdf_devmgr.md` §分层架构 / §设备生命周期 / §代码路径映射 |
| `framework/core/host/` 或 `adapter/uhdf2/host/` | `docs/knowledge/hdf_devhost.md` §分层架构 / §驱动加载机制 / §代码路径映射 |
| `framework/core/shared/` | `docs/knowledge/module-map.md#模块路由` 中"Manager 与 Host 共享的接口契约"行 |
| `framework/model/**` | `docs/knowledge/module-map.md#模块路由` 中对应硬件类别行（Audio/Display/Input/Sensor/WLAN/USB/Storage/Camera/Misc） |
| `framework/support/platform/**` | `docs/knowledge/module-map.md#模块路由` 中"平台设备中间层实现"行 + "平台设备内核态 API"/"用户态 API"两行 |
| `framework/include/**` 或 `interfaces/inner_api/**` | `docs/knowledge/module-map.md#模块路由` 中"驱动框架对外 C 语言头文件"、"HDF 内部稳定 API"行 + `docs/knowledge/hdf_devhost.md` §IPC Stub/Proxy（章节标题含 `/`，GFM 锚点为 `#ipc-stubproxy`） |
| `framework/tools/hdi-gen/` 或本仓 `.idl`（均在 `framework/tools/hdi-gen/test/**`；业务 IDL 位于 `drivers/interface` 仓） | `docs/knowledge/module-map.md#模块路由` 中"HDI 代码生成器"行 + `#容易放错的位置` 中"HDI 生成物（proxy/stub/service）不要手改"条 |
| `framework/tools/hc-gen/` 或任何 `.hcs` 文件 | `docs/knowledge/module-map.md#模块路由` 中"HCS 编译器"行 + `#容易放错的位置` 中"HCS 配置文件…`.hcb` 编译流程"条 |
| `adapter/khdf/<内核>/osal/` | `docs/knowledge/module-map.md#模块路由` 中对应内核适配行（linux/liteos/liteos_m/uniproton/hongmeng）+ `#容易放错的位置` 中"内核特定代码不要写在 framework/"条 |
| `framework/support/posix/` 或 `adapter/uhdf/posix/` | `docs/knowledge/module-map.md#模块路由` 中"POSIX OSAL 实现"行 |
| `adapter/uhdf/**`（小型系统） | `docs/knowledge/module-map.md#模块路由` 中"轻量系统用户态适配"行 + `adapter/uhdf/README.md` |
| `adapter/platform/**`（vendor SoC 板级） | `docs/knowledge/module-map.md#模块路由` 中"平台设备 SoC 板级适配"行 + `#容易放错的位置` 中"adapter/platform/ 是 SoC 板级平台驱动适配实现"条 |

> 注：若 `module-map.md` 缺失所需子章节，应在 PR 中同步补章节而非绕过引用。

> 编辑前声明（强制格式）：在首次回复开头以固定块输出以下 4 项，缺一不可：
>
> ```
> 【任务类别】 <新增驱动 | 修改框架核心 | 适配层变更 | 工具链变更 | 文档/示例 | 其它>
> 【已读文档】
>   - <docs/knowledge/xxx.md#章节标题>
>   - <相关子目录 README.md 路径>
> 【已识别约束】
>   - <类别>: 引用 AGENT.md 第 N 节 / 第 M 条（例：Ask before 第 3 条：新增第三方依赖 → 需法务评审）
>   - <类别>: 引用具体路由文档条目（例：AGENT.md《Do not》"错误码只能新增，不得复用"条；或 `module-map.md#容易放错的位置` 对应条目）
> 【风险等级】 <高 | 中 | 低>（依据 AGENT.md《风险等级与审批链》表判定）
> ```
>
> 未按此块声明的回复视为流程违规，Reviewer 有权直接打回。

## 核心规则

- 驱动框架采用 Manager-Host 多进程模型：`hdf_devmgr` 管理进程负责设备生命周期，每个 hostId 对应一个 `DevHostService` 进程负责驱动加载。新增进程间交互逻辑时先确认归属。
- **hdf_devmgr 进程**的代码分布在 `framework/core/manager/`（框架层）和 `adapter/uhdf2/manager/`（适配层），不要只改一侧。进程入口为 `adapter/uhdf2/manager/device_manager.c`。
- **hdf_devhost 进程**的代码分布在 `framework/core/host/`（框架层）和 `adapter/uhdf2/host/`（适配层），可执行文件入口在 `adapter/uhdf2/host/devhost.c`，链接 `libhdf_host` 共享库；`adapter/uhdf2/host/src/` 下含 devhost 服务实现（`devhost_service_full.c`、`devhost_service_stub.c`、`devhost_dump.c`、`devhost_object_config.c`、`device_service_stub.c`、`device_token_stub.c`）与代理/客户端（`devmgr_service_proxy.c`、`devsvc_manager_proxy.c`、`hdf_devsvc_manager_clnt.c`）、驱动加载与线程模型（`driver_loader_full.c`、`hdf_device_thread.c`）、电源管理（`hdf_pm_reg.c`）。
- `framework/core/shared/` 是接口契约层，不承载业务行为。新增 Manager-Host 交互应先在这里定义接口。
- `adapter/uhdf2/ipc/` 是 C 框架到 OHOS C++ IPC 的桥接，不承载 HDF 业务逻辑。
- 新增驱动入口通过 `HdfDriverEntry` 结构体注册（Bind/Init/Release 三接口），并在 HCS `device_info` 段下的 `host → device → deviceNode` 树中通过 `moduleName` 匹配对应驱动（示例见 `framework/sample/config/device_info/device_info.hcs`）。
- 平台设备 API 分内核态（`xxx_if.c`，直接调用 core 层 `xxx_core.c` 函数）和用户态（`xxx_if_u.c`，基于 `HdfIoService` + SBUF），改动时先确认目标运行环境。
- OSAL 层按内核分目录（与 `adapter/khdf/<内核>/` 的 5 个内核一一对应）：Linux → `adapter/khdf/linux/osal/`，LiteOS-A → `adapter/khdf/liteos/osal/`，LiteOS-M → `adapter/khdf/liteos_m/osal/`，UniProton → `adapter/khdf/uniproton/osal/`，HongMeng → `adapter/khdf/hongmeng/osal/`；用户态 POSIX 实现在 `framework/support/posix/` 与 `adapter/uhdf/posix/`。不要直接在 `framework/` 里写内核特定代码。
- HCS 配置文件经 `hc-gen` 编译为 `.hcb` 二进制，运行时通过 `device_resource_if.h` 解析。新增配置节点时同步检查 `hc-gen` 是否支持对应语法。
- HDI 接口由 `hdi-gen` 工具从 IDL 描述自动生成 proxy/stub/service/driver 代码，不要手改生成物。
- SBUF 是 HDF 的核心序列化机制，`enum HdfSbufType`（定义于 `interfaces/inner_api/utils/hdf_sbuf.h:50-55`）实际有 4 个成员：`SBUF_RAW`（用户-内核直通，DriverBinder-less 场景，如 kernel <-> user）、`SBUF_IPC`（进程间 IPC，构建于 `adapter/uhdf2/ipc:libhdf_ipc_adapter`，运行时产物 `libhdf_ipc_adapter.z.so`）、`SBUF_IPC_HW`（当前源码注释为 "Reserved for extension"，作为扩展保留位，改动前先与 SIG-Driver 确认语义）、`SBUF_TYPE_MAX`（sentinel，不作为业务类型使用）。新增数据传递场景时先确认 SBUF 类型。
- 驱动模型（model）为每类硬件提供归一化框架，新增外设驱动优先在对应 model 目录下实现，不要直接散落到 core 或 support。
- **API 兼容性**：修改 `framework/include/` 或 `interfaces/inner_api/` 中的头文件时，不得删除或修改已有公开函数的签名；新增函数需确认不与其他模块冲突。
- **安全边界**：HDF IPC 涉及 SELinux 检查，新增 IPC 接口或服务注册逻辑时需确认安全策略兼容性。
- **第三方依赖**：新增第三方库依赖需经架构评审，不得在框架核心代码中引入新许可证。
- **硬件操作**：平台设备 API 修改需确认不会在真实硬件上造成破坏性 IO 操作。
- **DFX 与日志**：修改日志打印级别或新增日志输出时不得引入敏感信息泄露；故障归属和诊断逻辑变更需与现有 HDF 事件机制兼容。
- **常见陷阱**：agent 容易在 `framework/` 中直接写内核特定代码（应放在 `adapter/khdf/`）；容易在 `adapter/uhdf2/ipc/` 中混入业务逻辑（该目录仅作 IPC 桥接）；容易单侧修改 manager 或 host 而忽略另一端。

### Do not（禁止项）

- 不要手改 `hdi-gen` / `idl-gen` / `hc-gen` 生成的代码；改动 IDL/HCS 后重新执行工具再提交。
- 不要在 `framework/` 目录下引入任何内核头文件（`<linux/*.h>` 等）或内核专属 API；内核适配一律进 `adapter/khdf/<内核>/`。
- 不要在 `adapter/uhdf2/ipc/` 中加入业务逻辑；该目录仅承担 C 框架到 OHOS C++ IPC 的桥接。
- 不要单侧修改 Manager 或 Host（`framework/core/*` 与 `adapter/uhdf2/*` 需同步）。
- 不要跨适配层混改（`adapter/uhdf/` 与 `adapter/uhdf2/` 分别服务 small/standard 系统，代码不共用）。
- 不要在框架代码中直接调用厂商适配（`adapter/platform/` 仅作 vendor 参考实现，不属产品链）。
- 不要删除或修改 `framework/include/**`、`interfaces/inner_api/**` 中已发布函数的签名、参数语义、错误码或生命周期；错误码只能新增，且必须落在已划定的错误码段。
- 不要在 `framework/core/shared/` 中承载业务代码，该目录仅定义跨进程接口契约。
- 不要在日志/DFX 中打印敏感信息（token、key、用户数据、IPC payload 原文）。
- 不要在 `adapter/uhdf/`、`adapter/uhdf2/` 等 Apache-2.0 许可目录引入 GPL/LGPL/AGPL 依赖；本仓采用**多重许可**（详见根目录 `LICENSE`）：`framework/` 与 `adapter/platform/` 为 GPLv2 或 BSD-3-Clause 双许可、`adapter/khdf/linux/` 为 GPLv2、`adapter/khdf/liteos*/` 为 BSD-3-Clause、`adapter/uhdf/`、`adapter/uhdf2/` 等目录为 Apache-2.0（`bundle.json:5` 声明的顶层 License 是 Apache-2.0，仅代表组件默认标签）。新增第三方依赖需明确其许可证与目标目录兼容，第三方 NOTICE / `README.OpenSource` 需同步更新。
- 不要绕过 `hdf_devmgr` 直接跨 host 调用驱动服务；必须通过 `DevmgrService` 注册与查询。
- 不要提交空 commit 或不带 `Signed-off-by` 的 commit（DCO 强制）。
- 不要将测试代码或 `framework/sample/` 引用为产品依赖。
- 不要在改动本仓公开 API/HDI 契约后忽视下游仓：`framework/include/**`、`interfaces/inner_api/**` 的 header 被 `drivers/interface/`、`drivers/peripheral/`、`vendor/**` 等下游仓 include，改动前须用 `grep -rln '<header名>' drivers/interface drivers/peripheral vendor 2>/dev/null` 列出下游依赖并给出联动 PR 清单或兼容性方案。

### Ask before（改前先确认）

1. 修改 `framework/include/**` 或 `interfaces/inner_api/**` 中已有 API：需 API 兼容性评审（负责人：本仓 SIG-Driver Maintainer）。
2. 新增 IPC 接口或服务注册：需确认 SELinux 策略（相关 `.te` 由 `security_selinux_adapter` 仓维护，本仓需同步给出策略变更清单）、权限校验点、调用者 UID/PID 白名单。
3. 新增第三方依赖或引入新许可证：需架构评审 + 法务/合规评审，并按 OpenHarmony 规范新增/更新 `README.OpenSource`（文件名以 `OAT.xml` 规范为准），并同步 `bundle.json` 的 `deps.components` / `deps.third_party`。
4. 修改 SBUF 类型（`SBUF_RAW` / `SBUF_IPC` / `SBUF_IPC_HW`，见 `interfaces/inner_api/utils/hdf_sbuf.h:50-55`）或序列化格式：需确认跨版本兼容与内核/用户态一致性，并给出 wire-format 迁移方案；`SBUF_IPC_HW` 当前为保留位，启用前须走架构评审。
5. 修改平台设备驱动中会触发真实硬件 IO 的路径：需硬件评审并给出回滚方案。
6. 修改 HCS 语法：需先在 `hc-gen` 中支持并回归历史 `.hcs` 编译。
7. 新增或首次适配某个内核（如 `HongMeng` / `UniProton` / 新 LiteOS 变体）到 `adapter/khdf/<内核>/`：需架构评审并约定 OSAL 覆盖范围。
8. 修改 `framework/support/posix/`（POSIX OSAL）：需确认对轻量系统/用户态 host 的兼容性，避免与内核态 OSAL 语义漂移。
9. 影响 devmgr/devhost 启动时长、IPC 单次调用延迟、Bind/Init 超时阈值等**性能敏感路径**：需给出基线数据（rk3568 参考产品）与前后对比，回归幅度 > 5% 需评审。
10. 修改错误码定义（`HDF_STATUS` 及其扩展）：需评审是否影响调用方兼容性，且严禁复用已废弃码值。

### 风险等级与审批链

| 变更类别 | 风险等级 | 审批链 |
|---|---|---|
| `interfaces/inner_api/**`、`framework/include/**` 已有 API 签名/错误码 | 高 | SIG-Driver Maintainer + 兼容性评审 |
| `framework/core/shared/`、SBUF wire-format、HDI IDL 契约 | 高 | SIG-Driver Maintainer + 架构评审 |
| SELinux/权限、第三方依赖、新内核适配 | 高 | 架构评审 + 安全/合规评审 |
| Manager/Host 内部实现（非契约层）、`adapter/uhdf2/**` 适配层 | 中 | 模块 Owner Review |
| `framework/model/**`、`framework/support/platform/**` 新增外设 | 中 | 模块 Owner Review |
| 文档、注释、`framework/sample/`、日志字面量微调（不改级别） | 低 | 单 Reviewer |

## 测试与验证

> 本仓通过 `bundle.json` 声明的 `uhdf_test_entry` 组件承载测试；单个测试目标为标准 OpenHarmony `ohos_unittest`/`ohos_fuzztest`，其 `module_output_path = "hdf_core/hdf_core"`。以下命令按此实际入口给出。

### 组件级测试入口

```bash
# 顶层测试组件（覆盖 uhdf2/hdi、uhdf2/host、uhdf2/osal、uhdf2/shared、uhdf2/test、build/test_common 等 unittest；完整清单见 adapter/BUILD.gn 的 uhdf_test_entry group）
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 --build-target "drivers/hdf_core/adapter:uhdf_test_entry"
```

### 平台设备单测（受 gn feature 开关控制）

`hdf_core_platform_test_support` 是 `bundle.json:17` 登记的 feature 名，其 `declare_args()` 默认值 `false` 定义于 `adapter/uhdf/test/platform_test.gni:10` 与 `adapter/uhdf2/test/BUILD.gn:18`；启用后编译 `adapter/uhdf/test/unittest/platform:hdf_adapter_uhdf_test_platform` 与 `adapter/uhdf2/test/unittest/platform:hdf_adapter_uhdf_test_platform`：

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 \
  --build-target "drivers/hdf_core/adapter:uhdf_test_entry" \
  --gn-args="hdf_core_platform_test_support=true"
```

### Fuzz 测试（真实 target 名）

```bash
cd ${OHOS_ROOT}
# 框架 fuzz（framework/test/fuzztest/BUILD.gn 定义 group("hdf_framework_fuzztest")）
./build.sh --product-name rk3568 --build-target "drivers/hdf_core/framework/test/fuzztest:hdf_framework_fuzztest"

# 平台设备 fuzz（framework/support/platform/test/fuzztest/BUILD.gn 定义 group("hdf_platform_fuzztest")）
./build.sh --product-name rk3568 --build-target "drivers/hdf_core/framework/support/platform/test/fuzztest:hdf_platform_fuzztest"
```

### 单个模块单测直接跑

以 `adapter/uhdf2/test/unittest/manager/BUILD.gn` 中的 `hdf_adapter_uhdf_test_manager` 为例：

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 \
  --build-target "drivers/hdf_core/adapter/uhdf2/test/unittest/manager:hdf_adapter_uhdf_test_manager"
```

其他真实存在的单测目标（可在 `${OHOS_ROOT}` 下用 `grep -rn 'ohos_unittest(' drivers/hdf_core` 列出；仓内相对路径为 `grep -rn 'ohos_unittest(' .`），例如：`DevMgrTest`、`DevHostTest`、`HdiServiceManagerTest`、`HdiSmqTest`、`stub_collector_test`、`object_collector_test`、`buffer_handle_test`、`native_buffer_test`、`OsalTest`、`dev_attribute_serialize_test`、`hdf_adapter_uhdf_test_osal`、`hdf_adapter_uhdf_test_config`、`khdf_audio_adm_test` 等。

## 编译命令

### 推荐编译策略

根据修改内容选择合适的编译方式：

| 场景 | 编译命令 | 耗时（参考） | 说明 |
|------|----------|--------------|------|
| **仅修改源码（增量）** | `ninja -C out/<product> <target>` | 几秒 ~ 数十秒 | 最快，跳过 GN gen 与 Python 封装 |
| **修改 GN 配置（增量）** | `./build.sh --fast-rebuild` | ~1-2 分钟（仅 GN gen） + 增量编译 | 跳过 preload，只重跑 GN gen 与解析依赖 |
| **首次编译或 GN 报错** | `./build.sh` | 单组件 ~3-5 分钟；含全量依赖首次 ~10-30 分钟 | 完整走 preload / GN gen / ninja；耗时受机器与产品差异影响很大 |

### 编译 HDF Core 组件（真实入口 `uhdf_entry`）

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 --build-target "drivers/hdf_core/adapter:uhdf_entry"
```

### 快速编译（ninja 直接调用；仅修改源码时使用）

```bash
cd ${OHOS_ROOT}
# 说明：GN 生成的 phony 别名在 build.ninja 里以 `drivers/hdf_core/adapter$:uhdf_entry` 表示
# （`$:` 是 ninja 语法里对 `:` 的转义），命令行传参时使用原始形式 `drivers/hdf_core/adapter:uhdf_entry`；
# 或直接用 stamp 目标 `obj/drivers/hdf_core/adapter/uhdf_entry.stamp`。
# 若你的 out 目录不是 out/rk3568，请以 `hb env` 或 `${OHOS_ROOT}/out/ohos_config.json` 中的实际路径为准。
prebuilts/build-tools/linux-x86/bin/ninja -w dupbuild=warn -C out/rk3568 \
  obj/drivers/hdf_core/adapter/uhdf_entry.stamp
```

### 快速重编译（仅修改源码）

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 \
  --build-target "drivers/hdf_core/adapter:uhdf_entry" --fast-rebuild
```

### 指定 CPU 架构

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 \
  --build-target "drivers/hdf_core/adapter:uhdf_entry" --target-cpu arm64
```

### 启用平台测试的 feature 开关（真实 gn arg）

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 \
  --build-target "drivers/hdf_core/adapter:uhdf_test_entry" \
  --gn-args="hdf_core_platform_test_support=true"
```

> 已知可用的本仓 gn feature（来自 `bundle.json:15-21`）：`hdf_core_khdf_test_support`、`hdf_core_platform_test_support`、`hdf_core_platform_rtc_test_support`、`hdf_core_default_peripheral_config`、`hdf_core_default_hicollie_config`。**不要臆造未在 `bundle.json` / `*.gn` 中定义的开关**（例如 `hdf_core_enable_clang_tidy` 本仓当前未定义）。

## 静态分析

本仓 `bundle.json` 与 `*.gn` 中**未定义**任何 `*_enable_clang_tidy` 类开关；静态分析依赖 OpenHarmony 主干构建集成，具体 gn 开关名（如 `use_clang_tidy` / `enable_check_abi` 等）以 `${OHOS_ROOT}/build/` 目录下主干构建脚本为准（`grep -R 'declare_args' ${OHOS_ROOT}/build/` 确认）。**不要臆造未在真实 gn 文件中出现的开关**。

推荐做法（按优先级）：

```bash
# 1) 优先使用 OpenHarmony 主干的 clang-tidy 集成（开关名以主干为准，先本地验证再写入流水线）
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 \
  --build-target "drivers/hdf_core/adapter:uhdf_entry" \
  --gn-args="use_clang_tidy=true"   # 若主干未定义则跳过并转用 (2)
```

```bash
# 2) 手工执行 clang-tidy（对本次 diff 涉及的 C/C++ 文件；POSIX/bash）
# 注 1：--diff-filter=d 排除已删除的文件，避免 clang-tidy 打不开被 rm 的路径报错
# 注 2：BASE 变量兼容本仓的多种 remote 命名（gitcode/master、origin/master、m/master 等）
#      也支持 HEAD~1 或个人 fork 场景。命令自动选择第一个可用的 base ref。
BASE=$(git rev-parse --verify --quiet origin/master \
       || git rev-parse --verify --quiet gitcode/master \
       || git rev-parse --verify --quiet m/master \
       || echo "HEAD~1")
git diff --name-only --diff-filter=d "$BASE"...HEAD -- '*.c' '*.cpp' '*.h' \
  | xargs -r -I{} clang-tidy {} -- -I. -Iframework/include -Iinterfaces/inner_api
```

```powershell
# 2') Windows PowerShell 等价写法
$base = (git rev-parse --verify --quiet origin/master) `
     ?? (git rev-parse --verify --quiet gitcode/master) `
     ?? (git rev-parse --verify --quiet m/master) `
     ?? 'HEAD~1'
git diff --name-only --diff-filter=d "$base...HEAD" -- '*.c' '*.cpp' '*.h' |
  Where-Object { $_ } |
  ForEach-Object { clang-tidy $_ -- -I. -Iframework/include -Iinterfaces/inner_api }
```

告警基线：以上面 `$BASE` 解析出的 base ref 同命令产物为基线，本次变更**不得引入新增告警**。若主干工具链尚未产出报告，需在 PR 中给出手工 clang-tidy 结论截图或日志，并注明所使用的 clang-tidy 版本与 base ref。

## 多产品验证

核心变更（`framework/core/**`、`interfaces/inner_api/**`、`framework/tools/hdi-gen/**`）应至少在 `bundle.json` 声明的三类形态（`adapted_system_type: ["standard", "small", "mini"]`）各选一款典型产品编译通过：

| 系统形态 | 参考产品（示例，以 vendor 实际交付为准） | 编译目标 |
|---|---|---|
| standard | `rk3568` | `drivers/hdf_core/adapter:uhdf_entry` |
| small | `ipcamera_hispark_taurus` | `drivers/hdf_core/adapter:uhdf_entry` |
| mini | `wifiiot_hispark_pegasus` | `drivers/hdf_core/adapter:uhdf_entry` |

命令示例：

```bash
cd ${OHOS_ROOT}
./build.sh --product-name ipcamera_hispark_taurus \
  --build-target "drivers/hdf_core/adapter:uhdf_entry"
./build.sh --product-name wifiiot_hispark_pegasus \
  --build-target "drivers/hdf_core/adapter:uhdf_entry"
```

> 说明：产品名（`--product-name`）以本地 OpenHarmony 主干 `vendor/<company>/<product-dir>/config.json` 中的 `product_name` 字段为准（例如 `vendor/hisilicon/hispark_taurus/config.json` 定义 `ipcamera_hispark_taurus`）；若指定产品未在本机 OpenHarmony 树中，改用同形态的等效产品。

## 任务专项检查

- 修改对外/内部 API 头文件后，运行签名差异检查，确认无破坏性改动：
  ```bash
  # 注：git pathspec 默认不递归 `**`，需要显式加 `:(glob)` magic
  git diff -- ':(glob)framework/include/**/*.h' ':(glob)interfaces/inner_api/**/*.h'
  ```
  若涉及删除、改签名、改错误码或改生命周期，走 Ask before 流程。
- 修改 hdi-gen 工具或其测试 fixtures（`framework/tools/hdi-gen/test/**/*.idl`，共 51 个）后重新生成并核对：
  ```bash
  # 使用本仓 hdi-gen 工具（framework/tools/hdi-gen/main.cpp）；
  # 具体命令行参数以 framework/tools/hdi-gen/BUILD.gn 与 main.cpp 为准。
  # 生成完后（pathspec 加 :(glob) 以递归匹配）：
  git status -- ':(glob)**/*_proxy.*' ':(glob)**/*_stub.*' ':(glob)**/*_service.*' ':(glob)**/*_driver.*'
  git diff   -- ':(glob)**/*_proxy.*' ':(glob)**/*_stub.*' ':(glob)**/*_service.*' ':(glob)**/*_driver.*'
  ```
  注：本仓不含**业务** IDL（产品 IDL 由 `drivers/interface` 仓承载）；这里排查的是 hdi-gen 本身的测试期望产物。手写的框架 proxy（如 `adapter/uhdf2/host/src/devmgr_service_proxy.c`）不在此约束内，需按文件头 header/所在目录人工判断。
- 修改任何 `.hcs` 后，用 `hc-gen`（`framework/tools/hc-gen/`）重新编译为 `.hcb`，并回归历史 `.hcs` 是否仍能通过。
- 修改 `adapter/khdf/<内核>/` 后，确认其他内核目录（`linux/`、`liteos/`（LiteOS-A）、`liteos_m/`（LiteOS-M）、`uniproton/`、`hongmeng/`）未被同步误改；跨内核变更需分别验证。
- 修改 SELinux 相关 IPC 接口后，附上对应 `.te` 变更清单（本仓不含 `.te`，`bundle.json:32` 依赖 `selinux_adapter` 组件；需在 PR 描述里指向 `security_selinux_adapter` 仓对应 PR）。
- DCO 与 commit-message 检查（跨平台命令）：
  ```bash
  # POSIX / bash：列出缺少 Signed-off-by 的 commit
  # 说明：
  #   - `valueonly,unfold` 让 trailer 单行输出，避免多行折叠导致 awk 误判
  #   - `NF>1` 过滤 `git log` 在末尾追加的空行分隔
  #   - BASE 解析兼容 origin/master、gitcode/master、m/master 与个人 fork（HEAD~1）
  BASE=$(git rev-parse --verify --quiet origin/master \
         || git rev-parse --verify --quiet gitcode/master \
         || git rev-parse --verify --quiet m/master \
         || echo "HEAD~1")
  git log --format='%H|%(trailers:key=Signed-off-by,valueonly,unfold)' "$BASE"..HEAD \
    | awk -F'|' 'NF>1 && $2==""{print $1}'
  ```
  ```powershell
  # PowerShell 等价写法
  $base = (git rev-parse --verify --quiet origin/master) `
       ?? (git rev-parse --verify --quiet gitcode/master) `
       ?? (git rev-parse --verify --quiet m/master) `
       ?? 'HEAD~1'
  git log --format='%H|%(trailers:key=Signed-off-by,valueonly,unfold)' "$base..HEAD" |
    Where-Object { $_ -match '\|' } |
    ForEach-Object {
      $p = $_ -split '\|', 2
      if ([string]::IsNullOrWhiteSpace($p[1])) { $p[0] }
    }
  ```
  两者输出应为空；有输出即为未签名 commit 的 SHA 列表。
- License / NOTICE 检查（新增第三方依赖时）：本仓根目录**许可证相关**文件为 `LICENSE`（多重许可，见"Do not"里的许可分区说明；`bundle.json:5` 顶层标注为 `Apache License 2.0`）与 `OAT.xml`（合规扫描规则）。OpenHarmony 规范的第三方来源声明文件为 `README.OpenSource`（见本仓 `OAT.xml` 中的 `readmeOpenSourcefileNamePolicyFilter`）。若引入第三方依赖，需按该规范新增 `README.OpenSource` 并同步 `bundle.json` 的 `deps.components` / `deps.third_party`。

> 如果验证命令无法运行（如缺少测试环境或硬件），在最终回复中说明原因、列出已执行的验证步骤、以及无法覆盖的验证项。可接受的替代证据示例：
> - **无编译环境**：贴出 `git diff` + CI 流水线（gitcode/gitee PR 页面 Jenkins/OBS 结果）链接
> - **无真机**：给出 QEMU/emulator 运行日志、或引用同类改动的历史 CI PASS 记录
> - **无 rk3568**：改用同形态等效产品（如 `hispark_taurus` 顶 rk3568 的 standard 位）并注明差异
> - **单测覆盖不到**：贴出人工 review checklist（对照 Do not 与 Ask before 逐项确认）
> - 所有替代证据须在《最终回复模板》的"未运行的验证项"字段中标注为 `SKIP`，并被 Reviewer 明确接受，否则视为流程违规。

## 修复 → 重验循环（Verification loop policy）

任何一步验证 FAIL 或 SKIP（非 N-A）时，**不得直接进入 Done**，必须按以下环路处理，直到全部通过或明确降级：

1. **分类**：判定失败属于（a）代码缺陷、（b）测试环境缺失、（c）AGENT.md/文档指引错误。
2. **修复**：
   - (a) 修改源码/生成物/HCS/IDL/配置，回到"任务专项检查"重跑相关命令。
   - (b) 在最终回复"未运行的验证项"中记录，并给出等效替代证据（如 CI 结果链接、跨设备日志、代码 review 结论）。
   - (c) 更新本 AGENT.md 或对应知识文档，并作为独立 commit 一起提交。
3. **重验**：对同一验证项**至少重跑一次**，将新的 PASS/FAIL 状态填入《最终回复模板》的验证结果表；FAIL 累计达 3 次仍未通过时，必须升级到对应《风险等级与审批链》审批人（不得静默降级为 SKIP）。
4. **记录**：每一轮循环需在 PR 或最终回复中追加一行"Round N: <项> <FAIL→PASS 或 FAIL→FAIL> · 措施 <...> · 命令 <...>"，用于回溯。
5. **终止条件**：Done 定义的 10 条全部为 PASS 或 N-A（不含 SKIP，除非对应"未运行的验证项"已给出替代证据并被 Reviewer 明确接受）。

> 违反本循环（例如直接把 FAIL 改成 SKIP、跳过重验、修完代码不重跑验证）视为流程违规，Reviewer 有权打回。

## Done 定义

当以下条件全部满足时，任务才算完成：

1. 源码编译通过（零 error、零新增 warning）—— 命令：`./build.sh --product-name rk3568 --build-target "drivers/hdf_core/adapter:uhdf_entry"`。
2. 涉及模块的单测通过（`drivers/hdf_core/adapter:uhdf_test_entry` 或具体 `ohos_unittest` target）；若改动公开接口或反序列化路径，跑 `hdf_framework_fuzztest` / `hdf_platform_fuzztest`。
3. 静态分析（`use_clang_tidy=true` 或手工 clang-tidy）无新增告警（与 `master` 基线 diff 为空）。
4. 多产品覆盖：核心变更至少在 standard/small/mini 各一款参考产品编译通过；若因环境或产品配置缺失未做，在最终回复中说明。
5. API 兼容性确认：未删除或修改已有公开函数签名（含 `framework/include/**` 与 `interfaces/inner_api/**`）；错误码只新增未复用。
6. 生成文件确认：未手改 hdi-gen / idl-gen / hc-gen 生成物；若 IDL/HCS 有改动，已重跑工具并附差异说明。
7. 跨内核/跨进程一致性：涉及 manager/host 或多内核 OSAL 的改动，双侧同步确认；`adapter/uhdf/` 与 `adapter/uhdf2/` 未交叉混改。
8. 安全 / 合规确认：涉及 IPC/权限变更给出 SELinux `.te` 变更清单（`security_selinux_adapter` 仓 PR 链接）；涉及第三方依赖，按 OpenHarmony 规范新增 `README.OpenSource` 并同步 `bundle.json` 依赖。
9. 子目录若存在 `AGENT.md`/`README.md`/`docs/`，已按其增量约束核对（清单见"嵌套指南"）。
10. 提交遵循 DCO：所有 commit 含 `Signed-off-by`（`git commit -s`），subject 采用 `<scope>: <summary>`。

最终回复应包含：
- 修改摘要（路径 + 文件列表 + 风险等级）。
- 验证结果（编译 / 单测 / 平台测试 / fuzz / 静态分析 / 多产品覆盖）。
- API 兼容性结论。
- 生成文件检查结论。
- 跨端一致性结论（Manager-Host、uhdf/uhdf2、khdf 多内核、posix vs khdf OSAL）。
- 安全 / 合规结论（SELinux `.te`、License/NOTICE）。
- 子目录指南遵循情况。
- 未运行的验证项及原因、替代证据。

### 最终回复模板（强制格式）

任务收尾时必须按以下模板输出，字段缺失或状态未填视为未完成：

```
## 修改摘要
- 变更路径: <path1>, <path2>, ...
- 变更文件数: <n>
- 风险等级: <高 | 中 | 低>（依据《风险等级与审批链》）
- 变更说明: <一句话>

## 验证结果
| 项 | 状态 | 证据/命令 |
|---|---|---|
| 编译 uhdf_entry (rk3568) | <PASS/FAIL/SKIP> | `./build.sh --product-name rk3568 --build-target "drivers/hdf_core/adapter:uhdf_entry"` |
| 组件测试 uhdf_test_entry | <PASS/FAIL/SKIP> | `./build.sh --product-name rk3568 --build-target "drivers/hdf_core/adapter:uhdf_test_entry"` |
| 平台单测 (需 hdf_core_platform_test_support=true) | <PASS/FAIL/SKIP/N-A> | `--gn-args="hdf_core_platform_test_support=true"` |
| Fuzz hdf_framework_fuzztest | <PASS/FAIL/SKIP/N-A> | `--build-target "drivers/hdf_core/framework/test/fuzztest:hdf_framework_fuzztest"` |
| Fuzz hdf_platform_fuzztest | <PASS/FAIL/SKIP/N-A> | `--build-target "drivers/hdf_core/framework/support/platform/test/fuzztest:hdf_platform_fuzztest"` |
| 静态分析 clang-tidy | <PASS/FAIL/SKIP> | 与 master 基线 diff: <空 / N 条> |
| 多产品编译 (small) | <PASS/FAIL/SKIP/N-A> | 产品名: <...>；命令: ... |
| 多产品编译 (mini) | <PASS/FAIL/SKIP/N-A> | 产品名: <...>；命令: ... |

## API 兼容性结论
- 头文件 diff: `git diff -- ':(glob)framework/include/**/*.h' ':(glob)interfaces/inner_api/**/*.h'` → <空 / 仅新增函数 / 破坏性变更(已走 Ask before)>
- 错误码: <未改 / 仅新增 / 修改(附评审记录)>

## 生成文件检查结论
- hdi-gen 生成物: <未涉及 / 已重跑并 diff 一致 / 有差异(说明)>
- hc-gen 生成物 (.hcb): <未涉及 / 已重编译 / ...>

## 跨端一致性结论
- Manager-Host 双侧: <一致 / 单侧改动(说明原因)>
- uhdf vs uhdf2: <未交叉 / 交叉(说明)>
- khdf 多内核: <未影响 / 已同步 <内核列表>>
- posix vs khdf OSAL: <未涉及 / 已确认无语义漂移>

## 安全 / 合规结论
- SELinux .te 变更清单: <无 / 指向 security_selinux_adapter 仓 PR#xxx>
- License / NOTICE: <未涉及 / 已按 `README.OpenSource`（OpenHarmony 规范）新增/更新 + 同步 `bundle.json` deps>

## 子目录指南遵循情况
- 影响的嵌套指南: <路径列表 或 无>
- 遵循结论: <全部满足 / 冲突项及处理>

## 未运行的验证项
- <项>: <原因> / 替代证据: <...>

## 修复 → 重验循环记录
- Round 1: <项> <FAIL→PASS 或 FAIL→FAIL> · 分类 <a代码/b环境/c文档> · 措施 <...> · 命令 <...>
- Round N: ...
（无失败时填 "无"）

## DCO
- 所有 commit 已 `Signed-off-by`: <是 / 否>
- commit subject 格式合规: <是 / 否>
```

模板中所有 `<...>` 占位符必须替换为真实内容；状态字段仅接受 `PASS / FAIL / SKIP / N-A` 四种取值。
# OpenHarmony HDF Core 驱动框架指引

## 项目定位

本仓库是 OpenHarmony 驱动子系统的核心源码，包含驱动框架（HDF）、配置管理（HCS）、配置解析、驱动通用框架模型、硬件通用平台能力接口等。核心职责是为不同系统形态（standard/small/mini）提供统一的驱动开发与部署环境，力求做到一次开发，多系统部署。

仓库代码最终编译为 **hdf_devmgr**（设备管理守护进程）、**libhdf_host**（驱动宿主共享库）、**libhdi**（HDI 客户端库）、**libhdf_platform**（平台设备用户态 API）等产物，由系统服务加载使用。

## 快速路由

- `framework/core/`：驱动框架核心实现，采用 Manager-Host-Shared 三层架构；Manager 负责设备生命周期管理，Host 负责驱动加载与设备服务分发，Shared 定义跨进程接口契约。
- `framework/include/`：驱动框架对外提供的 C 语言头文件，覆盖 osal、platform、wifi、audio、bluetooth、net、camera、utils 等。
- `framework/model/`：驱动通用框架模型，按硬件类别组织（audio、display、input、sensor、network、usb、storage、camera、misc）。
- `framework/support/platform/`：平台设备驱动框架中间层，每种外设（I2C、SPI、GPIO 等）均有 core/if/if_u 三层实现。
- `framework/support/posix/`：POSIX OSAL 实现，用于轻量系统或用户态环境。
- `framework/utils/`：基础数据结构（SBUF、Map、SList、DList）和 HCS 解析器。
- `framework/tools/`：开发工具链——hc-gen（HCS 编译器）、hdi-gen（HDI 代码生成器）、idl-gen（IDL 生成器）、hdf_dbg（调试工具）、hdf_dev_eco_tool（驱动生态工具）。
- `adapter/uhdf2/`：标准系统用户态适配层，包含 manager、host、ipc、hdi、platform、osal 等完整适配。
- `adapter/khdf/`：内核态适配层，支持 Linux、LiteOS-A、LiteOS-M、UniProton、HongMeng 等内核。
- `interfaces/inner_api/`：HDF 内部稳定 API，跨模块边界使用。

构建入口是 `adapter/BUILD.gn`（`uhdf_entry` 子组件）。提交使用 `git commit -s`（DCO 签名）。

## 知识索引

稳定背景知识放在 `docs/knowledge/`。改动前先按目录定位，再读取对应文档：

| 场景 | 先读 |
|---|---|
| 顶层目录职责、模块边界、术语约定 | `docs/knowledge/module-map.md` |
| 驱动管理进程 hdf_devmgr 架构、启动流程、设备生命周期、IPC 机制 | `docs/knowledge/hdf_devmgr.md` |
| 驱动孵化进程 hdf_devhost 架构、驱动加载、设备服务发布、电源管理 | `docs/knowledge/hdf_devhost.md` |

## 核心规则

- 驱动框架采用 Manager-Host 多进程模型：`hdf_devmgr` 管理进程负责设备生命周期，每个 hostId 对应一个 `DevHostService` 进程负责驱动加载。新增进程间交互逻辑时先确认归属。
- **hdf_devmgr 进程**的代码分布在 `framework/core/manager/`（框架层）和 `adapter/uhdf2/manager/`（适配层），不要只改一侧。进程入口为 `adapter/uhdf2/manager/device_manager.c`。
- **hdf_devhost 进程**的代码分布在 `framework/core/host/`（框架层）和 `adapter/uhdf2/host/`（适配层），可执行文件入口在 `drivers_peripheral_agent/devhost/host/devhost.c`，链接 `libhdf_host` 共享库。
- `framework/core/shared/` 是接口契约层，不承载业务行为。新增 Manager-Host 交互应先在这里定义接口。
- `adapter/uhdf2/ipc/` 是 C 框架到 OHOS C++ IPC 的桥接，不承载 HDF 业务逻辑。
- 新增驱动入口通过 `HdfDriverEntry` 结构体注册（Bind/Init/Release 三接口），对应 HCS 中 `device_info` 配置节点。
- 平台设备 API 分内核态（`xxx_if.c`，基于 ioctl）和用户态（`xxx_if_u.c`，基于 IPC/SBUF），改动时先确认目标运行环境。
- OSAL 层按内核分目录：Linux 在 `adapter/khdf/linux/osal/`，LiteOS-A 在 `adapter/khdf/liteos/osal/`，LiteOS-M 在 `adapter/khdf/liteos_m/osal/`。不要直接在 `framework/` 里写内核特定代码。
- HCS 配置文件经 `hc-gen` 编译为 `.hcb` 二进制，运行时通过 `device_resource_if.h` 解析。新增配置节点时同步检查 `hc-gen` 是否支持对应语法。
- HDI 接口由 `hdi-gen` 工具从 IDL 描述自动生成 proxy/stub/service/driver 代码，不要手改生成物。
- SBUF 是 HDF 的核心序列化机制，有三种类型：`SBUF_RAW`（用户-内核直通）、`SBUF_IPC`（IPC 通道）、`SBUF_IPC_HW`（硬件 IPC）。新增数据传递场景时先确认 SBUF 类型。
- 驱动模型（model）为每类硬件提供归一化框架，新增外设驱动优先在对应 model 目录下实现，不要直接散落到 core 或 support。

## 编译命令

### 推荐编译策略

根据修改内容选择合适的编译方式：

| 场景 | 编译命令 | 耗时 | 说明 |
|------|----------|------|------|
| **仅修改源码** | `ninja -C ...` | **几秒** | 最快，跳过 GN 和 Python 封装 |
| **修改 GN 配置** | `./build.sh --fast-rebuild` | ~1-2分钟 | 跳过 GN gen，重新解析依赖 |
| **GN 报错/首次编译** | `./build.sh` | ~3-5分钟 | 完整编译，包含 GN gen 阶段 |

### 快速编译（推荐，仅修改源码时使用）

```bash
cd ${OHOS_ROOT}
prebuilts/build-tools/linux-x86/bin/ninja -w dupbuild=warn -C out/rk3568 hdf_core
```

### 标准构建

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 --build-target hdf_core
```

### 快速重编译（仅修改源码）

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 --build-target hdf_core --fast-rebuild
```

### 指定 CPU 架构

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 --build-target hdf_core --target-cpu arm64
```

### 启用某特特性的编译

```bash
cd ${OHOS_ROOT}
./build.sh --product-name rk3568 --build-target hdf_core \
  --gn-args="hdf_core_platform_test_support=true"
```

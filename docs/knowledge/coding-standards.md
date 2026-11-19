# 编码铁律

## 进程与目录归属
- **禁止**在 `framework/` 下写内核特定代码（`<linux/*.h>`、内核专属 API），**采用**对应内核适配目录 `adapter/khdf/<内核>/`（linux/liteos/liteos_m/uniproton/hongmeng）。框架需跨 5 内核 + POSIX 用户态复用，内核依赖会破坏可移植性。
- **禁止**单侧修改 Manager 或 Host。Manager 分布在 `framework/core/manager/` + `adapter/uhdf2/manager/`；Host 分布在 `framework/core/host/` + `adapter/uhdf2/host/`；新增交互**采用**先在 `framework/core/shared/include/` 定义契约再双侧实现（依据 `docs/knowledge/hdf_devmgr.md`、`hdf_devhost.md` 改动规则）。
- **禁止**交叉混改 `adapter/uhdf/`（small）与 `adapter/uhdf2/`（standard），两者代码不共用，各自平行维护。
- **禁止**在 `adapter/uhdf2/ipc/` 写 HDF 业务逻辑，该目录**仅做** C 框架到 OHOS C++ IPC 的桥接。
- **禁止**直接调用 `adapter/platform/` 厂商参考实现作为产品依赖；正式产品由 vendor 自维护。

## 驱动入口与配置
- **禁止**绕过 `HdfDriverEntry`（Bind/Init/Release 三接口）+ `HDF_INIT` 注册驱动。驱动 .so **必须**导出 `driverDesc` 符号（用户态）或落在 `HDF_DRIVER_BEGIN()/END()` 链接器段（内核态）；VDI **必须**导出 `hdfVdiDesc`（`HDF_VDI_INIT`）。
- **禁止**手改 `hdi-gen`/`idl-gen`/`hc-gen` 生成的代码；改动 IDL/HCS 后**采用**重新执行工具再提交。
- **禁止**新增 HCS 语法而不扩展 `hc-gen`；新增配置节点**采用**同步检查 `hc-gen` 是否支持并回归历史 `.hcs` 编译。
- **禁止**在 `.hcs` 中为 `deviceNode` 配置 `policy` 与 `preload` 之外的非法字段；`policy` 取值见 `interfaces/inner_api/host/shared/hdf_device_desc.h:56-70`（SERVICE_POLICY_NONE/PUBLIC/CAPACITY/FRIENDLY/PRIVATE）。

## API 与错误码兼容
- **禁止**删除或修改 `framework/include/**`、`interfaces/inner_api/**` 中已发布函数的签名、参数语义或生命周期；**采用**仅新增函数并确认不与其它模块冲突。改动需走 API 兼容性评审（SIG-Driver Maintainer）。
- **禁止**复用已废弃的 `HDF_STATUS` 码值；新增错误码**采用**落在已划定段（通用 `0/-1~-20`、BSP `-100` 段 `HDF_BSP_ERR_NUM(v)`、设备 `-200` 段 `HDF_DEV_ERR_NUM(v)`，定义于 `interfaces/inner_api/utils/hdf_base.h:42-77`）。
- **禁止**修改 `enum HdfSbufType`（`interfaces/inner_api/utils/hdf_sbuf.h:50-55`）已有成员语义；`SBUF_IPC_HW` 为保留位，启用前须走架构评审；序列化格式变更需给出 wire-format 迁移方案。

## 并发与 IPC
- **禁止**把 devhost AddDevice/DelDevice 改为同步处理；`DevHostServiceStubDispatch` 在 `hostSvcMutex` 下序列化（`adapter/uhdf2/host/src/devhost_service_stub.c:90`），实际处理经 `MessageTask` 异步，避免 IPC 死锁。
- **禁止**在 `DeviceServiceStubDispatch`（`adapter/uhdf2/host/src/device_service_stub.c:37-57`）中绕过 `pthread_rwlock` 访问 `deviceObject.service`；读锁保护 Dispatch、写锁保护 Release 置空。
- **禁止**在 Manager hosts DList 遍历时不持锁；**采用** `g_hostsLock`（`pthread_rwlock_t`，`framework/core/manager/src/devmgr_service.c:28`）读写锁保护。
- **禁止**在 Host 死亡回调中同步清理；**采用** `HdfDeathRecipient` → 异步 `MessageTask` → `CleanupDiedHostResources`（见 `docs/knowledge/hdf_devmgr.md` Host 死亡处理）。

## 驱动模型与平台设备
- **禁止**把外设驱动业务逻辑散落到 `framework/core/` 或 `framework/support/`；**采用**对应 `framework/model/<类别>/` 目录实现。
- **禁止**修改平台设备 API 时不区分运行环境：内核态**改** `xxx_if.c`（直接调 `xxx_core.c`），用户态**改** `xxx_if_u.c`（基于 `HdfIoService` + SBUF），核心抽象**改** `xxx_core.c`。
- **禁止**在平台设备驱动中触发未评审的真实硬件 IO；涉及硬件破坏性操作需硬件评审并给出回滚方案。

## 电源管理
- **禁止**在 PM 逻辑中混淆 `HDF_POWER_SYS_CTRL` 与 `HDF_POWER_DYNAMIC_CTRL` 两种模式；SYS_CTRL 直接推送事件，DYNAMIC_CTRL 用 `HdfSRef` 引用计数 wake lock（`interfaces/inner_api/host/shared/hdf_pm.h:14-18`）。改 PM 逻辑前先确认目标模式。
- **禁止**改变电源遍历方向：唤醒**反向**遍历（后设备先醒），休眠**正向**遍历（先设备先睡）（`framework/core/host/src/devhost_service.c:210-222`）。

## 安全与日志
- **禁止**在日志/DFX 中打印敏感信息（token、key、用户数据、IPC payload 原文）；**采用** `%{public}s` 标注非敏感字段。
- **禁止**新增 IPC 接口或服务注册而不确认 SELinux `.te` 策略兼容性；`.te` 由 `security_selinux_adapter` 仓维护，本仓需同步给出策略变更清单。

## 许可与依赖
- **禁止**在 Apache-2.0 目录（`adapter/uhdf/`、`adapter/uhdf2/`）引入 GPL/LGPL/AGPL 依赖；`framework/` 与 `adapter/platform/` 为 GPLv2 或 BSD-3-Clause 双许可；`adapter/khdf/linux/` 为 GPLv2；`adapter/khdf/liteos*/` 为 BSD-3-Clause。新增第三方依赖需架构 + 法务评审，并按 OpenHarmony 规范新增 `README.OpenSource` + 同步 `bundle.json` 的 `deps.components`/`deps.third_party`。

## 提交规范
- **禁止**提交空 commit 或不带 `Signed-off-by` 的 commit（DCO 强制）；commit subject **采用** `<scope>: <summary>` 格式（scope 用本仓已有前缀如 `hdf_core`、`hdi-gen`、`uhdf2/host`、`khdf/linux`）。
- **禁止**将测试代码或 `framework/sample/` 引用为产品依赖；`framework/sample/` 只读参考，不参与产品编译。

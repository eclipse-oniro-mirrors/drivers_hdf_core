# khdf\_liteosm<a name="ZH-CN_TOPIC_0000001096228646"></a>

-   [简介](#section11660541593)
-   [目录](#section161941989596)
-   [相关仓](#section1371113476307)

## 简介<a name="section11660541593"></a>

该仓主要存放OpenHarmony驱动子系统适配liteos\_m内核的代码和编译脚本，在liteos\_m内核中部署OpenHarmony驱动框架。OSAL基于CMSIS-RTOS2标准实现，由中央分发层 khdf_dispatch.gni 按 ohos_kernel_type 路由，小型内核(liteos\_m/freertos/rtthread)共用本目录的 CMSIS OSAL。

## 目录<a name="section161941989596"></a>

```
/drivers/hdf_core/adapter/khdf/liteos_m
├── core                 #适配liteos_m内核的框架代码
├── hcs                  #hcs配置编译脚本
├── osal                 #基于CMSIS-RTOS2的系统接口(小型内核共用)
└── test                 #内核驱动框架测试代码
```

## 相关仓<a name="section1371113476307"></a>

[驱动子系统](https://gitee.com/openharmony/docs/blob/master/zh-cn/readme/%E9%A9%B1%E5%8A%A8%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

[drivers\_framework](https://gitee.com/openharmony/drivers_framework/blob/master/README_zh.md)


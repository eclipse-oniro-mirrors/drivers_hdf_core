# khdf\_liteosm<a name="EN-US_TOPIC_0000001096228646"></a>

-   [Introduction](#section11660541593)
-   [Directory Structure](#section161941989596)
-   [Repositories Involved](#section1371113476307)

## Introduction<a name="section11660541593"></a>

This repository stores the code and compilation scripts for the OpenHarmony driver subsystem to adapt to the liteos\_m kernel and to deploy the hardware driver foundation \(HDF\). The OSAL is implemented on top of CMSIS-RTOS2 and routed by the central khdf\_dispatch.gni according to ohos\_kernel\_type; small RTOS kernels (liteos\_m/freertos/rtthread) share this directory's CMSIS OSAL.

## Directory Structure<a name="section161941989596"></a>

```
/drivers/hdf_core/adapter/khdf/liteos_m
├── core                 # Driver code for adapting to the liteos_m kernel
├── hcs                  # Script for compiling the HDF configuration source (HCS) file
├── osal                 # CMSIS-RTOS2 based system APIs shared by small RTOS kernels
└── test                 # Test code for the kernel driver framework
```

## Repositories Involved<a name="section1371113476307"></a>

[Driver subsystem](https://gitee.com/openharmony/docs/blob/master/en/readme/driver.md)

[drivers\_framework](https://gitee.com/openharmony/drivers_framework/blob/master/README.md)


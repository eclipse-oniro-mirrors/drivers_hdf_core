#ifndef SENSOR_GAS_DRIVER_H
#define SENSOR_GAS_DRIVER_H

#include "hdf_workqueue.h"
#include "osal_mutex.h"
#include "osal_timer.h"
#include "sensor_config_parser.h"
#include "sensor_platform_if.h"

enum GasIAQPart {
    GAS_PART_IAQ = 0,
    GAS_PART_SUM,
};

enum GasClaPart {
    GAS_PART_GASRES = 0,
    GAS_PART_HEAT = 1,
    GAS_PART_TEMP = 2,
    GAS_PART_HUMI = 3,
    GAS_PART_PRE = 4,
    GAS_DEP_PART_SUM,
};

struct GasData {
    union {
        struct {
            uint32_t gasResitance;
            uint32_t heatSource;
            int16_t temperature;
            uint32_t humidity;
            uint32_t pressure;
        };
        float iaq;
    };
};

struct GasFieldData {
    uint8_t status;         /*! Contains new_data, gasm_valid & heat_stab */
    uint8_t gas_index;      /*! The index of the heater profile used */
    uint8_t meas_index;     /*! Measurement index to track order */
    uint8_t res_heat;       /*! Heater resistance */
    uint8_t idac;           /*! Current DAC */
    uint8_t gas_wait;       /*! Gas wait period */
    int16_t temperature;       /*! Temperature in degree celsius */
    uint32_t pressure;         /*! Pressure in Pascal */
    uint32_t humidity;         /*! Humidity in % relative humidity x1000 */
    uint32_t gas_resistance;   /*! Gas resistance in Ohms */
};

struct gasCfg {
    uint8_t humOs;
    uint8_t tempOs;
    uint8_t presOs;
    uint8_t filter;
    uint8_t odr;
};

struct GasOpsCall {
    int32_t (*Init)(struct SensorCfgData *data);
    int32_t (*ReadData)(struct SensorCfgData *data, struct SensorReportEvent *event);
};

struct GasDrvData {
    struct IDeviceIoService ioService;
    struct HdfDeviceObject *device;
    HdfWorkQueue gasWorkQueue;
    HdfWork gasWork;
    OsalTimer gasTimer;
    bool detectFlag;
    bool enable;
    int64_t interval;
    struct SensorCfgData *gasCfg;
    struct GasOpsCall ops;
};

int32_t GasRegisterChipOps(const struct GasOpsCall *ops);
struct SensorCfgData *GasCreateCfgData(const struct DeviceResourceNode *node);
void GasReleaseCfgData(struct SensorCfgData *gasCfg);

#endif /* SENSOR_GAS_DRIVER_H*/
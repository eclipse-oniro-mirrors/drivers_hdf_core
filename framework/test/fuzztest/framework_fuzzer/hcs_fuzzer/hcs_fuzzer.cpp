/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hcs_fuzzer.h"

#include "hcs_tree_if.h"
#include "hdf_base.h"
#include "hdf_log.h"

namespace OHOS {
constexpr size_t THRESHOLD = 10;
#define HDF_LOG_TAG hcs_fuzzer_fuzzer

enum FuncId {
    HCS_HCSGETBOOL,
    HCS_HCSGETUINT8,
    HCS_HCSGETUINT32,
    HCS_HCSGETUINT64,
    HCS_HCSGETUINT8ARRAYELEM,
    HCS_HCSGETUINT16ARRAYELEM,
    HCS_HCSGETUINT32ARRAYELEM,
    HCS_HCSGETUINT64ARRAYELEM,
    HCS_HCSGETUINT8ARRAY,
    HCS_HCSGETUINT16ARRAY,
    HCS_HCSGETUINT32ARRAY,
    HCS_HCSGETUINT64ARRAY,
    HCS_HCSGETELEMNUM,
    HCS_HCSGETNODEBYMATCHATTR,
    HCS_HCSGETCHILDNODE,
    HCS_HCSGETNODEBYREFATTR,
    HCS_END
};

void FuncHcsGetBool(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    HcsGetBool(node, attrName);
    return;
}

void FuncHcsGetUint8(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint8_t value = 0;
    uint8_t def = 0;

    HcsGetUint8(node, attrName, &value, def);
    return;
}

void FuncHcsGetUint32(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint32_t value = 0;
    uint32_t def = 0;

    HcsGetUint32(node, attrName, &value, def);
    return;
}

void FuncHcsGetUint64(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint64_t value = 0;
    uint64_t def = 0;

    HcsGetUint64(node, attrName, &value, def);
    return;
}

void FuncHcsGetUint8ArrayElem(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint32_t index = 0;
    uint8_t value = 0;
    uint8_t def = 0;

    HcsGetUint8ArrayElem(node, attrName, index, &value, def);
    return;
}

void FuncHcsGetUint16ArrayElem(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint32_t index = 0;
    uint16_t value = 0;
    uint16_t def = 0;

    HcsGetUint16ArrayElem(node, attrName, index, &value, def);
    return;
}

void FuncHcsGetUint32ArrayElem(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint32_t index = 0;
    uint32_t value = 0;
    uint32_t def = 0;

    HcsGetUint32ArrayElem(node, attrName, index, &value, def);
    return;
}

void FuncHcsGetUint64ArrayElem(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint32_t index = 0;
    uint64_t value = 0;
    uint64_t def = 0;

    HcsGetUint64ArrayElem(node, attrName, index, &value, def);
    return;
}

void FuncHcsGetUint8Array(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint8_t *value = (uint8_t *)(data + sizeof(struct DeviceResourceNode));
    uint32_t len = (size - sizeof(struct DeviceResourceNode)) / sizeof(uint8_t);
    uint8_t def = 0;

    HcsGetUint8Array(node, attrName, value, len, def);
    return;
}

void FuncHcsGetUint16Array(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint16_t *value = (uint16_t *)(data + sizeof(struct DeviceResourceNode));
    uint32_t len = (size - sizeof(struct DeviceResourceNode)) / sizeof(uint16_t);
    uint16_t def = 0;

    HcsGetUint16Array(node, attrName, value, len, def);
    return;
}

void FuncHcsGetUint32Array(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint32_t *value = (uint32_t *)(data + sizeof(struct DeviceResourceNode));
    uint32_t len = (size - sizeof(struct DeviceResourceNode)) / sizeof(uint32_t);
    uint32_t def = 0;

    HcsGetUint32Array(node, attrName, value, len, def);
    return;
}

void FuncHcsGetUint64Array(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";
    uint64_t *value = (uint64_t *)(data + sizeof(struct DeviceResourceNode));
    uint32_t len = (size - sizeof(struct DeviceResourceNode)) / sizeof(uint64_t);
    uint64_t def = 0;

    HcsGetUint64Array(node, attrName, value, len, def);
    return;
}

void FuncHcsGetElemNum(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";

    HcsGetElemNum(node, attrName);
    return;
}

void FuncHcsGetNodeByMatchAttr(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrValue = "nothing";

    HcsGetNodeByMatchAttr(node, attrValue);
    return;
}

void FuncHcsGetChildNode(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *nodeName = "nothing";

    HcsGetChildNode(node, nodeName);
    return;
}

void FuncHcsGetNodeByRefAttr(const uint8_t *data, size_t size)
{
    if (size < sizeof(struct DeviceResourceNode)) {
        return;
    }

    struct DeviceResourceNode *node = (struct DeviceResourceNode *)data;
    const char *attrName = "nothing";

    HcsGetNodeByRefAttr(node, attrName);
    return;
}

void FuncSwitch(uint32_t cmd, const uint8_t *data, size_t size)
{
    switch (cmd) {
        case HCS_HCSGETBOOL: {
            FuncHcsGetBool(data, size);
            break;
        }
        case HCS_HCSGETUINT8: {
            FuncHcsGetUint8(data, size);
            break;
        }
        case HCS_HCSGETUINT32: {
            FuncHcsGetUint32(data, size);
            break;
        }
        case HCS_HCSGETUINT64: {
            FuncHcsGetUint64(data, size);
            break;
        }
        case HCS_HCSGETUINT8ARRAYELEM: {
            FuncHcsGetUint8ArrayElem(data, size);
            break;
        }
        case HCS_HCSGETUINT16ARRAYELEM: {
            FuncHcsGetUint16ArrayElem(data, size);
            break;
        }
        case HCS_HCSGETUINT32ARRAYELEM: {
            FuncHcsGetUint32ArrayElem(data, size);
            break;
        }
        case HCS_HCSGETUINT64ARRAYELEM: {
            FuncHcsGetUint64ArrayElem(data, size);
            break;
        }
        case HCS_HCSGETUINT8ARRAY: {
            FuncHcsGetUint8Array(data, size);
            break;
        }
        case HCS_HCSGETUINT16ARRAY: {
            FuncHcsGetUint16Array(data, size);
            break;
        }
        case HCS_HCSGETUINT32ARRAY: {
            FuncHcsGetUint32Array(data, size);
            break;
        }
        case HCS_HCSGETUINT64ARRAY: {
            FuncHcsGetUint64Array(data, size);
            break;
        }
        case HCS_HCSGETELEMNUM: {
            FuncHcsGetElemNum(data, size);
            break;
        }
        case HCS_HCSGETNODEBYMATCHATTR: {
            FuncHcsGetNodeByMatchAttr(data, size);
            break;
        }
        case HCS_HCSGETCHILDNODE: {
            FuncHcsGetChildNode(data, size);
            break;
        }
        case HCS_HCSGETNODEBYREFATTR: {
            FuncHcsGetNodeByRefAttr(data, size);
            break;
        }
        default:
            return;
    }
}

void TraverseAllFunc(const uint8_t *data, size_t size)
{
    for (uint32_t cmd = 0; cmd < HCS_END; cmd++) {
        FuncSwitch(cmd, data, size);
    }
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return HDF_SUCCESS;
    }

    OHOS::TraverseAllFunc(data, size);
    return HDF_SUCCESS;
}

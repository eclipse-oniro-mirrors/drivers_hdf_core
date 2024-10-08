/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup DriverUtils
 * @{
 *
 * @brief Defines common macros and interfaces of the driver module.
 *
 * This module provides interfaces such as log printing, doubly linked list operations, and work queues.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file hdf_core_log.h
 *
 * @brief Declares log printing functions of the driver module.
 * This module provides functions for printing logs at the verbose, debug, information, warning, and error levels.
 *
 * To use these functions, you must define <b>HDF_LOG_TAG</b>, for example, #define HDF_LOG_TAG evt.
 *
 * @since 1.0
 * @version 1.0
 */
#include "hdf_log.h"
/**
 * @brief Defines the log domain of the HDF.
 */
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002510
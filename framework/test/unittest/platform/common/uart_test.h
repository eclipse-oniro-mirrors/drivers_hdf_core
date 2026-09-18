/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef UART_TEST_H
#define UART_TEST_H

#include "uart_if.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32_t UartTestExecute(int cmd);

/*
 * Test topologies for VMIN/VTIME, trans-mode and batch-read cases:
 *
 *  A (board dual-port cross): port TX/RX cross-wired to peer_port TX/RX.
 *     Configure both "port" and "peer_port" in uart_test_config.hcs.
 *  B (PC echo, default for single board): port TX/RX cross-wired to a
 *     USB-TTL adapter attached to a PC, PC runs uart_peer.py in echo mode.
 *     Configure "loopback = 1".
 *  C (single-port jumper): port TX/RX pinned together with a jumper cap.
 *     Configure "loopback = 1" as with topology B.
 *  D (open port, no wiring): control-plane cases only; data-plane cases
 *     are skipped automatically.
 *
 * Optional hcs fields (absent field keeps the default):
 *   peer_port : uint32, peer uart port number, default 0xffffffff (none).
 *   loopback  : uint32, 1 when the port data can be read back through an
 *               external path (PC echo or jumper), default 0.
 */

enum UartTestCmd {
    UART_TEST_CMD_WRITE = 0,
    UART_TEST_CMD_READ = 1,
    UART_TEST_CMD_SET_BAUD = 2,
    UART_TEST_CMD_GET_BAUD = 3,
    UART_TEST_CMD_SET_ATTRIBUTE = 4,
    UART_TEST_CMD_GET_ATTRIBUTE = 5,
    UART_TEST_CMD_SET_TRANSMODE = 6,
    UART_TEST_CMD_RELIABILITY = 7,
    UART_TEST_CMD_PERFORMANCE = 8,
    UART_MINI_BLOCK_WRITE_TEST = 9,
    UART_TEST_CMD_VMIN_VTIME_ATTR = 10,
    UART_TEST_CMD_TRANS_MODE_SWITCH = 11,
    UART_TEST_CMD_SELF_LOOP_DATA = 12,
    UART_TEST_CMD_VMIN_WAKE = 13,
    UART_TEST_CMD_VTIME_INTERBYTE = 14,
    UART_TEST_CMD_BATCH_READ_PERF = 15,
    UART_TEST_CMD_DUAL_PORT_DATA = 16,
    UART_TEST_CMD_MAX = 17,
};

#define UART_TEST_PEER_PORT_NONE 0xffffffff
#define UART_TEST_LOOPBACK_NONE 0

struct UartTestConfig {
    uint32_t port;
    uint32_t len;
    uint8_t *wbuf;
    uint8_t *rbuf;
    uint32_t peerPort;
    uint32_t loopback;
};

struct UartTester {
    struct UartTestConfig config;
    DevHandle handle;
    uint16_t total;
    uint16_t fails;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UART_TEST_H */

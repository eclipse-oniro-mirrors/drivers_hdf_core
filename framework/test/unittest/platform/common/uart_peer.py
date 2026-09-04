#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2026 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

"""
PC-side peer tool for UART platform tests (topology B).

Wiring: UART TX/RX of the board cross-connected to RX/TX of a USB-TTL
adapter attached to this PC, GND common. Board side runs the
hdf_adapter_uhdf_test_platform uart cases with "loopback = 1" configured
in uart_test_config.hcs (the PC replaces the jumper cap of topology C).

Modes:
  echo   Relay everything received back to the sender. Used as a software
         loopback so the board-side data cases (SELF_LOOP_DATA / VMIN_WAKE /
         VTIME_INTERBYTE / BATCH_READ_PERF) run unchanged.
  source Stream an LCG test pattern at a fixed chunk/interval. Used to check
         sustained input and CPU cost of the batch-read optimization.

Examples:
  python3 uart_peer.py --port /dev/ttyUSB0 --baud 115200 --mode echo
  python3 uart_peer.py --port /dev/ttyUSB0 --baud 921600 --mode source \
      --total 1048576 --chunk 4096 --interval-ms 20
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is required: pip3 install pyserial")


def lcg_pattern(total, seed=0x12345678):
    state = seed
    out = bytearray()
    for _ in range(total):
        state = (state * 1103515245 + 12345) & 0xFFFFFFFF
        out.append((state >> 16) & 0xFF)
    return bytes(out)


def do_echo(args):
    relayed = 0
    with serial.Serial(args.port, args.baud, bytesize=8,
                       parity=serial.PARITY_NONE, stopbits=1, timeout=0.1) as ser:
        print("echo: {} @{} open, ctrl-c to stop".format(args.port, args.baud))
        while True:
            data = ser.read(4096)
            if data:
                ser.write(data)
                ser.flush()
                relayed += len(data)
                print("\rrelayed {} bytes".format(relayed), end="", flush=True)


def do_source(args):
    pattern = lcg_pattern(args.total)
    with serial.Serial(args.port, args.baud, bytesize=8,
                       parity=serial.PARITY_NONE, stopbits=1, timeout=1) as ser:
        print("source: {} @{} sending {} bytes".format(args.port, args.baud, args.total))
        start = time.monotonic()
        sent = 0
        for off in range(0, args.total, args.chunk):
            piece = pattern[off:off + args.chunk]
            ser.write(piece)
            ser.flush()
            sent += len(piece)
            if args.interval_ms:
                time.sleep(args.interval_ms / 1000.0)
        elapsed = time.monotonic() - start
        print("done: {} bytes in {:.3f}s ({:.0f} B/s)".format(sent, elapsed, sent / elapsed))


def main():
    parser = argparse.ArgumentParser(description="PC peer tool for UART platform tests")
    parser.add_argument("--port", required=True, help="serial device, e.g. /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200, help="baud rate, default 115200")
    parser.add_argument("--mode", choices=["echo", "source"], default="echo")
    parser.add_argument("--total", type=int, default=1048576, help="source mode: total bytes")
    parser.add_argument("--chunk", type=int, default=4096, help="source mode: chunk size")
    parser.add_argument("--interval-ms", type=int, default=20, help="source mode: per-chunk delay")
    args = parser.parse_args()

    if args.mode == "echo":
        do_echo(args)
    else:
        do_source(args)


if __name__ == "__main__":
    main()

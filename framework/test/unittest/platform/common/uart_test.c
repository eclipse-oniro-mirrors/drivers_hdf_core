/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "uart_test.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_core_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "securec.h"
#include "uart_if.h"

#define HDF_LOG_TAG uart_test

static int32_t UartTestGetConfig(struct UartTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    service = HdfIoServiceBind("UART_TEST");
    if (service == NULL) {
        HDF_LOGE("UartTestGetConfig: fail to bind service!");
        return HDF_ERR_NOT_SUPPORT;
    }

    do {
        reply = HdfSbufObtainDefaultSize();
        if (reply == NULL) {
            HDF_LOGE("UartTestGetConfig: fail to obtain reply!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("UartTestGetConfig: remote dispatch fail, ret: %d!", ret);
            break;
        }

        if (!HdfSbufReadUint32(reply, &config->port)) {
            HDF_LOGE("UartTestGetConfig: read port fail!");
            ret = HDF_ERR_IO;
            break;
        }
        if (!HdfSbufReadUint32(reply, &config->len)) {
            HDF_LOGE("UartTestGetConfig: read len fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (!HdfSbufReadBuffer(reply, (const void **)&buf, &len)) {
            HDF_LOGE("UartTestGetConfig: read buf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (len != config->len) {
            HDF_LOGE("UartTestGetConfig: config len:%u, read size:%u!", config->len, len);
            ret = HDF_ERR_IO;
            break;
        }
        config->wbuf = NULL;
        config->wbuf = (uint8_t *)OsalMemCalloc(len);
        if (config->wbuf == NULL) {
            HDF_LOGE("UartTestGetConfig: malloc wbuf fail!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }
        config->rbuf = NULL;
        config->rbuf = (uint8_t *)OsalMemCalloc(len);
        if (config->rbuf == NULL) {
            HDF_LOGE("UartTestGetConfig: malloc rbuf fail!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        if (memcpy_s(config->wbuf, config->len, buf, len) != EOK) {
            HDF_LOGE("UartTestGetConfig: Memcpy wbuf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        /* optional fields, absent on legacy test drivers */
        if (!HdfSbufReadUint32(reply, &config->peerPort)) {
            config->peerPort = UART_TEST_PEER_PORT_NONE;
        }
        if (!HdfSbufReadUint32(reply, &config->loopback)) {
            config->loopback = 0;
        }

        HDF_LOGD("UartTestGetConfig: done!");
        ret = HDF_SUCCESS;
    } while (0);
    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return ret;
}

static inline void UartBufFree(struct UartTestConfig *config)
{
    OsalMemFree(config->wbuf);
    config->wbuf = NULL;
    OsalMemFree(config->rbuf);
    config->rbuf = NULL;
}

/*
 * Control-character dense pattern. The data-plane cases compare byte-exact
 * round trips of this pattern, which actively verifies the raw-mode flags
 * cleared in UartAdapterInit: any of OPOST/ONLCR (0x0a), ICRNL/IGNCR/INLCR
 * (0x0d/0x0a), IEXTEN (0x16), ISIG (0x03), ISTRIP or CSIZE (high-bit bytes),
 * ECHO (no stream amplification) or IXON (late 0x13 with no trailing 0x11
 * would stall later writes) left enabled would alter or stall the stream
 * and fail the compare.
 */
static const uint8_t g_uartTestPattern[] = {
    0x00, 0x03, 0x08, 0x0a, 0x0d, 0x11, 0x16, 0x1a,
    0x7f, 0x15, 0x04, 0x17, 0x12, 0x18, 0x0b, 0x0c,
    0x55, 0xc3, 0x7a, 0xe1, 0x96, 0xa8, 0x3f, 0xd4,
    0x62, 0x9e, 0x41, 0xb7, 0x28, 0xee, 0x73, 0xfa,
    0x39, 0xc0, 0x85, 0x1d, 0xa6, 0x4b, 0xd8, 0x21,
    0x97, 0x63, 0xe4, 0x0e, 0xb2, 0x58, 0xcc, 0x6d,
    0xf1, 0x32, 0x89, 0x44, 0xab, 0x76, 0xde, 0x19,
    0x53, 0x2c, 0x13, 0x2a, 0x9b, 0x06, 0xc7, 0x5e,
};

static struct UartTester *UartTesterGet(void)
{
    int32_t ret;
    static struct UartTester tester;

    ret = UartTestGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartTesterGet: read config fail, ret: %d!", ret);
        UartBufFree(&tester.config);
        return NULL;
    }
    /* override the hcs wbuf with the raw-mode-probing pattern */
    ret = memcpy_s(tester.config.wbuf, tester.config.len, g_uartTestPattern,
        (tester.config.len < sizeof(g_uartTestPattern)) ? tester.config.len : sizeof(g_uartTestPattern));
    if (ret != EOK) {
        HDF_LOGE("UartTesterGet: fill test pattern fail, ret: %d!", ret);
        UartBufFree(&tester.config);
        return NULL;
    }
    tester.handle = UartOpen(tester.config.port);
    if (tester.handle == NULL) {
        HDF_LOGE("UartTesterGet: open uart port:%u fail!", tester.config.port);
        UartBufFree(&tester.config);
        return NULL;
    }
    return &tester;
}

static void UartTesterPut(struct UartTester *tester)
{
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("UartTesterPut: tester or uart handle is null!");
        return;
    }
    UartBufFree(&tester->config);
    UartClose(tester->handle);
    tester->handle = NULL;
}

static int32_t UartWriteTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartWrite(tester->handle, tester->config.wbuf, tester->config.len);
    HDF_LOGD("UartWriteTest: len is %d", tester->config.len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartWriteTest: write fail!");
        return HDF_FAILURE;
    }
    HDF_LOGD("UartWriteTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartReadTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartReadTest: transmode error, ret: %d!", ret);
        return ret;
    }
    ret = UartRead(tester->handle, tester->config.rbuf, tester->config.len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartReadTest: read fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartReadTest: success!");
    return HDF_SUCCESS;
}

#define BAUD_921600 921600
static int32_t UartSetBaudTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartSetBaud(tester->handle, BAUD_921600);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetBaudTest: set baud fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartSetBaudTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartGetBaudTest(struct UartTester *tester)
{
    int32_t ret;
    uint32_t baud;

    ret = UartGetBaud(tester->handle, &baud);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartGetBaudTest: get baud fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartGetBaudTest: baud %u success!", baud);
    return HDF_SUCCESS;
}

static int32_t UartSetAttributeTest(struct UartTester *tester)
{
    struct UartAttribute attribute;
    int32_t ret;

    attribute.dataBits = UART_ATTR_DATABIT_7;
    attribute.parity = UART_ATTR_PARITY_NONE;
    attribute.stopBits = UART_ATTR_STOPBIT_1;
    attribute.rts = UART_ATTR_RTS_DIS;
    attribute.cts = UART_ATTR_CTS_DIS;
    attribute.fifoRxEn = UART_ATTR_RX_FIFO_EN;
    attribute.fifoTxEn = UART_ATTR_TX_FIFO_EN;
    ret = UartSetAttribute(tester->handle, &attribute);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetAttributeTest: set attribute fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartSetAttributeTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartGetAttributeTest(struct UartTester *tester)
{
    struct UartAttribute attribute;
    int32_t ret;

    ret = UartGetAttribute(tester->handle, &attribute);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartGetAttributeTest: get attribute fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartGetAttributeTest: dataBits %u", attribute.dataBits);
    HDF_LOGD("UartGetAttributeTest: parity %u", attribute.parity);
    HDF_LOGD("UartGetAttributeTest: stopBits %u", attribute.stopBits);
    HDF_LOGD("UartGetAttributeTest: rts %u", attribute.rts);
    HDF_LOGD("UartGetAttributeTest: cts %u", attribute.cts);
    HDF_LOGD("UartGetAttributeTest: fifoRxEn %u", attribute.fifoRxEn);
    HDF_LOGD("UartGetAttributeTest: fifoTxEn %u", attribute.fifoTxEn);
    HDF_LOGD("UartGetAttributeTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartSetTransModeTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetTransModeTest: set transmode fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartSetTransModeTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartReliabilityTest(struct UartTester *tester)
{
    uint32_t baud;
    struct UartAttribute attribute = {0};

    (void)UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK);
    (void)UartSetTransMode(tester->handle, -1);
    (void)UartWrite(tester->handle, tester->config.wbuf, tester->config.len);
    (void)UartWrite(tester->handle, NULL, -1);
    (void)UartRead(tester->handle, tester->config.rbuf, tester->config.len);
    (void)UartRead(tester->handle, NULL, -1);
    (void)UartSetBaud(tester->handle, BAUD_921600);
    (void)UartSetBaud(tester->handle, -1);
    (void)UartGetBaud(tester->handle, &baud);
    (void)UartGetBaud(tester->handle, NULL);
    (void)UartSetAttribute(tester->handle, &attribute);
    (void)UartSetAttribute(tester->handle, NULL);
    (void)UartGetAttribute(tester->handle, &attribute);
    (void)UartGetAttribute(tester->handle, NULL);
    HDF_LOGD("UartReliabilityTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartIfPerformanceTest(struct UartTester *tester)
{
#ifdef __LITEOS__
    if (tester == NULL) {
        HDF_LOGE("UartIfPerformanceTest: tester is null!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
#endif
    uint32_t baudRate;
    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime;    // ms

    if (tester == NULL) {
        HDF_LOGE("UartIfPerformanceTest: tester is null!");
        return HDF_FAILURE;
    }
    startMs = OsalGetSysTimeMs();
    UartGetBaud(tester->handle, &baudRate);
    endMs = OsalGetSysTimeMs();

    useTime = endMs - startMs;
    HDF_LOGI("UartIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%s]\r\n",
        useTime < 1 ? "yes" : "no");
    return HDF_SUCCESS;
}

static int32_t UartMiniBlockWriteTest(struct UartTester *tester)
{
#ifdef __KERNEL__
    uint8_t data;
    int32_t ret;

    ret = UartBlockWrite(tester->handle, &data, sizeof(data));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartMiniBlockWriteTest: uart block write fail, ret: %d!", ret);
        return ret;
    }
#else
    (void)tester;
#endif
    HDF_LOGI("UartMiniBlockWriteTest: all test done!");
    return HDF_SUCCESS;
}

struct UartTestEntry {
    int cmd;
    int32_t (*func)(struct UartTester *tester);
    const char *name;
};

/* vtime unit is decisecond: 10 means 1s */
#define UART_TEST_VTIME_DS             10
#define UART_TEST_VTIME_MIN_MS         700
#define UART_TEST_VTIME_MAX_MS         2000
#define UART_TEST_INTERBYTE_SLEEP_MS   1500
#define UART_TEST_IMMEDIATE_MS         200
#define UART_TEST_WAKE_MAX_MS          1000
#define UART_TEST_VMIN_PARTIAL         16
#define UART_TEST_VMIN_HALF            2
#define UART_TEST_READY_LEN            16
#define UART_TEST_ECHO_SETTLE_MS       1000
#define UART_TEST_DRAIN_QUIET_ROUNDS  15
#define UART_TEST_DRAIN_POLL_MS       20
#define UART_TEST_DRAIN_SLEEP_MS      10
#define UART_TEST_LOOP_PROBE_MS       500
#define UART_TEST_LOOP_PROBE_BYTE     0xA5
#define UART_TEST_RECV_TIMEOUT_MS      2000
#define UART_TEST_BATCH_LOOPS          8
#define UART_TEST_BATCH_CHUNK          64
#define UART_TEST_BAUD_115200          115200
#define UART_TEST_VMIN_PROBE           5
#define UART_TEST_VTIME_PROBE          10
#define UART_TEST_VMIN_MAX             255
#define UART_TEST_VTIME_MAX            255

static int UartTestBufMatch(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

/*
 * Wait for a genuinely quiet line. Data written by earlier cases (e.g. the
 * legacy reliability case) keeps echoing back for hundreds of milliseconds
 * when a PC-side echo peer is attached (the peer relays in bursts), so a few
 * back-to-back empty reads are not enough: require UART_TEST_DRAIN_QUIET_ROUNDS
 * polls of UART_TEST_DRAIN_POLL_MS with no data at all (300ms of silence).
 */
static void UartTestDrainInput(DevHandle handle, uint8_t *buf, uint32_t size)
{
    uint32_t quiet = 0;

    (void)UartSetTransMode(handle, UART_MODE_RD_NONBLOCK);
    while (quiet < UART_TEST_DRAIN_QUIET_ROUNDS) {
        if (UartRead(handle, buf, size) > 0) {
            quiet = 0;
        } else {
            quiet++;
        }
        OsalMSleep(UART_TEST_DRAIN_POLL_MS);
    }
}

static int UartTestVminVtimeSupported(DevHandle handle)
{
    struct UartAttribute set = {0};
    struct UartAttribute get = {0};

    set.vmin = UART_TEST_VMIN_PROBE;
    set.vtime = UART_TEST_VTIME_PROBE;
    if (UartSetAttribute(handle, &set) != HDF_SUCCESS) {
        return 0;
    }
    if (UartGetAttribute(handle, &get) != HDF_SUCCESS) {
        return 0;
    }
    return (get.vmin == set.vmin) && (get.vtime == set.vtime);
}

/*
 * Probe whether the configured loopback path really returns data: send one
 * byte in non-block mode and wait briefly for its echo. When the peer
 * (PC echo or jumper) is absent, the data-plane cases skip instead of
 * blocking forever: a vmin > 0 read without data never times out by
 * termios design, and killing the process mid-read would leak the kernel
 * reference count (reboot required).
 */
static int UartTestLoopbackAvailable(struct UartTester *tester)
{
    struct UartAttribute attr = {0};
    uint8_t probe = UART_TEST_LOOP_PROBE_BYTE;
    uint8_t buf[4];
    uint64_t startMs;

    /* pin the baud first: earlier cases (e.g. reliability) may leave 921600,
     * which never matches the fixed-115200 peer */
    if (UartSetBaud(tester->handle, UART_TEST_BAUD_115200) != HDF_SUCCESS) {
        HDF_LOGE("UartTestLoopbackAvailable: set baud fail!");
        return 0;
    }
    if (UartSetAttribute(tester->handle, &attr) != HDF_SUCCESS ||
        UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK) != HDF_SUCCESS) {
        HDF_LOGE("UartTestLoopbackAvailable: set nonblock fail!");
        return 0;
    }
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);
    if (UartWrite(tester->handle, &probe, sizeof(probe)) != HDF_SUCCESS) {
        HDF_LOGE("UartTestLoopbackAvailable: write probe fail!");
        return 0;
    }
    startMs = OsalGetSysTimeMs();
    while (OsalGetSysTimeMs() - startMs < UART_TEST_LOOP_PROBE_MS) {
        if (UartRead(tester->handle, buf, sizeof(buf)) > 0) {
            UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);
            return 1;
        }
        OsalMSleep(UART_TEST_DRAIN_POLL_MS);
    }
    return 0;
}

/* apply vmin/vtime attribute and trans mode for a read step */
static int32_t UartTestApplyReadCfg(DevHandle handle, uint8_t vmin, uint8_t vtime, enum UartTransMode mode)
{
    struct UartAttribute attr = {0};

    attr.vmin = vmin;
    attr.vtime = vtime;
    if (UartSetAttribute(handle, &attr) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    return UartSetTransMode(handle, mode);
}

/*
 * Timed idle read for the trans-mode steps: asserts the read returns zero
 * bytes within [minMs, maxMs].
 */
static int32_t UartTestTimedIdleRead(DevHandle handle, uint8_t *buf, uint32_t size,
    uint64_t minMs, uint64_t maxMs)
{
    uint64_t startMs;
    uint64_t useMs;
    int32_t nRead;

    startMs = OsalGetSysTimeMs();
    nRead = UartRead(handle, buf, size);
    useMs = OsalGetSysTimeMs() - startMs;
    if (nRead != 0 || useMs < minMs || useMs > maxMs) {
        HDF_LOGE("UartTestTimedIdleRead: nRead:%d useMs:%llu!", nRead, (unsigned long long)useMs);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

/* set vmin/vtime then read back; 1 when the round trip matches */
static int UartTestAttrRoundtrip(DevHandle handle, uint8_t vmin, uint8_t vtime)
{
    struct UartAttribute set = {0};
    struct UartAttribute get = {0};

    set.vmin = vmin;
    set.vtime = vtime;
    if (UartSetAttribute(handle, &set) != HDF_SUCCESS) {
        return 0;
    }
    if (UartGetAttribute(handle, &get) != HDF_SUCCESS) {
        return 0;
    }
    return (get.vmin == vmin) && (get.vtime == vtime);
}

/* dma trans-mode enums are rejected by this adapter */
static int UartTestCheckDmaModes(DevHandle handle)
{
    int32_t ret;

    ret = UartSetTransMode(handle, UART_MODE_DMA_RX_EN);
    ret |= UartSetTransMode(handle, UART_MODE_DMA_RX_DIS);
    ret |= UartSetTransMode(handle, UART_MODE_DMA_TX_EN);
    ret |= UartSetTransMode(handle, UART_MODE_DMA_TX_DIS);
    return (ret == HDF_ERR_NOT_SUPPORT);
}

/* out-of-range trans modes must be rejected */
static int UartTestCheckBadMode(DevHandle handle)
{
    return UartSetTransMode(handle, (enum UartTransMode)(UART_MODE_DMA_TX_DIS + 1)) ==
        HDF_ERR_NOT_SUPPORT;
}

static int32_t UartTestWriteAndRecv(struct UartTester *tester, DevHandle writer, DevHandle reader,
    uint32_t len, uint32_t *recvLen)
{
    uint32_t received = 0;
    int32_t nRead;
    uint64_t startMs = OsalGetSysTimeMs();

    if (recvLen != NULL) {
        *recvLen = 0;
    }
    if (UartWrite(writer, tester->config.wbuf, len) != HDF_SUCCESS) {
        HDF_LOGE("UartTestWriteAndRecv: write fail!");
        return HDF_FAILURE;
    }
    while (received < len) {
        if (OsalGetSysTimeMs() - startMs > UART_TEST_RECV_TIMEOUT_MS) {
            HDF_LOGE("UartTestWriteAndRecv: recv timeout, %u/%u!", received, len);
            return HDF_FAILURE;
        }
        nRead = UartRead(reader, tester->config.rbuf + received, len - received);
        if (nRead > 0) {
            if (!UartTestBufMatch(tester->config.rbuf + received, tester->config.wbuf + received, nRead)) {
                HDF_LOGE("UartTestWriteAndRecv: data mismatch at %u!", received);
                return HDF_FAILURE;
            }
            received += nRead;
        } else {
            OsalMSleep(UART_TEST_DRAIN_SLEEP_MS);
        }
    }
    if (recvLen != NULL) {
        *recvLen = received;
    }
    return HDF_SUCCESS;
}

/*
 * Common prechecks of the data-plane cases: 1 = ready, 0 = skip (environment
 * or capability not present), -1 = failure.
 */
static int32_t UartTestDataPlaneReady(struct UartTester *tester, const char *tag)
{
    if (tester->config.loopback == UART_TEST_LOOPBACK_NONE) {
        HDF_LOGW("%s: loopback not configured, skip!", tag);
        return 0;
    }
    if (!UartTestLoopbackAvailable(tester)) {
        HDF_LOGW("%s: loopback path not available, skip!", tag);
        return 0;
    }
    if (!UartTestVminVtimeSupported(tester->handle)) {
        HDF_LOGW("%s: vmin/vtime not supported, skip!", tag);
        return 0;
    }
    if (UartSetBaud(tester->handle, UART_TEST_BAUD_115200) != HDF_SUCCESS) {
        HDF_LOGE("%s: set baud fail!", tag);
        return -1;
    }
    return 1;
}


/* read configuration shared by the matrix step helpers */
struct UartTestReadCfg {
    uint8_t vmin;
    uint8_t vtime;
    enum UartTransMode mode;
};

/*
 * Positive (data-ready) direction of the mode x vmin/vtime matrix: write len
 * bytes, wait for the echo to settle into the rx buffer, then a single read
 * must return all of them immediately, whatever vmin/vtime/mode is set.
 */
static int32_t UartTestReadyDataRead(struct UartTester *tester, const struct UartTestReadCfg *cfg,
    uint32_t len, const char *tag)
{
    uint64_t startMs;
    uint64_t useMs;
    int32_t nRead;

    HDF_LOGI("%s", tag);
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);
    if (UartWrite(tester->handle, tester->config.wbuf, len) != HDF_SUCCESS) {
        HDF_LOGE("UartTestReadyDataRead: write fail!");
        return HDF_FAILURE;
    }
    OsalMSleep(UART_TEST_ECHO_SETTLE_MS); /* let the echo land in the rx buffer first */
    if (UartTestApplyReadCfg(tester->handle, cfg->vmin, cfg->vtime, cfg->mode) != HDF_SUCCESS) {
        HDF_LOGE("UartTestReadyDataRead: apply cfg fail!");
        return HDF_FAILURE;
    }
    startMs = OsalGetSysTimeMs();
    nRead = UartRead(tester->handle, tester->config.rbuf, len);
    useMs = OsalGetSysTimeMs() - startMs;
    if (nRead != (int32_t)len || useMs > UART_TEST_IMMEDIATE_MS ||
        !UartTestBufMatch(tester->config.rbuf, tester->config.wbuf, len)) {
        HDF_LOGE("UartTestReadyDataRead: nRead:%d useMs:%llu!", nRead, (unsigned long long)useMs);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t UartVminVtimeAttrTest(struct UartTester *tester)
{
    struct UartAttribute get = {0};
    int32_t ret;

    if (!UartTestAttrRoundtrip(tester->handle, UART_TEST_VMIN_PROBE, UART_TEST_VTIME_PROBE)) {
        HDF_LOGW("UartVminVtimeAttrTest: vmin/vtime not supported, skip!");
        return HDF_SUCCESS;
    }

    /* SetBaud must not clear vmin/vtime */
    ret = UartSetBaud(tester->handle, UART_TEST_BAUD_115200);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartVminVtimeAttrTest: set baud fail, ret: %d!", ret);
        return ret;
    }
    ret = UartGetAttribute(tester->handle, &get);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartVminVtimeAttrTest: get attribute fail, ret: %d!", ret);
        return ret;
    }
    if (get.vmin != UART_TEST_VMIN_PROBE || get.vtime != UART_TEST_VTIME_PROBE) {
        HDF_LOGE("UartVminVtimeAttrTest: set baud clears vmin/vtime!");
        return HDF_FAILURE;
    }

    /* boundary values of the 8-bit vmin/vtime bitfields: 255 and 0 */
    if (!UartTestAttrRoundtrip(tester->handle, UART_TEST_VMIN_MAX, UART_TEST_VTIME_MAX)) {
        HDF_LOGE("UartVminVtimeAttrTest: max boundary roundtrip fail!");
        return HDF_FAILURE;
    }
    if (!UartTestAttrRoundtrip(tester->handle, 0, 0)) {
        HDF_LOGE("UartVminVtimeAttrTest: zero boundary roundtrip fail!");
        return HDF_FAILURE;
    }
    HDF_LOGD("UartVminVtimeAttrTest: success!");
    return HDF_SUCCESS;
}

/* one trans-mode timing step: log, apply cfg, timed idle read */
static int32_t UartTestModeStep(struct UartTester *tester, const struct UartTestReadCfg *cfg,
    uint64_t minMs, uint64_t maxMs, const char *tag)
{
    HDF_LOGI("%s", tag);
    if (UartTestApplyReadCfg(tester->handle, cfg->vmin, cfg->vtime, cfg->mode) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    return UartTestTimedIdleRead(tester->handle, tester->config.rbuf, tester->config.len, minMs, maxMs);
}

static int32_t UartTransModeSwitchTest(struct UartTester *tester)
{
    int32_t ret;

    if (!UartTestVminVtimeSupported(tester->handle)) {
        HDF_LOGW("UartTransModeSwitchTest: vmin/vtime not supported, skip!");
        return HDF_SUCCESS;
    }

    /* dma enums are no-op success and out-of-range modes are rejected; checked
     * behind the probe because only the adapters implementing vmin/vtime
     * (khdf/linux) define this behaviour */
    if (!UartTestCheckDmaModes(tester->handle)) {
        HDF_LOGE("UartTransModeSwitchTest: dma no-op mode fail!");
        return HDF_FAILURE;
    }
    if (!UartTestCheckBadMode(tester->handle)) {
        HDF_LOGE("UartTransModeSwitchTest: out-of-range mode not rejected!");
        return HDF_FAILURE;
    }
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);

    /* mode x vmin/vtime matrix, idle-line (negative) direction. NOTE: with
     * vmin > 0 and no data a block read blocks forever by termios design
     * (the inter-byte timer only arms after the first byte arrives). */
    const struct UartTestReadCfg cfgPollNonblock = {0, 0, UART_MODE_RD_NONBLOCK};
    const struct UartTestReadCfg cfgPollBlock = {0, 0, UART_MODE_RD_BLOCK};
    const struct UartTestReadCfg cfgVtimeBlock = {0, UART_TEST_VTIME_DS, UART_MODE_RD_BLOCK};
    const struct UartTestReadCfg cfgVtimeNonblock = {0, UART_TEST_VTIME_DS, UART_MODE_RD_NONBLOCK};
    const struct UartTestReadCfg cfgVminVtimeNonblock = {UART_TEST_VMIN_PARTIAL,
        UART_TEST_VTIME_DS, UART_MODE_RD_NONBLOCK};
    const struct UartTestReadCfg cfgVminNonblock = {UART_TEST_VMIN_PARTIAL, 0, UART_MODE_RD_NONBLOCK};

    ret = UartTestModeStep(tester, &cfgPollNonblock, 0, UART_TEST_IMMEDIATE_MS,
        "UartTransModeSwitchTest: step 1, nonblock idle read");
    ret |= UartTestModeStep(tester, &cfgVtimeBlock, UART_TEST_VTIME_MIN_MS, UART_TEST_VTIME_MAX_MS,
        "UartTransModeSwitchTest: step 2, block vmin=0 vtime read-timer");
    ret |= UartTestModeStep(tester, &cfgPollNonblock, 0, UART_TEST_IMMEDIATE_MS,
        "UartTransModeSwitchTest: step 3, restore nonblock");
    ret |= UartTestModeStep(tester, &cfgVminVtimeNonblock, 0, UART_TEST_IMMEDIATE_MS,
        "UartTransModeSwitchTest: step 4, nonblock overrides vmin/vtime");
    ret |= UartTestModeStep(tester, &cfgVtimeNonblock, 0, UART_TEST_IMMEDIATE_MS,
        "UartTransModeSwitchTest: step 5, nonblock overrides vtime read-timer");
    ret |= UartTestModeStep(tester, &cfgVminNonblock, 0, UART_TEST_IMMEDIATE_MS,
        "UartTransModeSwitchTest: step 6, nonblock overrides vmin");
    ret |= UartTestModeStep(tester, &cfgPollBlock, 0, UART_TEST_IMMEDIATE_MS,
        "UartTransModeSwitchTest: step 7, block vmin=0 vtime=0 polling");
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    HDF_LOGD("UartTransModeSwitchTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartSelfLoopDataTest(struct UartTester *tester)
{
    struct UartAttribute attr = {0};
    uint32_t received;
    int32_t ret;

    ret = UartTestDataPlaneReady(tester, "UartSelfLoopDataTest");
    if (ret <= 0) {
        return (ret == 0) ? HDF_SUCCESS : HDF_FAILURE;
    }
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);
    if (UartSetAttribute(tester->handle, &attr) != HDF_SUCCESS ||
        UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK) != HDF_SUCCESS) {
        HDF_LOGE("UartSelfLoopDataTest: set nonblock fail!");
        return HDF_FAILURE;
    }
    ret = UartTestWriteAndRecv(tester, tester->handle, tester->handle, tester->config.len, &received);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    HDF_LOGD("UartSelfLoopDataTest: looped %u bytes, success!", received);

    /* positive direction of the mode x vmin/vtime matrix: with the echoed
     * bytes already in the rx buffer, a read returns them immediately under
     * every combination */
    if (tester->config.len >= UART_TEST_READY_LEN) {
        const struct UartTestReadCfg cfgPollBlock = {0, 0, UART_MODE_RD_BLOCK};
        const struct UartTestReadCfg cfgVtimeBlock = {0, UART_TEST_VTIME_DS, UART_MODE_RD_BLOCK};
        const struct UartTestReadCfg cfgVtimeNonblock = {0, UART_TEST_VTIME_DS, UART_MODE_RD_NONBLOCK};
        const struct UartTestReadCfg cfgVminVtimeNonblock = {UART_TEST_VMIN_PARTIAL,
            UART_TEST_VTIME_DS, UART_MODE_RD_NONBLOCK};
        const struct UartTestReadCfg cfgVminBlock = {UART_TEST_VMIN_PARTIAL, 0, UART_MODE_RD_BLOCK};
        const struct UartTestReadCfg cfgVminVtimeBlock = {UART_TEST_VMIN_PARTIAL,
            UART_TEST_VTIME_DS, UART_MODE_RD_BLOCK};

        ret = UartTestReadyDataRead(tester, &cfgPollBlock, UART_TEST_READY_LEN,
            "UartSelfLoopDataTest: block 0/0 data-ready");
        ret |= UartTestReadyDataRead(tester, &cfgVtimeBlock, UART_TEST_READY_LEN,
            "UartSelfLoopDataTest: block 0/vtime data-ready");
        ret |= UartTestReadyDataRead(tester, &cfgVtimeNonblock, UART_TEST_READY_LEN,
            "UartSelfLoopDataTest: nonblock 0/vtime data-ready");
        ret |= UartTestReadyDataRead(tester, &cfgVminVtimeNonblock,
            UART_TEST_READY_LEN / UART_TEST_VMIN_HALF,
            "UartSelfLoopDataTest: nonblock under-vmin data-ready");
        ret |= UartTestReadyDataRead(tester, &cfgVminBlock, UART_TEST_READY_LEN,
            "UartSelfLoopDataTest: block vmin-met data-ready");
        ret |= UartTestReadyDataRead(tester, &cfgVminVtimeBlock, UART_TEST_READY_LEN,
            "UartSelfLoopDataTest: block vmin-met vtime data-ready");
        if (ret != HDF_SUCCESS) {
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

/* positive counterpart of the "non-block overrides vmin" contract: with vmin
 * still configured, a non-block read returns the available bytes immediately
 * even when fewer than vmin have arrived */
static int32_t UartVminWakeUnderVminCheck(struct UartTester *tester, uint32_t wakeLen)
{
    uint32_t partial;
    uint32_t received;
    int32_t ret;

    HDF_LOGI("UartVminWakeTest: nonblock under-vmin read");
    if (UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK) != HDF_SUCCESS) {
        HDF_LOGE("UartVminWakeTest: set nonblock fail!");
        return HDF_FAILURE;
    }
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);
    partial = wakeLen / UART_TEST_VMIN_HALF; /* less than vmin, must not wait for the rest */
    ret = UartTestWriteAndRecv(tester, tester->handle, tester->handle, partial, &received);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    HDF_LOGD("UartVminWakeTest: got %u of %u vmin bytes without blocking, success!", received, wakeLen);
    return HDF_SUCCESS;
}

static int32_t UartVminWakeTest(struct UartTester *tester)
{
    uint64_t startMs;
    uint64_t useMs;
    uint32_t wakeLen;
    int32_t nRead;
    int32_t ret;

    ret = UartTestDataPlaneReady(tester, "UartVminWakeTest");
    if (ret <= 0) {
        return (ret == 0) ? HDF_SUCCESS : HDF_FAILURE;
    }
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);

    wakeLen = (tester->config.len < UART_TEST_VMIN_PARTIAL) ? tester->config.len : UART_TEST_VMIN_PARTIAL;
    ret = UartTestApplyReadCfg(tester->handle, (uint8_t)wakeLen, 0, UART_MODE_RD_BLOCK);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartVminWakeTest: set block vmin fail!");
        return HDF_FAILURE;
    }
    ret = UartWrite(tester->handle, tester->config.wbuf, wakeLen);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartVminWakeTest: write fail, ret: %d!", ret);
        return ret;
    }
    /* blocking read wakes up once wakeLen bytes have arrived */
    startMs = OsalGetSysTimeMs();
    nRead = UartRead(tester->handle, tester->config.rbuf, wakeLen);
    useMs = OsalGetSysTimeMs() - startMs;
    if (nRead != (int32_t)wakeLen || useMs > UART_TEST_WAKE_MAX_MS ||
        !UartTestBufMatch(tester->config.rbuf, tester->config.wbuf, wakeLen)) {
        HDF_LOGE("UartVminWakeTest: nRead:%d useMs:%llu!", nRead, (unsigned long long)useMs);
        return HDF_FAILURE;
    }
    HDF_LOGD("UartVminWakeTest: woken by %u bytes in %llu ms, success!", wakeLen, (unsigned long long)useMs);
    return UartVminWakeUnderVminCheck(tester, wakeLen);
}

static int32_t UartVtimeInterByteTest(struct UartTester *tester)
{
    uint64_t startMs;
    uint64_t useMs;
    uint32_t vmin;
    uint32_t partial;
    int32_t nRead;
    int32_t ret;

    ret = UartTestDataPlaneReady(tester, "UartVtimeInterByteTest");
    if (ret <= 0) {
        return (ret == 0) ? HDF_SUCCESS : HDF_FAILURE;
    }
    if (tester->config.len < UART_TEST_VMIN_HALF) { /* partial = len/2 must be >= 1 */
        HDF_LOGW("UartVtimeInterByteTest: config len too small, skip!");
        return HDF_SUCCESS;
    }
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);

    vmin = (tester->config.len < UART_TEST_VMIN_PARTIAL) ? tester->config.len : UART_TEST_VMIN_PARTIAL;
    partial = vmin / UART_TEST_VMIN_HALF; /* less than vmin, read must end by inter-byte vtime */
    if (UartTestApplyReadCfg(tester->handle, (uint8_t)vmin, UART_TEST_VTIME_DS,
        UART_MODE_RD_BLOCK) != HDF_SUCCESS) {
        HDF_LOGE("UartVtimeInterByteTest: set block vmin/vtime fail!");
        return HDF_FAILURE;
    }
    if (UartWrite(tester->handle, tester->config.wbuf, partial) != HDF_SUCCESS) {
        HDF_LOGE("UartVtimeInterByteTest: write fail!");
        return HDF_FAILURE;
    }
    /* The sleep makes sure the partial bytes have fully echoed back into the tty
     * buffer before the read starts, so the timing below is deterministic. */
    OsalMSleep(UART_TEST_INTERBYTE_SLEEP_MS);
    /* Per termios (n_tty.c): with vmin > 0 the inter-byte timer arms only after
     * the first byte is copied by THIS read, then expires vtime later. So the
     * read returns the partial bytes after about vtime (1s), neither blocking
     * forever nor returning early. */
    startMs = OsalGetSysTimeMs();
    nRead = UartRead(tester->handle, tester->config.rbuf, vmin);
    useMs = OsalGetSysTimeMs() - startMs;
    if (nRead != (int32_t)partial || useMs < UART_TEST_VTIME_MIN_MS || useMs > UART_TEST_VTIME_MAX_MS ||
        !UartTestBufMatch(tester->config.rbuf, tester->config.wbuf, partial)) {
        HDF_LOGE("UartVtimeInterByteTest: nRead:%d useMs:%llu!", nRead, (unsigned long long)useMs);
        return HDF_FAILURE;
    }
    HDF_LOGD("UartVtimeInterByteTest: got %d bytes after vtime timer, success!", nRead);
    return HDF_SUCCESS;
}

/* receive exactly len bytes in chunks with content verify, counting read calls */
static int32_t UartTestRecvExact(struct UartTester *tester, uint32_t len, uint32_t chunk,
    uint64_t deadlineMs, uint32_t *readCalls)
{
    uint32_t got = 0;
    int32_t nRead;

    while (got < len) {
        if (OsalGetSysTimeMs() > deadlineMs) {
            HDF_LOGE("UartTestRecvExact: timeout, %u/%u!", got, len);
            return HDF_FAILURE;
        }
        nRead = UartRead(tester->handle, tester->config.rbuf, chunk);
        if (nRead > 0) {
            if (!UartTestBufMatch(tester->config.rbuf, tester->config.wbuf + got, nRead)) {
                HDF_LOGE("UartTestRecvExact: data mismatch at %u!", got);
                return HDF_FAILURE;
            }
            got += nRead;
            (*readCalls)++;
        } else {
            OsalMSleep(UART_TEST_DRAIN_SLEEP_MS);
        }
    }
    return HDF_SUCCESS;
}

static int32_t UartBatchReadPerfTest(struct UartTester *tester)
{
    uint64_t startMs;
    uint64_t useMs;
    uint64_t deadline;
    uint32_t chunk;
    uint32_t total = 0;
    uint32_t readCalls = 0;
    uint32_t i;
    int32_t ret;

    ret = UartTestDataPlaneReady(tester, "UartBatchReadPerfTest");
    if (ret <= 0) {
        return (ret == 0) ? HDF_SUCCESS : HDF_FAILURE;
    }
    UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);
    if (UartTestApplyReadCfg(tester->handle, 0, 0, UART_MODE_RD_NONBLOCK) != HDF_SUCCESS) {
        HDF_LOGE("UartBatchReadPerfTest: set nonblock fail!");
        return HDF_FAILURE;
    }

    chunk = (tester->config.len < UART_TEST_BATCH_CHUNK) ? tester->config.len : UART_TEST_BATCH_CHUNK;
    startMs = OsalGetSysTimeMs();
    deadline = startMs + UART_TEST_RECV_TIMEOUT_MS * UART_TEST_BATCH_LOOPS;
    for (i = 0; i < UART_TEST_BATCH_LOOPS; i++) {
        ret = UartWrite(tester->handle, tester->config.wbuf, tester->config.len);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("UartBatchReadPerfTest: write fail at loop %u!", i);
            return ret;
        }
        ret = UartTestRecvExact(tester, tester->config.len, chunk, deadline, &readCalls);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
        total += tester->config.len;
    }
    useMs = OsalGetSysTimeMs() - startMs;
    HDF_LOGI("UartBatchReadPerfTest: %u bytes by %u reads in %llu ms", total, readCalls, (unsigned long long)useMs);
    if (total != tester->config.len * UART_TEST_BATCH_LOOPS) {
        HDF_LOGE("UartBatchReadPerfTest: total %u != %u!", total, tester->config.len * UART_TEST_BATCH_LOOPS);
        return HDF_FAILURE;
    }
    HDF_LOGD("UartBatchReadPerfTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartDualPortDataTest(struct UartTester *tester)
{
    struct UartAttribute attr = {0};
    DevHandle peerHandle = NULL;
    uint32_t received;
    int32_t ret;

    if (tester->config.peerPort == UART_TEST_PEER_PORT_NONE) {
        HDF_LOGW("UartDualPortDataTest: peer port not configured, skip!");
        return HDF_SUCCESS;
    }
    if (!UartTestVminVtimeSupported(tester->handle)) {
        HDF_LOGW("UartDualPortDataTest: vmin/vtime not supported, skip!");
        return HDF_SUCCESS;
    }
    peerHandle = UartOpen(tester->config.peerPort);
    if (peerHandle == NULL) {
        HDF_LOGE("UartDualPortDataTest: open peer port %u fail!", tester->config.peerPort);
        return HDF_FAILURE;
    }
    do {
        /* both ports must run at the same known rate */
        if (UartSetBaud(tester->handle, UART_TEST_BAUD_115200) != HDF_SUCCESS ||
            UartSetBaud(peerHandle, UART_TEST_BAUD_115200) != HDF_SUCCESS) {
            HDF_LOGE("UartDualPortDataTest: set baud fail!");
            ret = HDF_FAILURE;
            break;
        }
        if (UartSetAttribute(peerHandle, &attr) != HDF_SUCCESS ||
            UartSetTransMode(peerHandle, UART_MODE_RD_NONBLOCK) != HDF_SUCCESS) {
            HDF_LOGE("UartDualPortDataTest: set peer nonblock fail!");
            ret = HDF_FAILURE;
            break;
        }
        UartTestDrainInput(tester->handle, tester->config.rbuf, tester->config.len);
        UartTestDrainInput(peerHandle, tester->config.rbuf, tester->config.len);

        /* local tx -> peer rx */
        ret = UartTestWriteAndRecv(tester, tester->handle, peerHandle, tester->config.len, &received);
        if (ret != HDF_SUCCESS) {
            break;
        }
        /* peer tx -> local rx */
        ret = UartTestWriteAndRecv(tester, peerHandle, tester->handle, tester->config.len, &received);
    } while (0);
    UartClose(peerHandle);
    HDF_LOGD("UartDualPortDataTest: ret %d", ret);
    return ret;
}

static struct UartTestEntry g_entry[] = {
    { UART_TEST_CMD_WRITE, UartWriteTest, "UartWriteTest" },
    { UART_TEST_CMD_READ, UartReadTest, "UartReadTest" },
    { UART_TEST_CMD_SET_BAUD, UartSetBaudTest, "UartSetBaudTest" },
    { UART_TEST_CMD_GET_BAUD, UartGetBaudTest, "UartGetBaudTest" },
    { UART_TEST_CMD_SET_ATTRIBUTE, UartSetAttributeTest, "UartSetAttributeTest" },
    { UART_TEST_CMD_GET_ATTRIBUTE, UartGetAttributeTest, "UartGetAttributeTest" },
    { UART_TEST_CMD_SET_TRANSMODE, UartSetTransModeTest, "UartSetTransModeTest" },
    { UART_TEST_CMD_RELIABILITY, UartReliabilityTest, "UartReliabilityTest" },
    { UART_TEST_CMD_PERFORMANCE, UartIfPerformanceTest, "UartIfPerformanceTest" },
    { UART_MINI_BLOCK_WRITE_TEST, UartMiniBlockWriteTest, "UartMiniBlockWriteTest" },
    { UART_TEST_CMD_VMIN_VTIME_ATTR, UartVminVtimeAttrTest, "UartVminVtimeAttrTest" },
    { UART_TEST_CMD_TRANS_MODE_SWITCH, UartTransModeSwitchTest, "UartTransModeSwitchTest" },
    { UART_TEST_CMD_SELF_LOOP_DATA, UartSelfLoopDataTest, "UartSelfLoopDataTest" },
    { UART_TEST_CMD_VMIN_WAKE, UartVminWakeTest, "UartVminWakeTest" },
    { UART_TEST_CMD_VTIME_INTERBYTE, UartVtimeInterByteTest, "UartVtimeInterByteTest" },
    { UART_TEST_CMD_BATCH_READ_PERF, UartBatchReadPerfTest, "UartBatchReadPerfTest" },
    { UART_TEST_CMD_DUAL_PORT_DATA, UartDualPortDataTest, "UartDualPortDataTest" },
};

int32_t UartTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;
    struct UartTester *tester = NULL;

    tester = UartTesterGet();
    if (tester == NULL) {
        HDF_LOGE("UartTestExecute: tester is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cmd > UART_TEST_CMD_MAX) {
        HDF_LOGE("UartTestExecute: invalid cmd:%d!", cmd);
        ret = HDF_ERR_NOT_SUPPORT;
        HDF_LOGE("[UartTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
        UartTesterPut(tester);
        return ret;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = g_entry[i].func(tester);
        break;
    }

    HDF_LOGE("[UartTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    UartTesterPut(tester);
    return ret;
}

/*
 * Copyright (c) 2026 Nanjing Qinuo Information Technology Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mipi_tx_hi3403.h"
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <asm/barrier.h>
#include "hdf_core_log.h"
#include "securec.h"
#include "osal_time.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "mipi_dsi_define.h"
#include "mipi_dsi_core.h"
#include "mipi_tx_hi3403_reg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define HDF_LOG_TAG mipi_tx_hi3403
#define INT_MAX_VALUE 0x7fffffff

volatile  MipiTxRegsTypeTag *g_mipiTxRegsVa = NULL;
unsigned int g_mipiTxIrqNum = MIPI_TX_IRQ;
unsigned int g_actualPhyDataRate;
static unsigned int g_regMapFlag;
static bool g_enCfg = false;
static bool g_enHsMode = false;

/* ====== low-level helpers ====== */

static void WriteReg32(unsigned long *addr, unsigned int value, unsigned int mask)
{
    unsigned int t;

    t = (unsigned int)OSAL_READL(addr);
    t &= ~mask;
    t |= value & mask;
    OSAL_WRITEL(t, addr);
}

static void OsalIsb(void)
{
    isb();
}

static void OsalDsb(void)
{
    dsb(sy);
}

static void OsalDmb(void)
{
    dmb(sy);
}

static void HdfIsbDsbDmb(void)
{
    OsalIsb();
    OsalDsb();
    OsalDmb();
}

static void SetPhyReg(unsigned int addr, unsigned char value)
{
    HdfIsbDsbDmb();
    g_mipiTxRegsVa->PHY_TST_CTRL1.u32 = (0x10000 + addr);
    HdfIsbDsbDmb();
    g_mipiTxRegsVa->PHY_TST_CTRL0.u32 = 0x2;
    HdfIsbDsbDmb();
    g_mipiTxRegsVa->PHY_TST_CTRL0.u32 = 0x0;
    HdfIsbDsbDmb();
    g_mipiTxRegsVa->PHY_TST_CTRL1.u32 = value;
    HdfIsbDsbDmb();
    g_mipiTxRegsVa->PHY_TST_CTRL0.u32 = 0x2;
    HdfIsbDsbDmb();
    g_mipiTxRegsVa->PHY_TST_CTRL0.u32 = 0x0;
    HdfIsbDsbDmb();
}

static int mipi_tx_power(unsigned int base, unsigned int exp)
{
    int result = 1;
    while (exp) {
        if (exp & 0x01)
            result = result * (int)base;
        base = base * base;
        exp = exp >> 1;
    }
    return result;
}

static inline unsigned char Hi3403GetPhyTimingByType(unsigned int timingType)
{
    return (unsigned char)((g_actualPhyDataRate * timingType + ROUNDUP_VALUE) / INNER_PEROID);
}

/* ====== hi3403 command mode / clock lane helpers ====== */

static void Hi3403SetCmdLpMode(void)
{
    g_mipiTxRegsVa->CMD_MODE_CFG.u32 = 0xFFF;
}

static void Hi3403SetCmdHsMode(void)
{
    g_mipiTxRegsVa->CMD_MODE_CFG.u32 = 0x0;
}

static void Hi3403SetClkLaneCfg(int hsEnable)
{
    U_LPCLK_CTRL clkLane;
    clkLane.u32 = g_mipiTxRegsVa->LPCLK_CTRL.u32;
    clkLane.bits.phy_txrequestclkhs = (unsigned int)hsEnable;
    clkLane.bits.auto_clklane_ctrl = 0;
    g_mipiTxRegsVa->LPCLK_CTRL.u32 = clkLane.u32;
}

/* ====== hi3403 phy reset helpers ====== */

static void Hi3403PhyReset(void)
{
    U_PHY_RSTZ phyCtrl;
    phyCtrl.u32 = g_mipiTxRegsVa->PHY_RSTZ.u32;
    phyCtrl.bits.phy_rstz = 0x0;
    g_mipiTxRegsVa->PHY_RSTZ.u32 = phyCtrl.u32;
}

static void Hi3403PhyUnreset(void)
{
    U_PHY_RSTZ phyCtrl;
    phyCtrl.u32 = g_mipiTxRegsVa->PHY_RSTZ.u32;
    phyCtrl.bits.phy_rstz = 0x1;
    g_mipiTxRegsVa->PHY_RSTZ.u32 = phyCtrl.u32;
}

static void Hi3403ControllerPhyReset(void)
{
    g_mipiTxRegsVa->PWR_UP.u32 = 0x0;
    Hi3403PhyReset();
    OsalUDelay(1);
    Hi3403PhyUnreset();
    g_mipiTxRegsVa->PWR_UP.u32 = 0x1;
}

/* ====== hi3403 PLL config ====== */

static unsigned char Hi3403GetPhyPllSet063(unsigned int phyDataRate, unsigned char pllSet065)
{
    return (unsigned char)(mipi_tx_ceil(phyDataRate, MIPI_TX_REF_CLK) *
        mipi_tx_power(2, (pllSet065 % 8)) % 256);
}

static unsigned char Hi3403GetPhyPllSet064(unsigned int phyDataRate)
{
    return (unsigned char)(mipi_tx_ceil(phyDataRate, MIPI_TX_REF_CLK) / 256);
}

static void MipiTxDrvSetPhyPllSetX(unsigned int phyDataRate)
{
    unsigned char pllSet063, pllSet064;
    const unsigned char pllSet065 = 0x0;

    pllSet063 = Hi3403GetPhyPllSet063(phyDataRate, pllSet065);
    pllSet064 = Hi3403GetPhyPllSet064(phyDataRate);

    SetPhyReg(PLL_SET_0X63, pllSet063);
    SetPhyReg(PLL_SET_0X64, pllSet064);
    SetPhyReg(PLL_SET_0X65, pllSet065);
}

/* ====== hi3403 PHY timing config ====== */

static void MipiTxDrvGetPhyTimingParam(MipiTxPhyTimingParamTag *tp)
{
    tp->data_tpre_delay = Hi3403GetPhyTimingByType(TPRE_DELAY);
    tp->clk_tlpx = Hi3403GetPhyTimingByType(TLPX);
    tp->clk_tclk_prepare = Hi3403GetPhyTimingByType(TCLK_PREPARE);
    tp->clk_tclk_zero = Hi3403GetPhyTimingByType(TCLK_ZERO);
    tp->clk_tclk_trail = Hi3403GetPhyTimingByType(TCLK_TRAIL);
    tp->data_tlpx = Hi3403GetPhyTimingByType(TLPX);
    tp->data_ths_prepare = Hi3403GetPhyTimingByType(THS_PREPARE);
    tp->data_ths_zero = Hi3403GetPhyTimingByType(THS_ZERO);
    tp->data_ths_trail = Hi3403GetPhyTimingByType(THS_TRAIL);
    tp->clk_post_delay = Hi3403GetPhyTimingByType(TCLK_POST) + tp->data_ths_trail + 4;
}

static void MipiTxDrvSetPhyTimingParam(const MipiTxPhyTimingParamTag *tp)
{
    SetPhyReg(DATA0_TPRE_DELAY, tp->data_tpre_delay);
    SetPhyReg(DATA1_TPRE_DELAY, tp->data_tpre_delay);
    SetPhyReg(DATA2_TPRE_DELAY, tp->data_tpre_delay);
    SetPhyReg(DATA3_TPRE_DELAY, tp->data_tpre_delay);
    SetPhyReg(CLK_POST_DELAY, tp->clk_post_delay);
    SetPhyReg(CLK_TLPX, tp->clk_tlpx);
    SetPhyReg(CLK_TCLK_PREPARE, tp->clk_tclk_prepare);
    SetPhyReg(CLK_TCLK_ZERO, tp->clk_tclk_zero);
    SetPhyReg(CLK_TCLK_TRAIL, tp->clk_tclk_trail);
    SetPhyReg(DATA0_TLPX, tp->data_tlpx);
    SetPhyReg(DATA0_THS_PREPARE, tp->data_ths_prepare);
    SetPhyReg(DATA0_THS_ZERO, tp->data_ths_zero);
    SetPhyReg(DATA0_THS_TRAIL, tp->data_ths_trail);
    SetPhyReg(DATA1_TLPX, tp->data_tlpx);
    SetPhyReg(DATA1_THS_PREPARE, tp->data_ths_prepare);
    SetPhyReg(DATA1_THS_ZERO, tp->data_ths_zero);
    SetPhyReg(DATA1_THS_TRAIL, tp->data_ths_trail);
    SetPhyReg(DATA2_TLPX, tp->data_tlpx);
    SetPhyReg(DATA2_THS_PREPARE, tp->data_ths_prepare);
    SetPhyReg(DATA2_THS_ZERO, tp->data_ths_zero);
    SetPhyReg(DATA2_THS_TRAIL, tp->data_ths_trail);
    SetPhyReg(DATA3_TLPX, tp->data_tlpx);
    SetPhyReg(DATA3_THS_PREPARE, tp->data_ths_prepare);
    SetPhyReg(DATA3_THS_ZERO, tp->data_ths_zero);
    SetPhyReg(DATA3_THS_TRAIL, tp->data_ths_trail);
}

static void MipiTxDrvSetPhyHsLpSwitchTime(const MipiTxPhyTimingParamTag *tp)
{
    g_mipiTxRegsVa->PHY_TMR_CFG.u32 = tp->data_tpre_delay + tp->data_tlpx +
        tp->data_ths_prepare + tp->data_ths_zero + 4 +
        (tp->data_ths_trail + 1) * 65536;
    g_mipiTxRegsVa->PHY_TMR_LPCLK_CFG.u32 = tp->clk_tlpx + tp->clk_tclk_prepare +
        tp->clk_tclk_zero + 3 +
        (tp->clk_post_delay + 1 + tp->data_ths_trail) * 65536;
}

static void MipiTxDrvSetPhyCfg(const ComboDevCfgTag *cfg)
{
    MipiTxPhyTimingParamTag tp;

    (void)cfg;
    (void)memset_s(&tp, sizeof(tp), 0, sizeof(tp));

    /* Reset PHY */
    g_mipiTxRegsVa->PHY_RSTZ.u32 = 0x0;
    OsalUDelay(1000);

    /* Disable input */
    g_mipiTxRegsVa->OPERATION_MODE.u32 = 0x0;

    /* ctrl_reset toggle */
    g_mipiTxRegsVa->PWR_UP.u32 = 0x0;
    OsalUDelay(1);
    g_mipiTxRegsVa->PWR_UP.u32 = 0x1;

    /* PLL + Timing + HS/LP switch */
    MipiTxDrvSetPhyPllSetX(g_actualPhyDataRate);
    MipiTxDrvGetPhyTimingParam(&tp);
    MipiTxDrvSetPhyTimingParam(&tp);
    MipiTxDrvSetPhyHsLpSwitchTime(&tp);

    /* PHY enable */
    g_mipiTxRegsVa->PHY_RSTZ.u32 = 0xF;
    OsalMSleep(1);
}

/* ====== output / timing / lane config ====== */

static void SetOutputFormat(const ComboDevCfgTag *cfg)
{
    int colorCoding = 0;

    if (cfg->outputMode == OUTPUT_MODE_CSI) {
        if (cfg->outputFormat == OUT_FORMAT_YUV420_8_BIT_NORMAL)
            colorCoding = 0xD;
        else if (cfg->outputFormat == OUT_FORMAT_YUV422_8_BIT)
            colorCoding = 0x1E;
    } else {
        if (cfg->outputFormat == OUT_FORMAT_RGB_16_BIT)
            colorCoding = 0x0;
        else if (cfg->outputFormat == OUT_FORMAT_RGB_18_BIT)
            colorCoding = 0x3;
        else if (cfg->outputFormat == OUT_FORMAT_RGB_24_BIT)
            colorCoding = 0x5;
        else
            colorCoding = 0x5;
    }
    g_mipiTxRegsVa->COLOR_CODING.u32 = (unsigned int)colorCoding;
}

static void SetVideoModeCfg(const ComboDevCfgTag *cfg)
{
    unsigned int modeCfgVal;
    unsigned int videoModeType;
    U_READ_MEMORY_DELAY_CTRL readMemDelay;

    if (cfg->videoMode == NON_BURST_MODE_SYNC_PULSES)
        videoModeType = 0;
    else if (cfg->videoMode == NON_BURST_MODE_SYNC_EVENTS)
        videoModeType = 1;
    else
        videoModeType = 2;

    if ((cfg->outputMode == OUTPUT_MODE_CSI) || (cfg->outputMode == OUTPUT_MODE_DSI_CMD))
        videoModeType = 2;

    /* mode_cfg: cmd_video_mode=1, video_mode_type at bits 9:8 */
    modeCfgVal = g_mipiTxRegsVa->MODE_CFG.u32;
    modeCfgVal &= ~(0x3 << 8);
    modeCfgVal |= (videoModeType << 8);
    modeCfgVal |= 0x1;  /* cmd_video_mode = 1 */
    g_mipiTxRegsVa->MODE_CFG.u32 = modeCfgVal;

    /* video_lp_en = 0x3f */
    g_mipiTxRegsVa->VID_MODE_CFG.u32 = 0x3f;

    /* read_memory_delay_ctrl */
    readMemDelay.u32 = g_mipiTxRegsVa->READ_MEMORY_DELAY_CTRL.u32;
    if ((cfg->videoMode == NON_BURST_MODE_SYNC_PULSES) ||
        (cfg->videoMode == NON_BURST_MODE_SYNC_EVENTS))
        readMemDelay.bits.delay_abnormal = 0x1;
    else
        readMemDelay.bits.delay_abnormal = 0x0;
    g_mipiTxRegsVa->READ_MEMORY_DELAY_CTRL.u32 = readMemDelay.u32;
}

static void SetTimingConfig(const ComboDevCfgTag *cfg)
{
    unsigned int hsa, hbp, hline;
    unsigned int hact, hfp;

    if (cfg->pixelClk == 0) {
        HDF_LOGE("SetTimingConfig: pixelClk is 0!");
        return;
    }
    hact = cfg->syncInfo.vidHsaPixels + cfg->syncInfo.vidHbpPixels + cfg->syncInfo.vidPktSize;
    hfp = cfg->syncInfo.vidHlinePixels - hact;
    hsa = (unsigned int)((unsigned long long)g_actualPhyDataRate *
        cfg->syncInfo.vidHsaPixels * 125 / cfg->pixelClk);
    hbp = (unsigned int)((unsigned long long)g_actualPhyDataRate *
        cfg->syncInfo.vidHbpPixels * 125 / cfg->pixelClk);
    hline = (unsigned int)((unsigned long long)g_actualPhyDataRate *
        (cfg->syncInfo.vidHsaPixels + cfg->syncInfo.vidHbpPixels +
         cfg->syncInfo.vidPktSize + hfp) * 125 / cfg->pixelClk);

    g_mipiTxRegsVa->VID_HSA_TIME.u32 = hsa;
    g_mipiTxRegsVa->VID_HBP_TIME.u32 = hbp;
    g_mipiTxRegsVa->VID_HLINE_TIME.u32 = hline;
    g_mipiTxRegsVa->VID_VSA_LINES.u32 = cfg->syncInfo.vidVsaLines;
    g_mipiTxRegsVa->VID_VBP_LINES.u32 = cfg->syncInfo.vidVbpLines;
    g_mipiTxRegsVa->VID_VFP_LINES.u32 = cfg->syncInfo.vidVfpLines;
    g_mipiTxRegsVa->VID_VACTIVE_LINES.u32 = cfg->syncInfo.vidActiveLines;

    HDF_LOGI("%s: hsa=%u hbp=%u hline=%u vsa=%u vbp=%u vfp=%u vact=%u",
        __func__, hsa, hbp, hline,
        cfg->syncInfo.vidVsaLines, cfg->syncInfo.vidVbpLines,
        cfg->syncInfo.vidVfpLines, cfg->syncInfo.vidActiveLines);
}

static void SetLaneConfig(const short laneId[], int len)
{
    int num = 0;
    int i;

    for (i = 0; i < len; i++) {
        if (laneId[i] != MIPI_TX_DISABLE_LANE_ID)
            num++;
    }
    /* Preserve HW reset lane_id mapping, only update lane count */
    g_mipiTxRegsVa->PHY_IF_CFG.u32 = (unsigned int)(num - 1);
    HDF_LOGI("%s: lane count=%d", __func__, num);
}

static void MipiTxDrvSetClkMgrCfg(void)
{
    if (g_actualPhyDataRate / 160 < 2) {
        g_mipiTxRegsVa->CLKMGR_CFG.u32 = 0x102;
    } else {
        g_mipiTxRegsVa->CLKMGR_CFG.u32 = 0x100 + (g_actualPhyDataRate + 159) / 160;
    }
}
/* ====== command / read FIFO helpers ====== */

static void MipiTxDrvSetControllerCfg(const ComboDevCfgTag *cfg)
{
    if (cfg == NULL) {
        HDF_LOGE("MipiTxDrvSetControllerCfg: cfg is null!");
        return;
    }

    /* 1. interrupt mask */
    g_mipiTxRegsVa->GINT_MSK = 0x1ffff;

    /* 2. disable input */
    g_mipiTxRegsVa->OPERATION_MODE.u32 = 0x0;

    /* 3. vc_id */
    g_mipiTxRegsVa->VCID.u32 = 0x0;

    /* 4. output format, color coding */
    SetOutputFormat(cfg);

    /* 5. clock manager */
    g_actualPhyDataRate = ((cfg->phyDataRate + MIPI_TX_REF_CLK - 1) / MIPI_TX_REF_CLK) * MIPI_TX_REF_CLK;
    MipiTxDrvSetClkMgrCfg();

    /* 6. command transmission mode — LP mode */
    Hi3403SetCmdLpMode();

    /* 7. pck_en */
    if (cfg->outputMode == OUTPUT_MODE_CSI)
        g_mipiTxRegsVa->PCKHDL_CFG.u32 = 0x0C;
    else
        g_mipiTxRegsVa->PCKHDL_CFG.u32 = 0x1E;

    /* 8. gen_vc */
    g_mipiTxRegsVa->GEN_VCID.u32 = 0x0;

    /* 9. video mode cfg */
    SetVideoModeCfg(cfg);

    /* 10. pkt size */
    if ((cfg->outputMode == OUTPUT_MODE_DSI_VIDEO) || (cfg->outputMode == OUTPUT_MODE_CSI))
        g_mipiTxRegsVa->VID_PKT_SIZE.u32 = cfg->syncInfo.vidPktSize;
    else
        g_mipiTxRegsVa->EDPI_CMD_SIZE.u32 = cfg->syncInfo.vidPktSize;

    /* 11. num_chunks / null_size */
    g_mipiTxRegsVa->VID_NUM_CHUNKS.u32 = 0x0;
    g_mipiTxRegsVa->VID_NULL_SIZE.u32 = 0x0;

    /* 12. timing config */
    SetTimingConfig(cfg);

    /* 13. lp_cmd_tim */
    g_mipiTxRegsVa->LP_CMD_TIM.u32 = 0xff0000;

    /* 14. DSI CMD / CSI extra settings */
    if (cfg->outputMode == OUTPUT_MODE_CSI) {
        g_mipiTxRegsVa->EDPI_CMD_SIZE.u32 = 0;
        g_mipiTxRegsVa->HSRD_TO_SET.u32 = 0x0;
    }
    g_mipiTxRegsVa->HS_LP_TO_SET.u32 = 0x0;
    g_mipiTxRegsVa->LPRD_TO_SET.u32 = 0x0;
    g_mipiTxRegsVa->HSWR_TO_SET.u32 = 0x0;
    g_mipiTxRegsVa->LPWR_TO_SET.u32 = 0x0;
    g_mipiTxRegsVa->BTA_TO_SET.u32 = 0x0;
    g_mipiTxRegsVa->READ_CMD_TIME.u32 = 0x7fff;

    /* 15. datatype0/1, csi_ctrl */
    g_mipiTxRegsVa->DATATYPE0.u32 = 0x111213D;
    g_mipiTxRegsVa->DATATYPE1.u32 = 0x31081909;
    g_mipiTxRegsVa->CSI_CTRL.u32 = 0x10100;

    /* 16. lane config */
    SetLaneConfig(cfg->laneId, LANE_MAX_NUM);

    /* 17. ulps_ctrl */
    g_mipiTxRegsVa->PHY_ULPS_CTRL.u32 = 0x0;

    /* 18. tx_triggers */
    g_mipiTxRegsVa->TX_TRIGGERS.u32 = 0x0;

    /* 19. vid_shadow_ctrl */
    g_mipiTxRegsVa->VID_SHADOW_CTRL.u32 = 0x0;

    /* 20. int0_mask */
    g_mipiTxRegsVa->INT_MSK0.u32 = 0x0;

    /* 21. clklane_continue disable */
    Hi3403SetClkLaneCfg(0);
}

/* ====== clock manager config ====== */

static int MipiTxWaitCmdFifoEmpty(void)
{
    U_CMD_PKT_STATUS cmdPktStatus;
    unsigned int waitCnt;

    waitCnt = 0;
    do {
        cmdPktStatus.u32 = g_mipiTxRegsVa->CMD_PKT_STATUS.u32;
        waitCnt++;
        OsalUDelay(1);
        if (waitCnt >  MIPI_TX_READ_TIMEOUT_CNT) {
            HDF_LOGW("MipiTxWaitCmdFifoEmpty: timeout when send cmd buffer!");
            return HDF_ERR_TIMEOUT;
        }
    } while (cmdPktStatus.bits.gen_cmd_empty == 0);
    return HDF_SUCCESS;
}

static int MipiTxWaitWriteFifoEmpty(void)
{
    U_CMD_PKT_STATUS cmdPktStatus;
    unsigned int waitCnt;

    waitCnt = 0;
    do {
        cmdPktStatus.u32 = g_mipiTxRegsVa->CMD_PKT_STATUS.u32;
        waitCnt++;
        OsalUDelay(1);
        if (waitCnt >  MIPI_TX_READ_TIMEOUT_CNT) {
            HDF_LOGW("MipiTxWaitWriteFifoEmpty: timeout when send data buffer!");
            return HDF_ERR_TIMEOUT;
        }
    } while (cmdPktStatus.bits.gen_pld_w_empty == 0);
    return HDF_SUCCESS;
}

static int MipiTxWaitWriteFifoNotFull(void)
{
    U_CMD_PKT_STATUS cmdPktStatus;
    unsigned int waitCnt;

    waitCnt = 0;
    do {
        cmdPktStatus.u32 = g_mipiTxRegsVa->CMD_PKT_STATUS.u32;
        if (waitCnt > 0) {
            OsalUDelay(1);
            HDF_LOGW("MipiTxWaitWriteFifoNotFull: write fifo full happened wait count = %u!", waitCnt);
        }
        if (waitCnt >  MIPI_TX_READ_TIMEOUT_CNT) {
            HDF_LOGW("MipiTxWaitWriteFifoNotFull: timeout when wait write fifo not full buffer!");
            return HDF_ERR_TIMEOUT;
        }
        waitCnt++;
    } while (cmdPktStatus.bits.gen_pld_w_full == 1);
    return HDF_SUCCESS;
}

/*
 * set payloads data by writing register
 * each 4 bytes in cmd corresponds to one register
 */
static void MipiTxDrvSetPayloadData(const unsigned char *cmd, unsigned short cmdSize)
{
    int32_t ret;
    U_GEN_PLD_DATA genPldData;
    int i;
    int j;

    genPldData.u32 = g_mipiTxRegsVa->GEN_PLD_DATA.u32;

    for (i = 0; i < (cmdSize / 4); i++) { /* 4 cmd once */
        genPldData.bits.gen_pld_b1 = cmd[i * 4]; /* 0 in 4 */
        genPldData.bits.gen_pld_b2 = cmd[i * 4 + 1]; /* 1 in 4 */
        genPldData.bits.gen_pld_b3 = cmd[i * 4 + 2]; /* 2 in 4 */
        genPldData.bits.gen_pld_b4 = cmd[i * 4 + 3]; /* 3 in 4 */
        ret = MipiTxWaitWriteFifoNotFull();
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("MipiTxDrvSetPayloadData: [MipiTxWaitWriteFifoNotFull] fail!");
            return;
        }
        g_mipiTxRegsVa->GEN_PLD_DATA.u32 = genPldData.u32;
    }
    j = cmdSize % 4; /* remainder of 4 */
    if (j != 0) {
        if (j > 0) {
            genPldData.bits.gen_pld_b1 = cmd[i * 4]; /* 0 in 4 */
        }
        if (j > 1) {
            genPldData.bits.gen_pld_b2 = cmd[i * 4 + 1]; /* 1 in 4 */
        }
        if (j > 2) { /* bigger than 2 */
            genPldData.bits.gen_pld_b3 = cmd[i * 4 + 2]; /* 2 in 4 */
        }
        ret = MipiTxWaitWriteFifoNotFull();
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("MipiTxDrvSetPayloadData: [MipiTxWaitWriteFifoNotFull] fail!");
            return;
        }
        g_mipiTxRegsVa->GEN_PLD_DATA.u32 = genPldData.u32;
    }
}

static int32_t LinuxCopyToKernel(void *dest, uint32_t max, const void *src, uint32_t count)
{
    int32_t ret;

    if (access_ok(
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
        VERIFY_READ,
#endif
        src, count)) { /* user space */
        ret = (copy_from_user(dest, src, count) != 0) ? HDF_FAILURE : HDF_SUCCESS;
        if (ret == HDF_FAILURE) {
            HDF_LOGE("LinuxCopyToKernel: [copy_from_user] fail!");
        }
    } else { /* kernel space */
        ret = (memcpy_s(dest, max, src, count) != EOK) ? HDF_FAILURE : HDF_SUCCESS;
        if (ret == HDF_FAILURE) {
            HDF_LOGE("LinuxCopyToKernel: [memcpy_s] fail!");
        }
    }

    return ret;
}

static int MipiTxDrvSetCmdInfo(const CmdInfoTag *cmdInfo)
{
    int32_t ret;
    U_GEN_HDR genHdr;
    unsigned char *cmd = NULL;

    if (cmdInfo == NULL) {
        HDF_LOGE("MipiTxDrvSetCmdInfo: cmdInfo is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    Hi3403SetCmdLpMode();
    Hi3403SetClkLaneCfg(0); /* LP: disable HS clock request */
    genHdr.u32 = g_mipiTxRegsVa->GEN_HDR.u32;
    if (cmdInfo->cmd != NULL) {
        if ((cmdInfo->cmdSize > 200) || (cmdInfo->cmdSize == 0)) { /* 200 is max cmd size */
            HDF_LOGE("MipiTxDrvSetCmdInfo: set cmd size illegal, size =%u!", cmdInfo->cmdSize);
            return HDF_ERR_INVALID_PARAM;
        }
        cmd = (unsigned char *)OsalMemCalloc(cmdInfo->cmdSize);
        if (cmd == NULL) {
            HDF_LOGE("MipiTxDrvSetCmdInfo: OsalMemCalloc fail,please check,need %u bytes!", cmdInfo->cmdSize);
            return HDF_ERR_MALLOC_FAIL;
        }
        ret = LinuxCopyToKernel(cmd, cmdInfo->cmdSize, cmdInfo->cmd, cmdInfo->cmdSize);
        if (ret == HDF_SUCCESS) {
            MipiTxDrvSetPayloadData(cmd, cmdInfo->cmdSize);
        }
        OsalMemFree(cmd);
        cmd = NULL;
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("MipiTxDrvSetCmdInfo: [LinuxCopyToKernel] fail!");
            return HDF_ERR_IO;
        }
    }
    genHdr.bits.gen_dt = cmdInfo->dataType;
    genHdr.bits.gen_wc_lsbyte = cmdInfo->cmdSize & 0xff;
    genHdr.bits.gen_wc_msbyte = (cmdInfo->cmdSize & 0xff00) >> 8; /* height 8 bits */
    g_mipiTxRegsVa->GEN_HDR.u32 = genHdr.u32;
    OsalUDelay(350);  /* wait 350 us transfer end */
    ret = MipiTxWaitCmdFifoEmpty();
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiTxDrvSetCmdInfo: [MipiTxWaitCmdFifoEmpty] fail!");
        return HDF_FAILURE;
    }
    ret = MipiTxWaitWriteFifoEmpty();
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiTxDrvSetCmdInfo: [MipiTxWaitWriteFifoEmpty] fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int MipiTxWaitReadFifoNotEmpty(void)
{
    U_INT_ST0 intSt0;
    U_INT_ST1 intSt1;
    unsigned int waitCnt;
    U_CMD_PKT_STATUS cmdPktStatus;

    waitCnt = 0;
    do {
        intSt1.u32 =  g_mipiTxRegsVa->INT_ST1.u32;
        intSt0.u32 =  g_mipiTxRegsVa->INT_ST0.u32;
        if ((intSt1.u32 & 0x3e) != 0) {
            HDF_LOGE("MipiTxWaitReadFifoNotEmpty: err happened when read data, int_st1 = 0x%x,int_st0 = %x!",
                intSt1.u32, intSt0.u32);
            return HDF_FAILURE;
        }
        if (waitCnt >  MIPI_TX_READ_TIMEOUT_CNT) {
            HDF_LOGW("MipiTxWaitReadFifoNotEmpty: timeout when read data!");
            return HDF_ERR_TIMEOUT;
        }
        waitCnt++;
        OsalUDelay(1);
        cmdPktStatus.u32 = g_mipiTxRegsVa->CMD_PKT_STATUS.u32;
    } while (cmdPktStatus.bits.gen_pld_r_empty == 0x1);
    return HDF_SUCCESS;
}

static int MipiTxWaitReadFifoEmpty(void)
{
    U_GEN_PLD_DATA pldData;
    U_INT_ST1 intSt1;
    unsigned int waitCnt;

    waitCnt = 0;
    do {
        intSt1.u32 = g_mipiTxRegsVa->INT_ST1.u32;
        if ((intSt1.bits.gen_pld_rd_err) == 0x0) {
            pldData.u32 = g_mipiTxRegsVa->GEN_PLD_DATA.u32;
        }
        waitCnt++;
        OsalUDelay(1);
        if (waitCnt >  MIPI_TX_READ_TIMEOUT_CNT) {
            HDF_LOGW("MipiTxWaitReadFifoEmpty: timeout when clear data buffer, the last read data is 0x%x!",
                pldData.u32);
            return HDF_ERR_TIMEOUT;
        }
    } while ((intSt1.bits.gen_pld_rd_err) == 0x0);
    return HDF_SUCCESS;
}

static int MipiTxSendShortPacket(unsigned char virtualChannel,
    short unsigned dataType, unsigned short  dataParam)
{
    U_GEN_HDR genHdr;

    genHdr.bits.gen_vc = virtualChannel;
    genHdr.bits.gen_dt = dataType;
    genHdr.bits.gen_wc_lsbyte = (dataParam & 0xff);
    genHdr.bits.gen_wc_msbyte = (dataParam & 0xff00) >> 8; /* height 8 bits */
    g_mipiTxRegsVa->GEN_HDR.u32 = genHdr.u32;
    if (MipiTxWaitCmdFifoEmpty() != HDF_SUCCESS) {
        HDF_LOGE("MipiTxSendShortPacket: [MipiTxWaitCmdFifoEmpty] fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int MipiTxGetReadFifoData(unsigned int getDataSize, unsigned char *dataBuf)
{
    U_GEN_PLD_DATA pldData;
    unsigned int i;
    unsigned int j;

    for (i = 0; i < getDataSize / 4; i++) {   /* 4byte once */
        if (MipiTxWaitReadFifoNotEmpty() != HDF_SUCCESS) {
            HDF_LOGE("MipiTxGetReadFifoData: [MipiTxWaitReadFifoNotEmpty] fail at first!");
            return HDF_FAILURE;
        }
        pldData.u32 = g_mipiTxRegsVa->GEN_PLD_DATA.u32;
        dataBuf[i * 4] = pldData.bits.gen_pld_b1;     /* 0 in 4 */
        dataBuf[i * 4 + 1] = pldData.bits.gen_pld_b2; /* 1 in 4 */
        dataBuf[i * 4 + 2] = pldData.bits.gen_pld_b3; /* 2 in 4 */
        dataBuf[i * 4 + 3] = pldData.bits.gen_pld_b4; /* 3 in 4 */
    }

    j = getDataSize % 4; /* remainder of 4 */

    if (j != 0) {
        if (MipiTxWaitReadFifoNotEmpty() != HDF_SUCCESS) {
            HDF_LOGE("MipiTxGetReadFifoData: [MipiTxWaitReadFifoNotEmpty] fail at second!");
            return HDF_FAILURE;
        }
        pldData.u32 = g_mipiTxRegsVa->GEN_PLD_DATA.u32;
        if (j > 0) {
            dataBuf[i * 4] = pldData.bits.gen_pld_b1; /* 0 in 4 */
        }
        if (j > 1) {
            dataBuf[i * 4 + 1] = pldData.bits.gen_pld_b2; /* 1 in 4 */
        }
        if (j > 2) { /* bigger than 2 */
            dataBuf[i * 4 + 2] = pldData.bits.gen_pld_b3; /* 2 in 4 */
        }
    }
    return HDF_SUCCESS;
}

/* ====== MIPI TX reset ====== */

static void MipiTxReset(void)
{
    U_OPERATION_MODE operationMode;
    U_MODE_CFG modeCfg;

    /* disable input */
    operationMode.u32 = g_mipiTxRegsVa->OPERATION_MODE.u32;
    operationMode.bits.input_en = 0;
    g_mipiTxRegsVa->OPERATION_MODE.u32 = operationMode.u32;

    /* set to LP + command mode */
    Hi3403SetCmdLpMode();
    modeCfg.u32 = g_mipiTxRegsVa->MODE_CFG.u32;
    modeCfg.bits.cmd_video_mode = 1;
    g_mipiTxRegsVa->MODE_CFG.u32 = modeCfg.u32;
    Hi3403SetClkLaneCfg(0);

    /* phy + controller reset */
    Hi3403ControllerPhyReset();
}
static int MipiTxDrvGetCmdInfo(GetCmdInfoTag *getCmdInfo)
{
    unsigned char *dataBuf = NULL;

    dataBuf = (unsigned char*)OsalMemAlloc(getCmdInfo->getDataSize);
    if (dataBuf == NULL) {
        HDF_LOGE("MipiTxDrvGetCmdInfo: dataBuf is null!");
        return HDF_ERR_MALLOC_FAIL;
    }
    if (MipiTxWaitReadFifoEmpty() != HDF_SUCCESS) {
        HDF_LOGE("MipiTxDrvGetCmdInfo: [MipiTxWaitReadFifoEmpty] fail!");
        goto fail0;
    }
    if (MipiTxSendShortPacket(0, getCmdInfo->dataType, getCmdInfo->dataParam) != HDF_SUCCESS) {
        HDF_LOGE("MipiTxDrvGetCmdInfo: [MipiTxSendShortPacket] fail!");
        goto fail0;
    }
    if (MipiTxGetReadFifoData(getCmdInfo->getDataSize, dataBuf) != HDF_SUCCESS) {
        /* fail will block mipi data lane, so need reset */
        MipiTxReset();
        HDF_LOGE("MipiTxDrvGetCmdInfo: [MipiTxGetReadFifoData] fail!");
        goto fail0;
    }
    if (access_ok(
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
        VERIFY_WRITE,
#endif
        getCmdInfo->getData, getCmdInfo->getDataSize)) { /* user space */
        if (copy_to_user(getCmdInfo->getData, dataBuf, getCmdInfo->getDataSize) != 0) {
            HDF_LOGE("MipiTxDrvGetCmdInfo: copy_to_user fail");
            goto fail0;
        }
    } else { /* kernel space */
        if (memcpy_s(getCmdInfo->getData, getCmdInfo->getDataSize, dataBuf, getCmdInfo->getDataSize) != EOK) {
            HDF_LOGE("MipiTxDrvGetCmdInfo: memcpy_s fail");
            goto fail0;
        }
    }
    OsalMemFree(dataBuf);
    dataBuf = NULL;
    return HDF_SUCCESS;

fail0:
    OsalMemFree(dataBuf);
    dataBuf = NULL;
    return HDF_FAILURE;
}
static void MipiTxDrvEnableInput(const OutPutModeTag outputMode)
{
    U_OPERATION_MODE opMode;

    if ((outputMode == OUTPUT_MODE_DSI_VIDEO) || (outputMode == OUTPUT_MODE_CSI)) {
        U_MODE_CFG modeCfg;
        modeCfg.u32 = g_mipiTxRegsVa->MODE_CFG.u32;
        modeCfg.bits.cmd_video_mode = 0;
        g_mipiTxRegsVa->MODE_CFG.u32 = modeCfg.u32;
    }

    if (outputMode == OUTPUT_MODE_DSI_CMD)
        Hi3403SetCmdHsMode();

    OsalUDelay(1);
    Hi3403SetClkLaneCfg(1); /* HS clock enable */
    OsalUDelay(1);

    /* enable input */
    opMode.u32 = g_mipiTxRegsVa->OPERATION_MODE.u32;
    opMode.bits.mem_ck_en = 1;
    opMode.bits.input_en = 1;
    opMode.bits.hss_abnormal_rst = 1;
    opMode.bits.read_empty_vsync_en = 1;
    g_mipiTxRegsVa->OPERATION_MODE.u32 = opMode.u32;

    /* reset controller + phy */
    Hi3403ControllerPhyReset();
}

static void MipiTxDrvDisableInput(void)
{
    U_OPERATION_MODE operationMode;
    U_MODE_CFG modeCfg;

    /* disable input */
    operationMode.u32 = g_mipiTxRegsVa->OPERATION_MODE.u32;
    operationMode.bits.input_en = 0;
    g_mipiTxRegsVa->OPERATION_MODE.u32 = operationMode.u32;

    /* set to LP + command mode */
    Hi3403SetCmdLpMode();
    modeCfg.u32 = g_mipiTxRegsVa->MODE_CFG.u32;
    modeCfg.bits.cmd_video_mode = 1;
    g_mipiTxRegsVa->MODE_CFG.u32 = modeCfg.u32;
    Hi3403SetClkLaneCfg(0);

    /* phy + controller reset */
    Hi3403ControllerPhyReset();
}
/* ====== register / HW init ====== */

static int MipiTxDrvRegInit(void)
{
    if (!g_mipiTxRegsVa) {
        g_mipiTxRegsVa = (MipiTxRegsTypeTag *)OsalIoRemap(MIPI_TX_REGS_ADDR, (unsigned int)MIPI_TX_REGS_SIZE);
        if (g_mipiTxRegsVa == NULL) {
            HDF_LOGE("MipiTxDrvRegInit: remap mipi_tx reg addr fail!");
            return HDF_FAILURE;
        }
        g_regMapFlag = 1;
    }

    return HDF_SUCCESS;
}

static void MipiTxDrvRegExit(void)
{
    if (g_regMapFlag == 1) {
        if (g_mipiTxRegsVa != NULL) {
            OsalIoUnmap((void *)g_mipiTxRegsVa);
            g_mipiTxRegsVa = NULL;
        }
        g_regMapFlag = 0;
    }
}

static void MipiTxDrvHwInit(int smooth)
{
    unsigned long *mipiTxCrgAddr;

    mipiTxCrgAddr = (unsigned long *)OsalIoRemap(MIPI_TX_CRG, (unsigned long)0x4);
    if (mipiTxCrgAddr == NULL) {
        HDF_LOGE("%s: OsalIoRemap CRG failed", __func__);
        return;
    }
    /* mipi_tx gate clk enable */
    WriteReg32(mipiTxCrgAddr, 1, 0x1);
    /* reset */
    if (smooth == 0) {
        WriteReg32(mipiTxCrgAddr, 1 << 1, 0x1 << 1);
    }
    /* unreset */
    WriteReg32(mipiTxCrgAddr, 0 << 1, 0x1 << 1);
    /* ref clk */
    WriteReg32(mipiTxCrgAddr, 1 << 2, 0x1 << 2); /* 2 clk bit */
    OsalIoUnmap((void *)mipiTxCrgAddr);
}

static int MipiTxDrvInit(int smooth)
{
    int32_t ret;

    ret = MipiTxDrvRegInit();
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiTxDrvInit: MipiTxDrvRegInit fail!");
        return HDF_FAILURE;
    }
    MipiTxDrvHwInit(smooth);
    return HDF_SUCCESS;
}

static void MipiTxDrvExit(void)
{
    MipiTxDrvRegExit();
}

/* ====== combo dev cfg ====== */

static ComboDevCfgTag *GetDevCfg(struct MipiDsiCntlr *cntlr)
{
    static ComboDevCfgTag dev;
    int i;

    if (cntlr == NULL) {
        HDF_LOGE("GetDevCfg: cntlr is null!");
        return NULL;
    }
    dev.devno = cntlr->devNo;
    dev.outputMode = (OutPutModeTag)cntlr->cfg.mode;
    dev.videoMode = (VideoModeTag)cntlr->cfg.burstMode;
    dev.outputFormat = (OutputFormatTag)cntlr->cfg.format;
    dev.syncInfo.vidPktSize = cntlr->cfg.timing.xPixels;
    dev.syncInfo.vidHsaPixels = cntlr->cfg.timing.hsaPixels;
    dev.syncInfo.vidHbpPixels = cntlr->cfg.timing.hbpPixels;
    dev.syncInfo.vidHlinePixels = cntlr->cfg.timing.hlinePixels;
    dev.syncInfo.vidVsaLines = cntlr->cfg.timing.vsaLines;
    dev.syncInfo.vidVbpLines = cntlr->cfg.timing.vbpLines;
    dev.syncInfo.vidVfpLines = cntlr->cfg.timing.vfpLines;
    dev.syncInfo.vidActiveLines = cntlr->cfg.timing.ylines;
    dev.syncInfo.edpiCmdSize = cntlr->cfg.timing.edpiCmdSize;
    dev.phyDataRate = cntlr->cfg.phyDataRate;
    dev.pixelClk = cntlr->cfg.pixelClk;
    for (i = 0; i < LANE_MAX_NUM; i++) {
        dev.laneId[i] = -1;   /* -1 : not use */
    }
    for (i = 0; i < cntlr->cfg.lane; i++) {
        dev.laneId[i] = i;
    }
    return &dev;
}

static int MipiTxCheckCombDevCfg(const ComboDevCfgTag *devCfg)
{
    int i;
    int validLaneId[LANE_MAX_NUM] = {0, 1, 2, 3};

    if (g_enHsMode) {
        HDF_LOGE("MipiTxCheckCombDevCfg: mipi_tx dev has enable!");
        return HDF_FAILURE;
    }
    if (devCfg->devno != 0) {
        HDF_LOGE("MipiTxCheckCombDevCfg: mipi_tx dev devno err!");
        return HDF_ERR_INVALID_PARAM;
    }
    for (i = 0; i < LANE_MAX_NUM; i++) {
        if ((devCfg->laneId[i] != validLaneId[i]) && (devCfg->laneId[i] != MIPI_TX_DISABLE_LANE_ID)) {
            HDF_LOGE("MipiTxCheckCombDevCfg: mipi_tx dev laneId %d err!", devCfg->laneId[i]);
            return HDF_ERR_INVALID_PARAM;
        }
    }
    if ((devCfg->outputMode != OUTPUT_MODE_CSI) && (devCfg->outputMode != OUTPUT_MODE_DSI_VIDEO) &&
        (devCfg->outputMode != OUTPUT_MODE_DSI_CMD)) {
        HDF_LOGE("MipiTxCheckCombDevCfg: mipi_tx dev outputMode %d err!", devCfg->outputMode);
        return HDF_ERR_INVALID_PARAM;
    }
    if ((devCfg->videoMode != BURST_MODE) && (devCfg->videoMode != NON_BURST_MODE_SYNC_PULSES) &&
        (devCfg->videoMode != NON_BURST_MODE_SYNC_EVENTS)) {
        HDF_LOGE("MipiTxCheckCombDevCfg: mipi_tx dev videoMode %d err!", devCfg->videoMode);
        return HDF_ERR_INVALID_PARAM;
    }
    if ((devCfg->outputFormat != OUT_FORMAT_RGB_16_BIT) && (devCfg->outputFormat != OUT_FORMAT_RGB_18_BIT) &&
        (devCfg->outputFormat != OUT_FORMAT_RGB_24_BIT) && (devCfg->outputFormat !=
        OUT_FORMAT_YUV420_8_BIT_NORMAL) && (devCfg->outputFormat != OUT_FORMAT_YUV420_8_BIT_LEGACY) &&
        (devCfg->outputFormat != OUT_FORMAT_YUV422_8_BIT)) {
        HDF_LOGE("MipiTxCheckCombDevCfg: mipi_tx dev outputFormat %d err!", devCfg->outputFormat);
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

static int MipiTxSetComboDevCfg(const ComboDevCfgTag *devCfg)
{
    int32_t ret;

    ret = MipiTxCheckCombDevCfg(devCfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiTxSetComboDevCfg: check combo_dev config fail!");
        return ret;
    }

    /* Lazy HW init: defer CRG clock+reset to first use */
    if (!g_regMapFlag || g_regMapFlag == 1) {
        MipiTxDrvHwInit(0);
        g_regMapFlag = 2;
    }

    /* hi3403 HAL flow: disable first, then controller_cfg + phy_cfg */
    MipiTxDrvDisableInput();
    OsalUDelay(10);

    MipiTxDrvSetControllerCfg(devCfg);
    MipiTxDrvSetPhyCfg(devCfg);
    g_enCfg = true;
    return ret;
}

/* ====== HDF driver ops ====== */

static int32_t Hi3403SetCntlrCfg(struct MipiDsiCntlr *cntlr)
{
    ComboDevCfgTag *dev = GetDevCfg(cntlr);

    if (dev == NULL) {
        HDF_LOGE("Hi3403SetCntlrCfg: dev is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    return MipiTxSetComboDevCfg(dev);
}

static int MipiTxCheckSetCmdInfo(const CmdInfoTag *cmdInfo)
{
    if (g_enHsMode) {
        HDF_LOGE("MipiTxCheckSetCmdInfo: mipi_tx dev has enable!");
        return HDF_FAILURE;
    }

    if (!g_enCfg) {
        HDF_LOGE("MipiTxCheckSetCmdInfo: mipi_tx dev has not config!");
        return HDF_FAILURE;
    }
    if (cmdInfo->devno != 0) {
        HDF_LOGE("MipiTxCheckSetCmdInfo: mipi_tx devno %d err!", cmdInfo->devno);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cmdInfo->cmd != NULL) {
        if (cmdInfo->cmdSize > MIPI_TX_SET_DATA_SIZE) {
            HDF_LOGE("MipiTxCheckSetCmdInfo: mipi_tx dev cmd_size %d err!", cmdInfo->cmdSize);
            return HDF_ERR_INVALID_PARAM;
        }
    }
    return HDF_SUCCESS;
}

static int MipiTxSetCmd(const CmdInfoTag *cmdInfo)
{
    int32_t ret;
    if (cmdInfo == NULL) {
        HDF_LOGE("MipiTxSetCmd: cmdInfo is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = MipiTxCheckSetCmdInfo(cmdInfo);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiTxSetCmd: mipi_tx check combo_dev config fail!");
        return ret;
    }
    return MipiTxDrvSetCmdInfo(cmdInfo);
}

static int32_t Hi3403SetCmd(struct MipiDsiCntlr *cntlr, struct DsiCmdDesc *cmd)
{
    CmdInfoTag cmdInfo;

    (void)cntlr;
    if (cmd == NULL) {
        HDF_LOGE("Hi3403SetCmd: cmd is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    cmdInfo.devno = 0;
    if (cmd->dataLen > 2) {                     /* 2: use long data type */
        cmdInfo.cmdSize = cmd->dataLen;
        cmdInfo.dataType = cmd->dataType;       /* 0x29: long data type */
        cmdInfo.cmd = cmd->payload;
    } else if (cmd->dataLen == 2) {             /* 2: use short data type */
        uint16_t tmp = cmd->payload[1];         /* 3: payload */
        tmp = (tmp & 0x00ff) << 8;              /* 0x00ff , 8: payload to high */
        tmp = 0xff00 & tmp;
        tmp = tmp | cmd->payload[0];            /* 2: reg addr */
        cmdInfo.cmdSize = tmp;
        cmdInfo.dataType = cmd->dataType;       /* 0x23: short data type */
        cmdInfo.cmd = NULL;
    } else if (cmd->dataLen == 1) {
        cmdInfo.cmdSize = cmd->payload[0];      /* 2: reg addr */
        cmdInfo.dataType = cmd->dataType;       /* 0x05: short data type */
        cmdInfo.cmd = NULL;
    } else {
        HDF_LOGE("Hi3403SetCmd: dataLen error!");
        return HDF_ERR_INVALID_PARAM;
    }
    return MipiTxSetCmd(&cmdInfo);
}

static int MipiTxCheckGetCmdInfo(const GetCmdInfoTag *getCmdInfo)
{
    if (g_enHsMode) {
        HDF_LOGE("MipiTxCheckGetCmdInfo: mipi_tx dev has enable!");
        return HDF_FAILURE;
    }

    if (!g_enCfg) {
        HDF_LOGE("MipiTxCheckGetCmdInfo: mipi_tx dev has not config!");
        return HDF_FAILURE;
    }
    if (getCmdInfo->devno != 0) {
        HDF_LOGE("MipiTxCheckGetCmdInfo: mipi_tx dev devno %u err!", getCmdInfo->devno);
        return HDF_ERR_INVALID_PARAM;
    }
    if ((getCmdInfo->getDataSize == 0) || (getCmdInfo->getDataSize > MIPI_TX_GET_DATA_SIZE)) {
        HDF_LOGE("MipiTxCheckGetCmdInfo: mipi_tx dev getDataSize %hu err!", getCmdInfo->getDataSize);
        return HDF_ERR_INVALID_PARAM;
    }
    if (getCmdInfo->getData == NULL) {
        HDF_LOGE("MipiTxCheckGetCmdInfo: mipi_tx dev getData is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    return HDF_SUCCESS;
}

static int MipiTxGetCmd(GetCmdInfoTag *getCmdInfo)
{
    int32_t ret;

    ret = MipiTxCheckGetCmdInfo(getCmdInfo);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiTxGetCmd: [MipiTxCheckGetCmdInfo] fail!");
        return ret;
    }
    return MipiTxDrvGetCmdInfo(getCmdInfo);
}

static int32_t Hi3403GetCmd(struct MipiDsiCntlr *cntlr, struct DsiCmdDesc *cmd, uint32_t readLen, uint8_t *out)
{
    GetCmdInfoTag cmdInfo;

    (void)cntlr;
    if (cmd == NULL || out == NULL) {
        HDF_LOGE("Hi3403GetCmd: cmd or out is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    cmdInfo.devno = 0;
    cmdInfo.dataType = cmd->dataType;
    cmdInfo.dataParam = cmd->payload[0];
    cmdInfo.getDataSize = readLen;
    cmdInfo.getData = out;
    return MipiTxGetCmd(&cmdInfo);
}

static void Hi3403ToLp(struct MipiDsiCntlr *cntlr)
{
    (void)cntlr;
    MipiTxDrvDisableInput();
}

static void Hi3403ToHs(struct MipiDsiCntlr *cntlr)
{
    ComboDevCfgTag *dev = GetDevCfg(cntlr);

    if (dev == NULL) {
        HDF_LOGE("Hi3403ToHs: dev is null!");
        return;
    }
    MipiTxDrvEnableInput(dev->outputMode);
}

/* ====== HDF driver entry ====== */

static struct MipiDsiCntlr g_mipiTx = {
    .devNo = 0
};

static struct MipiDsiCntlrMethod g_method = {
    .setCntlrCfg = Hi3403SetCntlrCfg,
    .setCmd = Hi3403SetCmd,
    .getCmd = Hi3403GetCmd,
    .toHs = Hi3403ToHs,
    .toLp = Hi3403ToLp
};

static int32_t Hi3403MipiTxInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    g_mipiTx.priv = NULL;
    g_mipiTx.ops = &g_method;
    ret = MipiDsiRegisterCntlr(&g_mipiTx, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Hi3403MipiTxInit: [MipiDsiRegisterCntlr] fail!");
        return ret;
    }

    /* hi3403: defer HwInit (CRG clock+reset) to first SetComboDevCfg.
       Early reset may conflict with GPU/VO during kernel boot. */
    ret = MipiTxDrvRegInit();
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Hi3403MipiTxInit: [MipiTxDrvRegInit] fail.");
        return ret;
    }
    HDF_LOGI("Hi3403MipiTxInit: load mipi tx driver successfully!");

    return ret;
}

static void Hi3403MipiTxRelease(struct HdfDeviceObject *device)
{
    struct MipiDsiCntlr *cntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("Hi3403MipiTxRelease: device is null!");
        return;
    }
    cntlr = MipiDsiCntlrFromDevice(device);
    if (cntlr == NULL) {
        HDF_LOGE("Hi3403MipiTxRelease: cntlr is null!");
        return;
    }

    MipiTxDrvExit();
    MipiDsiUnregisterCntlr(&g_mipiTx);
    g_mipiTx.priv = NULL;
    HDF_LOGI("Hi3403MipiTxRelease: unload mipi tx driver successfully!");
}

struct HdfDriverEntry g_mipiTxDriverEntry = {
    .moduleVersion = 1,
    .Init = Hi3403MipiTxInit,
    .Release = Hi3403MipiTxRelease,
    .moduleName = "HDF_MIPI_TX",
};
HDF_INIT(g_mipiTxDriverEntry);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

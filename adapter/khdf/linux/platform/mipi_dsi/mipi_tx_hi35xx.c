/*
 * mipi_tx_hi35xx.c
 *
 * hi35xx mipi_tx driver implement
 *
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "mipi_tx_hi35xx.h"
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "hdf_core_log.h"
#include "securec.h"
#include "osal_time.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "mipi_dsi_define.h"
#include "mipi_dsi_core.h"
#include "mipi_tx_reg.h"
#include "mipi_tx_dev.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HDF_LOG_TAG mipi_tx_hi35xx
#define INT_MAX_VALUE 0x7fffffff

volatile  MipiTxRegsTypeTag *g_mipiTxRegsVa = NULL;
unsigned int g_mipiTxIrqNum = MIPI_TX_IRQ;
unsigned int g_actualPhyDataRate;
static unsigned int g_regMapFlag;
/**
 * @brief g_enCfg is the flag that the controller parameters have been set, which is independent of the high
 * and low speed modes. The program design requires setting parameters before operating the controller,
 * otherwise it will directly return to failure.
 */
static bool g_enCfg = false;
/**
 * @brief g_enHsMode is the high-speed mode flag. High speed is true, otherwise it is false.
 */
static bool g_enHsMode = false;

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
    dsb();
}

static void OsalDmb(void)
{
    dmb();
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

static unsigned char MipiTxDrvGetPhyPllSet0(unsigned int phyDataRate)
{
    unsigned char pllSet0;

    /* to get pllSet0, the parameters come from algorithm */
    if (phyDataRate > 750) {          /* 750: mipi clk */
        pllSet0 = 0x0;
    } else if (phyDataRate > 375) {   /* 375: mipi clk */
        pllSet0 = 0x8;
    } else if (phyDataRate > 188) {   /* 188: mipi clk */
        pllSet0 = 0xa;
    } else if (phyDataRate > 94) {    /* 94: mipi clk */
        pllSet0 = 0xc;
    } else {
        pllSet0 = 0xe;
    }
    return pllSet0;
}

static void MipiTxDrvGetPhyPllSet1Set5(unsigned int phyDataRate,
    unsigned char pllSet0,
    unsigned char *pllSet1,
    unsigned char *pllSet5)
{
    int dataRateClk;
    int pllRef;
    int64_t int_multiplication;

    dataRateClk = (int)(phyDataRate + MIPI_TX_REF_CLK - 1) / MIPI_TX_REF_CLK;

    /* to get pllSet1 and pllSet5, the parameters come from algorithm */
    if (pllSet0 / 2 == 4) {           /* 2: pll, 4: pll sel */
        pllRef = 2;                   /* 2: pll set */
    } else if (pllSet0 / 2 == 5) {    /* 2: pll, 5: pllSet5 */
        pllRef = 4;                   /* 4: pll set */
    } else if (pllSet0 / 2 == 6) {    /* 2: pll, 6: pll sel */
        pllRef = 8;                   /* 8: pll set */
    } else if (pllSet0 / 2 == 7) {    /* 2: pll, 7: pll sel */
        pllRef = 16;                  /* 16: pll set */
    } else {
        pllRef = 1;
    }
    int_multiplication = (int64_t)(dataRateClk * pllRef);
    if (int_multiplication > INT_MAX_VALUE) {
        HDF_LOGE("MipiTxDrvGetPhyPllSet1Set5: exceeds the maximum value of type int32_t!");
        return;
    }
    if (((dataRateClk * pllRef) % 2) != 0) { /* 2: pll */
        *pllSet1 = 0x10;
        *pllSet5 = (unsigned char)((dataRateClk * pllRef - 1) / 2); /* 2: pllRef sel */
    } else {
        *pllSet1 = 0x20;
        *pllSet5 = (unsigned char)(dataRateClk * pllRef / 2 - 1);   /* 2: pllRef sel */
    }

    return;
}

static void MipiTxDrvSetPhyPllSetX(unsigned int phyDataRate)
{
    unsigned char pllSet0;
    unsigned char pllSet1;
    unsigned char pllSet2;
#ifdef HI_FPGA
    unsigned char pllSet3;
#endif
    unsigned char pllSet4;
    unsigned char pllSet5;

    /* pllSet0 */
    pllSet0 = MipiTxDrvGetPhyPllSet0(phyDataRate);
    SetPhyReg(PLL_SET0, pllSet0);
    /* pllSet1 */
    MipiTxDrvGetPhyPllSet1Set5(phyDataRate, pllSet0, &pllSet1, &pllSet5);
    SetPhyReg(PLL_SET1, pllSet1);
    /* pllSet2 */
    pllSet2 = 0x2;
    SetPhyReg(PLL_SET2, pllSet2);
    /* pllSet4 */
    pllSet4 = 0x0;
    SetPhyReg(PLL_SET4, pllSet4);

#ifdef HI_FPGA
    pllSet3 = 0x1;
    SetPhyReg(PLL_SET3, pllSet3);
#endif
    /* pllSet5 */
    SetPhyReg(PLL_SET5, pllSet5);

#ifdef MIPI_TX_DEBUG
    HDF_LOGI("MipiTxDrvSetPhyPllSetX: \n==========phy pll info=======");
    HDF_LOGI("pllSet0(0x14): 0x%x", pllSet0);
    HDF_LOGI("pllSet1(0x15): 0x%x", pllSet1);
    HDF_LOGI("pllSet2(0x16): 0x%x", pllSet2);
#ifdef HI_FPGA
    HDF_LOGI("pllSet3(0x17): 0x%x", pllSet3);
#endif
    HDF_LOGI("pllSet4(0x1e): 0x%x", pllSet4);
    HDF_LOGI("=========================\n");
#endif
}

static void MipiTxDrvGetPhyClkPrepare(unsigned char *clkPrepare)
{
    unsigned char temp0;
    unsigned char temp1;

    temp0 = (unsigned char)((g_actualPhyDataRate * TCLK_PREPARE + ROUNDUP_VALUE) / INNER_PEROID - 1 +
            ((g_actualPhyDataRate * PREPARE_COMPENSATE + ROUNDUP_VALUE) / INNER_PEROID) -
            ((((g_actualPhyDataRate * TCLK_PREPARE + ROUNDUP_VALUE) / INNER_PEROID +
            ((g_actualPhyDataRate * PREPARE_COMPENSATE + ROUNDUP_VALUE) / INNER_PEROID)) * INNER_PEROID -
            PREPARE_COMPENSATE * g_actualPhyDataRate - TCLK_PREPARE * g_actualPhyDataRate) / INNER_PEROID));
    if (temp0 > 0) { /* 0 is the minimum */
        temp1 = temp0;
    } else {
        temp1 = 0; /* 0 is the minimum */
    }

    if (((temp1 + 1) * INNER_PEROID - PREPARE_COMPENSATE * g_actualPhyDataRate) > /* temp + 1 is next level period */
        94 * g_actualPhyDataRate) { /* 94 is the  maximum in mipi protocol */
        if (temp0 > 0) {
            *clkPrepare = temp0 - 1;
        } else {
            *clkPrepare = 255; /* set 255 will easy to found mistake */
            HDF_LOGE("MipiTxDrvGetPhyClkPrepare: err when calc phy timing!");
        }
    } else {
        if (temp0 > 0) { /* 0 is the minimum */
            *clkPrepare = temp0;
        } else {
            *clkPrepare = 0; /* 0 is the minimum */
        }
    }
}

static void MipiTxDrvGetPhyDataPrepare(unsigned char *dataPrepare)
{
    unsigned char temp0;
    unsigned char temp1;

    /* DATA_THS_PREPARE */
    temp0 = (unsigned char)((g_actualPhyDataRate * THS_PREPARE + ROUNDUP_VALUE) / INNER_PEROID - 1 +
            ((g_actualPhyDataRate * PREPARE_COMPENSATE + ROUNDUP_VALUE) / INNER_PEROID) -
            ((((g_actualPhyDataRate * THS_PREPARE + ROUNDUP_VALUE) / INNER_PEROID +
            ((g_actualPhyDataRate * PREPARE_COMPENSATE + ROUNDUP_VALUE) / INNER_PEROID)) * INNER_PEROID -
            PREPARE_COMPENSATE * g_actualPhyDataRate - THS_PREPARE * g_actualPhyDataRate) / INNER_PEROID));
    if (temp0 > 0) {
        temp1 = temp0;
    } else {
        temp1 = 0;
    }

    if ((g_actualPhyDataRate > 105) && /* bigger than 105 */
        (((temp1 + 1) * INNER_PEROID - PREPARE_COMPENSATE * g_actualPhyDataRate) >
        (85 * g_actualPhyDataRate + 6 * 1000))) { /* 85 + 6 * 1000 is the  maximum in mipi protocol */
        if (temp0 > 0) {
            *dataPrepare = temp0 - 1;
        } else {
            *dataPrepare = 255; /* set 255 will easy to found mistake */
            HDF_LOGE("MipiTxDrvGetPhyDataPrepare: err when calc phy timing!");
        }
    } else {
        if (temp0 > 0) {
            *dataPrepare = temp0;
        } else {
            *dataPrepare = 0;
        }
    }
}

/* get global operation timing parameters. */
static void MipiTxDrvGetPhyTimingParam(MipiTxPhyTimingParamTag *tp)
{
    /* DATA0~3 TPRE-DELAY */
    /* 1: compensate */
    tp->dataTpreDelay = (unsigned char)((g_actualPhyDataRate * TPRE_DELAY + ROUNDUP_VALUE) / INNER_PEROID - 1);
    /* CLK_TLPX */
    tp->clkTlpx = (unsigned char)((g_actualPhyDataRate * TLPX + ROUNDUP_VALUE) /
        INNER_PEROID - 1); /* 1 is compensate */
    /* CLK_TCLK_PREPARE */
    MipiTxDrvGetPhyClkPrepare(&tp->clkTclkPrepare);
    /* CLK_TCLK_ZERO */
    if ((g_actualPhyDataRate * TCLK_ZERO + ROUNDUP_VALUE) / INNER_PEROID > 4) {    /* 4 is compensate */
        tp->clkTclkZero = (unsigned char)((g_actualPhyDataRate * TCLK_ZERO + ROUNDUP_VALUE) / INNER_PEROID - 4);
    } else {
        tp->clkTclkZero = 0;       /* 0 is minimum */
    }
    /* CLK_TCLK_TRAIL */
    tp->clkTclkTrail = (unsigned char)((g_actualPhyDataRate * TCLK_TRAIL + ROUNDUP_VALUE) / INNER_PEROID);
    /* DATA_TLPX */
    tp->dataTlpx = (unsigned char)((g_actualPhyDataRate * TLPX + ROUNDUP_VALUE) /
        INNER_PEROID - 1); /* 1 is compensate */
    /* DATA_THS_PREPARE */
    MipiTxDrvGetPhyDataPrepare(&tp->dataThsPrepare);
    /* DATA_THS_ZERO */
    if ((g_actualPhyDataRate * THS_ZERO + ROUNDUP_VALUE) / INNER_PEROID > 4) {      /* 4 is compensate */
        tp->dataThsZero = (unsigned char)((g_actualPhyDataRate * THS_ZERO + ROUNDUP_VALUE) / INNER_PEROID - 4);
    } else {
        tp->dataThsZero = 0;       /* 0 is minimum */
    }
    /* DATA_THS_TRAIL */
    tp->dataThsTrail = (unsigned char)((g_actualPhyDataRate * THS_TRAIL + ROUNDUP_VALUE) /
        INNER_PEROID + 1); /* 1 is compensate */
}

/* set global operation timing parameters. */
static void MipiTxDrvSetPhyTimingParam(const MipiTxPhyTimingParamTag *tp)
{
    /* DATA0~3 TPRE-DELAY */
    SetPhyReg(DATA0_TPRE_DELAY, tp->dataTpreDelay);
    SetPhyReg(DATA1_TPRE_DELAY, tp->dataTpreDelay);
    SetPhyReg(DATA2_TPRE_DELAY, tp->dataTpreDelay);
    SetPhyReg(DATA3_TPRE_DELAY, tp->dataTpreDelay);

    /* CLK_TLPX */
    SetPhyReg(CLK_TLPX, tp->clkTlpx);
    /* CLK_TCLK_PREPARE */
    SetPhyReg(CLK_TCLK_PREPARE, tp->clkTclkPrepare);
    /* CLK_TCLK_ZERO */
    SetPhyReg(CLK_TCLK_ZERO, tp->clkTclkZero);
    /* CLK_TCLK_TRAIL */
    SetPhyReg(CLK_TCLK_TRAIL, tp->clkTclkTrail);

    /*
     * DATA_TLPX
     * DATA_THS_PREPARE
     * DATA_THS_ZERO
     * DATA_THS_TRAIL
     */
    SetPhyReg(DATA0_TLPX, tp->dataTlpx);
    SetPhyReg(DATA0_THS_PREPARE, tp->dataThsPrepare);
    SetPhyReg(DATA0_THS_ZERO, tp->dataThsZero);
    SetPhyReg(DATA0_THS_TRAIL, tp->dataThsTrail);
    SetPhyReg(DATA1_TLPX, tp->dataTlpx);
    SetPhyReg(DATA1_THS_PREPARE, tp->dataThsPrepare);
    SetPhyReg(DATA1_THS_ZERO, tp->dataThsZero);
    SetPhyReg(DATA1_THS_TRAIL, tp->dataThsTrail);
    SetPhyReg(DATA2_TLPX, tp->dataTlpx);
    SetPhyReg(DATA2_THS_PREPARE, tp->dataThsPrepare);
    SetPhyReg(DATA2_THS_ZERO, tp->dataThsZero);
    SetPhyReg(DATA2_THS_TRAIL, tp->dataThsTrail);
    SetPhyReg(DATA3_TLPX, tp->dataTlpx);
    SetPhyReg(DATA3_THS_PREPARE, tp->dataThsPrepare);
    SetPhyReg(DATA3_THS_ZERO, tp->dataThsZero);
    SetPhyReg(DATA3_THS_TRAIL, tp->dataThsTrail);

#ifdef MIPI_TX_DEBUG
    HDF_LOGI("MipiTxDrvSetPhyTimingParam:\n==========phy timing parameters=======");
    HDF_LOGI("data_tpre_delay(0x30/40/50/60): 0x%x", tp->dataTpreDelay);
    HDF_LOGI("clk_tlpx(0x22): 0x%x", tp->clkTlpx);
    HDF_LOGI("clk_tclk_prepare(0x23): 0x%x", tp->clkTclkPrepare);
    HDF_LOGI("clk_tclk_zero(0x24): 0x%x", tp->clkTclkZero);
    HDF_LOGI("clk_tclk_trail(0x25): 0x%x", tp->clkTclkTrail);
    HDF_LOGI("data_tlpx(0x32/42/52/62): 0x%x", tp->dataTlpx);
    HDF_LOGI("data_ths_prepare(0x33/43/53/63): 0x%x", tp->dataThsPrepare);
    HDF_LOGI("data_ths_zero(0x34/44/54/64): 0x%x", tp->dataThsZero);
    HDF_LOGI("data_ths_trail(0x35/45/55/65): 0x%x", tp->dataThsTrail);
    HDF_LOGI("=========================\n");
#endif
}

/*
 * set data lp2hs,hs2lp time
 * set clk lp2hs,hs2lp time
 * unit: hsclk
 */
static void MipiTxDrvSetPhyHsLpSwitchTime(const MipiTxPhyTimingParamTag *tp)
{
    /* data lp2hs,hs2lp time */
    g_mipiTxRegsVa->PHY_TMR_CFG.u32 = ((tp->dataThsTrail - 1) << 16) +  /* 16 set register */
        tp->dataTpreDelay + tp->dataTlpx + tp->dataThsPrepare + tp->dataThsZero + 7; /* 7 from algorithm */
    /* clk lp2hs,hs2lp time */
    g_mipiTxRegsVa->PHY_TMR_LPCLK_CFG.u32 = ((31 + tp->dataThsTrail) << 16) + /* 31 from algorithm, 16 set register */
        tp->clkTlpx + tp->clkTclkPrepare + tp->clkTclkZero + 6; /* 6 from algorithm */
#ifdef MIPI_TX_DEBUG
    HDF_LOGI("MipiTxDrvSetPhyHsLpSwitchTime: PHY_TMR_CFG(0x9C): 0x%x", g_mipiTxRegsVa->PHY_TMR_CFG.u32);
    HDF_LOGI("MipiTxDrvSetPhyHsLpSwitchTime: PHY_TMR_LPCLK_CFG(0x98): 0x%x", g_mipiTxRegsVa->PHY_TMR_LPCLK_CFG.u32);
#endif
}

static void MipiTxDrvSetPhyCfg(const ComboDevCfgTag *cfg)
{
    MipiTxPhyTimingParamTag tp = {0};

    if (cfg == NULL) {
        HDF_LOGE("MipiTxDrvSetPhyCfg: cfg is null!");
        return;
    }

    /* set phy pll parameters setx */
    MipiTxDrvSetPhyPllSetX(cfg->phyDataRate);
    /* get global operation timing parameters */
    MipiTxDrvGetPhyTimingParam(&tp);
    /* set global operation timing parameters */
    MipiTxDrvSetPhyTimingParam(&tp);
    /* set hs switch to lp and lp switch to hs time  */
    MipiTxDrvSetPhyHsLpSwitchTime(&tp);
    /* edpi_cmd_size */
    g_mipiTxRegsVa->EDPI_CMD_SIZE.u32 = 0xF0;
    /* phy enable */
    g_mipiTxRegsVa->PHY_RSTZ.u32 = 0xf;
    if (cfg->outputMode == OUTPUT_MODE_CSI) {
        if (cfg->outputFormat == OUT_FORMAT_YUV420_8_BIT_NORMAL) {
            g_mipiTxRegsVa->DATATYPE0.u32 = 0x10218;
            g_mipiTxRegsVa->CSI_CTRL.u32 = 0x1111;
        } else if (cfg->outputFormat == OUT_FORMAT_YUV422_8_BIT) {
            g_mipiTxRegsVa->DATATYPE0.u32 = 0x1021E;
            g_mipiTxRegsVa->CSI_CTRL.u32 = 0x1111;
        }
    } else {
        if (cfg->outputFormat == OUT_FORMAT_RGB_16_BIT) {
            g_mipiTxRegsVa->DATATYPE0.u32 = 0x111213D;
            g_mipiTxRegsVa->CSI_CTRL.u32 = 0x10100;
        } else if (cfg->outputFormat == OUT_FORMAT_RGB_18_BIT) {
            g_mipiTxRegsVa->DATATYPE0.u32 = 0x111213D;
            g_mipiTxRegsVa->CSI_CTRL.u32 = 0x10100;
        } else if (cfg->outputFormat == OUT_FORMAT_RGB_24_BIT) {
            g_mipiTxRegsVa->DATATYPE0.u32 = 0x111213D;
            g_mipiTxRegsVa->CSI_CTRL.u32 = 0x10100;
        }
    }
    g_mipiTxRegsVa->PHY_RSTZ.u32 = 0XF;
    OsalMSleep(1);
    g_mipiTxRegsVa->LPCLK_CTRL.u32 = 0x0;
}

void MipiTxDrvGetDevStatus(MipiTxDevPhyTag *phyCtx)
{
    if (phyCtx == NULL) {
        HDF_LOGE("MipiTxDrvGetDevStatus: phyCtx is null!");
        return;
    }
    phyCtx->hactDet = g_mipiTxRegsVa->HORI0_DET.bits.hact_det;
    phyCtx->hallDet = g_mipiTxRegsVa->HORI0_DET.bits.hline_det;
    phyCtx->hbpDet  = g_mipiTxRegsVa->HORI1_DET.bits.hbp_det;
    phyCtx->hsaDet  = g_mipiTxRegsVa->HORI1_DET.bits.hsa_det;
    phyCtx->vactDet = g_mipiTxRegsVa->VERT_DET.bits.vact_det;
    phyCtx->vallDet = g_mipiTxRegsVa->VERT_DET.bits.vall_det;
    phyCtx->vsaDet  = g_mipiTxRegsVa->VSA_DET.bits.vsa_det;
}

static void SetOutputFormat(const ComboDevCfgTag *cfg)
{
    int colorCoding = 0;

    if (cfg->outputMode == OUTPUT_MODE_CSI) {
        if (cfg->outputFormat == OUT_FORMAT_YUV420_8_BIT_NORMAL) {
            colorCoding = 0xd;
        } else if (cfg->outputFormat == OUT_FORMAT_YUV422_8_BIT) {
            colorCoding = 0x1;
        }
    } else {
        if (cfg->outputFormat == OUT_FORMAT_RGB_16_BIT) {
            colorCoding = 0x0;
        } else if (cfg->outputFormat == OUT_FORMAT_RGB_18_BIT) {
            colorCoding = 0x3;
        } else if (cfg->outputFormat == OUT_FORMAT_RGB_24_BIT) {
            colorCoding = 0x5;
        }
    }
    g_mipiTxRegsVa->COLOR_CODING.u32 = (uint32_t)colorCoding;
#ifdef MIPI_TX_DEBUG
    HDF_LOGI("SetOutputFormat: set output format: 0x%x", colorCoding);
#endif
}

static void SetVideoModeCfg(const ComboDevCfgTag *cfg)
{
    int videoMode;

    if (cfg->videoMode == NON_BURST_MODE_SYNC_PULSES) {
        videoMode = 0;
    } else if (cfg->videoMode == NON_BURST_MODE_SYNC_EVENTS) {
        videoMode = 1;
    } else {
        videoMode = 2; /* 2 register value */
    }
    if ((cfg->outputMode == OUTPUT_MODE_CSI) || (cfg->outputMode == OUTPUT_MODE_DSI_CMD)) {
        videoMode = 2; /* 2 register value */
    }
    g_mipiTxRegsVa->VID_MODE_CFG.u32 = 0x3f00 + videoMode;
}

static void SetTimingConfig(const ComboDevCfgTag *cfg)
{
    unsigned int hsa;
    unsigned int hbp;
    unsigned int hline;

    if (cfg->pixelClk == 0) {
        HDF_LOGE("SetTimingConfig: cfg->pixelClk is 0, illegal!");
        return;
    }
    /* 125 from algorithm */
    hsa = g_actualPhyDataRate * cfg->syncInfo.vidHsaPixels * 125 / cfg->pixelClk;
    /* 125 from algorithm */
    hbp = g_actualPhyDataRate * cfg->syncInfo.vidHbpPixels * 125 / cfg->pixelClk;
    /* 125 from algorithm */
    hline = g_actualPhyDataRate * cfg->syncInfo.vidHlinePixels * 125 / cfg->pixelClk;
    g_mipiTxRegsVa->VID_HSA_TIME.u32 = hsa;
    g_mipiTxRegsVa->VID_HBP_TIME.u32 = hbp;
    g_mipiTxRegsVa->VID_HLINE_TIME.u32 = hline;
    g_mipiTxRegsVa->VID_VSA_LINES.u32 = cfg->syncInfo.vidVsaLines;
    g_mipiTxRegsVa->VID_VBP_LINES.u32 = cfg->syncInfo.vidVbpLines;
    g_mipiTxRegsVa->VID_VFP_LINES.u32 = cfg->syncInfo.vidVfpLines;
    g_mipiTxRegsVa->VID_VACTIVE_LINES.u32 = cfg->syncInfo.vidActiveLines;
#ifdef MIPI_TX_DEBUG
    HDF_LOGI("SetTimingConfig:\n==========Set Timing Config=======");
    HDF_LOGI("VID_HSA_TIME(0x48): 0x%x", hsa);
    HDF_LOGI("VID_HBP_TIME(0x4c): 0x%x", hbp);
    HDF_LOGI("VID_HLINE_TIME(0x50): 0x%x", hline);
    HDF_LOGI("VID_VSA_LINES(0x54): 0x%x", cfg->syncInfo.vidVsaLines);
    HDF_LOGI("VID_VBP_LINES(0x58): 0x%x", cfg->syncInfo.vidVbpLines);
    HDF_LOGI("VID_VFP_LINES(0x5c): 0x%x", cfg->syncInfo.vidVfpLines);
    HDF_LOGI("VID_VACTIVE_LINES(0x60): 0x%x", cfg->syncInfo.vidActiveLines);
    HDF_LOGI("=========================\n");
#endif
}

static void SetLaneConfig(const short laneId[], int len)
{
    uint32_t num = 0;
    int i;

    for (i = 0; i < len; i++) {
        if (laneId[i] != -1) {
            num++;
        }
    }
    g_mipiTxRegsVa->PHY_IF_CFG.u32 = (unsigned int)(num - 1);
}

static void MipiTxDrvSetClkMgrCfg(void)
{
    if (g_actualPhyDataRate / 160 < 2) { /* 160 cal div, should not smaller than 2 */
        g_mipiTxRegsVa->CLKMGR_CFG.u32 = 0x102;
    } else {
        g_mipiTxRegsVa->CLKMGR_CFG.u32 = 0x100 + (g_actualPhyDataRate + 159) / 160; /* 159 160 cal div */
    }
}

static void MipiTxDrvSetControllerCfg(const ComboDevCfgTag *cfg)
{
    if (cfg == NULL) {
        HDF_LOGE("MipiTxDrvSetControllerCfg: cfg is null!");
        return;
    }
    /* disable input */
    g_mipiTxRegsVa->OPERATION_MODE.u32 = 0x0;
    /* vc_id */
    g_mipiTxRegsVa->VCID.u32 = 0x0;
    /* output format,color coding */
    SetOutputFormat(cfg);
    /* txescclk,timeout */
    g_actualPhyDataRate = ((cfg->phyDataRate + MIPI_TX_REF_CLK - 1) / MIPI_TX_REF_CLK) * MIPI_TX_REF_CLK;
    MipiTxDrvSetClkMgrCfg();
    /* cmd transmission mode */
    g_mipiTxRegsVa->CMD_MODE_CFG.u32 = 0xffffff00;
    /* crc,ecc,eotp tran */
    g_mipiTxRegsVa->PCKHDL_CFG.u32 = 0x1c;
    /* gen_vcid_rx */
    g_mipiTxRegsVa->GEN_VCID.u32 = 0x0;
    /* mode config */
    g_mipiTxRegsVa->MODE_CFG.u32 = 0x1;
    /* video mode cfg */
    SetVideoModeCfg(cfg);
    if ((cfg->outputMode == OUTPUT_MODE_DSI_VIDEO) || (cfg->outputMode == OUTPUT_MODE_CSI)) {
        g_mipiTxRegsVa->VID_PKT_SIZE.u32 = cfg->syncInfo.vidPktSize;
    } else {
        g_mipiTxRegsVa->EDPI_CMD_SIZE.u32 = cfg->syncInfo.edpiCmdSize;
    }
    /* num_chunks/null_size */
    g_mipiTxRegsVa->VID_NUM_CHUNKS.u32 = 0x0;
    g_mipiTxRegsVa->VID_NULL_SIZE.u32 = 0x0;
    /* timing config */
    SetTimingConfig(cfg);
    /* invact,outvact time */
    g_mipiTxRegsVa->LP_CMD_TIM.u32 = 0x0;
    g_mipiTxRegsVa->PHY_TMR_CFG.u32 = 0x9002D;
    g_mipiTxRegsVa->PHY_TMR_LPCLK_CFG.u32 = 0x29002E;
    g_mipiTxRegsVa->EDPI_CMD_SIZE.u32 = 0xF0;
    /* lp_wr_to_cnt */
    g_mipiTxRegsVa->LP_WR_TO_CNT.u32 = 0x0;
    /* bta_to_cnt */
    g_mipiTxRegsVa->BTA_TO_CNT.u32 = 0x0;
    /* lanes */
    SetLaneConfig(cfg->laneId, LANE_MAX_NUM);
    /* phy_tx requlpsclk */
    g_mipiTxRegsVa->PHY_ULPS_CTRL.u32 = 0x0;
    /* int msk0 */
    g_mipiTxRegsVa->INT_MSK0.u32 = 0x0;
    /* pwr_up unreset */
    g_mipiTxRegsVa->PWR_UP.u32 = 0x0;
    g_mipiTxRegsVa->PWR_UP.u32 = 0xf;
}

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
#ifdef MIPI_TX_DEBUG
    HDF_LOGI("MipiTxDrvSetPayloadData: \n=====set cmd=======");
    HDF_LOGI("GEN_PLD_DATA(0x70): 0x%x", genPldData);
#endif
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

static void MipiTxReset(void)
{
    g_mipiTxRegsVa->PWR_UP.u32 = 0x0;
    g_mipiTxRegsVa->PHY_RSTZ.u32 = 0xd;
    OsalUDelay(1);
    g_mipiTxRegsVa->PWR_UP.u32 = 0x1;
    g_mipiTxRegsVa->PHY_RSTZ.u32 = 0xf;
    OsalUDelay(1);
    return;
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
    if ((outputMode == OUTPUT_MODE_DSI_VIDEO) || (outputMode == OUTPUT_MODE_CSI)) {
        g_mipiTxRegsVa->MODE_CFG.u32 = 0x0;
    }
    if (outputMode == OUTPUT_MODE_DSI_CMD) {
        g_mipiTxRegsVa->CMD_MODE_CFG.u32 = 0x0;
    }
    /* enable input */
    g_mipiTxRegsVa->OPERATION_MODE.u32 = 0x80150000;
    g_mipiTxRegsVa->LPCLK_CTRL.u32 = 0x1;
    MipiTxReset();
    g_enHsMode = true;
}

static void MipiTxDrvDisableInput(void)
{
    /* disable input */
    g_mipiTxRegsVa->OPERATION_MODE.u32 = 0x0;
    g_mipiTxRegsVa->CMD_MODE_CFG.u32 = 0xffffff00;
    /* command mode */
    g_mipiTxRegsVa->MODE_CFG.u32 = 0x1;
    g_mipiTxRegsVa->LPCLK_CTRL.u32 = 0x0;
    MipiTxReset();
    g_enHsMode = false;
}

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
        HDF_LOGE("MipiTxSetComboDevCfg: mipi_tx check combo_dev config fail!");
        return ret;
    }
    /* set controller config */
    MipiTxDrvSetControllerCfg(devCfg);
    /* set phy config */
    MipiTxDrvSetPhyCfg(devCfg);
    g_enCfg = true;
    return ret;
}

static int32_t Hi35xxSetCntlrCfg(struct MipiDsiCntlr *cntlr)
{
    ComboDevCfgTag *dev = GetDevCfg(cntlr);

    if (dev == NULL) {
        HDF_LOGE("Hi35xxSetCntlrCfg: dev is null!");
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
    /* When cmd is not NULL, cmd_size means the length of cmd or it means cmd and addr */
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

static int32_t Hi35xxSetCmd(struct MipiDsiCntlr *cntlr, struct DsiCmdDesc *cmd)
{
    CmdInfoTag cmdInfo;

    (void)cntlr;
    if (cmd == NULL) {
        HDF_LOGE("Hi35xxSetCmd: cmd is null!");
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
        HDF_LOGE("Hi35xxSetCmd: dataLen error!");
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

static int32_t Hi35xxGetCmd(struct MipiDsiCntlr *cntlr, struct DsiCmdDesc *cmd, uint32_t readLen, uint8_t *out)
{
    GetCmdInfoTag cmdInfo;

    (void)cntlr;
    if (cmd == NULL || out == NULL) {
        HDF_LOGE("Hi35xxGetCmd: cmd or out is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    cmdInfo.devno = 0;
    cmdInfo.dataType = cmd->dataType;
    cmdInfo.dataParam = cmd->payload[0];
    cmdInfo.getDataSize = readLen;
    cmdInfo.getData = out;
    return MipiTxGetCmd(&cmdInfo);
}

static void Hi35xxToLp(struct MipiDsiCntlr *cntlr)
{
    (void)cntlr;
    MipiTxDrvDisableInput();
}

static void Hi35xxToHs(struct MipiDsiCntlr *cntlr)
{
    ComboDevCfgTag *dev = GetDevCfg(cntlr);

    if (dev == NULL) {
        HDF_LOGE("Hi35xxToHs: dev is null!");
        return;
    }
    MipiTxDrvEnableInput(dev->outputMode);
}

static struct MipiDsiCntlr g_mipiTx = {
    .devNo = 0
};

static struct MipiDsiCntlrMethod g_method = {
    .setCntlrCfg = Hi35xxSetCntlrCfg,
    .setCmd = Hi35xxSetCmd,
    .getCmd = Hi35xxGetCmd,
    .toHs = Hi35xxToHs,
    .toLp = Hi35xxToLp
};

static int32_t Hi35xxMipiTxInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    g_mipiTx.priv = NULL;
    g_mipiTx.ops = &g_method;
    ret = MipiDsiRegisterCntlr(&g_mipiTx, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Hi35xxMipiTxInit: [MipiDsiRegisterCntlr] fail!");
        return ret;
    }

    ret = MipiTxDrvInit(0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Hi35xxMipiTxInit: [MipiTxDrvInit] fail.");
        return ret;
    }
    ret = MipiDsiDevModuleInit(g_mipiTx.devNo);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Hi35xxMipiTxInit: [MipiDsiDevModuleInit] fail!");
        return ret;
    }
    HDF_LOGI("Hi35xxMipiTxInit: load mipi tx driver successfully!");

    return ret;
}

static void Hi35xxMipiTxRelease(struct HdfDeviceObject *device)
{
    struct MipiDsiCntlr *cntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("Hi35xxMipiTxRelease: device is null!");
        return;
    }
    cntlr = MipiDsiCntlrFromDevice(device);
    if (cntlr == NULL) {
        HDF_LOGE("Hi35xxMipiTxRelease: cntlr is null!");
        return;
    }

    MipiTxDrvExit();
    MipiDsiDevModuleExit(cntlr->devNo);
    MipiDsiUnregisterCntlr(&g_mipiTx);
    g_mipiTx.priv = NULL;
    HDF_LOGI("Hi35xxMipiTxRelease: unload mipi tx driver successfully!");
}

struct HdfDriverEntry g_mipiTxDriverEntry = {
    .moduleVersion = 1,
    .Init = Hi35xxMipiTxInit,
    .Release = Hi35xxMipiTxRelease,
    .moduleName = "HDF_MIPI_TX",
};
HDF_INIT(g_mipiTxDriverEntry);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

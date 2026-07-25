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

#ifndef MIPI_TX_HI3403_H
#define MIPI_TX_HI3403_H

/****************************************************************************
 * hi3403v100 specific definitions
 ****************************************************************************/
#define MIPI_TX_REGS_ADDR   0x17A80000
#define MIPI_TX_REGS_SIZE   0x10000

#define MIPI_TX_IRQ         121

#define MIPI_TX_CRG         0x11018140

#define MIPI_TX_REF_CLK     27

#define TLPX                60
#define TCLK_PREPARE        60
#define TCLK_ZERO           260
#define TCLK_TRAIL          70
#define TCLK_POST           95
#define TPRE_DELAY          100
#define THS_PREPARE         80
#define THS_ZERO            180
#define THS_TRAIL           80

/* phy pll addr */
#define PLL_SET_0X63        0x63
#define PLL_SET_0X64        0x64
#define PLL_SET_0X65        0x65

/* phy timing reg addr */
#define CLK_POST_DELAY      0x68
#define DATA0_TPRE_DELAY    0x28
#define DATA1_TPRE_DELAY    0x38
#define DATA2_TPRE_DELAY    0x48
#define DATA3_TPRE_DELAY    0x58

#define CLK_TLPX            0x10
#define CLK_TCLK_PREPARE    0x11
#define CLK_TCLK_ZERO       0x12
#define CLK_TCLK_TRAIL      0x13

#define DATA0_TLPX          0x20
#define DATA0_THS_PREPARE   0x21
#define DATA0_THS_ZERO      0x22
#define DATA0_THS_TRAIL     0x23
#define DATA1_TLPX          0x30
#define DATA1_THS_PREPARE   0x31
#define DATA1_THS_ZERO      0x32
#define DATA1_THS_TRAIL     0x33
#define DATA2_TLPX          0x40
#define DATA2_THS_PREPARE   0x41
#define DATA2_THS_ZERO      0x42
#define DATA2_THS_TRAIL     0x43
#define DATA3_TLPX          0x50
#define DATA3_THS_PREPARE   0x51
#define DATA3_THS_ZERO      0x52
#define DATA3_THS_TRAIL     0x53

#define MIPI_TX_READ_TIMEOUT_CNT 1000

#define PREPARE_COMPENSATE    10
#define ROUNDUP_VALUE     7999
#define INNER_PEROID      8000

#define MIPI_TX_INTERRUPTS_ENABLE 0

/* PLL helper */
#define mipi_tx_ceil(a, b) (((a) + (b) - 1) / (b))

typedef struct {
    unsigned char data_tpre_delay;
    unsigned char clk_tlpx;
    unsigned char clk_tclk_prepare;
    unsigned char clk_tclk_zero;
    unsigned char clk_tclk_trail;
    unsigned char data_tlpx;
    unsigned char data_ths_prepare;
    unsigned char data_ths_zero;
    unsigned char data_ths_trail;
    unsigned char clk_post_delay;
} MipiTxPhyTimingParamTag;

typedef struct {
    unsigned int vallDet;
    unsigned int vactDet;
    unsigned int hallDet;
    unsigned int hactDet;
    unsigned int hbpDet;
    unsigned int hsaDet;
    unsigned int vsaDet;
} MipiTxDevPhyTag;


#endif /* MIPI_TX_HI3403_H */

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

#ifndef MIPI_TX_HI3403_REG_H
#define MIPI_TX_HI3403_REG_H

/* ====== Register bitfield types ====== */

typedef union {
    struct {
        unsigned int pwr_up : 1;
        unsigned int reserved_0 : 31;
    } bits;
    unsigned int u32;
} U_PWR_UP;

typedef union {
    struct {
        unsigned int tx_esc_clk_division : 8;
        unsigned int reserved_0 : 8;
        unsigned int reserved_1 : 16;
    } bits;
    unsigned int u32;
} U_CLKMGR_CFG;

typedef union {
    struct {
        unsigned int vcid : 2;
        unsigned int reserved_0 : 30;
    } bits;
    unsigned int u32;
} U_VCID;

typedef union {
    struct {
        unsigned int color_coding : 4;
        unsigned int reserved_0 : 4;
        unsigned int loosely18_en : 1;
        unsigned int reserved_1 : 23;
    } bits;
    unsigned int u32;
} U_COLOR_CODING;

typedef union {
    struct {
        unsigned int invact_lpcmd_time : 8;
        unsigned int reserved_0 : 8;
        unsigned int outvact_lpcmd_time : 8;
        unsigned int reserved_1 : 8;
    } bits;
    unsigned int u32;
} U_LP_CMD_TIM;

typedef union {
    struct {
        unsigned int eotp_tx_en : 1;
        unsigned int eotp_rx_en : 1;
        unsigned int bta_en : 1;
        unsigned int ecc_rx_en : 1;
        unsigned int crc_rx_en : 1;
        unsigned int reserved_0 : 27;
    } bits;
    unsigned int u32;
} U_PCKHDL_CFG;

typedef union {
    struct {
        unsigned int gen_vcid_rx : 2;
        unsigned int reserved_0 : 30;
    } bits;
    unsigned int u32;
} U_GEN_VCID;

typedef union {
    struct {
        unsigned int cmd_video_mode : 1;
        unsigned int reserved_0 : 31;
    } bits;
    unsigned int u32;
} U_MODE_CFG;

typedef union {
    struct {
        unsigned int vid_mode_type : 2;
        unsigned int reserved_0 : 6;
        unsigned int lp_vsa_en : 1;
        unsigned int lp_vbp_en : 1;
        unsigned int lp_vfp_en : 1;
        unsigned int lp_vact_en : 1;
        unsigned int lp_hbp_en : 1;
        unsigned int lp_hfp_en : 1;
        unsigned int frame_bta_ack_en : 1;
        unsigned int lp_cmd_en : 1;
        unsigned int vpg_en : 1;
        unsigned int reserved_1 : 3;
        unsigned int vpg_mode : 1;
        unsigned int reserved_2 : 3;
        unsigned int vpg_orientation : 1;
        unsigned int reserved_3 : 7;
    } bits;
    unsigned int u32;
} U_VID_MODE_CFG;

typedef union {
    struct {
        unsigned int vid_pkt_size : 14;
        unsigned int reserved_0 : 18;
    } bits;
    unsigned int u32;
} U_VID_PKT_SIZE;

typedef union {
    struct {
        unsigned int vid_num_chunks : 13;
        unsigned int reserved_0 : 19;
    } bits;
    unsigned int u32;
} U_VID_NUM_CHUNKS;

typedef union {
    struct {
        unsigned int vid_null_size : 13;
        unsigned int reserved_0 : 19;
    } bits;
    unsigned int u32;
} U_VID_NULL_SIZE;

typedef union {
    struct {
        unsigned int vid_hsa_time : 12;
        unsigned int reserved_0 : 20;
    } bits;
    unsigned int u32;
} U_VID_HSA_TIME;

typedef union {
    struct {
        unsigned int vid_hbp_time : 12;
        unsigned int reserved_0 : 20;
    } bits;
    unsigned int u32;
} U_VID_HBP_TIME;

typedef union {
    struct {
        unsigned int vid_hline_time : 15;
        unsigned int reserved_0 : 17;
    } bits;
    unsigned int u32;
} U_VID_HLINE_TIME;

typedef union {
    struct {
        unsigned int vsa_lines : 10;
        unsigned int reserved_0 : 22;
    } bits;
    unsigned int u32;
} U_VID_VSA_LINES;

typedef union {
    struct {
        unsigned int vbp_lines : 10;
        unsigned int reserved_0 : 22;
    } bits;
    unsigned int u32;
} U_VID_VBP_LINES;

typedef union {
    struct {
        unsigned int vfp_lines : 10;
        unsigned int reserved_0 : 22;
    } bits;
    unsigned int u32;
} U_VID_VFP_LINES;

typedef union {
    struct {
        unsigned int v_active_lines : 14;
        unsigned int reserved_0 : 18;
    } bits;
    unsigned int u32;
} U_VID_VACTIVE_LINES;

typedef union {
    struct {
        unsigned int edpi_allowed_cmd_size : 16;
        unsigned int reserved_0 : 16;
    } bits;
    unsigned int u32;
} U_EDPI_CMD_SIZE;

typedef union {
    struct {
        unsigned int reserved_0 : 1;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 6;
        unsigned int gen_sw_0p_tx : 1;
        unsigned int gen_sw_1p_tx : 1;
        unsigned int gen_sw_2p_tx : 1;
        unsigned int gen_sr_0p_tx : 1;
        unsigned int gen_sr_1p_tx : 1;
        unsigned int gen_sr_2p_tx : 1;
        unsigned int gen_lw_tx : 1;
        unsigned int reserved_3 : 1;
        unsigned int dcs_sw_0p_tx : 1;
        unsigned int dcs_sw_1p_tx : 1;
        unsigned int dcs_sr_0p_tx : 1;
        unsigned int dcs_lw_tx : 1;
        unsigned int reserved_4 : 4;
        unsigned int max_rd_pkt_size : 1;
        unsigned int reserved_5 : 7;
    } bits;
    unsigned int u32;
} U_CMD_MODE_CFG;

typedef union {
    struct {
        unsigned int gen_dt : 6;
        unsigned int gen_vc : 2;
        unsigned int gen_wc_lsbyte : 8;
        unsigned int gen_wc_msbyte : 8;
        unsigned int reserved_0 : 8;
    } bits;
    unsigned int u32;
} U_GEN_HDR;

typedef union {
    struct {
        unsigned int gen_pld_b1 : 8;
        unsigned int gen_pld_b2 : 8;
        unsigned int gen_pld_b3 : 8;
        unsigned int gen_pld_b4 : 8;
    } bits;
    unsigned int u32;
} U_GEN_PLD_DATA;

typedef union {
    struct {
        unsigned int gen_cmd_empty : 1;
        unsigned int gen_cmd_full : 1;
        unsigned int gen_pld_w_empty : 1;
        unsigned int gen_pld_w_full : 1;
        unsigned int gen_pld_r_empty : 1;
        unsigned int gen_pld_r_full : 1;
        unsigned int gen_rd_cmd_busy : 1;
        unsigned int reserved_0 : 25;
    } bits;
    unsigned int u32;
} U_CMD_PKT_STATUS;

typedef union {
    struct {
        unsigned int lp_wr_to_cnt : 16;
        unsigned int reserved_0 : 16;
    } bits;
    unsigned int u32;
} U_LP_WR_TO_CNT;

typedef union {
    struct {
        unsigned int bta_to_cnt : 16;
        unsigned int reserved_0 : 16;
    } bits;
    unsigned int u32;
} U_BTA_TO_CNT;

typedef union {
    struct {
        unsigned int phy_txrequestclkhs : 1;
        unsigned int auto_clklane_ctrl : 1;
        unsigned int reserved_0 : 30;
    } bits;
    unsigned int u32;
} U_LPCLK_CTRL;

typedef union {
    struct {
        unsigned int phy_clklp2hs_time : 10;
        unsigned int reserved_0 : 6;
        unsigned int phy_clkhs2lp_time : 10;
        unsigned int reserved_1 : 6;
    } bits;
    unsigned int u32;
} U_PHY_TMR_LPCLK_CFG;

typedef union {
    struct {
        unsigned int phy_lp2hs_time : 10;
        unsigned int reserved_0 : 6;
        unsigned int phy_hs2lp_time : 10;
        unsigned int reserved_1 : 6;
    } bits;
    unsigned int u32;
} U_PHY_TMR_CFG;

typedef union {
    struct {
        unsigned int phy_shutdownz : 1;
        unsigned int phy_rstz : 1;
        unsigned int phy_enableclk : 1;
        unsigned int phy_forcepll : 1;
        unsigned int reserved_0 : 28;
    } bits;
    unsigned int u32;
} U_PHY_RSTZ;

typedef union {
    struct {
        unsigned int n_lanes : 2;
        unsigned int reserved_0 : 6;
        unsigned int phy_stop_wait_time : 8;
        unsigned int reserved_1 : 16;
    } bits;
    unsigned int u32;
} U_PHY_IF_CFG;

typedef union {
    struct {
        unsigned int phy_txrequlpsclk : 1;
        unsigned int phy_txexitulpsclk : 1;
        unsigned int phy_txrequlpslan : 1;
        unsigned int phy_txexitulpslan : 1;
        unsigned int reserved_0 : 28;
    } bits;
    unsigned int u32;
} U_PHY_ULPS_CTRL;

typedef union {
    struct {
        unsigned int phy_testclr : 1;
        unsigned int phy_testclk : 1;
        unsigned int reserved_0 : 30;
    } bits;
    unsigned int u32;
} U_PHY_TST_CTRL0;

typedef union {
    struct {
        unsigned int phy_testdin : 8;
        unsigned int phy_testdout : 8;
        unsigned int phy_testen : 1;
        unsigned int reserved_0 : 15;
    } bits;
    unsigned int u32;
} U_PHY_TST_CTRL1;

typedef union {
    struct {
        unsigned int ack_with_err_0 : 1;
        unsigned int ack_with_err_1 : 1;
        unsigned int ack_with_err_2 : 1;
        unsigned int ack_with_err_3 : 1;
        unsigned int ack_with_err_4 : 1;
        unsigned int ack_with_err_5 : 1;
        unsigned int ack_with_err_6 : 1;
        unsigned int ack_with_err_7 : 1;
        unsigned int ack_with_err_8 : 1;
        unsigned int ack_with_err_9 : 1;
        unsigned int ack_with_err_10 : 1;
        unsigned int ack_with_err_11 : 1;
        unsigned int ack_with_err_12 : 1;
        unsigned int ack_with_err_13 : 1;
        unsigned int ack_with_err_14 : 1;
        unsigned int ack_with_err_15 : 1;
        unsigned int dphy_errors_0 : 1;
        unsigned int dphy_errors_1 : 1;
        unsigned int dphy_errors_2 : 1;
        unsigned int dphy_errors_3 : 1;
        unsigned int dphy_errors_4 : 1;
        unsigned int reserved_0 : 11;
    } bits;
    unsigned int u32;
} U_INT_ST0;

typedef union {
    struct {
        unsigned int to_hs_tx : 1;
        unsigned int to_lp_rx : 1;
        unsigned int ecc_single_err : 1;
        unsigned int ecc_multi_err : 1;
        unsigned int crc_err : 1;
        unsigned int pkt_size_err : 1;
        unsigned int eopt_err : 1;
        unsigned int dpi_pld_wr_err : 1;
        unsigned int gen_cmd_wr_err : 1;
        unsigned int gen_pld_wr_err : 1;
        unsigned int gen_pld_send_err : 1;
        unsigned int gen_pld_rd_err : 1;
        unsigned int gen_pld_recev_err : 1;
        unsigned int reserved_0 : 3;
        unsigned int vsstart : 1;
        unsigned int reserved_1 : 3;
        unsigned int rxtrigger_0 : 1;
        unsigned int rxtrigger_1 : 1;
        unsigned int rxtrigger_2 : 1;
        unsigned int rxtrigger_3 : 1;
        unsigned int hss_abnormal : 1;
        unsigned int reserved_2 : 7;
    } bits;
    unsigned int u32;
} U_INT_ST1;

typedef union {
    struct {
        unsigned int mask_ack_with_err_0 : 1;
        unsigned int mask_ack_with_err_1 : 1;
        unsigned int mask_ack_with_err_2 : 1;
        unsigned int mask_ack_with_err_3 : 1;
        unsigned int mask_ack_with_err_4 : 1;
        unsigned int mask_ack_with_err_5 : 1;
        unsigned int mask_ack_with_err_6 : 1;
        unsigned int mask_ack_with_err_7 : 1;
        unsigned int mask_ack_with_err_8 : 1;
        unsigned int mask_ack_with_err_9 : 1;
        unsigned int mask_ack_with_err_10 : 1;
        unsigned int mask_ack_with_err_11 : 1;
        unsigned int mask_ack_with_err_12 : 1;
        unsigned int mask_ack_with_err_13 : 1;
        unsigned int mask_ack_with_err_14 : 1;
        unsigned int mask_ack_with_err_15 : 1;
        unsigned int mask_dphy_errors_0 : 1;
        unsigned int mask_dphy_errors_1 : 1;
        unsigned int mask_dphy_errors_2 : 1;
        unsigned int mask_dphy_errors_3 : 1;
        unsigned int mask_dphy_errors_4 : 1;
        unsigned int reserved_0 : 11;
    } bits;
    unsigned int u32;
} U_INT_MSK0;

typedef union {
    struct {
        unsigned int mask_to_hs_tx : 1;
        unsigned int mask_to_lp_rx : 1;
        unsigned int mask_ecc_single_err : 1;
        unsigned int mask_ecc_multi_err : 1;
        unsigned int mask_crc_err : 1;
        unsigned int mask_pkt_size_err : 1;
        unsigned int mask_eopt_err : 1;
        unsigned int mask_dpi_pld_wr_err : 1;
        unsigned int mask_gen_cmd_wr_err : 1;
        unsigned int mask_gen_pld_wr_err : 1;
        unsigned int mask_gen_pld_send_err : 1;
        unsigned int mask_gen_pld_rd_err : 1;
        unsigned int mask_gen_pld_recev_err : 1;
        unsigned int reserved_0 : 3;
        unsigned int mask_vsstart : 1;
        unsigned int reserved_1 : 3;
        unsigned int mask_trigger_0 : 1;
        unsigned int mask_trigger_1 : 1;
        unsigned int mask_trigger_2 : 1;
        unsigned int mask_trigger_3 : 1;
        unsigned int mask_hss_abnormal : 1;
        unsigned int reserved_2 : 7;
    } bits;
    unsigned int u32;
} U_INT_MSK1;

typedef union {
    struct {
        unsigned int reserved_0 : 1;
        unsigned int mem_ck_en : 1;
        unsigned int reserved_1 : 2;
        unsigned int reserved_2 : 1;
        unsigned int init_skew_en : 1;
        unsigned int period_skew_en : 1;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 1;
        unsigned int reserved_5 : 1;
        unsigned int reserved_6 : 1;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 1;
        unsigned int reserved_9 : 1;
        unsigned int reserved_10 : 1;
        unsigned int reserved_11 : 1;
        unsigned int read_empty_vsync_en : 1;
        unsigned int reserved_12 : 1;
        unsigned int buf_clr_en : 1;
        unsigned int reserved_13 : 1;
        unsigned int hss_abnormal_rst : 1;
        unsigned int reserved_14 : 3;
        unsigned int reserved_15 : 1;
        unsigned int reserved_16 : 1;
        unsigned int reserved_17 : 1;
        unsigned int reserved_18 : 1;
        unsigned int reserved_19 : 3;
        unsigned int input_en : 1;
    } bits;
    unsigned int u32;
} U_OPERATION_MODE;

typedef union {
    struct {
        unsigned int reserved_0 : 6;
        unsigned int reserved_1 : 2;
        unsigned int dt_hss : 6;
        unsigned int reserved_2 : 2;
        unsigned int dt_vse : 6;
        unsigned int reserved_3 : 2;
        unsigned int dt_vss : 6;
        unsigned int reserved_4 : 2;
    } bits;
    unsigned int u32;
} U_DATATYPE0;

typedef union {
    struct {
        unsigned int csi_en : 1;
        unsigned int reserved_0 : 3;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 3;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 3;
        unsigned int reserved_5 : 1;
        unsigned int reserved_6 : 3;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 15;
    } bits;
    unsigned int u32;
} U_CSI_CTRL;

typedef union {
    struct {
        unsigned int lane0_id : 2;
        unsigned int reserved_0 : 2;
        unsigned int lane1_id : 2;
        unsigned int reserved_1 : 2;
        unsigned int lane2_id : 2;
        unsigned int reserved_2 : 2;
        unsigned int lane3_id : 2;
        unsigned int reserved_3 : 18;
    } bits;
    unsigned int u32;
} U_LANE_ID;

/* ====== hi3403v100 additional register union types ====== */

typedef union {
    struct {
        unsigned int lprx_to_cnt : 16;
        unsigned int hstx_to_cnt : 16;
    } bits;
    unsigned int u32;
} U_HS_LP_TO_SET;

typedef union {
    struct {
        unsigned int hsrd_to_set : 16;
        unsigned int reserved_0 : 16;
    } bits;
    unsigned int u32;
} U_HSRD_TO_SET;

typedef union {
    struct {
        unsigned int lprd_to_set : 16;
        unsigned int reserved_0 : 16;
    } bits;
    unsigned int u32;
} U_LPRD_TO_SET;

typedef union {
    struct {
        unsigned int hswr_to_set : 16;
        unsigned int reserved_0 : 8;
        unsigned int pre_to_mode : 1;
        unsigned int reserved_1 : 7;
    } bits;
    unsigned int u32;
} U_HSWR_TO_SET;

typedef union {
    struct {
        unsigned int lpwr_to_set : 16;
        unsigned int reserved_0 : 16;
    } bits;
    unsigned int u32;
} U_LPWR_TO_SET;

typedef union {
    struct {
        unsigned int bta_to_set : 16;
        unsigned int reserved_0 : 16;
    } bits;
    unsigned int u32;
} U_BTA_TO_SET;

typedef union {
    struct {
        unsigned int tx_triggers : 4;
        unsigned int reserved_0 : 28;
    } bits;
    unsigned int u32;
} U_TX_TRIGGERS;

typedef union {
    struct {
        unsigned int read_cmd_time : 15;
        unsigned int reserved_0 : 17;
    } bits;
    unsigned int u32;
} U_READ_CMD_TIME;

typedef union {
    struct {
        unsigned int vid_shadow_en : 1;
        unsigned int reserved_0 : 7;
        unsigned int vid_shadow_req : 1;
        unsigned int reserved_1 : 23;
    } bits;
    unsigned int u32;
} U_VID_SHADOW_CTRL;

typedef union {
    struct {
        unsigned int dt_null : 6;
        unsigned int reserved_0 : 2;
        unsigned int dt_blank : 6;
        unsigned int reserved_1 : 2;
        unsigned int dt_etop : 6;
        unsigned int reserved_2 : 2;
        unsigned int dt_hse : 6;
        unsigned int reserved_3 : 2;
    } bits;
    unsigned int u32;
} U_DATATYPE1;

typedef union {
    struct {
        unsigned int delay_regin : 17;
        unsigned int reserved_0 : 3;
        unsigned int delay_abnormal : 1;
        unsigned int delay_from_reg : 1;
        unsigned int reserved_1 : 10;
    } bits;
    unsigned int u32;
} U_READ_MEMORY_DELAY_CTRL;

/* ====== hi3403v100 register struct (hi3403 hardware offsets) ====== */
typedef volatile struct {
    U_PWR_UP PWR_UP;                           /* 0x0 */
    unsigned int reserved_0;                   /* 0x4 */
    U_CLKMGR_CFG CLKMGR_CFG;                   /* 0x8 */
    U_VCID VCID;                               /* 0xc */
    U_COLOR_CODING COLOR_CODING;               /* 0x10 */
    unsigned int reserved_1;                   /* 0x14 */
    U_LP_CMD_TIM LP_CMD_TIM;                   /* 0x18 */
    unsigned int reserved_2[4];                /* 0x1c~0x28 */
    U_PCKHDL_CFG PCKHDL_CFG;                   /* 0x2c */
    U_GEN_VCID GEN_VCID;                       /* 0x30 */
    U_MODE_CFG MODE_CFG;                       /* 0x34 */
    U_VID_MODE_CFG VID_MODE_CFG;               /* 0x38 */
    U_VID_PKT_SIZE VID_PKT_SIZE;               /* 0x3c */
    U_VID_NUM_CHUNKS VID_NUM_CHUNKS;           /* 0x40 */
    U_VID_NULL_SIZE VID_NULL_SIZE;             /* 0x44 */
    U_VID_HSA_TIME VID_HSA_TIME;               /* 0x48 */
    U_VID_HBP_TIME VID_HBP_TIME;               /* 0x4c */
    U_VID_HLINE_TIME VID_HLINE_TIME;           /* 0x50 */
    U_VID_VSA_LINES VID_VSA_LINES;             /* 0x54 */
    U_VID_VBP_LINES VID_VBP_LINES;             /* 0x58 */
    U_VID_VFP_LINES VID_VFP_LINES;             /* 0x5c */
    U_VID_VACTIVE_LINES VID_VACTIVE_LINES;     /* 0x60 */
    U_EDPI_CMD_SIZE EDPI_CMD_SIZE;             /* 0x64 */
    U_CMD_MODE_CFG CMD_MODE_CFG;               /* 0x68 */
    U_GEN_HDR GEN_HDR;                         /* 0x6c */
    U_GEN_PLD_DATA GEN_PLD_DATA;               /* 0x70 */
    U_CMD_PKT_STATUS CMD_PKT_STATUS;           /* 0x74 */
    U_HS_LP_TO_SET HS_LP_TO_SET;               /* 0x78 */
    U_HSRD_TO_SET HSRD_TO_SET;                 /* 0x7c */
    U_LPRD_TO_SET LPRD_TO_SET;                 /* 0x80 */
    U_HSWR_TO_SET HSWR_TO_SET;                 /* 0x84 */
    U_LPWR_TO_SET LPWR_TO_SET;                 /* 0x88 */
    U_BTA_TO_SET BTA_TO_SET;                   /* 0x8c */
    unsigned int reserved_3;                   /* 0x90 */
    U_LPCLK_CTRL LPCLK_CTRL;                   /* 0x94 */
    U_PHY_TMR_LPCLK_CFG PHY_TMR_LPCLK_CFG;     /* 0x98 */
    U_PHY_TMR_CFG PHY_TMR_CFG;                 /* 0x9c */
    U_PHY_RSTZ PHY_RSTZ;                       /* 0xa0 */
    U_PHY_IF_CFG PHY_IF_CFG;                   /* 0xa4 */
    U_PHY_ULPS_CTRL PHY_ULPS_CTRL;             /* 0xa8 */
    U_TX_TRIGGERS TX_TRIGGERS;                 /* 0xac */
    unsigned int reserved_4;                   /* 0xb0 */
    U_PHY_TST_CTRL0 PHY_TST_CTRL0;             /* 0xb4 */
    U_PHY_TST_CTRL1 PHY_TST_CTRL1;             /* 0xb8 */
    U_INT_ST0 INT_ST0;                         /* 0xbc */
    U_INT_ST1 INT_ST1;                         /* 0xc0 */
    U_INT_MSK0 INT_MSK0;                       /* 0xc4 */
    U_INT_MSK1 INT_MSK1;                       /* 0xc8 */
    unsigned int reserved_6[10];               /* 0xcc~0xf0 */
    U_READ_CMD_TIME READ_CMD_TIME;             /* 0xf4 */
    unsigned int reserved_7[2];                /* 0xf8~0xfc */
    U_VID_SHADOW_CTRL VID_SHADOW_CTRL;         /* 0x100 */
    unsigned int reserved_8[82];               /* 0x104~0x248 */
    unsigned int GINT_MSK;                     /* 0x24c */
    unsigned int reserved_9[46];               /* 0x250~0x304 */
    U_OPERATION_MODE OPERATION_MODE;           /* 0x308 */
    unsigned int reserved_10[9];               /* 0x30c~0x32c */
    U_DATATYPE0 DATATYPE0;                     /* 0x330 */
    U_DATATYPE1 DATATYPE1;                     /* 0x334 */
    U_CSI_CTRL CSI_CTRL;                       /* 0x338 */
    unsigned int reserved_11[5];               /* 0x33c~0x34c */
    U_LANE_ID LANE_ID;                         /* 0x350 */
    U_READ_MEMORY_DELAY_CTRL READ_MEMORY_DELAY_CTRL; /* 0x354 */
} MipiTxRegsTypeTag;

#endif /* __MIPI_TX_HI3403_REG_H__ */

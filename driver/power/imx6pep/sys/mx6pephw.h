// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6pephw.h
//
// Abstract:
//
//   IMX6 CCM and GPC Hardware Definitions
//

#ifndef _MX6PEP_HW_H_
#define _MX6PEP_HW_H_

#pragma warning(push)
#pragma warning(disable:4201) // nameless struct/union

#include <pshpack1.h>

#define MX6_IOMUXC_BASE 0x020E0000
#define MX6_IOMUXC_LENGTH 0x4000

#define MX6_CCM_BASE 0x020C4000
#define MX6_CCM_LENGTH 0x4000

#define MX6_ANALOG_BASE 0x020C8000
#define MX6_ANALOG_LENGTH 0x1000

#define MX6_GPC_BASE 0x020DC000
#define MX6_GPC_LENGTH 0x1000

#define MX6_MMDC_BASE 0x021B0000
#define MX6_MMDC_LENGTH 0x8000

#define MX6_ARMMP_BASE 0x00A00000
#define MX6_ARMMP_LENGTH 0x2000

#define MX6_REF_CLK_24M_FREQ 24000000

union MX6_CCM_CCR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG OSCNT : 8;                    // 0-7 Oscillator ready counter value
        ULONG reserved1 : 4;                // 8-11
        ULONG COSC_EN : 1;                  // 12 On chip oscillator enable bit
        ULONG reserved2 : 3;                // 13-15
        ULONG WB_COUNT : 3;                 // 16-18 Well Bias counter
        ULONG reserved3 : 2;                // 19-20
        ULONG REG_BYPASS_COUNT : 6;         // 21-26 Counter for analog_reg_bypass signal assertion after standby voltage request by PMIC_STBY_REQ
        ULONG RBC_EN : 1;                   // 27 Enable for REG_BYPASS_COUNTER
        ULONG reserved4 : 4;                // 28-31
        // MSB
    };
};

enum MX6_CCM_PLL3_SW_CLK_SEL {
    MX6_CCM_PLL3_SW_CLK_SEL_PLL3_MAIN_CLK,
    MX6_CCM_PLL3_SW_CLK_SEL_PLL3_BYPASS_CLK,
};

union MX6_CCM_CCSR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG pll3_sw_clk_sel : 1;         // 0 Selects source to generate pll3_sw_clk
        ULONG reserved1 : 1;               // 1 reserved
        ULONG pll1_sw_clk_sel : 1;         // 2 Selects source to generate pll1_sw_clk.
        ULONG reserved2 : 5;               // 3-7
        ULONG step_sel : 1;                // 8 Selects the option to be chosen for the step frequency
        ULONG pfd_396m_dis_mask : 1;       // 9 Mask of 396M PFD auto-disable
        ULONG pfd_352m_dis_mask : 1;       // 10 Mask of 352M PFD auto-disable.
        ULONG pfd_594_dis_mask : 1;        // 11 Mask of 594M PFD auto-disable.
        ULONG pfd_508m_dis_mask : 1;       // 12 Mask of 508M PFD auto-disable
        ULONG pfd_454m_dis_mask : 1;       // 13 Mask of 454M PFD auto-disable.
        ULONG pfd_720m_dis_mask : 1;       // 14 Mask of 720M PFD auto-disable.
        ULONG pfd_540m_dis_mask : 1;       // 15 Mask of 540M PFD auto-disable.
        ULONG reserved3 : 16;              // 16-31
        // MSB
    };
};

union MX6_CCM_CACRR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG arm_podf : 3;                // 0-3 Divider for ARM clock root
        ULONG reserved : 29;               // 3-31
        // MSB
    };
} ;

// CBCMR.gpu2d_axi_clk_sel
enum MX6_CCM_GPU2D_AXI_CLK_SEL {
    MX6_CCM_GPU2D_AXI_CLK_SEL_AXI,
    MX6_CCM_GPU2D_AXI_CLK_SEL_AHB,
};

// CBCMR.gpu3d_axi_clk_sel
enum MX6_CCM_GPU3D_AXI_CLK_SEL {
    MX6_CCM_GPU3D_AXI_CLK_SEL_AXI,
    MX6_CCM_GPU3D_AXI_CLK_SEL_AHB,
};

// CBCMR.gpu2d_core_clk_sel
enum MX6_CCM_GPU2D_CORE_CLK_SEL {
    MX6_CCM_GPU2D_CORE_CLK_SEL_AXI,
    MX6_CCM_GPU2D_CORE_CLK_SEL_PLL3_SW,
    MX6_CCM_GPU2D_CORE_CLK_SEL_PLL2_PFD0,
    MX6_CCM_GPU2D_CORE_CLK_SEL_PLL2_PFD2,
};

// CBCMR.pre_periph_clk_sel
enum MX6_CCM_PRE_PERIPH_CLK_SEL {
    MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2,
    MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2_PFD2,
    MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2_PFD0,
    MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2_PFD2_DIV2,
};

// CBCMR.gpu3d_core_clk_sel
enum MX6_CCM_GPU3D_CORE_CLK_SEL {
    MX6_CCM_GPU3D_CORE_CLK_SEL_MMDC_CH0_AXI,
    MX6_CCM_GPU3D_CORE_CLK_SEL_PLL3_SW,
    MX6_CCM_GPU3D_CORE_CLK_SEL_PLL2_PFD1,
    MX6_CCM_GPU3D_CORE_CLK_SEL_PLL2_PFD2,
};

// CBCMR.gpu3d_shader_clk_sel
enum MX6_CCM_GPU3D_SHADER_CLK_SEL {
    MX6_CCM_GPU3D_SHADER_CLK_SEL_MMDC_CH0_AXI,
    MX6_CCM_GPU3D_SHADER_CLK_SEL_PLL3_SW,
    MX6_CCM_GPU3D_SHADER_CLK_SEL_PLL2_PFD1,
    MX6_CCM_GPU3D_SHADER_CLK_SEL_PLL3_PFD0,
};

// CBCMR.periph_clk2_sel
enum MX6_CCM_PERIPH_CLK2_SEL {
    MX6_CCM_PERIPH_CLK2_SEL_PLL3_SW_CLK,
    MX6_CCM_PERIPH_CLK2_SEL_OSC_CLK,
    MX6_CCM_PERIPH_CLK2_SEL_PLL2,
};

// CSCMR1.usdhcN_clk_sel
enum MX6_CCM_USDHC_CLK_SEL {
    MX6_CCM_USDHC_CLK_SEL_PLL2_PFD2,
    MX6_CCM_USDHC_CLK_SEL_PLL2_PFD0,
};

enum MX6_CCM_SSI_CLK_SEL {
    MX6_CCM_SSI_CLK_SEL_PLL3_PFD2,
    MX6_CCM_SSI_CLK_SEL_PLL3_PFD3,
    MX6_CCM_SSI_CLK_SEL_PLL4,
};

union MX6_CCM_CSCMR1_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG perclk_podf : 3;             // 0-2 Divider for perclk podf.
        ULONG reserved1 : 7;               // 3-9
        ULONG ssi1_clk_sel : 2;            // 10-11 Selector for ssi1 clock multiplexer
        ULONG ssi2_clk_sel : 2;            // 12-13 Selector for ssi2 clock multiplexer
        ULONG ssi3_clk_sel : 2;            // 14-15 Selector for ssi3 clock multiplexer
        ULONG usdhc1_clk_sel : 1;          // 16 Selector for usdhc1 clock multiplexer (MX6_CCM_USDHC_CLK_SEL)
        ULONG usdhc2_clk_sel : 1;          // 17 Selector for usdhc2 clock multiplexer (MX6_CCM_USDHC_CLK_SEL)
        ULONG usdhc3_clk_sel : 1;          // 18 Selector for usdhc3 clock multiplexer (MX6_CCM_USDHC_CLK_SEL)
        ULONG usdhc4_clk_sel : 1;          // 19 Selector for usdhc4 clock multiplexer (MX6_CCM_USDHC_CLK_SEL)
        ULONG aclk_podf : 3;               // 20-22 Divider for aclk clock root (These bits are inverted between R/W and are not sequential.)
        ULONG aclk_eim_slow_podf : 3;      // 23-25 Divider for aclk_eim_slow clock root.
        ULONG reserved2 : 1;               // 16
        ULONG aclk_sel : 2;                // 27-28 Selector for aclk root clock multiplexer
        ULONG aclk_eim_slow_sel : 2;       // 29-30 Selector for aclk_eim_slow root clock multiplexer
        ULONG reserved3 : 1;               // 31
        // MSB
    };
};

union MX6_CCM_CSCDR1_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG uart_clk_podf : 6;            // 0-5 Divider for uart clock podf.
        ULONG reserved1 : 5;                // 6-10
        ULONG usdhc1_podf : 3;              // 11-13 Divider for usdhc1 clock podf.
        ULONG reserved2 : 2;                // 14-15
        ULONG usdhc2_podf : 3;              // 16-18 Divider for usdhc2 clock.
        ULONG usdhc3_podf : 3;              // 19-21 Divider for usdhc3 clock podf.
        ULONG usdhc4_podf : 3;              // 22-24 Divider for esdhc4 clock pred
        ULONG vpu_axi_podf : 3;             // 25-27 Divider for vpu axi clock podf.
        ULONG reserved3 : 4;                // 28-31
        // MSB
    };
};

union MX6_CCM_CS1CDR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG ssi1_clk_podf : 6;            // 0-5 Divider for ssi1 clock podf
        ULONG ssi1_clk_pred : 3;            // 6-8 Divider for ssi1 clock pred.
        ULONG esai_clk_pred : 3;            // 9-11 Divider for esai clock pred
        ULONG reserved1 : 4;                // 12-15
        ULONG ssi3_clk_podf : 6;            // 16-21 Divider for ssi3 clock podf
        ULONG ssi3_clk_pred : 3;            // 22-24 Divider for ssi3 clock pred.
        ULONG esai_clk_podf : 3;            // 25-27 Divider for esai clock podf.
        ULONG reserved2 : 4;                // 28-31
        // MSB
    };
};

union MX6_CCM_CS2CDR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG ssi2_clk_podf : 6;            // 0-5 Divider for ssi2 clock podf
        ULONG ssi2_clk_pred : 3;            // 6-8 Divider for ssi2 clock pred.
        ULONG ldb_di0_clk_sel : 3;          // 9-11 Selector for ldb_di1 clock multiplexer
        ULONG ldb_di1_clk_sel : 3;          // 12-14 Selector for ldb_di1 clock multiplexer
        ULONG reserved1 : 1;                // 15
        ULONG enfc_clk_sel : 2;             // 16-17 Selector for enfc clock multiplexer
        ULONG enfc_clk_pred : 3;            // 18-20 Divider for enfc clock pred divider.
        ULONG enfc_clk_podf : 6;            // 21-26 Divider for enfc clock divider
        ULONG reserved2 : 5;                // 27-31
        // MSB
    };
};

enum VPU_AXI_CLK_SEL {
    VPU_AXI_CLK_SEL_AXI_CLK_ROOT,
    VPU_AXI_CLK_SEL_PLL2_PFD2,
    VPU_AXI_CLK_SEL_PLL2_PFD0,
};

union MX6_CCM_CBCMR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG gpu2d_axi_clk_sel : 1;       // 0 Selector for gpu2d_axi clock multiplexer (MX6_CCM_GPU2D_AXI_CLK_SEL)
        ULONG gpu3d_axi_clk_sel : 1;       // 1 Selector for gpu3d_axi clock multiplexer (MX6_CCM_GPU3D_AXI_CLK_SEL)
        ULONG reserved1 : 2;               // 2-3
        ULONG gpu3d_core_clk_sel : 2;      // 4-5 Selector for gpu3d_core clock multiplexer (MX6_CCM_GPU3D_CORE_CLK_SEL)
        ULONG reserved2 : 2;               // 6-7
        ULONG gpu3d_shader_clk_sel : 2;    // 8-9 Selector for gpu3d_shader clock multiplexer (MX6_CCM_GPU3D_SHADER_CLK_SEL)
        ULONG pcie_axi_clk_sel : 1;        // 10 Selector for pcie_axi clock multiplexer
        ULONG vdoaxi_clk_sel : 1;          // 11 Selector for vdoaxi clock multiplexer
        ULONG periph_clk2_sel : 2;         // 12-13 Selector for peripheral clk2 clock multiplexer
        ULONG vpu_axi_clk_sel : 2;         // 14-15 Selector for VPU axi clock multiplexer
        ULONG gpu2d_core_clk_sel : 2;      // 16-17 Selector for open vg (GPU2D Core) clock multiplexer (MX6_CCM_GPU2D_CORE_CLK_SEL)
        ULONG pre_periph_clk_sel : 2;      // 18-19 Selector for pre_periph clock multiplexer
        ULONG periph2_clk2_sel : 1;        // 20 Selector for periph2_clk2 clock multiplexer
        ULONG pre_periph2_clk_sel : 2;     // 21-22 Selector for pre_periph2 clock multiplexer
        ULONG gpu2d_core_clk_podf : 3;     // 23-25 Divider for gpu2d_core clock.
        ULONG gpu3d_core_podf : 3;         // 26-28 Divider for gpu3d_core clock
        ULONG gpu3d_shader_podf : 3;       // 29-31 Divider for gpu3d_shader clock.
        // MSB
    };
};

union MX6_CCM_CCGR0_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG aips_tz1_clk_enable : 2;     // 0-1 aips_tz1 clocks (aips_tz1_clk_enable)
        ULONG aips_tz2_clk_enable : 2;     // 2-3 aips_tz2 clocks (aips_tz2_clk_enable)
        ULONG apbhdma_hclk_enable : 2;     // 4-5 apbhdma hclk clock (apbhdma_hclk_enable)
        ULONG asrc_clk_enable : 2;         // 6-7 asrc clock (asrc_clk_enable)
        ULONG caam_secure_mem_clk_enable : 2; // 8-9 caam_secure_mem clock (caam_secure_mem_clk_enable)
        ULONG caam_wrapper_aclk_enable : 2; // 10-11 caam_wrapper_aclk clock (caam_wrapper_aclk_enable)
        ULONG caam_wrapper_ipg_enable : 2; // 12-13 caam_wrapper_ipg clock (caam_wrapper_ipg_enable)
        ULONG can1_clk_enable : 2;         // 14-15 can1 clock (can1_clk_enable)
        ULONG can1_serial_clk_enable : 2;  // 16-17 can1_serial clock (can1_serial_clk_enable)
        ULONG can2_clk_enable : 2;         // 18-19 can2 clock (can2_clk_enable)
        ULONG can2_serial_clk_enable : 2;  // 20-21 can2_serial clock (can2_serial_clk_enable)
        ULONG arm_dbg_clk_enable : 2;      // 22-23 CPU debug clocks (arm_dbg_clk_enable)
        ULONG dcic1_clk_enable : 2;        // 24-25 dcic 1 clocks (dcic1_clk_enable)
        ULONG dcic2_clk_enable : 2;        // 26-27 dcic2 clocks (dcic2_clk_enable)
        ULONG dtcp_clk_enable : 2;         // 28-29 dtcp clocks (dtcp_clk_enable)
        ULONG reserved : 2;                // 30-31
        // MSB
    };
};

//
// NOTE: OPENVG clock cannot be gated without gating GPU2D clock as well.
//       Configure both CG bits (CCM_ANALOG_CCGR1[CG12] and
//       CCM_ANALOG_CCGR3[CG15]) to gate OPENVG.
//

union MX6_CCM_CCGR1_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG ecspi1_clk_enable : 2;       // 0-1 ecspi1 clocks (ecspi1_clk_enable)
        ULONG ecspi2_clk_enable : 2;       // 2-3 ecspi2 clocks (ecspi2_clk_enable)
        ULONG ecspi3_clk_enable : 2;       // 4-5 ecspi3 clocks (ecspi3_clk_enable)
        ULONG ecspi4_clk_enable : 2;       // 6-7 ecspi4 clocks (ecspi4_clk_enable)
        ULONG ecspi5_clk_enable : 2;       // 8-9 ecspi5 clocks (ecspi5_clk_enable)
        ULONG enet_clk_enable : 2;         // 10-11 enet clock (enet_clk_enable)
        ULONG epit1_clk_enable : 2;        // 12-13 epit1 clocks (epit1_clk_enable)
        ULONG epit2_clk_enable : 2;        // 14-15 epit2 clocks (epit2_clk_enable)
        ULONG esai_clk_enable : 2;         // 16-17 esai clocks (esai_clk_enable)
        ULONG reserved1 : 2;               // 18-19
        ULONG gpt_clk_enable : 2;          // 20-21 gpt bus clock (gpt_clk_enable)
        ULONG gpt_serial_clk_enable : 2;   // 22-23 gpt serial clock (gpt_serial_clk_enable)
        ULONG gpu2d_clk_enable : 2;        // 24-25 gpu2d clock (gpu2d_clk_enable)
        ULONG gpu3d_clk_enable : 2;        // 26-27 gpu3d clock (gpu3d_clk_enable)
        ULONG reserved2 : 4;               // 28-31
        // MSB
    };
};

union MX6_CCM_CCGR2_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG hdmi_tx_enable : 2;          // 0-1 hdmi_tx_iahbclk, hdmi_tx_ihclk clock (hdmi_tx_enable)
        ULONG reserved1 : 2;               // 2-3
        ULONG hdmi_tx_isfrclk_enable : 2;  // 4-5 hdmi_tx_isfrclk clock (hdmi_tx_isfrclk_enable)
        ULONG i2c1_serial_clk_enable : 2;  // 6-7 i2c1_serial clock (i2c1_serial_clk_enable)
        ULONG i2c2_serial_clk_enable : 2;  // 8-9 i2c2_serial clock (i2c2_serial_clk_enable)
        ULONG i2c3_serial_clk_enable : 2;  // 10-11 i2c3_serial clock (i2c3_serial_clk_enable)
        ULONG iim_clk_enable : 2;          // 12-13 OCOTP_CTRL clock (iim_clk_enable)
        ULONG iomux_ipt_clk_io_enable : 2; // 14-15 iomux_ipt_clk_io clock (iomux_ipt_clk_io_enable)
        ULONG ipmux1_clk_enable : 2;       // 16-17 ipmux1 clock (ipmux1_clk_enable)
        ULONG ipmux2_clk_enable : 2;       // 18-19 ipmux2 clock (ipmux2_clk_enable)
        ULONG ipmux3_clk_enable : 2;       // 20-21 ipmux3 clock (ipmux3_clk_enable)
        ULONG ipsync_ip2apb_tzasc1_ipg_master_clk_enable : 2;  // 22-23 ipsync_ip2apb_tzasc1_ipg clocks (ipsync_ip2apb_tzasc1_ipg_master_clk_enable)
        ULONG ipsync_ip2apb_tzasc2_ipg_master_clk_enable : 2;  // 24-25 ipsync_ip2apb_tzasc2_ipg clocks (ipsync_ip2apb_tzasc2_ipg_master_clk_enable) >
        ULONG ipsync_vdoa_ipg_master_clk_enable : 2;  // 26-27 ipsync_vdoa_ipg clocks (ipsync_vdoa_ipg_master_clk_enable)
        ULONG reserved2 : 4;               // 28-31
        // MSB
    };
};

// CBCDR.axi_sel
enum MX6_CCM_AXI_SEL {
    MX6_CCM_AXI_SEL_PERIPH_CLK,
    MX6_CCM_AXI_SEL_AXI_ALT,
};

union MX6_CCM_CBCDR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG periph2_clk2_podf : 3;       // 0-2 Divider for periph2_clk2 podf
        ULONG mmdc_ch1_axi_podf : 3;       // 3-5 Divider for mmdc_ch1_axi podf
        ULONG axi_sel : 1;                 // 6 AXI clock source select (MX6_CCM_AXI_SEL)
        ULONG axi_alt_sel : 1;             // 7 AXI alternative clock select
        ULONG ipg_podf : 2;                // 8-9 Divider for ipg podf.
        ULONG ahb_podf : 3;                // 10-12 Divider for AHB PODF.
        ULONG reserved1 : 3;               // 13-15
        ULONG axi_podf : 3;                // 16-18 Divider for axi podf
        ULONG mmdc_ch0_axi_podf : 3;       // 19-21 Divider for mmdc_ch0_axi podf.
        ULONG reserved2 : 3;               // 22-24
        ULONG periph_clk_sel : 1;          // 25 Selector for peripheral main clock (source of MMDC_CH0_CLK_ROOT).
        ULONG periph2_clk_sel : 1;         // 16 Selector for peripheral2 main clock (source of mmdc_ch1_clk_root
        ULONG periph_clk2_podf : 3;        // 27-29 Divider for periph2 clock podf.
        ULONG reserved3 : 2;               // 30-31
        // MSB
    };
};

enum MX6_CCM_CLPCR_LPM {
    MX6_CCM_CLPCR_LPM_RUN = 0,
    MX6_CCM_CLPCR_LPM_WAIT = 1,
    MX6_CCM_CLPCR_LPM_STOP = 2,
};

union MX6_CCM_CLPCR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG LPM : 2;                      // 0-1 Low power mode (MX6_CCM_CLPCR_LPM)
        ULONG reserved1 : 3;                // 2-4
        ULONG ARM_clk_dis_on_lpm : 1;       // 5 Disable ARM clocks on wait mode
        ULONG SBYOS : 1;                    // 6 Standby clock oscillator bit
        ULONG dis_ref_osc : 1;              // 7 Disable external high frequency oscillator
        ULONG VSTBY : 1;                    // 8 Voltage standby request bit
        ULONG stby_count : 2;               // 9-10 Standby counter definition
        ULONG cosc_pwrdown : 1;             // 11 Power down on-chip oscillator in run mode
        ULONG reserved2 : 4;                // 12-15
        ULONG wb_per_at_lpm : 1;            // 16 Enable periphery charge pump for well biasing at low power mode (stop or wait)
        ULONG reserved3 : 2;                // 17-18
        ULONG bypass_mmdc_ch0_lpm_hs : 1;   // 19 Bypass handshake with mmdc_ch0 on next entrance to low power mode
        ULONG reserved4 : 1;                // 20
        ULONG bypass_mmdc_ch1_lpm_hs : 1;   // 21 Bypass handshake with mmdc_ch1 on next entrance to low power mode
        ULONG mask_core0_wfi : 1;           // 22 Mask WFI of core0 for entering low power mode
        ULONG mask_core1_wfi : 1;           // 23 Mask WFI of core1 for entering low power mode
        ULONG mask_core2_wfi : 1;           // 24 Mask WFI of core2 for entering low power mode
        ULONG mask_core3_wfi : 1;           // 25 Mask WFI of core3 for entering low power mode
        ULONG mask_scu_idle : 1;            // 26 Mask SCU IDLE for entering low power mode
        ULONG mask_l2cc_idle : 1;           // 27 Mask L2CC IDLE for entering low power mode
        ULONG reserved5 : 4;                // 28-31
        // MSB
    };
};

// CCOSR.CLKO1_SEL
enum MX6_CCM_CLKO1_SEL {
    MX6_CCM_CLKO1_SEL_PLL3_SW_CLK_2,
    MX6_CCM_CLKO1_SEL_PLL2_MAIN_CLK_2,
    MX6_CCM_CLKO1_SEL_PLL1_MAIN_CLK_2,
    MX6_CCM_CLKO1_SEL_PLL5_MAIN_CLK_2,
    MX6_CCM_CLKO1_SEL_VIDEO_27M_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_AXI_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_ENFC_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_IPU1_DI0_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_IPU1_DI1_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_IPU2_DI0_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_IPU2_DI1_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_AHB_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_IPG_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_PERCLK_ROOT,
    MX6_CCM_CLKO1_SEL_CKIL_SYNC_CLK_ROOT,
    MX6_CCM_CLKO1_SEL_PLL4_MAIN_CLK,
};

// CCOSR.CLK_OUT_SEL
enum MX6_CCM_CLK_OUT_SEL {
    MX6_CCM_CLK_OUT_SEL_CCM_CLKO1,
    MX6_CCM_CLK_OUT_SEL_CCM_CLKO2,
};

// CCOSR.CLKO2_SEL
enum MX6_CCM_CLKO2_SEL {
    MX6_CCM_CLKO2_SEL_MMDC_CH0_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_MMDC_CH1_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_USDHC4_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_USDHC1_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_GPU2D_AXI_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_WRCK_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_ECSPI_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_GPU3D_AXI_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_USDHC3_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_125M_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_ARM_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_IPU1_HSP_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_IPU2_HSP_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_VDO_AXI_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_OSC_CLK,
    MX6_CCM_CLKO2_SEL_GPU2D_CORE_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_GPU3D_CORE_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_USDHC2_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_SSI1_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_SSI2_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_SSI3_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_GPU3D_SHADER_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_VPU_AXI_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_CAN_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_LDB_DI0_SERIAL_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_LDB_DI1_SERIAL_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_ESAI_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_ACLK_EIM_SLOW_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_UART_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_SPDIF0_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_SPDIF1_CLK_ROOT,
    MX6_CCM_CLKO2_SEL_HSI_TX_CLK_ROOT,
};

union MX6_CCM_CCOSR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG CLKO1_SEL : 4;               // 0-3 Selection of the clock to be generated on CCM_CLKO1 (MX6_CCM_CLKO1_SEL)
        ULONG CLKO1_DIV : 3;               // 4-6 Setting the divider of CCM_CLKO1
        ULONG CLKO1_EN : 1;                // 7 Enable of CCM_CLKO1 clock
        ULONG CLK_OUT_SEL : 1;             // 8 CCM_CLKO1 output to reflect CCM_CLKO1 or CCM_CLKO2 clocks
        ULONG reserved1 : 7;               // 9-15
        ULONG CLKO2_SEL : 5;               // 16-20 Selection of the clock to be generated on CCM_CLKO2 (MX6_CCM_CLKO2_SEL)
        ULONG CLKO2_DIV : 3;               // 21-23 Setting the divider of CCM_CLKO2
        ULONG CLKO2_EN : 1;                // 24 Enable of CCM_CLKO2 clock
        ULONG reserved2 : 7;               // 25-31
        // MSB
    };
};

union MX6_CCM_CGPR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG pmic_delay_scaler : 1;        // 0 Defines clock dividion of clock for stby_count (pmic delay counter)
        ULONG must_be_one : 1;              // 1 As per datasheet
        ULONG mmdc_ext_clk_dis : 1;         // 2 Disable external clock driver of MMDC during STOP mode
        ULONG reserved1 : 1;                // 3
        ULONG efuse_prog_supply_gate : 1;   // 4 Defines the value of the output signal cgpr_dout[4
        ULONG reserved2 : 11;               // 5-15
        ULONG FPL : 1;                      // 16 Fast PLL enable.
        ULONG INT_MEM_CLK_LPM : 1;          // 17 Control for the Deep Sleep signal to the ARM Platform memories
        ULONG reserved3 : 14;               // 18 - 31
        // MSB
    };
};

union MX6_CCM_CCGR3_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG ipu1_ipu_clk_enable : 2;     // 0-1 ipu1_ipu clock (ipu1_ipu_clk_enable)
        ULONG ipu1_ipu_di0_clk_enable : 2; // 2-3 ipu1_di0 clock and pre-clock (ipu1_ipu_di0_clk_enable)
        ULONG ipu1_ipu_di1_clk_enable : 2; // 4-5 ipu1_di1 clock and pre-clock (ipu1_ipu_di1_clk_enable)
        ULONG ipu2_ipu_clk_enable : 2;     // 6-7 ipu2_ipu clock (ipu2_ipu_clk_enable)
        ULONG ipu2_ipu_di0_clk_enable : 2; // 8-9 ipu2_di0 clock and pre-clock (ipu2_ipu_di0_clk_enable)
        ULONG ipu2_ipu_di1_clk_enable : 2; // 10-11 ipu2_di1 clock and pre-clock (ipu2_ipu_di1_clk_enable)
        ULONG ldb_di0_clk_enable : 2;      // 12-13 ldb_di0 clock (ldb_di0_clk_enable)
        ULONG ldb_di1_clk_enable : 2;      // 14-15 ldb_di1 clock (ldb_di1_clk_enable)
        ULONG mipi_core_cfg_clk_enable : 2; // 16-17 mipi_core_cfg clock (mipi_core_cfg_clk_enable)
        ULONG mlb_clk_enable : 2;          // 18-19 mlb clock (mlb_clk_enable)
        ULONG mmdc_core_aclk_fast_core_p0_enable : 2; // 20-21 mmdc_core_aclk_fast_core_p0 clock (mmdc_core_aclk_fast_core_p0_enable)
        ULONG reserved1 : 2;               // 22-23
        ULONG mmdc_core_ipg_clk_p0_enable : 2; // 24-25 mmdc_core_ipg_clk_p0 clock (mmdc_core_ipg_clk_p0_enable)
        ULONG reserved2 : 2;               // 26-27
        ULONG ocram_clk_enable : 2;        // 28-29 ocram clock (ocram_clk_enable)
        ULONG openvgaxiclk_clk_root_enable : 2; // 30-31 openvgaxiclk clock (openvgaxiclk_clk_root_enable)
        // MSB
    };
};

struct MX6_CCM_REGISTERS {
    ULONG CCR;                             // 0x00 CCM Control Register (CCM_CCR)
    ULONG CCDR;                            // 0x04 CCM Control Divider Register (CCM_CCDR)
    ULONG CSR;                             // 0x08 CCM Status Register (CCM_CSR)
    ULONG CCSR;                            // 0x0C CCM Clock Switcher Register (CCM_CCSR)
    ULONG CACRR;                           // 0x10 CCM Arm Clock Root Register (CCM_CACRR)
    ULONG CBCDR;                           // 0x14 CCM Bus Clock Divider Register (CCM_CBCDR)
    ULONG CBCMR;                           // 0x18 CCM Bus Clock Multiplexer Register (CCM_CBCMR)
    ULONG CSCMR1;                          // 0x1C CCM Serial Clock Multiplexer Register 1 (CCM_CSCMR1)
    ULONG CSCMR2;                          // 0x20 CCM Serial Clock Multiplexer Register 2 (CCM_CSCMR2)
    ULONG CSCDR1;                          // 0x24 CCM Serial Clock Divider Register 1 (CCM_CSCDR1)
    ULONG CS1CDR;                          // 0x28 CCM SSI1 Clock Divider Register (CCM_CS1CDR)
    ULONG CS2CDR;                          // 0x2C CCM SSI2 Clock Divider Register (CCM_CS2CDR)
    ULONG CDCDR;                           // 0x30 CCM D1 Clock Divider Register (CCM_CDCDR)
    ULONG CHSCCDR;                         // 0x34 CCM HSC Clock Divider Register (CCM_CHSCCDR)
    ULONG CSCDR2;                          // 0x38 CCM Serial Clock Divider Register 2 (CCM_CSCDR2)
    ULONG CSCDR3;                          // 0x3C CCM Serial Clock Divider Register 3 (CCM_CSCDR3)
    ULONG reserved1[2];
    ULONG CDHIPR;                          // 0x48 CCM Divider Handshake In-Process Register (CCM_CDHIPR)
    ULONG reserved2[2];
    ULONG CLPCR;                           // 0x54 CCM Low Power Control Register (CCM_CLPCR)
    ULONG CISR;                            // 0x58 CCM Interrupt Status Register (CCM_CISR)
    ULONG CIMR;                            // 0x5C CCM Interrupt Mask Register (CCM_CIMR)
    ULONG CCOSR;                           // 0x60 CCM Clock Output Source Register (CCM_CCOSR)
    ULONG CGPR;                            // 0x64 CCM General Purpose Register (CCM_CGPR)
    ULONG CCGR[7];                         // 0x68-7C CCM Clock Gating Register 0-6 (CCM_CCGR0-CCM_CCGR6)
    ULONG reserved3[1];
    ULONG CMEOR;                           // 0x88 CCM Module Enable Override Register (CCM_CMEOR)
};

//
// CCM Analog
//

enum MX6_PLL_BYPASS_CLK_SRC {
    MX6_PLL_BYPASS_CLK_SRC_REF_CLK_24M,
    MX6_PLL_BYPASS_CLK_SRC_CLK1,
    MX6_PLL_BYPASS_CLK_SRC_CLK2,
    MX6_PLL_BYPASS_CLK_SRC_XOR,
};


union MX6_CCM_ANALOG_PLL_ARM_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG DIV_SELECT : 7;              // 0-6  Valid range for divider value: 54-108. Fout = Fin * div_select/2.0
        ULONG reserved1 : 5;               // 7-11
        ULONG POWERDOWN : 1;               // 12 Powers down the PLL.
        ULONG ENABLE: 1;                   // 13 Enable the clock output.
        ULONG BYPASS_CLK_SRC : 2;          // 14-15 Determines the bypass and PLL reference clock source.
        ULONG BYPASS : 1;                  // 16 Bypass the PLL.
        ULONG LVDS_SEL : 1;                // 17 Analog Debug Bit
        ULONG LVDS_24MHZ_SEL : 1;          // 18 Analog Debug Bit
        ULONG reserved2 : 1;               // 19 PLL_SEL (Reserved)
        ULONG reserved3 : 11;              // 20-30
        ULONG LOCK : 1;                    // 31 1 - PLL is currently locked. 0 - PLL is not currently locked.
        // MSB
    };
};

union MX6_CCM_ANALOG_PLL_SYS_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG DIV_SELECT : 1;              // 0 0 - Fout=Fref*20; 1 - Fout=Fref*22.
        ULONG reserved1 : 11;              // 1-11
        ULONG POWERDOWN : 1;               // 12 Powers down the PLL.
        ULONG ENABLE : 1;                  // 13 Enable PLL output
        ULONG BYPASS_CLK_SRC : 2;          // 14-15 Determines the bypass source.
        ULONG BYPASS : 1;                  // 16 Bypass the PLL.
        ULONG reserved2 : 1;               // 17
        ULONG PFD_OFFSET_EN : 1;           // 18 Enables an offset in the phase frequency detector
        ULONG reserved3 : 12;              // 19-30
        ULONG LOCK : 1;                    // 31 1 - PLL is currently locked; 0 - PLL is not currently locked.
        // MSB
    };
};

union MX6_CCM_ANALOG_PLL_USB1_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG DIV_SELECT : 2;              // 0 0 - Fout=Fref*20; 1 - Fout=Fref*22.
        ULONG reserved1 : 4;               // 2-5
        ULONG EN_USB_CLKS : 1;             // 6 Powers the 9-phase PLL outputs for USBPHYn
        ULONG reserved2 : 5;               // 7-11
        ULONG POWER : 1;                   // 12 Powers up the PLL.
        ULONG ENABLE : 1;                  // 13 Enable the PLL clock output
        ULONG BYPASS_CLK_SRC : 2;          // 14-15 Determines the bypass source
        ULONG BYPASS : 1;                  // 16 Bypass the PLL.
        ULONG reserved3 : 14;              // 17-30
        ULONG LOCK : 1;                    // 31 1 - PLL is currently locked
        // MSB
    };
};

union MX6_CCM_PFD_480_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG PFD0_FRAC : 6;               // 0-5 fractional divide value. The resulting frequency shall be 528*18/PFD0_FRAC where PFD0_FRAC is in the range 12-35.
        ULONG PFD0_STABLE : 1;             // 6
        ULONG PFD0_CLKGATE : 1;            // 7 Set to 1 to gate ref_pfd0
        ULONG PFD1_FRAC : 6;               // 8-13 fractional divide value
        ULONG PFD1_STABLE : 1;             // 14
        ULONG PFD1_CLKGATE : 1;            // 15 Set to 1 to gate ref_pfd1
        ULONG PFD2_FRAC : 6;               // 16-21 fractional divide value
        ULONG PFD2_STABLE : 1;             // 22
        ULONG PFD2_CLKGATE : 1;            // 23 Set to 1 to gate ref_pfd2
        ULONG PFD3_FRAC : 6;               // 24-29 fractional divide value
        ULONG PFD3_STABLE : 1;             // 30
        ULONG PFD3_CLKGATE : 1;            // 31 Set to 1 to gate ref_pfd3
        // MSB
    };
};

union MX6_CCM_PFD_528_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG PFD0_FRAC : 6;               // 0-5 fractional divide value. The resulting frequency shall be 528*18/PFD0_FRAC where PFD0_FRAC is in the range 12-35.
        ULONG PFD0_STABLE : 1;             // 6
        ULONG PFD0_CLKGATE : 1;            // 7 Set to 1 to gate ref_pfd0
        ULONG PFD1_FRAC : 6;               // 8-13 fractional divide value
        ULONG PFD1_STABLE : 1;             // 14
        ULONG PFD1_CLKGATE : 1;            // 15 Set to 1 to gate ref_pfd1
        ULONG PFD2_FRAC : 6;               // 16-21 fractional divide value
        ULONG PFD2_STABLE : 1;             // 22
        ULONG PFD2_CLKGATE : 1;            // 23 Set to 1 to gate ref_pfd2
        ULONG reserved : 8;                // 24-31
        // MSB
    };
};

union MX6_PMU_REG_CORE_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG REG0_TARG : 5;               // 0-4 target voltage for the ARM core power domain
        ULONG reserved1 : 4;               // 5-8
        ULONG REG1_TARG : 5;               // 9-13 target voltage for the VPU/GPU power domain
        ULONG reserved2 : 4;               // 14-17
        ULONG REG2_TARG : 5;               // 18-22 target voltage for the SOC power domain
        ULONG reserved3 : 6;               // 23-28
        ULONG FET_ODRIVE : 1;              // 29 increases the gate drive on power gating FET
        ULONG reserved4 : 2;               // 30-31
        // MSB
    };
};

union MX6_PMU_MISC0_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG REFTOP_PWD : 1;               // 0 Control bit to power-down the analog bandgap reference circuitry
        ULONG reserved1 : 2;                // 1-2
        ULONG REFTOP_SELFBIASOFF : 1;       // 3 Control bit to disable the self-bias circuit in the analog bandgap
        ULONG REFTOP_VBGADJ : 3;            // 4-6
        ULONG REFTOP_VBGUP : 1;             // 7 Status bit that signals the analog bandgap voltage is up and stable.
        ULONG reserved2 : 4;                // 8-11
        ULONG STOP_MODE_CONFIG : 1;         // 12 Configure the analog behavior in stop mode
        ULONG reserved3 : 1;                // 13
        ULONG OSC_I : 2;                    // 14-15 This field determines the bias current in the 24MHz oscillator.
        ULONG OSC_XTALOK : 1;               // 16 Status bit that signals that the output of the 24-MHz crystal oscillator is stable.
        ULONG OSC_XTALOK_EN : 1;            // 17 This bit enables the detector that signals when the 24MHz crystal oscillator is stable.
        ULONG WBCP_VPW_THRESH : 2;          // 18-19 This signal alters the voltage that the pwell is charged pumped to
        ULONG reserved4 : 5;                // 20-24
        ULONG CLKGATE_CTRL : 1;             // 25 This bit allows disabling the clock gate (always ungated) for the xtal 24MHz clock
        ULONG CLKGATE_DELAY : 3;            // 26-28
        ULONG reserved5 : 3;                // 29-31 reserved
        // MSB
    };
};

enum MX6_PMU_MISC0_BITS : ULONG {
    MX6_PMU_MISC0_STOP_MODE_CONFIG          = (1 << 12)
};

struct MX6_CCM_ANALOG_REGISTERS {
    ULONG PLL_ARM;                         // 0x000 Analog ARM PLL control Register (CCM_ANALOG_PLL_ARM)
    ULONG PLL_ARM_SET;                     // 0x004 Analog ARM PLL control Register (CCM_ANALOG_PLL_ARM_SET)
    ULONG PLL_ARM_CLR;                     // 0x008 Analog ARM PLL control Register (CCM_ANALOG_PLL_ARM_CLR)
    ULONG PLL_ARM_TOG;                     // 0x00C Analog ARM PLL control Register (CCM_ANALOG_PLL_ARM_TOG)
    ULONG PLL_USB1;                        // 0x010 Analog USB1 480MHz PLL Control Register (CCM_ANALOG_PLL_USB1)
    ULONG PLL_USB1_SET;                    // 0x014 Analog USB1 480MHz PLL Control Register (CCM_ANALOG_PLL_USB1_SET)
    ULONG PLL_USB1_CLR;                    // 0x018 Analog USB1 480MHz PLL Control Register (CCM_ANALOG_PLL_USB1_CLR)
    ULONG PLL_USB1_TOG;                    // 0x01C Analog USB1 480MHz PLL Control Register (CCM_ANALOG_PLL_USB1_TOG)
    ULONG PLL_USB2;                        // 0x020 Analog USB2 480MHz PLL Control Register (CCM_ANALOG_PLL_USB2)
    ULONG PLL_USB2_SET;                    // 0x024 Analog USB2 480MHz PLL Control Register (CCM_ANALOG_PLL_USB2_SET)
    ULONG PLL_USB2_CLR;                    // 0x028 Analog USB2 480MHz PLL Control Register (CCM_ANALOG_PLL_USB2_CLR)
    ULONG PLL_USB2_TOG;                    // 0x02C Analog USB2 480MHz PLL Control Register (CCM_ANALOG_PLL_USB2_TOG)
    ULONG PLL_SYS;                         // 0x030 Analog System PLL Control Register (CCM_ANALOG_PLL_SYS)
    ULONG PLL_SYS_SET;                     // 0x034 Analog System PLL Control Register (CCM_ANALOG_PLL_SYS_SET)
    ULONG PLL_SYS_CLR;                     // 0x038 Analog System PLL Control Register (CCM_ANALOG_PLL_SYS_CLR)
    ULONG PLL_SYS_TOG;                     // 0x03C Analog System PLL Control Register (CCM_ANALOG_PLL_SYS_CLR)
    ULONG PLL_SYS_SS;                      // 0x040 528MHz System PLL Spread Spectrum Register (CCM_ANALOG_PLL_SYS_SS)
    ULONG reserved1[3];
    ULONG PLL_SYS_NUM;                     // 0x050 Numerator of 528MHz System PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_SYS_NUM)
    ULONG reserved2[3];
    ULONG PLL_SYS_DENOM;                   // 0x060 Denominator of 528MHz System PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_SYS_DENOM)
    ULONG reserved3[3];
    ULONG PLL_AUDIO;                       // 0x070 Analog Audio PLL control Register (CCM_ANALOG_PLL_AUDIO)
    ULONG PLL_AUDIO_SET;                   // 0x074 Analog Audio PLL control Register (CCM_ANALOG_PLL_AUDIO_SET)
    ULONG PLL_AUDIO_CLR;                   // 0x078 Analog Audio PLL control Register (CCM_ANALOG_PLL_AUDIO_CLR)
    ULONG PLL_AUDIO_TOG;                   // 0x07C Analog Audio PLL control Register (CCM_ANALOG_PLL_AUDIO_TOG)
    ULONG PLL_AUDIO_NUM;                   // 0x080 Numerator of Audio PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_AUDIO_NUM)
    ULONG reserved4[3];
    ULONG PLL_AUDIO_DENOM;                 // 0x090 Denominator of Audio PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_AUDIO_DENOM)
    ULONG reserved5[3];
    ULONG PLL_VIDEO;                       // 0x0A0 Analog Video PLL control Register (CCM_ANALOG_PLL_VIDEO)
    ULONG PLL_VIDEO_SET;                   // 0x0A4 Analog Video PLL control Register (CCM_ANALOG_PLL_VIDEO_SET)
    ULONG PLL_VIDEO_CLR;                   // 0x0A8 Analog Video PLL control Register (CCM_ANALOG_PLL_VIDEO_CLR)
    ULONG PLL_VIDEO_TOG;                   // 0x0AC Analog Video PLL control Register (CCM_ANALOG_PLL_VIDEO_TOG)
    ULONG PLL_VIDEO_NUM;                   // 0x0B0 Numerator of Video PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_VIDEO_NUM)
    ULONG reserved6[3];
    ULONG PLL_VIDEO_DENOM;                 // 0x0C0 Denominator of Video PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_VIDEO_DENOM)
    ULONG reserved7[3];
    ULONG PLL_MLB;                         // 0x0D0 MLB PLL Control Register (CCM_ANALOG_PLL_MLB)
    ULONG PLL_MLB_SET;                     // 0x0D4 MLB PLL Control Register (CCM_ANALOG_PLL_MLB_SET)
    ULONG PLL_MLB_CLR;                     // 0x0D8 MLB PLL Control Register (CCM_ANALOG_PLL_MLB_CLR)
    ULONG PLL_MLB_TOG;                     // 0x0DC MLB PLL Control Register (CCM_ANALOG_PLL_MLB_TOG)
    ULONG PLL_ENET;                        // 0x0E0 Analog ENET PLL Control Register (CCM_ANALOG_PLL_ENET)
    ULONG PLL_ENET_SET;                    // 0x0E4 Analog ENET PLL Control Register (CCM_ANALOG_PLL_ENET_SET)
    ULONG PLL_ENET_CLR;                    // 0x0E8 Analog ENET PLL Control Register (CCM_ANALOG_PLL_ENET_CLR)
    ULONG PLL_ENET_TOG;                    // 0x0EC Analog ENET PLL Control Register (CCM_ANALOG_PLL_ENET_TOG)
    ULONG PFD_480;                         // 0x0F0 480MHz Clock (PLL3) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_480)
    ULONG PFD_480_SET;                     // 0x0F4 480MHz Clock (PLL3) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_480_SET)
    ULONG PFD_480_CLR;                     // 0x0F8 480MHz Clock (PLL3) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_480_CLR)
    ULONG PFD_480_TOG;                     // 0x0FC 480MHz Clock (PLL3) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_480_TOG)
    ULONG PFD_528;                         // 0x100 528MHz Clock (PLL2) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_528)
    ULONG PFD_528_SET;                     // 0x104 528MHz Clock (PLL2) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_528_SET)
    ULONG PFD_528_CLR;                     // 0x108 528MHz Clock (PLL2) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_528_CLR)
    ULONG PFD_528_TOG;                     // 0x10C 528MHz Clock (PLL2) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_528_TOG)
    ULONG PMU_REG_1P1;                     // 0x110 Regulator 1P1 Register (PMU_REG_1P1)
    ULONG PMU_REG_1P1_SET;                 // 0x114
    ULONG PMU_REG_1P1_CLR;                 // 0x118
    ULONG PMU_REG_1P1_TOG;                 // 0x11C
    ULONG PMU_REG_3P0;                     // 0X120 Regulator 3P0 Register (PMU_REG_3P0)
    ULONG PMU_REG_3P0_SET;                 // 0x124
    ULONG PMU_REG_3P0_CLR;                 // 0x128
    ULONG PMU_REG_3P0_TOG;                 // 0x12C
    ULONG PMU_REG_2P5;                     // 0x130 Regulator 2P5 Register (PMU_REG_2P5)
    ULONG PMU_REG_2P5_SET;                 // 0x134
    ULONG PMU_REG_2P5_CLR;                 // 0x138
    ULONG PMU_REG_2P5_TOG;                 // 0x13C
    ULONG PMU_REG_CORE;                    // 0x140 Digital Regulator Core Register (PMU_REG_CORE)
    ULONG PMU_REG_CORE_SET;                // 0x144
    ULONG PMU_REG_CORE_CLR;                // 0x148
    ULONG PMU_REG_CORE_TOG;                // 0x14C
    ULONG MISC0;                           // 0x150 Miscellaneous Register 0 (CCM_ANALOG_MISC0)
    ULONG MISC0_SET;                       // 0x154 Miscellaneous Register 0 (CCM_ANALOG_MISC0_SET)
    ULONG MISC0_CLR;                       // 0x158 Miscellaneous Register 0 (CCM_ANALOG_MISC0_CLR)
    ULONG MISC0_TOG;                       // 0x15C Miscellaneous Register 0 (CCM_ANALOG_MISC0_TOG)
    ULONG MISC1;                           // 0x160 Miscellaneous Register 1 (CCM_ANALOG_MISC1)
    ULONG MISC1_SET;                       // 0x164 Miscellaneous Register 1 (CCM_ANALOG_MISC1_SET)
    ULONG MISC1_CLR;                       // 0x168 Miscellaneous Register 1 (CCM_ANALOG_MISC1_CLR)
    ULONG MISC1_TOG;                       // 0x16C Miscellaneous Register 1 (CCM_ANALOG_MISC1_TOG)
    ULONG MISC2;                           // 0x170 Miscellaneous Register 2 (CCM_ANALOG_MISC2)
    ULONG MISC2_SET;                       // 0x174 Miscellaneous Register 2 (CCM_ANALOG_MISC2_SET)
    ULONG MISC2_CLR;                       // 0x178 Miscellaneous Register 2 (CCM_ANALOG_MISC2_CLR)
    ULONG MISC2_TOG;                       // 0x17C Miscellaneous Register 2 (CCM_ANALOG_MISC2_TOG)
};

//
// General Power Controller (GPC)
//

#define MX6_GPC_BASE 0x020DC000
#define MX6_GPC_LENGTH 0x1000

union MX6_GPC_PGC_CTRL_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG PCR : 1;                     // 0 Power Control
        ULONG reserved : 31;               // 1-31
        // MSB
    };
};

#define MX6_GPC_PGC_PUPSCR_SW_DEFAULT 1
#define MX6_GPC_PGC_PUPSCR_SW2ISO_DEFAULT 0xf

union MX6_GPC_PGC_PUPSCR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG SW : 6;                      // 0-5 number of IPG clock cycles before asserting power toggle on/off signal (switch_b)
        ULONG reserved1 : 2;               // 6-7
        ULONG SW2ISO : 6;                  // 8-13 IPG clock cycles before negating isolation
        ULONG reserved2 : 18;              // 14-31
        // MSB
    };
};

#define MX6_GPC_PGC_PDNSCR_ISO_DEFAULT 1
#define MX6_GPC_PGC_PDNSCR_ISO2SW_DEFAULT 1

union MX6_GPC_PGC_PDNSCR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG ISO : 6;                     // 0-5 number of IPG clocks before isolation
        ULONG reserved1 : 2;               // 6-7
        ULONG ISO2SW : 6;                  // 8-13 number of IPG clocks before negating power toggle on/off signal (switch_b)
        ULONG reserved2 : 18;              // 14-31
        // MSB
    };
};

struct MX6_GPC_PGC_REGISTERS {
    ULONG CTRL;                            // 0x0 PGC Control Register (PGC_GPU/CPU_CTRL)
    ULONG PUPSCR;                          // 0x4 Power Up Sequence Control Register (PGC_GPU/CPU_PUPSCR)
    ULONG PDNSCR;                          // 0x8 Pull Down Sequence Control Register (PGC_GPU/CPU_PDNSCR)
    ULONG SR;                              // 0xC Power Gating Controller Status Register (PGC_GPU/CPU_SR)
};

union MX6_GPC_CNTR_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG gpu_vpu_pdn_req : 1;         // 0 GPU/VPU Power Down request. Self-cleared bit.
        ULONG gpu_vpu_pup_req : 1;         // 1 GPU/VPU Power Up request. Self-cleared bit.
        ULONG reserved1 : 14;              // 2-15
        ULONG DVFS0CR : 1;                 // 16 DVFS0 (ARM) Change request (bit is read-only)
        ULONG reserved2 : 4;               // 17-20
        ULONG GPCIRQM : 1;                 // 21 GPC interrupt/event masking
        ULONG reserved3 : 10;              // 22-31
        // MSB
    };
};

struct MX6_GPC_REGISTERS {
    ULONG CNTR;                            // 0x000 GPC Interface control register (GPC_CNTR)
    ULONG PGR;                             // 0x004 GPC Power Gating Register (GPC_PGR)
    ULONG IMR[4];                          // 0x008 - 0x018 IRQ masking register 1-4 (GPC_IMR1..4)
    ULONG ISR[4];                          // 0x018 - 0x028 IRQ status resister 1-4 (GPC_ISR1..4)
    ULONG reserved1[142];
    MX6_GPC_PGC_REGISTERS PGC_GPU;          // 0x260-0x26C GPU PGC Control
    ULONG reserved2[12];
    MX6_GPC_PGC_REGISTERS PGC_CPU;          // 0x2A0-0x2AC CPU PGC Control
};

//
// Memory Controller Registers (MMDC)
//

union MX6_MMDC_MADPCR0_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG DBG_EN : 1;         // 0 Debug and profiling Enable
        ULONG DBG_RST : 1;        // 1 Debug and profiling reset
        ULONG PRF_FRZ : 1;        // 2 Profiling freeze
        ULONG CYC_OVF : 1;        // 3 Total Profiling Cycles Count Overflow
        ULONG reserved1 : 4;      // 4-7
        ULONG SBS_EN : 1;         // 8 Step By Step debug Enable
        ULONG SBS : 1;            // 9 Step By Step trigger
        ULONG reserved2 : 22;     // 10-31
        // MSB
    };
};

union MX6_MMDC_MADPCR1_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG PRF_AXI_ID : 16;        // 0-15 Profiling AXI ID
        ULONG PRF_AXI_ID_MASK : 16;   // 16-31 Profiling AXI ID Mask
        // MSB
    };
};

struct MX6_MMDC_REGISTERS {
    ULONG reserved1[260];
    ULONG MADPCR0;        // 0x410 MMDC Core Debug and Profiling Control Register 0 (MMDC1_MADPCR0)
    ULONG MADPCR1;        // 0x414 MMDC Core Debug and Profiling Control Register 1 (MMDC1_MADPCR1)
    ULONG MADPSR0;        // 0x418 MMDC Core Debug and Profiling Status Register 0 (MMDC1_MADPSR0)
    ULONG MADPSR1;        // 0x41C MMDC Core Debug and Profiling Status Register 1 (MMDC1_MADPSR1)
    ULONG MADPSR2;        // 0x420 MMDC Core Debug and Profiling Status Register 2 (MMDC1_MADPSR2)
    ULONG MADPSR3;        // 0x424 MMDC Core Debug and Profiling Status Register 3 (MMDC1_MADPSR3)
    ULONG MADPSR4;        // 0x428 MMDC Core Debug and Profiling Status Register 4 (MMDC1_MADPSR4)
    ULONG MADPSR5;        // 0x42C MMDC Core Debug and Profiling Status Register 5 (MMDC1_MADPSR5)
};

//
// Snoop Control Unit Registers (SCU)
//

//
// Offset from ARM MP base
//
#define MX6_SCU_OFFSET 0

union MX6_SCU_CONTROL_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG Enable : 1;                       // 0 SCU enable
        ULONG AddressFilterEnable : 1;          // 1 Address filtering enable
        ULONG RamParityEnable : 1;              // 2 SCU RAMs Parity enable
        ULONG SpeculativeLinefillEnable : 1;    // 3 SCU Speculative linefills enable
        ULONG ForcePort0 : 1;                   // 4 Force all Device to port0 enable
        ULONG StandbyEnable : 1;                // 5 SCU standby enable
        ULONG IcStandbyEnable : 1;              // 6 Interrupt controller standby enable
        ULONG reserved : 25;                    // 7-31
        // MSB
    };
};

struct MX6_SCU_REGISTERS {
    ULONG Control;
    ULONG Config;
    ULONG Status;
    ULONG Invalidate;
    ULONG FpgaRev;
};

//
// IOMUXC Registers
//

union MX6_IOMUXC_GPR1_REG {
    ULONG AsUlong;
    struct {
        // LSB
        ULONG ACT_CS0 : 1;                      // 0
        ULONG ADDRS0_10 : 2;                    // 1-2
        ULONG ACT_CS1 : 1;                      // 3
        ULONG ADDRS1_10 : 2;                    // 4-5
        ULONG ACT_CS2 : 1;                      // 6
        ULONG ADDRS2_10 : 2;                    // 7-8
        ULONG ACT_CS3 : 1;                      // 9
        ULONG ADDRS3_10 : 2;                    // 10-11
        ULONG GINT : 1;                         // 12
        ULONG USB_OTG_ID_SEL : 1;               // 13
        ULONG SYS_INT : 1;                      // 14
        ULONG USB_EXP_MODE : 1;                 // 15
        ULONG REF_SSP_EN : 1;                   // 16
        ULONG IPU_VPU_MUX : 1;                  // 17
        ULONG TEST_POWERDOWN : 1;               // 18
        ULONG MIPI_IPU1_MUX : 1;                // 19
        ULONG MIPI_IPU2_MUX : 1;                // 20
        ULONG ENET_CLK_SEL : 1;                 // 21
        ULONG EXC_MON : 1;                      // 22
        ULONG reserved1 : 1;                    // 23
        ULONG MIPI_DPI_OFF : 1;                 // 24
        ULONG MIPI_COLOR_SW : 1;                // 25
        ULONG APP_REQ_ENTR_L1 : 1;              // 26
        ULONG APP_READY_ENTR_L23 : 1;           // 27
        ULONG APP_REQ_EXIT_L1 : 1;              // 28
        ULONG reserved2 : 1;                    // 29
        ULONG APP_CLK_REQ_N : 1;                // 30
        ULONG CFG_L1_CLK_REMOVAL_EN : 1;        // 31
        // MSB
    };
};

struct MX6_IOMUXC_REGISTERS {
    ULONG Gpr[16];

    // Followed by pad mux and pad ctl registers
};

#define MX6DQ_IOMUXC_SW_MUX_CTL_PAD_GPIO06 0x230
#define MX6SDL_IOMUXC_SW_MUX_CTL_PAD_GPIO06 0x234

#define MX6_IOMUXC_SW_MUX_CTL_PAD_SION (1 << 4)

//
// CSU_CSL registers
//
union MX6_CSU_CSL_SLAVE_REG {
    USHORT AsUshort;
    struct {
        // LSB
        USHORT SUR_S : 1;       // 0
        USHORT SSR_S : 1;       // 1
        USHORT NUR_S : 1;       // 2
        USHORT NSR_S : 1;       // 3
        USHORT SUW_S : 1;       // 4
        USHORT SSW_S : 1;       // 5
        USHORT NUW_S : 1;       // 6
        USHORT NSW_S : 1;       // 7
        USHORT LOCK_S : 1;      // 8
        USHORT Reserved : 7;    // 9-15
    };
};

#define CSU_BASE_ADDRESS 0x021C0000
#define CSU_SIZE 0xA0
#define CSU_INDEX_INVALID 0xFFFF
#define CSU_SLAVE_LOW 0
#define CSU_SLAVE_HIGH 1
#define CSU_SLAVE_INDEX(num, isHigh) 2 * num + isHigh
#define CSU_NON_SECURE_MASK 0x00CC      // NUR_S | NSR_S | NUW_S | NSW_S

#define ACPI_OBJECT_NAME_STA  ((ULONG)'ATS_')

#include <poppack.h> // pshpack1.h
#pragma warning(pop) // nameless struct/union

#endif // _MX6PEP_HW_H_

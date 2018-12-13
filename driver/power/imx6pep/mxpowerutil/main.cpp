// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// mxpowerutil
//
//   Utility to dump clock and power information from the PEP.
//

#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>
#include <cfgmgr32.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <memory>

#include <wrl.h>

#include <initguid.h>
#include <mx6pephw.h>
#include <mx6pepioctl.h>
#include "mx6clktreehelper.h"
#include "util.h"

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

struct MX6_CCGR_INDEX {
    USHORT RegisterIndex;   // Register index (0-6)
    USHORT GateNumber;      // Gate number within register (0-15)
};

//
// Use Mx6CcgrIndexFromClkGate() to access
//
const MX6_CCGR_INDEX _mx6CcgrIndexMap[] = {
    {0, 0},  // MX6_AIPS_TZ1_CLK_ENABLE
    {0, 1},  // MX6_AIPS_TZ2_CLK_ENABLE
    {0, 2},  // MX6_APBHDMA_HCLK_ENABLE
    {0, 3},  // MX6_ASRC_CLK_ENABLE
    {0, 4},  // MX6_CAAM_SECURE_MEM_CLK_ENABLE
    {0, 5},  // MX6_CAAM_WRAPPER_ACLK_ENABLE
    {0, 6},  // MX6_CAAM_WRAPPER_IPG_ENABLE
    {0, 7},  // MX6_CAN1_CLK_ENABLE
    {0, 8},  // MX6_CAN1_SERIAL_CLK_ENABLE
    {0, 9},  // MX6_CAN2_CLK_ENABLE
    {0, 10}, // MX6_CAN2_SERIAL_CLK_ENABLE
    {0, 11}, // MX6_ARM_DBG_CLK_ENABLE
    {0, 12}, // MX6_DCIC1_CLK_ENABLE
    {0, 13}, // MX6_DCIC2_CLK_ENABLE
    {0, 14}, // MX6_DTCP_CLK_ENABLE
    {1, 0},  // MX6_ECSPI1_CLK_ENABLE
    {1, 1},  // MX6_ECSPI2_CLK_ENABLE
    {1, 2},  // MX6_ECSPI3_CLK_ENABLE
    {1, 3},  // MX6_ECSPI4_CLK_ENABLE
    {1, 4},  // MX6_ECSPI5_CLK_ENABLE
    {1, 5},  // MX6_ENET_CLK_ENABLE
    {1, 6},  // MX6_EPIT1_CLK_ENABLE
    {1, 7},  // MX6_EPIT2_CLK_ENABLE
    {1, 8},  // MX6_ESAI_CLK_ENABLE
    {1, 10}, // MX6_GPT_CLK_ENABLE
    {1, 11}, // MX6_GPT_SERIAL_CLK_ENABLE
    {1, 12}, // MX6_GPU2D_CLK_ENABLE
    {1, 13}, // MX6_GPU3D_CLK_ENABLE
    {2, 0},  // MX6_HDMI_TX_ENABLE
    {2, 2},  // MX6_HDMI_TX_ISFRCLK_ENABLE
    {2, 3},  // MX6_I2C1_SERIAL_CLK_ENABLE
    {2, 4},  // MX6_I2C2_SERIAL_CLK_ENABLE
    {2, 5},  // MX6_I2C3_SERIAL_CLK_ENABLE
    {2, 6},  // MX6_IIM_CLK_ENABLE
    {2, 7},  // MX6_IOMUX_IPT_CLK_IO_ENABLE
    {2, 8},  // MX6_IPMUX1_CLK_ENABLE
    {2, 9},  // MX6_IPMUX2_CLK_ENABLE
    {2, 10}, // MX6_IPMUX3_CLK_ENABLE
    {2, 11}, // MX6_IPSYNC_IP2APB_TZASC1_IPG_MASTER_CLK_ENABLE
    {2, 12}, // MX6_IPSYNC_IP2APB_TZASC2_IPG_MASTER_CLK_ENABLE
    {2, 13}, // MX6_IPSYNC_VDOA_IPG_MASTER_CLK_ENABLE
    {3, 0},  // MX6_IPU1_IPU_CLK_ENABLE
    {3, 1},  // MX6_IPU1_IPU_DI0_CLK_ENABLE
    {3, 2},  // MX6_IPU1_IPU_DI1_CLK_ENABLE
    {3, 3},  // MX6_IPU2_IPU_CLK_ENABLE
    {3, 4},  // MX6_IPU2_IPU_DI0_CLK_ENABLE
    {3, 5},  // MX6_IPU2_IPU_DI1_CLK_ENABLE
    {3, 6},  // MX6_LDB_DI0_CLK_ENABLE
    {3, 7},  // MX6_LDB_DI1_CLK_ENABLE
    {3, 8},  // MX6_MIPI_CORE_CFG_CLK_ENABLE
    {3, 9},  // MX6_MLB_CLK_ENABLE
    {3, 10}, // MX6_MMDC_CORE_ACLK_FAST_CORE_P0_ENABLE
    {3, 12}, // MX6_MMDC_CORE_IPG_CLK_P0_ENABLE
    {3, 14}, // MX6_OCRAM_CLK_ENABLE
    {3, 15}, // MX6_OPENVGAXICLK_CLK_ROOT_ENABLE
    {4, 0},  // MX6_PCIE_ROOT_ENABLE
    {4, 4},  // MX6_PL301_MX6QFAST1_S133CLK_ENABLE
    {4, 6},  // MX6_PL301_MX6QPER1_BCHCLK_ENABLE
    {4, 7},  // MX6_PL301_MX6QPER2_MAINCLK_ENABLE
    {4, 8},  // MX6_PWM1_CLK_ENABLE
    {4, 9},  // MX6_PWM2_CLK_ENABLE
    {4, 10}, // MX6_PWM3_CLK_ENABLE
    {4, 11}, // MX6_PWM4_CLK_ENABLE
    {4, 12}, // MX6_RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE
    {4, 13}, // MX6_RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE
    {4, 14}, // MX6_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE
    {4, 15}, // MX6_RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE
    {5, 0},  // MX6_ROM_CLK_ENABLE
    {5, 2},  // MX6_SATA_CLK_ENABLE
    {5, 3},  // MX6_SDMA_CLK_ENABLE
    {5, 6},  // MX6_SPBA_CLK_ENABLE
    {5, 7},  // MX6_SPDIF_CLK_ENABLE
    {5, 9},  // MX6_SSI1_CLK_ENABLE
    {5, 10}, // MX6_SSI2_CLK_ENABLE
    {5, 11}, // MX6_SSI3_CLK_ENABLE
    {5, 12}, // MX6_UART_CLK_ENABLE
    {5, 13}, // MX6_UART_SERIAL_CLK_ENABLE
    {6, 0},  // MX6_USBOH3_CLK_ENABLE
    {6, 1},  // MX6_USDHC1_CLK_ENABLE
    {6, 2},  // MX6_USDHC2_CLK_ENABLE
    {6, 3},  // MX6_USDHC3_CLK_ENABLE
    {6, 4},  // MX6_USDHC4_CLK_ENABLE
    {6, 5},  // MX6_EIM_SLOW_CLK_ENABLE
    {6, 6},  // MX6_VDOAXICLK_CLK_ENABLE
    {6, 7},  // MX6_VPU_CLK_ENABLE
};

MX6_CCGR_INDEX Mx6CcgrIndexFromClkGate (MX6_CLK_GATE ClockGate)
{
    static_assert(
        MX6_CLK_GATE_MAX == ARRAYSIZE(_mx6CcgrIndexMap),
        "Verifying size of _mx6CcgrIndexMap");

    return _mx6CcgrIndexMap[ClockGate];
}

PCWSTR StringFromMx6Clk (MX6_CLK Value)
{
    switch (Value) {
    case MX6_CLK_NONE: return L"(none)";
    case MX6_OSC_CLK: return L"OSC_CLK";
    case MX6_PLL1_MAIN_CLK: return L"PLL1_MAIN_CLK";
    case MX6_PLL2_MAIN_CLK: return L"PLL2_MAIN_CLK";
    case MX6_PLL2_PFD0: return L"PLL2_PFD0";
    case MX6_PLL2_PFD1: return L"PLL2_PFD1";
    case MX6_PLL2_PFD2: return L"PLL2_PFD2";
    case MX6_PLL3_MAIN_CLK: return L"PLL3_MAIN_CLK";
    case MX6_PLL3_PFD0: return L"PLL3_PFD0";
    case MX6_PLL3_PFD1: return L"PLL3_PFD1";
    case MX6_PLL3_PFD2: return L"PLL3_PFD2";
    case MX6_PLL3_PFD3: return L"PLL3_PFD3";
    case MX6_PLL4_MAIN_CLK: return L"PLL4_MAIN_CLK";
    case MX6_PLL5_MAIN_CLK: return L"PLL5_MAIN_CLK";
    case MX6_CLK1: return L"CLK1";
    case MX6_CLK2: return L"CLK2";
    case MX6_PLL1_SW_CLK: return L"PLL1_SW_CLK";
    case MX6_STEP_CLK: return L"STEP_CLK";
    case MX6_PLL3_SW_CLK: return L"PLL3_SW_CLK";
    case MX6_PLL3_80M: return L"PLL3_80M";
    case MX6_AXI_ALT: return L"AXI_ALT";
    case MX6_AXI_CLK_ROOT: return L"AXI_CLK_ROOT";
    case MX6_PERIPH_CLK2: return L"PERIPH_CLK2";
    case MX6_PERIPH_CLK: return L"PERIPH_CLK";
    case MX6_PRE_PERIPH_CLK: return L"PRE_PERIPH_CLK";
    case MX6_PRE_PERIPH2_CLK: return L"PRE_PERIPH2_CLK";
    case MX6_PERIPH2_CLK: return L"PERIPH2_CLK";
    case MX6_ARM_CLK_ROOT: return L"ARM_CLK_ROOT";
    case MX6_MMDC_CH0_CLK_ROOT: return L"MMDC_CH0_CLK_ROOT";
    case MX6_MMDC_CH1_CLK_ROOT: return L"MMDC_CH1_CLK_ROOT";
    case MX6_AHB_CLK_ROOT: return L"AHB_CLK_ROOT";
    case MX6_IPG_CLK_ROOT: return L"IPG_CLK_ROOT";
    case MX6_PERCLK_CLK_ROOT: return L"PERCLK_CLK_ROOT";
    case MX6_USDHC1_CLK_ROOT: return L"USDHC1_CLK_ROOT";
    case MX6_USDHC2_CLK_ROOT: return L"USDHC2_CLK_ROOT";
    case MX6_USDHC3_CLK_ROOT: return L"USDHC3_CLK_ROOT";
    case MX6_USDHC4_CLK_ROOT: return L"USDHC4_CLK_ROOT";
    case MX6_SSI1_CLK_ROOT: return L"SSI1_CLK_ROOT";
    case MX6_SSI2_CLK_ROOT: return L"SSI2_CLK_ROOT";
    case MX6_SSI3_CLK_ROOT: return L"SSI3_CLK_ROOT";
    case MX6_GPU2D_AXI_CLK_ROOT: return L"GPU2D_AXI_CLK_ROOT";
    case MX6_GPU3D_AXI_CLK_ROOT: return L"GPU3D_AXI_CLK_ROOT";
    case MX6_PCIE_AXI_CLK_ROOT: return L"PCIE_AXI_CLK_ROOT";
    case MX6_VDO_AXI_CLK_ROOT: return L"VDO_AXI_CLK_ROOT";
    case MX6_IPU1_HSP_CLK_ROOT: return L"IPU1_HSP_CLK_ROOT";
    case MX6_IPU2_HSP_CLK_ROOT: return L"IPU2_HSP_CLK_ROOT";
    case MX6_GPU2D_CORE_CLK_ROOT: return L"GPU2D_CORE_CLK_ROOT";
    case MX6_ACLK_EIM_SLOW_CLK_ROOT: return L"ACLK_EIM_SLOW_CLK_ROOT";
    case MX6_ACLK_CLK_ROOT: return L"ACLK_CLK_ROOT";
    case MX6_ENFC_CLK_ROOT: return L"ENFC_CLK_ROOT";
    case MX6_GPU3D_CORE_CLK_ROOT: return L"GPU3D_CORE_CLK_ROOT";
    case MX6_GPU3D_SHADER_CLK_ROOT: return L"GPU3D_SHADER_CLK_ROOT";
    case MX6_VPU_AXI_CLK_ROOT: return L"VPU_AXI_CLK_ROOT";
    case MX6_IPU1_DI0_CLK_ROOT: return L"IPU1_DI0_CLK_ROOT";
    case MX6_IPU1_DI1_CLK_ROOT: return L"IPU1_DI1_CLK_ROOT";
    case MX6_IPU2_DI0_CLK_ROOT: return L"IPU2_DI0_CLK_ROOT";
    case MX6_IPU2_DI1_CLK_ROOT: return L"IPU2_DI1_CLK_ROOT";
    case MX6_LDB_DI0_SERIAL_CLK_ROOT: return L"LDB_DI0_SERIAL_CLK_ROOT";
    case MX6_LDB_DI0_IPU: return L"LDB_DI0_IPU";
    case MX6_LDB_DI1_SERIAL_CLK_ROOT: return L"LDB_DI1_SERIAL_CLK_ROOT";
    case MX6_LDB_DI1_IPU: return L"LDB_DI1_IPU";
    case MX6_SPDIF0_CLK_ROOT: return L"SPDIF0_CLK_ROOT";
    case MX6_SPDIF1_CLK_ROOT: return L"SPDIF1_CLK_ROOT";
    case MX6_ESAI_CLK_ROOT: return L"ESAI_CLK_ROOT";
    case MX6_HSI_TX_CLK_ROOT: return L"HSI_TX_CLK_ROOT";
    case MX6_CAN_CLK_ROOT: return L"CAN_CLK_ROOT";
    case MX6_ECSPI_CLK_ROOT: return L"ECSPI_CLK_ROOT";
    case MX6_UART_CLK_ROOT: return L"UART_CLK_ROOT";
    case MX6_VIDEO_27M_CLK_ROOT: return L"VIDEO_27M_CLK_ROOT";
    case MX6_IPG_CLK_MAC0: return L"MX6_IPG_CLK_MAC0";
    default: return L"[invalid MX6_CLK value]";
    }
}

MX6_CLK Mx6ClkFromString (PCWSTR String)
{
    for (ULONG i = 0; i < MX6_CLK_MAX; ++i) {
        auto value = static_cast<MX6_CLK>(i);
        if (_wcsicmp(String, StringFromMx6Clk(value)) == 0) {
            return value;
        }
    }

    return MX6_CLK_MAX;
}

PCWSTR StringFromMx6ClkGate (MX6_CLK_GATE ClockGate)
{
    switch (ClockGate) {
    case MX6_AIPS_TZ1_CLK_ENABLE: return L"AIPS_TZ1_CLK_ENABLE";
    case MX6_AIPS_TZ2_CLK_ENABLE: return L"AIPS_TZ2_CLK_ENABLE";
    case MX6_APBHDMA_HCLK_ENABLE: return L"APBHDMA_HCLK_ENABLE";
    case MX6_ASRC_CLK_ENABLE: return L"ASRC_CLK_ENABLE";
    case MX6_CAAM_SECURE_MEM_CLK_ENABLE: return L"CAAM_SECURE_MEM_CLK_ENABLE";
    case MX6_CAAM_WRAPPER_ACLK_ENABLE: return L"CAAM_WRAPPER_ACLK_ENABLE";
    case MX6_CAAM_WRAPPER_IPG_ENABLE: return L"CAAM_WRAPPER_IPG_ENABLE";
    case MX6_CAN1_CLK_ENABLE: return L"CAN1_CLK_ENABLE";
    case MX6_CAN1_SERIAL_CLK_ENABLE: return L"CAN1_SERIAL_CLK_ENABLE";
    case MX6_CAN2_CLK_ENABLE: return L"CAN2_CLK_ENABLE";
    case MX6_CAN2_SERIAL_CLK_ENABLE: return L"CAN2_SERIAL_CLK_ENABLE";
    case MX6_ARM_DBG_CLK_ENABLE: return L"ARM_DBG_CLK_ENABLE";
    case MX6_DCIC1_CLK_ENABLE: return L"DCIC1_CLK_ENABLE";
    case MX6_DCIC2_CLK_ENABLE: return L"DCIC2_CLK_ENABLE";
    case MX6_DTCP_CLK_ENABLE: return L"DTCP_CLK_ENABLE";
    case MX6_ECSPI1_CLK_ENABLE: return L"ECSPI1_CLK_ENABLE";
    case MX6_ECSPI2_CLK_ENABLE: return L"ECSPI2_CLK_ENABLE";
    case MX6_ECSPI3_CLK_ENABLE: return L"ECSPI3_CLK_ENABLE";
    case MX6_ECSPI4_CLK_ENABLE: return L"ECSPI4_CLK_ENABLE";
    case MX6_ECSPI5_CLK_ENABLE: return L"ECSPI5_CLK_ENABLE";
    case MX6_ENET_CLK_ENABLE: return L"ENET_CLK_ENABLE";
    case MX6_EPIT1_CLK_ENABLE: return L"EPIT1_CLK_ENABLE";
    case MX6_EPIT2_CLK_ENABLE: return L"EPIT2_CLK_ENABLE";
    case MX6_ESAI_CLK_ENABLE: return L"ESAI_CLK_ENABLE";
    case MX6_GPT_CLK_ENABLE: return L"GPT_CLK_ENABLE";
    case MX6_GPT_SERIAL_CLK_ENABLE: return L"GPT_SERIAL_CLK_ENABLE";
    case MX6_GPU2D_CLK_ENABLE: return L"GPU2D_CLK_ENABLE";
    case MX6_GPU3D_CLK_ENABLE: return L"GPU3D_CLK_ENABLE";
    case MX6_HDMI_TX_ENABLE: return L"HDMI_TX_ENABLE";
    case MX6_HDMI_TX_ISFRCLK_ENABLE: return L"HDMI_TX_ISFRCLK_ENABLE";
    case MX6_I2C1_SERIAL_CLK_ENABLE: return L"I2C1_SERIAL_CLK_ENABLE";
    case MX6_I2C2_SERIAL_CLK_ENABLE: return L"I2C2_SERIAL_CLK_ENABLE";
    case MX6_I2C3_SERIAL_CLK_ENABLE: return L"I2C3_SERIAL_CLK_ENABLE";
    case MX6_IIM_CLK_ENABLE: return L"IIM_CLK_ENABLE";
    case MX6_IOMUX_IPT_CLK_IO_ENABLE: return L"IOMUX_IPT_CLK_IO_ENABLE";
    case MX6_IPMUX1_CLK_ENABLE: return L"IPMUX1_CLK_ENABLE";
    case MX6_IPMUX2_CLK_ENABLE: return L"IPMUX2_CLK_ENABLE";
    case MX6_IPMUX3_CLK_ENABLE: return L"IPMUX3_CLK_ENABLE";
    case MX6_IPSYNC_IP2APB_TZASC1_IPG_MASTER_CLK_ENABLE: return L"IPSYNC_IP2APB_TZASC1_IPG_MASTER_CLK_ENABLE";
    case MX6_IPSYNC_IP2APB_TZASC2_IPG_MASTER_CLK_ENABLE: return L"IPSYNC_IP2APB_TZASC2_IPG_MASTER_CLK_ENABLE";
    case MX6_IPSYNC_VDOA_IPG_MASTER_CLK_ENABLE: return L"IPSYNC_VDOA_IPG_MASTER_CLK_ENABLE";
    case MX6_IPU1_IPU_CLK_ENABLE: return L"IPU1_IPU_CLK_ENABLE";
    case MX6_IPU1_IPU_DI0_CLK_ENABLE: return L"IPU1_IPU_DI0_CLK_ENABLE";
    case MX6_IPU1_IPU_DI1_CLK_ENABLE: return L"IPU1_IPU_DI1_CLK_ENABLE";
    case MX6_IPU2_IPU_CLK_ENABLE: return L"IPU2_IPU_CLK_ENABLE";
    case MX6_IPU2_IPU_DI0_CLK_ENABLE: return L"IPU2_IPU_DI0_CLK_ENABLE";
    case MX6_IPU2_IPU_DI1_CLK_ENABLE: return L"IPU2_IPU_DI1_CLK_ENABLE";
    case MX6_LDB_DI0_CLK_ENABLE: return L"LDB_DI0_CLK_ENABLE";
    case MX6_LDB_DI1_CLK_ENABLE: return L"LDB_DI1_CLK_ENABLE";
    case MX6_MIPI_CORE_CFG_CLK_ENABLE: return L"MIPI_CORE_CFG_CLK_ENABLE";
    case MX6_MLB_CLK_ENABLE: return L"MLB_CLK_ENABLE";
    case MX6_MMDC_CORE_ACLK_FAST_CORE_P0_ENABLE: return L"MMDC_CORE_ACLK_FAST_CORE_P0_ENABLE";
    case MX6_MMDC_CORE_IPG_CLK_P0_ENABLE: return L"MMDC_CORE_IPG_CLK_P0_ENABLE";
    case MX6_OCRAM_CLK_ENABLE: return L"OCRAM_CLK_ENABLE";
    case MX6_OPENVGAXICLK_CLK_ROOT_ENABLE: return L"OPENVGAXICLK_CLK_ROOT_ENABLE";
    case MX6_PCIE_ROOT_ENABLE: return L"PCIE_ROOT_ENABLE";
    case MX6_PL301_MX6QFAST1_S133CLK_ENABLE: return L"PL301_MX6QFAST1_S133CLK_ENABLE";
    case MX6_PL301_MX6QPER1_BCHCLK_ENABLE: return L"PL301_MX6QPER1_BCHCLK_ENABLE";
    case MX6_PL301_MX6QPER2_MAINCLK_ENABLE: return L"PL301_MX6QPER2_MAINCLK_ENABLE";
    case MX6_PWM1_CLK_ENABLE: return L"PWM1_CLK_ENABLE";
    case MX6_PWM2_CLK_ENABLE: return L"PWM2_CLK_ENABLE";
    case MX6_PWM3_CLK_ENABLE: return L"PWM3_CLK_ENABLE";
    case MX6_PWM4_CLK_ENABLE: return L"PWM4_CLK_ENABLE";
    case MX6_RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE: return L"RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE";
    case MX6_RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE: return L"RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE";
    case MX6_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE: return L"RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE";
    case MX6_RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE: return L"RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE";
    case MX6_ROM_CLK_ENABLE: return L"ROM_CLK_ENABLE";
    case MX6_SATA_CLK_ENABLE: return L"SATA_CLK_ENABLE";
    case MX6_SDMA_CLK_ENABLE: return L"SDMA_CLK_ENABLE";
    case MX6_SPBA_CLK_ENABLE: return L"SPBA_CLK_ENABLE";
    case MX6_SPDIF_CLK_ENABLE: return L"SPDIF_CLK_ENABLE";
    case MX6_SSI1_CLK_ENABLE: return L"SSI1_CLK_ENABLE";
    case MX6_SSI2_CLK_ENABLE: return L"SSI2_CLK_ENABLE";
    case MX6_SSI3_CLK_ENABLE: return L"SSI3_CLK_ENABLE";
    case MX6_UART_CLK_ENABLE: return L"UART_CLK_ENABLE";
    case MX6_UART_SERIAL_CLK_ENABLE: return L"UART_SERIAL_CLK_ENABLE";
    case MX6_USBOH3_CLK_ENABLE: return L"USBOH3_CLK_ENABLE";
    case MX6_USDHC1_CLK_ENABLE: return L"USDHC1_CLK_ENABLE";
    case MX6_USDHC2_CLK_ENABLE: return L"USDHC2_CLK_ENABLE";
    case MX6_USDHC3_CLK_ENABLE: return L"USDHC3_CLK_ENABLE";
    case MX6_USDHC4_CLK_ENABLE: return L"USDHC4_CLK_ENABLE";
    case MX6_EIM_SLOW_CLK_ENABLE: return L"EIM_SLOW_CLK_ENABLE";
    case MX6_VDOAXICLK_CLK_ENABLE: return L"VDOAXICLK_CLK_ENABLE";
    case MX6_VPU_CLK_ENABLE: return L"VPU_CLK_ENABLE";
    default: return L"[Invalid MX6_CLK_GATE value]";
    }
}

MX6_CLK_GATE Mx6ClkGateFromString (PCWSTR String)
{
    for (ULONG i = 0; i < MX6_CLK_GATE_MAX; ++i) {
        auto value = static_cast<MX6_CLK_GATE>(i);
        if (_wcsicmp(String, StringFromMx6ClkGate(value)) == 0) {
            return value;
        }
    }

    return MX6_CLK_GATE_MAX;
}

MX6_CCM_CCGR Mx6ClkGateStateFromString (PCWSTR String)
{
    if (_wcsicmp(String, L"off") == 0) {
        return MX6_CCM_CCGR_OFF;
    } else if (_wcsicmp(String, L"on") == 0) {
        return MX6_CCM_CCGR_ON;
    } else if (_wcsicmp(String, L"run") == 0) {
        return MX6_CCM_CCGR_ON_RUN;
    } else {
        return static_cast<MX6_CCM_CCGR>(-1);
    }
}

PCWSTR StringFromMx6ClkGateState (MX6_CCM_CCGR State)
{
    switch (State) {
    case MX6_CCM_CCGR_OFF: return L"off";
    case MX6_CCM_CCGR_ON: return L"on";
    case MX6_CCM_CCGR_ON_RUN: return L"run";
    default: return L"[Invalid MX6_CCM_CCGR value]";
    }
}

std::wstring GetInterfacePath (const GUID& InterfaceGuid)
{
    ULONG length;
    CONFIGRET cr = CM_Get_Device_Interface_List_SizeW(
            &length,
            const_cast<GUID*>(&InterfaceGuid),
            nullptr,        // pDeviceID
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

    if (cr != CR_SUCCESS) {
        throw wexception::make(
            HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_NOT_FOUND)),
            L"Failed to get size of device interface list. (cr = 0x%x)",
            cr);
    }

    if (length < 2) {
        throw wexception::make(
            HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_NOT_FOUND)),
            L"The MX6PEP device was not found on this system. (cr = 0x%x)",
            cr);
    }

    std::unique_ptr<WCHAR[]> buf(new WCHAR[length]);
    cr = CM_Get_Device_Interface_ListW(
            const_cast<GUID*>(&InterfaceGuid),
            nullptr,        // pDeviceID
            buf.get(),
            length,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

    if (cr != CR_SUCCESS) {
        throw wexception::make(
            HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_NOT_FOUND)),
            L"Failed to get device interface list. (cr = 0x%x)",
            cr);
    }

    // Return the first string in the multistring
    return std::wstring(buf.get());
}

FileHandle OpenMx6PepHandle ()
{
    auto interfacePath = GetInterfacePath(GUID_DEVINTERFACE_MX6PEP);

    FileHandle fileHandle(CreateFile(
            interfacePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,          // dwShareMode
            nullptr,    // lpSecurityAttributes
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr));  // hTemplateFile


    if (!fileHandle.IsValid()) {
        if (GetLastError() == ERROR_ACCESS_DENIED) {
            // Try opening read-only
            fileHandle.Attach(CreateFile(
                interfacePath.c_str(),
                GENERIC_READ,
                0,          // dwShareMode
                nullptr,    // lpSecurityAttributes
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr));  // hTemplateFile

            if (fileHandle.IsValid()) {
                return fileHandle;
            }
        }

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"Failed to open a handle to the mx6pep device. "
            L"(GetLastError() = 0x%x, interfacePath = %s)",
            GetLastError(),
            interfacePath.c_str());
    }

    return fileHandle;
}

void PrintClockInfo (MX6_CLK Clock, const MX6_CLK_INFO& Info)
{
    // Frequency of 0 means invalid/not implemented
    if (Info.Frequency == 0) return;

    wprintf(
        L"%s\n"
        L"    Frequency: %d (%d Mhz)\n"
        L"    Parent: %s\n"
        L"\n",
        StringFromMx6Clk(Clock),
        Info.Frequency,
        Info.Frequency / 1000000,
        StringFromMx6Clk(Info.Parent));
}

void DumpClockTree (HANDLE PepHandle)
{
    // Query all clocks
    MX6PEP_DUMP_REGISTERS_OUTPUT output;

    DWORD information;
    if (!DeviceIoControl(
            PepHandle,
            IOCTL_MX6PEP_DUMP_REGISTERS,
            nullptr,
            0,
            &output,
            sizeof(output),
            &information,
            nullptr) || (information != sizeof(output))) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_DUMP_REGISTERS failed. "
            L"(GetLastError() = 0x%x, information = %d)",
            GetLastError(),
            information);
    }

    Mx6ClockTreeHelper clockTreeHelper(output);

    for (int i = MX6_CLK_NONE + 1; i < MX6_CLK_MAX; ++i) {
        const auto clock = static_cast<MX6_CLK>(i);
        MX6_CLK_INFO clockInfo;
        HRESULT hr = clockTreeHelper.GetClockInfo(clock, &clockInfo);
        if (FAILED(hr)) {
            continue;
        }

        PrintClockInfo(clock, clockInfo);
    }
}

void DumpGates (HANDLE PepHandle)
{
    MX6PEP_GET_CLOCK_GATE_REGISTERS_OUTPUT output = {};

    DWORD information;
    if (!DeviceIoControl(
            PepHandle,
            IOCTL_MX6PEP_GET_CLOCK_GATE_REGISTERS,
            nullptr,
            0,
            &output,
            sizeof(output),
            &information,
            nullptr) || (information != sizeof(output))) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_GET_CLOCK_GATE_REGISTERS failed. "
            L"(GetLastError() = 0x%x, information = %d)",
            GetLastError(),
            information);
    }

    for (int i = 0; i < MX6_CLK_GATE_MAX; ++i) {
        auto clockGate = static_cast<MX6_CLK_GATE>(i);
        auto index = Mx6CcgrIndexFromClkGate(clockGate);
        auto state = static_cast<MX6_CCM_CCGR>(
            (output.CcgrRegisters[index.RegisterIndex] >>
            (2 * index.GateNumber)) & 0x3);

        wprintf(
            L"%50s - %s\n",
            StringFromMx6ClkGate(clockGate),
            StringFromMx6ClkGateState(state));
    }
}

void SetClockGate (HANDLE PepHandle, MX6_CLK_GATE ClockGate, MX6_CCM_CCGR State)
{
    MX6PEP_SET_CLOCK_GATE_INPUT input = {};
    input.ClockGate = ClockGate;
    input.State = State;

    DWORD information;
    if (!DeviceIoControl(
            PepHandle,
            IOCTL_MX6PEP_SET_CLOCK_GATE,
            &input,
            sizeof(input),
            nullptr,
            0,
            &information,
            nullptr)) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_GET_CLOCK_GATE_REGISTERS failed. "
            L"(GetLastError() = 0x%x, information = %d)",
            GetLastError(),
            information);
    }

    wprintf(
        L"Successfully set clock gate %s to %s\n",
        StringFromMx6ClkGate(ClockGate),
        StringFromMx6ClkGateState(State));
}

int HandleSetGateCommand (int argc, _In_reads_(argc) wchar_t *argv[])
{
    if (argc < 4) {
        fwprintf(stderr, L"Not enough arguments for setgate command.\n");
        return 1;
    }

    auto clockGate = Mx6ClkGateFromString(argv[2]);
    if (clockGate == MX6_CLK_GATE_MAX) {
        fwprintf(stderr, L"Invalid clock gate identifier: %s.\n", argv[2]);
        return 1;
    }

    auto state = Mx6ClkGateStateFromString(argv[3]);
    if (state == -1) {
        fwprintf(
            stderr,
            L"Invalid state value: %s. Valid values are: on, off, run\n",
            argv[3]);

        return 1;
    }

    auto pepHandle = OpenMx6PepHandle();
    SetClockGate(pepHandle.Get(), clockGate, state);

    return 0;
}

//
// MMDC Profiling
//

enum MX6_AXI_ID {
    ALL_DEVICES,
    ARM_S0,
    ARM_S1,
    IPU1,
    IPU2,
    GPU3D_A,
    GPU2D_A,
    VDOA,
    OPENVG,
    HDMI,
    SDMA_BURST,
    SDMA_PERIPH,
    CAAM,
    USB,
    ENET,
    HSI,
    USDHC1,
    GPU3D_B,
    GPU2D_B,
    VPU,
    PCIE,
    DAP,
    APBH,
    BCH40,
    SATA,
    MLB150,
    USDHC2,
    USDHC3,
    USDHC4,

    _COUNT,
};

PCWSTR StringFromAxiId (MX6_AXI_ID AxiId)
{
    switch (AxiId) {
    case MX6_AXI_ID::ALL_DEVICES: return L"Whole system";
    case MX6_AXI_ID::ARM_S0: return L"ARM_S0";
    case MX6_AXI_ID::ARM_S1: return L"ARM_S1";
    case MX6_AXI_ID::IPU1: return L"IPU1";
    case MX6_AXI_ID::IPU2: return L"IPU2";
    case MX6_AXI_ID::GPU3D_A: return L"GPU3D_a";
    case MX6_AXI_ID::GPU2D_A: return L"GPU2D_a";
    case MX6_AXI_ID::VDOA: return L"VDOA";
    case MX6_AXI_ID::OPENVG: return L"OpenVG";
    case MX6_AXI_ID::HDMI: return L"HDMI";
    case MX6_AXI_ID::SDMA_BURST: return L"SDMA (Burst)";
    case MX6_AXI_ID::SDMA_PERIPH: return L"SDMA (Periph)";
    case MX6_AXI_ID::CAAM: return L"CAAM";
    case MX6_AXI_ID::USB: return L"USB";
    case MX6_AXI_ID::ENET: return L"ENET";
    case MX6_AXI_ID::HSI: return L"HSI";
    case MX6_AXI_ID::USDHC1: return L"uSDHC1";
    case MX6_AXI_ID::GPU3D_B: return L"GPU3D_b";
    case MX6_AXI_ID::GPU2D_B: return L"GPU2D_b";
    case MX6_AXI_ID::VPU: return L"VPU";
    case MX6_AXI_ID::PCIE: return L"PCIe";
    case MX6_AXI_ID::DAP: return L"DAP";
    case MX6_AXI_ID::APBH: return L"APBH";
    case MX6_AXI_ID::BCH40: return L"BCH40";
    case MX6_AXI_ID::SATA: return L"SATA";
    case MX6_AXI_ID::MLB150: return L"MLB150";
    case MX6_AXI_ID::USDHC2: return L"uSDHC2";
    case MX6_AXI_ID::USDHC3: return L"uSDHC3";
    case MX6_AXI_ID::USDHC4: return L"uSDHC4";
    default: return L"[Invalid AxiId]";
    }
}

struct MX6_AXI_FILTER {
    UINT16 AxiId;
    UINT16 AxiIdMask;
};

MX6_AXI_FILTER GetAxiFilter (MX6_AXI_ID AxiId)
{
    static const MX6_AXI_FILTER filters[] = {
        {0x0, 0x0},         // whole system
        {0x0, 0x3807},
        {0x1, 0x3807},
        {0x4, 0x3FE7},
        {0x5, 0x3FE7},
        {0x2, 0x3C3F},
        {0xA, 0x3C3F},
        {0x12, 0x3F3F},
        {0x22, 0x3C3F},
        {0x11A, 0x3FFF},
        {0x15A, 0x3FFF},
        {0x19A, 0x3FFF},
        {0x1A, 0x3FF},
        {0x5A, 0x33FF},
        {0x9A, 0x3FFF},
        {0xDA, 0x3FFF},
        {0x1DA, 0x3FFF},
        {0x3, 0x3C3F},
        {0xB, 0x3C3F},
        {0x13, 0x3C3F},
        {0x1B, 0x383F},
        {0x23, 0x3FFF},
        {0xA3, 0x3FFF},
        {0x63, 0x3FF},
        {0xE3, 0x3FFF},
        {0x123, 0x3FFF},
        {0x163, 0x3FFF},
        {0x1A3, 0x3FFF},
        {0x1E3, 0x3FFF},
    };

    return filters[static_cast<int>(AxiId)];
}

void ProfileMmdc (
    HANDLE PepHandle,
    bool ProfileAllDevices,
    unsigned int DurationMillis
    )
{
    static_assert(
        MX6_AXI_ID::ALL_DEVICES == 0,
        "Verifying ALL_DEVICES is 0");

    wprintf(L"         Block    Busy  ReadCount    BytesRead  WriteCount  BytesWritten\n");

    const int count =
        ProfileAllDevices ? MX6_AXI_ID::_COUNT : (MX6_AXI_ID::ALL_DEVICES + 1);

    for (int i = MX6_AXI_ID::ALL_DEVICES; i < count; ++i) {
        const auto axiId = static_cast<MX6_AXI_ID>(i);
        const auto axiFilter = GetAxiFilter(axiId);

        MX6PEP_PROFILE_MMDC_INPUT input = {};
        input.DurationMillis = DurationMillis;
        input.AxiId = axiFilter.AxiId;
        input.AxiIdMask = axiFilter.AxiIdMask;

        MX6PEP_PROFILE_MMDC_OUTPUT output;
        DWORD information;
        if (!DeviceIoControl(
                PepHandle,
                IOCTL_MX6PEP_PROFILE_MMDC,
                &input,
                sizeof(input),
                &output,
                sizeof(output),
                &information,
                nullptr) || (information != sizeof(output))) {

            throw wexception::make(
                HRESULT_FROM_WIN32(GetLastError()),
                L"IOCTL_MX6PEP_PROFILE_MMDC failed. "
                L"(GetLastError() = 0x%x, information = %d)",
                GetLastError(),
                information);
        }

        wprintf(
            L"%14s %6.1f%% %10d %10fMB %11d %11fMB\n",
            StringFromAxiId(axiId),
            100.0 * output.BusyCycleCount / output.TotalProfilingCount,
            output.ReadAccessCount,
            output.BytesRead / (1024.0 * 1024.0),
            output.WriteAccessCount,
            output.BytesWritten / (1024.0 * 1024.0));
    }
}

int HandleDdrProfCommand (int argc, _In_reads_(argc) wchar_t *argv[])
{
    // mxpowerutil ddrprof [-all] [ms]
    bool all = false;
    int optind;
    for (optind = 2; optind < argc; ++optind) {
        if (argv[optind][0] != L'-') {
            break;
        }

        if (_wcsicmp(argv[optind], L"-all") == 0) {
            all = true;
        } else {
            fwprintf(
                stderr,
                L"Invalid option: '%s'. Run mxpowerutil /? for usage.\n",
                argv[optind]);

            return 1;
        }
    }

    unsigned int duration;
    if (optind < argc) {
        duration = wcstoul(argv[optind], nullptr, 10);
        if ((duration == 0) || (duration > MX6_MMDC_PROFILE_DURATION_MAX)) {
            fwprintf(
                stderr,
                L"Invalid profiling interval: '%s'. "
                "Please specify an integer between 1 and %d.\n",
                argv[optind],
                MX6_MMDC_PROFILE_DURATION_MAX);

            return 1;
        }
    } else {
        // default profiling interval is 1s
        duration = 1000;
    }

    auto pepHandle = OpenMx6PepHandle();
    ProfileMmdc(pepHandle.Get(), all, duration);

    return 0;
}

void PinOutClock (HANDLE PepHandle, MX6_CLK Clock, ULONG Divider)
{
    enum : ULONG { CCOSR_DIVIDER_MAX = 8 };
    if ((Divider < 1) || (Divider > CCOSR_DIVIDER_MAX)) {
        throw wexception::make(
            E_INVALIDARG,
            L"Clock divider value is out of range. "
            L"(Divider = %lu, CCOSR_DIVIDER_MAX = %lu)",
            Divider,
            CCOSR_DIVIDER_MAX);
    }

    MX6_CCM_CCOSR_REG ccosrReg = { 0 };

    switch (Clock) {
    case MX6_OSC_CLK:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_OSC_CLK;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;
        
    case MX6_ARM_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_ARM_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_PLL2_MAIN_CLK:
        ccosrReg.CLKO1_SEL = MX6_CCM_CLKO1_SEL_PLL2_MAIN_CLK_2;
        ccosrReg.CLKO1_DIV = Divider - 1;
        ccosrReg.CLKO1_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO1;
        break;
        
    case MX6_PLL3_SW_CLK:
        ccosrReg.CLKO1_SEL = MX6_CCM_CLKO1_SEL_PLL3_SW_CLK_2;
        ccosrReg.CLKO1_DIV = Divider - 1;
        ccosrReg.CLKO1_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO1;
        break;

    case MX6_AXI_CLK_ROOT:
        ccosrReg.CLKO1_SEL = MX6_CCM_CLKO1_SEL_AXI_CLK_ROOT;
        ccosrReg.CLKO1_DIV = Divider - 1;
        ccosrReg.CLKO1_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO1;
        break;

    case MX6_IPG_CLK_ROOT:
        ccosrReg.CLKO1_SEL = MX6_CCM_CLKO1_SEL_IPG_CLK_ROOT;
        ccosrReg.CLKO1_DIV = Divider - 1;
        ccosrReg.CLKO1_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO1;
        break;

    case MX6_PERCLK_CLK_ROOT:
        ccosrReg.CLKO1_SEL = MX6_CCM_CLKO1_SEL_PERCLK_ROOT;
        ccosrReg.CLKO1_DIV = Divider - 1;
        ccosrReg.CLKO1_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO1;
        break;

    case MX6_GPU2D_AXI_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_GPU2D_AXI_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_GPU3D_AXI_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_GPU3D_AXI_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_GPU2D_CORE_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_GPU2D_CORE_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_GPU3D_CORE_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_GPU3D_CORE_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_GPU3D_SHADER_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_GPU3D_SHADER_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_VPU_AXI_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_VPU_AXI_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_UART_CLK_ROOT:
        ccosrReg.CLKO2_SEL = MX6_CCM_CLKO2_SEL_UART_CLK_ROOT;
        ccosrReg.CLKO2_DIV = Divider - 1;
        ccosrReg.CLKO2_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO2;
        break;

    case MX6_VIDEO_27M_CLK_ROOT:
        ccosrReg.CLKO1_SEL = MX6_CCM_CLKO1_SEL_VIDEO_27M_CLK_ROOT;
        ccosrReg.CLKO1_DIV = Divider - 1;
        ccosrReg.CLKO1_EN = 1;
        ccosrReg.CLK_OUT_SEL = MX6_CCM_CLK_OUT_SEL_CCM_CLKO1;
        break;

    default:
        throw wexception::make(
            E_NOTIMPL,
            L"Clock is not currently supported. (Clock = %s)",
            StringFromMx6Clk(Clock));
    }

    DWORD information;
    if (!DeviceIoControl(
            PepHandle,
            IOCTL_MX6PEP_WRITE_CCOSR,
            &ccosrReg,
            sizeof(ccosrReg),
            nullptr,
            0,
            &information,
            nullptr)) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_WRITE_CCOSR failed. "
            L"(GetLastError() = 0x%x)",
            GetLastError());
    }
}

int HandlePinoutCommand (int argc, _In_reads_(argc) wchar_t *argv[])
{
    // mxpowerutil pinout clock_name
    if (argc < 3) {
        fwprintf(
            stderr,
            L"Missing required parameter clock_name. "
            L"Run 'mxpowerutil.exe clocks' to see available clocks.\n");

        return 1;
    }

    MX6_CLK clk = Mx6ClkFromString(argv[2]);
    if (clk == MX6_CLK_MAX) {
        fwprintf(
            stderr,
            L"Invalid clock name: '%s'. "
            L"Run 'mxpowerutil.exe clocks' to see available clocks.\n",
            argv[2]);

        return 1;
    }

    auto pepHandle = OpenMx6PepHandle();
    const ULONG divider = 8;
    PinOutClock(pepHandle.Get(), clk, divider);

    wprintf(
        L"Successfully pinned out %s / %d on CLKO1.\n"
        L"To view the signal, ensure CLKO1 is muxed to an output pad.\n",
        StringFromMx6Clk(clk),
        divider);

    return 0;
}

//
// Pad Command
//

enum MX6_PAD {
    MX6_PAD_GPIO0,
    MX6_PAD_ENET_CRS_DV,
    MX6_PAD_KEY_COL3,
    MX6_PAD_KEY_ROW3,
    MX6_PAD_MAX,
    MX6_PAD_INVALID = MX6_PAD_MAX,
};

MX6_PAD_DESCRIPTOR Mx6PadDescFromPad (MX6_PAD Pad)
{
    static const MX6_PAD_DESCRIPTOR padDescriptorMap[] = {
        // CtlReg, MuxReg
        {0x5F0, 0x220},             // MX6_PAD_GPIO0
        {0x4F0, 0x1DC},             // MX6_PAD_ENET_CRS_DV
        {0x5E0, 0x210},             // MX6_PAD_KEY_COL3
        {0x5E4, 0x214},             // MX6_PAD_KEY_ROW3
    };

    return padDescriptorMap[Pad];
}

PCWSTR StringFromMx6Pad (MX6_PAD Pad)
{
    switch (Pad) {
    case MX6_PAD_GPIO0: return L"GPIO0";
    case MX6_PAD_ENET_CRS_DV: return L"ENET_CRS_DV";
    case MX6_PAD_KEY_COL3: return L"KEY_COL3";
    case MX6_PAD_KEY_ROW3: return L"KEY_ROW3";
    default:
        return L"[Invalid Pad]\n";
    }
}

MX6_PAD Mx6PadFromString (PCWSTR Str)
{
    for (int i = 0; i < MX6_PAD_MAX; ++i) {
        auto pad = static_cast<MX6_PAD>(i);
        if (_wcsicmp(Str, StringFromMx6Pad(pad)) == 0) {
            return pad;
        }
    }

    return MX6_PAD_INVALID;
}

//
// Pad control settings
//

enum MX6_HYS : ULONG {
    MX6_HYS_DISABLED,
    MX6_HYS_ENABLED,
};

enum MX6_PUS : ULONG {
    MX6_PUS_100K_OHM_PD,
    MX6_PUS_47K_OHM_PU,
    MX6_PUS_100K_OHM_PU,
    MX6_PUS_22K_OHM_PU,
    MX6_PUS_MAX,
    MX6_PUS_INVALID = MX6_PUS_MAX,
};

PCWSTR StringFromMx6Pus (MX6_PUS Value)
{
    switch (Value) {
    case MX6_PUS_100K_OHM_PD: return L"100pd";
    case MX6_PUS_47K_OHM_PU: return L"47pu";
    case MX6_PUS_100K_OHM_PU: return L"100pu";
    case MX6_PUS_22K_OHM_PU: return L"22pu";
    default: return L"[Invalid]";
    }
}

MX6_PUS Mx6PusFromString (PCWSTR Str)
{
    for (int i = 0; i < MX6_PUS_MAX; ++i) {
        auto pus = static_cast<MX6_PUS>(i);
        if (_wcsicmp(Str, StringFromMx6Pus(pus)) == 0) {
            return pus;
        }
    }

    return MX6_PUS_INVALID;
}

enum MX6_PUE : ULONG {
    MX6_PUE_KEEP,
    MX6_PUE_PULL,
    MX6_PUE_MAX,
    MX6_PUE_INVALID = MX6_PUE_MAX,
};

PCWSTR StringFromMx6Pue (MX6_PUE Value)
{
    switch (Value) {
    case MX6_PUE_KEEP: return L"keep";
    case MX6_PUE_PULL: return L"pull";
    default: return L"[invalid]";
    }
}

MX6_PUE Mx6PueFromString (PCWSTR Str)
{
   for (int i = 0; i < MX6_PUE_MAX; ++i) {
        auto pue = static_cast<MX6_PUE>(i);
        if (_wcsicmp(Str, StringFromMx6Pue(pue)) == 0) {
            return pue;
        }
    }

    return MX6_PUE_INVALID;
}

enum MX6_PKE : ULONG {
    MX6_PKE_DISABLE,
    MX6_PKE_ENABLE,
};

enum MX6_ODE : ULONG {
    MX6_ODE_DISABLE,
    MX6_ODE_ENABLE,
};

enum MX6_SPEED : ULONG {
    MX6_SPEED_LOW,
    MX6_SPEED_MEDIUM = 2,
    MX6_SPEED_MAXIMUM,
    MX6_SPEED_INVALID,
};

PCWSTR StringFromMx6Speed (MX6_SPEED Value)
{
    switch (Value) {
    case MX6_SPEED_LOW: return L"low";
    case MX6_SPEED_MEDIUM: return L"medium";
    case MX6_SPEED_MAXIMUM: return L"maximum";
    default: return L"invalid";
    }
}

MX6_SPEED Mx6SpeedFromString (PCWSTR Str)
{
    if (_wcsicmp(Str, L"low") == 0) {
        return MX6_SPEED_LOW;
    } else if (_wcsicmp(Str, L"med") == 0) {
        return MX6_SPEED_MEDIUM;
    } else if (_wcsicmp(Str, L"max") == 0) {
        return MX6_SPEED_MAXIMUM;
    } else {
        return MX6_SPEED_INVALID;
    }
}

enum MX6_DSE : ULONG {
    MX6_DSE_HIZ,
    MX6_DSE_260_OHM,
    MX6_DSE_130_OHM,
    MX6_DSE_90_OHM,
    MX6_DSE_60_OHM,
    MX6_DSE_50_OHM,
    MX6_DSE_40_OHM,
    MX6_DSE_33_OHM,

    MX6_DSE_INVALID,
};

PCWSTR StringFromMx6Dse (MX6_DSE Value)
{
    switch (Value) {
    case MX6_DSE_HIZ: return L"Hi-Z";
    case MX6_DSE_260_OHM: return L"260 Ohms";
    case MX6_DSE_130_OHM: return L"130 Ohms";
    case MX6_DSE_90_OHM: return L"90 Ohms";
    case MX6_DSE_60_OHM: return L"60 Ohms";
    case MX6_DSE_50_OHM: return L"50 Ohms";
    case MX6_DSE_40_OHM: return L"40 Ohms";
    case MX6_DSE_33_OHM: return L"33 Ohms";
    default: return L"[invalid]";
    }
}

MX6_DSE Mx6DseFromString (PCWSTR Str)
{
    if (_wcsicmp(Str, L"hiz") == 0) {
        return MX6_DSE_HIZ;
    } else if (_wcsicmp(Str, L"260") == 0) {
        return MX6_DSE_260_OHM;
    } else if (_wcsicmp(Str, L"130") == 0) {
        return MX6_DSE_130_OHM;
    } else if (_wcsicmp(Str, L"90") == 0) {
        return MX6_DSE_90_OHM;
    } else if (_wcsicmp(Str, L"60") == 0) {
        return MX6_DSE_60_OHM;
    } else if (_wcsicmp(Str, L"50") == 0) {
        return MX6_DSE_50_OHM;
    } else if (_wcsicmp(Str, L"40") == 0) {
        return MX6_DSE_40_OHM;
    } else if (_wcsicmp(Str, L"33") == 0) {
        return MX6_DSE_33_OHM;
    } else {
        return MX6_DSE_INVALID;
    }
}

enum MX6_SRE {
    MX6_SRE_SLOW,
    MX6_SRE_FAST,

    MX6_SRE_INVALID,
};

PCWSTR StringFromMx6Sre (MX6_SRE Value)
{
    switch (Value) {
    case MX6_SRE_SLOW: return L"slow";
    case MX6_SRE_FAST: return L"fast";
    default: return L"[invalid]";
    }
}

MX6_SRE Mx6SreFromString (PCWSTR Str)
{
    if (_wcsicmp(Str, L"slow") == 0) {
        return MX6_SRE_SLOW;
    } else if (_wcsicmp(Str, L"fast") == 0) {
        return MX6_SRE_FAST;
    } else {
        return MX6_SRE_INVALID;
    }
}

/**
  Define a configuration for a pad, including drive settings and MUX setting.

  Sre - MX6_SRE - Slew Rate setting
  Dse - MX6_DSE - Drive strength
  Speed - MX6_SPEED - Pad speed setting
  Ode - MX6_ODE - Open drain enable
  Pke - MX6_PKE - Pull/Keeper enable
  Pue - MX6_PUE - Pull/Keep mode select
  Pus - MX6_PUS - Pull strength
  Hys - MX6_HYS - Hysteresis enable/disable
  MuxAlt- Alternate function number

**/
#define _MX6_PADCFG(Sre, Dse, Speed, Ode, Pke, Pue, Pus, Hys) \
          (((Sre) & 0x1) | \
          (((Dse) & 0x7) << 3) | \
          (((Speed) & 0x3) << 6) | \
          (((Ode) & 0x1) << 11) | \
          (((Pke) & 0x1) << 12) | \
          (((Pue) & 0x1) << 13) | \
          (((Pus) & 0x3) << 14) | \
          (((Hys) & 0x1) << 16))

void DumpPadInfo (HANDLE PepHandle, MX6_PAD Pad, MX6_PAD_DESCRIPTOR PadDesc)
{
    MX6PEP_GET_PAD_CONFIG_INPUT input = {};
    input.PadDescriptor = PadDesc;

    MX6PEP_GET_PAD_CONFIG_OUTPUT output = {};

    DWORD information;
    if (!DeviceIoControl(
            PepHandle,
            IOCTL_MX6PEP_GET_PAD_CONFIG,
            &input,
            sizeof(input),
            &output,
            sizeof(output),
            &information,
            nullptr) || (information != sizeof(output))) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_GET_PAD_CONFIG failed. "
            L"(GetLastError() = 0x%x, information = %d)",
            GetLastError(),
            information);
    }

    const ULONG muxReg = output.PadMuxRegister;
    const ULONG ctrlReg = output.PadControlRegister;

    if (Pad != MX6_PAD_INVALID) {
        wprintf(L"Settings for pad %s:\n", StringFromMx6Pad(Pad));
    } else {
        wprintf(L"Settings for pad:\n");
    }

    wprintf(
        L"              MuxRegOffset: 0x%x\n"
        L"             CtrlRegOffset: 0x%x\n"
        L"                   Pad Mux: %d\n"
        L"         Hysteresis (-hys): %s\n"
        L"      Pull Strength (-pus): %s\n"
        L"   Pull/Keep Select (-pue): %s\n"
        L"   Pull/Keep Enable (-pke): %s\n"
        L"  Open Drain Enable (-ode): %s\n"
        L"              Speed (-spd): %s\n"
        L"     Drive Strength (-dse): %s\n"
        L"          Slew Rate (-sre): %s\n",
        PadDesc.MuxRegOffset,
        PadDesc.CtrlRegOffset,
        muxReg & 0x7,
        ((ctrlReg >> 16) & 0x1) ? L"enabled" : L"disabled",
        StringFromMx6Pus(MX6_PUS((ctrlReg >> 14) & 0x3)),
        StringFromMx6Pue(MX6_PUE((ctrlReg >> 13) & 0x1)),
        ((ctrlReg >> 12) & 0x1) ? L"enabled" : L"disabled",
        ((ctrlReg >> 11) & 0x1) ? L"enabled" : L"disabled",
        StringFromMx6Speed(MX6_SPEED((ctrlReg >> 6) & 0x3)),
        StringFromMx6Dse(MX6_DSE((ctrlReg >> 3) & 0x7)),
        StringFromMx6Sre(MX6_SRE(ctrlReg & 0x1)));
}

int ParsePadId (PCWSTR Str, MX6_PAD* PadPtr, MX6_PAD_DESCRIPTOR* PadDescPtr)
{
    // parse pad
    MX6_PAD pad = Mx6PadFromString(Str);
    MX6_PAD_DESCRIPTOR padDesc;
    if (pad == MX6_PAD_INVALID) {
        // try to interpret as a hex PAD_INFO structure
        ULONG padHex = wcstoul(Str, nullptr, 0);
        if (padHex == 0) {
            fwprintf(stderr, L"Invalid pad: '%s'\n", Str);
            return 1;
        }

        padDesc.CtrlRegOffset =  HIWORD(padHex);
        padDesc.MuxRegOffset = LOWORD(padHex);
    } else {
        padDesc = Mx6PadDescFromPad(pad);
    }

    *PadPtr = pad;
    *PadDescPtr = padDesc;
    return 0;
}

int HandlePadCommand (int argc, _In_reads_(argc) wchar_t *argv[])
{
    // mxpowerutil pad pad_id
    if (argc < 3) {
        fwprintf(stderr, L"Missing required parameter 'pad_id'. Type 'mxpowerutil /?' for usage.\n");
        return 1;
    }

    // parse pad
    MX6_PAD pad;
    MX6_PAD_DESCRIPTOR padDesc;
    int ret = ParsePadId(argv[2], &pad, &padDesc);
    if (ret != 0) {
        return ret;
    }

    // if no more arguments, dump pad configuration
    auto pepHandle = OpenMx6PepHandle();
    DumpPadInfo(pepHandle.Get(), pad, padDesc);
    return 0;
}

int HandlePadMuxCommand (int argc, _In_reads_(argc) wchar_t *argv[])
{
    // mxpowerutil padmux pad_id pad_mux
    if (argc < 3) {
        fwprintf(
            stderr,
            L"Missing required parameter pad_id. Type 'mxpowerutil /?' for usage.\n");

        return 1;
    }

    if (argc < 4) {
        fwprintf(
            stderr,
            L"Missing required parameter pad_mux. Type 'mxpowerutil /?' for usage.\n");

        return 1;
    }

    // parse pad_id
    MX6_PAD pad;
    MX6_PAD_DESCRIPTOR padDesc;
    int ret = ParsePadId(argv[2], &pad, &padDesc);
    if (ret != 0) {
        return ret;
    }

    // expect an unsigned integer between 0 and 7
    ULONG padMux = wcstoul(argv[3], nullptr, 0);
    if (padMux > 7) {
        fwprintf(
            stderr,
            L"pad_mux must be in the range [0,7]\n");

        return 1;
    }

    MX6PEP_SET_PAD_CONFIG_INPUT input = {};
    input.PadDescriptor = padDesc;
    input.PadMuxRegister = padMux;
    input.PadControlRegister = ULONG(MX6_PAD_REGISTER_INVALID);

    auto pepHandle = OpenMx6PepHandle();
    DWORD information;
    if (!DeviceIoControl(
            pepHandle.Get(),
            IOCTL_MX6PEP_SET_PAD_CONFIG,
            &input,
            sizeof(input),
            nullptr,
            0,
            &information,
            nullptr)) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_SET_PAD_CONFIG failed. "
            L"(GetLastError() = 0x%x)",
            GetLastError());
    }

    wprintf(
        L"Successfully set pad %s (0x%04x%04x) mux to %d\n",
        StringFromMx6Pad(pad),
        padDesc.CtrlRegOffset,
        padDesc.MuxRegOffset,
        padMux);

    DumpPadInfo(pepHandle.Get(), pad, padDesc);

    return 0;
}

int HandlePadCtlCommand (int argc, _In_reads_(argc) wchar_t *argv[])
{
    // mxpowerutil padctl pad_id [options]
    if (argc < 3) {
        fwprintf(
            stderr,
            L"Missing required parameter pad_id. Type 'mxpowerutil /?' for usage.\n");

        return 1;
    }

    // parse pad_id
    MX6_PAD pad;
    MX6_PAD_DESCRIPTOR padDesc;
    int ret = ParsePadId(argv[2], &pad, &padDesc);
    if (ret != 0) {
        return ret;
    }

    // parse other options
    MX6_HYS hys = MX6_HYS_DISABLED;
    MX6_PUS pus = MX6_PUS_100K_OHM_PD;
    MX6_PUE pue = MX6_PUE_PULL;
    MX6_PKE pke = MX6_PKE_DISABLE;
    MX6_ODE ode = MX6_ODE_DISABLE;
    MX6_SPEED spd = MX6_SPEED_MEDIUM;
    MX6_DSE dse = MX6_DSE_130_OHM;
    MX6_SRE sre = MX6_SRE_SLOW;
    for (int optind = 3; optind < argc; ++optind) {
        PCWSTR const arg = argv[optind];
        if (_wcsicmp(arg, L"-hys") == 0) {
            hys = MX6_HYS_ENABLED;
        } else if (_wcsicmp(arg, L"-pus") == 0) {
            // -pus 100pd|47pu|100pu|22pu
            ++optind;
            if (optind == argc) {
                fwprintf(
                    stderr,
                    L"Missing required parameter for -pus option\n");

                return 1;
            }

            pus = Mx6PusFromString(argv[optind]);
            if (pus == MX6_PUS_INVALID) {
                fwprintf(
                    stderr,
                    L"Invalid pull strength: '%s'.\n",
                    argv[optind]);

                return 1;
            }
        } else if (_wcsicmp(arg, L"-pue") == 0) {
            // -pue keep|pull
            ++optind;
            if (optind == argc) {
                fwprintf(
                    stderr,
                    L"Missing required parameter for -pue option\n");

                return 1;
            }

            pue = Mx6PueFromString(argv[optind]);
            if (pue == MX6_PUE_INVALID) {
                fwprintf(
                    stderr,
                    L"Invalid pull/keep setting: '%s'.\n",
                    argv[optind]);

                return 1;
            }
        } else if (_wcsicmp(arg, L"-pke") == 0) {
            pke = MX6_PKE_ENABLE;
        } else if (_wcsicmp(arg, L"-ode") == 0) {
            ode = MX6_ODE_ENABLE;
        } else if (_wcsicmp(arg, L"-spd") == 0) {
            // -spd low|med|max
            ++optind;
            if (optind == argc) {
                fwprintf(
                    stderr,
                    L"Missing required parameter for -spd option\n");

                return 1;
            }

            spd = Mx6SpeedFromString(argv[optind]);
            if (spd == MX6_SPEED_INVALID) {
                fwprintf(
                    stderr,
                    L"Invalid speed: '%s'.\n",
                    argv[optind]);

                return 1;
            }
        } else if (_wcsicmp(arg, L"-dse") == 0) {
            // -dse hiz|260|130|90|60|50|40|33
            ++optind;
            if (optind == argc) {
                fwprintf(
                    stderr,
                    L"Missing required parameter for -dse option\n");

                return 1;
            }

            dse = Mx6DseFromString(argv[optind]);
            if (dse == MX6_DSE_INVALID) {
                fwprintf(
                    stderr,
                    L"Invalid drive strength: '%s'.\n",
                    argv[optind]);

                return 1;
            }
        } else if (_wcsicmp(arg, L"-sre") == 0) {
            // -sre slow|fast
            ++optind;
            if (optind == argc) {
                fwprintf(
                    stderr,
                    L"Missing required parameter for -sre option\n");

                return 1;
            }

            sre = Mx6SreFromString(argv[optind]);
            if (sre == MX6_SRE_INVALID) {
                fwprintf(
                    stderr,
                    L"Invalid slew rate: '%s'\n",
                    argv[optind]);

                return 1;
            }
        } else if (arg[0] == L'-') {
            fwprintf(stderr, L"Unrecognized option: %s.\n", arg);
            return 1;
        } else {
            fwprintf(stderr, L"Unexpected positional parameter: %s.\n", arg);
            return 1;
        }
    }

    MX6PEP_SET_PAD_CONFIG_INPUT input = {};
    input.PadDescriptor = padDesc;
    input.PadMuxRegister = ULONG(MX6_PAD_REGISTER_INVALID);
    input.PadControlRegister = _MX6_PADCFG(sre, dse, spd, ode, pke, pue, pus, hys);

    auto pepHandle = OpenMx6PepHandle();
    DWORD information;
    if (!DeviceIoControl(
            pepHandle.Get(),
            IOCTL_MX6PEP_SET_PAD_CONFIG,
            &input,
            sizeof(input),
            nullptr,
            0,
            &information,
            nullptr)) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_SET_PAD_CONFIG failed. "
            L"(GetLastError() = 0x%x)",
            GetLastError());
    }

    wprintf(
        L"Successfully set pad %s (0x%04x%04x) control to 0x%x\n",
        StringFromMx6Pad(pad),
        padDesc.CtrlRegOffset,
        padDesc.MuxRegOffset,
        input.PadControlRegister);

    DumpPadInfo(pepHandle.Get(), pad, padDesc);

    return 0;
}

int HandlePadsCommand (int argc, _In_reads_(argc) wchar_t * /*argv*/ [])
{
    // mxpowerutil pads
    if (argc > 2) {
        fwprintf(stderr, L"Too many arguments to 'pads' command\n");
        return 1;
    }

    for (int i = 0; i < MX6_PAD_MAX; ++i) {
        auto pad = static_cast<MX6_PAD>(i);
        wprintf(L"%s\n", StringFromMx6Pad(pad));
    }

    return 0;
}

int HandleDumpCommand (int argc, _In_reads_(argc) wchar_t * /*argv*/ [])
{
    // mxpowerutil dump
    if (argc > 2) {
        fwprintf(stderr, L"Too many arguments to 'dump' command\n");
        return 1;
    }

    auto pepHandle = OpenMx6PepHandle();

    MX6PEP_DUMP_REGISTERS_OUTPUT output;
    DWORD information;
    if (!DeviceIoControl(
            pepHandle.Get(),
            IOCTL_MX6PEP_DUMP_REGISTERS,
            nullptr,
            0,
            &output,
            sizeof(output),
            &information,
            nullptr) || (information != sizeof(output))) {

        throw wexception::make(
            HRESULT_FROM_WIN32(GetLastError()),
            L"IOCTL_MX6PEP_DUMP_REGISTERS failed. "
            L"(GetLastError() = 0x%x, information = %d)",
            GetLastError(),
            information);
    }

    struct REG_DESCRIPTOR {
        PCWSTR Name;
        ULONG Value;
    };

    const REG_DESCRIPTOR ccmRegisters[] = {
        { L"CCR",       output.Ccm.CCR },
        { L"CCDR",      output.Ccm.CCDR },
        { L"CSR",       output.Ccm.CSR },
        { L"CCSR",      output.Ccm.CCSR },
        { L"CACRR",     output.Ccm.CACRR },
        { L"CBCDR",     output.Ccm.CBCDR },
        { L"CBCMR",     output.Ccm.CBCMR },
        { L"CSCMR1",    output.Ccm.CSCMR1 },
        { L"CSCMR2",    output.Ccm.CSCMR2 },
        { L"CSCDR1",    output.Ccm.CSCDR1 },
        { L"CS1CDR",    output.Ccm.CS1CDR },
        { L"CS2CDR",    output.Ccm.CS2CDR },
        { L"CDCDR",     output.Ccm.CDCDR },
        { L"CHSCCDR",   output.Ccm.CHSCCDR },
        { L"CSCDR2",    output.Ccm.CSCDR2 },
        { L"CSCDR3",    output.Ccm.CSCDR3 },
        { L"CDHIPR",    output.Ccm.CDHIPR },
        { L"CLPCR",     output.Ccm.CLPCR },
        { L"CISR",      output.Ccm.CISR },
        { L"CIMR",      output.Ccm.CIMR },
        { L"CCOSR",     output.Ccm.CCOSR },
        { L"CGPR",      output.Ccm.CGPR },
        { L"CCGR[0]",   output.Ccm.CCGR[0] },
        { L"CCGR[1]",   output.Ccm.CCGR[1] },
        { L"CCGR[2]",   output.Ccm.CCGR[2] },
        { L"CCGR[3]",   output.Ccm.CCGR[3] },
        { L"CCGR[4]",   output.Ccm.CCGR[4] },
        { L"CCGR[5]",   output.Ccm.CCGR[5] },
        { L"CCGR[6]",   output.Ccm.CCGR[6] },
        { L"CMEOR",     output.Ccm.CMEOR }
    };
    
    const REG_DESCRIPTOR analogRegisters[] = {
        { L"PLL_ARM", output.Analog.PLL_ARM },
        { L"PLL_USB1", output.Analog.PLL_USB1 },
        { L"PLL_USB2", output.Analog.PLL_USB2 },
        { L"PLL_SYS", output.Analog.PLL_SYS },
        { L"PLL_SYS_SS", output.Analog.PLL_SYS_SS },
        { L"PLL_SYS_NUM", output.Analog.PLL_SYS_NUM },
        { L"PLL_SYS_DENOM", output.Analog.PLL_SYS_DENOM },
        { L"PLL_AUDIO", output.Analog.PLL_AUDIO },
        { L"PLL_AUDIO_NUM", output.Analog.PLL_AUDIO_NUM },
        { L"PLL_AUDIO_DENOM", output.Analog.PLL_AUDIO_DENOM },
        { L"PLL_VIDEO", output.Analog.PLL_VIDEO },
        { L"PLL_VIDEO_NUM", output.Analog.PLL_VIDEO_NUM },
        { L"PLL_VIDEO_DENOM", output.Analog.PLL_VIDEO_DENOM },
        { L"PLL_MLB", output.Analog.PLL_MLB },
        { L"PLL_ENET", output.Analog.PLL_ENET },
        { L"PFD_480", output.Analog.PFD_480 },
        { L"PFD_528", output.Analog.PFD_528 },
        { L"PMU_REG_1P1", output.Analog.PMU_REG_1P1 },
        { L"PMU_REG_3P0", output.Analog.PMU_REG_3P0 },
        { L"PMU_REG_2P5", output.Analog.PMU_REG_2P5 },
        { L"PMU_REG_CORE", output.Analog.PMU_REG_CORE },
        { L"MISC0", output.Analog.MISC0 },
        { L"MISC1", output.Analog.MISC1 },
        { L"MISC2", output.Analog.MISC2 },
    };
    
    const REG_DESCRIPTOR gpcRegisters[] = {
        { L"CNTR", output.Gpc.CNTR },
        { L"PGR", output.Gpc.PGR },
        { L"IMR1", output.Gpc.IMR[0] },
        { L"IMR2", output.Gpc.IMR[1] },
        { L"IMR3", output.Gpc.IMR[2] },
        { L"IMR4", output.Gpc.IMR[3] },
        { L"ISR1", output.Gpc.ISR[0] },
        { L"ISR2", output.Gpc.ISR[1] },
        { L"ISR3", output.Gpc.ISR[2] },
        { L"ISR4", output.Gpc.ISR[3] },
        { L"PGC_GPU.CTRL", output.Gpc.PGC_GPU.CTRL },
        { L"PGC_GPU.PUPSCR", output.Gpc.PGC_GPU.PUPSCR },
        { L"PGC_GPU.PDNSCR", output.Gpc.PGC_GPU.PDNSCR },
        { L"PGC_GPU.SR", output.Gpc.PGC_GPU.SR },
        { L"PGC_CPU.CTRL", output.Gpc.PGC_CPU.CTRL },
        { L"PGC_CPU.PUPSCR", output.Gpc.PGC_CPU.PUPSCR },
        { L"PGC_CPU.PDNSCR", output.Gpc.PGC_CPU.PDNSCR },
        { L"PGC_CPU.SR", output.Gpc.PGC_CPU.SR },
    };

    wprintf(L"CCM Register Dump\n");
    for (const auto& entry : ccmRegisters) {
        wprintf(L"    %20s = 0x%08x\n", entry.Name, entry.Value);
    }

    wprintf(L"\nANALOG Register Dump\n");
    for (const auto& entry : analogRegisters) {
        wprintf(L"    %20s = 0x%08x\n", entry.Name, entry.Value);
    }

    wprintf(L"\nGPC Register Dump\n");
    for (const auto& entry : gpcRegisters) {
        wprintf(L"    %20s = 0x%08x\n", entry.Name, entry.Value);
    }

    const MX6_CCM_CLPCR_REG clpcr = { output.Ccm.CLPCR };

    wprintf(
        L"\nCLPCR = 0x%x\n"
        L"    CLPCR[LPM] = %d\n"
        L"    CLPCR[ARM_clk_dis_on_lpm] = %d\n"
        L"    CLPCR[SBYOS] = %d\n"
        L"    CLPCR[dis_ref_osc] = %d\n"
        L"    CLPCR[VSTBY] = %d\n"
        L"    CLPCR[stby_count] = %d\n"
        L"    CLPCR[cosc_pwrdown] = %d\n"
        L"    CLPCR[wb_per_at_lpm] = %d\n"
        L"    CLPCR[bypass_mmdc_ch0_lpm_hs] = %d\n"
        L"    CLPCR[bypass_mmdc_ch1_lpm_hs] = %d\n"
        L"    CLPCR[mask_core0_wfi] = %d\n"
        L"    CLPCR[mask_core1_wfi] = %d\n"
        L"    CLPCR[mask_core2_wfi] = %d\n"
        L"    CLPCR[mask_core3_wfi] = %d\n"
        L"    CLPCR[mask_scu_idle] = %d\n"
        L"    CLPCR[mask_l2cc_idle] = %d\n",
        clpcr.AsUlong,
        clpcr.LPM,
        clpcr.ARM_clk_dis_on_lpm,
        clpcr.SBYOS,
        clpcr.dis_ref_osc,
        clpcr.VSTBY,
        clpcr.stby_count,
        clpcr.cosc_pwrdown,
        clpcr.wb_per_at_lpm,
        clpcr.bypass_mmdc_ch0_lpm_hs,
        clpcr.bypass_mmdc_ch1_lpm_hs,
        clpcr.mask_core0_wfi,
        clpcr.mask_core1_wfi,
        clpcr.mask_core2_wfi,
        clpcr.mask_core3_wfi,
        clpcr.mask_scu_idle,
        clpcr.mask_l2cc_idle);

    return 0;
}

void PrintUsage ()
{
    PCWSTR Usage =
L"mxpowerutil: IMX6 Clock and Power Utility\n"
L"Usage: mxpowerutil [clocks|gates|setgate|ddrprof|pinout|pad|padmux|padctl|pads]\n"
L"\n"
L" clocks                      Dump clock tree\n"
L" gates                       Dump state of all clock gates\n"
L" setgate gate off|on|run     Configure a clock gate to the specified state\n"
L" ddrprof [-all] [ms]         Run MMDC profiling. Measures DDR usage.\n"
L"   -all                      Profile each AXI device individually\n"
L"   ms                        Profiling duration in milliseconds (default 1000)\n"
L" pinout clock_name           Pin out clock_name / 8 on CCM_CLKO1\n"
L" pad pad_id [options]        Dump pad settings\n"
L"  where pad_id is pad_name|pad_hex:\n"
L"   pad_name                  The symbolic name of the pad (e.g. GPIO0)\n"
L"   pad_hex                   The hex identifier of the pad. The high word\n"
L"                             is the offset to the control register, and the\n"
L"                             low word is the offset to the mux register.\n"
L" padmux pad_id pad_mux       Set mux configuration for a pad to the\n"
L"                             specified value. pad_mux must be in the\n"
L"                             range [0,7]\n"
L" padctl pad_id [options]     Set the configuration for a pad.\n"
L"  where options are:\n"
L"  -pus 100pd|47pu|100pu|22pu Specify pull strength (default: 100pu)\n"
L"  -pue keep|pull             Specify keep or pull (default: pull)\n"
L"  -pke                       Enable pull/keep (default: disable)\n"
L"  -ode                       Enable open-drain (default: disable)\n"
L"  -spd low|med|max           Specify pad speed (default: medium)\n"
L"  -dse hiz|260|130|90|       Specify drive strength (default: 130Ohm)\n"
L"       60|50|40|33\n"
L"  -sre slow|fast             Specify slew rate (default: slow)\n"
L" pads                        Dump the friendly names of supported pads\n"
L" dump                        Dump miscellaneous information about the CCM/GPC\n"
L"\n"
L"Examples:\n"
L"  Dump all clocks:\n"
L"    mxpowerutil clocks\n"
L"\n"
L"  Dump the state of all clock gates:\n"
L"    mxpowerutil gates\n"
L"\n"
L"  Ungate a clock:\n"
L"    mxpowerutil setgate MX6_SSI1_CLK_ROOT on\n"
L"\n"
L"  Profile DDR activity:\n"
L"    mxpowerutil ddrprof\n"
L"\n"
L"  Profile each peripheral's DDR activity (takes ~30 seconds):\n"
L"    mxpowerutil ddrprof -all\n"
L"\n"
L"  Expose AXI_CLK_ROOT / 8 on CCM_CLKO1:\n"
L"    mxpowerutil pinout AXI_CLK_ROOT\n"
L"\n"
L"  Dump settings for GPIO0:\n"
L"    mxpowerutil pad gpio0\n"
L"\n"
L"  Configure GPIO0 to CLKO1 function:\n"
L"    mxpowerutil padmux gpio0 0\n"
L"    mxpowerutil padctl gpio0 -spd max -dse 50 -sre fast\n"
L"\n"
L"  Dump the friendly names of supported pads:\n"
L"    mxpowerutil pads\n";

    wprintf(Usage);
}

int mainexcpt (_In_ int argc, _In_reads_(argc) wchar_t* argv[])
{
    if (argc < 2) {
        fwprintf(
            stderr,
            L"Missing required parameter 'command'. Run 'mxpowerutil /?' for usage.\n");

        return 1;
    }

    PCWSTR command = argv[1];
    if (!_wcsicmp(command, L"-h") || !_wcsicmp(command, L"/h") ||
        !_wcsicmp(command, L"-?") || !_wcsicmp(command, L"/?")) {

        PrintUsage();
        return 0;
    }

    if (!_wcsicmp(command, L"clocks")) {
        auto pepHandle = OpenMx6PepHandle();
        DumpClockTree(pepHandle.Get());
        return 0;
    } else if (!_wcsicmp(command, L"gates")) {
        auto pepHandle = OpenMx6PepHandle();
        DumpGates(pepHandle.Get());
        return 0;
    } else if (!_wcsicmp(command, L"setgate")) {
        return HandleSetGateCommand(argc, argv);
    } else if (!_wcsicmp(command, L"ddrprof")) {
        return HandleDdrProfCommand(argc, argv);
    } else if (!_wcsicmp(command, L"pinout")) {
        return HandlePinoutCommand(argc, argv);
    } else if (!_wcsicmp(command, L"pad")) {
        return HandlePadCommand(argc, argv);
    } else if (!_wcsicmp(command, L"padmux")) {
        return HandlePadMuxCommand(argc, argv);
    } else if (!_wcsicmp(command, L"padctl")) {
        return HandlePadCtlCommand(argc, argv);
    } else if (!_wcsicmp(command, L"pads")) {
        return HandlePadsCommand(argc, argv);
    } else if (!_wcsicmp(command, L"dump")) {
        return HandleDumpCommand(argc, argv);
    } else {
        fwprintf(
            stderr,
            L"Unrecognized command: %s. Type '%s /?' for usage.\n",
            command,
            argv[0]);

        return 1;
    }
}

int __cdecl wmain (_In_ int argc, _In_reads_(argc) wchar_t* argv[])
{
    try {
        return mainexcpt(argc, argv);
    } catch (const wexception& ex) {
        fwprintf(stderr, L"Error: %s\n", ex.wwhat());
        return 1;
    }
}

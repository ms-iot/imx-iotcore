// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6pepioctl.h
//
// Abstract:
//
//   IOCTL interface for IMX6 Pep diagnostic facilities
//
// Environment:
//
//   User and kernel mode
//

#ifndef _MX6PEPIOCTL_H_
#define _MX6PEPIOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum { MX6PEP_FILE_DEVICE = FILE_DEVICE_UNKNOWN };

enum {
    MX6PEP_IOCTL_ID_DUMP_REGISTERS = 100,
    MX6PEP_IOCTL_ID_GET_CLOCK_GATE_REGISTERS,
    MX6PEP_IOCTL_ID_SET_CLOCK_GATE,
    MX6PEP_IOCTL_ID_PROFILE_MMDC,
    MX6PEP_IOCTL_ID_WRITE_CCOSR,
    MX6PEP_IOCTL_ID_GET_PAD_CONFIG,
    MX6PEP_IOCTL_ID_SET_PAD_CONFIG,
};

enum MX6_CLK {
    MX6_CLK_NONE,

    MX6_OSC_CLK,
    MX6_PLL1_MAIN_CLK,
    MX6_PLL2_MAIN_CLK,
    MX6_PLL2_PFD0,
    MX6_PLL2_PFD1,
    MX6_PLL2_PFD2,
    MX6_PLL3_MAIN_CLK,
    MX6_PLL3_PFD0,
    MX6_PLL3_PFD1,
    MX6_PLL3_PFD2,
    MX6_PLL3_PFD3,
    MX6_PLL4_MAIN_CLK,
    MX6_PLL5_MAIN_CLK,
    MX6_CLK1,
    MX6_CLK2,

    MX6_PLL1_SW_CLK,
    MX6_STEP_CLK,
    MX6_PLL3_SW_CLK,
    MX6_PLL3_80M,
    MX6_AXI_ALT,
    MX6_AXI_CLK_ROOT,
    MX6_PERIPH_CLK2,
    MX6_PERIPH_CLK,
    MX6_PRE_PERIPH_CLK,
    MX6_PRE_PERIPH2_CLK,
    MX6_PERIPH2_CLK,

    MX6_ARM_CLK_ROOT,
    MX6_MMDC_CH0_CLK_ROOT,
    MX6_MMDC_CH1_CLK_ROOT,
    MX6_AHB_CLK_ROOT,
    MX6_IPG_CLK_ROOT,
    MX6_PERCLK_CLK_ROOT,
    MX6_USDHC1_CLK_ROOT,
    MX6_USDHC2_CLK_ROOT,
    MX6_USDHC3_CLK_ROOT,
    MX6_USDHC4_CLK_ROOT,
    MX6_SSI1_CLK_ROOT,
    MX6_SSI2_CLK_ROOT,
    MX6_SSI3_CLK_ROOT,
    MX6_GPU2D_AXI_CLK_ROOT,
    MX6_GPU3D_AXI_CLK_ROOT,
    MX6_PCIE_AXI_CLK_ROOT,
    MX6_VDO_AXI_CLK_ROOT,
    MX6_IPU1_HSP_CLK_ROOT,
    MX6_IPU2_HSP_CLK_ROOT,
    MX6_GPU2D_CORE_CLK_ROOT,
    MX6_ACLK_EIM_SLOW_CLK_ROOT,
    MX6_ACLK_CLK_ROOT,
    MX6_ENFC_CLK_ROOT,
    MX6_GPU3D_CORE_CLK_ROOT,
    MX6_GPU3D_SHADER_CLK_ROOT,
    MX6_VPU_AXI_CLK_ROOT,
    MX6_IPU1_DI0_CLK_ROOT,
    MX6_IPU1_DI1_CLK_ROOT,
    MX6_IPU2_DI0_CLK_ROOT,
    MX6_IPU2_DI1_CLK_ROOT,
    MX6_LDB_DI0_SERIAL_CLK_ROOT,
    MX6_LDB_DI0_IPU,
    MX6_LDB_DI1_SERIAL_CLK_ROOT,
    MX6_LDB_DI1_IPU,
    MX6_SPDIF0_CLK_ROOT,
    MX6_SPDIF1_CLK_ROOT,
    MX6_ESAI_CLK_ROOT,
    MX6_HSI_TX_CLK_ROOT,
    MX6_CAN_CLK_ROOT,
    MX6_ECSPI_CLK_ROOT,
    MX6_UART_CLK_ROOT,
    MX6_VIDEO_27M_CLK_ROOT,
    MX6_IPG_CLK_MAC0,

    MX6_CLK_MAX,
};

//
// Clock gating definitions
//

enum MX6_CLK_GATE {
    MX6_AIPS_TZ1_CLK_ENABLE,
    MX6_AIPS_TZ2_CLK_ENABLE,
    MX6_APBHDMA_HCLK_ENABLE,
    MX6_ASRC_CLK_ENABLE,
    MX6_CAAM_SECURE_MEM_CLK_ENABLE,
    MX6_CAAM_WRAPPER_ACLK_ENABLE,
    MX6_CAAM_WRAPPER_IPG_ENABLE,
    MX6_CAN1_CLK_ENABLE,
    MX6_CAN1_SERIAL_CLK_ENABLE,
    MX6_CAN2_CLK_ENABLE,
    MX6_CAN2_SERIAL_CLK_ENABLE,
    MX6_ARM_DBG_CLK_ENABLE,
    MX6_DCIC1_CLK_ENABLE,
    MX6_DCIC2_CLK_ENABLE,
    MX6_DTCP_CLK_ENABLE,
    MX6_ECSPI1_CLK_ENABLE,
    MX6_ECSPI2_CLK_ENABLE,
    MX6_ECSPI3_CLK_ENABLE,
    MX6_ECSPI4_CLK_ENABLE,
    MX6_ECSPI5_CLK_ENABLE,
    MX6_ENET_CLK_ENABLE,
    MX6_EPIT1_CLK_ENABLE,
    MX6_EPIT2_CLK_ENABLE,
    MX6_ESAI_CLK_ENABLE,
    MX6_GPT_CLK_ENABLE,
    MX6_GPT_SERIAL_CLK_ENABLE,
    MX6_GPU2D_CLK_ENABLE,
    MX6_GPU3D_CLK_ENABLE,
    MX6_HDMI_TX_ENABLE,
    MX6_HDMI_TX_ISFRCLK_ENABLE,
    MX6_I2C1_SERIAL_CLK_ENABLE,
    MX6_I2C2_SERIAL_CLK_ENABLE,
    MX6_I2C3_SERIAL_CLK_ENABLE,
    MX6_IIM_CLK_ENABLE,
    MX6_IOMUX_IPT_CLK_IO_ENABLE,
    MX6_IPMUX1_CLK_ENABLE,
    MX6_IPMUX2_CLK_ENABLE,
    MX6_IPMUX3_CLK_ENABLE,
    MX6_IPSYNC_IP2APB_TZASC1_IPG_MASTER_CLK_ENABLE,
    MX6_IPSYNC_IP2APB_TZASC2_IPG_MASTER_CLK_ENABLE,
    MX6_IPSYNC_VDOA_IPG_MASTER_CLK_ENABLE,
    MX6_IPU1_IPU_CLK_ENABLE,
    MX6_IPU1_IPU_DI0_CLK_ENABLE,
    MX6_IPU1_IPU_DI1_CLK_ENABLE,
    MX6_IPU2_IPU_CLK_ENABLE,
    MX6_IPU2_IPU_DI0_CLK_ENABLE,
    MX6_IPU2_IPU_DI1_CLK_ENABLE,
    MX6_LDB_DI0_CLK_ENABLE,
    MX6_LDB_DI1_CLK_ENABLE,
    MX6_MIPI_CORE_CFG_CLK_ENABLE,
    MX6_MLB_CLK_ENABLE,
    MX6_MMDC_CORE_ACLK_FAST_CORE_P0_ENABLE,
    MX6_MMDC_CORE_IPG_CLK_P0_ENABLE,
    MX6_OCRAM_CLK_ENABLE,
    MX6_OPENVGAXICLK_CLK_ROOT_ENABLE,
    MX6_PCIE_ROOT_ENABLE,
    MX6_PL301_MX6QFAST1_S133CLK_ENABLE,
    MX6_PL301_MX6QPER1_BCHCLK_ENABLE,
    MX6_PL301_MX6QPER2_MAINCLK_ENABLE,
    MX6_PWM1_CLK_ENABLE,
    MX6_PWM2_CLK_ENABLE,
    MX6_PWM3_CLK_ENABLE,
    MX6_PWM4_CLK_ENABLE,
    MX6_RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE,
    MX6_RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE,
    MX6_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE,
    MX6_RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE,
    MX6_ROM_CLK_ENABLE,
    MX6_SATA_CLK_ENABLE,
    MX6_SDMA_CLK_ENABLE,
    MX6_SPBA_CLK_ENABLE,
    MX6_SPDIF_CLK_ENABLE,
    MX6_SSI1_CLK_ENABLE,
    MX6_SSI2_CLK_ENABLE,
    MX6_SSI3_CLK_ENABLE,
    MX6_UART_CLK_ENABLE,
    MX6_UART_SERIAL_CLK_ENABLE,
    MX6_USBOH3_CLK_ENABLE,
    MX6_USDHC1_CLK_ENABLE,
    MX6_USDHC2_CLK_ENABLE,
    MX6_USDHC3_CLK_ENABLE,
    MX6_USDHC4_CLK_ENABLE,
    MX6_EIM_SLOW_CLK_ENABLE,
    MX6_VDOAXICLK_CLK_ENABLE,
    MX6_VPU_CLK_ENABLE,

    MX6_CLK_GATE_MAX,
};

enum MX6_CCM_CCGR {
    MX6_CCM_CCGR_OFF = 0x0,                 // Clock is off during all modes. Stop enter hardware handshake is disabled.
    MX6_CCM_CCGR_ON_RUN = 0x1,              // Clock is on in run mode, but off in WAIT and STOP modes
    MX6_CCM_CCGR_ON = 0x3,                  // Clock is on during all modes, except STOP mode.
};

//
// IOCTL_MX6PEP_DUMP_REGISTERS
//
// Retreive values of CCM and CCM Analog registers
//
// Input: none
// Output: MX6PEP_DUMP_REGISTERS_OUTPUT
//
enum {
    IOCTL_MX6PEP_DUMP_REGISTERS = ULONG(
        CTL_CODE(
            MX6PEP_FILE_DEVICE,
            MX6PEP_IOCTL_ID_DUMP_REGISTERS,
            METHOD_BUFFERED,
            FILE_READ_DATA))
};


typedef struct _MX6PEP_DUMP_REGISTERS_OUTPUT {
    struct {
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
        ULONG CDHIPR;                          // 0x48 CCM Divider Handshake In-Process Register (CCM_CDHIPR)
        ULONG CLPCR;                           // 0x54 CCM Low Power Control Register (CCM_CLPCR)
        ULONG CISR;                            // 0x58 CCM Interrupt Status Register (CCM_CISR)
        ULONG CIMR;                            // 0x5C CCM Interrupt Mask Register (CCM_CIMR)
        ULONG CCOSR;                           // 0x60 CCM Clock Output Source Register (CCM_CCOSR)
        ULONG CGPR;                            // 0x64 CCM General Purpose Register (CCM_CGPR)
        ULONG CCGR[7];                         // 0x68-7C CCM Clock Gating Register 0-6 (CCM_CCGR0-CCM_CCGR6)
        ULONG CMEOR;                           // 0x88 CCM Module Enable Override Register (CCM_CMEOR)
    } Ccm;

    struct {
        ULONG PLL_ARM;                         // 0x000 Analog ARM PLL control Register (CCM_ANALOG_PLL_ARM)
        ULONG PLL_USB1;                        // 0x010 Analog USB1 480MHz PLL Control Register (CCM_ANALOG_PLL_USB1)
        ULONG PLL_USB2;                        // 0x020 Analog USB2 480MHz PLL Control Register (CCM_ANALOG_PLL_USB2)
        ULONG PLL_SYS;                         // 0x030 Analog System PLL Control Register (CCM_ANALOG_PLL_SYS)
        ULONG PLL_SYS_SS;                      // 0x040 528MHz System PLL Spread Spectrum Register (CCM_ANALOG_PLL_SYS_SS)
        ULONG PLL_SYS_NUM;                     // 0x050 Numerator of 528MHz System PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_SYS_NUM)
        ULONG PLL_SYS_DENOM;                   // 0x060 Denominator of 528MHz System PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_SYS_DENOM)
        ULONG PLL_AUDIO;                       // 0x070 Analog Audio PLL control Register (CCM_ANALOG_PLL_AUDIO)
        ULONG PLL_AUDIO_NUM;                   // 0x080 Numerator of Audio PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_AUDIO_NUM)
        ULONG PLL_AUDIO_DENOM;                 // 0x090 Denominator of Audio PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_AUDIO_DENOM)
        ULONG PLL_VIDEO;                       // 0x0A0 Analog Video PLL control Register (CCM_ANALOG_PLL_VIDEO)
        ULONG PLL_VIDEO_NUM;                   // 0x0B0 Numerator of Video PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_VIDEO_NUM)
        ULONG PLL_VIDEO_DENOM;                 // 0x0C0 Denominator of Video PLL Fractional Loop Divider Register (CCM_ANALOG_PLL_VIDEO_DENOM)
        ULONG PLL_MLB;                         // 0x0D0 MLB PLL Control Register (CCM_ANALOG_PLL_MLB)
        ULONG PLL_ENET;                        // 0x0E0 Analog ENET PLL Control Register (CCM_ANALOG_PLL_ENET)
        ULONG PFD_480;                         // 0x0F0 480MHz Clock (PLL3) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_480)
        ULONG PFD_528;                         // 0x100 528MHz Clock (PLL2) Phase Fractional Divider Control Register (CCM_ANALOG_PFD_528)
        ULONG PMU_REG_1P1;                     // 0x110 Regulator 1P1 Register (PMU_REG_1P1)
        ULONG PMU_REG_3P0;                     // 0X120 Regulator 3P0 Register (PMU_REG_3P0)
        ULONG PMU_REG_2P5;                     // 0x130 Regulator 2P5 Register (PMU_REG_2P5)
        ULONG PMU_REG_CORE;                    // 0x140 Digital Regulator Core Register (PMU_REG_CORE)
        ULONG MISC0;                           // 0x150 Miscellaneous Register 0 (CCM_ANALOG_MISC0)
        ULONG MISC1;                           // 0x160 Miscellaneous Register 1 (CCM_ANALOG_MISC1)
        ULONG MISC2;                           // 0x170 Miscellaneous Register 2 (CCM_ANALOG_MISC2)
    } Analog;

    struct {
        ULONG CNTR;                            // 0x000 GPC Interface control register (GPC_CNTR)
        ULONG PGR;                             // 0x004 GPC Power Gating Register (GPC_PGR)
        ULONG IMR[4];                          // 0x008 - 0x018 Interrupt mask registers
        ULONG ISR[4];                          // 0x018 - 0x028 IRQ status resisters

        struct {                               // 0x260-0x26C GPU PGC Control
            ULONG CTRL;
            ULONG PUPSCR;
            ULONG PDNSCR;
            ULONG SR;
        } PGC_GPU;

        struct {                               // 0x2A0-0x2AC CPU PGC Control
            ULONG CTRL;
            ULONG PUPSCR;
            ULONG PDNSCR;
            ULONG SR;
        } PGC_CPU;
    } Gpc;

} MX6PEP_DUMP_REGISTERS_OUTPUT, *PMX6PEP_DUMP_REGISTERS_OUTPUT;

//
// IOCTL_MX6PEP_GET_CLOCK_GATE_REGISTERS
//
// Get the raw values of the clock gating registers (CCGR[0-6])
//
// Input: None
// Output: MX6PEP_GET_CLOCK_GATE_REGISTERS_OUTPUT
//
enum {
    IOCTL_MX6PEP_GET_CLOCK_GATE_REGISTERS = ULONG(
        CTL_CODE(
            MX6PEP_FILE_DEVICE,
            MX6PEP_IOCTL_ID_GET_CLOCK_GATE_REGISTERS,
            METHOD_BUFFERED,
            FILE_READ_DATA))
};

typedef struct _MX6PEP_GET_CLOCK_GATE_REGISTERS_OUTPUT {
    ULONG CcgrRegisters[7];
} MX6PEP_GET_CLOCK_GATE_REGISTERS_OUTPUT, *PMX6PEP_GET_CLOCK_REGISTERS_OUTPUT;

//
// IOCTL_MX6PEP_SET_CLOCK_GATE
//
// Configure a clock gate.
//
// Input: MX6PEP_SET_CLOCK_GATE_INPUT
// Output: None
//
enum {
    IOCTL_MX6PEP_SET_CLOCK_GATE = ULONG(
        CTL_CODE(
            MX6PEP_FILE_DEVICE,
            MX6PEP_IOCTL_ID_SET_CLOCK_GATE,
            METHOD_BUFFERED,
            FILE_WRITE_DATA))
};

typedef struct _MX6PEP_SET_CLOCK_GATE_INPUT {
    MX6_CLK_GATE ClockGate;
    MX6_CCM_CCGR State;
} MX6PEP_SET_CLOCK_GATE_INPUT, *PMX6PEP_SET_CLOCK_GATE_INPUT;

//
// IOCTL_MX6PEP_PROFILE_MMDC
//
// Profile the MMDC (DDR) controller.
//
// Input: MX6PEP_PROFILE_MMDC_INPUT
// Output: MX6PEP_PROFILE_MMDC_OUTPUT
//
enum {
    IOCTL_MX6PEP_PROFILE_MMDC = ULONG(
        CTL_CODE(
            MX6PEP_FILE_DEVICE,
            MX6PEP_IOCTL_ID_PROFILE_MMDC,
            METHOD_BUFFERED,
            FILE_READ_DATA | FILE_WRITE_DATA))
};

enum { MX6_MMDC_PROFILE_DURATION_MAX = 4000 };

typedef struct _MX6PEP_PROFILE_MMDC_INPUT {
    ULONG DurationMillis;
    UINT16 AxiId;
    UINT16 AxiIdMask;
} MX6PEP_PROFILE_MMDC_INPUT, *PMX6PEP_PROFILE_MMDC_INPUT;

typedef struct _MX6PEP_PROFILE_MMDC_OUTPUT {
    ULONG ActualDurationMillis;
    ULONG TotalProfilingCount;
    ULONG BusyCycleCount;
    ULONG ReadAccessCount;
    ULONG WriteAccessCount;
    ULONG BytesRead;
    ULONG BytesWritten;
} MX6PEP_PROFILE_MMDC_OUTPUT, *PMX6PEP_PROFILE_MMDC_OUTPUT;

//
// IOCTL_MX6PEP_WRITE_CCOSR
//
// Write a value to the CCOSR register, which can be used to pin out a clock
// externally.
//
// Input: ULONG (CCOSR register value)
// Output: none
//
enum {
    IOCTL_MX6PEP_WRITE_CCOSR = ULONG(
        CTL_CODE(
            MX6PEP_FILE_DEVICE,
            MX6PEP_IOCTL_ID_WRITE_CCOSR,
            METHOD_BUFFERED,
            FILE_READ_DATA | FILE_WRITE_DATA))
};

//
// IOCTL_MX6PEP_GET_PAD_CONFIG
//
// Get the pad control and mux settings for an IO pad.
//
// Input: MX6PEP_GET_PAD_CONFIG_INPUT
// Output: MX6PEP_GET_PAD_CONFIG_OUTPUT
//
enum {
    IOCTL_MX6PEP_GET_PAD_CONFIG = ULONG(
        CTL_CODE(
            MX6PEP_FILE_DEVICE,
            MX6PEP_IOCTL_ID_GET_PAD_CONFIG,
            METHOD_BUFFERED,
            FILE_READ_DATA))
};

struct MX6_PAD_DESCRIPTOR {
    UINT16 CtrlRegOffset;
    UINT16 MuxRegOffset;
};

typedef struct _MX6PEP_GET_PAD_CONFIG_INPUT {
    MX6_PAD_DESCRIPTOR PadDescriptor;
} MX6PEP_GET_PAD_CONFIG_INPUT, *PMX6PEP_GET_PAD_CONFIG_INPUT;

typedef struct _MX6PEP_GET_PAD_CONFIG_OUTPUT {
    ULONG PadMuxRegister;
    ULONG PadControlRegister;
} MX6PEP_GET_PAD_CONFIG_OUTPUT, *PMX6PEP_GET_PAD_CONFIG_OUTPUT;

//
// IOCTL_MX6PEP_SET_PAD_CONFIG
//
// Set the pad control and mux settings for an IO pad.
// Set PadMuxRegister or PadControlRegister to PAD_CONFIG_INVALID to skip
// setting the register.
//
// Input: MX6PEP_SET_PAD_CONFIG_INPUT
// Output: none
//
enum {
    IOCTL_MX6PEP_SET_PAD_CONFIG = ULONG(
        CTL_CODE(
            MX6PEP_FILE_DEVICE,
            MX6PEP_IOCTL_ID_SET_PAD_CONFIG,
            METHOD_BUFFERED,
            FILE_READ_DATA | FILE_WRITE_DATA))
};

enum { MX6_PAD_REGISTER_INVALID = 0xffffffff };

typedef struct _MX6PEP_SET_PAD_CONFIG_INPUT {
    MX6_PAD_DESCRIPTOR PadDescriptor;
    ULONG PadMuxRegister;
    ULONG PadControlRegister;
} MX6PEP_SET_PAD_CONFIG_INPUT, *PMX6PEP_SET_PAD_CONFIG_INPUT;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // _MX6PEPIOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// {828C6655-A8E1-4409-B505-87DAC2E24DDE}
DEFINE_GUID(
    GUID_DEVINTERFACE_MX6PEP,
    0x828c6655, 0xa8e1, 0x4409, 0xb5, 0x5, 0x87, 0xda, 0xc2, 0xe2, 0x4d, 0xde);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

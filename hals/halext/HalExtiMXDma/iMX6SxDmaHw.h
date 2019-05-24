/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    iMX6SxSdmaHw.h

Abstract:

    This file includes the types/enums definitions associated with the
    i.MX6 SoloX SDMA controller configuration.

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _IMX6_SDMA_SOLOX_HW_H
#define _IMX6_SDMA_SOLOX_HW_H

//
// ----------------------------------------------------------- Type Definitions
//


//
// IMX6 SDMA event definitions.
// These values match the datasheet.
//
typedef enum _IMX6SX_SDMA_EVENT_ID {
    IMX6SX_SDMA_EVENT_UART6_RX = 0,
    IMX6SX_SDMA_EVENT_ADC1_I2C4 = 1,
    IMX6SX_SDMA_EVENT_IOMUX1_CSI2 = 2,
    IMX6SX_SDMA_EVENT_ECSPI1_RX_I2C3 = 3,
    IMX6SX_SDMA_EVENT_ECSPI1_TX_I2C2 = 4,
    IMX6SX_SDMA_EVENT_ECSPI2_RX_I2C1 = 5,
    IMX6SX_SDMA_EVENT_ECSPI2_TX_PXP = 6,
    IMX6SX_SDMA_EVENT_ECSPI3_RX_CSI1 = 7,
    IMX6SX_SDMA_EVENT_ECSPI3_TX_LCDIF1 = 8,
    IMX6SX_SDMA_EVENT_ECSPI4_RX_EPIT2 = 9,
    IMX6SX_SDMA_EVENT_ECSPI4_TX_QSPI1_RX = 10,
    IMX6SX_SDMA_EVENT_ECSPI5_RX_QSPI2_RX = 11,
    IMX6SX_SDMA_EVENT_ECSPI5_TX_LCDIF2 = 12,
    IMX6SX_SDMA_EVENT_ADC2_GPT = 13,
    IMX6SX_SDMA_EVENT_SPDIF_RX_IOMUX2 = 14,
    IMX6SX_SDMA_EVENT_SPDIF_TX = 15,
    IMX6SX_SDMA_EVENT_EPIT1 = 16,
    IMX6SX_SDMA_EVENT_ASRC_RXA = 17,
    IMX6SX_SDMA_EVENT_ASRC_RXB = 18,
    IMX6SX_SDMA_EVENT_ASRC_RXC = 19,
    IMX6SX_SDMA_EVENT_ASRC_TXA = 20,
    IMX6SX_SDMA_EVENT_ASRC_TXB = 21,
    IMX6SX_SDMA_EVENT_ASRC_TXC = 22,
    IMX6SX_SDMA_EVENT_ESAI_RX_I2C3_ALT = 23,
    IMX6SX_SDMA_EVENT_ESAI_TX = 24,
    IMX6SX_SDMA_EVENT_UART1_RX = 25,
    IMX6SX_SDMA_EVENT_UART1_TX = 26,
    IMX6SX_SDMA_EVENT_UART2_RX = 27,
    IMX6SX_SDMA_EVENT_UART2_TX = 28,
    IMX6SX_SDMA_EVENT_UART3_RX_QSPI1_TX = 29,
    IMX6SX_SDMA_EVENT_UART3_TX_QSPI2_TX = 30,
    IMX6SX_SDMA_EVENT_UART4_RX_SAI1_RX = 31,
    IMX6SX_SDMA_EVENT_UART4_TX_SAI1_TX = 32,
    IMX6SX_SDMA_EVENT_UART5_RX_SAI2_RX = 33,
    IMX6SX_SDMA_EVENT_UART5_TX_SAI2_TX = 34,
    IMX6SX_SDMA_EVENT_SSI1_RX1 = 35,
    IMX6SX_SDMA_EVENT_SSI1_TX1 = 36,
    IMX6SX_SDMA_EVENT_SSI1_RX0 = 37,
    IMX6SX_SDMA_EVENT_SSI1_TX0 = 38,
    IMX6SX_SDMA_EVENT_SSI2_RX1 = 39,
    IMX6SX_SDMA_EVENT_SSI2_TX1 = 40,
    IMX6SX_SDMA_EVENT_SSI2_RX0 = 41,
    IMX6SX_SDMA_EVENT_SSI2_TX0 = 42,
    IMX6SX_SDMA_EVENT_SSI3_RX1 = 43,
    IMX6SX_SDMA_EVENT_SSI3_TX1 = 44,
    IMX6SX_SDMA_EVENT_SSI3_RX0 = 45,
    IMX6SX_SDMA_EVENT_SSI3_TX0 = 46,
    IMX6SX_SDMA_EVENT_UART6_TX_ESAI_TX = 47
} IMX6SX_SDMA_EVENT_ID;


//
// Shared DMA event selection
//

#define IMX6SX_DMA_EVENT_MUX_SEL0_UART6_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL0_RESERVED 1

#define IMX6SX_DMA_EVENT_MUX_SEL1_ADC1 0
#define IMX6SX_DMA_EVENT_MUX_SEL1_I2C4 1

#define IMX6SX_DMA_EVENT_MUX_SEL2_ECSPI1_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL2_I2C3 1

#define IMX6SX_DMA_EVENT_MUX_SEL3_ECSPI1_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL3_I2C2 1

#define IMX6SX_DMA_EVENT_MUX_SEL4_ECSPI2_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL4_I2C1 1

#define IMX6SX_DMA_EVENT_MUX_SEL5_ECSPI2_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL5_PXP 1

#define IMX6SX_DMA_EVENT_MUX_SEL6_ECSPI3_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL6_CSI1 1

#define IMX6SX_DMA_EVENT_MUX_SEL7_ECSPI3_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL7_LCDIF1 1

#define IMX6SX_DMA_EVENT_MUX_SEL8_ECSPI4_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL8_EPIT2 1

#define IMX6SX_DMA_EVENT_MUX_SEL9_ECSPI4_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL9_RESERVED 1

#define IMX6SX_DMA_EVENT_MUX_SEL10_ECSPI5_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL10_RESERVED 1

#define IMX6SX_DMA_EVENT_MUX_SEL11_ECSPI5 0
#define IMX6SX_DMA_EVENT_MUX_SEL11_LCDIF2 1

#define IMX6SX_DMA_EVENT_MUX_SEL12_ADC2 0
#define IMX6SX_DMA_EVENT_MUX_SEL12_GPT 1

#define IMX6SX_DMA_EVENT_MUX_SEL13_SPDIF_B 0
#define IMX6SX_DMA_EVENT_MUX_SEL13_IOMUX 1

#define IMX6SX_DMA_EVENT_MUX_SEL14_ESAI_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL14_I2C3 1

#define IMX6SX_DMA_EVENT_MUX_SEL15_UART4_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL15_SAI1_RX 1

#define IMX6SX_DMA_EVENT_MUX_SEL16_UART4_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL16_SAI1_TX 1

#define IMX6SX_DMA_EVENT_MUX_SEL17_UART5_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL17_SAI2_RX 1

#define IMX6SX_DMA_EVENT_MUX_SEL18_UART5_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL18_SAI2_TX 1

#define IMX6SX_DMA_EVENT_MUX_SEL19_UART6_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL19_RESERVED 1

#define IMX6SX_DMA_EVENT_MUX_SEL20_IOMUX 0
#define IMX6SX_DMA_EVENT_MUX_SEL20_CSI2 1

#define IMX6SX_DMA_EVENT_MUX_SEL21_UART3_RX 0
#define IMX6SX_DMA_EVENT_MUX_SEL21_RESERVED 1

#define IMX6SX_DMA_EVENT_MUX_SEL22_UART3_TX 0
#define IMX6SX_DMA_EVENT_MUX_SEL22_RESERVED 1

//
// Peripheral registers addresses for pPeripheral - peripheral transfers
//

#define IMX6SX_ESAI_BASE_ADDRESS   0x02024000UL
    #define IMX6SX_ESAI_ETDR_OFFSET    0UL


//
// -------------------------------------------------------------------- Globals
//


//
// IMX6 SoloX DMA request -> Channel configuration
//

SDMA_CHANNEL_CONFIG Imx6SxDmaReqToChannelConfig[] = {

    // (0) SDMA_REQ_VPU
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_VPU,                   // DMA request ID
    },

    // (1) SDMA_REQ_IPU2
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_IPU2,                  // DMA request ID
    },

    // (2) SDMA_REQ_IPU1
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_IPU1,                  // DMA request ID
    },

    // (3) SDMA_REQ_HDMI_AUDIO
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_HDMI_AUDIO,            // DMA request ID
    },

    // (4) SDMA_REQ_ECSPI1_RX -> Event 3
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ECSPI1_RX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI1_RX_I2C3,
                DMA_EVENT_MUX_SEL2,
                IMX6SX_DMA_EVENT_MUX_SEL2_ECSPI1_RX
            },
        },
    },

    // (5) SDMA_REQ_ECSPI1_TX -> Event 4
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ECSPI1_TX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI1_TX_I2C2,
                DMA_EVENT_MUX_SEL3,
                IMX6SX_DMA_EVENT_MUX_SEL3_ECSPI1_TX
            },
        },
    },

    // (6) SDMA_REQ_ECSPI2_RX -> Event 5
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ECSPI2_RX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI2_RX_I2C1,
                DMA_EVENT_MUX_SEL4,
                IMX6SX_DMA_EVENT_MUX_SEL4_ECSPI2_RX
            },
        },
    },

    // (7) SDMA_REQ_ECSPI2_TX -> Event 6
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ECSPI2_TX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI2_TX_PXP,
                DMA_EVENT_MUX_SEL5,
                IMX6SX_DMA_EVENT_MUX_SEL5_ECSPI2_TX
            },
        },
    },

    // (8) SDMA_REQ_ECSPI3_RX -> Event 7
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ECSPI3_RX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI3_RX_CSI1,
                DMA_EVENT_MUX_SEL6,
                IMX6SX_DMA_EVENT_MUX_SEL6_ECSPI3_RX
            }
        },
    },

    // (9) SDMA_REQ_ECSPI3_TX -> Event 8
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ECSPI3_TX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI3_TX_LCDIF1, 
                DMA_EVENT_MUX_SEL7,
                IMX6SX_DMA_EVENT_MUX_SEL7_ECSPI3_TX
            },
        },
    },

    // (10) SDMA_REQ_ECSPI4_RX -> Event 9
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ECSPI4_RX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI4_RX_EPIT2,
                DMA_EVENT_MUX_SEL8,
                IMX6SX_DMA_EVENT_MUX_SEL8_ECSPI4_RX
            }
        },
    },

    // (11) SDMA_REQ_ECSPI4_TX -> Event 10
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ECSPI4_TX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI4_TX_QSPI1_RX,
                DMA_EVENT_MUX_SEL9,
                IMX6SX_DMA_EVENT_MUX_SEL9_ECSPI4_TX
            }
        },
    },

    // (12) SDMA_REQ_ECSPI5_RX -> Event 11
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ECSPI5_RX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI5_RX_QSPI2_RX, 
                DMA_EVENT_MUX_SEL10,
                IMX6SX_DMA_EVENT_MUX_SEL10_ECSPI5_RX
            },
        },
    },

    // (13) SDMA_REQ_ECSPI5_TX -> Event 12
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ECSPI5_TX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI5_TX_LCDIF2, 
                DMA_EVENT_MUX_SEL11,
                IMX6SX_DMA_EVENT_MUX_SEL11_ECSPI5
            },
        },
    },

    // (14) SDMA_REQ_I2C1_RX -> Event 5
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C1_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI2_RX_I2C1,
                DMA_EVENT_MUX_SEL4,
                IMX6SX_DMA_EVENT_MUX_SEL4_I2C1
            },
        },
    },

    // (15) SDMA_REQ_I2C1_TX -> Event 5
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C1_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI2_RX_I2C1,
                DMA_EVENT_MUX_SEL4,
                IMX6SX_DMA_EVENT_MUX_SEL4_I2C1
            },
        },
    },

    // (16) SDMA_REQ_I2C2_RX -> Event 4
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C2_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI1_TX_I2C2,
                DMA_EVENT_MUX_SEL3,
                IMX6SX_DMA_EVENT_MUX_SEL3_I2C2
            }
        },
    },

    // (17) SDMA_REQ_I2C2_TX -> Event 4
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C2_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI1_TX_I2C2,
                DMA_EVENT_MUX_SEL3,
                IMX6SX_DMA_EVENT_MUX_SEL3_I2C2
            }
        },
    },

    // (18) SDMA_REQ_I2C3_RX -> Event 3/23
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C3_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI1_RX_I2C3,
                DMA_EVENT_MUX_SEL2,
                IMX6SX_DMA_EVENT_MUX_SEL2_I2C3
            }
        },
    },

    // (19) SDMA_REQ_I2C3_TX -> Event 3/23
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C3_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI1_RX_I2C3,
                DMA_EVENT_MUX_SEL2,
                IMX6SX_DMA_EVENT_MUX_SEL2_I2C3
            }
        },
    },

    // (20) SDMA_REQ_UART1_RX -> Event 25
    {
        imx6_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART1_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART1_RX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (21) SDMA_REQ_UART1_TX -> Event 26
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART1_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART1_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (22) SDMA_REQ_UART2_RX -> Event 27
    {
        imx6_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART2_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART2_RX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (23) SDMA_REQ_UART2_TX -> Event 28
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART2_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART2_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (24) SDMA_REQ_UART3_RX -> Event 29
    {
        imx6_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART3_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART3_RX_QSPI1_TX,
                DMA_EVENT_MUX_SEL21,
                IMX6SX_DMA_EVENT_MUX_SEL21_UART3_RX
            },
        },
    },

    // (25) SDMA_REQ_UART3_TX -> Event 30
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART3_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART3_TX_QSPI2_TX, 
                DMA_EVENT_MUX_SEL22,
                IMX6SX_DMA_EVENT_MUX_SEL22_UART3_TX
            },
        },
    },

    // (26) SDMA_REQ_UART4_RX -> Event 31
    {
        imx6_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART4_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART4_RX_SAI1_RX, 
                DMA_EVENT_MUX_SEL15,
                IMX6SX_DMA_EVENT_MUX_SEL15_UART4_RX
            },
        },
    },

    // (27) SDMA_REQ_UART4_TX -> Event 32
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART4_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART4_TX_SAI1_TX, 
                DMA_EVENT_MUX_SEL16,
                IMX6SX_DMA_EVENT_MUX_SEL16_UART4_TX
            },
        },
    },

    // (28) SDMA_REQ_UART5_RX -> Event 33
    {
        imx6_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART5_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART5_RX_SAI2_RX, 
                DMA_EVENT_MUX_SEL17,
                IMX6SX_DMA_EVENT_MUX_SEL17_UART5_RX
            },
        },
    },

    // (29) SDMA_REQ_UART5_TX -> Event 34
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART5_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART5_TX_SAI2_TX, 
                DMA_EVENT_MUX_SEL18,
                IMX6SX_DMA_EVENT_MUX_SEL18_UART5_TX
            },
        },
    },

    // (30) SDMA_REQ_SPDIF_RX -> Event 14
    {
        imx6_spdif_2_mcu_ADDR,          // Script address
        SDMA_REQ_SPDIF_RX,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SPDIF_RX_IOMUX2, 
                DMA_EVENT_MUX_SEL13,
                IMX6SX_DMA_EVENT_MUX_SEL13_SPDIF_B
            },
        },
    },

    // (31) SDMA_REQ_SPDIF_TX -> Event 15
    {
        imx6_mcu_2_spdif_ADDR,          // Script address
        SDMA_REQ_SPDIF_TX,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SPDIF_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    //
    // Timer SDMA events (EPIT1/EPIT2/GPT) can be used for triggering any 
    // channel. We configure it for 32bit memory to memory transfers.
    //

    // (32) SDMA_REQ_EPIT1 -> Event 16
    {
        imx6_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_EPIT1,                 // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_EPIT1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (33) SDMA_REQ_EPIT2 -> Event 9
    {
        imx6_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_EPIT2,                 // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI4_RX_EPIT2,
                DMA_EVENT_MUX_SEL8,
                IMX6SX_DMA_EVENT_MUX_SEL8_EPIT2
            },
        },
    },

    // (34) SDMA_REQ_GPT1 -> Event 13
    {
        imx6_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_GPT1,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ADC2_GPT,
                DMA_EVENT_MUX_SEL12,
                IMX6SX_DMA_EVENT_MUX_SEL12_GPT
            },
        },
    },

    // (35) SDMA_REQ_ASRC_RXA -> Event 17
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ASRC_RXA,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ASRC_RXA, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (36) SDMA_REQ_ASRC_RXB -> Event 18
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ASRC_RXB,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ASRC_RXB, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (37) SDMA_REQ_ASRC_RXC -> Event 19
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ASRC_RXC,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ASRC_RXC, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (38) SDMA_REQ_ASRC_TXA -> Event 20
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ASRC_TXA,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ASRC_TXA, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (39) SDMA_REQ_ASRC_TXB -> Event 21
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ASRC_TXB,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ASRC_TXB, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (40) SDMA_REQ_ASRC_TXC -> Event 22
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ASRC_TXC,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ASRC_TXC, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (41) SDMA_REQ_ESAI_RX -> Event 23
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ESAI_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ESAI_RX_I2C3_ALT,
                DMA_EVENT_MUX_SEL14,
                IMX6SX_DMA_EVENT_MUX_SEL14_ESAI_RX
            }
        },
    },

    // (42) SDMA_REQ_ESAI_TX -> Event 24
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ESAI_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ESAI_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (43) SDMA_REQ_ASRC_TXA_2_ESAI_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_ASRC_TXA_2_ESAI_TX,    // DMA request ID
    },

    // (44) SDMA_REQ_ASRC_TXB_2_ESAI_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_ASRC_TXB_2_ESAI_TX,    // DMA request ID
    },

    // (45) SDMA_REQ_ASRC_TXC_2_ESAI_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_ASRC_TXC_2_ESAI_TX,    // DMA request ID
    },


    // (46) SDMA_REQ_SSI1_RX1 -> Event 35
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SSI1_RX1,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI1_RX1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (47) SDMA_REQ_SSI1_TX1 -> Event 36
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SSI1_TX1,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI1_TX1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (48) SDMA_REQ_SSI1_RX0 -> Event 37
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SSI1_RX0,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI1_RX0, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (49) SDMA_REQ_SSI1_TX0 -> Event 38
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SSI1_TX0,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI1_TX0, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (50) SDMA_REQ_SSI2_RX1 -> Event 39
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SSI2_RX1,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI2_RX1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (51) SDMA_REQ_SSI2_TX1 -> Event 40
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SSI2_TX1,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI2_TX1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (52) SDMA_REQ_SSI2_RX0 -> Event 41
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SSI2_RX0,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI2_RX0, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (53) SDMA_REQ_SSI2_TX0 -> Event 42
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SSI2_TX0,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI2_TX0, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (54) SDMA_REQ_SSI3_RX1 -> Event 43
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SSI3_RX1,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI3_RX1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (55) SDMA_REQ_SSI3_TX1 -> Event 44
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SSI3_TX1,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI3_TX1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (56) SDMA_REQ_SSI3_RX0 -> Event 45
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SSI3_RX0,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI3_RX0, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (57) SDMA_REQ_SSI3_TX0 -> Event 46
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SSI3_TX0,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_SSI3_TX0, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (58) SDMA_REQ_EXT1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EXT1
    },

    // (59) SDMA_REQ_EXT2
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EXT2
    },

    // (60) SDMA_REQ_UART6_RX -> Event 0
    {
        imx6_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART6_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART6_RX,
                DMA_EVENT_MUX_SEL0,
                IMX6SX_DMA_EVENT_MUX_SEL0_UART6_RX
            },
        },
    },

    // (61) SDMA_REQ_UART6_TX -> Event 47
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART6_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART6_TX_ESAI_TX,
                DMA_EVENT_MUX_SEL19, // -- check
                IMX6SX_DMA_EVENT_MUX_SEL19_UART6_TX
            },
        },
    },

    // (62) SDMA_REQ_ADC1 -> Event 1
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_ADC1,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ADC1_I2C4,
                DMA_EVENT_MUX_SEL1,
                IMX6SX_DMA_EVENT_MUX_SEL1_ADC1
            },
        },
    },

    // (63) SDMA_REQ_ADC2 -> Event 13
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_ADC2,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ADC2_GPT,
                DMA_EVENT_MUX_SEL12,
                IMX6SX_DMA_EVENT_MUX_SEL12_ADC2
            },
        },
    },

    // (64) SDMA_REQ_I2C4_RX -> Event 1
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C4_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ADC1_I2C4,
                DMA_EVENT_MUX_SEL1,
                IMX6SX_DMA_EVENT_MUX_SEL1_I2C4
            }
        },
    },

    // (65) SDMA_REQ_I2C4_TX -> Event 1
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C4_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ADC1_I2C4,
                DMA_EVENT_MUX_SEL1,
                IMX6SX_DMA_EVENT_MUX_SEL1_I2C4
            }
        },
    },

    // (66) SDMA_REQ_CSI1 -> Event 7
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_CSI1,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI3_RX_CSI1,
                DMA_EVENT_MUX_SEL6,
                IMX6SX_DMA_EVENT_MUX_SEL6_CSI1
            },
        },
    },

    // (67) SDMA_REQ_CSI2 -> Event 2
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_CSI2,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_IOMUX1_CSI2,
                DMA_EVENT_MUX_SEL20,
                IMX6SX_DMA_EVENT_MUX_SEL20_CSI2
            },
        },
    },

    // (68) SDMA_REQ_PXP -> Event 6
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_PXP,                   // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI2_TX_PXP,
                DMA_EVENT_MUX_SEL5,
                IMX6SX_DMA_EVENT_MUX_SEL5_PXP
            },
        },
    },

    // (69) SDMA_REQ_LCDIF1 -> Event 8
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_LCDIF1,                // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI3_TX_LCDIF1,
                DMA_EVENT_MUX_SEL7,
                IMX6SX_DMA_EVENT_MUX_SEL7_LCDIF1
            },
        },
    },

    // (70) SDMA_REQ_LCDIF2 -> Event 12
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_LCDIF2,                // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI5_TX_LCDIF2,
                DMA_EVENT_MUX_SEL11,
                IMX6SX_DMA_EVENT_MUX_SEL11_LCDIF2
            },
        },
    },

    // (71) SDMA_REQ_QSPI1_RX -> Event 10
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_QSPI1_RX,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI4_TX_QSPI1_RX,
                DMA_EVENT_MUX_SEL9,
                IMX6SX_DMA_EVENT_MUX_SEL9_RESERVED
            },
        },
    },

    // (72) SDMA_REQ_QSPI1_TX -> Event 29
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_QSPI1_TX,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART3_RX_QSPI1_TX,
                DMA_EVENT_MUX_SEL21,
                IMX6SX_DMA_EVENT_MUX_SEL21_RESERVED
            },
        },
    },

    // (73) SDMA_REQ_QSPI2_RX -> Event 11
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_QSPI2_RX,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_ECSPI5_RX_QSPI2_RX,
                DMA_EVENT_MUX_SEL10,
                IMX6SX_DMA_EVENT_MUX_SEL10_RESERVED
            },
        },
    },

    // (74) SDMA_REQ_QSPI2_TX -> Event 30
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_QSPI2_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART3_TX_QSPI2_TX,
                DMA_EVENT_MUX_SEL22,
                IMX6SX_DMA_EVENT_MUX_SEL22_RESERVED
            },
        },
    },

    // (75) SDMA_REQ_SAI1_TX -> Event 32
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SAI1_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART4_TX_SAI1_TX,
                DMA_EVENT_MUX_SEL16,
                IMX6SX_DMA_EVENT_MUX_SEL16_SAI1_TX
            },
        },
    },

    // (76) SDMA_REQ_SAI1_RX -> Event 31
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI1_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART4_RX_SAI1_RX,
                DMA_EVENT_MUX_SEL15,
                IMX6SX_DMA_EVENT_MUX_SEL15_SAI1_RX
            },
        },
    },

    // (77) SDMA_REQ_SAI2_TX -> Event 32
    {
        imx6_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_SAI2_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART5_TX_SAI2_TX,
                DMA_EVENT_MUX_SEL18,
                IMX6SX_DMA_EVENT_MUX_SEL18_SAI2_TX
            },
        },
    },

    // (78) SDMA_REQ_SAI2_RX -> Event 33
    {
        imx6_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI2_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6SX_SDMA_EVENT_UART5_RX_SAI2_RX,
                DMA_EVENT_MUX_SEL17,
                IMX6SX_DMA_EVENT_MUX_SEL17_SAI2_RX
            },
        },
    },
};
ULONG Imx6SxDmaReqMax = ARRAYSIZE(Imx6SxDmaReqToChannelConfig) - 1;


#endif // !_IMX6_SDMA_SOLOX_HW_H

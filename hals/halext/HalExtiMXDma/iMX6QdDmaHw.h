/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    iMX6QdDmaHw.h

Abstract:

    This file includes the types/enums definitions associated with the
    i.MX6 Quad/Dual SDMA controller configuration.

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _IMX6_SDMA_QUAD_DUAL_HW_H
#define _IMX6_SDMA_QUAD_DUAL_HW_H

//
// ----------------------------------------------------------- Type Definitions
//


//
// IMX6 SDMA event definitions.
// These values match the datasheet.
//
typedef enum _IMX6QD_SDMA_EVENT_ID {
    IMX6_SDMA_EVENT_VPU = 0,
    IMX6_SDMA_EVENT_IPU2 = 1,
    IMX6_SDMA_EVENT_IPU1_HDMI_AUDIO = 2,
    IMX6_SDMA_EVENT_ECSPI1_RX_I2C3 = 3,
    IMX6_SDMA_EVENT_ECSPI1_TX_I2C2 = 4,
    IMX6_SDMA_EVENT_ECSPI2_RX_I2C1 = 5,
    IMX6_SDMA_EVENT_ECSPI2_TX = 6,
    IMX6_SDMA_EVENT_ECSPI3_RX = 7,
    IMX6_SDMA_EVENT_ECSPI3_TX = 8,
    IMX6_SDMA_EVENT_ECSPI4_RX_EPIT2 = 9,
    IMX6_SDMA_EVENT_ECSPI4_TX_I2C1_ALT = 10,
    IMX6_SDMA_EVENT_ECSPI5_RX = 11,
    IMX6_SDMA_EVENT_ECSPI5_TX = 12,
    IMX6_SDMA_EVENT_GPT = 13,
    IMX6_SDMA_EVENT_SPDIF_RX_EXT1 = 14,
    IMX6_SDMA_EVENT_SPDIF_TX = 15,
    IMX6_SDMA_EVENT_EPIT1 = 16,
    IMX6_SDMA_EVENT_ASRC_RXA = 17,
    IMX6_SDMA_EVENT_ASRC_RXB = 18,
    IMX6_SDMA_EVENT_ASRC_RXC = 19,
    IMX6_SDMA_EVENT_ASRC_TXA = 20,
    IMX6_SDMA_EVENT_ASRC_TXB = 21,
    IMX6_SDMA_EVENT_ASRC_TXC = 22,
    IMX6_SDMA_EVENT_ESAI_RX_I2C3_ALT = 23,
    IMX6_SDMA_EVENT_ESAI_TX = 24,
    IMX6_SDMA_EVENT_UART1_RX = 25,
    IMX6_SDMA_EVENT_UART1_TX = 26,
    IMX6_SDMA_EVENT_UART2_RX = 27,
    IMX6_SDMA_EVENT_UART2_TX = 28,
    IMX6_SDMA_EVENT_UART3_RX = 29,
    IMX6_SDMA_EVENT_UART3_TX = 30,
    IMX6_SDMA_EVENT_UART4_RX = 31,
    IMX6_SDMA_EVENT_UART4_TX = 32,
    IMX6_SDMA_EVENT_UART5_RX = 33,
    IMX6_SDMA_EVENT_UART5_TX = 34,
    IMX6_SDMA_EVENT_SSI1_RX1 = 35,
    IMX6_SDMA_EVENT_SSI1_TX1 = 36,
    IMX6_SDMA_EVENT_SSI1_RX0 = 37,
    IMX6_SDMA_EVENT_SSI1_TX0 = 38,
    IMX6_SDMA_EVENT_SSI2_RX1 = 39,
    IMX6_SDMA_EVENT_SSI2_TX1 = 40,
    IMX6_SDMA_EVENT_SSI2_RX0 = 41,
    IMX6_SDMA_EVENT_SSI2_TX0 = 42,
    IMX6_SDMA_EVENT_SSI3_RX1 = 43,
    IMX6_SDMA_EVENT_SSI3_TX1 = 44,
    IMX6_SDMA_EVENT_SSI3_RX0 = 45,
    IMX6_SDMA_EVENT_SSI3_TX0 = 46,
    IMX6_SDMA_EVENT_RESERVED47 = 47
} IMX6QD_SDMA_EVENT_ID;


//
// Shared DMA event selection
//

#define IMX6QD_DMA_EVENT_MUX_SEL0_IPU1 0
#define IMX6QD_DMA_EVENT_MUX_SEL0_HDMI 1

#define IMX6QD_DMA_EVENT_MUX_SEL1_ECSPI1_RX 0
#define IMX6QD_DMA_EVENT_MUX_SEL1_I2C3 1

#define IMX6QD_DMA_EVENT_MUX_SEL2_ECSPI1_TX 0
#define IMX6QD_DMA_EVENT_MUX_SEL2_I2C2 1

#define IMX6QD_DMA_EVENT_MUX_SEL3_ECSPI2_RX 0
#define IMX6QD_DMA_EVENT_MUX_SEL3_I2C1 1

#define IMX6QD_DMA_EVENT_MUX_SEL4_ECSPI4_TX 0
#define IMX6QD_DMA_EVENT_MUX_SEL4_I2C1 1

#define IMX6QD_DMA_EVENT_MUX_SEL5_ECSPI4_RX 0
#define IMX6QD_DMA_EVENT_MUX_SEL5_EPIT2 1

#define IMX6QD_DMA_EVENT_MUX_SEL6_ESAI 0
#define IMX6QD_DMA_EVENT_MUX_SEL6_I2C3 1

#define IMX6QD_DMA_EVENT_MUX_SEL7_SPDIF_RX 0
#define IMX6QD_DMA_EVENT_MUX_SEL7_EXT1 1


//
// Peripheral registers addresses for peripheral - peripheral transfers
//

#define IMX6QD_ESAI_BASE_ADDRESS   0x02024000UL
#define IMX6QD_ESAI_ETDR_OFFSET    0UL


//
// -------------------------------------------------------------------- Globals
//


//
// IMX6 Quad/Dual DMA request -> Channel configuration
//

SDMA_CHANNEL_CONFIG Imx6QdDmaReqToChannelConfig[] = {

    // (0) SDMA_REQ_VPU -> Event 0
    {
        imx6_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_VPU,                   // DMA request ID
        DMA_WIDTH_NA,                   // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_EXT_ADDRESS,   // Flags
        0,                              // Peripheral 2 address
        0,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6_SDMA_EVENT_VPU, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (1) SDMA_REQ_IPU2 -> Event 1
    {
        imx6_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_IPU2,                  // DMA request ID
        DMA_WIDTH_NA,                   // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_EXT_ADDRESS,   // Flags
        0,                              // Peripheral 2 address
        0,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6_SDMA_EVENT_IPU2, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (2) SDMA_REQ_IPU1 -> Event 2
    {
        imx6_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_IPU1,                  // DMA request ID
        DMA_WIDTH_NA,                   // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_EXT_ADDRESS,   // Flags
        0,                              // Peripheral 2 address
        0,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6_SDMA_EVENT_IPU1_HDMI_AUDIO, 
                DMA_EVENT_MUX_SEL0,
                IMX6QD_DMA_EVENT_MUX_SEL0_IPU1
            }, 
        },
    },

    // (3) SDMA_REQ_HDMI_AUDIO -> Event 2
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Not supported
        SDMA_REQ_HDMI_AUDIO,            // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_EXT_ADDRESS,   // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX6_SDMA_EVENT_IPU1_HDMI_AUDIO,
                DMA_EVENT_MUX_SEL0,
                IMX6QD_DMA_EVENT_MUX_SEL0_HDMI
            },
        },
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
                IMX6_SDMA_EVENT_ECSPI1_RX_I2C3,
                DMA_EVENT_MUX_SEL1,
                IMX6QD_DMA_EVENT_MUX_SEL1_ECSPI1_RX
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
                IMX6_SDMA_EVENT_ECSPI1_TX_I2C2,
                DMA_EVENT_MUX_SEL2,
                IMX6QD_DMA_EVENT_MUX_SEL2_ECSPI1_TX
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
                IMX6_SDMA_EVENT_ECSPI2_RX_I2C1,
                DMA_EVENT_MUX_SEL3,
                IMX6QD_DMA_EVENT_MUX_SEL3_ECSPI2_RX
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
                IMX6_SDMA_EVENT_ECSPI2_TX,
                DMA_EVENT_EXCLUSIVE
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
            {IMX6_SDMA_EVENT_ECSPI3_RX, DMA_EVENT_EXCLUSIVE},
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
                IMX6_SDMA_EVENT_ECSPI3_TX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_ECSPI4_RX_EPIT2,
                DMA_EVENT_MUX_SEL5,
                IMX6QD_DMA_EVENT_MUX_SEL5_ECSPI4_RX
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
                IMX6_SDMA_EVENT_ECSPI4_TX_I2C1_ALT,
                DMA_EVENT_MUX_SEL4,
                IMX6QD_DMA_EVENT_MUX_SEL4_ECSPI4_TX
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
                IMX6_SDMA_EVENT_ECSPI5_RX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_ECSPI5_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (14) SDMA_REQ_I2C1_RX -> Event 5/10
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
                IMX6_SDMA_EVENT_ECSPI2_RX_I2C1,
                DMA_EVENT_MUX_SEL3,
                IMX6QD_DMA_EVENT_MUX_SEL3_I2C1
            },
        },
    },

    // (15) SDMA_REQ_I2C1_TX -> Event 5/10
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
                IMX6_SDMA_EVENT_ECSPI2_RX_I2C1,
                DMA_EVENT_MUX_SEL3,
                IMX6QD_DMA_EVENT_MUX_SEL3_I2C1
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
                IMX6_SDMA_EVENT_ECSPI1_TX_I2C2,
                DMA_EVENT_MUX_SEL4,
                IMX6QD_DMA_EVENT_MUX_SEL4_I2C1
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
                IMX6_SDMA_EVENT_ECSPI1_TX_I2C2,
                DMA_EVENT_MUX_SEL4,
                IMX6QD_DMA_EVENT_MUX_SEL4_I2C1
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
                IMX6_SDMA_EVENT_ECSPI1_RX_I2C3,
                DMA_EVENT_MUX_SEL1,
                IMX6QD_DMA_EVENT_MUX_SEL1_I2C3
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
                IMX6_SDMA_EVENT_ECSPI1_RX_I2C3,
                DMA_EVENT_MUX_SEL1,
                IMX6QD_DMA_EVENT_MUX_SEL1_I2C3
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
                IMX6_SDMA_EVENT_UART1_RX, 
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
                IMX6_SDMA_EVENT_UART1_TX, 
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
                IMX6_SDMA_EVENT_UART2_RX, 
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
                IMX6_SDMA_EVENT_UART2_TX, 
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
                IMX6_SDMA_EVENT_UART3_RX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_UART3_TX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_UART4_RX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_UART4_TX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_UART5_RX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_UART5_TX, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_SPDIF_RX_EXT1, 
                DMA_EVENT_MUX_SEL7,
                IMX6QD_DMA_EVENT_MUX_SEL7_SPDIF_RX
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
                IMX6_SDMA_EVENT_SPDIF_TX, 
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
                IMX6_SDMA_EVENT_EPIT1, 
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
                IMX6_SDMA_EVENT_ECSPI4_RX_EPIT2,
                DMA_EVENT_MUX_SEL5,
                IMX6QD_DMA_EVENT_MUX_SEL5_EPIT2
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
                IMX6_SDMA_EVENT_GPT, 
                DMA_EVENT_EXCLUSIVE
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
                IMX6_SDMA_EVENT_ASRC_RXA, 
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
                IMX6_SDMA_EVENT_ASRC_RXB, 
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
                IMX6_SDMA_EVENT_ASRC_RXC, 
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
                IMX6_SDMA_EVENT_ASRC_TXA, 
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
                IMX6_SDMA_EVENT_ASRC_TXB, 
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
                IMX6_SDMA_EVENT_ASRC_TXC, 
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
                IMX6_SDMA_EVENT_ESAI_RX_I2C3_ALT,
                DMA_EVENT_MUX_SEL6,
                IMX6QD_DMA_EVENT_MUX_SEL6_ESAI
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
                IMX6_SDMA_EVENT_ESAI_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (43) SDMA_REQ_ASRC_TXA_2_ESAI_TX -> Event 20 and 24
    {
        imx6_p_2_p_ADDR,                // Script address
        SDMA_REQ_ASRC_TXA_2_ESAI_TX,    // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS |// Flags
            SDMA_DEVICE_FLAG_P2P,
        IMX6QD_ESAI_BASE_ADDRESS +         // Peripheral 2 address
            IMX6QD_ESAI_ETDR_OFFSET,       
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        2,                              // Number of DMA events to follow
        {
            {
                IMX6_SDMA_EVENT_ASRC_TXA,
                DMA_EVENT_EXCLUSIVE
            },
            {
                IMX6_SDMA_EVENT_ESAI_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (44) SDMA_REQ_ASRC_TXB_2_ESAI_TX -> Event 21 and 24
    {
        imx6_p_2_p_ADDR,                // Script address
        SDMA_REQ_ASRC_TXB_2_ESAI_TX,    // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS |// Flags
            SDMA_DEVICE_FLAG_P2P,
        IMX6QD_ESAI_BASE_ADDRESS +         // Peripheral 2 address
            IMX6QD_ESAI_ETDR_OFFSET,       
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        2,                              // Number of DMA events to follow
        {
            {
                IMX6_SDMA_EVENT_ASRC_TXB,
                DMA_EVENT_EXCLUSIVE
            },
            {
                IMX6_SDMA_EVENT_ESAI_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },

    // (45) SDMA_REQ_ASRC_TXC_2_ESAI_TX -> Event 22 and 24
    {
        imx6_p_2_p_ADDR,                // Script address
        SDMA_REQ_ASRC_TXC_2_ESAI_TX,    // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS |// Flags
            SDMA_DEVICE_FLAG_P2P,
        IMX6QD_ESAI_BASE_ADDRESS +         // Peripheral 2 address
            IMX6QD_ESAI_ETDR_OFFSET,
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        2,                              // Number of DMA events to follow
        {
            {
                IMX6_SDMA_EVENT_ASRC_TXC,
                DMA_EVENT_EXCLUSIVE
            },
            {
                IMX6_SDMA_EVENT_ESAI_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
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
                IMX6_SDMA_EVENT_SSI1_RX1, 
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
                IMX6_SDMA_EVENT_SSI1_TX1, 
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
                IMX6_SDMA_EVENT_SSI1_RX0, 
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
                IMX6_SDMA_EVENT_SSI1_TX0, 
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
                IMX6_SDMA_EVENT_SSI2_RX1, 
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
                IMX6_SDMA_EVENT_SSI2_TX1, 
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
                IMX6_SDMA_EVENT_SSI2_RX0, 
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
                IMX6_SDMA_EVENT_SSI2_TX0, 
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
                IMX6_SDMA_EVENT_SSI3_RX1, 
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
                IMX6_SDMA_EVENT_SSI3_TX1, 
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
                IMX6_SDMA_EVENT_SSI3_RX0, 
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
                IMX6_SDMA_EVENT_SSI3_TX0, 
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
};
ULONG Imx6QdDmaReqMax = ARRAYSIZE(Imx6QdDmaReqToChannelConfig) - 1;


#endif // !_IMX6_SDMA_QUAD_DUAL_HW_H

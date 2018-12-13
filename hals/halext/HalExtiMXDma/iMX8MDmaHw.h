/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    iMX8mDmaHw.h

Abstract:

    This file includes the types/enums definitions associated with the
    i.MX8M SDMA controller configuration.

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _IMX8M_SDMA_HW_H
#define _IMX8M_SDMA_HW_H

//
// ----------------------------------------------------------- Type Definitions
//


//
// IMX8M SDMA event definitions.
// These values match the datasheet.
//
typedef enum _IMX8M_SDMA1_EVENT_ID {
    IMX8M_SDMA1_EVENT_ECSPI1_RX = 0,
    IMX8M_SDMA1_EVENT_ECSPI1_TX = 1,
    IMX8M_SDMA1_EVENT_ECSPI2_RX = 2,
    IMX8M_SDMA1_EVENT_ECSPI2_TX = 3,
    IMX8M_SDMA1_EVENT_SPDIF1_RX = 8,
    IMX8M_SDMA1_EVENT_SPDIF1_TX = 9,
    IMX8M_SDMA1_EVENT_SAI2_RX   = 10,
    IMX8M_SDMA1_EVENT_SAI2_TX   = 11,
    IMX8M_SDMA1_EVENT_SAI3_RX   = 12,
    IMX8M_SDMA1_EVENT_SAI3_TX   = 13,
    IMX8M_SDMA1_EVENT_IOMUX_1   = 14,
    IMX8M_SDMA1_EVENT_IOMUX_2   = 15,
    IMX8M_SDMA1_EVENT_SPDIF2_RX = 16,
    IMX8M_SDMA1_EVENT_SPDIF2_TX = 17,
    IMX8M_SDMA1_EVENT_I2C1_SIM1 = 18,
    IMX8M_SDMA1_EVENT_I2C2_SIM1 = 19,
    IMX8M_SDMA1_EVENT_I2C3_SIM2 = 20,
    IMX8M_SDMA1_EVENT_I2C4_SIM2 = 21,
    IMX8M_SDMA1_EVENT_UART1_RX  = 22,
    IMX8M_SDMA1_EVENT_UART1_TX  = 23,
    IMX8M_SDMA1_EVENT_UART2_RX  = 24,
    IMX8M_SDMA1_EVENT_UART2_TX  = 25,
    IMX8M_SDMA1_EVENT_UART3_RX  = 26,
    IMX8M_SDMA1_EVENT_UART3_TX  = 27,
    IMX8M_SDMA1_EVENT_UART4_RX  = 28,
    IMX8M_SDMA1_EVENT_UART4_TX  = 29,
    IMX8M_SDMA1_EVENT_QSPI1_TX  = 36,
    IMX8M_SDMA1_EVENT_QSPI1_RX  = 37,
    IMX8M_SDMA1_EVENT_GPT1      = 38,
    IMX8M_SDMA1_EVENT_GPT2      = 39,
    IMX8M_SDMA1_EVENT_GPT3      = 40,
    IMX8M_SDMA1_EVENT_ENET1_EV2 = 44,
    IMX8M_SDMA1_EVENT_ENET1_EV0 = 45,
    IMX8M_SDMA1_EVENT_ENET1_EV3 = 46,
    IMX8M_SDMA1_EVENT_ENET1_EV1 = 47
} IMX8M_SDMA1_EVENT_ID;

typedef enum _IMX8M_SDMA2_EVENT_ID {
    IMX8M_SDMA2_EVENT_SAI4_RX   = 0,
    IMX8M_SDMA2_EVENT_SAI4_TX   = 1,
    IMX8M_SDMA2_EVENT_SAI5_RX   = 2,
    IMX8M_SDMA2_EVENT_SAI5_TX   = 3,
    IMX8M_SDMA2_EVENT_SAI6_RX   = 4,
    IMX8M_SDMA2_EVENT_SAI6_TX   = 5,
    IMX8M_SDMA2_EVENT_SAI1_RX   = 8,
    IMX8M_SDMA2_EVENT_SAI1_TX   = 9,
    IMX8M_SDMA2_EVENT_IOMUX_1   = 14,
    IMX8M_SDMA2_EVENT_IOMUX_2   = 15,
    IMX8M_SDMA2_EVENT_GPT4      = 38,
    IMX8M_SDMA2_EVENT_GPT5      = 39,
    IMX8M_SDMA2_EVENT_GPT6      = 40,
} IMX8M_SDMA2_EVENT_ID;

//
// -------------------------------------------------------------------- Globals
//

//
// IMX8 Dual DMA request -> Channel configuration
//

SDMA_CHANNEL_CONFIG Imx8mDmaReqToChannelConfig[] = {
    // (0) SDMA_REQ_VPU
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_VPU
    },
    // (1) SDMA_REQ_IPU2
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_IPU2
    },
    // (2) SDMA_REQ_IPU1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_IPU1
    },
    // (3) SDMA_REQ_HDMI_AUDIO
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_HDMI_AUDIO
    },
    // (4) SDMA_REQ_ECSPI1_RX -> Event 0
    {
        imx7_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ECSPI1_RX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_ECSPI1_RX,
                DMA_EVENT_EXCLUSIVE,
            },
        },
    },
    // (5) SDMA_REQ_ECSPI1_TX -> Event 1
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ECSPI1_TX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_ECSPI1_TX,
                DMA_EVENT_EXCLUSIVE,
            },
        },
    },
    // (6) SDMA_REQ_ECSPI2_RX -> Event 2
    {
        imx7_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_ECSPI2_RX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_ECSPI2_RX,
                DMA_EVENT_EXCLUSIVE,
            },
        },
    },
    // (7) SDMA_REQ_ECSPI2_TX -> Event 3
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_ECSPI2_TX,             // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_ECSPI2_TX,
                DMA_EVENT_EXCLUSIVE,
            },
        },
    },
    // (8) SDMA_REQ_ECSPI3_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ECSPI3_RX
    },
    // (9) SDMA_REQ_ECSPI3_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ECSPI3_TX
    },
    // (10) SDMA_REQ_ECSPI4_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ECSPI4_RX
    },
    // (11) SDMA_REQ_ECSPI4_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ECSPI4_TX
    },
    // (12) SDMA_REQ_ECSPI5_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ECSPI5_RX
    },
    // (13) SDMA_REQ_ECSPI5_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ECSPI5_TX
    },
    // (14) SDMA_REQ_I2C1_RX -> Event 18
    {
        imx7_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C1_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C1_SIM1,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (15) SDMA_REQ_I2C1_TX -> Event 18
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C1_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C1_SIM1,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (16) SDMA_REQ_I2C2_RX -> Event 19
    {
        imx7_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C2_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C2_SIM1,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (17) SDMA_REQ_I2C2_TX -> Event 19
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C2_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C2_SIM1,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (18) SDMA_REQ_I2C3_RX -> Event 20
    {
        imx7_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C3_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C3_SIM2,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (19) SDMA_REQ_I2C3_TX -> Event 20
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C3_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C3_SIM2,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (20) SDMA_REQ_UART1_RX -> Event 22
    {
        imx7_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART1_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART1_RX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (21) SDMA_REQ_UART1_TX -> Event 23
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART1_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART1_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (22) SDMA_REQ_UART2_RX -> Event 24
    {
        imx7_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART2_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART2_RX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (23) SDMA_REQ_UART2_TX -> Event 25
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART2_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART2_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (24) SDMA_REQ_UART3_RX -> Event 26
    {
        imx7_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART3_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART3_RX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (25) SDMA_REQ_UART3_TX -> Event 27
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART3_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART3_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (26) SDMA_REQ_UART4_RX -> Event 28
    {
        imx7_uart_2_mcu_ADDR,           // Script address
        SDMA_REQ_UART4_RX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART4_RX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (27) SDMA_REQ_UART4_TX -> Event 29
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_UART4_TX,              // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_UART4_TX, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (28) SDMA_REQ_UART5_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_UART5_RX
    },
    // (29) SDMA_REQ_UART5_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_UART5_TX
    },
    // (30) SDMA_REQ_SPDIF_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SPDIF_RX
    },
    // (31) SDMA_REQ_SPDIF_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SPDIF_TX
    },
    // (32) SDMA_REQ_EPIT1 -> Event 0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EPIT1
    },
    // (33) SDMA_REQ_EPIT2 -> Event 0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EPIT2
    },
    //
    // Timer SDMA events (GPTx) can be used for triggering any 
    // channel. We configure it for 32bit memory to memory transfers.
    //
    // (34) SDMA_REQ_GPT1 -> Event 38
    {
        imx7_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_GPT1,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_GPT1, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (35) SDMA_REQ_ASRC_RXA
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_RXA
    },
    // (36) SDMA_REQ_ASRC_RXB
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_RXB
    },
    // (37) SDMA_REQ_ASRC_RXC
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_RXC
    },
    // (38) SDMA_REQ_ASRC_TXA
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_TXA
    },
    // (39) SDMA_REQ_ASRC_TXB
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_TXB
    },
    // (40) SDMA_REQ_ASRC_TXC
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_TXC
    },
    // (41) SDMA_REQ_ESAI_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ESAI_RX
    },
    // (42) SDMA_REQ_ESAI_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ESAI_TX
    },
    // (43) SDMA_REQ_ASRC_TXA_2_ESAI_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_TXA_2_ESAI_TX
    },
    // (44) SDMA_REQ_ASRC_TXB_2_ESAI_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_TXB_2_ESAI_TX
    },
    // (45) SDMA_REQ_ASRC_TXC_2_ESAI_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ASRC_TXC_2_ESAI_TX
    },
    // (46) SDMA_REQ_SSI1_RX1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI1_RX1
    },
    // (47) SDMA_REQ_SSI1_TX1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI1_TX1
    },
    // (48) SDMA_REQ_SSI1_RX0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI1_RX0
    },
    // (49) SDMA_REQ_SSI1_TX0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI1_TX0
    },
    // (50) SDMA_REQ_SSI2_RX1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI2_RX1
    },
    // (51) SDMA_REQ_SSI2_TX1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI2_TX1
    },
    // (52) SDMA_REQ_SSI2_RX0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI2_RX0
    },
    // (53) SDMA_REQ_SSI2_TX0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI2_TX0
    },
    // (54) SDMA_REQ_SSI3_RX1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI3_RX1
    },
    // (55) SDMA_REQ_SSI3_TX1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI3_TX1
    },
    // (56) SDMA_REQ_SSI3_RX0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI3_RX0
    },
    // (57) SDMA_REQ_SSI3_TX0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_SSI3_TX0
    },
    // (58) SDMA_REQ_EXT1 -> Map to IMX8M_SDMA1_EVENT_IOMUX_1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EXT1
    },
    // (59) SDMA_REQ_EXT2 -> Map to IMX8M_SDMA1_EVENT_IOMUX_2
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EXT2
    },
    // (60) SDMA_REQ_UART6_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_UART6_RX
    },
    // (61) SDMA_REQ_UART6_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_UART6_TX
    },
    // (62) SDMA_REQ_ADC1
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_ADC1,                  // DMA request ID
    },
    // (63) SDMA_REQ_ADC2
    {
        SDMA_UNSUPPORTED_REQUEST_ID,    // Script address
        SDMA_REQ_ADC2,                  // DMA request ID
    },
    // (64) SDMA_REQ_I2C4_RX -> Event 21
    {
        imx7_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_I2C4_RX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C4_SIM2,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (65) SDMA_REQ_I2C4_TX -> Event 21
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_I2C4_TX,               // DMA request ID
        DMA_WIDTH_8BIT,                 // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_I2C4_SIM2,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (66) SDMA_REQ_CSI1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_CSI1
    },
    // (67) SDMA_REQ_CSI2 -> Event 0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_CSI2
    },
    // (68) SDMA_REQ_PXP
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_PXP
    },
    // (69) SDMA_REQ_LCDIF1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_LCDIF1,
    },
    // (70) SDMA_REQ_LCDIF2
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_LCDIF2
    },
    // (71) SDMA_REQ_QSPI1_RX -> Event 37
    {
        imx7_app_2_mcu_ADDR,            // Script address
        SDMA_REQ_QSPI1_RX,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_QSPI1_RX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (72) SDMA_REQ_QSPI1_TX -> Event 36
    {
        imx7_mcu_2_app_ADDR,            // Script address
        SDMA_REQ_QSPI1_TX,              // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_QSPI1_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (73) SDMA_REQ_QSPI2_RX -> Event 0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_QSPI2_RX
    },
    // (74) SDMA_REQ_QSPI2_TX -> Event 0
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_QSPI2_TX
    },
    // (75) SDMA_REQ_SAI1_TX -> SDMA instance 2 Event 9
    {
        imx7_mcu_2_sai_ADDR,            // Script address
        SDMA_REQ_SAI1_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI1_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (76) SDMA_REQ_SAI1_RX -> SDMA instance 2 Event 8
    {
        imx7_sai_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI1_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI1_RX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (77) SDMA_REQ_SAI2_TX -> Event 11
    {
        imx7_mcu_2_sai_ADDR,            // Script address
        SDMA_REQ_SAI2_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_SAI2_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (78) SDMA_REQ_SAI2_RX -> Event 10
    {
        imx7_sai_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI2_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_SAI2_RX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (79) SDMA_REQ_SAI3_TX -> Event 13
    {
        imx7_mcu_2_sai_ADDR,            // Script address
        SDMA_REQ_SAI3_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_SAI3_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (80) SDMA_REQ_SAI3_RX -> Event 12
    {
        imx7_sai_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI3_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_SAI3_RX,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (81) SDMA_REQ_SAI4_TX -> SDMA2 Event 1
    {
        imx7_mcu_2_sai_ADDR,            // Script address
        SDMA_REQ_SAI4_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI4_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (82) SDMA_REQ_SAI4_RX -> SDMA2 Event 0
    {
        imx7_sai_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI4_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI4_RX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (83) SDMA_REQ_SAI5_TX -> SDMA2 Event 3
    {
        imx7_mcu_2_sai_ADDR,            // Script address
        SDMA_REQ_SAI5_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI5_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (84) SDMA_REQ_SAI5_RX -> SDMA2 Event 2
    {
        imx7_sai_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI5_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI5_RX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (85) SDMA_REQ_SAI6_TX -> SDMA2 Event 5
    {
        imx7_mcu_2_sai_ADDR,            // Script address
        SDMA_REQ_SAI6_TX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI6_TX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (86) SDMA_REQ_SAI6_RX -> SDMA2 Event 4
    {
        imx7_sai_2_mcu_ADDR,            // Script address
        SDMA_REQ_SAI6_RX,               // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_SAI6_RX,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (87) SDMA_REQ_UART7_RX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_UART7_RX
    },
    // (88) SDMA_REQ_UART7_TX
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_UART7_TX
    },
    //
    // Timer SDMA events (GPTx) can be used for triggering any 
    // channel. We configure it for 32bit memory to memory transfers.
    //
    // (89) SDMA_REQ_GPT2 -> Event 39
    {
        imx7_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_GPT2,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_GPT2, 
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (90) SDMA_REQ_GPT3 -> Event 40
    {
        imx7_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_GPT3,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA1_EVENT_GPT3,
                DMA_EVENT_EXCLUSIVE
            },
        },
    },
    // (91) SDMA_REQ_GPT4 -> SDMA instance 2 Event 38
    {
        imx7_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_GPT4,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_GPT4,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (92) SDMA_REQ_GPT5 -> SDMA instance 2 Event 39
    {
        imx7_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_GPT5,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_GPT5,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (93) SDMA_REQ_GPT6 -> SDMA instance 2 Event 40
    {
        imx7_ap_2_ap_ADDR,              // Script address
        SDMA_REQ_GPT6,                  // DMA request ID
        DMA_WIDTH_32BIT,                // Transfer width (24 bit)
        0,                              // Owner channel
        SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
        0,                              // Peripheral 2 address
        1,                              // Watermark level scale (percent)
        1,                              // Number of DMA events to follow
        {
            {
                IMX8M_SDMA2_EVENT_GPT6,
                DMA_EVENT_EXCLUSIVE
            },
        },
        1,                              // SDMA instance 2
    },
    // (94) SDMA_REQ_ENET1_EV0 -> Event 45
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ENET1_EV0
    },
    // (95) SDMA_REQ_ENET1_EV1 -> Event 47
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ENET1_EV1
    },
    // (96) SDMA_REQ_ENET2_EV0 (actually ENET1_EV2) -> Event 44
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ENET2_EV0
    },
    // (97) SDMA_REQ_ENET2_EV1 (actually ENET1_EV3) -> Event 46
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_ENET2_EV1
    },
    // (98) SDMA_REQ_EXT3 -> Map to IMX8M_SDMA2_EVENT_IOMUX_1
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EXT3
    },
    // (99) SDMA_REQ_EXT4 -> Map to IMX8M_SDMA2_EVENT_IOMUX_2
    {
        SDMA_UNSUPPORTED_REQUEST_ID, // Not supported
        SDMA_REQ_EXT4
    },
};

ULONG Imx8mDmaReqMax = ARRAYSIZE(Imx8mDmaReqToChannelConfig) - 1;

#endif // !_IMX8M_SDMA_HW_H

/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

  iMX6UllDmaHw.h

Abstract:

  This file includes the types/enums definitions associated with the
  i.MX6ULL SDMA controller configuration.

Environment:

  Kernel mode only.

Revision History:

*/

#ifndef  _IMX6_SDMA_ULL_HW_H
#define  _IMX6_SDMA_ULL_HW_H

//
// ----------------------------------------------------------- Type Definitions
//


//
// IMX6ULL SDMA event definitions.
// These values match the datasheet.
//
typedef enum _IMX6ULL_SDMA_EVENT_ID {
  IMX6ULL_SDMA_EVENT_UART6_RX_ESAI_RX = 0,
  IMX6ULL_SDMA_EVENT_ADC1 = 1,
  IMX6ULL_SDMA_EVENT_EPIT2_PXP = 2,
  IMX6ULL_SDMA_EVENT_ECSPI1_RX = 3,
  IMX6ULL_SDMA_EVENT_ECSPI1_TX = 4,
  IMX6ULL_SDMA_EVENT_ECSPI2_RX = 5,
  IMX6ULL_SDMA_EVENT_ECSPI2_TX = 6,
  IMX6ULL_SDMA_EVENT_ECSPI3_RX_I2C1 = 7,
  IMX6ULL_SDMA_EVENT_ECSPI3_TX_I2C2 = 8,
  IMX6ULL_SDMA_EVENT_ECSPI4_RX_I2C3 = 9,
  IMX6ULL_SDMA_EVENT_ECSPI4_TX_I2C4 = 10,
  IMX6ULL_SDMA_EVENT_QSPI_RX = 11,
  IMX6ULL_SDMA_EVENT_QSPI_TX = 12,
  IMX6ULL_SDMA_EVENT_ADC2_TSC = 13,
  IMX6ULL_SDMA_EVENT_MUX_BOTTOM_1 = 14,
  IMX6ULL_SDMA_EVENT_MUX_BOTTOM_2 = 15,
  IMX6ULL_SDMA_EVENT_EPIT1_CSI = 16,
  IMX6ULL_SDMA_EVENT_ASRC_RXA = 17,
  IMX6ULL_SDMA_EVENT_ASRC_RXB = 18,
  IMX6ULL_SDMA_EVENT_ASRC_RXC = 19,
  IMX6ULL_SDMA_EVENT_ASRC_TXA = 20,
  IMX6ULL_SDMA_EVENT_ASRC_TXB = 21,
  IMX6ULL_SDMA_EVENT_ASRC_TXC = 22,
  IMX6ULL_SDMA_EVENT_GPT1 = 23,
  IMX6ULL_SDMA_EVENT_GPT2_LCDIF = 24,
  IMX6ULL_SDMA_EVENT_UART1_RX = 25,
  IMX6ULL_SDMA_EVENT_UART1_TX = 26,
  IMX6ULL_SDMA_EVENT_UART2_RX = 27,
  IMX6ULL_SDMA_EVENT_UART2_TX = 28,
  IMX6ULL_SDMA_EVENT_UART3_RX = 29,
  IMX6ULL_SDMA_EVENT_UART3_TX = 30,
  IMX6ULL_SDMA_EVENT_UART4_RX = 31,
  IMX6ULL_SDMA_EVENT_UART4_TX = 32,
  IMX6ULL_SDMA_EVENT_UART5_RX = 33,
  IMX6ULL_SDMA_EVENT_UART5_TX_EPDC = 34,
  IMX6ULL_SDMA_EVENT_SAI1_RX = 35,
  IMX6ULL_SDMA_EVENT_SAI1_TX = 36,
  IMX6ULL_SDMA_EVENT_SAI2_RX = 37,
  IMX6ULL_SDMA_EVENT_SAI2_TX = 38,
  IMX6ULL_SDMA_EVENT_SAI3_RX = 39,
  IMX6ULL_SDMA_EVENT_SAI3_TX = 40,
  IMX6ULL_SDMA_EVENT_SPDIF_RX = 41,
  IMX6ULL_SDMA_EVENT_SPDIF_TX = 42,
  IMX6ULL_SDMA_EVENT_UART7_RX_ENET1_EV0 = 43,
  IMX6ULL_SDMA_EVENT_UART7_TX_ENET1_EV1 = 44,
  IMX6ULL_SDMA_EVENT_UART8_RX_ENET2_EV0 = 45,
  IMX6ULL_SDMA_EVENT_UART8_TX_ENET2_EV1 = 46,
  IMX6ULL_SDMA_EVENT_UART6_TX_ESAI_TX = 47
} IMX6ULL_SDMA_EVENT_ID;


//
// Shared DMA event selection
//
#define IMX6ULL_DMA_EVENT_MUX_SEL15_UART6_TX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL15_ESAI_TX 1

#define IMX6ULL_DMA_EVENT_MUX_SEL14_UART6_RX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL14_ESAI_RX 1

#define IMX6ULL_DMA_EVENT_MUX_SEL13_UART5_TX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL13_EPDC 1

#define IMX6ULL_DMA_EVENT_MUX_SEL11_UART8_TX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL11_ENET2_EV1 1

#define IMX6ULL_DMA_EVENT_MUX_SEL10_UART8_RX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL10_ENET2_EV0 1

#define IMX6ULL_DMA_EVENT_MUX_SEL9_UART7_TX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL9_ENET1_EV1 1

#define IMX6ULL_DMA_EVENT_MUX_SEL8_UART7_RX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL8_ENET1_EV0 1

#define IMX6ULL_DMA_EVENT_MUX_SEL7_ADC2 0
#define IMX6ULL_DMA_EVENT_MUX_SEL7_TSC 1

#define IMX6ULL_DMA_EVENT_MUX_SEL6_GPT2 0
#define IMX6ULL_DMA_EVENT_MUX_SEL6_LCDIF 1

#define IMX6ULL_DMA_EVENT_MUX_SEL5_EPIT1 0
#define IMX6ULL_DMA_EVENT_MUX_SEL5_CSI 1

#define IMX6ULL_DMA_EVENT_MUX_SEL4_ECSPI4_TX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL4_I2C4 1

#define IMX6ULL_DMA_EVENT_MUX_SEL3_ECSPI4_RX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL3_I2C3 1

#define IMX6ULL_DMA_EVENT_MUX_SEL2_ECSPI3_TX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL2_I2C2 1

#define IMX6ULL_DMA_EVENT_MUX_SEL1_ECSPI3_RX 0
#define IMX6ULL_DMA_EVENT_MUX_SEL1_I2C1 1

#define IMX6ULL_DMA_EVENT_MUX_SEL0_EPIT2 0
#define IMX6ULL_DMA_EVENT_MUX_SEL0_PXP 1


//
// -------------------------------------------------------------------- Globals
//


//
// IMX6ULL DMA request -> Channel configuration
//

#define NOT_SUPPORTED_REQ(ReqId) \
{\
  SDMA_UNSUPPORTED_REQUEST_ID, /* Not supported */\
  ReqId\
}

SDMA_CHANNEL_CONFIG Imx6UllDmaReqToChannelConfig[] = {

  // (0) SDMA_REQ_VPU
  NOT_SUPPORTED_REQ(SDMA_REQ_VPU),

  // (1) SDMA_REQ_IPU2
  NOT_SUPPORTED_REQ(SDMA_REQ_IPU2),

  // (2) SDMA_REQ_IPU1
  NOT_SUPPORTED_REQ(SDMA_REQ_IPU1),

  // (3) SDMA_REQ_HDMI_AUDIO
  NOT_SUPPORTED_REQ(SDMA_REQ_HDMI_AUDIO),

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
        IMX6ULL_SDMA_EVENT_ECSPI1_RX,
        DMA_EVENT_EXCLUSIVE
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
        IMX6ULL_SDMA_EVENT_ECSPI1_TX,
        DMA_EVENT_EXCLUSIVE
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
        IMX6ULL_SDMA_EVENT_ECSPI2_RX,
        DMA_EVENT_EXCLUSIVE
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
        IMX6ULL_SDMA_EVENT_ECSPI2_TX,
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
      {
        IMX6ULL_SDMA_EVENT_ECSPI3_RX_I2C1,
        DMA_EVENT_MUX_SEL1,
        IMX6ULL_DMA_EVENT_MUX_SEL1_ECSPI3_RX
      },
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
        IMX6ULL_SDMA_EVENT_ECSPI3_TX_I2C2,
        DMA_EVENT_MUX_SEL2,
        IMX6ULL_DMA_EVENT_MUX_SEL2_ECSPI3_TX
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
        IMX6ULL_SDMA_EVENT_ECSPI4_RX_I2C3,
        DMA_EVENT_MUX_SEL3,
        IMX6ULL_DMA_EVENT_MUX_SEL3_ECSPI4_RX
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
        IMX6ULL_SDMA_EVENT_ECSPI4_TX_I2C4,
        DMA_EVENT_MUX_SEL4,
        IMX6ULL_DMA_EVENT_MUX_SEL4_ECSPI4_TX
      }
    },
  },

  // (12) SDMA_REQ_ECSPI5_RX
  NOT_SUPPORTED_REQ(SDMA_REQ_ECSPI5_RX),

  // (13) SDMA_REQ_ECSPI5_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_ECSPI5_TX),

  // (14) SDMA_REQ_I2C1_RX -> Event 7
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
        IMX6ULL_SDMA_EVENT_ECSPI3_RX_I2C1,
        DMA_EVENT_MUX_SEL1,
        IMX6ULL_DMA_EVENT_MUX_SEL1_I2C1
      },
    },
  },

  // (15) SDMA_REQ_I2C1_TX -> Event 7
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
        IMX6ULL_SDMA_EVENT_ECSPI3_RX_I2C1,
        DMA_EVENT_MUX_SEL1,
        IMX6ULL_DMA_EVENT_MUX_SEL1_I2C1
      },
    },
  },

  // (16) SDMA_REQ_I2C2_RX -> Event 8
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
        IMX6ULL_SDMA_EVENT_ECSPI3_TX_I2C2,
        DMA_EVENT_MUX_SEL2,
        IMX6ULL_DMA_EVENT_MUX_SEL2_I2C2
      }
    },
  },

  // (17) SDMA_REQ_I2C2_TX -> Event 8
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
        IMX6ULL_SDMA_EVENT_ECSPI3_TX_I2C2,
        DMA_EVENT_MUX_SEL2,
        IMX6ULL_DMA_EVENT_MUX_SEL2_I2C2
      }
    },
  },

  // (18) SDMA_REQ_I2C3_RX -> Event 9
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
        IMX6ULL_SDMA_EVENT_ECSPI4_RX_I2C3,
        DMA_EVENT_MUX_SEL3,
        IMX6ULL_DMA_EVENT_MUX_SEL3_I2C3
      }
    },
  },

  // (19) SDMA_REQ_I2C3_TX -> Event 9
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
        IMX6ULL_SDMA_EVENT_ECSPI4_RX_I2C3,
        DMA_EVENT_MUX_SEL3,
        IMX6ULL_DMA_EVENT_MUX_SEL3_I2C3
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
        IMX6ULL_SDMA_EVENT_UART1_RX,
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
        IMX6ULL_SDMA_EVENT_UART1_TX,
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
        IMX6ULL_SDMA_EVENT_UART2_RX,
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
        IMX6ULL_SDMA_EVENT_UART2_TX,
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
        IMX6ULL_SDMA_EVENT_UART3_RX,
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
        IMX6ULL_SDMA_EVENT_UART3_TX,
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
        IMX6ULL_SDMA_EVENT_UART4_RX,
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
        IMX6ULL_SDMA_EVENT_UART4_TX,
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
        IMX6ULL_SDMA_EVENT_UART5_RX,
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
        IMX6ULL_SDMA_EVENT_UART5_TX_EPDC,
        DMA_EVENT_MUX_SEL13,
        IMX6ULL_DMA_EVENT_MUX_SEL13_UART5_TX
      },
    },
  },

  // (30) SDMA_REQ_SPDIF_RX -> Event 41
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
        IMX6ULL_SDMA_EVENT_SPDIF_RX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (31) SDMA_REQ_SPDIF_TX -> Event 42
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
        IMX6ULL_SDMA_EVENT_SPDIF_TX,
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
        IMX6ULL_SDMA_EVENT_EPIT1_CSI,
        DMA_EVENT_MUX_SEL5,
        IMX6ULL_DMA_EVENT_MUX_SEL5_EPIT1
      },
    },
  },

  // (33) SDMA_REQ_EPIT2
  NOT_SUPPORTED_REQ(SDMA_REQ_EPIT2),

  // (34) SDMA_REQ_GPT1 -> Event 23
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
        IMX6ULL_SDMA_EVENT_GPT1,
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
        IMX6ULL_SDMA_EVENT_ASRC_RXA,
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
        IMX6ULL_SDMA_EVENT_ASRC_RXB,
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
        IMX6ULL_SDMA_EVENT_ASRC_RXC,
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
        IMX6ULL_SDMA_EVENT_ASRC_TXA,
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
        IMX6ULL_SDMA_EVENT_ASRC_TXB,
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
        IMX6ULL_SDMA_EVENT_ASRC_TXC,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (41) SDMA_REQ_ESAI_RX -> Event 0
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
        IMX6ULL_SDMA_EVENT_UART6_RX_ESAI_RX,
        DMA_EVENT_MUX_SEL14,
        IMX6ULL_DMA_EVENT_MUX_SEL14_ESAI_RX
      },
    },
  },

  // (42) SDMA_REQ_ESAI_TX -> Event 47
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
        IMX6ULL_SDMA_EVENT_UART6_TX_ESAI_TX,
        DMA_EVENT_MUX_SEL15,
        IMX6ULL_DMA_EVENT_MUX_SEL15_ESAI_TX
      },
    },
  },

  // (43) SDMA_REQ_ASRC_TXA_2_ESAI_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_ASRC_TXA_2_ESAI_TX),

  // (44) SDMA_REQ_ASRC_TXB_2_ESAI_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_ASRC_TXB_2_ESAI_TX),

  // (45) SDMA_REQ_ASRC_TXC_2_ESAI_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_ASRC_TXC_2_ESAI_TX),

  // (46) SDMA_REQ_SSI1_RX1
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI1_RX1),

  // (47) SDMA_REQ_SSI1_TX1
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI1_TX1),

  // (48) SDMA_REQ_SSI1_RX0
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI1_RX0),

  // (49) SDMA_REQ_SSI1_TX0
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI1_TX0),

  // (50) SDMA_REQ_SSI2_RX1
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI2_RX1),

  // (51) SDMA_REQ_SSI2_TX1
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI2_TX1),

  // (52) SDMA_REQ_SSI2_RX0
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI2_RX0),

  // (53) SDMA_REQ_SSI2_TX0
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI2_TX0),

  // (54) SDMA_REQ_SSI3_RX1
   NOT_SUPPORTED_REQ(SDMA_REQ_SSI3_RX1),

  // (55) SDMA_REQ_SSI3_TX1
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI3_TX1),

  // (56) SDMA_REQ_SSI3_RX0
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI3_RX0),

  // (57) SDMA_REQ_SSI3_TX0
  NOT_SUPPORTED_REQ(SDMA_REQ_SSI3_TX0),

  // (58) SDMA_REQ_EXT1
  NOT_SUPPORTED_REQ(SDMA_REQ_EXT1),

  // (59) SDMA_REQ_EXT2
  NOT_SUPPORTED_REQ(SDMA_REQ_EXT2),

  // (60) SDMA_REQ_UART6_RX -> Event 0
  {
    imx6_mcu_2_app_ADDR,            // Script address
    SDMA_REQ_UART6_RX,              // DMA request ID
    DMA_WIDTH_8BIT,                 // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_UART6_RX_ESAI_RX,
        DMA_EVENT_MUX_SEL14,
        IMX6ULL_DMA_EVENT_MUX_SEL14_UART6_RX
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
        IMX6ULL_SDMA_EVENT_UART6_TX_ESAI_TX,
        DMA_EVENT_MUX_SEL15,
        IMX6ULL_DMA_EVENT_MUX_SEL15_UART6_TX
      },
    },
  },

  // (62) SDMA_REQ_ADC1 -> Event 1
  {
     imx6_app_2_mcu_ADDR,            // Script address
     SDMA_REQ_ADC1,                  // DMA request ID
     DMA_WIDTH_32BIT,                // Transfer width
     0,                              // Owner channel
     SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
     0,                              // Peripheral 2 address
     IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
     1,                              // Number of DMA events to follow
     {
       {
         IMX6ULL_SDMA_EVENT_ADC1,
         DMA_EVENT_EXCLUSIVE
       },
     },
  },

  // (63) SDMA_REQ_ADC2 -> Event 13
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_ADC2,                  // DMA request ID
    DMA_WIDTH_32BIT,                // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_ADC2_TSC,
        DMA_EVENT_MUX_SEL7,
        IMX6ULL_DMA_EVENT_MUX_SEL7_ADC2
      },
    },
  },

  // (64) SDMA_REQ_I2C4_RX -> Event 10
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
        IMX6ULL_SDMA_EVENT_ECSPI4_TX_I2C4,
        DMA_EVENT_MUX_SEL4,
        IMX6ULL_DMA_EVENT_MUX_SEL4_I2C4
      },
    },
  },

  // (65) SDMA_REQ_I2C4_TX -> Event 10
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_I2C4_TX,               // DMA request ID
    DMA_WIDTH_8BIT,                 // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_ECSPI4_TX_I2C4,
        DMA_EVENT_MUX_SEL4,
        IMX6ULL_DMA_EVENT_MUX_SEL4_I2C4
      },
    },
  },

  // (66) SDMA_REQ_CSI1 -> Event 16
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
        IMX6ULL_SDMA_EVENT_EPIT1_CSI,
        DMA_EVENT_MUX_SEL5,
        IMX6ULL_DMA_EVENT_MUX_SEL5_CSI
      },
    },
  },

  // (67) SDMA_REQ_CSI2
  NOT_SUPPORTED_REQ(SDMA_REQ_CSI2),

  // (68) SDMA_REQ_PXP -> Event 2
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
        IMX6ULL_SDMA_EVENT_EPIT2_PXP,
        DMA_EVENT_MUX_SEL0,
        IMX6ULL_DMA_EVENT_MUX_SEL0_PXP
      },
    },
  },

  // (69) SDMA_REQ_LCDIF1 -> Event 24
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
        IMX6ULL_SDMA_EVENT_GPT2_LCDIF,
        DMA_EVENT_MUX_SEL6,
        IMX6ULL_DMA_EVENT_MUX_SEL6_LCDIF
      },
    },
  },

  // (70) SDMA_REQ_LCDIF2
  NOT_SUPPORTED_REQ(SDMA_REQ_LCDIF2),

  // (71) SDMA_REQ_QSPI1_RX -> Event 11
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
        IMX6ULL_SDMA_EVENT_QSPI_RX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (72) SDMA_REQ_QSPI1_TX -> Event 12
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_QSPI1_TX,              // DMA request ID
    DMA_WIDTH_32BIT,                // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_QSPI_TX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (73) SDMA_REQ_QSPI2_RX
  NOT_SUPPORTED_REQ(SDMA_REQ_QSPI2_RX),

  // (74) SDMA_REQ_QSPI2_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_QSPI2_TX),

  // (75) SDMA_REQ_SAI1_TX -> Event 36
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_SAI1_TX,               // DMA request ID
    DMA_WIDTH_32BIT,                // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_SAI1_TX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (76) SDMA_REQ_SAI1_RX -> Event 35
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
        IMX6ULL_SDMA_EVENT_SAI1_RX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (77) SDMA_REQ_SAI2_TX -> Event 38
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_SAI2_TX,               // DMA request ID
    DMA_WIDTH_32BIT,                // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_SAI2_TX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (78) SDMA_REQ_SAI2_RX -> Event 37
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_SAI2_RX,                  // DMA request ID
    DMA_WIDTH_32BIT,                 // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_SAI2_RX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },


  // (79) SDMA_REQ_SAI3_TX -> Event 40
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_SAI3_TX,               // DMA request ID
    DMA_WIDTH_32BIT,                // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_TX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_SAI3_TX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (80) SDMA_REQ_SAI3_RX -> Event 39
  {
    imx6_app_2_mcu_ADDR,            // Script address
    SDMA_REQ_SAI3_RX,               // DMA request ID
    DMA_WIDTH_32BIT,                // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_DEFAULT_RX,        // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_SAI3_RX,
        DMA_EVENT_EXCLUSIVE
      },
    },
  },

  // (81) SDMA_REQ_SAI4_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_SAI4_TX),

  // (82) SDMA_REQ_SAI4_RX
  NOT_SUPPORTED_REQ(SDMA_REQ_SAI4_RX),

  // (83) SDMA_REQ_SAI5_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_SAI5_TX),

  // (84) SDMA_REQ_SAI5_RX
  NOT_SUPPORTED_REQ(SDMA_REQ_SAI5_RX),

  // (85) SDMA_REQ_SAI6_TX
  NOT_SUPPORTED_REQ(SDMA_REQ_SAI6_TX),

  // (86) SDMA_REQ_SAI6_RX
  NOT_SUPPORTED_REQ(SDMA_REQ_SAI6_RX),

  // (87) SDMA_REQ_UART7_RX -> Event 43
  {
    imx6_mcu_2_app_ADDR,            // Script address
    SDMA_REQ_UART7_RX,              // DMA request ID
    DMA_WIDTH_8BIT,                 // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_UART_RX,           // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_UART7_RX_ENET1_EV0,
        DMA_EVENT_MUX_SEL8,
        IMX6ULL_DMA_EVENT_MUX_SEL8_UART7_RX
      },
    },
  },

  // (88) SDMA_REQ_UART7_TX -> Event 44
  {
    imx6_mcu_2_app_ADDR,            // Script address
    SDMA_REQ_UART7_TX,              // DMA request ID
    DMA_WIDTH_8BIT,                 // Transfer width
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    IMX_WL_SCALE_UART_TX,           // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_UART7_TX_ENET1_EV1,
        DMA_EVENT_MUX_SEL9,
        IMX6ULL_DMA_EVENT_MUX_SEL9_UART7_TX
      },
    },
  },

  // (89) SDMA_REQ_GPT2 -> Event 24
  {
    imx6_ap_2_ap_ADDR,              // Script address
    SDMA_REQ_GPT2,                  // DMA request ID
    DMA_WIDTH_32BIT,                // Transfer width (24 bit)
    0,                              // Owner channel
    SDMA_DEVICE_FLAG_FIXED_ADDRESS, // Flags
    0,                              // Peripheral 2 address
    1,                              // Watermark level scale (percent)
    1,                              // Number of DMA events to follow
    {
      {
        IMX6ULL_SDMA_EVENT_GPT2_LCDIF,
        DMA_EVENT_MUX_SEL6,
        IMX6ULL_DMA_EVENT_MUX_SEL6_GPT2
      },
    },
  },

  // (90) SDMA_REQ_GPT3
  NOT_SUPPORTED_REQ(SDMA_REQ_GPT3),

  // (91) SDMA_REQ_GPT4
  NOT_SUPPORTED_REQ(SDMA_REQ_GPT4),

  // (92) SDMA_REQ_GPT5
  NOT_SUPPORTED_REQ(SDMA_REQ_GPT5),

  // (93) SDMA_REQ_GPT6
  NOT_SUPPORTED_REQ(SDMA_REQ_GPT6),

  // (94) SDMA_REQ_ENET1_EV0 -> Event 43
  NOT_SUPPORTED_REQ(SDMA_REQ_ENET1_EV0),

  // (95) SDMA_REQ_ENET1_EV1 -> Event 44
  NOT_SUPPORTED_REQ(SDMA_REQ_ENET1_EV1),

  // (96) SDMA_REQ_ENET2_EV0 -> Event 45
  NOT_SUPPORTED_REQ(SDMA_REQ_ENET2_EV0),

  // (97) SDMA_REQ_ENET2_EV1 -> Event 46
  NOT_SUPPORTED_REQ(SDMA_REQ_ENET2_EV1),

  // (98) SDMA_REQ_EXT3
  NOT_SUPPORTED_REQ(SDMA_REQ_EXT3),

  // (99) SDMA_REQ_EXT4
  NOT_SUPPORTED_REQ(SDMA_REQ_EXT4),

};
ULONG Imx6UllDmaReqMax = ARRAYSIZE(Imx6UllDmaReqToChannelConfig) - 1;


#endif // ! _IMX6_SDMA_ULL_HW_H

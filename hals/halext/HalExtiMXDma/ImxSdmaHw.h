/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    ImxSdmaHw.h

Abstract:

    This file includes the types/enums definitions associated with the
    IMX SDMA controller hardware.

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _IMX_SDMA_HW_H
#define _IMX_SDMA_HW_H


//
// SDMA channels are virtual channels, not associated with a HW resource.
// Since SDMA is a RISC controller, a DMA channel is merely a thread, means
// SDMA can run up to 32 concurrent threads.
//

#define SDMA_NUM_CHANNELS 32UL


//
// Number of SDMA requests (events)
//

#define SDMA_NUM_EVENTS 48UL

//
// IMX peripherals watermark level scale, applied to 
// values configured by the client driver through 
// SdmaConfigureChannel(SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL).
//

#define IMX_WL_SCALE_DEFAULT_RX 100UL
#define IMX_WL_SCALE_DEFAULT_TX 100UL
#define IMX_WL_SCALE_UART_RX 90UL
#define IMX_WL_SCALE_UART_TX 100UL

//
// SDMA ROM/RAM sizes
// SDMA ROM/RAM can be accessed as intersection memory or data memory.
// Instruction memory access (PM mode) uses 16bit WORDS address space.
// Data memory access (DM mode) uses 32bit DWORDS address space.
//

#define SDMA_ROM_SIZE               (4*1024)
#define SDMA_RAM_SIZE               (8*1024)
#define SDRAM_CHANNEL_CONTEXT_SIZE  128UL
#define SDRAM_CONTEXT_SIZE          (SDMA_NUM_CHANNELS *\
                                     SDRAM_CHANNEL_CONTEXT_SIZE)
#define SDMA_MAX_RAM_CODE_SIZE      (SDMA_RAM_SIZE - SDRAM_CONTEXT_SIZE)


//
// SDMA instruction memory map
// Instruction memory is referenced as 16bit WORDS address.
//

#define SDMA_PM_ROM_START       0x0000UL
#define SDMA_PM_RAM_START       0x1000UL
#define SDMA_PM_CONTEXT_START   0x1000UL
#define SDMA_PM_CODE_START      0x1800UL


//
// SDMA data memory map.
// Data memory is referenced as 32bit DWORDS address, thus
// address is the PM mode address divided by 2.
//

#define SDMA_DM_ROM_START       (SDMA_PM_ROM_START >> 1)
#define SDMA_DM_RAM_START       (SDMA_PM_RAM_START >> 1)
#define SDMA_DM_CONTEXT_START   (SDMA_PM_CONTEXT_START >> 1)
#define SDMA_DM_CODE_START      (SDMA_PM_CODE_START >> 1)


//
// Channel 0 boot script commands
//

typedef enum _SDMA_CH0_COMMANDS {
    CMD_C0_SET_DM = 0x01,
    CMD_C0_GET_DM = 0x02,
    CMD_C0_SET_PM = 0x04,
    CMD_C0_GET_PM = 0x06,
    CMD_C0_SETCTX = 0x07,
    CMD_C0_GETCTX = 0x03,
} SDMA_CH0_COMMANDS;

#define CH0_CMD_SETCTX(_Channel) (ULONG)((_Channel) << 3 | CMD_C0_SETCTX)
#define CH0_CMD_GETCTX(_Channel) (ULONG)((_Channel) << 3 | CMD_C0_GETCTX)


//
// Channel priority
//

typedef enum _SDMA_CHANNEL_PRIORITY {
    CHN_PRI_DISABLED = 0,
    CHN_PRI_LOWEST = 1,
    CHN_PRI_NORMAL = 2,
    CHN_PRI_HIGHEST = 7
} SDMA_CHANNEL_PRIORITY;


//
// DMA transfer width codes
//

typedef enum _SDMA_TRANSFER_WIDTH {
    DMA_WIDTH_NA = -1 ,
    DMA_WIDTH_32BIT = 0,
    DMA_WIDTH_8BIT = 1,
    DMA_WIDTH_16BIT = 2,
    DMA_WIDTH_24BIT = 3
} SDMA_TRANSFER_WIDTH;


#pragma pack(push, SdmaHw, 1)

//
// Channel Control Block
//

typedef struct _SDMA_CCB
{
    //
    // Current buffer descriptor base address
    // (physical address)
    //

    ULONG CurrentBdAddress;

    //
    // Base buffer descriptor base address
    // (physical address)
    //

    ULONG BasedBdAddress;

    //
    // Channel Descriptor (not used)
    //

    ULONG ChannelDescriptor;

    //
    // Channel Status (not used)
    //

    ULONG ChannelStatus;

} SDMA_CCB;


//
// SDMA buffer descriptor format
//

typedef union _SDMA_BD_ATTRIBUTES {

    ULONG AsUlong;

    struct {
        int Count    : 16; // Bit# 15-0

        int D        : 1;  // Bit# 16 (Done)
        int W        : 1;  // Bit# 17 (Wrap)
        int C        : 1;  // Bit# 18 (Continuous)
        int I        : 1;  // Bit# 19 (Interrupt)
        int R        : 1;  // Bit# 20 (Error)
        int L        : 1;  // Bit# 21 (Last)
        int reserved : 2;  // Bit# 22-23

        int Command  : 8;  // Bit# 34-31
    };

} SDMA_BD_ATTRIBUTES;

typedef struct _SDMA_BD
{
    //
    // Buffer command, size and attributes
    //

    SDMA_BD_ATTRIBUTES Attributes;

    //
    // Data buffer logical address.
    //

    ULONG Address;

    //
    // Extended Buffer logical address
    // Additional buffer address required by some scripts,
    // like memory to memory.
    //

    ULONG ExtendedAddress;

} SDMA_BD;


//
// SDMA per channel context format.
// 32 DWORDS per channel starting at SDMA_DM/PM_CONTEXT_START
//

typedef union _SDMA_CHANNEL_STATE_REGS {

    ULONGLONG AsQword;
    ULONG AsUlong[2];

    struct {
        int PC        : 14; // Bit# 13-0 (Program Counter)
        int reserved1 : 1;  // Bit# 14
        int T         : 1;  // Bit# 15 (Test)
        int RPC       : 14; // Bit# 29-16 (Return Program Counter)
        int reserved2 : 1;  // Bit# 30
        int SF        : 1;  // Bit# 31 (Source Fault)

        int SPC       : 14; // Bit# 13-0 (Loop Start Program Counter)
        int reserved3 : 1;  // Bit# 14
        int DF        : 1;  // Bit# 15 (Destination Fault)
        int EPC       : 14; // Bit# 29-16 (Loop End Program Counter)
        int LM        : 2;  // Bit# 31-30 (Loop Mode)
    };

} SDMA_CHANNEL_STATE_REGS;

typedef struct _SDMA_CHANNEL_CONTEXT {

    volatile SDMA_CHANNEL_STATE_REGS StateRegs;

    //
    // Core general purpose registers
    //

    volatile ULONG GR[8];
    
    //
    // Functional units state registers
    //

    volatile ULONG MDA;
    volatile ULONG MSA;
    volatile ULONG MS;
    volatile ULONG MD;
    volatile ULONG PDA;
    volatile ULONG PSA;
    volatile ULONG PS;
    volatile ULONG PD;
    volatile ULONG CA;
    volatile ULONG CS;
    volatile ULONG DDA;
    volatile ULONG DSA;
    volatile ULONG DS;
    volatile ULONG DD;

    //
    // Scratch RAM
    //

    volatile ULONG Scratch[8];

} SDMA_CHANNEL_CONTEXT;


//
// Various SDMA registers mapping
//

typedef enum _SDMA_CONFIG_CSM {
    CSM_STATIC = 0,
    CSM_DYNAMIC_LOW_POWER = 1,
    CSM_DYNAMIC_NO_LOOP = 2,
    CSM_DYNAMIC = 3,
} SDMA_CONFIG_CSM;

typedef union _SDMA_CONFIG_REG {

    ULONG AsUlong;

    struct {
        int CSM         : 2;  // Bit# 1-0 (Context Switch Mode, 
                              //           refer to SDMA_CONFIG_CSM)
        int reserved1   : 2;  // Bit# 3-2
        int ACR         : 1;  // Bit# 4 (AHB/SDMA Core Clock Ratio)
        int reserved2   : 6;  // Bit# 10-5
        int RTDOBS      : 1;  // Bit# 11 (If real-time debug pins are used)
        int DSPDMA      : 1;  // Bit# 12 (Should be 0)
        int reserved3   : 19; // Bit# 31-13 (Loop End Program Counter)
    };

} SDMA_CONFIG_REG;

typedef union _SDMA_CHN0ADDR_REG {

    ULONG AsUlong;

    struct {
        int CHN0ADDR : 14;  // Bit# 13-0 (Channel0 address), 
        int SMSZ     : 1;   // Bit# 14 (Scratch Memory Size)
        int reserved : 17;  // Bit# 31-15
    };

} SDMA_CHN0ADDR_REG;

typedef union _SDMA_RESET_REG {

    ULONG AsUlong;

    struct {
        int RESET       : 1;    // Bit# 0
        int RESCHED     : 1;    // Bit# 1
        int reserved    : 30;   // Bit# 2-31
    };

} SDMA_RESET_REG;

//
// SDMA registers layout
//

typedef struct _SDMA_REGS {

    ULONG MC0PTR;                      //00: AP (MCU) Channel 0 Pointer
    ULONG INTR;                        //04: Channel Interrupts
    ULONG STOP_STAT;                   //08: Channel Stop/Channel Status
    ULONG HSTART;                      //0c: Channel Start
    ULONG EVTOVR;                      //10: Channel Event Override
    ULONG DSPOVR;                      //14: Channel BP Override
    ULONG HOSTOVR;                     //18: Channel AP Override
    ULONG EVTPEND;                     //1C: Channel Event Pending
    ULONG reserved0;                   //20: 
    ULONG RESET;                       //24: Reset Register
    ULONG EVTERR;                      //28: DMA Request Error Register
    ULONG INTRMASK;                    //2C: Channel AP Interrupt Mask
    ULONG PSW;                         //30: Schedule Status
    ULONG EVTERRDBG;                   //34: DMA Request Error Register
    ULONG CONFIG;                      //38: Configuration Register
    ULONG SDMA_LOCK;                   //3C: SDMA LOCK for MX37, MX51
    ULONG ONCE_ENB;                    //40: OnCE Enable
    ULONG ONCE_DATA;                   //44: OnCE Data Register
    ULONG ONCE_INSTR;                  //48: OnCE Instruction Register
    ULONG ONCE_STAT;                   //4C: OnCE Status Register
    ULONG ONCE_CMD;                    //50: OnCE Command Register
    ULONG reserved1;                   //54:
    ULONG ILLINSTADDR;                 //58: Illegal Instruction Trap Address
    ULONG CHN0ADDR;                    //5C: Channel 0 Boot Address
    ULONG EVT_MIRROR;                  //60: DMA Requests 
    ULONG EVT_MIRROR2;                 //64: DMA Requests 2
    ULONG reserved2[2];                //68, 6C
    ULONG XTRIG_CONF1;                 //70: Cross-Trigger Events Register1
    ULONG XTRIG_CONF2;                 //74: Cross-Trigger Events Register2
    ULONG OTB;                         //78:
    ULONG PRF_CNTX[6];                 //7C..90:
    ULONG PRF_CFG;                     //94:
    ULONG reserved3[26];               //98..FC: reserved
    ULONG CHNPRI[SDMA_NUM_CHANNELS];   //100: 
    ULONG reserved4[32];
    ULONG CHNENBL[SDMA_NUM_EVENTS];    //200:
    ULONG reserved5[16];
} SDMA_REGS;

#pragma pack (pop, SdmaHw)

#endif // !_IMX_SDMA_HW_H

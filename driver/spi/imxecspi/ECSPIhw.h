// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    PL011hw.h
//
// Abstract:
//
//    This module contains common enum, types, and methods definitions
//    for accessing the IMX ECSPI controller hardware.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//

#ifndef _ECSPI_HW_H_
#define _ECSPI_HW_H_

WDF_EXTERN_C_START


//
// ECSPI controller registers layout.
//
#include <pshpack1.h>

//
// Control Register (ECSPIx_CONREG)
//
enum ECSPI_CHANNEL : ULONG {
    CS0 = 0x0,
    CS1 = 0x1,
    CS2 = 0x2,
    CS3 = 0x3,

    COUNT = 4
};
enum ECSPI_DRCTL {
    SPI_RDY_OFF = 0x0,
    SPI_RDY_FALLING_EDGE = 0x1,
    SPI_RDY_LOW_LEVEL = 0x2
};
enum ECSPI_CHANNEL_MODE : ULONG {
    SLAVE = 0x0,
    MASTER = 0x1,

    ALL_MASTERS = 0xF,
};
enum ECSPI_START_MODE {
    XCH = 0x0,
    IMMEDIATE = 0x1
};
union ECSPI_CONREG {
    ULONG AsUlong;
    struct {
        ULONG EN            : 1; // Bit# 0
        ULONG HT            : 1; // Bit# 1
        ULONG XCH           : 1; // Bit# 2
        ULONG SMC           : 1; // Bit# 3 (ECSPI_START_MODE)
        ULONG CHANNEL_MODE  : 4; // Bit# 4-7 (ECSPI_CHANNEL_MODE)
        ULONG POST_DIVIDER  : 4; // Bit# 8-11
        ULONG PRE_DIVIDER   : 4; // Bit# 12-15
        ULONG DRCTL         : 2; // Bit# 16-17 (ECSPI_DRCTL)
        ULONG CHANNEL_SELECT: 2; // Bit# 18-19 (ECSPI_CHANNEL)
        ULONG BURST_LENGTH  : 12;// Bit# 20-31
    };
};

//
// Configuration Register (ECSPIx_CONFIGREG)
//
enum ECSPI_INACTIVE_SCLK_CTL : ULONG {
    SCLK_LOW = 0x0,
    SCLK_HIGH = 0x1
};
enum ECSPI_INACTIVE_DATA_CTL : ULONG {
    DATA_HIGH = 0x0,
    DATA_LOW = 0x1
};
enum ECSPI_SS_POL : ULONG {
    SS_ACTIVE_LOW = 0x0,
    SS_ACTIVE_HIGH = 0x1
};
enum ECSPI_SS_CTL : ULONG {
    SS_SINGLE_BURST = 0x0,
    SS_MULTI_BURST = 0x1
};
enum ECSPI_SCLK_POL : ULONG {
    SCLK_POL0  = 0x0, // Active high
    SCLK_POL1 = 0x1 // Active low
};
enum ECSPI_SCLK_PHA : ULONG {
    SCLK_PHASE0 = 0x0,
    SCLK_PHASE1 = 0x1
};
union ECSPI_CONFIGREG {
    ULONG AsUlong;
    struct {
        ULONG SCLK_PHA  : 4; // Bit# 0-3 (ECSPI_SCLK_PHA)
        ULONG SCLK_POL  : 4; // Bit# 4-7 (ECSPI_SCLK_POL)
        ULONG SS_CTL    : 4; // Bit# 8-11 (ECSPI_SS_CTL)
        ULONG SS_POL    : 4; // Bit# 12-15 (ECSPI_SS_POL)
        ULONG DATA_CTL  : 4; // Bit# 16-19 (ECSPI_INACTIVE_DATA_CTL)
        ULONG SCLK_CTL  : 4; // Bit# 20-23 (ECSPI_INACTIVE_SCLK_CTL)
        ULONG HT_LENGTH : 5; // Bit# 24-28
        ULONG Reserverd : 3; // Bit# 29-31
    };
};

//
// Interrupt Control Register (ECSPIx_INTREG)
//
enum ECSPI_INTERRUPT_TYPE : ULONG {
    TX_EMPTY        = 0x01,
    TX_THRESHOLD    = 0x02,
    TX_FULL         = 0x04,
    RX_READY        = 0x08,
    RX_THRESHOLD    = 0x10,
    RX_FULL         = 0x20,
    RX_OVERFLOW     = 0x40,
    XFER_DONE       = 0x80,

    ALL             = 0xFF
};
union ECSPI_INTREG {
    ULONG AsUlong;
    struct {
        ULONG TEEN      : 1; // Bit# 0
        ULONG TDREN     : 1; // Bit# 1
        ULONG TFEN      : 1; // Bit# 2
        ULONG RREN      : 1; // Bit# 3
        ULONG RDREN     : 1; // Bit# 4
        ULONG RFEN      : 1; // Bit# 5
        ULONG ROEN      : 1; // Bit# 6
        ULONG TCEN      : 1; // Bit# 7
        ULONG Reserverd : 24;// Bit# 8-31
    };
};

//
// DMA Control Register (ECSPIx_DMAREG)
//
union ECSPI_DMAREG {
    ULONG AsUlong;
    struct {
        ULONG TX_THRESHOLD  : 6; // Bit# 0-5
        ULONG Reserverd1    : 1; // Bit# 6
        ULONG TEDEN         : 1; // Bit# 7
        ULONG Reserverd     : 8; // Bit# 8-15
        ULONG RX_THRESHOLD  : 6; // Bit# 16-21
        ULONG Reserverd2    : 1; // Bit# 22
        ULONG RXDEN         : 1; // Bit# 23
        ULONG RX_DMA_LENGTH : 6; // Bit# 24-29
        ULONG Reserverd3    : 1; // Bit# 30
        ULONG RXTDEN        : 1; // Bit# 31
    };
};

//
// Status Register (ECSPIx_STATREG)
//
enum ECSPI_INTERRUPT_GROUP : ULONG {
    ECSPI_TX_INTERRUPTS = 0x00000087,
    ECSPI_RX_INTERRUPTS = 0x00000078,
    ECSPI_ACKABLE_INTERRUPTS = 0x000000C0,
    ECSPI_ALL_INTERRUPTS = 0x000000FF
};
union ECSPI_STATREG {
    ULONG AsUlong;
    struct {
        ULONG TE        : 1; // Bit# 0
        ULONG TDR       : 1; // Bit# 1
        ULONG TF        : 1; // Bit# 2
        ULONG RR        : 1; // Bit# 3
        ULONG RDR       : 1; // Bit# 4
        ULONG RF        : 1; // Bit# 5
        ULONG RO        : 1; // Bit# 6
        ULONG TC        : 1; // Bit# 7
        ULONG Reserverd : 24;// Bit# 8-31
    };
};

//
// Sample Period Control Register (ECSPIx_PERIODREG)
//
union ECSPI_PERIODREG {
    ULONG AsUlong;
    struct {
        ULONG SAMPLE_PERIOD : 15;// Bit# 0-14
        ULONG CSRC          : 1; // Bit# 15
        ULONG CSD_CTL       : 6; // Bit# 16-21
        ULONG Reserverd		: 10;// Bit# 22-31
    };
};

//
// Test Control Register (ECSPIx_TESTREG)
//
union ECSPI_TESTREG {
    ULONG AsUlong;
    struct {
        ULONG TXCNT         : 7; // Bit# 0-6
        ULONG Reserverd1    : 1; // Bit# 7
        ULONG RXCNT         : 7; // Bit# 8-14
        ULONG Reserverd2    : 13;// Bit# 15-27
        ULONG Reserverd3    : 3; // Bit# 28-30
        ULONG LBC           : 1; // Bit# 31
    };
};

//
// The ECSPI register file 
//
struct ECSPI_REGISTERS {
    ULONG   RXDATA;     // 0x0000
    ULONG   TXDATA;     // 0x0004
    ULONG   CONREG;     // 0x0008
    ULONG   CONFIGREG;  // 0x000C
    ULONG   INTREG;     // 0x0010
    ULONG   DMAREG;     // 0x0014
    ULONG   STATREG;    // 0x0018
    ULONG   PERIODREG;  // 0x001C
    ULONG   TESTREG;    // 0x0020
    ULONG   Reserved[7];// 0x0024-0x003C
    ULONG   MSGDATA;    // 0x0040
};
#include <poppack.h> // pshpack1

//
// The ECSPI configuration parameters
//
enum : ULONG {
    ECSPI_REGISTERS_ADDRESS_SPACE_SIZE = 0x4000,
};

//
// ECSPI properties 
//
enum : ULONG {
    ECSPI_DEFAULT_CLOCK_FREQ = 60000000, // 60 MHz
    ECSPI_PRE_DIVIDER_MAX = 16,
    ECSPI_POST_DIVIDER_MAX = 15,
    ECSPI_MAX_DIVIDER = (16 << 15),
    ECSPI_FIFO_DEPTH = 64,
    ECSPI_MAX_DATA_BIT_LENGTH = 32, // Due to API assumption
    ECSPI_MAX_BURST_LENGTH_BYTES = ((1 << 12) / 8)
};

enum : ULONG {
    MAX_SPEED_ERROR_PERCENT = 10
};

//
// Routine Description:
//
//  ECSPI_CH_ATTR set an attribute for a given channel
//  is full.
//
// Arguments:
//
//  ChannelId - Channel Id
//
//  Attribute - The Attribute (binary value) to set to channel.
//
// Return Value:
// 
//  The HW expected formatted value for the given channel with the
//  given attribute.
//
__forceinline
ULONG
ECSPI_CH_ATTR (
    _In_ ECSPI_CHANNEL ChannelId,
    _In_range_(0,1) ULONG Attribute
    )
{
    NT_ASSERT(ULONG(ChannelId) <= ECSPI_CHANNEL::COUNT);
    NT_ASSERT(Attribute <= 1);

    return Attribute << ULONG(ChannelId);
}

//
// Routine Description:
//
//  ECSPIHwIsTxFifoFull returns TRUE if TX FIFO is full.
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  TRUE if TX FIFO is full, otherwise FALSE.
//
__forceinline
BOOLEAN
ECSPIHwIsTxFifoFull (
    _In_ volatile ECSPI_REGISTERS* ECSPIRegsPtr
    )
{
    ECSPI_STATREG statReg = { 
        READ_REGISTER_NOFENCE_ULONG(&ECSPIRegsPtr->STATREG) 
        };
    return statReg.TF != 0;
}

//
// Routine Description:
//
//  ECSPIHwIsTxFifoEmpty returns TRUE if TX FIFO is empty.
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  TRUE if TX FIFO is empty, otherwise FALSE.
//
__forceinline
BOOLEAN
ECSPIHwIsTxFifoEmpty (
    _In_ volatile ECSPI_REGISTERS* ECSPIRegsPtr
    )
{
    ECSPI_STATREG statReg = {
        READ_REGISTER_NOFENCE_ULONG(&ECSPIRegsPtr->STATREG)
    };
    return statReg.TE != 0;
}

//
// Routine Description:
//
//  ECSPIHwIsRxFifoEmpty returns TRUE if RX FIFO is empty.
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  TRUE if RX FIFO is empty, otherwise FALSE.
//
__forceinline
BOOLEAN
ECSPIHwIsRxFifoEmpty (
    _In_ volatile ECSPI_REGISTERS* ECSPIRegsPtr
    )
{
    ECSPI_STATREG statReg = {
        READ_REGISTER_NOFENCE_ULONG(&ECSPIRegsPtr->STATREG)
        };
    return statReg.RR == 0;
}

//
// Routine Description:
//
//  ECSPIHwQueryRxFifoCount returns the current number of words in RX FIFO.
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  Current RX FIFO count.
//
__forceinline
ULONG
ECSPIHwQueryRxFifoCount (
    _In_ volatile ECSPI_REGISTERS* ECSPIRegsPtr
    )
{
    ECSPI_TESTREG testReg = {
        READ_REGISTER_NOFENCE_ULONG(&ECSPIRegsPtr->TESTREG)
    };
    return testReg.RXCNT;
}

//
// Routine Description:
//
//  ECSPIHwQueryTransferComplete returns the status register transfer complete flag
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  Current TC value.
//
__forceinline
ULONG
ECSPIHwQueryTransferComplete (
    _In_ volatile ECSPI_REGISTERS* ECSPIRegsPtr
    )
{
    ECSPI_STATREG statReg = {
        READ_REGISTER_NOFENCE_ULONG(&ECSPIRegsPtr->STATREG)
    };
    return statReg.TC;
}

//
// Routine Description:
//
//  ECSPIHwQueryXCH returns the control register SPI Exchange bit
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  Current XCH value.
//
__forceinline
ULONG
ECSPIHwQueryXCH (
    _In_ volatile ECSPI_REGISTERS* ECSPIRegsPtr
    )
{
    ECSPI_CONREG conReg = {
        READ_REGISTER_NOFENCE_ULONG(&ECSPIRegsPtr->CONREG)
    };
    return conReg.XCH;
}

//
// Routine Description:
//
//  ECSPIHwQueryTxFifoSpace returns the current space in TX FIFO.
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  Current RX FIFO count.
//
__forceinline
ULONG
ECSPIHwQueryTxFifoSpace (
    _In_ volatile ECSPI_REGISTERS* ECSPIRegsPtr
    )
{
    ECSPI_TESTREG testReg = {
        READ_REGISTER_NOFENCE_ULONG(&ECSPIRegsPtr->TESTREG)
    };
    return ECSPI_FIFO_DEPTH - testReg.TXCNT;
}

NTSTATUS
ECSPIHwInitController (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr
    );

NTSTATUS
ECSPIHwSetTargetConfiguration (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr
    );

BOOLEAN
ECSPIHwIsSupportedDataBitLength (
    _In_ ULONG DataBitLength
    );

VOID
ECSPIHwSelectTarget (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr
    );

VOID
ECSPIHwUnselectTarget (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr
    );

VOID
ECSPIHwGetClockRange (
    _Out_ ULONG* MinClockHzPtr,
    _Out_ ULONG* MaxClockHzPtr
    );

ULONG
ECSPIHwInterruptControl (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ BOOLEAN IsEnable,
    _In_ ULONG InterruptMask
    );

VOID
ECSPIHwUpdateTransferConfiguration (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ ECSPI_SPB_TRANSFER* TransferPtr
    );

VOID
ECSPIHwConfigureTransfer (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ ECSPI_SPB_TRANSFER* TransferPtr,
    _In_ BOOLEAN IsInitialSetup,
    _Inout_ PULONG InterruptRegPtr
    );

BOOLEAN
ECSPIHwWriteTxFIFO (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ ECSPI_SPB_TRANSFER* TransferPtr
    );

BOOLEAN
ECSPIHwReadRxFIFO (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ ECSPI_SPB_TRANSFER* TransferPtr
    );

BOOLEAN
ECSPIHwWriteZerosTxFIFO (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ ECSPI_SPB_TRANSFER* TransferPtr
    );

VOID
ECSPIHwClearFIFOs (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr
    );

VOID
ECSPIHwDisableTransferInterrupts (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ const ECSPI_SPB_TRANSFER* TransferPtr
    );

BOOLEAN
ECSPIpHwStartBurstIf (
    _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
    _In_ ECSPI_SPB_TRANSFER* TransferPtr
    );

//
// ECSPIhw private methods
//
#ifdef _ECSPI_HW_CPP_

    static NTSTATUS
    ECSPIpHwCalcFreqDivider (
        _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
        _In_ ULONG ConnectionSpeedHz,
        _Inout_ ECSPI_CONREG* CtrlRegPtr
        );

    static ULONG
    ECSPIpHwReadWordFromMdl (
        _In_ ECSPI_SPB_TRANSFER* TransferPtr,
        _Out_ PULONG DataPtr
        );

    static ULONG
    ECSPIpHwWriteWordToMdl (
        _In_ ECSPI_SPB_TRANSFER* TransferPtr,
        _In_ ULONG Data
        );

    //
    // Routine Description:
    //
    //  ECSPIpHwByteSwap swaps the given number of bytes within a
    //  32 bit variable.
    //
    // Arguments:
    //
    //  Data - The original base value
    //
    //  BytesToSwap - The number of bytes to swap.
    //
    // Return Value:
    //
    //  A ULONG value with the bytes swapped.
    //
    __forceinline
    ULONG
    ECSPIpHwByteSwap (
        _In_ ULONG Data,
        _In_ ULONG BytesToSwap
        ) 
    {
        switch (BytesToSwap) {
        case 1:
            return Data;

        case 2:
            return ULONG(RtlUshortByteSwap(Data));

        case 3:
            return RtlUlongByteSwap(Data) >> 8;

        case 4:
            return RtlUlongByteSwap(Data);

        default:
            NT_ASSERT(FALSE);
            return ULONG(-1);
        }
    }

    //
    // Routine Description:
    //
    //  ECSPIpHwDataSwap swaps the given number based on
    //  the data bit length.
    //  Since ECSPI always shifts MSB first. When we are doing 
    //  8 bit transfers, we need to make sure data is sent 
    //  in the expected order.
    //
    // Arguments:
    //
    //  Data - The original base value
    //
    //  DataBitLengthBytes - The data bit length in bytes
    //
    // Return Value:
    //
    //  A ULONG value with the bytes swapped.
    //
    __forceinline
    ULONG
    ECSPIpHwDataSwap (
        _In_ ULONG Data,
        _In_ ULONG BytesToSwap,
        _In_ ULONG DataBitLengthBytes
        )
    {
        switch (DataBitLengthBytes) {
        case 1: // 8 bit transfers
            return ECSPIpHwByteSwap(Data, BytesToSwap);

        case 2: // 16 bit transfers
            if (BytesToSwap == 4) {

                return UlongWordSwap(Data);
            }
            NT_ASSERT(BytesToSwap == 2);
            __fallthrough;
        case 4: // 32 bit transfers
            return Data;

        default:
            NT_ASSERT(FALSE);
            return ULONG(-1);
        }
    }

    static VOID
    ECSPIpHwUpdateTransfer (
        _In_ ECSPI_SPB_TRANSFER* TransferPtr,
        _In_ ULONG BytesTransferred
        );

    static VOID
    ECSPIpHwEnableLoopbackIf (
        _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr
        );

#endif // _ECSPI_HW_CPP_

WDF_EXTERN_C_END

#endif // !_ECSPI_HW_H_

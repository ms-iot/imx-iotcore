// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   imxuarthw.h
//
// Abstract:
//
//   IMX UART Hardware Register Definitions
//

#ifndef _IMX_UART_HW_H_
#define _IMX_UART_HW_H_

//
// UART Receiver Register bit definitions
//
enum IMX_UART_RXD : ULONG {
    IMX_UART_RXD_RX_DATA_MASK =      (0xff << 0),
    IMX_UART_RXD_PRERR =             (1 << 10),
    IMX_UART_RXD_BRK =               (1 << 11),
    IMX_UART_RXD_FRMERR =            (1 << 12),
    IMX_UART_RXD_OVRRUN =            (1 << 13),
    IMX_UART_RXD_ERR =               (1 << 14),
    IMX_UART_RXD_CHARRDY =           (1 << 15),
};

//
// UART Control Register 1 bit definitions
//
enum IMX_UART_UCR1 : ULONG {
    IMX_UART_UCR1_UARTEN =           (1 << 0),
    IMX_UART_UCR1_DOZE =             (1 << 1),
    IMX_UART_UCR1_ATDMAEN =          (1 << 2),
    IMX_UART_UCR1_TXDMAEN =          (1 << 3),
    IMX_UART_UCR1_SNDBRK =           (1 << 4),
    IMX_UART_UCR1_RTSDEN =           (1 << 5),
    IMX_UART_UCR1_TXMPTYEN =         (1 << 6),
    IMX_UART_UCR1_IREN =             (1 << 7),
    IMX_UART_UCR1_RXDMAEN =          (1 << 8),
    IMX_UART_UCR1_RRDYEN =           (1 << 9),
    IMX_UART_UCR1_ICD_MASK =         (3 << 10),
    IMX_UART_UCR1_ICD_4 =            (0 << 10),
    IMX_UART_UCR1_ICD_8 =            (1 << 10),
    IMX_UART_UCR1_ICD_16 =           (2 << 10),
    IMX_UART_UCR1_ICD_32 =           (3 << 10),
    IMX_UART_UCR1_IDEN =             (1 << 12),
    IMX_UART_UCR1_TRDYEN =           (1 << 13),
    IMX_UART_UCR1_ADBR =             (1 << 14),
    IMX_UART_UCR1_ADEN =             (1 << 15),
};

//
// UART Control Register 2 bit definitions
//
enum IMX_UART_UCR2 : ULONG {
    IMX_UART_UCR2_SRST =             (1 << 0),
    IMX_UART_UCR2_RXEN =             (1 << 1),
    IMX_UART_UCR2_TXEN =             (1 << 2),
    IMX_UART_UCR2_ATEN =             (1 << 3),
    IMX_UART_UCR2_RTSEN =            (1 << 4),
    IMX_UART_UCR2_WS =               (1 << 5),
    IMX_UART_UCR2_STPB =             (1 << 6),
    IMX_UART_UCR2_PROE =             (1 << 7),
    IMX_UART_UCR2_PREN =             (1 << 8),
    IMX_UART_UCR2_RTEC_MASK =        (3 << 9),
    IMX_UART_UCR2_RTEC_RISING =      (0 << 9),
    IMX_UART_UCR2_RTEC_FALLING =     (1 << 9),
    IMX_UART_UCR2_RTEC_BOTH =        (2 << 9),
    IMX_UART_UCR2_ESCEN =            (1 << 11),
    IMX_UART_UCR2_CTS =              (1 << 12),
    IMX_UART_UCR2_CTSC =             (1 << 13),
    IMX_UART_UCR2_IRTS =             (1 << 14),
    IMX_UART_UCR2_ESCI =             (1 << 15),
};

//
// UART Control Register 3 bit definitions
//
enum IMX_UART_UCR3 : ULONG {
    IMX_UART_UCR3_ACIEN =            (1 << 0),
    IMX_UART_UCR3_INVT =             (1 << 1),
    IMX_UART_UCR3_RXDMUXSEL =        (1 << 2),
    IMX_UART_UCR3_DTRDEN =           (1 << 3),
    IMX_UART_UCR3_AWAKEN =           (1 << 4),
    IMX_UART_UCR3_AIRINTEN =         (1 << 5),
    IMX_UART_UCR3_RXDSEN =           (1 << 6),
    IMX_UART_UCR3_ADNIMP =           (1 << 7),
    IMX_UART_UCR3_RI =               (1 << 8),
    IMX_UART_UCR3_DCD =              (1 << 9),
    IMX_UART_UCR3_DSR =              (1 << 10),
    IMX_UART_UCR3_FRAERREN =         (1 << 11),
    IMX_UART_UCR3_PARERREN =         (1 << 12),
    IMX_UART_UCR3_DTREN =            (1 << 13),
    IMX_UART_UCR3_DPEC_MASK =        (3 << 14),
    IMX_UART_UCR3_DPEC_RISING =      (0 << 14),
    IMX_UART_UCR3_DPEC_FALLING =     (1 << 14),
    IMX_UART_UCR3_DPEC_BOTH =        (2 << 14),
};

//
// UART Control Register 4 bit definitions
//
enum IMX_UART_UCR4 : ULONG {
    IMX_UART_UCR4_DREN =             (1 << 0),
    IMX_UART_UCR4_OREN =             (1 << 1),
    IMX_UART_UCR4_BKEN =             (1 << 2),
    IMX_UART_UCR4_TCEN =             (1 << 3),
    IMX_UART_UCR4_LPBYP =            (1 << 4),
    IMX_UART_UCR4_IRSC =             (1 << 5),
    IMX_UART_UCR4_IDDMAEN =          (1 << 6),
    IMX_UART_UCR4_WKEN =             (1 << 7),
    IMX_UART_UCR4_ENIRI =            (1 << 8),
    IMX_UART_UCR4_INVR =             (1 << 9),
    IMX_UART_UCR4_CTSTL_MASK =       (0x3f << 10),
    IMX_UART_UCR4_CTSTL_SHIFT =      10,
};

//
// UART FIFO Control Register bit definitions
//
enum IMX_UART_UFCR : ULONG {
    IMX_UART_UFCR_RXTL_MASK =        (0x3f << 0),
    IMX_UART_UFCR_RXTL_SHIFT =       0,
    IMX_UART_UFCR_DCEDTE =           (1 << 6),
    IMX_UART_UFCR_RFDIV_MASK =       (7 << 7),
    IMX_UART_UFCR_RFDIV_6 =          (0 << 7),
    IMX_UART_UFCR_RFDIV_5 =          (1 << 7),
    IMX_UART_UFCR_RFDIV_4 =          (2 << 7),
    IMX_UART_UFCR_RFDIV_3 =          (3 << 7),
    IMX_UART_UFCR_RFDIV_2 =          (4 << 7),
    IMX_UART_UFCR_RFDIV_1 =          (5 << 7),
    IMX_UART_UFCR_RFDIV_7 =          (6 << 7),
    IMX_UART_UFCR_TXTL_MASK =        (0x3f << 10),
    IMX_UART_UFCR_TXTL_SHIFT =       10,
};

//
// Gets RFDIV mask from integer divider value
//
FORCEINLINE IMX_UART_UFCR IMXUartRfDivMaskFromIntegerValue (ULONG RfDivValue)
{
    switch (RfDivValue) {
    case 1: return IMX_UART_UFCR_RFDIV_1;
    case 2: return IMX_UART_UFCR_RFDIV_2;
    case 3: return IMX_UART_UFCR_RFDIV_3;
    case 4: return IMX_UART_UFCR_RFDIV_4;
    case 5: return IMX_UART_UFCR_RFDIV_5;
    case 6: return IMX_UART_UFCR_RFDIV_6;
    case 7: return IMX_UART_UFCR_RFDIV_7;
    default:
        NT_ASSERT(!"Invalid RfDiv value");
        return IMX_UART_UFCR_RFDIV_6;
    }
}

//
// UART Status Register 1 bit definitions
//
enum IMX_UART_USR1 : ULONG {
    IMX_UART_USR1_SAD =              (1 << 3),
    IMX_UART_USR1_AWAKE =            (1 << 4),
    IMX_UART_USR1_AIRINT =           (1 << 5),
    IMX_UART_USR1_RXDS =             (1 << 6),
    IMX_UART_USR1_DTRD =             (1 << 7),
    IMX_UART_USR1_AGTIM =            (1 << 8),
    IMX_UART_USR1_RRDY =             (1 << 9),
    IMX_UART_USR1_FRAMERR =          (1 << 10),
    IMX_UART_USR1_ESCF =             (1 << 11),
    IMX_UART_USR1_RTSD =             (1 << 12),
    IMX_UART_USR1_TRDY =             (1 << 13),
    IMX_UART_USR1_RTSS =             (1 << 14),
    IMX_UART_USR1_PARITYERR =        (1 << 15),
};

//
// UART Status Register 2 bit definitions
//
enum IMX_UART_USR2 : ULONG {
    IMX_UART_USR2_RDR =              (1 << 0),
    IMX_UART_USR2_ORE =              (1 << 1),
    IMX_UART_USR2_BRCD =             (1 << 2),
    IMX_UART_USR2_TXDC =             (1 << 3),
    IMX_UART_USR2_RTSF =             (1 << 4),
    IMX_UART_USR2_DCDIN =            (1 << 5),
    IMX_UART_USR2_DCDDELT =          (1 << 6),
    IMX_UART_USR2_WAKE =             (1 << 7),
    IMX_UART_USR2_IRINT =            (1 << 8),
    IMX_UART_USR2_RIIN =             (1 << 9),
    IMX_UART_USR2_RIDLET =           (1 << 10),
    IMX_UART_USR2_ACST =             (1 << 11),
    IMX_UART_USR2_IDLE =             (1 << 12),
    IMX_UART_USR2_DTRF =             (1 << 13),
    IMX_UART_USR2_TXFE =             (1 << 14),
    IMX_UART_USR2_ADET =             (1 << 15),
};

//
// UART Test Register bit definitions
//
enum IMX_UART_UTS : ULONG {
    IMX_UART_UTS_SOFTRST =           (1 << 0),
    IMX_UART_UTS_RXFULL =            (1 << 3),
    IMX_UART_UTS_TXFULL =            (1 << 4),
    IMX_UART_UTS_RXEMPTY =           (1 << 5),
    IMX_UART_UTS_TXEMPTY =           (1 << 6),
    IMX_UART_UTS_RXDBG =             (1 << 9),
    IMX_UART_UTS_LOOPIR =            (1 << 10),
    IMX_UART_UTS_DBGEN =             (1 << 11),
    IMX_UART_UTS_LOOP =              (1 << 12),
    IMX_UART_UTS_FRCPERR =           (1 << 13),
};

//
// Size of RX and TX FIFOs
//
enum : ULONG { IMX_UART_FIFO_COUNT = 32 };

struct IMX_UART_REGISTERS {
    ULONG Rxd;                  // 0x00: UART Receiver Register
    ULONG reserved1[15];
    ULONG Txd;                  // 0x40: UART Transmitter Register
    ULONG reserved2[15];
    ULONG Ucr1;                 // 0x80: UART Control Register 1
    ULONG Ucr2;                 // 0x84: UART Control Register 2
    ULONG Ucr3;                 // 0x88: UART Control Register 3
    ULONG Ucr4;                 // 0x8C: UART Control Register 4
    ULONG Ufcr;                 // 0x90: UART FIFO Control Register
    ULONG Usr1;                 // 0x94: UART Status Register 1
    ULONG Usr2;                 // 0x98: UART Status Register 2
    ULONG Uesc;                 // 0x9C: UART Escape Character Register
    ULONG Utim;                 // 0xA0: UART Escape Timer Register
    ULONG Ubir;                 // 0xA4: UART BRM Incremental Register
    ULONG Ubmr;                 // 0xA8: UART BRM Modulator Register (UART1_UBMR)
    ULONG Ubrc;                 // 0xAC: UART Baud Rate Count Register
    ULONG Onems;                // 0xB0: UART One Millisecond Register
    ULONG Uts;                  // 0xB4: UART Test Register
    ULONG Umcr;                 // 0xB8: UART RS-485 Mode Control Register
};

#endif // _IMX_UART_HW_H_

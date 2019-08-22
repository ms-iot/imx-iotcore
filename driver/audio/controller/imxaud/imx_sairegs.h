/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Typedefs for the IMX SAI register blocks.

*/

#pragma once

#ifdef _ARM64_
//
// IMX8M: SAI Parameter Register (I2Sx_PARAM)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG DataLines : 4;                    // bit 0:3
        ULONG Reserved0 : 4;                    // bit 4:7
        ULONG FifoSize : 4;                     // bit 8:11
        ULONG Reserved1 : 4;                    // bit 12:16
        ULONG FrameSize : 4;                    // bit 17:19
        ULONG Reserved2 : 12;                   // bit 20:31
    };
} SAI_PARAM_REGISTER, *PSAI_PARAM_REGISTER;

//
// IMX8M: SAI Transmit Control Register (I2Sx_TCSR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FifoRequestDMAEnable : 1;         // bit 0
        ULONG FifoWarningDMAEnable : 1;         // bit 1
        ULONG Reserved0 : 3;                    // bit 2:4
        ULONG Reserved1 : 3;                    // bit 5:7
        ULONG FifoRequestInterruptEnable : 1;   // bit 8
        ULONG FifoWarningInterruptEnable : 1;   // bit 9
        ULONG FifoErrorInterruptEnable : 1;     // bit 10
        ULONG SyncErrorInterruptEnable : 1;     // bit 11
        ULONG WordStartInterruptEnable : 1;     // bit 12
        ULONG Reserved2 : 3;                    // bit 13:15
        ULONG FifoRequestFlag : 1;              // bit 16
        ULONG FifoWarningFlag : 1;              // bit 17
        ULONG FifoErrorFlag : 1;                // bit 18
        ULONG SyncErrorFlag : 1;                // bit 19
        ULONG WordStartFlag : 1;                // bit 20
        ULONG Reserved3 : 3;                    // bit 21:23
        ULONG SoftwareReset : 1;                // bit 24
        ULONG FifoReset : 1;                    // bit 25
        ULONG Reserved4 : 2;                    // bit 26:27
        ULONG BitClockEnable : 1;               // bit 28
        ULONG DebugEnable : 1;                  // bit 29
        ULONG Reserved5 : 1;                    // bit 30
        ULONG TransmitterEnable : 1;            // bit 31
    };
} SAI_TRANSMIT_CONTROL_REGISTER, *PSAI_TRANSMIT_CONTROL_REGISTER;

#else

//
// IMX7D: SAI Transmit Control Register (I2Sx_TCSR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FifoRequestDMAEnable : 1;         // bit 0
        ULONG FifoWarningDMAEnable : 1;         // bit 1
        ULONG Reserved0 : 3;                    // bit 2:4
        ULONG Reserved1 : 3;                    // bit 5:7
        ULONG FifoRequestInterruptEnable : 1;   // bit 8
        ULONG FifoWarningInterruptEnable : 1;   // bit 9
        ULONG FifoErrorInterruptEnable : 1;     // bit 10
        ULONG SyncErrorInterruptEnable : 1;     // bit 11
        ULONG WordStartInterruptEnable : 1;     // bit 12
        ULONG Reserved2 : 3;                    // bit 13:15
        ULONG FifoRequestFlag : 1;              // bit 16
        ULONG FifoWarningFlag : 1;              // bit 17
        ULONG FifoErrorFlag : 1;                // bit 18
        ULONG SyncErrorFlag : 1;                // bit 19
        ULONG WordStartFlag : 1;                // bit 20
        ULONG Reserved3 : 3;                    // bit 21:23
        ULONG SoftwareReset : 1;                // bit 24
        ULONG FifoReset : 1;                    // bit 25
        ULONG Reserved4 : 2;                    // bit 26:27
        ULONG BitClockEnable : 1;               // bit 28
        ULONG DebugEnable : 1;                  // bit 29
        ULONG StopEnable : 1;                   // bit 30
        ULONG TransmitterEnable : 1;            // bit 31
    };
} SAI_TRANSMIT_CONTROL_REGISTER, *PSAI_TRANSMIT_CONTROL_REGISTER;
#endif

#define SAI_CTRL_INTERRUPTMASK 0x00001F00
#define SAI_CTRL_ERRORFLAGS 0x001C0000

#ifdef _ARM64_

//
// IMX8M: SAI Transmit Configuration 1 Register (I2Sx_TCR1)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG TransmitFifoWatermark : 7;    // bit 0:6
        ULONG Reserved0 : 25;               // bit 7:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_1, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_1;

#else

//
// IMX7D: SAI Transmit Configuration 1 Register (I2Sx_TCR1)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG TransmitFifoWatermark : 5;    // bit 0:4
        ULONG Reserved0 : 27;               // bit 5:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_1, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_1;
#endif

//
// IMX7D/8M: SAI Transmit Configuration 2 Register (I2Sx_TCR2)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG BitClockDivide : 8;       // bit 0:7
        ULONG Reserved0 : 16;           // bit 8:23
        ULONG BitClockDirection : 1;    // bit 24
        ULONG BitClockParity : 1;       // bit 25
        ULONG MasterClockSelect : 2;    // bit 26:27
        ULONG BitClockInput : 1;        // bit 28
        ULONG BitClockSwap : 1;         // bit 29
        ULONG SynchronousMode : 2;      // bit 30:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_2, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_2;

#ifdef _ARM64_

//
// IMX8M: SAI Transmit Configuration 3 Register (I2Sx_TCR3)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG WordFlagConfiguration : 5;    // bit 0:4
        ULONG Reserved0 : 11;               // bit 5:15
        ULONG TransmitChannelEnable : 8;    // bit 16:23
        ULONG ChannelFifoReset : 8;         // bit 24:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_3, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_3;

#else

//
// IMX7D: SAI Transmit Configuration 3 Register (I2Sx_TCR3)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG WordFlagConfiguration : 5;    // bit 0:4
        ULONG Reserved0 : 11;               // bit 5:15
        ULONG TransmitChannelEnable : 1;    // bit 16
        ULONG Reserved1 : 7;                // bit 17:23
        ULONG Reserved2 : 8;                // bit 24:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_3, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_3;
#endif

#ifdef _ARM64_

//
// IMX8M: SAI Transmit Configuration 4 Register (I2Sx_TCR4)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FrameSyncDirection : 1;  // bit 0
        ULONG FrameSyncPolarity : 1;   // bit 1
        ULONG OnDemandMode : 1;        // bit 2
        ULONG FrameSyncEarly : 1;      // bit 3
        ULONG MSBFirst : 1;            // bit 4
        ULONG ChannelMode : 1;         // bit 5
        ULONG Reserved1: 2;            // bit 6:7
        ULONG SyncWidth : 5;           // bit 8:12
        ULONG Reserved2 : 3;           // bit 13:15
        ULONG FrameSize : 5;           // bit 16:20
        ULONG Reserved3 : 3;           // bit 21:23
        ULONG FifoPackMode : 2;        // bit 24:25
        ULONG FifoCombineMode : 2;     // bit 26:27
        ULONG FifoContinueOnErr : 1;   // bit 28
        ULONG Reserved7 : 3;           // bit 29:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_4, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_4;

#else

//
// IMX7D: SAI Transmit Configuration 4 Register (I2Sx_TCR4)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FrameSyncDirection : 1;  // bit 0
        ULONG FrameSyncPolarity : 1;   // bit 1
        ULONG Reserved0 : 1;           // bit 2
        ULONG FrameSyncEarly : 1;      // bit 3
        ULONG MSBFirst : 1;            // bit 4
        ULONG Reserved1 : 3;           // bit 5:7
        ULONG SyncWidth : 5;           // bit 8:12
        ULONG Reserved2 : 3;           // bit 13:15
        ULONG FrameSize : 5;           // bit 16:20
        ULONG Reserved3 : 3;           // bit 21:23
        ULONG Reserved4 : 2;           // bit 24:25
        ULONG Reserved5 : 2;           // bit 26:27
        ULONG Reserved6 : 1;           // bit 28
        ULONG Reserved7 : 3;           // bit 29:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_4, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_4;
#endif

//
// IMX7D/8M: SAI Transmit Configuration 5 Register (I2Sx_TCR5)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG Reserved0 : 8;        // bit 0:7
        ULONG FirstBitShifted : 5;  // bit 8:12
        ULONG Reserved1 : 3;        // bit 13:15
        ULONG Word0Width : 5;       // bit 16:20
        ULONG Reserved2 : 3;        // bit 21:23
        ULONG WordNWidth : 5;       // bit 24:28
        ULONG Reserved3 : 3;         // bit 29:31
    };
} SAI_TRANSMIT_CONFIGURATION_REGISTER_5, *PSAI_TRANSMIT_CONFIGURATION_REGISTER_5;

//
// IMX7D/8M: SAI Transmit Data Register (I2Sx_TDRn)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG TransmitDataRegister : 32;    // bit 0:31
    };
} SAI_TRANSMIT_DATA_REGISTER, *PSAI_TRANSMIT_DATA_REGISTER;

#ifdef _ARM64_

//
// IMX8M: SAI Transmit FIFO Register (I2Sx_TFRn)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG ReadFifoPointer : 8;      // bit 0:7
        ULONG Reserved0 : 8;            // bit 8:15
        ULONG WriteFifoPointer : 8;     // bit 16:24
        ULONG Reserved1 : 7;            // bit 25:30
        ULONG WriteChannelPointer : 1;  // bit 31
    };
} SAI_TRANSMIT_FIFO_REGISTER, *PSAI_TRANSMIT_FIFO_REGISTER;

#define FIFO_PTR_MSB(fifoptr) (fifoptr&0x40)
#define FIFO_PTR_NOMSB(fifoptr) (fifoptr&0x3F)

#else

//
// IMX7D: SAI Transmit FIFO Register (I2Sx_TFRn)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG ReadFifoPointer : 6;      // bit 0:5
        ULONG Reserved0 : 10;           // bit 6:15
        ULONG WriteFifoPointer : 6;     // bit 16:21
        ULONG Reserved1 : 9;            // bit 22:30
        ULONG Reserved2 : 1;            // bit 31
    };
} SAI_TRANSMIT_FIFO_REGISTER, *PSAI_TRANSMIT_FIFO_REGISTER;

#define FIFO_PTR_MSB(fifoptr) (fifoptr&0x20)
#define FIFO_PTR_NOMSB(fifoptr) (fifoptr&0x1F)

#endif

//
// IMX7D/8M: SAI Transmit Mask Register (I2Sx_TMR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG TransmitWordMask : 32;      // bit 0:31
    };
} SAI_TRANSMIT_MASK_REGISTER, *PSAI_TRANSMIT_MASK_REGISTER;

// TODO update RX registers for imx8m
//
// IMX7D: 13.8.4.10 SAI Receive Control Register (I2Sx_RCSR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FifoRequestDMAEnable : 1;         // bit 0
        ULONG FifoWarningDMAEnable : 1;         // bit 1
        ULONG Reserved0 : 3;                    // bit 2:4
        ULONG Reserved1 : 3;                    // bit 5:7
        ULONG FifoRequestInterruptEnable : 1;   // bit 8
        ULONG FifoWarningInterruptEnable : 1;   // bit 9
        ULONG FifoErrorInterruptEnable : 1;     // bit 10
        ULONG SyncErrorInterruptEnable : 1;     // bit 11
        ULONG WordStartInterruptEnable : 1;     // bit 12
        ULONG Reserved2 : 3;                    // bit 13:15
        ULONG FifoRequestFlag : 1;              // bit 16
        ULONG FifoWarningFlag : 1;              // bit 17
        ULONG FifoErrorFlag : 1;                // bit 18
        ULONG SyncErrorFlag : 1;                // bit 19
        ULONG WordStartFlag : 1;                // bit 20
        ULONG Reserved3 : 3;                    // bit 21:23
        ULONG SoftwareReset : 1;                // bit 24
        ULONG FifoReset : 1;                    // bit 25
        ULONG Reserved4 : 2;                    // bit 26:27
        ULONG BitClockEnable : 1;               // bit 28
        ULONG DebugEnable : 1;                  // bit 29
        ULONG StopEnable : 1;                   // bit 30
        ULONG ReceiverEnable : 1;               // bit 31
    };
} SAI_RECEIVE_CONTROL_REGISTER, *PSAI_RECEIVE_CONTROL_REGISTER;

//
// IMX7D: 13.8.4.11 SAI Receive Configuration 1 Register (I2Sx_RCR1)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG ReceiveFifoWatermark : 5;     // bit 0:4
        ULONG Reserved0 : 27;               // bit 5:31
    };
} SAI_RECEIVE_CONFIGURATION_REGISTER_1, *PSAI_RECEIVE_CONFIGURATION_REGISTER_1;

//
// IMX7D: 13.8.4.12 SAI Receive Configuration 2 Register (I2Sx_RCR2)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG BitClockDivide : 8;       // bit 0:7
        ULONG Reserved0 : 16;           // bit 8:23
        ULONG BitClockDirection : 1;    // bit 24
        ULONG BitClockPolarity : 1;     // bit 25
        ULONG MasterClockSelect : 2;    // bit 26:27
        ULONG BitClockInput : 1;        // bit 28
        ULONG BitClockSwap : 1;         // bit 29
        ULONG SynchronousMode : 2;      // bit 30:31
    };
} SAI_RECEIVE_CONFIGURATION_REGISTER_2, *PSAI_RECEIVE_CONFIGURATION_REGISTER_2;

//
// IMX7D: 13.8.4.13 SAI Receive Configuration 3 Register (I2Sx_RCR3)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG WordFlagConfiguration : 5;    // bit 0:4
        ULONG Reserved0 : 11;               // bit 5:15
        ULONG ReceiveChannelEnable : 1;     // bit 16
        ULONG Reserved1 : 7;                // bit 17:23
        ULONG Reserved2 : 8;                // bit 24:31
    };
} SAI_RECEIVE_CONFIGURATION_REGISTER_3, *PSAI_RECEIVE_CONFIGURATION_REGISTER_3;

//
// IMX7D: 13.8.4.14 SAI Receive Configuration 4 Register (I2Sx_RCR4)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FrameSyncDirection : 1;  // bit 0
        ULONG FrameSyncPolarity : 1;   // bit 1
        ULONG Reserved0 : 1;           // bit 2
        ULONG FrameSyncEarly : 1;      // bit 3
        ULONG MSBFirst : 1;            // bit 4
        ULONG Reserved1 : 3;           // bit 5:7
        ULONG SyncWidth : 5;           // bit 8:12
        ULONG Reserved2 : 3;           // bit 13:15
        ULONG FrameSize : 5;           // bit 16:20
        ULONG Reserved3 : 3;           // bit 21:23
        ULONG Reserved4 : 2;           // bit 24:25
        ULONG Reserved5 : 2;           // bit 26:27
        ULONG Reserved6 : 1;           // bit 28
        ULONG Reserved7 : 3;           // bit 29:31
    };
} SAI_RECEIVE_CONFIGURATION_REGISTER_4, *PSAI_RECEIVE_CONFIGURATION_REGISTER_4;

//
// IMX7D: 13.8.4.15 SAI Receive Configuration 5 Register (I2Sx_RCR5)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG Reserved0 : 8;        // bit 0:7
        ULONG FirstBitShifted : 5;  // bit 8:12
        ULONG Reserved1 : 3;        // bit 13:15
        ULONG Word0Width : 5;       // bit 16:20
        ULONG Reserved2 : 3;        // bit 21:23
        ULONG WordNWidth : 5;       // bit 24:28
        ULONG Reserved3 : 3;         // bit 29:31
    };
} SAI_RECEIVE_CONFIGURATION_REGISTER_5, *PSAI_RECEIVE_CONFIGURATION_REGISTER_5;

//
// IMX7D: 13.8.4.16 SAI Receive Data Register (I2Sx_RDRn)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG ReceiveDataRegister : 32;    // bit 0:31
    };
} SAI_RECEIVE_DATA_REGISTER, *PSAI_RECEIVE_DATA_REGISTER;

//
// IMX7D: 13.8.4.17 SAI Receive FIFO Register (I2Sx_RFRn)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG ReadFifoPointer : 6;  // bit 0:5
        ULONG Reserved0 : 9;        // bit 6:14
        ULONG Reserved1 : 1;        // bit 15
        ULONG WriteFifoPointer : 6; // bit 16:21
        ULONG Reserved2 : 10;       // bit 22:31

    };
} SAI_RECEIVE_FIFO_REGISTER, *PSAI_RECEIVE_FIFO_REGISTER;

//
// IMX7D: 13.8.4.18 SAI Receive Mask Register (I2Sx_RMR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG ReceiveWordMask : 32;     // bit 0:31
    };
} SAI_RECEIVE_MASK_REGISTER, *PSAI_RECEIVE_MASK_REGISTER;

#ifdef _ARM64_
//
// IMX8M: SAIx register block
//
typedef struct SaiRegisters {
    ULONG32 VersionId;
    SAI_PARAM_REGISTER ParamRegister;
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_1 TransmitConfigRegister1;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_2 TransmitConfigRegister2;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_3 TransmitConfigRegister3;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_4 TransmitConfigRegister4;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_5 TransmitConfigRegister5;
    SAI_TRANSMIT_DATA_REGISTER TransmitDataRegister;
    ULONG Reserved1[7];
    SAI_TRANSMIT_FIFO_REGISTER TransmitFifoRegister;
    ULONG Reserved2[7];
    SAI_TRANSMIT_MASK_REGISTER TransmitMaskRegister;
    ULONG Reserved3[9];
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;
    SAI_RECEIVE_CONFIGURATION_REGISTER_1 ReceiveConfigRegister1;
    SAI_RECEIVE_CONFIGURATION_REGISTER_2 ReceiveConfigRegister2;
    SAI_RECEIVE_CONFIGURATION_REGISTER_3 ReceiveConfigRegister3;
    SAI_RECEIVE_CONFIGURATION_REGISTER_4 ReceiveConfigRegister4;
    SAI_RECEIVE_CONFIGURATION_REGISTER_5 ReceiveConfigRegister5;
    SAI_RECEIVE_DATA_REGISTER ReceiveDataRegister;
    ULONG Reserved5[7];
    SAI_RECEIVE_FIFO_REGISTER ReceiveFifoRegister;
    ULONG Reserved6[7];
    SAI_RECEIVE_MASK_REGISTER ReceiveMaskRegister;
} SAI_REGISTERS, *PSAI_REGISTERS;

#else

//
// IMX7D: SAIx register block
//
typedef struct SaiRegisters {
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_1 TransmitConfigRegister1;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_2 TransmitConfigRegister2;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_3 TransmitConfigRegister3;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_4 TransmitConfigRegister4;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_5 TransmitConfigRegister5;
    ULONG Reserved0[2];
    SAI_TRANSMIT_DATA_REGISTER TransmitDataRegister;
    ULONG Reserved1[7];
    SAI_TRANSMIT_FIFO_REGISTER TransmitFifoRegister;
    ULONG Reserved2[7];
    SAI_TRANSMIT_MASK_REGISTER TransmitMaskRegister;
    ULONG Reserved3[7];
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;
    SAI_RECEIVE_CONFIGURATION_REGISTER_1 ReceiveConfigRegister1;
    SAI_RECEIVE_CONFIGURATION_REGISTER_2 ReceiveConfigRegister2;
    SAI_RECEIVE_CONFIGURATION_REGISTER_3 ReceiveConfigRegister3;
    SAI_RECEIVE_CONFIGURATION_REGISTER_4 ReceiveConfigRegister4;
    SAI_RECEIVE_CONFIGURATION_REGISTER_5 ReceiveConfigRegister5;
    ULONG Reserved4[2];
    SAI_RECEIVE_DATA_REGISTER ReceiveDataRegister;
    ULONG Reserved5[7];
    SAI_RECEIVE_FIFO_REGISTER ReceiveFifoRegister;
    ULONG Reserved6[7];
    SAI_RECEIVE_MASK_REGISTER ReceiveMaskRegister;
} SAI_REGISTERS, *PSAI_REGISTERS;

#endif


/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Typedefs for the IMX6 AUDMUX and SSI register blocks.

*/

#pragma once

//
// AUDMUX Block: There are 7 pairs of control registers - one pair for each port (endpoint) in the AUDMUX.
// Refer to section 16.6 AUDMUX Memory Map/Register Definition in the 
// i.MX 6Dual/6Quad Applications Processor Reference Manual, Rev. 3 for details.
//

// 16.6.1 Port Timing Control Register 1 (AUDMUX_PTCR1)
// PTCR1 is the Port Timing Control Register for Port 1.
// Address: 21D_8000h base + 0h offset = 21D_8000h

typedef struct PortTimingControlRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG  Reserved : 11;                         // bit 10:0
            ULONG  SynchronousAsynchronousSelect : 1;     // bit 11
            ULONG  ReceiveClockSelect : 4;                // bit 15:12
            ULONG  ReceiveClockDirectionControl : 1;      // bit 16
            ULONG  ReceiveFrameSyncSelect : 4;            // bit 20:17
            ULONG  ReceiveFrameDirectionControl : 1;      // bit 21
            ULONG  TransmitClockSelect : 4;               // bit 25:22
            ULONG  TransmitClockDirectionControl : 1;     // bit 26
            ULONG  TransmitFrameSyncSelect : 4;           // bit 30:27
            ULONG  TransmitFrameSyncDirectionControl : 1; // bit 31
        } Fields;
    } u;
} AUDMUX_PTCR, *PAUDMUX_PTCR;

#define AUDMUX_SYNCHRONOUS_MODE (1)
#define AUDMUX_ASYNCHRONOUS_MODE (0)

#define AUDMUX_OUTPUT (1)
#define AUDMUX_INPUT  (0)



// 16.6.2 Port Data Control Register 1 (AUDMUX_PDCR1)
// PDCR1 is the Port Data Control Register for Port 1.
// Address: 21D_8000h base + 4h offset = 21D_8004h

typedef struct PortDataControlRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG  InternalNetworkModeMask : 8;           // bit 7:0
            ULONG  Mode : 1;                              // bit 8
            ULONG  Reserved : 3;                          // bit 11:9
            ULONG  TransmitReceiveSwitchEnable : 1;       // bit 12
            ULONG  ReceiveDataSelect : 3;                 // bit 15:13
            ULONG  Reserved2 : 16;                        // bit 31:16
        } Fields;
    } u;
} AUDMUX_PDCR, *PAUDMUX_PDCR;

#define NORMAL_MODE (0)
#define INTERNAL_NETWORK_MODE (1)

#define TRSE_NOSWITCH (0)
#define TRSE_SWITCH (1)

//
// Port numbers are zero based. I.e. Port1 is 000.
//

typedef enum 
{
    AUDMUX_PORT1 = 0,
    AUDMUX_PORT2 = 1,
    AUDMUX_PORT3 = 2,
    AUDMUX_PORT4 = 3,
    AUDMUX_PORT5 = 4,
    AUDMUX_PORT6 = 5,
    AUDMUX_PORT7 = 6,
} AUDMUX_PORTS;


typedef struct AudMuxRegisters
{
    AUDMUX_PTCR PortTimingControlRegister1;
    AUDMUX_PDCR PortDataControlRegister1;

    AUDMUX_PTCR PortTimingControlRegister2;
    AUDMUX_PDCR PortDataControlRegister2;

    AUDMUX_PTCR PortTimingControlRegister3;
    AUDMUX_PDCR PortDataControlRegister3;

    AUDMUX_PTCR PortTimingControlRegister4;
    AUDMUX_PDCR PortDataControlRegister4;

    AUDMUX_PTCR PortTimingControlRegister5;
    AUDMUX_PDCR PortDataControlRegister5;

    AUDMUX_PTCR PortTimingControlRegister6;
    AUDMUX_PDCR PortDataControlRegister6;

    AUDMUX_PTCR PortTimingControlRegister7;
    AUDMUX_PDCR PortDataControlRegister7;

} AUDMUX_REGISTERS, *PAUDMUX_REGISTERS;


//
// SSIx register block. There are 3 blocks of SSI registers on IMX6.
// Refer to Section 61.9 SSI Memory Map/Register Definition for details.
//

//
// 61.9.3 SSI Control Register (SSIx_SCR)
//

typedef struct SsiControlRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG  SsiEnable : 1;                         // bit 0    
            ULONG  TransmitEnable : 1;                    // bit 1
            ULONG  ReceiveEnable : 1;                     // bit 2
            ULONG  NetworkMode : 1;                       // bit 3
            ULONG  SynchronousMode : 1;                   // bit 4
            ULONG  I2sMode : 2;                           // bit 6:5
            ULONG  NetworkClockEnable : 1;                // bit 7
            ULONG  TwoChannelEnable : 1;                  // bit 8
            ULONG  ClockIdleState : 1;                    // bit 9
            ULONG  TransmitFrameClockDisable : 1;         // bit 10
            ULONG  ReceiveFrameClockDisable : 1;          // bit 11
            ULONG  SyncTxFs : 1;                          // bit 12
            ULONG  Reserved : 19;                         // bit 31:13
        } Fields;
    } u;
} SSI_CONTROL_REGISTER, *PSSI_CONTROL_REGISTER;

#define SSI_CONTROL_ENABLED (1)
#define SSI_CONTROL_DISABLED (0)

#define FRAME_CLOCK_CONTINUE (0)
#define FRAME_CLOCK_STOP (1)

#define I2S_MODE_MASTER (1)

//
// 61.9.4 SSI Interrupt Status Register (SSIx_SISR)
//

typedef struct InterruptStatusRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG TxFifoEmpty0 : 1;                       // bit 0
            ULONG TxFifoEmpty1 : 1;                       // bit 1
            ULONG RxFifoFull0 : 1;                        // bit 2
            ULONG RxFifoFull1 : 1;                        // bit 3
            ULONG RxLastTimeSlot : 1;                     // bit 4
            ULONG TxLastTimeSlot : 1;                     // bit 5
            ULONG RxFrameSync : 1;                        // bit 6
            ULONG TxFrameSync : 1;                        // bit 7
            ULONG TxUnderRunError0 : 1;                   // bit 8
            ULONG TxUnderRunError1 : 1;                   // bit 9
            ULONG RxOverRunError0 : 1;                    // bit 10
            ULONG RxOverRunError1 : 1;                    // bit 11
            ULONG TxDataRegisterEmpty0 : 1;               // bit 12
            ULONG TxDataRegisterEmpty1 : 1;               // bit 13
            ULONG RxDataReady0 : 1;                       // bit 14
            ULONG RxDataReady1 : 1;                       // bit 15
            ULONG RxTagUpdated : 1;                       // bit 16
            ULONG CommandDataRegisterUpdated : 1;         // bit 17
            ULONG CommandAddressRegisterUpdated : 1;      // bit 18
            ULONG Reserved : 4;                           // bit 22:19
            ULONG TxFrameComplete : 1;                    // bit 23
            ULONG RxFrameComplete : 1;                    // bit 24
            ULONG Reserved2 : 7;                          // bit 31:25
        } Fields;
    } u;
} INTERRUPT_STATUS_REGISTER, *PINTERRUPT_STATUS_REGISTER;

//
// 61.9.5 SSI Interrupt Enable Register (SSIx_SIER)
// 

typedef struct InterruptEnableRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG TxFifoEmpty0En : 1;                       // bit 0
            ULONG TxFifoEmpty1En : 1;                       // bit 1
            ULONG RxFifoFull0En : 1;                        // bit 2
            ULONG RxFifoFull1En : 1;                        // bit 3
            ULONG RxLastTimeSlotEn : 1;                     // bit 4
            ULONG TxLastTimeSlotEn : 1;                     // bit 5
            ULONG RxFrameSyncEn : 1;                        // bit 6
            ULONG TxFrameSyncEn : 1;                        // bit 7
            ULONG TxUnderRunError0En : 1;                   // bit 8
            ULONG TxUnderRunError1En : 1;                   // bit 9
            ULONG RxOverRunError0En : 1;                    // bit 10
            ULONG RxOverRunError1En : 1;                    // bit 11
            ULONG TxDataRegisterEmpty0En : 1;               // bit 12
            ULONG TxDataRegisterEmpty1En : 1;               // bit 13
            ULONG RxDataReady0En : 1;                       // bit 14
            ULONG RxDataReady1En : 1;                       // bit 15
            ULONG RxTagUpdatedEn : 1;                       // bit 16
            ULONG CommandDataRegisterUpdatedEn : 1;         // bit 17
            ULONG CommandAddressRegisterUpdatedEn : 1;      // bit 18
            ULONG TxInterruptEn: 1;                         // bit 19
            ULONG TxDmaEn : 1;                              // bit 20
            ULONG RxInterruptEn : 1;                        // bit 21
            ULONG RxDmaEn : 1;                              // bit 22
            ULONG TxFrameCompleteEn : 1;                    // bit 23
            ULONG RxFrameCompleteEn : 1;                    // bit 24
            ULONG Reserved2 : 7;                            // bit 31:25
        } Fields;
    } u;
} INTERRUPT_ENABLE_REGISTER, *PINTERRUPT_ENABLE_REGISTER;


//
// 61.9.6 SSI Transmit Configuration Register (SSIx_STCR)
//

typedef struct TransmitConfigRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG  EarlyFrameSync : 1;                    // bit 0
            ULONG  FrameSyncLength : 1;                   // bit 1
            ULONG  FrameSyncInvert : 1;                   // bit 2
            ULONG  ClockPolarity : 1;                     // bit 3
            ULONG  ShiftDirection : 1;                    // bit 4
            ULONG  ClockDirection : 1;                    // bit 5
            ULONG  FrameDirection : 1;                    // bit 6
            ULONG  FifoEnable0 : 1;                       // bit 7
            ULONG  FifoEnable1 : 1;                       // bit 8
            ULONG  TxBit0 : 1;                            // bit 9
            ULONG  Reserved : 22;                         // bit 31:10
        } Fields;
    } u;
} TRANSMIT_CONFIG_REGISTER, *PTRANSMIT_CONFIG_REGISTER;

#define MSB_ALIGNED (0)
#define LSB_ALIGNED (1)

#define TCRDIR_INTERNAL (1)

#define MSB_FIRST (0)

#define FALLING_EDGE (1)

#define ONE_BIT_BEFORE (1)


//
// 61.9.6 SSI Recieve Configuration Register (SSIx_SRCR)
//

typedef struct RecieveConfigRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG  EarlyFrameSync : 1;                    // bit 0
            ULONG  FrameSyncLength : 1;                   // bit 1
            ULONG  FrameSyncInvert : 1;                   // bit 2
            ULONG  ClockPolarity : 1;                     // bit 3
            ULONG  ShiftDirection : 1;                    // bit 4
            ULONG  ClockDirection : 1;                    // bit 5
            ULONG  FrameDirection : 1;                    // bit 6
            ULONG  FifoEnable0 : 1;                       // bit 7
            ULONG  FifoEnable1 : 1;                       // bit 8
            ULONG  RxBit0 : 1;                            // bit 9
            ULONG  RxExt : 1;                             // bit 10
            ULONG  Reserved : 21;                         // bit 31:11
        } Fields;
    } u;
} RECEIVE_CONFIG_REGISTER, *PRECEIVE_CONFIG_REGISTER;

#define RXEXT_NO_SIGN_EXTEND  (0)

//
// 61.9.8 SSI Transmit Clock Control Register (SSIx_STCCR)
//

typedef struct TransmitClockControl
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG  PrescalerModulusSelect : 8;            // bit 7:0
            ULONG  FrameRateDivider : 5;                  // bit 12:8    a.k.a. Divider control (DC)
            ULONG  WordLengthControl : 4;                 // bit 16:3
            ULONG  PrescalerRange : 1;                    // bit 17
            ULONG  DivideByTwo : 1;                       // bit 18
            ULONG  Reserved : 13;                         // bit 31:19
        } Fields;
    } u;
} TRANSMIT_CLOCK_CONTROL_REGISTER, *PTRANSMIT_CLOCK_CONTROL_REGISTER;

#define WLC_24_BIT (0x0b);

//
// 61.9.10 SSI FIFO Control/Status Register (SSIx_SFCSR)
//

typedef struct FifoControlRegister
{
    union
    {
        ULONG AsUlong;
        struct
        {
            ULONG TransmitFifoWaterMark0 : 4;             // bit 3:0
            ULONG ReceiveFifoWaterMark0 : 4;              // bit 7:4
            ULONG TransmitFifoCounter0 : 4;               // bit 11:8
            ULONG ReceiveFifoCounter0 : 4;                // bit 15:12
            ULONG TransmitFifoWaterMark1 : 4;             // bit 19:16
            ULONG ReceiveFifoWaterMark1 : 4;              // bit 23:20
            ULONG TransmitFifoCounter1 : 4;               // bit 27:24
            ULONG ReceiveFifoCounter1 : 4;                // bit 31:28
        } Fields;
    } u;
} FIFO_CONTROL_REGISTER, *PFIFO_CONTROL_REGISTER;


typedef struct SsiRegisters
{
    ULONG TransmitDataRegister0;
    ULONG TransmitDataRegister1;
    ULONG ReceiveDataRegister0;
    ULONG ReceiveDataRegister1;
    SSI_CONTROL_REGISTER ControlRegister;
    INTERRUPT_STATUS_REGISTER InterruptStatusRegister;
    INTERRUPT_ENABLE_REGISTER InterruptEnableRegister;
    TRANSMIT_CONFIG_REGISTER TransmitConfigRegister;
    RECEIVE_CONFIG_REGISTER ReceiveConfigRegister;
    TRANSMIT_CLOCK_CONTROL_REGISTER TransmitClockControlRegister;
    ULONG ReceiveClockControlRegister;
    FIFO_CONTROL_REGISTER FifoControlRegister;
    ULONG Ac97ControlRegister;
    ULONG Ac97CommandAddressRegister;
    ULONG Ac97CommandDataRegister;
    ULONG Ac97TagRegister;
    ULONG TransmitTimeSlotMaskRegister;
    ULONG ReceiveTimeSlotMaskRegister;
    ULONG Ac97ChannelStatusRegister;
    ULONG Ac97ChannelEnableRegister;
    ULONG Ac97ChannelDisableRegister;
} SSI_REGISTERS, *PSSI_REGISTERS;



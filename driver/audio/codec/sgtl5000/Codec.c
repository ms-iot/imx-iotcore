/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    Codec.c - Sends commands to codec via I2c.

Environment:

    User-mode Driver Framework 2

*/

#include "Driver.h"
#include "Codec.h"
#include <spb.h>
#include "codec.tmh"

#define CODEC_I2C_ADDR  (0x1a)

typedef struct {
    DWORD MsSleepTime;
    UCHAR CommandBytes[4];
} CODEC_COMMAND, *PCODEC_COMMAND;

NTSTATUS
CodecSendCommands(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_reads_(NumCommands) CODEC_COMMAND CodecCommand[],
    USHORT NumCommands
    );

NTSTATUS
CodecSendCommand(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ PCODEC_COMMAND CodecCommand
    );

NTSTATUS
CodecInitializeHeadphoneOutJack(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    CODEC_COMMAND Commands[] =
    {   
                { 0,    0x00, 0x30, 0x40, 0x60 },    // CHIP_ANA_POWER 0x0030       
                                                     // chip is externally driven, so disable power regulators                               
        { 0,    0x00, 0x26, 0x00, 0x6c },    // CHIP_LINREG_CTRL 0x0026
                                             // Configure the charge pump to use the VDDIO rail (set bit 5 and bit 6)
        { 0,    0x00, 0x28, 0x01, 0xf0 },    // CHIP_REF_CTRL 0x0028
                                             // 
        { 0,    0x00, 0x2c, 0x03, 0x20 },    // CHIP_LINE_OUT_CTRL 0x002C
                                             // Set LINEOUT reference voltage to VDDIO/2 (1.6 V) (bits 5:0),
                                             // bias current (bits 11:8) to the recommended value of 0.36 mA 
        { 0,    0x00, 0x28, 0x01, 0xf9 },    // CHIP_REF_CTRL 0x0028
                                             // Set analog ground voltage to 1.575V, Bias control to 12.5%, 
                                             // enable slow volume ramp to minimize the startup pop
        { 0,    0x00, 0x2a, 0x02, 0x31 },    // CHIP_MIC_CTRL 0x002a
                                             // Select 4 ohm input, MIC Bias at 2.0V, 20 dB MIC amplifier gain.
        { 0,    0x00, 0x3c, 0x66, 0x66 },    // CHIP_SHORT_CTRL 0x003C
                                             // Enable short detect mode for headphone left/right
                                             // and center channel and set short detect current trip level
                                             // to 175 mA.
        { 0,    0x00, 0x24, 0x01, 0x22 },    // CHIP_ANA_CTRL 0x0024                                                     
                                             // Unmute the headphone and ADC, leave LINEOUT muted
        { 0,    0x00, 0x30, 0x40, 0x7f },    // CHIP_ANA_POWER 0x0030
                                             // enable DACs, Headphone power
        { 0,    0x00, 0x30, 0x40, 0xff },    // CHIP_ANA_POWER 0x0030
                                             // Enable the VAG reference buffer to slowly ramp out the headphone
                                             // amplifier while avoiding pops
        { 0,    0x00, 0x02, 0x00, 0x73 },    // CHIP_DIG_POWER 0x0002
                                             // Power up desired digital blocks
                                             // I2S_IN (bit 0), I2S_OUT (bit 1), DAP (bit 4), DAC (bit 5),
                                             // ADC (bit 6) are powered on 
        { 0,    0x00, 0x06, 0x00, 0x00 },    // CHIP_I2S_CTRL 0x0006
                                             // I2s Slave mode, 32bit
        { 0,    0x00, 0x04, 0x00, 0x06 },    // CHIP_CLK_CTRL 0x0004
                                             // 44.1 KHz, MCLK_FREQ is 512x the audio sample rate
        { 0,    0x00, 0x0e, 0x02, 0x00 },    // CHIP_ADCDAC_CTRL 0x000E                                                     
                                             // unmute DACs, leave the volume ramp enable on.

    };

    NTSTATUS status;

    status = CodecSendCommands(DeviceContext, Commands, ARRAYSIZE(Commands));

    return status;
}

NTSTATUS
CodecSendCommand(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ PCODEC_COMMAND CodecCommand
    )
{
    NTSTATUS status;

    ULONG_PTR bytesWritten;
    
    WDF_MEMORY_DESCRIPTOR MemDescriptor;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&MemDescriptor,&CodecCommand->CommandBytes[0], sizeof(CodecCommand->CommandBytes));

    status = WdfIoTargetSendWriteSynchronously(DeviceContext->I2cTarget, NULL, &MemDescriptor, 0, NULL, &bytesWritten);
    
    if (CodecCommand->MsSleepTime != 0)
    {
        Sleep(CodecCommand->MsSleepTime);
    }

    return status;
}

NTSTATUS
CodecSendCommands(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_reads_(NumCommands) CODEC_COMMAND CodecCommand[],
    USHORT NumCommands
    )
{
    ULONG i;
    NTSTATUS status;

    for (i = 0; i < NumCommands; i++)
    {
        status = CodecSendCommand(DeviceContext, &CodecCommand[i]);

        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
    return STATUS_SUCCESS;
}

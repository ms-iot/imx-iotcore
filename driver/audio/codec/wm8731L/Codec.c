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
#define CODEC_I2C_ADDR_ALT (0x1b)

typedef struct {
    DWORD MsSleepTime;
    UCHAR CommandBytes[2];
} CODEC_COMMAND, *PCODEC_COMMAND;

#define CODEC_COMMAND(wait, addr, data) {(DWORD)wait, (UCHAR)((addr << 1)|((data & 0x1FF) >> 8)), (UCHAR)(data & 0xFF)}

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
    // Format of a Codec command
    // Address [15:9], Data [8:0]
    // A6 A5 A4 A3 A2 A1 A0 D8 || D7 D6 D5 D4 D3 D2 D1 D0
    // 15 14 13 12 11 10  9  8 ||  7  6  5  4  3  2  1  0
    CODEC_COMMAND Commands[] =
    {
        // Setup for 11.2896MHz mclk clock source mode (256 * fs)  - slave mode
        CODEC_COMMAND (0x0, 0x0F, 0x000),  // Register F - Reset - Writing all 0s to the reset register resets the device.
        CODEC_COMMAND (0x0, 0x06, 0x062),  // Register 6 - power down CLKOUT, OSC and MIC, power up everything else
        CODEC_COMMAND (0x0, 0x08, 0x020),  // Register 8 - Core Clock is Mclk, Clockout is Core Clock, 256fs, BOSR=0, SR3=1
        CODEC_COMMAND (0x0, 0x07, 0x00A),  // Register 7 - slave-mode, I2S, MSB-First left 1 justified 24 bits
        CODEC_COMMAND (0x0, 0x00, 0x01F),  // Register 0 - line input left channel volume +12db (0x00 = -34.5db) 1.5db steps
        CODEC_COMMAND (0x0, 0x01, 0x01F),  // Register 1 - line input right channel volume +12db
        CODEC_COMMAND (0x0, 0x02, 0x079),  // Register 2 - left volume +0db  (0x30 = -73db) 1db steps
        CODEC_COMMAND (0x0, 0x03, 0x079),  // Register 2 - right volume +0db
        CODEC_COMMAND (0x0, 0x04, 0x012),  // Register 4 - Disable Bypass, enable DAC Select, mute mic, line-in select
        CODEC_COMMAND (0x0, 0x05, 0x000),  // Register 5 - Disable DAC soft mute
        CODEC_COMMAND (0x0, 0x09, 0x001),  // Register 9 - Activate Interface
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


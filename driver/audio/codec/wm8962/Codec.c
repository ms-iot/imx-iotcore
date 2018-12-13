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
        { 0,    0x00, 0x08, 0x08, 0x20 },  // R8 - Clocking2. Set codec to use external clock.
        { 0,    0x00, 0x1b, 0x00, 0x00 },  // R27 - Additional control 3. Set the sample rate to 44.1 kHz.
        { 0,    0x00, 0x26, 0x00, 0x13 },  // R38 - Right input PGA control - select IN3R/IN4R as the inputs to the input PGA.
        { 0,    0x00, 0x57, 0x00, 0xa0 },  // R87 - Write Sequencer Control 1 - enable the sequencer.
        { 100,  0x00, 0x5a, 0x00, 0x80 },  // R90 - Write Sequencer Control 1 - run the 'DAC to Headphone Power Up' sequence.
        { 100,  0x00, 0x5a, 0x00, 0x92 },  // R90 - Write Sequencer Control 1 - run the 'Analogue Input Power Up' sequence.
        { 0,    0x00, 0x07, 0x00, 0x0e },  // R7 - Audio interface 0 - set word length to 32 bit. CAUTION: this gets reset by the
                                           // 'DAC to Headphone Power Up' sequence.
        { 0,    0x00, 0x01, 0x01, 0x1f },  // R1 - Mic right volume - Set the right mic channel to max volume.
        { 0,    0x00, 0x02, 0x01, 0xff },  // R2 - HPOUTL volume - Set the left headphone channel to max volume.
        { 0,    0x00, 0x03, 0x01, 0xff },  // R3 - HPOUTR volume - Set the right headphone channel to max volume.
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

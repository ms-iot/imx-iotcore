/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    HalExtension.c

Abstract:

    This file implements a HAL Extension Module for the i.MX6 Timers.

*/

//
// ------------------------------------------------------------------- Includes
//

#include <nthalext.h>
#include "iMX6Timers.h"
#include <ImxCpuRev.h>

//
// ---------------------------------------------------------------- Definitions
//

//
// Timer control constants
//

#define GPT_RESET_DONE_MAX_RETRY 100    // Max retry counter for GPT reset complete

typedef enum {
  IMX_TIMER_TYPE_INVALID,
  IMX_TIMER_TYPE_GPT,
  IMX_TIMER_TYPE_EPIT,
  IMX_TIMER_TYPE_SNVS_RTC,
} IMX_TIMER_TYPE;

//
// ------------------------------------------------- Data Structure Definitions
//

#pragma pack(push, 1)


//
// CSRT Timer descriptor
// Matches the CSRT resource descriptor (RD_TIMER).
//

typedef struct _CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE {

    CSRT_RESOURCE_DESCRIPTOR_HEADER Header;

    UINT32 TimerType;
    UINT32 ClockSource;
    UINT32 Frequency;
    UINT32 FrequencyScale;
    UINT32 BaseAddress;
    UINT32 Interrupt;

} CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE, *PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE;

#pragma pack(pop)

//
// iMX6 runtime timer internal data, that is
// passed by the framework to each timer related callback routine.
//
typedef struct _IMX6_GPT_INTERNAL_DATA {

    //
    // Timer base mapped (virtual) address
    //

    PULONG BaseAddressPtr;

    ULONG ClockSource;

    //
    // Frequency of the source clock, before any dividers
    //

    ULONG ClockSourceFrequency;

    //
    // The source clock should be divided by this value
    //

    ULONG FrequencyScale;

} IMX6_GPT_INTERNAL_DATA, *PIMX6_GPT_INTERNAL_DATA;

typedef struct _IMX6_EPIT_INTERNAL_DATA {

    //
    // Timer base mapped (virtual) address
    //

    PULONG BaseAddressPtr;

    //
    // Current EPIT Control Register
    //

    EPIT_CR EpitCR;

    //
    // Current Timer Period. If not 0, we're in periodic mode
    //

    ULONG PeriodInTicks;

    //
    // Clock source
    //

    ULONG ClockSource;

} IMX6_EPIT_INTERNAL_DATA, *PIMX6_EPIT_INTERNAL_DATA;

//
// ----------------------------------------------- Internal Function Prototypes
//

static
ULONG
ReadTimerReg(
    PVOID BaseAddressPtr,
    ULONG Reg
    );

static
void
WriteTimerReg(
    PVOID BaseAddressPtr,
    ULONG Reg,
    ULONG Value
    );

//
// GPT Timer Functions
//

static
NTSTATUS
HalpGptTimerRegister(
    ULONG Handle,
    PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
    PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE CsrtTimerDescPtr
    );

static
NTSTATUS
HalpGptTimerInitialize(
    __in PVOID TimerDataPtr
    );

static
ULONGLONG
HalpGptTimerQueryCounter(
    __in PVOID TimerDataPtr
    );

//
// EPIT Timer Functions
//

static
NTSTATUS
HalpEpitTimerRegister(
    ULONG Handle,
    PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
    PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE CsrtTimerDescPtr
    );

static
NTSTATUS
HalpEpitTimerInitialize(
    __in PVOID TimerDataPtr
    );

static
NTSTATUS
HalpEpitTimerArm(
    __in PVOID TimerDataPtr,
    __in TIMER_MODE Mode,
    __in ULONGLONG TickCount
    );

static
VOID
HalpEpitTimerAcknowledgeInterrupt(
    __in PVOID TimerDataPtr
    );

static
VOID
HalpEpitTimerStop(
    __in PVOID TimerDataPtr
    );

//
// ------------------------------------------------------------------ Functions
//

FORCEINLINE
ULONG
ReadTimerReg(
    PVOID BaseAddressPtr,
    ULONG Reg
    )
{
    ULONG* RegAddressPtr;

    RegAddressPtr = (ULONG*)((PUCHAR)BaseAddressPtr + Reg);

    return READ_REGISTER_NOFENCE_ULONG(RegAddressPtr);
}

FORCEINLINE
void
WriteTimerReg(
    PVOID BaseAddressPtr,
    ULONG Reg,
    ULONG Value
    )
{
    ULONG* RegAddressPtr;

    RegAddressPtr = (ULONG*)((PUCHAR)BaseAddressPtr + Reg);

    WRITE_REGISTER_NOFENCE_ULONG(RegAddressPtr, Value);
}

NTSTATUS
HalpGptTimerRegister(
    ULONG Handle,
    PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
    PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE CsrtTimerDescPtr
    )
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;
    IMX6_GPT_INTERNAL_DATA InternalData;
    TIMER_INITIALIZATION_BLOCK NewTimer;

    if (CsrtTimerDescPtr->FrequencyScale > GPT_PRESCALER_MAX) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&InternalData, sizeof(InternalData));
    InternalData.ClockSource = CsrtTimerDescPtr->ClockSource;
    InternalData.ClockSourceFrequency = CsrtTimerDescPtr->Frequency;
    InternalData.FrequencyScale = CsrtTimerDescPtr->FrequencyScale;

    //
    // Register this physical address space with the HAL.
    //

    PhysicalAddress.QuadPart = (LONGLONG)CsrtTimerDescPtr->BaseAddress;
    Status = HalRegisterPermanentAddressUsage(PhysicalAddress, 0x1000);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Map IO space
    //

    InternalData.BaseAddressPtr = HalMapIoSpace(PhysicalAddress,
                                                0x1000,
                                                MmNonCached);
    if (InternalData.BaseAddressPtr == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the timer registration structure.
    //

    RtlZeroMemory(&NewTimer, sizeof(NewTimer));
    INITIALIZE_TIMER_HEADER(&NewTimer);

    NewTimer.FunctionTable.Initialize = HalpGptTimerInitialize;
    NewTimer.FunctionTable.QueryCounter = HalpGptTimerQueryCounter;

    NewTimer.InternalData = &InternalData;
    NewTimer.InternalDataSize = sizeof(InternalData);
    NewTimer.CounterBitWidth = 32;
    NewTimer.CounterFrequency =
        CsrtTimerDescPtr->Frequency / CsrtTimerDescPtr->FrequencyScale;

    NewTimer.MaxDivisor = 1;
    NewTimer.Capabilities = TIMER_COUNTER_READABLE;
    NewTimer.KnownType = TimerUnknown;
    NewTimer.Identifier = CsrtTimerDescPtr->Header.Uid;

    Status = RegisterResourceDescriptor(Handle,
                                        ResourceGroup,
                                        &CsrtTimerDescPtr->Header,
                                        &NewTimer);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
HalpGptTimerInitialize (
    __in PVOID TimerDataPtr
    )

/*++

Routine Description:

    This routine initializes the GPT i.MX61 timer hardware.

Arguments:

    TimerDataPtr - Supplies a pointer to the timer's internal data.

Return Value:

    Status code.

--*/

{
    PIMX6_GPT_INTERNAL_DATA TimerPtr;
    PULONG BaseAddressPtr;
    GPT_CR GptCR;
    GPT_PR GptPR;
    GPT_IR GptIR;
    GPT_SR GptSR;
    UINT32 ResetRetry;
    UINT32 CpuRev;
    IMX_CPU CpuType;
    NTSTATUS Status;

    TimerPtr = (IMX6_GPT_INTERNAL_DATA *)TimerDataPtr;
    BaseAddressPtr = TimerPtr->BaseAddressPtr;

    //
    // Disable GPT
    //

    GptCR.Dword = ReadTimerReg(BaseAddressPtr, GptControlReg);
    GptCR.Bits.EN = 0;
    WriteTimerReg(BaseAddressPtr, GptControlReg, GptCR.Dword);

    //
    // Disable interrupts
    //

    GptIR.Dword = ReadTimerReg(BaseAddressPtr, GptInterruptReg);
    GptIR.Bits2.AllInterrupts = 0;
    WriteTimerReg(BaseAddressPtr, GptInterruptReg, GptIR.Dword);

    //
    // Configure output mode to disconnected
    //

    GptCR.Bits.OM1 = 0;
    GptCR.Bits.OM2 = 0;
    GptCR.Bits.OM3 = 0;
    WriteTimerReg(BaseAddressPtr, GptControlReg, GptCR.Dword);

    //
    // Disable input capture mode
    //

    GptCR.Bits.IM1 = 0;
    GptCR.Bits.IM2 = 0;
    WriteTimerReg(BaseAddressPtr, GptControlReg, GptCR.Dword);

    //
    // Do a software reset
    //

    GptCR.Bits.SWR = 1;
    WriteTimerReg(BaseAddressPtr, GptControlReg, GptCR.Dword);

    //
    // Wait for reset the complete.
    // We cannot use a stall function since it may have a dependency
    // on this timer.
    //

    for (ResetRetry = 0;
         ResetRetry < GPT_RESET_DONE_MAX_RETRY;
         ResetRetry += 1) {

        GptCR.Dword = ReadTimerReg(BaseAddressPtr, GptControlReg);
        if (GptCR.Bits.SWR == 0) {
            break;
        }
    }

    if (GptCR.Bits.SWR != 0) {
        return STATUS_DEVICE_NOT_READY;
    }

    //
    // Ack all pending interrupts
    //

    GptSR.Dword = ReadTimerReg(BaseAddressPtr, GptStatusReg);
    GptSR.Bits2.AllInterrupts = 0x3F;
    WriteTimerReg(BaseAddressPtr, GptStatusReg, GptSR.Dword);

    //
    // Set the pre-scaler to get the target frequency
    //

    GptPR.Dword = ReadTimerReg(BaseAddressPtr, GptPreScalerReg);

    //
    // iMX6 DualLite requires both parts of PR register
    // to be populated for timer to function correctly
    //
    Status = ImxGetCpuRev(&CpuRev);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    CpuType = IMX_CPU_TYPE(CpuRev);

    if ((CpuType == IMX_CPU_MX6SOLO) ||
        (CpuType == IMX_CPU_MX6DL)) {

        // prescaler24 divider is limited to a maximum of 16. To divide
        // 24MHz by 24 we need to split divider into two parts -
        // for upper(prescaler24) and for lower(prescaler) bits.
        // Example 24 = 12(upper) * 2(lower)

        if (TimerPtr->FrequencyScale > 16) {

            UINT32 Divider = TimerPtr->FrequencyScale / 16 + 1;

            GptPR.Dword = (TimerPtr->FrequencyScale / Divider - 1)
                            << 12;                  // bits 12-15 PRESCALER24M
            GptPR.Bits.PRESCALER = Divider - 1;     // bits 0-11 PRESCALER
        } else {

            GptPR.Dword = (TimerPtr->FrequencyScale - 1) << 12;     // bits 12-15 PRESCALER24M
            GptPR.Bits.PRESCALER = 0;
        }
    } else {
        GptPR.Bits.PRESCALER = TimerPtr->FrequencyScale - 1;    // bits 0-11 PRESCALER
    }

    WriteTimerReg(BaseAddressPtr, GptPreScalerReg, GptPR.Dword);

    //
    // Select clock source.
    // Despite what the documentation says, SWR resets all
    // registers to their default values, thus we do all settings after
    // reset was successfully completed.
    //

    GptCR.Bits.CLKSRC = TimerPtr->ClockSource;
    WriteTimerReg(BaseAddressPtr, GptControlReg, GptCR.Dword);

    //
    // Enable GPT
    //

    GptCR.Bits.EN = 1;      // Enable GPT
    GptCR.Bits.ENMOD = 1;   // Counter and pre-scaler are reset when GPT is enabled.
    GptCR.Bits.STOPEN = 1;  // GPT is enabled in 'stop mode'
    GptCR.Bits.WAITEN = 1;  // GPT is enabled in 'wait mode'
    GptCR.Bits.DOZEEN = 1;  // GPT is enabled in 'doze mode'
    GptCR.Bits.DBGEN = 1;   // GPT is enabled in 'debug mode'
    GptCR.Bits.FRR = 1;     // Freerun mode.

    switch (CpuType) {
    case IMX_CPU_MX6SL:
    case IMX_CPU_MX6DL:
    case IMX_CPU_MX6SX:
    case IMX_CPU_MX6UL:
    case IMX_CPU_MX6ULL:
    case IMX_CPU_MX6SOLO:
    case IMX_CPU_MX6SLL:
        GptCR.Bits.EN_24M = 1;  // Enable 24MHz crystal oscillator
        break;

    case IMX_CPU_MX6Q:
    case IMX_CPU_MX6D:
    case IMX_CPU_MX6DP:
    case IMX_CPU_MX6QP:
    default:
        break;
    }

    WriteTimerReg(BaseAddressPtr, GptControlReg, GptCR.Dword);

    return STATUS_SUCCESS;
}

ULONGLONG
HalpGptTimerQueryCounter (
    __in PVOID TimerDataPtr
    )

/*++

Routine Description:

    This routine queries the GPT timer hardware and retrieves the current
    counter value.

Arguments:

    TimerDataPtr - Supplies a pointer to the timer's internal data.

Return Value:

    Returns the hardware's current count.

--*/

{

    PIMX6_GPT_INTERNAL_DATA TimerPtr;

    TimerPtr = (PIMX6_GPT_INTERNAL_DATA)TimerDataPtr;

    return ReadTimerReg(TimerPtr->BaseAddressPtr, GptCounterReg);
}

NTSTATUS
HalpEpitTimerRegister(
    ULONG Handle,
    PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
    PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE CsrtTimerDescPtr
    )
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;
    IMX6_EPIT_INTERNAL_DATA InternalData;
    TIMER_INITIALIZATION_BLOCK NewTimer;

    if (CsrtTimerDescPtr->FrequencyScale != 1) {
        NT_ASSERT(!"Frequency scaling not supported for EPIT");
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&InternalData, sizeof(InternalData));
    InternalData.ClockSource = CsrtTimerDescPtr->ClockSource;

    //
    // Register this physical address space with the HAL.
    //

    PhysicalAddress.QuadPart = (LONGLONG)CsrtTimerDescPtr->BaseAddress;
    Status = HalRegisterPermanentAddressUsage(PhysicalAddress, 0x1000);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Map IO space
    //

    InternalData.BaseAddressPtr = HalMapIoSpace(PhysicalAddress,
                                                0x1000,
                                                MmNonCached);
    if (InternalData.BaseAddressPtr == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the timer registration structure.
    //

    RtlZeroMemory(&NewTimer, sizeof(NewTimer));
    INITIALIZE_TIMER_HEADER(&NewTimer);

    //
    // Timer callback functions
    //
    NewTimer.FunctionTable.Initialize = HalpEpitTimerInitialize;
    NewTimer.FunctionTable.AcknowledgeInterrupt = HalpEpitTimerAcknowledgeInterrupt;
    NewTimer.FunctionTable.ArmTimer = HalpEpitTimerArm;
    NewTimer.FunctionTable.Stop = HalpEpitTimerStop;

    NewTimer.InternalData = &InternalData;
    NewTimer.InternalDataSize = sizeof(InternalData);
    NewTimer.CounterBitWidth = 32;
    NewTimer.CounterFrequency = CsrtTimerDescPtr->Frequency;
    NewTimer.MaxDivisor = 1;

    //
    // Timer is not always on, but Windows requires an always
    // on timer, so we must give it one. When an idle state
    // is implemented that causes this timer to stop,
    // the ALWAYS_ON capability will need to be removed, and a
    // suitable timer supplied.
    //
    NewTimer.Capabilities = TIMER_PERIODIC_CAPABLE |
                            TIMER_ONE_SHOT_CAPABLE  |
                            TIMER_GENERATES_LINE_BASED_INTERRUPTS |
                            TIMER_ALWAYS_ON;

    NewTimer.Interrupt.Mode = LevelSensitive;
    NewTimer.Interrupt.Polarity = InterruptActiveHigh;
    NewTimer.Interrupt.Gsi = CsrtTimerDescPtr->Interrupt;
    NewTimer.KnownType = TimerUnknown;
    NewTimer.Identifier = CsrtTimerDescPtr->Header.Uid;

    Status = RegisterResourceDescriptor(Handle,
                                        ResourceGroup,
                                        &CsrtTimerDescPtr->Header,
                                        &NewTimer);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
HalpEpitTimerInitialize (
    __in PVOID TimerDataPtr
    )

/*++

Routine Description:

    This routine initializes the EPIT1/2 i.MX61 timer hardware.

Arguments:

    TimerDataPtr - Supplies a pointer to the timer's internal data.

Return Value:

    Status code.

--*/

{
    PIMX6_EPIT_INTERNAL_DATA TimerPtr;
    PULONG BaseAddressPtr;
    EPIT_CR EpitCR;
    EPIT_SR EpitSR;

    TimerPtr = (PIMX6_EPIT_INTERNAL_DATA)TimerDataPtr;
    BaseAddressPtr = TimerPtr->BaseAddressPtr;

    //
    // Disable EPITx
    //
    WriteTimerReg(BaseAddressPtr, EpitControlReg, 0);

    ///
    // Set EPITx configuration
    //
    EpitCR.Dword = 0;
    EpitCR.Bits.STOPEN = 1;     // EPIT is enabled in 'stop mode'
    EpitCR.Bits.WAITEN = 1;     // EPIT is enabled in 'wait mode'
    EpitCR.Bits.DBGEN = 1;      // EPIT is enabled in 'debug mode'
    EpitCR.Bits.IOVW = 1;       // Overwrite enable
    EpitCR.Bits.PRESCALAR = 0;  // Pre-scaler: divide by one (0)
    EpitCR.Bits.RLD = 1;        // Reload from LR register
    EpitCR.Bits.ENMOD = 1;      // Start from 0xffffffff
    EpitCR.Bits.CLKSRC = TimerPtr->ClockSource;

    TimerPtr->EpitCR = EpitCR;
    WriteTimerReg(BaseAddressPtr, EpitControlReg, EpitCR.Dword);

    //
    // ACK any pending interrupts
    //
    EpitSR.Dword = 0;
    EpitSR.Bits.OCIF = 1;
    WriteTimerReg(BaseAddressPtr, EpitStatusReg, EpitSR.Dword);

    //
    // Initialize load register and counter value
    //
    WriteTimerReg(BaseAddressPtr, EpitLoadReg, 0xffffffff);

    //
    // We want the interrupt to assert when the counter reaches 0
    //
    WriteTimerReg(BaseAddressPtr, EpitCompareReg, 0);

    return STATUS_SUCCESS;
}

NTSTATUS
HalpEpitTimerArm (
    __in PVOID TimerDataPtr,
    __in TIMER_MODE Mode,
    __in ULONGLONG TickCount
    )

/*++

Routine Description:

    This routine arms an EPITx timer to fire an interrupt after a
    given period of time.

Arguments:

    TimerDataPtr - Supplies a pointer to the timer's internal data.

    Mode - Supplies the desired mode to arm the timer with (pseudo-periodic or
        one-shot).

    TickCount - Supplies the number of ticks from now that the timer should
        interrupt in.

Return Value:

    STATUS_SUCCESS on success.

    STATUS_INVALID_PARAMETER if too large a tick count or an invalid mode was
        supplied.

    STATUS_UNSUCCESSFUL if the hardware could not be programmed.

--*/

{
    PIMX6_EPIT_INTERNAL_DATA TimerPtr;
    PULONG BaseAddressPtr;
    EPIT_CR EpitCR;

    TimerPtr = (PIMX6_EPIT_INTERNAL_DATA)TimerDataPtr;
    BaseAddressPtr = TimerPtr->BaseAddressPtr;

    //
    // Ensured by timer width
    //
    NT_ASSERT(TickCount < 0xFFFFFFFF);

    //
    // If we're in periodic mode, save the tick count so we can
    // program the next match value from AcknowledgeInterrupt.
    //
    switch (Mode) {
    case TimerModePeriodic:
        TimerPtr->PeriodInTicks = (ULONG)TickCount;
        break;
    case TimerModeOneShot:
        TimerPtr->PeriodInTicks = 0;
        break;
    default:
        NT_ASSERT(!"Invalid timer mode");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Program load register with desired tick count
    // This will also load the counter with this value.
    //
    WriteTimerReg(BaseAddressPtr, EpitLoadReg, (ULONG)TickCount);

    //
    // Enable interrupts and counter
    //
    EpitCR = TimerPtr->EpitCR;
    EpitCR.Bits.OCIEN = 1;
    EpitCR.Bits.EN = 1;
    WriteTimerReg(BaseAddressPtr, EpitControlReg, EpitCR.Dword);

    return STATUS_SUCCESS;
}

VOID
HalpEpitTimerAcknowledgeInterrupt (
    __in PVOID TimerDataPtr
    )

/*++

Routine Description:

    This routine acknowledges a EPITx timer interrupt.

Arguments:

    TimerData - Supplies a pointer to the timer's internal data.

Return Value:

    None.

--*/

{
    PIMX6_EPIT_INTERNAL_DATA TimerPtr;
    PULONG BaseAddressPtr;

    TimerPtr = (PIMX6_EPIT_INTERNAL_DATA)TimerDataPtr;
    BaseAddressPtr = TimerPtr->BaseAddressPtr;

    //
    // If we're in one-shot mode, stop the timer
    //

    if (TimerPtr->PeriodInTicks == 0) {

        WriteTimerReg(BaseAddressPtr, EpitControlReg, TimerPtr->EpitCR.Dword);
    }

    //
    // ACK pending interrupts
    //

    WriteTimerReg(BaseAddressPtr, EpitStatusReg, 0x1);

    return;
}

VOID
HalpEpitTimerStop (
    __in PVOID TimerDataPtr
    )

/*++

Routine Description:

    This routine stops the EPITx timer from generating interrupts.

Arguments:

    TimerDataPtr - Supplies a pointer to the timer's internal data.

Return Value:

    None.

--*/

{
    PIMX6_EPIT_INTERNAL_DATA TimerPtr;
    PULONG BaseAddressPtr;
    EPIT_SR EpitSR;

    TimerPtr = (PIMX6_EPIT_INTERNAL_DATA)TimerDataPtr;
    BaseAddressPtr = TimerPtr->BaseAddressPtr;

    //
    // Disable interrupts and counter
    //

    WriteTimerReg(BaseAddressPtr, EpitControlReg, TimerPtr->EpitCR.Dword);

    //
    // ACK any pending interrupts
    //

    EpitSR.Dword = 0;
    EpitSR.Bits.OCIF = 1;
    WriteTimerReg(BaseAddressPtr, EpitStatusReg, EpitSR.Dword);

    return;
}

NTSTATUS
AddResourceGroup (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    )

/*++

Routine Description:

    This function registers the iMX6 GPT/EPIT1/EPIT2 with the HAL.

Arguments:

    Handle - Supplies the HAL Extension handle which must be passed to other
        HAL Extension APIs.

    ResourceGroup - Supplies a pointer to the Resource Group which the
        HAL Extension has been installed on.

Return Value:

    NTSTATUS code.

--*/

{
    PCSRT_RESOURCE_DESCRIPTOR_HEADER ResourceHeader = NULL;
    PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE TimerResource;
    NTSTATUS Status;

    //
    // Loop through all timers in the CSRT
    //
    for (;;) {

        ResourceHeader = GetNextResourceDescriptor(Handle,
                                                   ResourceGroup,
                                                   ResourceHeader,
                                                   CSRT_RD_TYPE_TIMER,
                                                   CSRT_RD_SUBTYPE_TIMER,
                                                   CSRT_RD_UID_ANY);

        if (ResourceHeader == NULL)
            break;

        if (ResourceHeader->Length !=
            sizeof(CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE)) {

            NT_ASSERT(!"Invalid CSRT descriptor");
            return STATUS_INVALID_PARAMETER;
        }

        TimerResource =
            (PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE)ResourceHeader;

        switch (TimerResource->TimerType) {
        case IMX_TIMER_TYPE_GPT:
            Status = HalpGptTimerRegister(Handle,
                                          ResourceGroup,
                                          TimerResource);
            break;
        case IMX_TIMER_TYPE_EPIT:
            Status = HalpEpitTimerRegister(Handle,
                                           ResourceGroup,
                                           TimerResource);
            break;
        default:
            NT_ASSERT(!"Unknown timer type!");
            return STATUS_INVALID_PARAMETER;
        }

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

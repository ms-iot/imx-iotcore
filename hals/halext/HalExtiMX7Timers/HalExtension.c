/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    HalExtension.c

Abstract:

    This file implements a HAL Extension Module for the i.MX7 Timers.

*/

//
// ------------------------------------------------------------------- Includes
//

#include <nthalext.h>
#include "iMX7Timers.h"

//
// ---------------------------------------------------------------- Definitions
//

//
// Timer control constants
//

#define GPT_RESET_DONE_MAX_RETRY 100    // Max retry counter for GPT reset complete

//
// Timer ID. Matches the CSRT resource.
//

typedef enum
{
    TIMER_TYPE_GPT = 0,

} IMX7_TIMER_TYPE;

//
// ------------------------------------------------- Data Structure Definitions
//

#pragma pack(push, 1)

//
// CSRT Timer descriptor
// Matches the CSRT resource descriptor (RD_TIMER).
//

// Timer descriptor
typedef struct _CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE {

    CSRT_RESOURCE_DESCRIPTOR_HEADER Header;

    UINT32 BaseAddress;         // Base address
    UINT32 BaseAddressSize;     // Base address size
    UINT32 Interrupt;           // IRQ
    UINT32 RefFrequency;        // Reference frequency
    IMX7_TIMER_TYPE TimerType;  // Timer Type

    // HAL Extension timer registration info
    UINT32 CounterBitWidth;     // Number of width in timer counter
    UINT64 CounterFrequency;    // Timer frequency in Hz
    UINT32 MaxDivisor;          // Stores the highest supported counter divisor 
    UINT32 Capabilities;        // Timer capabilities

    // GPT specific
    UINT32 ClockSource;         // Clock source
    UINT32 FreqScaler;          // Frequency scaler
    UINT32 FreqScaler24M;       // Frequency scaler 24M

} CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE, *PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE;

#pragma pack(pop)

//
// iMX6 timer descriptor, used for registering
// exposed timers.
//

typedef struct _IMX7_TIMER_DESCRIPTOR {

    //
    // Timer ID
    //

    IMX7_TIMER_TYPE                 Id;

    //
    // Timer functions
    //

    PTIMER_INITIALIZE               InitializePtr;
    PTIMER_QUERY_COUNTER            QueryCounterPtr;
    PTIMER_ACKNOWLEDGE_INTERRUPT    AcknowledgeInterruptPtr;
    PTIMER_ARM_TIMER                ArmTimerPtr;
    PTIMER_STOP                     StopPtr;

} IMX7_TIMER_DESCRIPTOR, *PIMX7_TIMER_DESCRIPTOR;

//
// iMX6 runtime timer internal data, that is
// passed by the framework to each timer related callback routine.
//
typedef struct _IMX7_TIMER_INTERNAL_DATA {

    //
    // The CSRT descriptor
    //

    CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE CsrtDesc;

    //
    // Timer base mapped (virtual) address
    //

    PULONG BaseAddressPtr;

    //
    // Current timer mode
    //

    TIMER_MODE Mode;

} IMX7_TIMER_INTERNAL_DATA, *PIMX7_TIMER_INTERNAL_DATA;

//
// ----------------------------------------------- Internal Function Prototypes
//

NTSTATUS
HalpGptTimerInitialize(
    __in PVOID TimerDataPtr
    );
    
ULONGLONG
HalpGptTimerQueryCounter(
    __in PVOID TimerDataPtr
    );

VOID
HalpGptTimerAcknowledgeInterrupt(
    __in PVOID TimerDataPtr
    );

NTSTATUS
HalpGptTimerArm(
    __in PVOID TimerDataPtr,
    __in TIMER_MODE Mode,
    __in ULONGLONG TickCount
    );

VOID
HalpGptTimerStop(
    __in PVOID TimerDataPtr
    );
    

//
// -------------------------------------------------------------------- Globals
//

//
// The list of supported timer descriptors. The array index must match value
// defined IMX7_TIMER_TYPE enum.
//

static IMX7_TIMER_DESCRIPTOR HalpIMX7TimerDescriptors[] = {

    //
    // GPT timer:
    // A 32 bit GPT up counter, which cannot generate
    // interrupts and cannot be stopped.
    //

    {
        TIMER_TYPE_GPT,              // Timer ID

        //
        // Timer functions
        //

        HalpGptTimerInitialize,             // Initialize handler
        HalpGptTimerQueryCounter,           // QueryCounter handler
        HalpGptTimerAcknowledgeInterrupt,   // AcknowledgeInterrupt handler
        HalpGptTimerArm,                    // ArmTimer handler
        HalpGptTimerStop,                   // Stop handler
    },
};


//
// ------------------------------------------------------------------ Functions
//


FORCEINLINE
ULONG
ReadTimerReg(
    __in PIMX7_TIMER_INTERNAL_DATA TimerPtr,
    __in ULONG Reg
    )
{

    volatile ULONG* RegAddressPtr;

    RegAddressPtr = (volatile ULONG*)((PUCHAR)(TimerPtr->BaseAddressPtr) + Reg);

    return READ_REGISTER_ULONG(RegAddressPtr);
}


FORCEINLINE
VOID
WriteTimerReg(
    __in PIMX7_TIMER_INTERNAL_DATA TimerPtr,
    __in ULONG Reg, ULONG Value
    )
{

    volatile ULONG* RegAddressPtr;

    RegAddressPtr = (volatile ULONG*)((PUCHAR)(TimerPtr->BaseAddressPtr) + Reg);

    WRITE_REGISTER_ULONG(RegAddressPtr, Value);
}

NTSTATUS
AddResourceGroup(
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    )

/*++

Routine Description:

    This function registers the iMX7 GPT with HAL.

Arguments:

    Handle - Supplies the HAL Extension handle which must be passed to other
        HAL Extension APIs.

    ResourceGroup - Supplies a pointer to the Resource Group which the
        HAL Extension has been installed on.

Return Value:

    NTSTATUS code.

--*/

{

    PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE CsrtTimerDescPtr = NULL;
    IMX7_TIMER_INTERNAL_DATA InternalData;
    TIMER_INITIALIZATION_BLOCK NewTimer;
    PHYSICAL_ADDRESS PhysicalAddress;
    CSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptorHeader;
    NTSTATUS Status;
    PIMX7_TIMER_DESCRIPTOR TimerDescPtr;
    
    //
    // Loop through all timer resource
    //

    for (;;) {

        CsrtTimerDescPtr = (PCSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE)
            GetNextResourceDescriptor(Handle,
                ResourceGroup,
                (PCSRT_RESOURCE_DESCRIPTOR_HEADER)CsrtTimerDescPtr,
                CSRT_RD_TYPE_TIMER,
                CSRT_RD_SUBTYPE_TIMER,
                CSRT_RD_UID_ANY);

        if (CsrtTimerDescPtr == NULL) {
            break;
        }

        //
        // Retrieve the timer descriptor
        //

        TimerDescPtr = &HalpIMX7TimerDescriptors[CsrtTimerDescPtr->TimerType];

        //
        // Register this physical address space with the HAL.
        //

        PhysicalAddress.QuadPart = (LONGLONG)CsrtTimerDescPtr->BaseAddress;
        Status = HalRegisterPermanentAddressUsage(PhysicalAddress,
            CsrtTimerDescPtr->BaseAddressSize);
        if (!NT_SUCCESS(Status)) {
            goto done;
        }

        //
        // Initialize the timer internal data and copy over the resource
        // descriptors
        //

        RtlZeroMemory(&InternalData, sizeof(IMX7_TIMER_INTERNAL_DATA));
        RtlCopyMemory(&InternalData.CsrtDesc,
            CsrtTimerDescPtr,
            sizeof(CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE));

        //
        // Initialize the timer structure.
        //

        RtlZeroMemory(&NewTimer, sizeof(TIMER_INITIALIZATION_BLOCK));
        INITIALIZE_TIMER_HEADER(&NewTimer);
        NewTimer.InternalData = &InternalData;
        NewTimer.InternalDataSize = sizeof(IMX7_TIMER_INTERNAL_DATA);
        NewTimer.CounterBitWidth = CsrtTimerDescPtr->CounterBitWidth;
        NewTimer.KnownType = TimerUnknown;
        NewTimer.CounterFrequency = (ULONGLONG)
            (CsrtTimerDescPtr->CounterFrequency);
        NewTimer.MaxDivisor = CsrtTimerDescPtr->MaxDivisor;
        NewTimer.Capabilities = CsrtTimerDescPtr->Capabilities;

        //
        // Set generated interrupt information, if any...
        //

        if ((NewTimer.Capabilities & TIMER_GENERATES_LINE_BASED_INTERRUPTS) != 0) {

            NewTimer.Interrupt.Mode = LevelSensitive;
            NewTimer.Interrupt.Polarity = InterruptActiveHigh;
            NewTimer.Interrupt.Gsi = CsrtTimerDescPtr->Interrupt;
        }

        //
        // Timer callback functions
        //

        NewTimer.FunctionTable.Initialize = TimerDescPtr->InitializePtr;
        NewTimer.FunctionTable.QueryCounter = TimerDescPtr->QueryCounterPtr;
        NewTimer.FunctionTable.ArmTimer = TimerDescPtr->ArmTimerPtr;
        NewTimer.FunctionTable.Stop = TimerDescPtr->StopPtr;
        NewTimer.FunctionTable.AcknowledgeInterrupt =
            TimerDescPtr->AcknowledgeInterruptPtr;

        //
        // Register a timer.
        //

        ResourceDescriptorHeader.Type = CSRT_RD_TYPE_TIMER;
        ResourceDescriptorHeader.Subtype = CSRT_RD_SUBTYPE_TIMER;
        ResourceDescriptorHeader.Length = sizeof(CSRT_RESOURCE_DESCRIPTOR_HEADER);

        ResourceDescriptorHeader.Uid = (UINT32)CsrtTimerDescPtr->Header.Uid;
        Status = RegisterResourceDescriptor(Handle,
            ResourceGroup,
            &ResourceDescriptorHeader,
            &NewTimer);
        if (!NT_SUCCESS(Status)) {
            goto done;
        }

    }
    
    Status = STATUS_SUCCESS;

done:
    return Status;
}

NTSTATUS
HalpGptTimerInitialize(
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

    const CSRT_RESOURCE_DESCRIPTOR_TIMER_INSTANCE* CsrtTimerDescPtr;
    GPT_CR GptCR;
    GPT_PR GptPR;
    GPT_IR GptIR;
    GPT_SR GptSR;
    PHYSICAL_ADDRESS PhysicalAddress;
    UINT32 ResetRetry;
    NTSTATUS Status;
    IMX7_TIMER_INTERNAL_DATA* TimerPtr;

    TimerPtr = (PIMX7_TIMER_INTERNAL_DATA)TimerDataPtr;

    CsrtTimerDescPtr = &TimerPtr->CsrtDesc;

    NT_ASSERT(CsrtTimerDescPtr->TimerType == TIMER_TYPE_GPT);

    //
    // Map the timer hardware if it has not yet been mapped.
    //

    if (TimerPtr->BaseAddressPtr == NULL) {

        PhysicalAddress.QuadPart = (LONGLONG)CsrtTimerDescPtr->BaseAddress;

        TimerPtr->BaseAddressPtr = HalMapIoSpace(PhysicalAddress,
                                        CsrtTimerDescPtr->BaseAddressSize,
                                        MmNonCached);
        if (TimerPtr->BaseAddressPtr == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }
    }

    if (CsrtTimerDescPtr->FreqScaler > GPT_PRESCALER_MAX) {
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // Initialize GPT timer
    //
    //  Note:
    //    We assume the clock sources are turned on by UEFI.
    //

    //
    // Disable GPT
    //

    GptCR.Dword = ReadTimerReg(TimerPtr, GptControlReg);
    GptCR.Bits.EN = 0;
    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    //
    // Disable interrupts
    //

    GptIR.Dword = ReadTimerReg(TimerPtr, GptInterruptReg);
    GptIR.Bits2.AllInterrupts = 0;
    WriteTimerReg(TimerPtr, GptInterruptReg, GptIR.Dword);

    //
    // Configure output mode to disconnected
    //

    GptCR.Bits.OM1 = 0;
    GptCR.Bits.OM2 = 0;
    GptCR.Bits.OM3 = 0;
    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    //
    // Disable input capture mode
    //

    GptCR.Bits.IM1 = 0;
    GptCR.Bits.IM2 = 0;
    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    //
    // Do a software reset
    //

    GptCR.Bits.SWR = 1;
    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    //
    // Wait for reset the complete.
    // We cannot use a stall function since it may have a dependency
    // on this timer.
    //

    for (ResetRetry = 0;
         ResetRetry < GPT_RESET_DONE_MAX_RETRY;
         ResetRetry += 1) {

        GptCR.Dword = ReadTimerReg(TimerPtr, GptControlReg);
        if (GptCR.Bits.SWR == 0) {
            break;
        }
    }

    if (GptCR.Bits.SWR != 0) {
        Status =  STATUS_DEVICE_NOT_READY;
        goto done;
    }

    //
    // Ack all pending interrupts
    //

    GptSR.Dword = ReadTimerReg(TimerPtr, GptStatusReg);
    GptSR.Bits2.AllInterrupts = 0x3F;
    WriteTimerReg(TimerPtr, GptStatusReg, GptSR.Dword);

    //
    // Set the pre-scaler to get the target frequency
    //

    GptPR.Bits.PRESCALER24M = CsrtTimerDescPtr->FreqScaler24M;
    GptPR.Bits.PRESCALER = CsrtTimerDescPtr->FreqScaler;
    WriteTimerReg(TimerPtr, GptPreScalerReg, GptPR.Dword);

    //
    // Select clock source.
    // Despite what the documentation says, SWR resets all
    // registers to their default values, thus we do all settings after
    // reset was successfully completed.
    //

    GptCR.Bits.CLKSRC = CsrtTimerDescPtr->ClockSource;
    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    //
    // Enable GPT if it is readable
    //

    if (TimerPtr->CsrtDesc.Capabilities & TIMER_COUNTER_READABLE) {

        GptCR.Bits.EN = 1;      // Enable GPT
        GptCR.Bits.ENMOD = 1;   // Counter and pre-scaler are reset when GPT is enabled.
        GptCR.Bits.STOPEN = 1;  // GPT is enabled in 'stop mode'
        GptCR.Bits.WAITEN = 1;  // GPT is enabled in 'wait mode'
        GptCR.Bits.DOZEEN = 1;  // GPT is enabled in 'doze mode'
        GptCR.Bits.DBGEN = 1;   // GPT is enabled in 'debug mode'
        GptCR.Bits.FRR = 1;     // Freerun mode.
        GptCR.Bits.EN_24M = 1;

        WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);
    }

    Status = STATUS_SUCCESS;

done:

    NT_ASSERT(NT_SUCCESS(Status));

    return Status;
}


ULONGLONG
HalpGptTimerQueryCounter(
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

    PIMX7_TIMER_INTERNAL_DATA TimerPtr;

    TimerPtr = (PIMX7_TIMER_INTERNAL_DATA)TimerDataPtr;

    NT_ASSERT(TimerPtr->CsrtDesc.TimerType == TIMER_TYPE_GPT);

    return ReadTimerReg(TimerPtr, GptCounterReg);
}

VOID
HalpGptTimerAcknowledgeInterrupt(
    __in PVOID TimerDataPtr
    )
/*++

Routine Description:

    This routine acknowledges a GPT timer interrupt.

Arguments:

    TimerData - Supplies a pointer to the timer's internal data.

Return Value:

    None.

--*/
{
    GPT_IR GptIR;
    GPT_SR GptSR = { 0 };
    PIMX7_TIMER_INTERNAL_DATA TimerPtr;

    TimerPtr = (PIMX7_TIMER_INTERNAL_DATA)TimerDataPtr;

    NT_ASSERT(TimerPtr->CsrtDesc.TimerType == TIMER_TYPE_GPT);

    switch (TimerPtr->Mode)  {
    case TimerModeOneShot:

        //
        // Disable all interrupt. We don't disable counter as HAL
        // does not make any assumption if it is disable
        //

        GptIR.Dword = ReadTimerReg(TimerPtr, GptInterruptReg);
        GptIR.Bits2.AllInterrupts = 0;
        WriteTimerReg(TimerPtr, GptInterruptReg, GptIR.Dword);

        break;

    case TimerModePeriodic:

        break;

    default:

        NT_ASSERT(FALSE);

    }

    //
    // Acknowledge all interrupts
    //

    GptSR.Bits2.AllInterrupts = 0x3F;
    WriteTimerReg(TimerPtr, GptStatusReg, GptSR.Dword);

    return;
}

NTSTATUS
HalpGptTimerArm(
    __in PVOID TimerDataPtr,
    __in TIMER_MODE Mode,
    __in ULONGLONG TickCount
    )
/*++

Routine Description:

    This routine arms an GPT timer to fire an interrupt after a
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
    GPT_CR GptCR;
    GPT_IR GptIR;
    GPT_SR GptSR;
    PIMX7_TIMER_INTERNAL_DATA TimerPtr;

    TimerPtr = (PIMX7_TIMER_INTERNAL_DATA)TimerDataPtr;

    NT_ASSERT(TimerPtr->CsrtDesc.TimerType == TIMER_TYPE_GPT);
    NT_ASSERT((Mode == TimerModePeriodic) || (Mode == TimerModeOneShot));
    NT_ASSERT(TickCount > 0);    

    //
    // We only support 32 bit compare value
    //
    if (TickCount > 0xFFFFFFFF) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Disable GPT
    //

    GptCR.Dword = ReadTimerReg(TimerPtr, GptControlReg);
    GptCR.Bits.EN = 0;
    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    //
    // Disable and acknowledge any interrupt
    //

    GptIR.Dword = ReadTimerReg(TimerPtr, GptInterruptReg);
    GptIR.Bits2.AllInterrupts = 0;
    WriteTimerReg(TimerPtr, GptInterruptReg, GptIR.Dword);

    GptSR.Dword = ReadTimerReg(TimerPtr, GptStatusReg);
    GptSR.Bits2.AllInterrupts = 0;
    WriteTimerReg(TimerPtr, GptStatusReg, GptSR.Dword);

    //
    // Set compare1 registers
    //

    WriteTimerReg(TimerPtr, GptOutCompareReg1, (ULONG)(TickCount - 1));

    //
    // Enable compare1 interrupt
    //

    GptIR.Bits.OF1E = 1;
    WriteTimerReg(TimerPtr, GptInterruptReg, GptIR.Dword);

    //
    // Save the current mode so we can support one shot by manually disabling
    // interrupt once out compare interrupt is asserted
    //

    TimerPtr->Mode = Mode;
        
    //
    // Enable GPT
    //

    GptCR.Bits.EN = 1;      // Enable GPT
    GptCR.Bits.ENMOD = 1;   // Counter and pre-scaler are reset when GPT is enabled.
    GptCR.Bits.STOPEN = 1;  // GPT is enabled in 'stop mode'
    GptCR.Bits.WAITEN = 1;  // GPT is enabled in 'wait mode'
    GptCR.Bits.DOZEEN = 1;  // GPT is enabled in 'doze mode'
    GptCR.Bits.DBGEN = 1;   // GPT is enabled in 'debug mode'
    GptCR.Bits.FRR =        // Run as reset for periodic mode free run for one shot
        (Mode == TimerModePeriodic) ? 0 : 1;
    GptCR.Bits.EN_24M = 1;

    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    return STATUS_SUCCESS;
}

VOID
HalpGptTimerStop(
    __in PVOID TimerDataPtr
    )
/*++

Routine Description:

    This routine stops the GPT timer from generating interrupts.

Arguments:

    TimerDataPtr - Supplies a pointer to the timer's internal data.

Return Value:

    None.

--*/
{
    GPT_CR GptCR;
    GPT_IR GptIR;
    GPT_SR GptSR;
    PIMX7_TIMER_INTERNAL_DATA TimerPtr;

    TimerPtr = (PIMX7_TIMER_INTERNAL_DATA)TimerDataPtr;

    NT_ASSERT(TimerPtr->CsrtDesc.TimerType == TIMER_TYPE_GPT);

    //
    // Disable GPT
    //

    GptCR.Dword = ReadTimerReg(TimerPtr, GptControlReg);
    GptCR.Bits.EN = 0;
    WriteTimerReg(TimerPtr, GptControlReg, GptCR.Dword);

    //
    // Disable and acknowledge any interrupt
    //

    GptIR.Dword = ReadTimerReg(TimerPtr, GptInterruptReg);
    GptIR.Bits2.AllInterrupts = 0;
    WriteTimerReg(TimerPtr, GptInterruptReg, GptIR.Dword);

    GptSR.Dword = ReadTimerReg(TimerPtr, GptStatusReg);
    GptSR.Bits2.AllInterrupts = 0;
    WriteTimerReg(TimerPtr, GptStatusReg, GptSR.Dword);

    return;
}


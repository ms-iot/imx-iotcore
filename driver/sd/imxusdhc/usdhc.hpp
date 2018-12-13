// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   usdhc.hpp
//
// Abstract:
//
//  This module contains common enums, types, constants and definitions
//  for iMX Ultra Secure Digital Host Controller (uSDHC) miniport driver
//
// Environment:
//
//  Kernel mode only
//

#ifndef __USDHC_HPP__
#define __USDHC_HPP__

#pragma warning(push)
#pragma warning(disable:4201) // nameless struct/union

//
// Number of register maximum polls
//
#define USDHC_POLL_RETRY_COUNT              100000

//
// Waits between each registry poll
//
#define USDHC_POLL_WAIT_TIME_US             10

//
// Not supporting more than 1 request at a time
//
#define USDHC_MAX_OUTSTANDING_REQUESTS      1

//
// The error register in a standard SDHC is 16-bit, this
// mask used to select all errors
//
#define SDHC_ALL_STANDARD_ERRORS_MASK       0xFFFF

//
// In crashdump mode, calling into ACPI to get device properties is not possible
// Will assume a 198MHz uSDHC base clock on iMX platforms.
//
#define USDHC_DEFAULT_BASE_CLOCK_FREQ_HZ    198000000
#define USDHC_DEFAULT_SLOT_COUNT            1

//
// Delay in microsoeconds when setting the SDBUS power.
//
#define USDHC_CARD_STABILIZATION_DELAY 100000

//
// uSDHC Device Specific Method UUID
//
// {D4AC1EA1-BC53-416A-9B8C-481FEE75365C}
//
DEFINE_GUID(
    USDHC_DSM_GUID, 
    0xd4ac1ea1, 0xbc53, 0x416a, 0x9b, 0x8c, 0x48, 0x1f, 0xee, 0x75, 0x36, 0x5c);

//
// ACPI _DSM functions to control slot supply voltage On/Off
//
#define USDHC_DSM_FUNCTION_IDX_SLOT_POWER_ON     1
#define USDHC_DSM_FUNCTION_IDX_SLOT_POWER_OFF    2

#define USDHC_DSM_POWER_FUNCTIONS_MASK \
    ((1 << USDHC_DSM_FUNCTION_IDX_SLOT_POWER_ON) | (1 << USDHC_DSM_FUNCTION_IDX_SLOT_POWER_OFF))

//
// Current revision for uSDHC _DSM
//
#define USDHC_DSM_REVISION_ID    0

#include <pshpack1.h>

//
// Standard SDHC interrupt and error status register used for
// conversion between uSDHC and standard SDHC events/errors
// The below registers are defined as 32-bit instead of 16-bit
// to simplify conversion code since uSDHC registers are 32-bit
//
typedef union {
    UINT32 AsUint32;
    struct {
        UINT16 CommandComplete      : 1; // 0
        UINT16 TransferCompelte     : 1; // 1
        UINT16 BlockGapEvent        : 1; // 2
        UINT16 DmaInterrupt         : 1; // 3
        UINT16 BufferWriteReady     : 1; // 4
        UINT16 BufferReadReady      : 1; // 5
        UINT16 CardInsertion        : 1; // 6
        UINT16 CardRemoval          : 1; // 7
        UINT16 CardInterrupt        : 1; // 8
        UINT16 INT_A                : 1; // 9
        UINT16 INT_B                : 1; // 10
        UINT16 INT_C                : 1; // 11
        UINT16 ReTuningEvent        : 1; // 12
        UINT16 _reserved0           : 2; // 13:14
        UINT16 ErrorInterrupt       : 1; // 15
        UINT16 _reserved1;
    };
} SDHC_STANDARD_EVENTS_STATUS;

typedef union {
    UINT32 AsUint32;
    struct {
        UINT16 CommandTimeoutError  : 1; // 0
        UINT16 CommandCrcError      : 1; // 1
        UINT16 CommandEndBitError   : 1; // 2
        UINT16 CommandIndexError    : 1; // 3
        UINT16 DataTimeoutError     : 1; // 4
        UINT16 DataCrcError         : 1; // 5
        UINT16 DataEndBitError      : 1; // 6
        UINT16 CurrentLimitError    : 1; // 7
        UINT16 AutoCmdError         : 1; // 8
        UINT16 AdmaError            : 1; // 9
        UINT16 TuningError          : 1; // 10
        UINT16 _reserved0           : 5; // 11:15
        UINT16 _reserved1;
    };
} SDHC_STANDARD_ERROR_STATUS;

#include <poppack.h> // pshpack1.h

//
// SDHC Private Extension
//
typedef struct {
    volatile USDHC_REGISTERS* RegistersPtr;
    volatile USDHC_REGISTERS_DEBUG* DebugRegistersPtr;
    VOID* PhysicalAddress;
    SDPORT_CAPABILITIES Capabilities;
    UINT32 CurrentTransferRemainingLength;
    RECORDER_LOG IfrLogHandle; 
    BOOLEAN CrashdumpMode;
    BOOLEAN BreakOnDdiEnter;
    BOOLEAN BreakOnDdiExit;
    BOOLEAN BreakOnError;

    //
    // Information populated from ACPI
    //
    USDHC_DEVICE_PROPERTIES DeviceProperties;
} USDHC_EXTENSION;

//
// Sdhost SDHC miniport DDI/Callbacks
//

SDPORT_GET_SLOT_COUNT SdhcGetSlotCount;
SDPORT_CLEANUP SdhcCleanup;
SDPORT_GET_SLOT_CAPABILITIES SdhcSlotGetCapabilities;
SDPORT_INITIALIZE SdhcSlotInitialize;
SDPORT_ISSUE_BUS_OPERATION SdhcSlotIssueBusOperation;
SDPORT_INTERRUPT SdhcSlotInterrupt;
SDPORT_ISSUE_REQUEST SdhcSlotIssueRequest;
SDPORT_GET_RESPONSE SdhcSlotGetResponse;
SDPORT_REQUEST_DPC SdhcSlotRequestDpc;
SDPORT_SAVE_CONTEXT SdhcSlotSaveContext;
SDPORT_RESTORE_CONTEXT SdhcSlotRestoreContext;
SDPORT_TOGGLE_EVENTS SdhcSlotToggleEvents;
SDPORT_CLEAR_EVENTS SdhcSlotClearEvents;
SDPORT_GET_CARD_DETECT_STATE SdhcSlotGetCardDetectState;
SDPORT_GET_WRITE_PROTECT_STATE SdhcSlotGetWriteProtectState;

//
// Bus operations routines
//

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcResetHost(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_RESET_TYPE ResetType);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcSetClock(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ ULONG Frequency);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcSetBusWidth(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_BUS_WIDTH BusWidth);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcSetSpeed(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_BUS_SPEED Speed);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcSetSignaling(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ BOOLEAN Enable1V8Signaling);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcSetBusVoltage(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_BUS_VOLTAGE Voltage);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcSetSdBusPower(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ BOOLEAN PowerOn);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcExecuteTuning(
    _In_ USDHC_EXTENSION* SdhcExtPtr);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
SdhcEnableBlockGapInterrupt(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ BOOLEAN Enable);

//
// Interrupt helper routines
//

VOID
SdhcEnableInterrupt(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ UINT32 InterruptMask);

VOID
SdhcDisableInterrupt(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ UINT32 InterruptMask);

VOID
SdhcAcknowledgeInterrupts(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ UINT32 InterruptMask);

//
// Request helper routines
//

NTSTATUS
SdhcSendCommand(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_REQUEST* RequestPtr);

NTSTATUS
SdhcConfigureTransfer(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_REQUEST* RequestPtr);

NTSTATUS
SdhcStartTransfer(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_REQUEST* RequestPtr);

NTSTATUS
SdhcStartPioTransfer(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_REQUEST* RequestPtr);

NTSTATUS
SdhcStartAdmaTransfer(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_REQUEST* RequestPtr);

//
// General utility routines
//

VOID
SdhcReadDataPort(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _Out_writes_all_(WordCount) UINT32* BufferPtr,
    _In_ UINT32 WordCount);

VOID
SdhcWriteDataPort(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_reads_(WordCount) const UINT32* BufferPtr,
    _In_ UINT32 WordCount);

NTSTATUS
SdhcCreateAdmaDescriptorTable(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_REQUEST* RequestPtr);

UINT32
SdhcConvertStandardEventsToIntStatusMask(
    _In_ ULONG StdEventMask,
    _In_ ULONG StdErrorMask);

VOID
SdhcConvertIntStatusToStandardEvents(
    _In_ USDHC_INT_STATUS_REG IntStatus,
    _Out_ ULONG* StdEventMask,
    _Out_ ULONG* StdErrorMask);

NTSTATUS
SdhcConvertStandardErrorToStatus(
    _In_ ULONG ErrorMask);

VOID
SdhcCompleteRequest(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_REQUEST* RequestPtr,
    _In_ NTSTATUS Status);

NTSTATUS
SdhcWaitForStableSdClock(
    _In_ USDHC_EXTENSION* SdhcExtPtr);

VOID
SdhcSdClockEnableInIdle(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ BOOLEAN Enable);

_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
VOID
SdhcLogInit(
    _In_ USDHC_EXTENSION* SdhcExtPtr);

_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
VOID
SdhcLogCleanup(
    _In_ SD_MINIPORT* MiniportPtr);

//
// ACPI utilities
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SdhcReadDeviceProperties(
    _In_ DEVICE_OBJECT* PdoPtr,
    _Outptr_opt_ USDHC_DEVICE_PROPERTIES** DevPropsPtr);

BOOLEAN
SdhcIsSlotPowerControlSupported(
    _In_ DEVICE_OBJECT* PdoPtr);

__forceinline
DEVICE_OBJECT*
SdhcMiniportGetPdo(
    _In_ SD_MINIPORT* MiniportPtr
    )
{
    DEVICE_OBJECT* sdhcFdoPtr =
        static_cast<DEVICE_OBJECT*>(MiniportPtr->ConfigurationInfo.DeviceObject);
    NT_ASSERT(sdhcFdoPtr  != nullptr);
    DEVOBJ_EXTENSION* sdhcFdoExtPtr = sdhcFdoPtr ->DeviceObjectExtension;
    NT_ASSERT(sdhcFdoExtPtr);
    DEVICE_OBJECT* sdhcPdoPtr = sdhcFdoExtPtr->AttachedTo;
    NT_ASSERTMSG("SDHC PDO can't be null", sdhcPdoPtr);

    return sdhcPdoPtr;
}

//
// Register 32-bit access routines
//

__forceinline
UINT32
SdhcReadRegister(
    _In_ volatile UINT32* RegisterPtr
    )
{
    C_ASSERT(sizeof(UINT32) == sizeof(ULONG));
    volatile ULONG* regPtr = reinterpret_cast<volatile ULONG*>(RegisterPtr);
    return READ_REGISTER_ULONG(regPtr);
}

__forceinline
VOID
SdhcWriteRegister(
    _In_ volatile UINT32* RegisterPtr,
    _In_ UINT32 Value
    )
{
    C_ASSERT(sizeof(UINT32) == sizeof(ULONG));
    volatile ULONG* regPtr = reinterpret_cast<volatile ULONG*>(RegisterPtr);
    WRITE_REGISTER_ULONG(regPtr, Value);
}

__forceinline
UINT32
SdhcReadRegisterNoFence(
    _In_ volatile UINT32* RegisterPtr
    )
{
    C_ASSERT(sizeof(UINT32) == sizeof(ULONG));
    volatile ULONG* regPtr = reinterpret_cast<volatile ULONG*>(RegisterPtr);
    return READ_REGISTER_NOFENCE_ULONG(regPtr);
}

__forceinline
VOID
SdhcWriteRegisterNoFence(
    _In_ volatile UINT32* RegisterPtr,
    _In_ UINT32 Value
    )
{
    C_ASSERT(sizeof(UINT32) == sizeof(ULONG));
    volatile ULONG* regPtr = reinterpret_cast<volatile ULONG*>(RegisterPtr);
    WRITE_REGISTER_NOFENCE_ULONG(regPtr, Value);
}

extern "C" DRIVER_INITIALIZE DriverEntry;

#pragma warning(pop)

#endif // __USDHC_HPP__
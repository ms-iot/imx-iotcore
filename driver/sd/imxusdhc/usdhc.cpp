// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   usdhc.cpp
//
// Abstract:
//
//  This module contains methods implementation for SDHC miniport callbacks
//
// Environment:
//
//  Kernel mode only
//

#include "precomp.hpp"
#pragma hdrstop

#include "trace.hpp"
#include "usdhc.tmh"

#include "acpiutil.hpp"
#include "devpropslist.hpp"
#include "usdhchw.h"
#include "usdhc.hpp"

INIT_SEGMENT_BEGIN; //======================================================

//
// A driver wide log to track non SDHC instance specific events, i.e an event
// not associated with an SdhcExtPtr
//
// A default value of NULL IFR log handle will be indicate to WPP as no IFR 
// logging in case default log is unavailable
//
RECORDER_LOG DriverLogHandle = NULL;

ULONG32 gForcePio = 0;

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    DRIVER_OBJECT* DriverObjectPtr,
    UNICODE_STRING* RegistryPathPtr
    )
{
    //
    // Crashdump stack will call DriverEntry at IRQL >= DISPATCH_LEVEL, which is not
    // possible to initialize WPP at by design. WPP will be disabled during Crash dump
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        _SdhcDebugPrint(
            DPFLTR_INFO_LEVEL,
            _SdhcCrashdumpDebugLevel,
            "Skipping WPP initialization in Crashdump mode");
    } else {

        //
        // After WPP is enabled for the driver project, WPP creates a default log. The buffer
        // size of the default log to which WPP prints is 4096 bytes. For a debug build, the buffer is 24576 bytes.
        //
        WPP_INIT_TRACING(DriverObjectPtr, RegistryPathPtr);

        //
        // If the default log buffer fails to allocate, trace messages are sent to the WPP. That is, the trace 
        // messages are not recorded through the IFR, but the traces can still be seen as live WPP traces
        //
        if (WppRecorderIsDefaultLogAvailable())
        {
            RECORDER_CONFIGURE_PARAMS recorderConfig;
            RECORDER_CONFIGURE_PARAMS_INIT(&recorderConfig);
            WppRecorderConfigure(&recorderConfig);
            DriverLogHandle = WppRecorderLogGetDefault();
        }

#if DBG
        //
        // Enable verbose output for specific flags on debug build
        //
        // The trace definition file trace.hpp was written in such away that
        // emitting a trace is controlled by tracing level, except for TRACE_LEVEL_VERBOSE
        // for which emitting the trace is controlled by explicit enable property
        // AutoLogVerboseEnabled in the flag control block WPP_CONTROL(WPP_BIT_#FLAGS)
        //
        WPP_CONTROL(WPP_BIT_DEFAULT).AutoLogVerboseEnabled = TRUE;
        //
        // DDI Enter/Exit tracing is disabled by default on debug builds due to the
        // excess amount of info it emits. Enable on purpose to track SDHC miniport DDI 
        // calls closely
        //
        WPP_CONTROL(WPP_BIT_ENTEREXIT).AutoLogVerboseEnabled = FALSE;
#endif // DBG
    }

    USDHC_LOG_INFORMATION(
        DriverLogHandle,
        NullPrivateExtensionPtr,
        "DriverObjectPtr:0x%p",
        DriverObjectPtr);

    SDPORT_INITIALIZATION_DATA initData;

    RtlZeroMemory(&initData, sizeof(initData));
    initData.StructureSize = sizeof(initData);

    //
    // Initialize the entry points/callbacks for the miniport interface
    //

    initData.GetSlotCount = SdhcGetSlotCount;
    initData.GetSlotCapabilities = SdhcSlotGetCapabilities;
    initData.Initialize = SdhcSlotInitialize;
    initData.IssueBusOperation = SdhcSlotIssueBusOperation;
    initData.GetCardDetectState = SdhcSlotGetCardDetectState;
    initData.GetWriteProtectState = SdhcSlotGetWriteProtectState;
    initData.Interrupt = SdhcSlotInterrupt;
    initData.IssueRequest = SdhcSlotIssueRequest;
    initData.GetResponse = SdhcSlotGetResponse;
    initData.ToggleEvents = SdhcSlotToggleEvents;
    initData.ClearEvents = SdhcSlotClearEvents;
    initData.RequestDpc = SdhcSlotRequestDpc;
    initData.SaveContext = SdhcSlotSaveContext;
    initData.RestoreContext = SdhcSlotRestoreContext;
    initData.PowerControlCallback = nullptr;
    initData.Cleanup = SdhcCleanup;

    initData.PrivateExtensionSize = sizeof(USDHC_EXTENSION);
    initData.CrashdumpSupported = TRUE;

    //
    // Initialize the device properties bookkeeping data structures
    //
    // N.B. Decision made to remove Device Properties List structure from current
    //  release. Device Properties List was using _DSD to obtain information from
    //  the platform in a manner that violates the _DSD specification.
    //  However, the Device Properties List structure is necessary for future
    //  enhancements to this driver so we are leaving it in the code.
    //  Search for "DevicePropertiesList Code"
    //
#if 0   // DevicePropertiesList Code
    SdhcDevicePropertiesListInit();
#endif

    //
    // Hook up the IRP dispatch routines
    //
    NTSTATUS status = SdPortInitialize(DriverObjectPtr, RegistryPathPtr, &initData);
    if (!NT_SUCCESS(status)) {
        NT_ASSERTMSG("SdPortInitialize() failed", FALSE);
        return status;
    }

    return STATUS_SUCCESS;
}

INIT_SEGMENT_END; //======================================================
NONPAGED_SEGMENT_BEGIN; //======================================================

_Use_decl_annotations_
BOOLEAN
SdhcSlotInterrupt(
    PVOID PrivateExtensionPtr,
    PULONG EventsPtr,
    PULONG ErrorsPtr,
    PBOOLEAN CardChangePtr,
    PBOOLEAN SdioInterruptPtr,
    PBOOLEAN TuningPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");

    volatile USDHC_REGISTERS* registersPtr = sdhcExtPtr->RegistersPtr;
    USDHC_INT_STATUS_REG intStatus = { SdhcReadRegister(&registersPtr->INT_STATUS) };

    if (intStatus.AsUint32 & USDHC_INT_STATUS_ERROR) {
        USDHC_LOG_ERROR(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "INT_STATUS: CTOE:%lu CCE:%lu CEBE:%lu CIE:%lu DTOE:%lu DCE:%lu DEBE:%lu "
            "AC12E:%lu TNE:%lu DMAE:%lu CC:%lu TC:%lu BGE:%lu DINT:%lu BWR:%lu BRR:%lu "
            "CINS:%lu CRM:%lu CINT:%lu RTE:%lu TP:%lu",
            intStatus.CTOE,
            intStatus.CCE,
            intStatus.CEBE,
            intStatus.CIE,
            intStatus.DTOE,
            intStatus.DCE,
            intStatus.DEBE,
            intStatus.AC12E,
            intStatus.TNE,
            intStatus.DMAE,
            intStatus.CC,
            intStatus.TC,
            intStatus.BGE,
            intStatus.DINT,
            intStatus.BWR,
            intStatus.BRR,
            intStatus.CINS,
            intStatus.CRM,
            intStatus.CINT,
            intStatus.RTE,
            intStatus.TP);
    } else {
        USDHC_LOG_TRACE(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "INT_STATUS: CC:%lu TC:%lu BGE:%lu DINT:%lu "
            "BWR:%lu BRR:%lu CINS:%lu CRM:%lu CINT:%lu RTE:%lu TP:%lu",
            intStatus.CC,
            intStatus.TC,
            intStatus.BGE,
            intStatus.DINT,
            intStatus.BWR,
            intStatus.BRR,
            intStatus.CINS,
            intStatus.CRM,
            intStatus.CINT,
            intStatus.RTE,
            intStatus.TP);
    }

    *EventsPtr = 0;
    *ErrorsPtr = 0;
    *CardChangePtr = 0;
    *SdioInterruptPtr = 0;
    *TuningPtr = 0;

    //
    // If there aren't any events to handle, then we don't need to
    // process anything.
    //

    if (intStatus.AsUint32 == 0) {
        USDHC_LOG_INFORMATION(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,"Unexpected spurious interrupt");
        return FALSE;
    }

    //
    // If a card has changed, notify the port driver.
    //
    if (intStatus.CINS || intStatus.CRM) {
        *CardChangePtr = TRUE;
    }

    //
    // If we have an external SDIO interrupt, notify the port driver.
    //
    if (intStatus.CINT) {
        *SdioInterruptPtr = TRUE; 
    }

    //
    // If there's a tuning request, notify the port driver.
    //
    if (intStatus.RTE) {
        *TuningPtr = TRUE;
    }

    if (intStatus.DMAE) {
        USDHC_ADMA_ERR_STATUS_REG admaErrStatus = { SdhcReadRegister(&registersPtr->ADMA_ERR_STATUS) };
        UINT32 admaSysAddr = SdhcReadRegister(&registersPtr->ADMA_SYS_ADDR);
        USDHC_LOG_ERROR(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "DMA Error Detected ADMAES:%lu ADMALME:%lu ADMADCE:%lu ADMA_SYS_ADDR:0x%08X",
            admaErrStatus.ADMAES,
            admaErrStatus.ADMALME,
            admaErrStatus.ADMADCE,
            admaSysAddr);
    }

    if (intStatus.BRR || intStatus.BWR) {
        USDHC_INT_STATUS_REG intStatusEnableMask = { 0 };
        intStatusEnableMask.BRR = intStatus.BRR;
        intStatusEnableMask.BWR = intStatus.BWR;
        SdhcDisableInterrupt(sdhcExtPtr, intStatusEnableMask.AsUint32);
    }

    //
    // Acknowledge/clear interrupt status. Request completions will occur in
    // the port driver's slot completion DPC. We need to make the members of 
    // SDPORT_REQUEST that only the port driver uses opaque to the miniport.
    // See how Storport does this (if it does).
    //
    SdhcAcknowledgeInterrupts(sdhcExtPtr, intStatus.AsUint32);

    intStatus.CINS = 0;
    intStatus.CRM = 0;
    intStatus.CINT = 0;
    intStatus.RTE = 0;

    SdhcConvertIntStatusToStandardEvents(intStatus, EventsPtr, ErrorsPtr);

    BOOLEAN handled = (*EventsPtr != 0) ||
        (*ErrorsPtr != 0) ||
        (*CardChangePtr) ||
        (*SdioInterruptPtr) ||
        (*TuningPtr);

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "%!bool!", handled);
    return handled;
}

_Use_decl_annotations_
NTSTATUS
SdhcSlotIssueRequest(
    PVOID PrivateExtensionPtr,
    SDPORT_REQUEST *RequestPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(
        sdhcExtPtr->IfrLogHandle,
        sdhcExtPtr,
        "()");

    if (RequestPtr->Type == SdRequestTypeCommandNoTransfer) {
        
        //
        // Disable ASSERT for now until matching WDK fix is used
        //
        //NT_ASSERT(RequestPtr->Command.BlockCount == 0);
        //NT_ASSERT(RequestPtr->Command.Length == 0);
        
        RequestPtr->Command.BlockCount = 0;
        RequestPtr->Command.Length = 0;

        USDHC_LOG_INFORMATION(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "Type:%!REQUESTTYPE! %s%d(0x%08X)",
            RequestPtr->Type,
            (RequestPtr->Command.Class == SdCommandClassApp ? "ACMD" : "CMD"),
            RequestPtr->Command.Index,
            RequestPtr->Command.Argument);

    } else {
        USDHC_LOG_INFORMATION(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "Type:%!REQUESTTYPE! %s%d(0x%08X) %s(#Blocks:%lu, Length:%lu)",
            RequestPtr->Type,
            (RequestPtr->Command.Class == SdCommandClassApp ? "ACMD" : "CMD"),
            RequestPtr->Command.Index,
            RequestPtr->Command.Argument,
            (RequestPtr->Command.TransferDirection == SdTransferDirectionRead ? "Read" : "Write"),
            (UINT32)RequestPtr->Command.BlockCount,
            RequestPtr->Command.Length);
    }

    NTSTATUS status;

    //
    // Always initialize the request status with a known error value
    //
    RequestPtr->Status = STATUS_IO_DEVICE_ERROR;

    switch (RequestPtr->Type) {
    case SdRequestTypeCommandNoTransfer:
    case SdRequestTypeCommandWithTransfer:
        status = SdhcSendCommand(sdhcExtPtr, RequestPtr);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcSendCommand() failed");
        }
        break;

    case SdRequestTypeStartTransfer:
        status = SdhcStartTransfer(sdhcExtPtr, RequestPtr);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcStartTransfer() failed");
        }
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
        USDHC_LOG_ERROR(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "Unsupported request type %!REQUESTTYPE!",
            RequestPtr->Type);
    }

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "%!STATUS!", status);
    return status;
}

_Use_decl_annotations_
VOID
SdhcSlotGetResponse(
    PVOID PrivateExtensionPtr,
    SDPORT_COMMAND* CommandPtr,
    PVOID ResponseBufferPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");

    volatile USDHC_REGISTERS* registersPtr = sdhcExtPtr->RegistersPtr;

    switch (CommandPtr->ResponseType) {
    case SdResponseTypeR1:
    case SdResponseTypeR3:
    case SdResponseTypeR4:
    case SdResponseTypeR5:
    case SdResponseTypeR6:
    case SdResponseTypeR1B:
    case SdResponseTypeR5B:
    {
        UINT32* bufferPtr = static_cast<UINT32*>(ResponseBufferPtr);
        *bufferPtr = SdhcReadRegister(&registersPtr->CMD_RSP0);
        USDHC_LOG_INFORMATION(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "RSP[0]: %08X" , *bufferPtr);
    }
        break;

    case SdResponseTypeR2:
    {
        UINT32* bufferPtr = static_cast<UINT32*>(ResponseBufferPtr);
        bufferPtr[0] = SdhcReadRegister(&registersPtr->CMD_RSP0);
        bufferPtr[1] = SdhcReadRegister(&registersPtr->CMD_RSP1);
        bufferPtr[2] = SdhcReadRegister(&registersPtr->CMD_RSP2);
        bufferPtr[3] = SdhcReadRegister(&registersPtr->CMD_RSP3);

        USDHC_LOG_INFORMATION(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "RSP[0-3]: %08X, %08X, %08X, %08X",
            bufferPtr[0],
            bufferPtr[1],
            bufferPtr[2],
            bufferPtr[3]);
    }
        break;

    case SdResponseTypeNone:
        break;

    default:
        NT_ASSERTMSG("Unexpected response type value", FALSE);
    }

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
}

_Use_decl_annotations_
VOID
SdhcSlotRequestDpc(
    PVOID PrivateExtensionPtr,
    SDPORT_REQUEST* RequestPtr,
    ULONG Events,
    ULONG Errors
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(
        sdhcExtPtr->IfrLogHandle,
        sdhcExtPtr,
        "RequiredEvents:0x%08X Events:0x%08X Errors:0x%08X",
        RequestPtr->RequiredEvents,
        Events,
        Errors);

    //
    // Clear the request's required events if they have completed.
    //
    RequestPtr->RequiredEvents &= ~Events;

    //
    // If there are errors, we need to fail whatever outstanding request
    // was on the bus. Otherwise, the request succeeded.
    // 
    if (Errors) {
        RequestPtr->RequiredEvents = 0;
        NTSTATUS status = SdhcConvertStandardErrorToStatus(Errors);
        SdhcCompleteRequest(sdhcExtPtr, RequestPtr, status);

    } else if (RequestPtr->RequiredEvents == 0) {
        if (RequestPtr->Status != STATUS_MORE_PROCESSING_REQUIRED) {

            RequestPtr->Status = STATUS_SUCCESS;

            //
            // WORKAROUND: uSDHC does not fire TC interrupt as specified in the datasheet
            // for commands with Busy signal i.e R1b and R5b. To get around this and make sure
            // Busy signal is deasserted we poll on PRES_STATE.DLA which will be 0 once
            // the DATA line becomes inactive
            //
            if ((RequestPtr->Command.ResponseType == SdResponseTypeR1B) ||
                (RequestPtr->Command.ResponseType == SdResponseTypeR5B)) {

                volatile USDHC_REGISTERS* registersPtr = sdhcExtPtr->RegistersPtr;
                USDHC_PRES_STATE_REG presState = { SdhcReadRegister(&registersPtr->PRES_STATE) };
                UINT32 retries = USDHC_POLL_RETRY_COUNT;

                while (presState.DLA && retries) {
                    SdPortWait(USDHC_POLL_WAIT_TIME_US);
                    presState.AsUint32 = SdhcReadRegister(&registersPtr->PRES_STATE);
                    --retries;
                }

                if (presState.DLA) {
                    NT_ASSERT(!retries);
                    USDHC_LOG_ERROR(
                        sdhcExtPtr->IfrLogHandle,
                        sdhcExtPtr,
                        "Time-out waiting on DAT0 to get released");
                    RequestPtr->Status = STATUS_IO_TIMEOUT;
                }
            }
        }

        SdhcCompleteRequest(sdhcExtPtr, RequestPtr, RequestPtr->Status);
    }

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
}

_Use_decl_annotations_
VOID
SdhcSlotSaveContext(
    PVOID PrivateExtensionPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
}

_Use_decl_annotations_
VOID
SdhcSlotRestoreContext(
    PVOID PrivateExtensionPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
}

_Use_decl_annotations_
VOID
SdhcSlotToggleEvents(
    PVOID PrivateExtensionPtr,
    ULONG EventMask,
    BOOLEAN Enable
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(
        sdhcExtPtr->IfrLogHandle,
        sdhcExtPtr,
        "EventMask:0x%08X, Enable:%!bool!",
        EventMask,
        Enable);

    UINT32 InterruptMask = SdhcConvertStandardEventsToIntStatusMask(
        EventMask, 
        SDHC_ALL_STANDARD_ERRORS_MASK);
    if (Enable) {
        SdhcEnableInterrupt(sdhcExtPtr, InterruptMask);
    } else {
        SdhcDisableInterrupt(sdhcExtPtr, InterruptMask);
    }

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
}

_Use_decl_annotations_
VOID
SdhcSlotClearEvents(
    PVOID PrivateExtensionPtr,
    ULONG EventMask
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(
        sdhcExtPtr->IfrLogHandle,
        sdhcExtPtr,
        "EventMask:0x%08X",
        EventMask);
    
    UINT32 Interrupts =
        SdhcConvertStandardEventsToIntStatusMask(
            EventMask,
            SDHC_ALL_STANDARD_ERRORS_MASK);
    SdhcAcknowledgeInterrupts(sdhcExtPtr, Interrupts);

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
}

_Use_decl_annotations_
VOID
SdhcCleanup(
    SD_MINIPORT* MiniportPtr
    )
{
    USDHC_DDI_ENTER(
        DriverLogHandle,
        NullPrivateExtensionPtr,
        "()");

#if 0   //DevicePropertiesList Code
    USDHC_DEVICE_PROPERTIES* devPropsPtr =
        DevicePropertiesListSafeFindByPdo(SdhcMiniportGetPdo(MiniportPtr));
    NT_ASSERTMSG("Device properties entry should always exist", devPropsPtr != nullptr);
    if (devPropsPtr != nullptr) {
        //
        // The current design assumes and supports only 1 Slot. Which implies that for each
        // uSDHC instances, there will be 1 device properties entry and 1 slot
        //
        NT_ASSERT(MiniportPtr->SlotCount == 1);
        (VOID)DevicePropertiesListSafeRemoveByKey(devPropsPtr->Key);
    }
#endif

    SdhcLogCleanup(MiniportPtr);
    WPP_CLEANUP(NULL);
}

_Use_decl_annotations_
VOID
SdhcEnableInterrupt(
    USDHC_EXTENSION* SdhcExtPtr,
    UINT32 InterruptMask
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    UINT32 interruptEnable = SdhcReadRegister(&registersPtr->INT_STATUS_EN);

    interruptEnable |= InterruptMask;

    //
    // Enable the interrupt signals from controller to OS
    //
    if (!SdhcExtPtr->CrashdumpMode) {
        SdhcWriteRegister(&registersPtr->INT_SIGNAL_EN, interruptEnable);
    }

    //
    // Enable the interrupt signals on the controller
    //
    SdhcWriteRegister(&registersPtr->INT_STATUS_EN, interruptEnable);
}

_Use_decl_annotations_
VOID
SdhcDisableInterrupt(
    USDHC_EXTENSION* SdhcExtPtr,
    UINT32 InterruptMask
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    UINT32 interruptDisable = SdhcReadRegister(&registersPtr->INT_STATUS_EN);

    interruptDisable &= ~InterruptMask;

    //
    // Disable the interrupt signals on the controller
    //
    SdhcWriteRegister(&registersPtr->INT_STATUS_EN, interruptDisable);

    //
    // Disable the interrupt signals from controller to OS
    //
    SdhcWriteRegister(&registersPtr->INT_SIGNAL_EN, interruptDisable);
}

_Use_decl_annotations_
VOID
SdhcAcknowledgeInterrupts(
    USDHC_EXTENSION* SdhcExtPtr,
    UINT32 InterruptMask
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    SdhcWriteRegister(&registersPtr->INT_STATUS, InterruptMask);
}

_Use_decl_annotations_
NTSTATUS
SdhcSendCommand(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_REQUEST *RequestPtr
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    SDPORT_COMMAND* cmdPtr = &RequestPtr->Command;
    USDHC_CMD_XFR_TYP_REG cmdXfrTyp = { 0 };
    NTSTATUS status;

    //
    // Initialize transfer parameters if this command is a data command
    //
    if ((cmdPtr->TransferType != SdTransferTypeNone) &&
        (cmdPtr->TransferType != SdTransferTypeUndefined)) {

        status = SdhcConfigureTransfer(SdhcExtPtr, RequestPtr);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    InterlockedExchange(
        reinterpret_cast<volatile LONG*>(&SdhcExtPtr->CurrentTransferRemainingLength),
        static_cast<LONG>(RequestPtr->Command.Length));

    //
    // Write the CMD argument
    //
    SdhcWriteRegister(&registersPtr->CMD_ARG, cmdPtr->Argument);

    cmdXfrTyp.CMDINX = cmdPtr->Index;

    //
    // Set the response parameters based off the given response type
    //
    switch (cmdPtr->ResponseType) {
    case SdResponseTypeNone:
        break;

    case SdResponseTypeR1:
    case SdResponseTypeR5:
    case SdResponseTypeR6:
        cmdXfrTyp.RSPTYP = USDHC_CMD_XFR_TYP_RSPTYP_RSP_48;
        cmdXfrTyp.CCCEN = 1;
        cmdXfrTyp.CICEN = 1;
        break;

    case SdResponseTypeR1B:
    case SdResponseTypeR5B:
        cmdXfrTyp.RSPTYP = USDHC_CMD_XFR_TYP_RSPTYP_RSP_48_CHK_BSY;
        cmdXfrTyp.CCCEN = 1;
        cmdXfrTyp.CICEN = 1;
        break;

    case SdResponseTypeR2:
        cmdXfrTyp.RSPTYP = USDHC_CMD_XFR_TYP_RSPTYP_RSP_136;
        cmdXfrTyp.CCCEN = 1;
        break;

    case SdResponseTypeR3:
    case SdResponseTypeR4:
        cmdXfrTyp.RSPTYP = USDHC_CMD_XFR_TYP_RSPTYP_RSP_48;
        break;
    
    default:
        NT_ASSERTMSG("Invalid response type", FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    if (cmdPtr->TransferType != SdTransferTypeNone) {
        cmdXfrTyp.DPSEL = 1;
    } else {
        USDHC_MIX_CTRL_REG mixCtrl = { SdhcReadRegister(&registersPtr->MIX_CTRL) };
        mixCtrl.DMAEN = 0;
        mixCtrl.AC12EN = 0;
        mixCtrl.AC23EN = 0;
        SdhcWriteRegister(&registersPtr->MIX_CTRL, mixCtrl.AsUint32);
    }

    switch (cmdPtr->Type) {
    case SdCommandTypeSuspend:
        cmdXfrTyp.CMDTYP = USDHC_CMD_XFR_TYP_CMDTYP_SUSPEND;
        break;

    case SdCommandTypeResume:
        cmdXfrTyp.CMDTYP = USDHC_CMD_XFR_TYP_CMDTYP_RESUME;
        break;

    case SdCommandTypeAbort:
        cmdXfrTyp.CMDTYP = USDHC_CMD_XFR_TYP_CMDTYP_ABORT;
        break;

    default:
        cmdXfrTyp.CMDTYP = USDHC_CMD_XFR_TYP_CMDTYP_NORMAL;
    }

    //
    // Set the bitmask for the required events that will fire after
    // writing to the command register. Depending on the response
    // type or whether the command involves data transfer, we will need
    // to wait on a number of different events
    //
    USDHC_INT_STATUS_REG requiredEvents = { 0 };
    requiredEvents.CC = 1;

    //
    // uSDHC does not fire TC on Busy signal deassertion. A workaround
    // is implemented in the command completion DPC. If uSDHC behaves
    // like the datasheet, the code below should be uncommented and the
    // workaround in the DPC should be removed
    //
    // if ((cmdPtr->ResponseType == SdResponseTypeR1B) ||
    //     (cmdPtr->ResponseType == SdResponseTypeR5B)) {
    //     requiredEvents.TC = 1;
    // }
    //

    if (cmdPtr->TransferMethod == SdTransferMethodSgDma) {
        requiredEvents.TC = 1;
    } else if (cmdPtr->TransferMethod == SdTransferMethodPio) {
        if (cmdPtr->TransferDirection == SdTransferDirectionRead) {
            requiredEvents.BRR = 1;
        } else {
            requiredEvents.BWR = 1;
        }

        USDHC_INT_STATUS_REG intStatusEnableMask = { 0 };
        intStatusEnableMask.BRR = requiredEvents.BRR;
        intStatusEnableMask.BWR = requiredEvents.BWR;
        SdhcEnableInterrupt(SdhcExtPtr, intStatusEnableMask.AsUint32);
    }

    SdhcConvertIntStatusToStandardEvents(
        requiredEvents,
        &RequestPtr->RequiredEvents,
        nullptr);

    //
    // Issue the actual command
    //
    SdhcWriteRegister(&registersPtr->CMD_XFR_TYP, cmdXfrTyp.AsUint32);

    //
    // We must wait asynchronously until the request completes
    //
    return STATUS_PENDING;
}

_Use_decl_annotations_
NTSTATUS
SdhcConfigureTransfer(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_REQUEST* RequestPtr
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    SDPORT_COMMAND* cmdPtr = &RequestPtr->Command;

    NT_ASSERT(cmdPtr->TransferMethod != SdTransferMethodUndefined);
    NT_ASSERT(
        (cmdPtr->TransferDirection == SdTransferDirectionRead) ||
        (cmdPtr->TransferDirection == SdTransferDirectionWrite));

    USDHC_MIX_CTRL_REG mixCtrl = { SdhcReadRegister(&registersPtr->MIX_CTRL) };

    mixCtrl.AC12EN = 0;
    mixCtrl.AC23EN = 0;
    mixCtrl.DMAEN = 0;
    mixCtrl.BCEN = 0;
    mixCtrl.MSBSEL = 0;

    if (cmdPtr->BlockCount > 1) {
        mixCtrl.MSBSEL = 1;
        mixCtrl.BCEN = 1;
        if (cmdPtr->TransferType != SdTransferTypeMultiBlockNoStop) {
            NT_ASSERT(cmdPtr->TransferType == SdTransferTypeMultiBlock);
            mixCtrl.AC12EN = 1;
        }
    }

    if (cmdPtr->TransferMethod == SdTransferMethodSgDma) {
        mixCtrl.DMAEN= 1;
    } else {
        NT_ASSERT(cmdPtr->TransferMethod == SdTransferMethodPio);
    }

    mixCtrl.DTDSEL = 0;
    if (cmdPtr->TransferDirection == SdTransferDirectionRead) {
        mixCtrl.DTDSEL = 1;
    }

    SdhcWriteRegister(&registersPtr->MIX_CTRL, mixCtrl.AsUint32);

    USDHC_BLK_ATT_REG blkAtt = { 0 };
    blkAtt.BLKSIZE = cmdPtr->BlockSize;
    blkAtt.BLKCNT = cmdPtr->BlockCount;
    SdhcWriteRegister(&registersPtr->BLK_ATT, blkAtt.AsUint32);

    //
    // Configure FIFO watermark levels to half the FIFO capacity
    // In case the whole transfer can fit in the FIFO, then use
    // the whole transfer length as the FIFO threshold, so we do
    // the read/write in one-shot
    //
    USDHC_WTMK_LVL_REG wtmkLvl = { 0 };
    UINT32 fifoThresholdWordCount = USDHC_FIFO_MAX_WORD_COUNT / 2;
    NT_ASSERT(cmdPtr->Length % sizeof(UINT32) == 0);
    const UINT32 transferWordCount = cmdPtr->Length / sizeof(UINT32);
    fifoThresholdWordCount = Min(transferWordCount, fifoThresholdWordCount);

    NT_ASSERT(fifoThresholdWordCount <= MAXUINT16);
    const UINT16 wml = static_cast<UINT16>(fifoThresholdWordCount);
    NT_ASSERT(wml <= USDHC_FIFO_MAX_WORD_COUNT);
    wtmkLvl.RD_WML = static_cast<UINT8>(wml);
    wtmkLvl.RD_BRST_LEN = Min(wml, UINT16(8));
    wtmkLvl.WR_WML = wtmkLvl.RD_WML;
    wtmkLvl.WR_BRST_LEN = wtmkLvl.RD_BRST_LEN;

    SdhcWriteRegister(&registersPtr->WTMK_LVL, wtmkLvl.AsUint32);

    if (cmdPtr->TransferMethod == SdTransferMethodSgDma) {
        NT_ASSERT(cmdPtr->ScatterGatherList != NULL);
        //
        // Create the ADMA2 descriptor table in the host's DMA buffer
        //
        NTSTATUS status = SdhcCreateAdmaDescriptorTable(SdhcExtPtr, RequestPtr);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        NT_ASSERT(cmdPtr->DmaPhysicalAddress.HighPart == 0);
        SdhcWriteRegister(
            &registersPtr->ADMA_SYS_ADDR, 
            static_cast<UINT32>(cmdPtr->DmaPhysicalAddress.LowPart));
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID
SdhcReadDataPort(
    USDHC_EXTENSION* SdhcExtPtr,
    UINT32* BufferPtr,
    UINT32 WordCount
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    UINT32* readBufferPtr = static_cast<UINT32*>(BufferPtr);

    while (WordCount) {
        *readBufferPtr = SdhcReadRegisterNoFence(&registersPtr->DATA_BUFF_ACC_PORT);
        ++readBufferPtr;
        --WordCount;
    }
}

_Use_decl_annotations_
VOID
SdhcWriteDataPort(
    USDHC_EXTENSION* SdhcExtPtr,
    const UINT32* BufferPtr,
    UINT32 WordCount
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    const UINT32* writeBufferPtr = static_cast<const UINT32*>(BufferPtr);

    while (WordCount) {
        SdhcWriteRegisterNoFence(&registersPtr->DATA_BUFF_ACC_PORT, *writeBufferPtr);
        ++writeBufferPtr;
        --WordCount;
    }
}

_Use_decl_annotations_
NTSTATUS
SdhcStartTransfer(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_REQUEST* RequestPtr
    )
{
    NT_ASSERT(RequestPtr->Command.TransferType != SdTransferTypeNone);

    switch (RequestPtr->Command.TransferMethod) {
    case SdTransferMethodPio:
        return SdhcStartPioTransfer(SdhcExtPtr, RequestPtr);

    case SdTransferMethodSgDma:
        return SdhcStartAdmaTransfer(SdhcExtPtr, RequestPtr);

    default:
        NT_ASSERTMSG("Transfer method not supported", FALSE);
        return STATUS_NOT_SUPPORTED;
    }
}

_Use_decl_annotations_
NTSTATUS
SdhcStartPioTransfer(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_REQUEST* RequestPtr
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    SDPORT_COMMAND* cmdPtr = &RequestPtr->Command;

    NT_ASSERT((cmdPtr->TransferDirection == SdTransferDirectionRead) ||
              (cmdPtr->TransferDirection == SdTransferDirectionWrite));

    //
    // PIO transfers are paced by FIFO interrupts, where each interrupt indicates that
    // there is at least 1 RD_WML words to read or enough space for WR_WML words to write.
    // To safely read/write to/from the FIFO without polling on any state bit, we should
    // read exactly number of words equal to the FIFOs watermark for each interrupt
    //
    USDHC_WTMK_LVL_REG wtmkLvl = { SdhcReadRegister(&registersPtr->WTMK_LVL) };
    UINT32 subTransferLength;

    if (cmdPtr->TransferDirection == SdTransferDirectionRead) {
        SdhcReadDataPort(SdhcExtPtr,
                         reinterpret_cast<UINT32*>(cmdPtr->DataBuffer),
                         wtmkLvl.RD_WML);
        subTransferLength = (wtmkLvl.RD_WML * sizeof(UINT32));
    } else {
        SdhcWriteDataPort(SdhcExtPtr,
                          reinterpret_cast<const UINT32*>(cmdPtr->DataBuffer),
                          wtmkLvl.WR_WML);
        subTransferLength = (wtmkLvl.WR_WML * sizeof(UINT32));
    }

    //
    // Achieve atomic subtraction by perform atomic add with negative transfer length
    //
    InterlockedExchangeAdd(
        reinterpret_cast<volatile LONG*>(&SdhcExtPtr->CurrentTransferRemainingLength),
        -static_cast<LONG>(subTransferLength));

    USDHC_INT_STATUS_REG requiredEventsMask = { 0 };

    if (InterlockedCompareExchange(
            reinterpret_cast<volatile LONG*>(&SdhcExtPtr->CurrentTransferRemainingLength),
            0,
            0) > 0) {
        cmdPtr->DataBuffer += subTransferLength;
        if (cmdPtr->TransferDirection == SdTransferDirectionRead) {
            requiredEventsMask.BRR = 1;
        } else {
            requiredEventsMask.BWR = 1;
        }
        RequestPtr->Status = STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        requiredEventsMask.TC = 1;
        RequestPtr->Status = STATUS_SUCCESS;
    }

    SdhcConvertIntStatusToStandardEvents(
        requiredEventsMask,
        &RequestPtr->RequiredEvents,
        nullptr);

    //
    // Re-enable BRR and BWR if needed to chain the data transfer
    //
    // WORKAROUND: It is generally unsafe to enable interrupts from a DPC
    // without the use of an interrupt synchronization construct, but
    // it was found that uSDHC keeps firing the BRR and BWR interrupts and
    // they don't get acknowledged as expected, so we disable them in the ISR
    //
    if (requiredEventsMask.BRR || requiredEventsMask.BWR) {
        USDHC_INT_STATUS_REG intStatusEnableMask = { 0 };
        intStatusEnableMask.BRR = requiredEventsMask.BRR;
        intStatusEnableMask.BWR = requiredEventsMask.BWR;
        SdhcEnableInterrupt(SdhcExtPtr, intStatusEnableMask.AsUint32);
    }

    return STATUS_PENDING;
}

_Use_decl_annotations_
NTSTATUS
SdhcStartAdmaTransfer(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_REQUEST* RequestPtr
    )
{
    SdhcCompleteRequest(SdhcExtPtr, RequestPtr, STATUS_SUCCESS);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcCreateAdmaDescriptorTable(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_REQUEST* RequestPtr
    )
{
    USDHC_ADMA2_DESCRIPTOR_TABLE_ENTRY* descriptorPtr =
        reinterpret_cast<USDHC_ADMA2_DESCRIPTOR_TABLE_ENTRY*>(RequestPtr->Command.DmaVirtualAddress);
    ULONG sgListEelementCount = RequestPtr->Command.ScatterGatherList->NumberOfElements;
    NT_ASSERT(sgListEelementCount > 0);
    const SCATTER_GATHER_ELEMENT* sgListElementPtr = &RequestPtr->Command.ScatterGatherList->Elements[0];
    PHYSICAL_ADDRESS nextAddress;
    ULONG nextLength;
    ULONG remainingLength;
    ULONG tableEntryCount = 0;

    //
    // Iterate through each element in the SG list and convert it into the
    // descriptor table required by the controller.
    //
    while (sgListEelementCount > 0) {
        remainingLength = sgListElementPtr->Length;
        nextAddress.QuadPart = sgListElementPtr->Address.QuadPart;

        NT_ASSERT(remainingLength > 0);

        while (remainingLength > 0) {

            nextLength = Min(ULONG(SDHC_ADMA2_MAX_LENGTH_PER_ENTRY), remainingLength);
            remainingLength -= nextLength;

            //
            // Set the entry attributes and length.
            //
            descriptorPtr->AsUint64 = 0;
            descriptorPtr->Valid = 1;
            descriptorPtr->Action = USDHC_ADMA2_ACTION_TRAN;

            NT_ASSERT(nextLength <= SDHC_ADMA2_MAX_LENGTH_PER_ENTRY);
            descriptorPtr->Length = static_cast<UINT32>(nextLength);

            //
            // The HighPart should not be a non-zero value, since in the
            // DMA adapter object, we declared that this device only
            // supports 32-bit addressing.
            //
            NT_ASSERT(nextAddress.HighPart == 0);

            //
            // ADMA address field shall be set on word aligned boundaries
            //
            NT_ASSERT((ULONG_PTR(nextAddress.LowPart) & 0x3) == 0);
            descriptorPtr->Address = static_cast<UINT32>(nextAddress.LowPart);
    
            nextAddress.QuadPart += nextLength;
            ++descriptorPtr;
            ++tableEntryCount;
        }

        ++sgListElementPtr;
        --sgListEelementCount;
    }

    //
    // Set the END bit at the last descriptor
    //
    --descriptorPtr;
    descriptorPtr->End = 1;

    USDHC_LOG_TRACE(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "ADMA2 Descriptor Table Entry#:%lu PA:%p",
        tableEntryCount,
        reinterpret_cast<UINT32*>(RequestPtr->Command.DmaPhysicalAddress.LowPart));

#if DEBUG_ADMA2_DESCRIPTOR_TABLE
    descriptorPtr =
        reinterpret_cast<USDHC_ADMA2_DESCRIPTOR_TABLE_ENTRY*>(RequestPtr->Command.DmaVirtualAddress);
    for (UINT32 entryIdx = 0; entryIdx < tableEntryCount; ++entryIdx) {
        USDHC_LOG_TRACE
            SdhcExtPtr->IfrLogHandle,
            SdhcExtPtr,
            "[Valid:%lu End:%lu Int:%lu Act:%lu Length:0x%X Addr:0x%p]",
            descriptorPtr->Valid,
            descriptorPtr->End,
            descriptorPtr->Int,
            descriptorPtr->Action,
            descriptorPtr->Length,
            reinterpret_cast<UINT32*>(descriptorPtr->Address));
        ++descriptorPtr;
    }
#endif

    return STATUS_SUCCESS;
}

UINT32
SdhcConvertStandardEventsToIntStatusMask(
    _In_ ULONG StdEventMask,
    _In_ ULONG StdErrorMask
    )
{
    SDHC_STANDARD_EVENTS_STATUS stdEventsStatus = { StdEventMask };
    USDHC_INT_STATUS_REG intStatus;

    intStatus.CC = stdEventsStatus.CommandComplete;
    intStatus.TC = stdEventsStatus.TransferCompelte;
    intStatus.BGE = stdEventsStatus.BlockGapEvent;
    intStatus.DINT = stdEventsStatus.DmaInterrupt;
    intStatus.BWR = stdEventsStatus.BufferWriteReady;
    intStatus.BRR = stdEventsStatus.BufferReadReady;
    intStatus.CINS = stdEventsStatus.CardInsertion;
    intStatus.CRM = stdEventsStatus.CardRemoval;
    intStatus.CINT = stdEventsStatus.CardInterrupt;
    intStatus.RTE = stdEventsStatus.ReTuningEvent;

    if (StdErrorMask) {
        SDHC_STANDARD_ERROR_STATUS stdErrorStatus = { StdErrorMask }; 
        intStatus.CTOE = stdErrorStatus.CommandTimeoutError;
        intStatus.CCE = stdErrorStatus.CommandCrcError;
        intStatus.CEBE = stdErrorStatus.CommandEndBitError;
        intStatus.CIE = stdErrorStatus.CommandIndexError;
        intStatus.DTOE = stdErrorStatus.DataTimeoutError;
        intStatus.DCE = stdErrorStatus.DataCrcError;
        intStatus.DEBE  = stdErrorStatus.DataEndBitError;
        intStatus.AC12E  = stdErrorStatus.AutoCmdError;
        intStatus.TNE  = stdErrorStatus.TuningError;
        intStatus.DMAE  = stdErrorStatus.AdmaError;
    }

    return intStatus.AsUint32;
}

VOID
SdhcConvertIntStatusToStandardEvents(
    _In_ USDHC_INT_STATUS_REG IntStatus,
    _Out_ ULONG* StdEventMask,
    _Out_ ULONG* StdErrorMask
    )
{
    SDHC_STANDARD_EVENTS_STATUS stdEventsStatus = { 0 };

    stdEventsStatus.CommandComplete = IntStatus.CC;
    stdEventsStatus.TransferCompelte = IntStatus.TC;
    stdEventsStatus.BlockGapEvent = IntStatus.BGE;
    stdEventsStatus.DmaInterrupt = IntStatus.DINT;
    stdEventsStatus.BufferWriteReady = IntStatus.BWR;
    stdEventsStatus.BufferReadReady = IntStatus.BRR;
    stdEventsStatus.CardInsertion = IntStatus.CINS;
    stdEventsStatus.CardRemoval = IntStatus.CRM;
    stdEventsStatus.CardInterrupt = IntStatus.CINT;
    stdEventsStatus.ReTuningEvent = IntStatus.RTE;

    if (IntStatus.AsUint32 & USDHC_INT_STATUS_ERROR) {
        stdEventsStatus.ErrorInterrupt = 1;
    }

    *StdEventMask = static_cast<ULONG>(stdEventsStatus.AsUint32);

    if (StdEventMask && stdEventsStatus.ErrorInterrupt) {
        SDHC_STANDARD_ERROR_STATUS stdErrorStatus = { 0 };

        stdErrorStatus.CommandTimeoutError = IntStatus.CTOE;
        stdErrorStatus.CommandCrcError = IntStatus.CCE;
        stdErrorStatus.CommandEndBitError = IntStatus.CEBE;
        stdErrorStatus.CommandIndexError = IntStatus.CIE;
        stdErrorStatus.DataTimeoutError = IntStatus.DTOE;
        stdErrorStatus.DataCrcError = IntStatus.DCE;
        stdErrorStatus.DataEndBitError = IntStatus.DEBE;
        stdErrorStatus.AutoCmdError = IntStatus.AC12E;
        stdErrorStatus.TuningError = IntStatus.TNE;
        stdErrorStatus.AdmaError = IntStatus.DMAE;

        *StdErrorMask = static_cast<ULONG>(stdErrorStatus.AsUint32);
    }
}

NTSTATUS
SdhcConvertStandardErrorToStatus(
    _In_ ULONG ErrorMask
    )
{
    if (ErrorMask == 0) {
        return STATUS_SUCCESS;
    }

    SDHC_STANDARD_ERROR_STATUS stdErrorStatus = { ErrorMask };

    if (stdErrorStatus.CommandTimeoutError | stdErrorStatus.DataTimeoutError) {
        return STATUS_IO_TIMEOUT;
    }

    if (stdErrorStatus.CommandCrcError | stdErrorStatus.DataCrcError) {
        return STATUS_CRC_ERROR;
    }

    if (stdErrorStatus.CommandEndBitError | stdErrorStatus.DataEndBitError) {
        return STATUS_DEVICE_DATA_ERROR;
    }

    if (stdErrorStatus.CommandIndexError) {
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    return STATUS_IO_DEVICE_ERROR;
}

_Use_decl_annotations_
VOID
SdhcCompleteRequest(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_REQUEST* RequestPtr,
    NTSTATUS Status
    )
{
    USDHC_LOG_TRACE(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "Type:%!REQUESTTYPE! %s%d(0x%08X) (#Blocks:%lu, Length:%lu)  %!STATUS!",
        RequestPtr->Type,
        (RequestPtr->Command.Class == SdCommandClassApp ? "ACMD" : "CMD"),
        RequestPtr->Command.Index,
        RequestPtr->Command.Argument,
        (UINT32)RequestPtr->Command.BlockCount,
        RequestPtr->Command.Length,
        Status);

    RequestPtr->Status = Status;

    switch (RequestPtr->Type) {
    case SdRequestTypeCommandNoTransfer:
    case SdRequestTypeCommandWithTransfer:
        NT_ASSERT(RequestPtr->Status != STATUS_MORE_PROCESSING_REQUIRED);
        break;

    case SdRequestTypeStartTransfer:
        if (NT_SUCCESS(Status)) {
            USDHC_LOG_INFORMATION(
                SdhcExtPtr->IfrLogHandle,
                SdhcExtPtr,
                "%s Blocks#:%lu Length:%lu LBA:0x%x",
                (RequestPtr->Command.TransferDirection == SdTransferDirectionRead ? "Read" : "Write"),
                (UINT32)RequestPtr->Command.BlockCount,
                RequestPtr->Command.Length,
                RequestPtr->Command.Argument);
        }
        break;

    default:
        NT_ASSERTMSG("Unsupported request type", FALSE);
    }

    SdPortCompleteRequest(RequestPtr, Status);
}

_Use_decl_annotations_
BOOLEAN
SdhcSlotGetCardDetectState(
    PVOID PrivateExtensionPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");

    volatile USDHC_REGISTERS* registersPtr = sdhcExtPtr->RegistersPtr;
    USDHC_PRES_STATE_REG presState = { SdhcReadRegister(&registersPtr->PRES_STATE) };
    BOOLEAN isCardInserted = (presState.CINST ? TRUE : FALSE);

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "%!bool!", isCardInserted);
    return isCardInserted;
}

_Use_decl_annotations_
BOOLEAN
SdhcSlotGetWriteProtectState(
    PVOID PrivateExtensionPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
    
    volatile USDHC_REGISTERS* registersPtr = sdhcExtPtr->RegistersPtr;
    USDHC_PRES_STATE_REG presState = { SdhcReadRegister(&registersPtr->PRES_STATE) };
    BOOLEAN isWriteProtected = (presState.WPSPL ? FALSE : TRUE);

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "%!bool!", isWriteProtected);
    return isWriteProtected;
}

_Use_decl_annotations_
NTSTATUS
SdhcGetSlotCount(
    SD_MINIPORT* MiniportPtr,
    UCHAR* SlotCountPtr
    )
{
    USDHC_DDI_ENTER(DriverLogHandle, NullPrivateExtensionPtr, "()");
    NTSTATUS status;

#if 0   // DevicePropertiesList Code
    DEVICE_OBJECT* sdhcPdoPtr = SdhcMiniportGetPdo(MiniportPtr);
    USDHC_DEVICE_PROPERTIES* devPropsPtr;

    //
    // Read Device Properties associated with the SDHC PDO
    //
    status = SdhcReadDeviceProperties(sdhcPdoPtr, &devPropsPtr);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            status,
            "Failed to read Device Properties for SDHC PDO:0x%p",
            sdhcPdoPtr);
        goto Cleanup;
    }
#endif

    switch (MiniportPtr->ConfigurationInfo.BusType) {
    case SdBusTypeAcpi:
        *SlotCountPtr = USDHC_DEFAULT_SLOT_COUNT;
        status = STATUS_SUCCESS;
        goto Cleanup;
        break;

    default:
        NT_ASSERTMSG("Unsupported SDPORT_BUS_TYPE", FALSE);
        status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

Cleanup:

    USDHC_DDI_EXIT(DriverLogHandle, NullPrivateExtensionPtr, "()");
    return status;
}

_Use_decl_annotations_
VOID
SdhcSlotGetCapabilities(
    VOID* PrivateExtensionPtr,
    SDPORT_CAPABILITIES* CapabilitiesPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr =
        static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");

    RtlCopyMemory(
        CapabilitiesPtr, 
        &sdhcExtPtr->Capabilities,
        sizeof(sdhcExtPtr->Capabilities));

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "()");
}

_Use_decl_annotations_
NTSTATUS
SdhcSlotInitialize(
    PVOID PrivateExtensionPtr,
    PHYSICAL_ADDRESS PhysicalBase,
    PVOID VirtualBasePtr,
    ULONG Length,
    BOOLEAN CrashdumpMode
    )
{
    USDHC_DDI_ENTER(
        DriverLogHandle,
        NullPrivateExtensionPtr,
        "PhysicalBase:0x%p, VirtualBasePtr:0x%p, Length=0x%lu, CrashdumpMode=%!bool!",
        PVOID(PhysicalBase.LowPart),
        VirtualBasePtr,
        Length,
        CrashdumpMode);

    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);
    NTSTATUS status;
    USDHC_HOST_CTRL_CAP_REG hostCtrlCap;

    //
    // Initialize the USDHC_EXTENSION register space.
    //
    sdhcExtPtr->RegistersPtr = static_cast<USDHC_REGISTERS*>(VirtualBasePtr);
    sdhcExtPtr->DebugRegistersPtr = static_cast<USDHC_REGISTERS_DEBUG*>(VirtualBasePtr);
    sdhcExtPtr->PhysicalAddress = reinterpret_cast<VOID*>(PhysicalBase.LowPart);
    NT_ASSERT(PhysicalBase.HighPart == 0);

    //
    // Check whether the driver is in crashdump mode.
    //
    sdhcExtPtr->CrashdumpMode = CrashdumpMode;
 #if DBG
    if (CrashdumpMode) {
        sdhcExtPtr->BreakOnDdiEnter = FALSE;
        sdhcExtPtr->BreakOnDdiExit = FALSE;
        sdhcExtPtr->BreakOnError = TRUE;
        //
        // Give a chance to change break flags from debugger to control debugging experience
        // from within crashdump environment
        //
        DbgBreakPoint();
    }
#endif // DBG

    //
    // Only if not running in crashdump mode:
    // - Initialize IFR log dedicated to this miniport instance for
    //   non-crashdump mode
    //
    if (!CrashdumpMode) {
        SdhcLogInit(sdhcExtPtr);
#if 0   // DevicePropertiesList Code
        //
        // Grab device properties and keep a local copy of it
        //
        USDHC_DEVICE_PROPERTIES* devPropsPtr =
            DevicePropertiesListSafeFindByKey(
                reinterpret_cast<UINT32>(sdhcExtPtr->PhysicalAddress));
        if (devPropsPtr == nullptr) {
            NT_ASSERTMSG("Can't operate without device properties", FALSE);
            //
            // We should have been preceded by GetSlotCount bus operation in which
            // we should have read the device properties
            //
            USDHC_LOG_ERROR(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                "Unable to locate Device Properties entry for uSDHC with "
                "PhysicalAddress:0x%p",
                sdhcExtPtr->PhysicalAddress);
            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto Cleanup;
        }

        RtlCopyMemory(
            &sdhcExtPtr->DeviceProperties,
            devPropsPtr,
            sizeof(*devPropsPtr));
#endif
    }

    //
    // Currently, we don't support 1.8V switching, SDR50/SDR104/DDR, and
    // SDBus power control
    //
    sdhcExtPtr->DeviceProperties.Regulator1V8Exist = FALSE;
    sdhcExtPtr->DeviceProperties.SlotPowerControlSupported = FALSE;
    sdhcExtPtr->DeviceProperties.SlotCount = USDHC_DEFAULT_SLOT_COUNT;
    sdhcExtPtr->DeviceProperties.BaseClockFrequencyHz =
        USDHC_DEFAULT_BASE_CLOCK_FREQ_HZ;

    //
    // Power-off the SD bus initially. Sdport will ask for power-up later on
    //
    // N.B. By default, this flow will not be executed but is left in for future
    //  work when slot power control is fully supported.
    //
    if (sdhcExtPtr->DeviceProperties.SlotPowerControlSupported != FALSE) {
        BOOLEAN enable = FALSE;
        status = SdhcSetSdBusPower(sdhcExtPtr, enable);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SDBus power-control not working as expected");

            goto Cleanup;
        }
    }

    //
    // Initialize host capabilities
    //
    SDPORT_CAPABILITIES* capabilitiesPtr = &sdhcExtPtr->Capabilities;
    hostCtrlCap.AsUint32 = SdhcReadRegister(&sdhcExtPtr->RegistersPtr->HOST_CTRL_CAP);

    //
    // SDHC is SD/SDIO 3.0 compliant
    //
    capabilitiesPtr->SpecVersion = 3;
    capabilitiesPtr->MaximumOutstandingRequests = USDHC_MAX_OUTSTANDING_REQUESTS;
    capabilitiesPtr->MaximumBlockSize = (USHORT)(512 << hostCtrlCap.MBL);
    capabilitiesPtr->MaximumBlockCount = 0xFFFF;
    capabilitiesPtr->BaseClockFrequencyKhz = sdhcExtPtr->DeviceProperties.BaseClockFrequencyHz / 1000;

    capabilitiesPtr->DmaDescriptorSize =
        sizeof(USDHC_ADMA2_DESCRIPTOR_TABLE_ENTRY);

    capabilitiesPtr->AlignmentRequirement = sizeof(ULONG) - 1;

    if (hostCtrlCap.ADMAS) {
        capabilitiesPtr->Supported.ScatterGatherDma = 1;
    }

    if (hostCtrlCap.HSS) {
        capabilitiesPtr->Supported.HighSpeed = 1;
    }

    capabilitiesPtr->Supported.BusWidth8Bit = 1;

    //
    // 1.8V switching is supported if the supporting regulator circuit exist
    // and SDBus power-control functionality is supported and reliable
    //
    capabilitiesPtr->Supported.SignalingVoltage18V = 0;
    if ((sdhcExtPtr->DeviceProperties.Regulator1V8Exist != 0) &&
        (sdhcExtPtr->DeviceProperties.SlotPowerControlSupported != FALSE)) {
        capabilitiesPtr->Supported.SignalingVoltage18V = 1;
    }

    //
    // uSDHC supports 1.8V signaling, SDR50, SDR105 and DDR50 modes
    // However, we claim not supporting SD104 due to missing tuning implementation,
    // and not supporting DDR50 due to Sdport not having that working properly
    // at the moment
    //
    capabilitiesPtr->Supported.SDR50 = capabilitiesPtr->Supported.SignalingVoltage18V;
    capabilitiesPtr->Supported.SDR104 = 0;
    capabilitiesPtr->Supported.DDR50 = 0;

    capabilitiesPtr->Supported.HS200 = 0;
    capabilitiesPtr->Supported.HS400 = 0;

    capabilitiesPtr->Supported.TuningForSDR50 = 0;
    capabilitiesPtr->Supported.DriverTypeA = 1;
    capabilitiesPtr->Supported.DriverTypeB = 1;
    capabilitiesPtr->Supported.DriverTypeC = 1;
    capabilitiesPtr->Supported.DriverTypeD = 1;

    capabilitiesPtr->Supported.Limit200mA = 1;
    capabilitiesPtr->Supported.Limit400mA = 1;
    capabilitiesPtr->Supported.Limit600mA = 1;
    capabilitiesPtr->Supported.Limit800mA = 1;

    capabilitiesPtr->Supported.AutoCmd12 = 1;
    capabilitiesPtr->Supported.AutoCmd23 = 1;

    if (hostCtrlCap.VS18) {
        capabilitiesPtr->Supported.Voltage18V = 1;
    }

    if (hostCtrlCap.VS30) {
        capabilitiesPtr->Supported.Voltage30V = 1;
    }

    if (hostCtrlCap.VS33) {
        capabilitiesPtr->Supported.Voltage33V = 1;
    }

    if (gForcePio)
    {
        capabilitiesPtr->Flags.UsePioForRead = 1;
        capabilitiesPtr->Flags.UsePioForWrite = 1;
        capabilitiesPtr->DmaDescriptorSize = 0;
        capabilitiesPtr->Supported.ScatterGatherDma = 0;
    }

    status = STATUS_SUCCESS;

Cleanup:

    USDHC_DDI_EXIT(DriverLogHandle, NullPrivateExtensionPtr, "()");
    return status;
}

_Use_decl_annotations_
NTSTATUS
SdhcSlotIssueBusOperation(
    PVOID PrivateExtensionPtr,
    SDPORT_BUS_OPERATION* BusOperationPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr = static_cast<USDHC_EXTENSION*>(PrivateExtensionPtr);

    USDHC_DDI_ENTER(
        sdhcExtPtr->IfrLogHandle,
        sdhcExtPtr,
        "Type:%!BUSOPERATIONTYPE!",
        BusOperationPtr->Type);

    NTSTATUS status;

    switch (BusOperationPtr->Type) {
    case SdResetHost:
        status = SdhcResetHost(sdhcExtPtr, BusOperationPtr->Parameters.ResetType);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcResetHost() failed");
        }
        break;

    case SdSetClock:
        status = SdhcSetClock(sdhcExtPtr, BusOperationPtr->Parameters.FrequencyKhz);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcSetClock() failed");
        }
        break;

    case SdSetBusWidth:
        status = SdhcSetBusWidth(sdhcExtPtr, BusOperationPtr->Parameters.BusWidth);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcSetBusWidth() failed");
        }
        break;

    case SdSetBusSpeed:
        status = SdhcSetSpeed(sdhcExtPtr, BusOperationPtr->Parameters.BusSpeed);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcSetSpeed() failed");
        }
        break;

    case SdSetSignalingVoltage:
        status = 
            SdhcSetSignaling(
                sdhcExtPtr, 
                (BOOLEAN)(BusOperationPtr->Parameters.SignalingVoltage == SdSignalingVoltage18));
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcSetSignaling() failed");
        }
        break;

    case SdSetVoltage:
        status = SdhcSetBusVoltage(sdhcExtPtr, BusOperationPtr->Parameters.Voltage);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcSetBusVoltage() failed");
        }
        break;
    case SdResetHw:
    case SdSetDriveStrength:
    case SdSetDriverType:
    case SdSetPresetValue:
        //
        // Convince Sdport that we did that unsupported bus operation
        //
        status = STATUS_SUCCESS;
        break;

    case SdSetBlockGapInterrupt:
        status = SdhcEnableBlockGapInterrupt(
                    sdhcExtPtr,
                    BusOperationPtr->Parameters.BlockGapIntEnabled);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcEnableBlockGapInterrupt() failed");
        }
        break;

    case SdExecuteTuning:
        status = SdhcExecuteTuning(sdhcExtPtr);
        if (!NT_SUCCESS(status)) {
            USDHC_LOG_ERROR_STATUS(
                sdhcExtPtr->IfrLogHandle,
                sdhcExtPtr,
                status,
                "SdhcExecuteTuning() failed");
        }
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        USDHC_LOG_ERROR(
            sdhcExtPtr->IfrLogHandle,
            sdhcExtPtr,
            "Unsupported bus operation %!BUSOPERATIONTYPE!",
            BusOperationPtr->Type);
    }

    USDHC_DDI_EXIT(sdhcExtPtr->IfrLogHandle, sdhcExtPtr, "%!STATUS!", status);
    return status;
}

_Use_decl_annotations_
NTSTATUS
SdhcResetHost(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_RESET_TYPE ResetType
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    USDHC_SYS_CTRL_REG sysCtrlMask = { 0 };

    USDHC_LOG_INFORMATION(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "ResetType:%!RESETTYPE!",
        ResetType);

    switch (ResetType) {
    case SdResetTypeAll:
        sysCtrlMask.RSTA = 1;
        break;

    case SdResetTypeCmd:
        sysCtrlMask.RSTC = 1;
        break;

    case SdResetTypeDat:
        sysCtrlMask.RSTD = 1;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    USDHC_SYS_CTRL_REG sysCtrl = { SdhcReadRegister(&registersPtr->SYS_CTRL) };
    sysCtrl.AsUint32 |= sysCtrlMask.AsUint32;
    SdhcWriteRegister(&registersPtr->SYS_CTRL, sysCtrl.AsUint32);

    UINT32 retries = USDHC_POLL_RETRY_COUNT;
    while ((sysCtrl.AsUint32 & sysCtrlMask.AsUint32) &&
           retries) {
        SdPortWait(USDHC_POLL_WAIT_TIME_US);
        --retries;
        sysCtrl.AsUint32 = SdhcReadRegister(&registersPtr->SYS_CTRL);
    }

    if (sysCtrl.AsUint32 & sysCtrlMask.AsUint32) {
        NT_ASSERT(!retries);
        return STATUS_IO_TIMEOUT;
    }

    if (ResetType != SdResetTypeAll) {
        return STATUS_SUCCESS;
    }

    sysCtrlMask.AsUint32 = 0;
    sysCtrlMask.INITA = 1;
    sysCtrl.AsUint32 = SdhcReadRegister(&registersPtr->SYS_CTRL);
    sysCtrl.AsUint32 |= sysCtrlMask.AsUint32;

    SdhcWriteRegister(
        &registersPtr->SYS_CTRL,
        sysCtrl.AsUint32);

    retries = USDHC_POLL_RETRY_COUNT;
    while ((sysCtrl.AsUint32 & sysCtrlMask.AsUint32) &&
           retries) {
        SdPortWait(USDHC_POLL_WAIT_TIME_US);
        --retries;
        sysCtrl.AsUint32 = SdhcReadRegister(&registersPtr->SYS_CTRL);
    }

    if (sysCtrl.AsUint32 & sysCtrlMask.AsUint32) {
        NT_ASSERT(!retries);
        return STATUS_IO_TIMEOUT;
    }

    //
    // Initially disable and acknowledge all interrupts, they will
    // get toggled on demand by Sdport
    //
    SdhcDisableInterrupt(SdhcExtPtr, UINT32(~0));
    SdhcAcknowledgeInterrupts(SdhcExtPtr, UINT32(~0));

    //
    // Set the max HW timeout for bus operations.
    //
    sysCtrl.AsUint32 = SdhcReadRegister(&registersPtr->SYS_CTRL);
    sysCtrl.DTOCV = USDHC_SYS_CTRL_DTOCV_MAX_TIMEOUT;
    SdhcWriteRegister(&registersPtr->SYS_CTRL, sysCtrl.AsUint32);

    //
    // Not all registers get their reset value with ResetAll, make sure
    // to manually reset these registers
    //
    SdhcWriteRegister(&registersPtr->MIX_CTRL, 0);
    SdhcWriteRegister(&registersPtr->DLL_CTRL, 0);
    SdhcWriteRegister(&registersPtr->CLK_TUNE_CTRL_STATUS, 0);
    SdhcWriteRegister(&registersPtr->VEND_SPEC, USDHC_VEND_SPEC_RESET_VALUE);
    SdhcWriteRegister(&registersPtr->MMC_BOOT, 0);
    SdhcWriteRegister(&registersPtr->VEND_SPEC2, USDHC_VEND_SPEC2_RESET_VALUE);

    //
    // Initialize DMA if the controller supports it.
    //
    USDHC_PROT_CTRL_REG protCtrl = { USDHC_PROT_CTRL_RESET_VALUE };
    protCtrl.DMASEL = USDHC_PROT_CTRL_DMASEL_NO_DMA;
    if (SdhcExtPtr->Capabilities.Supported.ScatterGatherDma) {
        protCtrl.DMASEL = USDHC_PROT_CTRL_DMASEL_ADMA2;
    }
    SdhcWriteRegister(&registersPtr->PROT_CTRL, protCtrl.AsUint32);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcSetClock(
    USDHC_EXTENSION* SdhcExtPtr,
    ULONG FrequencyKhz
    )
{
    USDHC_LOG_INFORMATION(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "FrequencyKhz:%lu",
        FrequencyKhz);

    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;

    //
    // SDCLK = (Base Clock) / (prescaler x divisor)
    //
    UINT32 prescaler;
    UINT32 divisor;
    UINT32 sdClk;

    UINT32 minFreqDistance = 0xFFFFFFFF;
    UINT32 freqDistance;
    UINT32 bestPrescaler = 0;
    UINT32 bestDivisor = 0;
    UINT32 targetFreqHz = FrequencyKhz * 1000;
    const UINT32 baseClockFreqHz = SdhcExtPtr->DeviceProperties.BaseClockFrequencyHz;

    USDHC_MIX_CTRL_REG mixCtrl = { SdhcReadRegister(&registersPtr->MIX_CTRL) };
    //
    // Bruteforce to find the best prescaler and divisor that result
    // in SDCLK less than or equal to the requested frequency
    //
    // Allowed |Base clock divided By
    // SDCLKFS |DDR_EN=0   |DDR_EN=1
    // 80h      256         512
    // 40h      128         256
    // 20h      64          128
    // 10h      32          64
    // 08h      16          32
    // 04h      8           16
    // 02h      4           8
    // 01h      2           4
    // 00h      1           2
    //
    const UINT32 prescalarMin = (mixCtrl.DDR_EN ? 2 : 1);
    const UINT32 prescalarMax = (mixCtrl.DDR_EN ? 512 : 256);;
    const UINT32 divisorMin = 1;
    const UINT32 divisorMax = 16;

    //
    // Clamp the target frequency to SDClock limits
    //
    const UINT32 minFreqHz = baseClockFreqHz / (prescalarMax * divisorMax);
    const UINT32 maxFreqHz = baseClockFreqHz / (prescalarMin * divisorMin);
    NT_ASSERT(minFreqHz < maxFreqHz);
    NT_ASSERTMSG("Can't set a zero SDClock frequency", targetFreqHz > 0);
    if (targetFreqHz < minFreqHz) {
        targetFreqHz = minFreqHz;
    } else if (targetFreqHz > maxFreqHz) {
        targetFreqHz = maxFreqHz;
    }

    bool foundExactTargetFreq = false;

    for (prescaler = prescalarMax;
         prescaler >= prescalarMin && !foundExactTargetFreq;
         prescaler /= 2) {
        for (divisor = divisorMin; divisor <= divisorMax; ++divisor) {
            sdClk = baseClockFreqHz / (prescaler * divisor);

            //
            // We are not willing to choose clocks higher than the target one
            // to avoid exceeding device limits
            //
            if (sdClk > targetFreqHz) {
                continue;
            } else if (sdClk == targetFreqHz) {
                bestPrescaler = prescaler;
                bestDivisor = divisor;
                foundExactTargetFreq = true;
                break;
            } else {
                //
                // This is the first possible frequency less than the target freq
                // produced using current prescaler and divisor
                // Going further in inner loop will result in bigger divisor and thus
                // smaller resulting SDCLK, and since this is the highest acceptable
                // freq we can get we will quit the inner loop early
                //
                freqDistance = targetFreqHz - sdClk;
                if (freqDistance < minFreqDistance) {
                    minFreqDistance = freqDistance;
                    bestPrescaler = prescaler;
                    bestDivisor = divisor;
                }
                break;
            }
        }
    }

    //
    // Wait for clock to become stable before any clock modifications
    //
    NTSTATUS status = SdhcWaitForStableSdClock(SdhcExtPtr);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    USDHC_SYS_CTRL_REG sysCtrl = { SdhcReadRegister(&registersPtr->SYS_CTRL) };
    sysCtrl.SDCLKFS = bestPrescaler / (mixCtrl.DDR_EN ? 4 : 2);
    sysCtrl.DVS = bestDivisor - 1;

    SdhcWriteRegister(&registersPtr->SYS_CTRL, sysCtrl.AsUint32);

    sdClk = baseClockFreqHz / (bestPrescaler * bestDivisor);

    USDHC_LOG_TRACE(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "Current SDCLK:%luHz SDCLKFS:0x%x DVS:0x%X",
        sdClk,
        sysCtrl.SDCLKFS,
        sysCtrl.DVS);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcSetBusWidth(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_BUS_WIDTH Width
    )
{
    USDHC_LOG_INFORMATION(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "Width:%lu-Bit",
        UINT32(Width));

    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    USDHC_PROT_CTRL_REG protCtrl = { SdhcReadRegister(&registersPtr->PROT_CTRL) };

    switch (Width) {
    case SdBusWidth1Bit:
        protCtrl.DTW = USDHC_PROT_CTRL_DTW_1BIT;
        break;
    case SdBusWidth4Bit:
        protCtrl.DTW = USDHC_PROT_CTRL_DTW_4BIT;
        break;
    case SdBusWidth8Bit:
        protCtrl.DTW = USDHC_PROT_CTRL_DTW_8BIT;
        break;
    default:
        NT_ASSERTMSG("Provided bus width is invalid", FALSE);
    }

    SdhcWriteRegister(&registersPtr->PROT_CTRL, protCtrl.AsUint32);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcSetSpeed(
    USDHC_EXTENSION* SdhcExtPtr,
    SDPORT_BUS_SPEED Speed
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;

    USDHC_LOG_INFORMATION(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "Speed:%!BUSSPEED!",
        Speed);

    USDHC_VEND_SPEC_REG vendSpec = { SdhcReadRegister(&registersPtr->VEND_SPEC) };

    switch (Speed) {
    case SdBusSpeedNormal:
    case SdBusSpeedHigh:
        if (vendSpec.VSELECT != 0) {
            NT_ASSERTMSG("Expected SDHC to be in 3V3 signaling state", FALSE);
            USDHC_LOG_ERROR(
                SdhcExtPtr->IfrLogHandle,
                SdhcExtPtr,
                "Unexpected SDHC signaling voltage state. Expected:3V3 Actual:1V8");
            return STATUS_IO_DEVICE_ERROR;
        }
        break;

    case SdBusSpeedDDR50:
    case SdBusSpeedSDR12:
    case SdBusSpeedSDR25:
    case SdBusSpeedSDR50:
    case SdBusSpeedSDR104:
        if (vendSpec.VSELECT == 0) {
            NT_ASSERTMSG("Expected SDHC to be in 1V8 signaling state", FALSE);
            USDHC_LOG_ERROR(
                SdhcExtPtr->IfrLogHandle,
                SdhcExtPtr,
                "Unexpected SDHC signaling voltage state. Expected:1V8 Actual:3v3");
            return STATUS_IO_DEVICE_ERROR;
        }
        break;

    case SdBusSpeedHS200:
    case SdBusSpeedHS400:
        USDHC_LOG_ERROR(
            SdhcExtPtr->IfrLogHandle,
            SdhcExtPtr,
            "Selected bus speed class is not supported");
        return STATUS_NOT_SUPPORTED;

    default:
        NT_ASSERTMSG("Invalid speed mode selected", FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    if (Speed == SdBusSpeedDDR50) {
        USDHC_MIX_CTRL_REG mixCtrl = { SdhcReadRegister(&registersPtr->MIX_CTRL) };
        mixCtrl.DDR_EN = 1;
        SdhcWriteRegister(&registersPtr->MIX_CTRL, mixCtrl.AsUint32);
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcSetSignaling(
    USDHC_EXTENSION* SdhcExtPtr,
    BOOLEAN Enable1V8Signaling
    )
{
    USDHC_LOG_INFORMATION(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "Enable1V8Signaling:%!bool!",
        UINT32(Enable1V8Signaling));

    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    USDHC_VEND_SPEC_REG vendSpec = { SdhcReadRegister(&registersPtr->VEND_SPEC) };

    //
    // Specs describe the 1.8V switching sequence and the expectations from the SDCard
    // on successful switching.
    //
    // On switching from 3.3V to 1.8V: verify switching was successful
    // On switching from 1.8V to 3.3V: skip voltage switching verification, because 
    // switching SD/MMC lines voltage back from 1.8V to 3.3V is not supported by SD/MMC specs,
    // and thus the DAT lines state will not go through the standard transitions.
    //
    BOOLEAN verify1V8Switching = FALSE;

    //
    // On no-state change, do nothing
    //
    if (BOOLEAN(vendSpec.VSELECT) == Enable1V8Signaling) {
        return STATUS_SUCCESS;
    } else if ((BOOLEAN(vendSpec.VSELECT) == FALSE) &&
        (Enable1V8Signaling != FALSE))
    {
        //
        // Transitioning lines from 3.3V to 1.8V, verify switching
        //
        verify1V8Switching = TRUE;
    }

    if (verify1V8Switching) {
        //
        // SD Clock Enable = 0
        //
        SdhcSdClockEnableInIdle(SdhcExtPtr, FALSE);

        //
        // Check DAT[3:0] equals 0000b
        //
        USDHC_PRES_STATE_REG presState = { SdhcReadRegister(&registersPtr->PRES_STATE) };
        if ((presState.DLSL & 0xF) != 0) {
            USDHC_LOG_ERROR(
                SdhcExtPtr->IfrLogHandle,
                SdhcExtPtr,
                "DLSL[3:0] expected:0x0 actual:0x%x",
                presState.DLSL & 0xF);
            SdhcSdClockEnableInIdle(SdhcExtPtr, TRUE);
            return STATUS_IO_DEVICE_ERROR;
        }
    }

    //
    // Set the signaling voltage to either 1.8V or 3.3V
    //
    vendSpec.AsUint32 = SdhcReadRegister(&registersPtr->VEND_SPEC);
    if (Enable1V8Signaling) {
        vendSpec.VSELECT = USDHC_VEND_SPEC_VSELECT_S1V8;
    } else {
        vendSpec.VSELECT = USDHC_VEND_SPEC_VSELECT_S3V3;
    }
    SdhcWriteRegister(&registersPtr->VEND_SPEC, vendSpec.AsUint32);

    if (verify1V8Switching) {
        //
        // Wait 10ms after voltage switching, specs recommend 5ms
        //
        SdPortWait(10000);

        //
        // SD Clock Enable = 1
        //
        SdhcSdClockEnableInIdle(SdhcExtPtr, TRUE);

        //
        // Wait 2ms after enabling/gating-on clock, specs recommend 1ms
        //
        SdPortWait(2000);

        //
        // Check DAT[3:0] equals 1111b
        //
        USDHC_PRES_STATE_REG presState = { SdhcReadRegister(&registersPtr->PRES_STATE) };
        if ((presState.DLSL & 0xF) != 0xF) {
            USDHC_LOG_ERROR(
                SdhcExtPtr->IfrLogHandle,
                SdhcExtPtr,
                "DLSL[3:0] expected:0xF actual:0x%x",
                presState.DLSL & 0xF);
            return STATUS_IO_DEVICE_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcSetBusVoltage(
    _In_ USDHC_EXTENSION* SdhcExtPtr,
    _In_ SDPORT_BUS_VOLTAGE Voltage
    )
{
    USDHC_LOG_INFORMATION(
        SdhcExtPtr->IfrLogHandle,
        SdhcExtPtr,
        "Voltage(VDD):%!BUSVOLTAGE!",
        Voltage);

    BOOLEAN powerOn;

    switch (Voltage) {
    case SdBusVoltage33:
        powerOn = TRUE;
        break;
    case SdBusVoltageOff:
        powerOn = FALSE;
        break;
    default:
        //
        // We know that uSDHC does not support other SD/MMC VDD supply voltages
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // The following power-up/power-cycle sequence wouldn't be necessary in
    // a spec compliant SDHC as it will be implemented on a HW level as part
    // of the SDHC state-machine. We try to mimic the power-up/power-cycle
    // sequence in software as close as possible to the specs.
    //

    //
    // In case of power-up:
    // Disable SDCLK auto gate-off feature of the uSDHC. We desire the clock to
    // be active all the time to avoid problems with SD//MMC expectations during
    // 1.8V switching negotiation
    //
    // In case of power-down:
    // Make sure the CLK line is inactive and that no clock is being fed to the
    // card to avoid any current leakage which may result in the card not performing
    // power-cycling correctly
    //
    SdhcSdClockEnableInIdle(SdhcExtPtr, powerOn);

    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    USDHC_VEND_SPEC_REG vendSpec = { SdhcReadRegister(&registersPtr->VEND_SPEC) };

    //
    // Since there is no standard SDBus voltage control as in standard SDHC
    // the closest we can do to help saving power is to force gate-off all SDHC
    // input and output clocks when SDBus PowerOff is issued and ungate all
    // clocks again on SDBus PowerOn.
    //
    if (powerOn) {
        vendSpec.IPG_CLK_SOFT_EN = 1;
        vendSpec.IPG_PERCLK_SOFT_EN = 1;
        vendSpec.HCLK_SOFT_EN = 1;
        vendSpec.CARD_CLK_SOFT_EN = 1;
    } else {
        vendSpec.IPG_CLK_SOFT_EN = 0;
        vendSpec.IPG_PERCLK_SOFT_EN = 0;
        vendSpec.HCLK_SOFT_EN = 0;
        vendSpec.CARD_CLK_SOFT_EN = 0;
    }

    SdhcWriteRegister(&registersPtr->VEND_SPEC, vendSpec.AsUint32);

    //
    // It is mandatory to power-cycle the SDBus to reset the card signaling
    // voltage to 3.3V if a 1.8V switch happened earlier, otherwise the card will
    // always reply to S18R with S18A=0 (i.e 1.8V switch is not accepted) since it
    // is already operating in 1.8V
    //
    if (SdhcExtPtr->DeviceProperties.SlotPowerControlSupported != FALSE) {
        NTSTATUS status = SdhcSetSdBusPower(SdhcExtPtr, powerOn);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcSetSdBusPower(
    USDHC_EXTENSION* SdhcExtPtr,
    BOOLEAN PowerOn
    )
{
    UINT32 dsmPowerFunctionIdx = USDHC_DSM_FUNCTION_IDX_SLOT_POWER_OFF;

    if (PowerOn != FALSE) {
        dsmPowerFunctionIdx = USDHC_DSM_FUNCTION_IDX_SLOT_POWER_ON;
    }

    NT_ASSERT(SdhcExtPtr->DeviceProperties.PdoPtr != nullptr);
    NTSTATUS status =
        AcpiExecuteDsmFunctionNoParams(
            SdhcExtPtr->DeviceProperties.PdoPtr,
            &USDHC_DSM_GUID,
            USDHC_DSM_REVISION_ID,
            dsmPowerFunctionIdx,
            nullptr);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            SdhcExtPtr->IfrLogHandle,
            SdhcExtPtr,
            status,
            "AcpiExecuteDsmFunctionNoParams(%d) failed",
            dsmPowerFunctionIdx);
        return status;
    }

    //
    // Give the card an empirical 100ms to stabilize the power-up/power-down.
    // According to specs, the max power ramp-up time for 2.7-3.3V power supply
    // is 35ms per specs. Host should also wait at least 1ms for the VDD supply
    // voltage to be stable after power ramp-up is complete
    //
    SdPortWait(USDHC_CARD_STABILIZATION_DELAY);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
BOOLEAN
SdhcIsSlotPowerControlSupported(
    DEVICE_OBJECT* PdoPtr
    )
{
    UINT32 supportedFunctionsMask;
    NTSTATUS status =
        AcpiQueryDsm(
            PdoPtr,
            &USDHC_DSM_GUID,
            USDHC_DSM_REVISION_ID,
            &supportedFunctionsMask);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            status,
            "AcpiQueryDsm() failed");
        return FALSE;
    }

    if (NT_SUCCESS(status) &&
        (supportedFunctionsMask & USDHC_DSM_POWER_FUNCTIONS_MASK)) {
        return TRUE;
    }

    return FALSE;
}

_Use_decl_annotations_
NTSTATUS
SdhcExecuteTuning(
    USDHC_EXTENSION* /* SdhcExtPtr */
    )
{
    return STATUS_NOT_SUPPORTED;
}

_Use_decl_annotations_
NTSTATUS
SdhcEnableBlockGapInterrupt(
    USDHC_EXTENSION* /* SdhcExtPtr */,
    BOOLEAN /* Enable */
    )
{
    return STATUS_NOT_SUPPORTED;
}

_Use_decl_annotations_
VOID
SdhcSdClockEnableInIdle(
    USDHC_EXTENSION* SdhcExtPtr,
    BOOLEAN Enable
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    USDHC_VEND_SPEC_REG vendSpec = { SdhcReadRegister(&registersPtr->VEND_SPEC) };

    //
    // uSDHC tends to gate-off the SD clock (SDCLK) fed to the SDCard when idle,
    // setting this bit forces the SDCLK to be active all the time and disable the
    // auto gating behavior of uSDHC. Such auto gating-on/off behavior causes problems
    // in situations when the SDCard expects the SDCLK to be fed in specific 
    // situations during the identification and initialization phase even if the uSDHC
    // is idle
    //
    if (Enable) {
        vendSpec.FRC_SDCLK_ON = 1;
    } else {
        vendSpec.FRC_SDCLK_ON = 0;
    }

    SdhcWriteRegister(&registersPtr->VEND_SPEC, vendSpec.AsUint32);
}

_Use_decl_annotations_
NTSTATUS
SdhcWaitForStableSdClock(
    USDHC_EXTENSION* SdhcExtPtr
    )
{
    volatile USDHC_REGISTERS* registersPtr = SdhcExtPtr->RegistersPtr;
    USDHC_PRES_STATE_REG presState = { SdhcReadRegister(&registersPtr->PRES_STATE) };
    UINT32 retries = USDHC_POLL_RETRY_COUNT;

    while (!presState.SDSTB &&
           retries) {
        SdPortWait(USDHC_POLL_WAIT_TIME_US);
        --retries;
        presState.AsUint32 = SdhcReadRegister(&registersPtr->PRES_STATE);
    }

    if (!presState.SDSTB) {
        NT_ASSERT(!retries);
        USDHC_LOG_ERROR(
            SdhcExtPtr->IfrLogHandle,
            SdhcExtPtr,
            "Time-out waiting on SD clock to stabilize");
        return STATUS_IO_TIMEOUT;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SdhcReadDeviceProperties(
    DEVICE_OBJECT* PdoPtr,
    USDHC_DEVICE_PROPERTIES** DevPropsPptr
    )
{
    ASSERT_MAX_IRQL(APC_LEVEL);

    ACPI_EVAL_OUTPUT_BUFFER UNALIGNED* dsdBufferPtr = nullptr;
    NTSTATUS status;

    USDHC_DEVICE_PROPERTIES* devPropsPtr =
        DevicePropertiesListSafeAddEntry();
    if (devPropsPtr == nullptr) {
        USDHC_LOG_ERROR(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            "Failed to allocate a Device Properties entry for SDHC PDO:0x%p",
            PdoPtr);
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Cleanup;
    }

    devPropsPtr->PdoPtr = PdoPtr;

    status = AcpiQueryDsd(PdoPtr, &dsdBufferPtr);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            status,
            "AcpiQueryDsd() failed");
        goto Cleanup;
    }

    const ACPI_METHOD_ARGUMENT UNALIGNED* devicePropertiesPkgPtr;
    status = AcpiParseDsdAsDeviceProperties(dsdBufferPtr, &devicePropertiesPkgPtr);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status =
        AcpiDevicePropertiesQueryIntegerValue(
            devicePropertiesPkgPtr,
            "RegisterBasePA",
            &devPropsPtr->Key);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            status,
            "AcpiDevicePropertiesQueryIntegerValue(RegisterBasePA) failed");
        goto Cleanup;
    }

    status =
        AcpiDevicePropertiesQueryIntegerValue(
            devicePropertiesPkgPtr,
            "BaseClockFrequencyHz",
            &devPropsPtr->BaseClockFrequencyHz);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            status,
            "AcpiDevicePropertiesQueryIntegerValue(BaseClockFrequencyHz) failed");
        goto Cleanup;
    }

    status =
        AcpiDevicePropertiesQueryIntegerValue(
            devicePropertiesPkgPtr,
            "Regulator1V8Exist",
            &devPropsPtr->Regulator1V8Exist);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            status,
            "AcpiDevicePropertiesQueryIntegerValue(Regulator1V8Exist) failed");
        goto Cleanup;
    }

    status =
        AcpiDevicePropertiesQueryIntegerValue(
            devicePropertiesPkgPtr,
            "SlotCount",
            &devPropsPtr->SlotCount);
    if (!NT_SUCCESS(status)) {
        USDHC_LOG_ERROR_STATUS(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            status,
            "AcpiDevicePropertiesQueryIntegerValue(SlotCount) failed");
        goto Cleanup;
    }

    //
    // Supporting slot power control is board design specific, and
    // not all uSDHCs will support slot power control via firmware
    //
    devPropsPtr->SlotPowerControlSupported =
        SdhcIsSlotPowerControlSupported(PdoPtr);
    if (devPropsPtr->SlotPowerControlSupported == FALSE) {
        USDHC_LOG_INFORMATION(
            DriverLogHandle,
            NullPrivateExtensionPtr,
            "Slot power control not supported, 1.8V switching may not work as expected");
    }

    if (ARGUMENT_PRESENT(DevPropsPptr)) {
        *DevPropsPptr = devPropsPtr;
    }

Cleanup:

    if (dsdBufferPtr != nullptr) {
        ExFreePoolWithTag(dsdBufferPtr, ACPI_TAG_EVAL_OUTPUT_BUFFER);
    }

    if (!NT_SUCCESS(status) && (devPropsPtr != nullptr)) {
        NT_ASSERT(devPropsPtr->IsAllocated);
        DevicePropertiesListSafeRemoveEntry(devPropsPtr);
    }

    return status;
}

_Use_decl_annotations_
VOID
SdhcLogInit(
    USDHC_EXTENSION* SdhcExtPtr
    )
{
    //
    // After WPP is enabled for the driver, WPP creates a default IFR log. The buffer size of the 
    // default log to which WPP prints is 4096 bytes. For a debug build, the buffer is 24576 bytes.
    //
    // If we are to use the default IFR log created by WPP, we would
    // do it like the commented code below
    //

    RECORDER_LOG_CREATE_PARAMS recorderLogCreateParams;
    RECORDER_LOG_CREATE_PARAMS_INIT(&recorderLogCreateParams, nullptr);

    //
    // NOTE: Actual log size may be adjusted down by WPP
    //
    recorderLogCreateParams.TotalBufferSize = USDHC_RECORDER_TOTAL_BUFFER_SIZE;
    recorderLogCreateParams.ErrorPartitionSize = USDHC_RECORDER_ERROR_PARTITION_SIZE;
    RtlStringCchPrintfA(recorderLogCreateParams.LogIdentifier,
                    sizeof(recorderLogCreateParams.LogIdentifier),
                    USDHC_RECORDER_LOG_ID,
                    SdhcExtPtr->PhysicalAddress);

    NTSTATUS status = WppRecorderLogCreate(
        &recorderLogCreateParams,
        &SdhcExtPtr->IfrLogHandle);
    if (!NT_SUCCESS(status)) {
        NT_ASSERTMSG("Unable to create trace log recorder - default log will be used instead", FALSE);
        SdhcExtPtr->IfrLogHandle = WppRecorderLogGetDefault();
    }
}

_Use_decl_annotations_
VOID
SdhcLogCleanup(
    SD_MINIPORT* MiniportPtr
    )
{
    USDHC_EXTENSION* sdhcExtPtr;

    //
    // Iterate over all uSDHC instances and delete their IFR logs
    //
    for (LONG i = 0; i < MiniportPtr->SlotCount; ++i) {
        sdhcExtPtr = reinterpret_cast<USDHC_EXTENSION*>(
            MiniportPtr->SlotExtensionList[i]->PrivateExtension);
        if (sdhcExtPtr->IfrLogHandle) {
            WppRecorderLogDelete(sdhcExtPtr->IfrLogHandle);
        }
    }
}

NONPAGED_SEGMENT_END; //======================================================
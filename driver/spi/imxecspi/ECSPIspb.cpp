// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPIspb.cpp
//
// Abstract:
//
//    This module contains the implementation of the IMX ECSPI controller driver
//    SpbCx callback functions.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//
#include "precomp.h"
#pragma hdrstop

#define _ECSPI_SPB_CPP_

// Logging header files
#include "ECSPItrace.h"
#include "ECSPIspb.tmh"

// Common driver header files
#include "ECSPIcommon.h"

// Module specific header files
#include "ECSPIhw.h"
#include "ECSPIspb.h"
#include "ECSPIdevice.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, ECSPIEvtSpbTargetConnect)
    #pragma alloc_text(PAGE, ECSPIEvtSpbTargetDisconnect)
#endif


//
// Routine Description:
//
//  ECSPIEvtSpbTargetConnect is called by the framework when a peripheral 
//  driver opens a target. 
//  The routine retrieves and validates target settings. 
//  If settings are valid, target context is updated
//
// Arguments:
//
//  Controller - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
// Return Value:
//
//  NTSTATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPIEvtSpbTargetConnect (
    WDFDEVICE Controller,
    SPBTARGET SpbTarget
    )
{
    PAGED_CODE();

    ECSPI_DEVICE_EXTENSION* devExtPtr = ECSPIDeviceGetExtension(Controller);

    //
    // Get target connection parameters.
    //
    const PNP_SERIAL_BUS_DESCRIPTOR* serialBusDescriptor;
    const PNP_SPI_SERIAL_BUS_DESCRIPTOR* spiSerialBusDescriptorPtr;
    {
        SPB_CONNECTION_PARAMETERS spbConnectionParams;
        SPB_CONNECTION_PARAMETERS_INIT(&spbConnectionParams);
        SpbTargetGetConnectionParameters(SpbTarget, &spbConnectionParams);

        RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER* connectionPropPtr =
            static_cast<RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER*>(
                spbConnectionParams.ConnectionParameters
                );
        if (connectionPropPtr->PropertiesLength < sizeof(PNP_SPI_SERIAL_BUS_DESCRIPTOR)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "Invalid connection parameters "
                "for PNP_SPI_SERIAL_BUS_DESCRIPTOR! "
                "Size %lu, required size %lu",
                connectionPropPtr->PropertiesLength,
                sizeof(PNP_SPI_SERIAL_BUS_DESCRIPTOR)
                );
            return STATUS_INVALID_PARAMETER;
        }

        spiSerialBusDescriptorPtr =
            reinterpret_cast<const PNP_SPI_SERIAL_BUS_DESCRIPTOR*>(
                &connectionPropPtr->ConnectionProperties
                );
        serialBusDescriptor = &spiSerialBusDescriptorPtr->SerialBusDescriptor;
        if (serialBusDescriptor->SerialBusType != PNP_SERIAL_BUS_TYPE_SPI) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "Invalid connection parameters, not a SPI descriptor!"
                "Type 0x%lX, required type 0x%lX",
                serialBusDescriptor->SerialBusType,
                PNP_SERIAL_BUS_TYPE_SPI
                );
            return STATUS_INVALID_PARAMETER;
        }

    } // Get target connection parameters.

    if ((serialBusDescriptor->GeneralFlags & PNP_SERIAL_GENERAL_FLAGS_SLV_BIT)
        != 0) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "Slave mode not supported."
            );
        return STATUS_NOT_SUPPORTED;
    }

    if ((serialBusDescriptor->TypeSpecificFlags & PNP_SPI_WIREMODE_BIT) != 0) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "3-wire mode not supported."
            );
        return STATUS_NOT_SUPPORTED;
    }

    if (!ECSPIHwIsSupportedDataBitLength(
            spiSerialBusDescriptorPtr->DataBitLength)
            ) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "Data bit length of %d is not supported! "
            "Driver supports 8/16/32 data bit length.",
            spiSerialBusDescriptorPtr->DataBitLength
            );
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Validate connection speed
    //
    {
        ULONG minClockHz, maxClockHz;
        ECSPIHwGetClockRange(&minClockHz, &maxClockHz);

        if ((spiSerialBusDescriptorPtr->ConnectionSpeed > maxClockHz) ||
            (spiSerialBusDescriptorPtr->ConnectionSpeed < minClockHz)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "Connection speed %luHz is not supported. "
                "Supported range: %luHz..%luHz",
                spiSerialBusDescriptorPtr->ConnectionSpeed,
                minClockHz,
                maxClockHz
                );
            return STATUS_NOT_SUPPORTED;
        }

    } // Validate connection speed

    if (spiSerialBusDescriptorPtr->DeviceSelection >= ECSPI_CHANNEL::COUNT) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "Device selection %d, is not supported. "
            "Supported device selection: %d..%d",
            spiSerialBusDescriptorPtr->DeviceSelection,
            ECSPI_CHANNEL::CS0,
            ECSPI_CHANNEL::CS3
            );
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Initialize TARGET context.
    //
    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);
    {
        ECSPI_TARGET_SETTINGS* trgSettingsPtr = &trgCtxPtr->Settings;

        trgCtxPtr->SpbTarget = SpbTarget;
        trgCtxPtr->DevExtPtr = devExtPtr;

        trgSettingsPtr->ConnectionSpeed =
            spiSerialBusDescriptorPtr->ConnectionSpeed;
        trgSettingsPtr->Polarity = spiSerialBusDescriptorPtr->Polarity;
        trgSettingsPtr->Phase = spiSerialBusDescriptorPtr->Phase;
        trgSettingsPtr->DeviceSelection = 
            spiSerialBusDescriptorPtr->DeviceSelection;
        trgSettingsPtr->DataBitLength = 
            spiSerialBusDescriptorPtr->DataBitLength;

        trgSettingsPtr->BufferStride = 
            ECSPISpbGetBufferStride(trgSettingsPtr->DataBitLength);
        
        if ((serialBusDescriptor->TypeSpecificFlags &
             PNP_SPI_FLAGS::PNP_SPI_DEVICEPOLARITY_BIT) != 0) {

            trgSettingsPtr->CsActiveValue = 1;

        } else {

            trgSettingsPtr->CsActiveValue = 0;
        }

        ECSPI_LOG_INFORMATION(
            devExtPtr->IfrLogHandle,
            "New target settings: Target %p, "
            "Channel %lu, flags (gen/type) 0x%lX/0x%lX, speed %luHz, "
            "Data length %lu bits, polarity %lu, phase %lu",
            trgCtxPtr,
            trgSettingsPtr->DeviceSelection,
            serialBusDescriptor->GeneralFlags,
            serialBusDescriptor->TypeSpecificFlags,
            trgSettingsPtr->ConnectionSpeed,
            trgSettingsPtr->DataBitLength,
            trgSettingsPtr->Polarity,
            trgSettingsPtr->Phase
            );

    } // Initialize TARGET context.

    NTSTATUS status = ECSPIDeviceOpenGpioTarget(trgCtxPtr);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "ECSPIDeviceOpenGpioTarget failed, "
            "wdfDevice = %p, status = %!STATUS!",
            Controller,
            status
        );
        return status;
    }

    //
    // Calculate the HW setup.
    //
    status = ECSPIHwSetTargetConfiguration(trgCtxPtr);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "ECSPIHwSetTargetConfiguration failed, "
            "wdfDevice = %p, status = %!STATUS!",
            Controller,
            status
            );
        ECSPIDeviceCloseGpioTarget(trgCtxPtr);
        return status;
    }

    //
    // Associate the new target with the 
    // request object.
    //
    devExtPtr->CurrentRequest.SpbTargetPtr = trgCtxPtr;

    status = ECSPIDeviceNegateCS(trgCtxPtr, TRUE);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "ECSPIDeviceNegateCS failed, "
            "wdfDevice = %p, status = %!STATUS!",
            Controller,
            status
            );
        ECSPIDeviceCloseGpioTarget(trgCtxPtr);
        return status;
    }

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPIEvtSpbTargetDisconnect is called by the framework when a peripheral 
//  driver closes a target. 
//  The routine closes the GPIO target, if it was opened. 
//
// Arguments:
//
//  Controller - The WdfDevice object the represent the ECSPI instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
// Return Value:
//
//  NTSTATUS
//
VOID
ECSPIEvtSpbTargetDisconnect (
    WDFDEVICE /*Controller*/,
    SPBTARGET SpbTarget
    )
{
    PAGED_CODE();

    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);

    ECSPIDeviceCloseGpioTarget(trgCtxPtr);
}


//
// Routine Description:
//
//  ECSPIEvtSpbControllerLock is called by the framework to lock bus access
//  to a specific target.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
//  SpbRequestLock - The Lock SPB request
//
// Return Value:
//
//  Through SpbRequest completion.
//
_Use_decl_annotations_
VOID
ECSPIEvtSpbControllerLock (
    WDFDEVICE /*WdfDevice*/,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest
    )
{
    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);
    ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;

    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        !devExtPtr->IsControllerLocked &&
            (devExtPtr->LockUnlockRequest == NULL)
        );

    devExtPtr->LockUnlockRequest = SpbRequest;
    devExtPtr->IsControllerLocked = TRUE;

    NTSTATUS status = ECSPIDeviceAssertCS(trgCtxPtr, FALSE);
    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        NT_SUCCESS(status)
        );
}


//
// Routine Description:
//
//  ECSPIEvtSpbControllerLock is called by the framework to lock bus access
//  to a specific target.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
//  SpbRequestLock - The Unlock SPB request
//
// Return Value:
//
//  Through SpbRequest completion.
//
_Use_decl_annotations_
VOID
ECSPIEvtSpbControllerUnlock (
    WDFDEVICE /*WdfDevice*/,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest
    )
{
    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);
    ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;

    ECSPIHwUnselectTarget(trgCtxPtr);

    devExtPtr->LockUnlockRequest = SpbRequest;
    devExtPtr->IsControllerLocked = FALSE;

    NTSTATUS status = ECSPIDeviceNegateCS(trgCtxPtr, FALSE);
    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        NT_SUCCESS(status)
        );
}


//
// Routine Description:
//
//  ECSPIEvtSpbIoRead is called by the framework to perform a read
//  transfer from a target.
//  The routine retrieves the request parameters, prepares the HW and
//  enables the RX interrupts.
//  Handling of the read request continues in ISR/DPC.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
//  SpbRequest - The SPB READ request.
//
//  Length - Number of bytes to read.
//
// Return Value:
//
//  Through SpbRequest completion.
//
_Use_decl_annotations_
VOID
ECSPIEvtSpbIoRead (
    WDFDEVICE /*WdfDevice*/,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest,
    size_t Length
    )
{
    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);
    const ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;

    ECSPI_LOG_INFORMATION(
        devExtPtr->IfrLogHandle,
        "READ request: target %p, request %p, length %Iu",
        trgCtxPtr,
        SpbRequest,
        Length
        );

    ECSPIpSpbInitiateIo(
        trgCtxPtr,
        SpbRequest,
        ECSPI_REQUEST_TYPE::READ
        );
}


//
// Routine Description:
//
//  ECSPIEvtSpbIoWrite is called by the framework to perform a write
//  transfer to a target.
//  The routine retrieves the request parameters, prepares the HW, 
//  write the first bytes to TX FIFO, and enables the TX interrupts.
//  Handling of the write request continues in ISR/DPC.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
//  SpbRequestLock - The SPB WRITE request.
//
//  Length - Number of bytes to write.
//
// Return Value:
//
//  Through SpbRequest completion.
//
_Use_decl_annotations_
VOID
ECSPIEvtSpbIoWrite (
    WDFDEVICE /*WdfDevice*/,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest,
    size_t Length
    )
{
    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);
    const ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;

    ECSPI_LOG_INFORMATION(
        devExtPtr->IfrLogHandle,
        "WRITE request: target %p, request %p, length %Iu",
        trgCtxPtr,
        SpbRequest,
        Length
        );

    ECSPIpSpbInitiateIo(
        trgCtxPtr, 
        SpbRequest, 
        ECSPI_REQUEST_TYPE::WRITE
        );
}


//
// Routine Description:
//
//  ECSPIEvtSpbIoSequence is called by the framework to perform a sequence of
//  transfers to/from a target.
//  The routine retrieves the request parameters, prepares the HW and
//  enables the RX interrupts.
//  Handling of the read/write transfers continues in ISR/DPC.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
//  SpbRequest - The SPB SEQUENCE request.
//
//  TransferCount - Number of transfers.
//
// Return Value:
//
//  Through SpbRequest completion.
//
_Use_decl_annotations_
VOID
ECSPIEvtSpbIoSequence (
    WDFDEVICE /*WdfDevice*/,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest,
    ULONG TransferCount
    )
{
    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);
    const ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;

    ECSPI_LOG_INFORMATION(
        devExtPtr->IfrLogHandle,
        "Sequence request: target %p, request %p, transfers %lu",
        trgCtxPtr,
        SpbRequest,
        TransferCount
        );

    ECSPIpSpbInitiateIo(
        trgCtxPtr,
        SpbRequest,
        ECSPI_REQUEST_TYPE::SEQUENCE
        );
}


//
// Routine Description:
//
//  ECSPIEvtSpbIoOther is called by the framework to handle custom
//  IO control requests. 
//  SPB uses this path for handling IOCTL_SPB_FULL_DUPLEX.
//  The routine retrieves the request parameters, prepares the HW and
//  enables interrupts.
//  Handling of the read request continues in ISR/DPC.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  SpbTarget - The SPB target object
//
//  SpbRequest - The SPB SEQUENCE request.
//
//  OutputBufferLength - Output buffer length.
//
//  InputBufferLength - Input buffer length.
//
//  IoControlCode - IO control code
//
// Return Value:
//
//  Through SpbRequest completion.
//
_Use_decl_annotations_
VOID 
ECSPIEvtSpbIoOther (
    WDFDEVICE /*WdfDevice*/,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest,
    size_t /*OutputBufferLength*/,
    size_t /*InputBufferLength*/,
    ULONG IoControlCode
    )
{
    ECSPI_TARGET_CONTEXT* trgCtxPtr = ECSPITargetGetContext(SpbTarget);
    const ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;

    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        IoControlCode == IOCTL_SPB_FULL_DUPLEX
        );

    ECSPI_LOG_INFORMATION(
        devExtPtr->IfrLogHandle,
        "FULL_DUPLEX request: target %p, request %p",
        trgCtxPtr,
        SpbRequest
        );

    ECSPIpSpbInitiateIo(
        trgCtxPtr,
        SpbRequest,
        ECSPI_REQUEST_TYPE::FULL_DUPLEX
        );
}


//
// Routine Description:
//
//  ECSPISpbPrepareNextTransfer is called by either ECSPIpSpbPrepareRequest or
//  by ECSPIEvtInterruptDpc to prepare the next transfers(s).
//  If the request has multiple transfers, the routine tries to prepare as many
//  as it can. 
//
// Arguments:
//
//  RequestPtr - The SPB request context.
//
// Return Value:
//
//  NTSTATUS: STATUS_SUCCESS, or STATUS_NO_MORE_FILES if no
//      transfers were prepared.
//
_Use_decl_annotations_
NTSTATUS
ECSPISpbPrepareNextTransfer (
    ECSPI_SPB_REQUEST* RequestPtr
    )
{
    const ECSPI_DEVICE_EXTENSION* devExtPtr = 
        RequestPtr->SpbTargetPtr->DevExtPtr;
    const ECSPI_TARGET_SETTINGS* trgSettingsPtr = 
        &RequestPtr->SpbTargetPtr->Settings;

    //
    // Calculate how many transfers we can prepare
    //
    ULONG maxTransferToPrepare = MAX_PREPARED_TRANSFERS_COUNT - 
        ECSPISpbGetReadyTransferCount(RequestPtr);
    ULONG transfersToPrepare = 
        RequestPtr->TransferCount - RequestPtr->NextTransferIndex;
    transfersToPrepare = min(transfersToPrepare, maxTransferToPrepare);

    //
    // Prepare the next 'transfersToPrepare' transfers
    //
    ULONG preparedTransfers = 0;
    ULONG transferIn = RequestPtr->TransferIn;
    for (ULONG xferIndex = 0;
         xferIndex < transfersToPrepare;
         ++xferIndex) {
        
        ECSPI_SPB_TRANSFER* reqXferPtr =  &RequestPtr->Transfers[transferIn];

        //
        // Initialize transfer parameters
        //
        RtlZeroMemory(reqXferPtr, sizeof(*reqXferPtr));

        PMDL baseMdlPtr;
        SPB_TRANSFER_DESCRIPTOR_INIT(&reqXferPtr->SpbTransferDescriptor);
        SpbRequestGetTransferParameters(
            RequestPtr->SpbRequest,
            RequestPtr->NextTransferIndex + xferIndex,
            &reqXferPtr->SpbTransferDescriptor,
            &baseMdlPtr
            );

        if ((reqXferPtr->SpbTransferDescriptor.TransferLength %
            trgSettingsPtr->BufferStride) != 0) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "Invalid transfer length (%Iu). "
                "Transfer length should we a integer product of %d.",
                reqXferPtr->SpbTransferDescriptor.TransferLength,
                trgSettingsPtr->BufferStride
                );
            return STATUS_INVALID_PARAMETER;
        }

        if (reqXferPtr->SpbTransferDescriptor.DelayInUs != 0) {
            //
            // The ECSPI supports delay between bursts.
            // Since we use a single burst, delay is not supported.
            //
            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "Transfer delay is not supported!"
                );
            return STATUS_NOT_SUPPORTED;
        }

        reqXferPtr->CurrentMdlPtr = baseMdlPtr;
        reqXferPtr->BufferStride = trgSettingsPtr->BufferStride;
        reqXferPtr->AssociatedRequestPtr = RequestPtr;
        reqXferPtr->BurstLength = min(
            reqXferPtr->SpbTransferDescriptor.TransferLength,
            ECSPI_MAX_BURST_LENGTH_BYTES
            );
        reqXferPtr->BytesLeftInBurst = reqXferPtr->BurstLength;
		reqXferPtr->BurstWords = ECSPISpbWordsLeftInBurst(reqXferPtr);
        ECSPI_LOG_INFORMATION(
            devExtPtr->IfrLogHandle,
            "Preparing transfer %p: target %p, request %p ,"
            "type %!REQUESTTYPE!, direction %s, "
            "length %Iu, delay %lu uSec",
            reqXferPtr,
            RequestPtr->SpbTargetPtr,
            RequestPtr,
            RequestPtr->Type,
            DIR2STR(reqXferPtr->SpbTransferDescriptor.Direction),
            reqXferPtr->SpbTransferDescriptor.TransferLength,
            reqXferPtr->SpbTransferDescriptor.DelayInUs
            );

        //
        // Map MDL(s)
        //
        for (PMDL mdlPtr = baseMdlPtr;
             mdlPtr != nullptr;
             mdlPtr = mdlPtr->Next) {

            ULONG pagePriority = NormalPagePriority | MdlMappingNoExecute;
            if (ECSPISpbIsWriteTransfer(reqXferPtr)) {

                pagePriority |= MdlMappingNoWrite;
            }

            VOID const* mdlVaPtr = MmGetSystemAddressForMdlSafe(
                mdlPtr, 
                pagePriority
                );
            if (mdlVaPtr == nullptr) {

                ECSPI_LOG_ERROR(
                    devExtPtr->IfrLogHandle,
                    "MmGetSystemAddressForMdlSafe failed. "
                    "request %p, MDL %p",
                    RequestPtr->SpbRequest,
                    mdlPtr
                    );
                return STATUS_INSUFFICIENT_RESOURCES;
            }

        } // More MDLs

        transferIn = (transferIn + 1) % MAX_PREPARED_TRANSFERS_COUNT;
        ++preparedTransfers;

        InterlockedIncrement(&RequestPtr->ReadyTransferCount);

    } // More transfers to initialize

    RequestPtr->TransferIn = transferIn;
    RequestPtr->NextTransferIndex += preparedTransfers;

    return preparedTransfers != 0 ? STATUS_SUCCESS : STATUS_NO_MORE_FILES;
}


//
// Routine Description:
//
//  ECSPISpbStartNextTransfer is called to start the next IO transfer.
//  The routine prepares the HW and starts the transfer.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  RequestPtr - The SPB request context.
//
// Return Value:
//
//  NTSTATUS: STATUS_SUCCESS, or STATUS_NO_MORE_FILES if there are no
//      more prepared transfers.
//
_Use_decl_annotations_
NTSTATUS
ECSPISpbStartNextTransfer (
    ECSPI_SPB_REQUEST* RequestPtr
    )
{
    if (ECSPISpbGetReadyTransferCount(RequestPtr) == 0) {

        return STATUS_NO_MORE_FILES;
    }

    ECSPI_TARGET_CONTEXT* spbTargetPtr = RequestPtr->SpbTargetPtr;
    ECSPI_DEVICE_EXTENSION* devExtPtr = spbTargetPtr->DevExtPtr;

    ECSPI_SPB_TRANSFER* activeXfer1Ptr;
    ECSPI_SPB_TRANSFER* activeXfer2Ptr;
    ECSPISpbGetActiveTransfers(RequestPtr, &activeXfer1Ptr, &activeXfer2Ptr);

    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        activeXfer1Ptr->CurrentMdlPtr != nullptr
        );

    ECSPI_LOG_TRACE(
        devExtPtr->IfrLogHandle,
        "Starting transfer %p (1/%d): target %p, request %p, "
        "type %!REQUESTTYPE!, direction %s, "
        "length %Iu, delay %lu uSec",
        activeXfer1Ptr,
        activeXfer2Ptr == nullptr ? 1 : 2,
        spbTargetPtr,
        RequestPtr,
        RequestPtr->Type,
        DIR2STR(activeXfer1Ptr->SpbTransferDescriptor.Direction),
        activeXfer1Ptr->SpbTransferDescriptor.TransferLength,
        activeXfer1Ptr->SpbTransferDescriptor.DelayInUs
        );
    if (activeXfer2Ptr != nullptr) {

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            activeXfer2Ptr->CurrentMdlPtr != nullptr
            );

        ECSPI_LOG_TRACE(
            devExtPtr->IfrLogHandle,
            "Starting transfer %p (1/2): target %p, request %p, "
            "type %!REQUESTTYPE!, direction %s, "
            "length %Iu, delay %lu uSec",
            activeXfer2Ptr,
            spbTargetPtr,
            RequestPtr,
            RequestPtr->Type,
            DIR2STR(activeXfer2Ptr->SpbTransferDescriptor.Direction),
            activeXfer2Ptr->SpbTransferDescriptor.TransferLength,
            activeXfer2Ptr->SpbTransferDescriptor.DelayInUs
            );
    }

    ECSPIHwClearFIFOs(devExtPtr);  // only clears Rx fifo

    //
    // Configure the HW with the transfer(s) parameters
    //
    ULONG interruptMaskForTransfer = 0;
    ECSPIHwConfigureTransfer(
        devExtPtr, 
        activeXfer1Ptr, 
        TRUE, // Initial setup
        &interruptMaskForTransfer
        );

    if (activeXfer2Ptr != nullptr) {

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            ECSPISpbIsWriteTransfer(activeXfer1Ptr)
            );
        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            !ECSPISpbIsWriteTransfer(activeXfer2Ptr)
            );

        ECSPIHwConfigureTransfer(
            devExtPtr, 
            activeXfer2Ptr, 
            FALSE, // Add to current setup
            &interruptMaskForTransfer
            );
    }

    if (ECSPISpbIsWriteTransfer(activeXfer1Ptr)) {
        //
        // On a write transfer, write the first chunk.
        //
        (void)ECSPIHwWriteTxFIFO(devExtPtr, activeXfer1Ptr);

    } else {
        //
        // On a read transfer, write 0s to clock in RX data.
        //
        ECSPIHwWriteZerosTxFIFO(devExtPtr, activeXfer1Ptr);
    }

    //
    // Allow ISR to take over
    //
    (void)ECSPIHwInterruptControl(
        devExtPtr,
        TRUE, // Enable
        interruptMaskForTransfer
        );

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPISpbStartTransferSafe starts the next transfer 
//  synchronized with the cancel routine.
//
// Arguments:
//
//  RequestPtr - The request object.
//
// Return Value:
//
//  NTSTATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPISpbStartTransferSafe (
    ECSPI_SPB_REQUEST* RequestPtr
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = RequestPtr->SpbTargetPtr->DevExtPtr;

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->DeviceLock, &lockHandle);

    NTSTATUS status = ECSPISpbStartNextTransfer(RequestPtr);

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    return status;
}


//
// Routine Description:
//
//  ECSPISpbGetActiveTransfers returns the active transfers of a give request.
//  Only FULL_DUPLEX request has 2 active requests, all other request types have 1.
//
// Arguments:
//
//  RequestPtr - The Request context.
//
//  Transfer1PPtr - The 1st active transfer.
//
//  Transfer2PPtr - The 2nd active transfer.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPISpbGetActiveTransfers(
    ECSPI_SPB_REQUEST* RequestPtr,
    ECSPI_SPB_TRANSFER** Transfer1PPtr,
    ECSPI_SPB_TRANSFER** Transfer2PPtr
    )
{
    *Transfer1PPtr = &RequestPtr->Transfers[RequestPtr->TransferOut];
    *Transfer2PPtr = nullptr;

    switch (RequestPtr->Type) {
    case ECSPI_REQUEST_TYPE::READ:
    case ECSPI_REQUEST_TYPE::WRITE:
    case ECSPI_REQUEST_TYPE::SEQUENCE:
        break;

    case ECSPI_REQUEST_TYPE::FULL_DUPLEX:
    {
        ULONG nextXferIndex = (RequestPtr->TransferOut + 1) %
            MAX_PREPARED_TRANSFERS_COUNT;
        *Transfer2PPtr = &RequestPtr->Transfers[nextXferIndex];
        break;

    } // ECSPI_REQUEST_TYPE::FULL_DUPLEX

    default:
        NT_ASSERT(FALSE);
    } // switch (...)
}


//
// Routine Description:
//
//  ECSPISpbCompleteTransfer is called when a SEQUENCE transfer is complete.
//  The routines attempts to start the next transfer if available.
//
// Arguments:
//
//  TransferPtr - The completed transfer context.
//      the ECSPI controller.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPISpbCompleteSequenceTransfer (
    ECSPI_SPB_TRANSFER* TransferPtr
    )
{
    ECSPI_SPB_REQUEST* requestPtr = TransferPtr->AssociatedRequestPtr;
    ECSPI_DEVICE_EXTENSION* devExtPtr = requestPtr->SpbTargetPtr->DevExtPtr;

    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        requestPtr->Type == ECSPI_REQUEST_TYPE::SEQUENCE
        );
    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        TransferPtr == &requestPtr->Transfers[requestPtr->TransferOut]
        );

    ECSPI_LOG_TRACE(
        devExtPtr->IfrLogHandle,
        "Sequence transfer %p completed, %d left. Target %p, request %p, "
        "type %!REQUESTTYPE!, direction %s, length %Iu",
        TransferPtr,
        requestPtr->TransfersLeft,
        requestPtr->SpbTargetPtr,
        requestPtr,
        requestPtr->Type,
        DIR2STR(TransferPtr->SpbTransferDescriptor.Direction),
        TransferPtr->SpbTransferDescriptor.TransferLength
        );

    ECSPIHwDisableTransferInterrupts(devExtPtr, TransferPtr);

    InterlockedDecrement(
        reinterpret_cast<volatile LONG*>(&requestPtr->ReadyTransferCount)
        );
    requestPtr->TransfersLeft -= 1;

    requestPtr->TransferOut =
        (requestPtr->TransferOut + 1) % MAX_PREPARED_TRANSFERS_COUNT;

    //
    // If request has more transfers, start the next one...
    //
    if (requestPtr->TransfersLeft != 0) {

        if (ECSPISpbStartNextTransfer(requestPtr) == STATUS_NO_MORE_FILES) {
            //
            // Mark that transfer is idle due to lack of prepared transfers, since
            // transfers can only be prepared at IRQL <= DISPATCH_LEVEL.
            // When a request is marked as 'idle', DPC knows it needs 
            // to start the next transfer after preparing it.
            //
            InterlockedExchange(&requestPtr->IsIdle, 1);
        }
    }
}

//
// Routine Description:
//
//  ECSPIDeviceCompleteTrasnferRequest is called to complete the
//  current transfer request.
//
// Arguments:
//
//  RequestPtr - The request to complete
//
//  Status - Request status
//
//  Information - Request information (number of bytes transferred).
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPISpbCompleteTransferRequest (
    ECSPI_SPB_REQUEST* RequestPtr,
    NTSTATUS Status,
    ULONG_PTR Information
    )
{
    SPBREQUEST spbRequest = static_cast<SPBREQUEST>(
        InterlockedExchangePointer(
            reinterpret_cast<PVOID volatile *>(&RequestPtr->SpbRequest),
            nullptr
        ));

    if (spbRequest != NULL) {

        RequestPtr->Type = ECSPI_REQUEST_TYPE::INVALID;
        RequestPtr->State = ECSPI_REQUEST_STATE::INACTIVE;

        WdfRequestSetInformation(spbRequest, Information);
        SpbRequestComplete(spbRequest, Status);
    }
}


//
// Routine Description:
//
//  ECSPISpbAbortAllTransfers is called to abort all active transfers.
//  The routine resets the block.
//
// Arguments:
//
//  RequestPtr - The request who's transfers should be aborted.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPISpbAbortAllTransfers (
    _In_ ECSPI_SPB_REQUEST* RequestPtr
    )
{
    ECSPI_TARGET_CONTEXT* trgCtxPtr = RequestPtr->SpbTargetPtr;
    ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;

    WdfInterruptAcquireLock(devExtPtr->WdfSpiInterrupt);

    ECSPIHwUnselectTarget(trgCtxPtr);
    
    RequestPtr->ReadyTransferCount = 0;

    WdfInterruptReleaseLock(devExtPtr->WdfSpiInterrupt);
}


// 
// ECSPIspb private methods.
// ------------------------
//


//
// Routine Description:
//
//  ECSPIpSpbInitiateIo is called by SPB IO callback function to prepare 
//  a SPB request for IO, and initiate the transfer(s).
//  If the process fails the routine completes the request.
//
// Arguments:
//
//  TrgCtxPtr - The SPB target context.
//
//  SpbRequest - The SPB request.
//
//  RequestType - The request type
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIpSpbInitiateIo (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr,
    SPBREQUEST SpbRequest,
    ECSPI_REQUEST_TYPE RequestType
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;

    NTSTATUS status = ECSPIpSpbPrepareRequest(TrgCtxPtr, SpbRequest, RequestType);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "ECSPIpSpbPrepareRequest failed. "
            "request %p, status = %!STATUS!",
            SpbRequest,
            status
            );
        ECSPISpbCompleteTransferRequest(&devExtPtr->CurrentRequest, status, 0);
        return;
    }

    if (devExtPtr->IsControllerLocked) {
        //
        // Controller is locked, CS is already asserted,
        // just start the transfer...
        //
        status = ECSPIDeviceEnableRequestCancellation(&devExtPtr->CurrentRequest);
        if (!NT_SUCCESS(status)) {

            return;
        }

        status = ECSPISpbStartTransferSafe(&devExtPtr->CurrentRequest);
        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            NT_SUCCESS(status)
            );
        return;
    }

    status = ECSPIDeviceAssertCS(TrgCtxPtr, FALSE);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "ECSPISpbStartNextTransfer failed. "
            "request %p, status = %!STATUS!",
            SpbRequest,
            status
            );
        ECSPISpbCompleteTransferRequest(&devExtPtr->CurrentRequest, status, 0);
        return;
    }
}


//
// Routine Description:
//
//  ECSPIpSpbPrepareRequest is called by ECSPIpSpbInitiateIo to prepare 
//  a SPB request for IO.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  TrgCtxPtr - The SPB target context.
//
//  SpbRequest - The SPB request.
//
//  RequestType - The request type
//
// Return Value:
//
//  NTSTATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPIpSpbPrepareRequest (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr,
    SPBREQUEST SpbRequest,
    ECSPI_REQUEST_TYPE RequestType
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;
    ECSPI_SPB_REQUEST* requestPtr = &devExtPtr->CurrentRequest;
    NTSTATUS status;

    //
    // Initialize request parameters
    //
    {
        KLOCK_QUEUE_HANDLE lockHandle;
        KeAcquireInStackQueuedSpinLock(&devExtPtr->DeviceLock, &lockHandle);

        RtlZeroMemory(requestPtr, sizeof(ECSPI_SPB_REQUEST));
        requestPtr->SpbRequest = SpbRequest;
        requestPtr->SpbTargetPtr = TrgCtxPtr;
        requestPtr->Type = RequestType;
        requestPtr->State = ECSPI_REQUEST_STATE::INACTIVE;

        SPB_REQUEST_PARAMETERS spbRequestParameters;
        SPB_REQUEST_PARAMETERS_INIT(&spbRequestParameters);
        SpbRequestGetParameters(SpbRequest, &spbRequestParameters);

        requestPtr->TransferCount = spbRequestParameters.SequenceTransferCount;
        requestPtr->TransfersLeft = requestPtr->TransferCount;
        if (requestPtr->TransferCount == 0) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "Invalid transfer count: 0"
                );
            KeReleaseInStackQueuedSpinLock(&lockHandle);
            return STATUS_INVALID_PARAMETER;
        }

        switch (RequestType) {
        case ECSPI_REQUEST_TYPE::READ:
        case ECSPI_REQUEST_TYPE::WRITE:
        case ECSPI_REQUEST_TYPE::SEQUENCE:
            break;

        case ECSPI_REQUEST_TYPE::FULL_DUPLEX:
            if (requestPtr->TransferCount != 2) {

                ECSPI_LOG_ERROR(
                    devExtPtr->IfrLogHandle,
                    "Invalid transfer count (%lu) for a FULL_DUPLEX request!"
                    " 2 transfers are required.",
                    requestPtr->TransferCount
                    );
                KeReleaseInStackQueuedSpinLock(&lockHandle);
                return STATUS_INVALID_PARAMETER;
            }
            break;

        default:
            ECSPI_ASSERT(devExtPtr->IfrLogHandle, FALSE);
            KeReleaseInStackQueuedSpinLock(&lockHandle);
            return STATUS_INVALID_PARAMETER;
        } // switch (...)

        KeReleaseInStackQueuedSpinLock(&lockHandle);

    } // Get request parameters

    status = ECSPISpbPrepareNextTransfer(requestPtr);
    if (!NT_SUCCESS(status)) {

        return status;
    }

    return STATUS_SUCCESS;
}

#undef _ECSPI_SPB_CPP_
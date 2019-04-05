// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPIdevice.cpp
//
// Abstract:
//
//    This module contains the IMX ECSPI controller's PnP device functions.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//
#include "precomp.h"
#pragma hdrstop

#define _ECSPI_DEVICE_CPP_

// Logging header files
#include "ECSPItrace.h"
#include "ECSPIdevice.tmh"

// Common driver header files
#include "ECSPIcommon.h"

// Module specific header files
#include "ECSPIhw.h"
#include "ECSPIspb.h"
#include "ECSPIdriver.h"
#include "ECSPIdevice.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, ECSPIEvtDevicePrepareHardware)
    #pragma alloc_text(PAGE, ECSPIEvtDeviceReleaseHardware)
#endif

//
// Routine Description:
//
//  ECSPIEvtDevicePrepareHardware is called by the framework when a ECSPI
//  device is coming online, after being it's resources have been negotiated and
//  translated.
//  The routine reads and map the device resources and initializes the device.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  ResourcesRaw - Hardware resource list.
//
//  ResourcesTranslated - Hardware translated resource list.
//
// Return Value:
//
//  Resources mapping status and controller initialization completion code.
//
_Use_decl_annotations_
NTSTATUS
ECSPIEvtDevicePrepareHardware (
    WDFDEVICE WdfDevice,
    WDFCMRESLIST /*ResourcesRaw*/,
    WDFCMRESLIST ResourcesTranslated
    )
{
    PAGED_CODE();

    ECSPI_DEVICE_EXTENSION* devExtPtr = ECSPIDeviceGetExtension(WdfDevice);
    ULONG numResourses = WdfCmResourceListGetCount(ResourcesTranslated);
    ULONG numIntResourcesFound = 0;
    ULONG numMemResourcesFound = 0;
    ULONG numConnectionResourcesFound = 0;
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* memResourceDescPtr = nullptr;
    ULONG traceLogId = 0;

    for (ULONG resInx = 0; resInx < numResourses; ++resInx) {

        const CM_PARTIAL_RESOURCE_DESCRIPTOR* resDescPtr =
            WdfCmResourceListGetDescriptor(ResourcesTranslated, resInx);

        switch (resDescPtr->Type) {
        case CmResourceTypeMemory:
            ++numMemResourcesFound;
            if (numMemResourcesFound > 1) {

                ECSPI_LOG_ERROR(
                    DRIVER_LOG_HANDLE,
                    "Unexpected additional memory resource!"
                    );
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            if (resDescPtr->u.Memory.Length ==
                ECSPI_REGISTERS_ADDRESS_SPACE_SIZE) {

                memResourceDescPtr = resDescPtr;
                //
                // Use the lower part of the register base address
                // as trace log ID.
                //
                traceLogId = resDescPtr->u.Memory.Start.LowPart;

            } else {

                ECSPI_LOG_ERROR(
                    DRIVER_LOG_HANDLE,
                    "Invalid ECSPI register file span (%lu), expected %lu!",
                    resDescPtr->u.Memory.Length,
                    ECSPI_REGISTERS_ADDRESS_SPACE_SIZE
                    );
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            break;

        case CmResourceTypeInterrupt:
            ++numIntResourcesFound;
            if (numIntResourcesFound > 1) {

                ECSPI_LOG_ERROR(
                    DRIVER_LOG_HANDLE,
                    "Unexpected additional interrupt resource %lu, expected 1!",
                    numIntResourcesFound
                    );
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            break;

        case CmResourceTypeConnection:
            if ((resDescPtr->u.Connection.Class ==
                 CM_RESOURCE_CONNECTION_CLASS_FUNCTION_CONFIG) &&
                (resDescPtr->u.Connection.Type ==
                 CM_RESOURCE_CONNECTION_TYPE_FUNCTION_CONFIG)) {
                //
                // ignore FunctionConfig resources
                //
                continue;
            }

            if (numConnectionResourcesFound >= ECSPI_CHANNEL::COUNT) {

                ECSPI_LOG_ERROR(
                    DRIVER_LOG_HANDLE,
                    "Unexpected additional connection resource %lu, "
                    "expected %lu!",
                    numConnectionResourcesFound,
                    ECSPI_CHANNEL::COUNT
                    );
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            if ((resDescPtr->u.Connection.Class !=
                 CM_RESOURCE_CONNECTION_CLASS_GPIO) ||
                (resDescPtr->u.Connection.Type !=
                 CM_RESOURCE_CONNECTION_TYPE_GPIO_IO)) {

                ECSPI_LOG_ERROR(
                    DRIVER_LOG_HANDLE,
                    "Unexpected connection class/type (%lu/%lu), "
                    "only GPIO class/type (%lu/%lu) is expected!",
                    resDescPtr->u.Connection.Class,
                    resDescPtr->u.Connection.Type,
                    CM_RESOURCE_CONNECTION_CLASS_GPIO,
                    CM_RESOURCE_CONNECTION_TYPE_GPIO_IO
                    );
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            //
            // New CS GPIO pin entry
            //
            {
                ECSPI_CS_GPIO_PIN* csGpioPinPtr = 
                    &devExtPtr->CsGpioPins[numConnectionResourcesFound];

                csGpioPinPtr->GpioConnectionId.LowPart =
                    resDescPtr->u.Connection.IdLowPart;
                csGpioPinPtr->GpioConnectionId.HighPart =
                    resDescPtr->u.Connection.IdHighPart;

                KeInitializeEvent(
                    &csGpioPinPtr->SyncCallEvent,
                    NotificationEvent,
                    FALSE
                    );

                ++numConnectionResourcesFound;

            } // New CS GPIO pin entry
            break;

        default:
            ECSPI_ASSERT(DRIVER_LOG_HANDLE, FALSE);
            break;

        } // switch

    } // for (resource list)

    //
    // Make sure we got everything we need...
    //
    if (numMemResourcesFound != 1) {

        ECSPI_LOG_ERROR(DRIVER_LOG_HANDLE, "Invalid or no memory resource!");
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (numIntResourcesFound != 1) {

        ECSPI_LOG_ERROR(DRIVER_LOG_HANDLE, "Invalid or not interrupt resource!");
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // Map device registers into virtual memory...
    // The registers address space is 4 PAGES, but registers
    // span just 0x44 bytes, so we map the minimum number of pages.
    //
    {
        PHYSICAL_ADDRESS ecspiRegistersPhysAddress = memResourceDescPtr->u.Memory.Start;
        SIZE_T registersMappedSize = ROUND_TO_PAGES(sizeof(ECSPI_REGISTERS));

        devExtPtr->ECSPIRegsPtr = static_cast<ECSPI_REGISTERS*>(MmMapIoSpaceEx(
            ecspiRegistersPhysAddress,
            registersMappedSize,
            PAGE_READWRITE | PAGE_NOCACHE
            ));
        if (devExtPtr->ECSPIRegsPtr == nullptr) {

            ECSPI_LOG_ERROR(
                DRIVER_LOG_HANDLE,
                "Failed to map ECSPI regs, span %Iu bytes!",
                registersMappedSize
                );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    NTSTATUS status = ECSPIHwInitController(devExtPtr);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            DRIVER_LOG_HANDLE,
            "ECSPIHwInitController failed. "
            "wdfDevice = %p, status = %!STATUS!",
            WdfDevice,
            status
            );
        return status;
    }

    ECSPIpDeviceLogInit(devExtPtr, traceLogId);

    ECSPI_LOG_INFORMATION(
        devExtPtr->IfrLogHandle,
        "ECSPI regs mapped at %p",
        PVOID(devExtPtr->ECSPIRegsPtr)
        );

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPIEvtDeviceReleaseHardware is called by the framework when a ECSPI
//  device is offline and not accessible anymore.
//  The routine just releases device resources.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
//  ResourcesTranslated - Hardware translated resource list.
//
// Return Value:
//
//  STATUS_SUCCESS
//
_Use_decl_annotations_
NTSTATUS
ECSPIEvtDeviceReleaseHardware (
    WDFDEVICE WdfDevice,
    WDFCMRESLIST ResourcesTranslated
    )
{
    PAGED_CODE();

    ECSPI_DEVICE_EXTENSION* devExtPtr = ECSPIDeviceGetExtension(WdfDevice);

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    if (devExtPtr->ECSPIRegsPtr != nullptr) {

        MmUnmapIoSpace(
            PVOID(devExtPtr->ECSPIRegsPtr),
            ROUND_TO_PAGES(sizeof(ECSPI_REGISTERS))
            );
    }
    devExtPtr->ECSPIRegsPtr = nullptr;

    for (ULONG spiCh = 0; spiCh < ECSPI_CHANNEL::COUNT; ++spiCh) {

        ECSPI_CS_GPIO_PIN* csGpioPinPtr = &devExtPtr->CsGpioPins[spiCh];

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            csGpioPinPtr->WdfIoTargetGpio == NULL
            );
        csGpioPinPtr->GpioConnectionId.QuadPart = 0;
    }

    //
    // Disconnecting from interrupt will automatically be done
    // by the framework....
    //

    ECSPIpDeviceLogDeinit(devExtPtr);

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPIEvtIoInCallerContext is called by the framework to pre-process
//  requests before they are put in a WDF IO queue.
//  It is used for custom IO control requests and specifically for FULL_DUPLEX
//  transfers.
//
// Arguments:
//
//  WdfDevice - The SPI device object
//
//  WdfRequest - The request.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIEvtIoInCallerContext (
    WDFDEVICE WdfDevice,
    WDFREQUEST WdfRequest
    )
{
    const ECSPI_DEVICE_EXTENSION* devExtPtr = ECSPIDeviceGetExtension(WdfDevice);

    WDF_REQUEST_PARAMETERS wdfRequestParams;
    WDF_REQUEST_PARAMETERS_INIT(&wdfRequestParams);
    WdfRequestGetParameters(WdfRequest, &wdfRequestParams);

    //
    // Filter out unrecognized IO control requests.
    //
    switch (wdfRequestParams.Type) {
    case WdfRequestTypeDeviceControl:
    case WdfRequestTypeDeviceControlInternal:
        break;

    default:
        WdfRequestComplete(WdfRequest, STATUS_NOT_SUPPORTED);
        return;

    } // switch

    switch (wdfRequestParams.Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SPB_FULL_DUPLEX:
        break;

    default:
        WdfRequestComplete(WdfRequest, STATUS_NOT_SUPPORTED);
        return;
    }

    NTSTATUS status = SpbRequestCaptureIoOtherTransferList(
        static_cast<SPBREQUEST>(WdfRequest)
        );
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "SpbRequestCaptureIoOtherTransferList failed. "
            "wdfDevice = %p, status = %!STATUS!",
            WdfDevice,
            status
            );
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    status = WdfDeviceEnqueueRequest(WdfDevice, WdfRequest);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "WdfDeviceEnqueueRequest failed. "
            "wdfDevice = %p, status = %!STATUS!",
            WdfDevice,
            status
            );
        WdfRequestComplete(WdfRequest, status);
        return;
    }
}


//
// Routine Description:
//
//  ECSPIEvtRequestCancel is called by the framework to cancel
//  A submitted request.
//  The routine aborts all associated transfers and complete the request.
//
// Arguments:
//
//  WdfRequest - The canceled request.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIEvtRequestCancel (
    WDFREQUEST WdfRequest
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = ECSPIDeviceGetExtension(
        WdfFileObjectGetDevice(WdfRequestGetFileObject(WdfRequest))
        );

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->DeviceLock, &lockHandle);

    ECSPI_SPB_REQUEST* requestPtr = &devExtPtr->CurrentRequest;
    SPBREQUEST spbRequest = requestPtr->SpbRequest;
    BOOLEAN isCompleteRequest = FALSE;

    ECSPI_LOG_WARNING(
        devExtPtr->IfrLogHandle,
        "ECSPIEvtRequestCancel called for request = 0x%p",
        spbRequest
        );

    ECSPISpbAbortAllTransfers(requestPtr);

    if (spbRequest != NULL) {

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            spbRequest == static_cast<SPBREQUEST>(WdfRequest)
            );

        requestPtr->State = ECSPI_REQUEST_STATE::CANCELED;

        if (devExtPtr->IsControllerLocked) {
            //
            // Device is locked, no need to negate CS, 
            // complete the request locally.
            //
            requestPtr->SpbRequest = NULL;
            isCompleteRequest = TRUE;

        } else {

            NTSTATUS status = ECSPIDeviceNegateCS(requestPtr->SpbTargetPtr, FALSE);
            if (status != STATUS_PENDING) {
                //
                // Negate CS failed, or GPIO for CS is not used,
                // complete the request locally.
                //
                requestPtr->SpbRequest = NULL;
                isCompleteRequest = TRUE;
            }
        }
    }

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    //
    // We complete the request outside the device lock protected 
    // zone to avoid getting a new request while the device lock
    // is held.
    //
    if (isCompleteRequest) {

        requestPtr->Type = ECSPI_REQUEST_TYPE::INVALID;
        requestPtr->State = ECSPI_REQUEST_STATE::INACTIVE;

        WdfRequestSetInformation(spbRequest, 0);
        SpbRequestComplete(spbRequest, STATUS_CANCELLED);
    }
}


//
// Routine Description:
//
//  ECSPIEvtInterruptIsr is the ECSPI ISR.
//  It continues driving the active transfer(s), and schedules DPC
//  when transfer is done, to complete the SPB request, or to prepare more
//  transfers during a SEQUENCE request processing.
//
// Arguments:
//
//  WdfInterrupt - The SPI interrupt object
//
//  MessageID - Message ID (not used)
//
// Return Value:
//
//  TRUE: interrupt originated from SPI controller, otherwise FALSE.
//
_Use_decl_annotations_
BOOLEAN
ECSPIEvtInterruptIsr (
    WDFINTERRUPT WdfInterrupt,
    ULONG /*MessageID*/
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = 
        ECSPIDeviceGetExtension(WdfInterruptGetDevice(WdfInterrupt));
    ECSPI_SPB_REQUEST* requestPtr = &devExtPtr->CurrentRequest;

    //
    // Read and ACK current interrupts
    //
    ECSPI_STATREG statReg;
    {
        volatile ECSPI_REGISTERS* ecspiRegsPtr = devExtPtr->ECSPIRegsPtr;

        statReg.AsUlong = READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->STATREG);
        if ((statReg.AsUlong & 
             ECSPI_INTERRUPT_GROUP::ECSPI_ALL_INTERRUPTS) == 0) {
            //
            // Not our interrupt
            //
            return FALSE;
        }
        WRITE_REGISTER_NOFENCE_ULONG(
            &ecspiRegsPtr->STATREG,
            (statReg.AsUlong & ECSPI_INTERRUPT_GROUP::ECSPI_ACKABLE_INTERRUPTS)
            );

    } // Read and ACK current interrupts

    ECSPI_SPB_TRANSFER* transfer1Ptr;
    ECSPI_SPB_TRANSFER* transfer2Ptr;
    ECSPISpbGetActiveTransfers(requestPtr, &transfer1Ptr, &transfer2Ptr);

    ECSPI_ASSERT(devExtPtr->IfrLogHandle, transfer1Ptr != nullptr);

    switch (requestPtr->Type) {
    case ECSPI_REQUEST_TYPE::READ:
    { // READ

        if (ECSPIHwReadRxFIFO(devExtPtr, transfer1Ptr)) {

            if (transfer1Ptr->IsStartBurst && !ECSPISpbIsAllDataTransferred(transfer1Ptr)) {
                // Schedule DPC to continue current transfer
                break;
            }

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                ECSPISpbIsAllDataTransferred(transfer1Ptr)
                );
            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                !transfer1Ptr->IsStartBurst
                );

            ECSPIHwDisableTransferInterrupts(devExtPtr, transfer1Ptr);

            //
            // READ transfer is done.
            // Schedule a DPC to complete the request
            //
            break;
        }
        return TRUE;

    } // READ

    case ECSPI_REQUEST_TYPE::WRITE:
    { // WRITE

        if (ECSPISpbIsTransferDone(transfer1Ptr)) {
            //
            // Make sure all TX data was sent
            //
            if (statReg.TC == 0) {

                return TRUE;
            }
            ECSPIHwDisableTransferInterrupts(devExtPtr, transfer1Ptr);

            //
            // WRITE transfer is done.
            // Schedule a DPC to complete the request
            //
            break;
        }

        (void)ECSPIHwWriteTxFIFO(devExtPtr, transfer1Ptr);
        return TRUE;

    } // WRITE

    case ECSPI_REQUEST_TYPE::SEQUENCE:
    { // SEQUENCE

        if (ECSPISpbIsWriteTransfer(transfer1Ptr)) {
            //
            // A WRITE transfer
            // 
            if (ECSPISpbIsTransferDone(transfer1Ptr)) {
                //
                // Make sure all TX data was sent
                //
                if (statReg.TC == 0) {

                    return TRUE;
                }

                //
                // Request is done or more transfers need to be prepared.
                // Schedule a DPC to complete the request, or prepare more
                // transfers.
                //
                ECSPISpbCompleteSequenceTransfer(transfer1Ptr);
                break;
            }

            //
            // Write transfer not done, keep sending data...
            //
            (void)ECSPIHwWriteTxFIFO(devExtPtr, transfer1Ptr);
            return TRUE;

        } else {
            //
            // A READ transfer
            // 
            if (ECSPIHwReadRxFIFO(devExtPtr, transfer1Ptr)) {

                //
                // Request is done or more transfers need to be prepared.
                // Schedule a DPC to complete the request, or prepare more
                // transfers.
                //
                ECSPISpbCompleteSequenceTransfer(transfer1Ptr);
                break;
            }

            return TRUE;
        }

    } // SEQUENCE

    case ECSPI_REQUEST_TYPE::FULL_DUPLEX:
    { // FULL_DUPLEX

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle, 
            transfer2Ptr != nullptr
            );
        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle, 
            ECSPISpbIsWriteTransfer(transfer1Ptr)
            );
        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle, 
            !ECSPISpbIsWriteTransfer(transfer2Ptr)
            );

        if (ECSPISpbIsTransferDone(transfer1Ptr)) {

            ECSPIHwDisableTransferInterrupts(devExtPtr, transfer1Ptr);

        } else {

            (void)ECSPIHwWriteTxFIFO(devExtPtr, transfer1Ptr);
        }

        if (ECSPIHwReadRxFIFO(devExtPtr, transfer2Ptr)) {

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                ECSPISpbIsTransferDone(transfer1Ptr)
                );

            ECSPIHwDisableTransferInterrupts(devExtPtr, transfer2Ptr);
            //
            // Both transfers are done.
            // Schedule a DPC to complete the request.
            //
            break;
        }
        return TRUE;

    } // FULL_DUPLEX

    default:
        ECSPI_ASSERT(devExtPtr->IfrLogHandle, FALSE);
        return TRUE;

    } // switch

    //
    // Continue in DPC...
    //
    WdfInterruptQueueDpcForIsr(WdfInterrupt);
    return TRUE;
}


//
// Routine Description:
//
//  ECSPIEvtInterruptDpc is the ECSPI DPC.
//  It is scheduled by ECSPIEvtInterruptDpc to complete requests or to
//  prepare and possible start more transfers during a SEQUENCE request
//  processing.
//
// Arguments:
//
//  WdfInterrupt - The SPI interrupt object
//
//  AssociatedWdfObject - 
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIEvtInterruptDpc (
    WDFINTERRUPT WdfInterrupt,
    WDFOBJECT /*AssociatedWdfObject*/
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr =
        ECSPIDeviceGetExtension(WdfInterruptGetDevice(WdfInterrupt));
    ECSPI_SPB_REQUEST* requestPtr = &devExtPtr->CurrentRequest;
    BOOLEAN hwTimeOut = FALSE;

    // See if transfer request isn't finished and needs continuing
    // before marking request as uncancelable
    if (requestPtr->Type == ECSPI_REQUEST_TYPE::READ) {
        ECSPI_SPB_TRANSFER* transfer1Ptr;
        ECSPI_SPB_TRANSFER* transfer2Ptr;

        ECSPISpbGetActiveTransfers(requestPtr, &transfer1Ptr, &transfer2Ptr);

        // StartBurst delayed, poll XCH and start next burst
        if (!ECSPISpbIsAllDataTransferred(transfer1Ptr) && transfer1Ptr->IsStartBurst) {
            KLOCK_QUEUE_HANDLE lockHandle;

            LARGE_INTEGER freq;
            LARGE_INTEGER start;
            LARGE_INTEGER finish;
            LONGLONG timeout;

            KeAcquireInStackQueuedSpinLock(&devExtPtr->DeviceLock, &lockHandle);

            // wait up to 1 SPI clock cycle for XCH status change
            start = KeQueryPerformanceCounter(&freq);
            timeout = freq.QuadPart / requestPtr->SpbTargetPtr->Settings.ConnectionSpeed;

            do {
                if (ECSPIHwQueryXCH(devExtPtr->ECSPIRegsPtr) == 0) {

                    ECSPIpHwStartBurstIf(devExtPtr, transfer1Ptr);
                    KeReleaseInStackQueuedSpinLock(&lockHandle);
                    return;
                }

                finish = KeQueryPerformanceCounter(NULL);
            }
            while ((finish.QuadPart - start.QuadPart) < timeout);

            KeReleaseInStackQueuedSpinLock(&lockHandle);

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "Timeout error waiting for XCH state change.");
            hwTimeOut = TRUE;
        }
    }

    //
    // Make sure request has not been already canceled, and 
    // prevent request from going away while request processing continues.
    //
    NTSTATUS status = ECSPIDeviceDisableRequestCancellation(requestPtr);
    if (!NT_SUCCESS(status)) {
        //
        // More than one instance of the DPC may have been scheduled 
        // during a multi-transfer sequence request, so the request 
        // cancellation may have already been disabled. 
        // This is a benign case, in which we can just leave...
        //
        if (status != STATUS_NO_WORK_DONE) {

            ECSPI_LOG_WARNING(
                devExtPtr->IfrLogHandle,
                "SPB request already canceled or completed!"
                );
        }
        return;
    }

    if (hwTimeOut) {
        ECSPISpbCompleteTransferRequest(
            requestPtr,
            STATUS_IO_TIMEOUT,
            requestPtr->TotalBytesTransferred
            );
        return;
    }

    switch (requestPtr->Type) {
    case ECSPI_REQUEST_TYPE::READ:
    case ECSPI_REQUEST_TYPE::WRITE:
    case ECSPI_REQUEST_TYPE::FULL_DUPLEX:
    {
        ECSPI_SPB_TRANSFER* transfer1Ptr;
        ECSPI_SPB_TRANSFER* transfer2Ptr;
        ECSPISpbGetActiveTransfers(requestPtr, &transfer1Ptr, &transfer2Ptr);

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            ECSPISpbIsAllDataTransferred(transfer1Ptr)
            );
        if (transfer2Ptr != nullptr) {

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                ECSPISpbIsAllDataTransferred(transfer2Ptr)
                );
        }

        break;

    } // READ/WRITE/FULL_DUPLEX

    case ECSPI_REQUEST_TYPE::SEQUENCE:
    { // SEQUENCE
        
        if(ECSPISpbIsRequestDone(requestPtr)) {
            //
            // Continue to request completion
            //
            break;
        }

        status = ECSPISpbPrepareNextTransfer(requestPtr);
        if (!NT_SUCCESS(status) && (status != STATUS_NO_MORE_FILES)) {
            
            ECSPI_ASSERT(devExtPtr->IfrLogHandle, NT_SUCCESS(status));

            //
            // Continue to request completion
            //
            break;
        }

        //
        // Before we continue handling remaining transfers, make sure
        // request has not been canceled, and setup the cancellation routine.
        //
        status = ECSPIDeviceEnableRequestCancellation(requestPtr);
        if (!NT_SUCCESS(status)) {

            return;
        }

        //
        // If request is idle, start the next transfer...
        //
        if (InterlockedExchange(&requestPtr->IsIdle, 0) != 0) {

            status = ECSPISpbStartTransferSafe(requestPtr);
            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                status == STATUS_SUCCESS
                );
        }

        //
        // Continue handling remaining transfers
        //
        return;

    } // SEQUENCE

    default:
        ECSPI_ASSERT(devExtPtr->IfrLogHandle, FALSE);
        return;

    } // switch

    //
    // Request completion
    //

    if (!devExtPtr->IsControllerLocked) {

        status = ECSPIDeviceNegateCS(requestPtr->SpbTargetPtr, FALSE);
        if (status == STATUS_PENDING) {

            return;
        }
    }

    //
    // Complete the request, we get here when one of the following is TRUE:
    // - Not using GPIO for CS.
    // - CS negation failed.
    // - Controller is locked so we do not negate CS.
    //
    ECSPISpbCompleteTransferRequest(
        requestPtr,
        status,
        requestPtr->TotalBytesTransferred
        );
}


//
// Routine Description:
//
//  ECSPIDeviceEnableRequestCancellation is called to set the current request 
//  cancellation routine. 
//  If the request is already canceled, the routine completes the 
//  request if needed.
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
ECSPIDeviceEnableRequestCancellation (
    ECSPI_SPB_REQUEST* RequestPtr
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = RequestPtr->SpbTargetPtr->DevExtPtr;

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->DeviceLock, &lockHandle);

    SPBREQUEST spbRequest = RequestPtr->SpbRequest;
    ECSPI_ASSERT(devExtPtr->IfrLogHandle, spbRequest != NULL);

    NTSTATUS status = WdfRequestMarkCancelableEx(spbRequest, ECSPIEvtRequestCancel);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "WdfRequestMarkCancelableEx failed. "
            "request %p, status = %!STATUS!",
            spbRequest,
            status
            );

        ECSPISpbAbortAllTransfers(RequestPtr);
        ECSPISpbCompleteTransferRequest(RequestPtr, status, 0);

    } else {

        RequestPtr->State = ECSPI_REQUEST_STATE::CANCELABLE;
    }

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    return status;
}


//
// Routine Description:
//
//  ECSPIDeviceDisableRequestCancellation is called to remove the current 
//  request cancellation routine, so it cannot be canceled.
//  If the request is already canceled, the routine completes the 
//  request if needed.
//
// Arguments:
//
//  RequestPtr - The request object.
//
// Return Value:
//
//  STATUS_NO_WORK_DONE is the request cancellation has 
//  already been disabled, which is benign or NTSTATUS.
//
_Use_decl_annotations_
NTSTATUS
ECSPIDeviceDisableRequestCancellation (
    ECSPI_SPB_REQUEST* RequestPtr
    )
{
    NTSTATUS status;
    ECSPI_DEVICE_EXTENSION* devExtPtr = RequestPtr->SpbTargetPtr->DevExtPtr;

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->DeviceLock, &lockHandle);

    SPBREQUEST spbRequest = RequestPtr->SpbRequest;
    if (spbRequest == NULL) {

        ECSPI_LOG_INFORMATION(
            devExtPtr->IfrLogHandle,
            "SPB request already canceled!"
            );
        status = STATUS_NO_WORK_DONE;
        goto done;
    }

    if (RequestPtr->State != ECSPI_REQUEST_STATE::CANCELABLE) {

        status = STATUS_NO_WORK_DONE;
        goto done;
    }

    status = WdfRequestUnmarkCancelable(spbRequest);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "WdfRequestUnmarkCancelable failed. "
            "Request = 0x%p, status = %!STATUS!",
            spbRequest,
            status
            );

        ECSPISpbAbortAllTransfers(RequestPtr);

        if (status != STATUS_CANCELLED) {

            ECSPISpbCompleteTransferRequest(RequestPtr, status, 0);
        }
        goto done;
    }

    RequestPtr->State = ECSPI_REQUEST_STATE::NOT_CANCELABLE;
    status = STATUS_SUCCESS;

done:

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    return status;
}


//
// Routine Description:
//
//  ECSPIDeviceOpenGpioTarget is called to create and open the
//  GPIO target device.
//  Since ECSPI burst length is limited to 512 bytes, we need to extend it
//  on the fly for long transfers.
//  To avoid CS negation between bursts, we control the CS with a GPIO pin.
//
// Arguments:
//
//  TrgCtxPtr - The target context
//
// Return Value:
//
//  NTSTATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPIDeviceOpenGpioTarget (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr
    )
{
    NTSTATUS status;
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;
    ECSPI_CS_GPIO_PIN* csGpioPinPtr = ECSPIDeviceGetCsGpio(TrgCtxPtr);

    WDFIOTARGET wdfIoTargetGpio = NULL;
    WDFREQUEST wdfRequestGpioAssert = NULL;
    WDFREQUEST wdfRequestGpioNegate = NULL;
    WDFMEMORY wdfMemoryGpio = NULL;

    if (csGpioPinPtr->GpioConnectionId.QuadPart == 0) {
        //
        // We do not mandate using a GPIO pin for CS.
        //
        status = STATUS_SUCCESS;
        goto done;
    }

    if (csGpioPinPtr->WdfIoTargetGpio != NULL) {

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            csGpioPinPtr->OpenCount != 0
            );
        status = STATUS_SUCCESS;
        goto done;
    }

    //
    // Create and open the IO target
    //
    {
        status = WdfIoTargetCreate(
            devExtPtr->WdfDevice,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfIoTargetGpio
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "WdfIoTargetCreate failed, "
                "(status = %!STATUS!)",
                status
                );
            goto done;
        }

        DECLARE_UNICODE_STRING_SIZE(gpioDeviceName, RESOURCE_HUB_PATH_CHARS);
        status = RESOURCE_HUB_CREATE_PATH_FROM_ID(
            &gpioDeviceName,
            csGpioPinPtr->GpioConnectionId.LowPart,
            csGpioPinPtr->GpioConnectionId.HighPart
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "RESOURCE_HUB_CREATE_PATH_FROM_ID failed for GPIO connection, "
                "(status = %!STATUS!)",
                status
                );
            goto done;
        }

        WDF_IO_TARGET_OPEN_PARAMS wdfIoTargetOpenParams;
        WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
            &wdfIoTargetOpenParams,
            &gpioDeviceName,
            FILE_GENERIC_WRITE
            );
        wdfIoTargetOpenParams.ShareAccess = FILE_SHARE_WRITE;

        status = WdfIoTargetOpen(
            wdfIoTargetGpio,
            &wdfIoTargetOpenParams
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "WdfIoTargetOpen failed, "
                "(status = %!STATUS!)",
                status
                );
            goto done;
        }

    } // Create and open the IO target

    //
    // Create the GPIO requests and associated resources
    //
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = wdfIoTargetGpio;

        status = WdfRequestCreate(
            &attributes,
            wdfIoTargetGpio,
            &wdfRequestGpioAssert
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "WdfRequestCreate failed, for GPIO assert request"
                "(status = %!STATUS!)",
                status
                );
            goto done;
        }

        status = WdfRequestCreate(
            &attributes,
            wdfIoTargetGpio,
            &wdfRequestGpioNegate
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "WdfRequestCreate failed, for GPIO negate request"
                "(status = %!STATUS!)",
                status
            );
            goto done;
        }

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = wdfIoTargetGpio;

        status = WdfMemoryCreatePreallocated(
            &attributes,
            &csGpioPinPtr->GpioData,
            sizeof(UCHAR),
            &wdfMemoryGpio
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "WdfMemoryCreatePreallocated failed, for GPIO request data"
                "(status = %!STATUS!)",
                status
                );
            goto done;
        }

    } // Create the GPIO request and associated resources

    csGpioPinPtr->WdfIoTargetGpio = wdfIoTargetGpio;
    csGpioPinPtr->WdfRequestGpioAssert = wdfRequestGpioAssert;
    csGpioPinPtr->WdfRequestGpioNegate = wdfRequestGpioNegate;
    csGpioPinPtr->WdfMemoryGpio = wdfMemoryGpio;
    status = STATUS_SUCCESS;

done:

    if (NT_SUCCESS(status)) {

        csGpioPinPtr->OpenCount += 1;

    } else {

        if (wdfIoTargetGpio != NULL) {

            WdfObjectDelete(wdfIoTargetGpio);
        }
    }

    return status;
}


//
// Routine Description:
//
//  ECSPIDeviceCloseGpioTarget closes the GPIO target device, 
//  and releases associated resources.
//  Since ECSPI burst length is limited to 512 bytes, we need to extend it
//  on the fly for long transfers.
//  To avoid CS negation between bursts, we control the CS with a GPIO pin.
//
// Arguments:
//
//  TrgCtxPtr - The target context
//
// Return Value:
//
//  NTSTATUS
//
_Use_decl_annotations_
VOID
ECSPIDeviceCloseGpioTarget (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;
    ECSPI_CS_GPIO_PIN* csGpioPinPtr = ECSPIDeviceGetCsGpio(TrgCtxPtr);

    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        csGpioPinPtr->OpenCount != 0
        );

    csGpioPinPtr->OpenCount -= 1;

    if (csGpioPinPtr->OpenCount == 0) {

        if (csGpioPinPtr->WdfIoTargetGpio != NULL) {

            WdfObjectDelete(csGpioPinPtr->WdfIoTargetGpio);
        }
        csGpioPinPtr->WdfIoTargetGpio = NULL;
        csGpioPinPtr->WdfRequestGpioAssert = NULL;
        csGpioPinPtr->WdfRequestGpioNegate = NULL;
        csGpioPinPtr->WdfMemoryGpio = NULL;
    }
}


//
// Routine Description:
//
//  ECSPIDeviceAssertCS asserts the CS GPIO pin.
//  If no GPIO pin resource is available, it starts the transfer directly.
//
// Arguments:
//
//  TrgCtxPtr - The target context
//
//  IsWait - If to wait for CS negation completion
//
// Return Value:
//
//  NTSTATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPIDeviceAssertCS (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr,
    BOOLEAN IsWait
    )
{
    ECSPI_CS_GPIO_PIN* csGpioPinPtr = ECSPIDeviceGetCsGpio(TrgCtxPtr);
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;
    ECSPI_SPB_REQUEST* requestPtr = &devExtPtr->CurrentRequest;

    //
    // Select (reset) the target (there is no other way to reset RX/TX FIFO).
    // All interrupts are disabled.
    //
    ECSPIHwSelectTarget(TrgCtxPtr);

    if (csGpioPinPtr->GpioConnectionId.QuadPart == 0) {
        //
        // Not using GPIO for CS
        //
        SPBREQUEST lockRequest = devExtPtr->LockUnlockRequest;
        if (lockRequest != NULL) {

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                requestPtr->SpbRequest == NULL
                );
            devExtPtr->LockUnlockRequest = NULL;
            SpbRequestComplete(lockRequest, STATUS_SUCCESS);
            return STATUS_SUCCESS;
        }

        NTSTATUS status = ECSPIDeviceEnableRequestCancellation(requestPtr);
        if (!NT_SUCCESS(status)) {

            return status;
        }

        return ECSPISpbStartTransferSafe(requestPtr);
    }

    KeResetEvent(&csGpioPinPtr->SyncCallEvent);

    NTSTATUS status = ECSPIpDeviceSetGpioPin(
        TrgCtxPtr,
        ECSPISpbGetCsActiveValue(TrgCtxPtr)
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }

    if (!IsWait) {

        return STATUS_PENDING;
    }

    status = KeWaitForSingleObject(
        &csGpioPinPtr->SyncCallEvent,
        Executive,
        KernelMode,
        FALSE,
        nullptr
        );
    ECSPI_ASSERT(
        TrgCtxPtr->DevExtPtr->IfrLogHandle,
        status == STATUS_WAIT_0
        );

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPIDeviceNegateCS negates the CS GPIO pin.
//  If no GPIO pin resource is available, it starts the transfer directly.
//
// Arguments:
//
//  TrgCtxPtr - The target context
//
//  IsWait - If to wait for CS negation completion
//
// Return Value:
//
//  NTSTATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPIDeviceNegateCS (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr,
    BOOLEAN IsWait
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;
    ECSPI_CS_GPIO_PIN* csGpioPinPtr = ECSPIDeviceGetCsGpio(TrgCtxPtr);

    if (csGpioPinPtr->GpioConnectionId.QuadPart == 0) {
        //
        // Not using GPIO for CS
        //
        SPBREQUEST lockRequest = devExtPtr->LockUnlockRequest;
        if (lockRequest != NULL) {

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                devExtPtr->CurrentRequest.SpbRequest == NULL
                );
            devExtPtr->LockUnlockRequest = NULL;
            SpbRequestComplete(lockRequest, STATUS_SUCCESS);
            return STATUS_SUCCESS;
        }

        return STATUS_SUCCESS;
    }

    KeResetEvent(&csGpioPinPtr->SyncCallEvent);

    NTSTATUS status = ECSPIpDeviceSetGpioPin(
        TrgCtxPtr,
        ECSPISpbGetCsNonActiveValue(TrgCtxPtr)
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }

    if (!IsWait) {

        return STATUS_PENDING;
    }

    status = KeWaitForSingleObject(
        &csGpioPinPtr->SyncCallEvent,
        Executive,
        KernelMode,
        FALSE,
        nullptr
        );
    ECSPI_ASSERT(
        TrgCtxPtr->DevExtPtr->IfrLogHandle,
        status == STATUS_WAIT_0
        );

    return STATUS_SUCCESS;
}


//
// ECSPIdevice private methods
//


//
// Routine Description:
//
//  ECSPIpDeviceLogInit is called to create a per device log.
//  To differentiate among the ECSPI devices, the routine creates
//  a separate trace log for the device and uses the given device unique 
//  DeviceLogId the identifier.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
//  DeviceLogId - The unique device trace ID
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIpDeviceLogInit (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ULONG DeviceLogId
    )
{
    RECORDER_LOG_CREATE_PARAMS recorderLogCreateParams;
    RECORDER_LOG_CREATE_PARAMS_INIT(&recorderLogCreateParams, nullptr);

    recorderLogCreateParams.TotalBufferSize = ECSPI_RECORDER_TOTAL_BUFFER_SIZE;
    recorderLogCreateParams.ErrorPartitionSize = ECSPI_RECORDER_ERROR_PARTITION_SIZE;

    RtlStringCchPrintfA(
        recorderLogCreateParams.LogIdentifier,
        sizeof(recorderLogCreateParams.LogIdentifier),
        ECSPI_RECORDER_LOG_ID,
        DeviceLogId
        );

    NTSTATUS status = WppRecorderLogCreate(
        &recorderLogCreateParams,
        &DevExtPtr->IfrLogHandle
        );
    if (!NT_SUCCESS(status)) {

        NT_ASSERTMSG(
            "Unable to create trace log recorder!"
            "Default log will be used instead", 
            FALSE
            );
        DevExtPtr->IfrLogHandle = WppRecorderLogGetDefault();
    }
}


//
// Routine Description:
//
//  ECSPIpDeviceLogDeinit is called to delete the device trace log.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIpDeviceLogDeinit (
    ECSPI_DEVICE_EXTENSION* DevExtPtr
    )
{
    if (DevExtPtr->IfrLogHandle != NULL) {

        WppRecorderLogDelete(DevExtPtr->IfrLogHandle);
    }
    DevExtPtr->IfrLogHandle = NULL;
}


//
// Routine Description:
//
//  ECSPIpDeviceSetGpioPin is called to change the state of the
//  CS GPIO pin.
//
// Arguments:
//
//  TrgCtxPtr - The target context.
//
//  AssertNegateValue - The new GPIO pin value,
//      should be ECSPISpbGetCsActiveValue(TrgCtxPtr) or 
//      ECSPISpbGetCsNonActiveValue(TrgCtxPtr).
//
// Return Value:
//
//  NT_STATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPIpDeviceSetGpioPin (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr,
    ULONG AssertNegateValue
    )
{
    NTSTATUS status;
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;

    ECSPI_ASSERT(
        devExtPtr->IfrLogHandle,
        (AssertNegateValue == ECSPISpbGetCsActiveValue(TrgCtxPtr)) ||
            (AssertNegateValue == ECSPISpbGetCsNonActiveValue(TrgCtxPtr))
        );

    //
    // Prepare the GPIO request
    //
    ECSPI_CS_GPIO_PIN* csGpioPinPtr = ECSPIDeviceGetCsGpio(TrgCtxPtr);
    ECSPI_SPB_REQUEST* requestPtr = &devExtPtr->CurrentRequest;
    WDFIOTARGET wdfIoTargetGpio = csGpioPinPtr->WdfIoTargetGpio;
    WDFMEMORY wdfMemoryGpio = csGpioPinPtr->WdfMemoryGpio;
    BOOLEAN isAssert = AssertNegateValue == ECSPISpbGetCsActiveValue(TrgCtxPtr);

    WDFREQUEST wdfRequestGpio = isAssert ? 
        csGpioPinPtr->WdfRequestGpioAssert : 
        csGpioPinPtr->WdfRequestGpioNegate;
    {
        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            (wdfIoTargetGpio != NULL) &&
            (wdfRequestGpio != NULL) &&
            (wdfMemoryGpio != NULL)
            );

        WDF_REQUEST_REUSE_PARAMS wdfRequestReuseParams;
        WDF_REQUEST_REUSE_PARAMS_INIT(
            &wdfRequestReuseParams,
            WDF_REQUEST_REUSE_NO_FLAGS,
            STATUS_SUCCESS
            );

        status = WdfRequestReuse(wdfRequestGpio, &wdfRequestReuseParams);
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "WdfRequestReuse failed for Read Request, "
                "(status = %!STATUS!)",
                status
                );
            return status;
        }

        csGpioPinPtr->GpioData = AssertNegateValue;

        status = WdfIoTargetFormatRequestForIoctl(
            wdfIoTargetGpio,
            wdfRequestGpio,
            IOCTL_GPIO_WRITE_PINS,
            wdfMemoryGpio,
            nullptr,
            NULL,
            nullptr
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                devExtPtr->IfrLogHandle,
                "WdfIoTargetFormatRequestForIoctl failed, for GPIO request"
                "(status = %!STATUS!)",
                status
                );
            return status;
        }

        WdfRequestSetCompletionRoutine(
            wdfRequestGpio,
            isAssert ? 
                ECSPIpGpioAssertCompletionRoutine : 
                ECSPIpGpioNegateCompletionRoutine,
            requestPtr
            );

    } // Prepare the GPIO request

    BOOLEAN isRequestSent = WdfRequestSend(
        wdfRequestGpio,
        wdfIoTargetGpio,
        WDF_NO_SEND_OPTIONS
        );
    if (!isRequestSent) {

        status = WdfRequestGetStatus(wdfRequestGpio);
        ECSPI_LOG_ERROR(
            devExtPtr->IfrLogHandle,
            "WdfRequestSend failed for 'CS Assert' GPIO Request, "
            "(status = %!STATUS!)",
            status
            );
        return status;
    }

    return STATUS_PENDING;
}


//
// Routine Description:
//
//  This is completion routine of the CS GPIO pin assertion.
//  If request was successful the routine starts the transfer.
//
// Arguments:
//
//  WdfRequest - The IOCTL_GPIO_WRITE_PINS request.
//
//  WdfIoTarget - The GPIO driver WDF IO Target
//
//  WdfRequestCompletopnParamsPtr - Completion parameters
//
//  WdfContext - The ECSPI_SPB_REQUEST.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIpGpioAssertCompletionRoutine (
    WDFREQUEST /*WdfRequest*/,
    WDFIOTARGET /*WdfIoTarget*/,
    PWDF_REQUEST_COMPLETION_PARAMS WdfRequestCompletopnParamsPtr,
    WDFCONTEXT WdfContext
    )
{
    ECSPI_SPB_REQUEST* requestPtr =
        static_cast<ECSPI_SPB_REQUEST*>(WdfContext);
    ECSPI_TARGET_CONTEXT* trgCtxPtr = requestPtr->SpbTargetPtr;
    ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;
    ECSPI_CS_GPIO_PIN* csGpioPinPtr = ECSPIDeviceGetCsGpio(trgCtxPtr);

    KeSetEvent(&csGpioPinPtr->SyncCallEvent, 0, FALSE);

    //
    // During lock controller request...
    //
    {
        SPBREQUEST lockRequest = devExtPtr->LockUnlockRequest;
        if (lockRequest != NULL) {

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                requestPtr->SpbRequest == NULL
                );
            devExtPtr->LockUnlockRequest = NULL;
            SpbRequestComplete(lockRequest, STATUS_SUCCESS);
            return;
        }

    } // During lock controller request...

    //
    // Normal transfer
    //
    {
        ULONG_PTR information = 0;
        NTSTATUS status = WdfRequestCompletopnParamsPtr->IoStatus.Status;
        if (!NT_SUCCESS(status)) {

            ECSPISpbCompleteTransferRequest(requestPtr, status, information);
            return;
        }

        ECSPI_ASSERT(
            devExtPtr->IfrLogHandle,
            csGpioPinPtr->GpioData ==
            ECSPISpbGetCsActiveValue(requestPtr->SpbTargetPtr)
            );

        status = ECSPIDeviceEnableRequestCancellation(requestPtr);
        if (!NT_SUCCESS(status)) {

            return;
        }

        //
        // Now that CS is asserted we can start the transfer...
        //
        status = ECSPISpbStartTransferSafe(requestPtr);
        if (!NT_SUCCESS(status)) {

            ECSPISpbCompleteTransferRequest(requestPtr, status, information);
            return;
        }

        //
        // Transfer has started successfully!
        //

    } // Normal transfer
}


//
// Routine Description:
//
//  This is completion routine of the CS GPIO pin negation.
//  The routine complete the SPB transfer requests.
//
// Arguments:
//
//  WdfRequest - The IOCTL_GPIO_WRITE_PINS request.
//
//  WdfIoTarget - The GPIO driver WDF IO Target
//
//  WdfRequestCompletopnParamsPtr - Completion parameters
//
//  WdfContext - The ECSPI_SPB_REQUEST.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIpGpioNegateCompletionRoutine (
    WDFREQUEST /*WdfRequest*/,
    WDFIOTARGET /*WdfIoTarget*/,
    PWDF_REQUEST_COMPLETION_PARAMS WdfRequestCompletopnParamsPtr,
    WDFCONTEXT WdfContext
    )
{
    ECSPI_SPB_REQUEST* requestPtr =
        static_cast<ECSPI_SPB_REQUEST*>(WdfContext);
    ECSPI_TARGET_CONTEXT* trgCtxPtr = requestPtr->SpbTargetPtr;
    ECSPI_DEVICE_EXTENSION* devExtPtr = trgCtxPtr->DevExtPtr;
    ECSPI_CS_GPIO_PIN* csGpioPinPtr = ECSPIDeviceGetCsGpio(trgCtxPtr);

    KeSetEvent(&csGpioPinPtr->SyncCallEvent, 0, FALSE);

    //
    // During controller unlock request...
    //
    {
        SPBREQUEST unlockRequest = devExtPtr->LockUnlockRequest;
        if (unlockRequest != NULL) {

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                requestPtr->SpbRequest == NULL
            );
            devExtPtr->LockUnlockRequest = NULL;
            SpbRequestComplete(unlockRequest, STATUS_SUCCESS);
            return;
        }

    } // During controller unlock request...

    //
    // Normal transfer...
    //
    {
        ULONG_PTR information = 0;
        NTSTATUS status = WdfRequestCompletopnParamsPtr->IoStatus.Status;

        //
        // We do not need to sync access to requestPtr->SpbRequest with
        // cancel routine, since ECSPIDeviceNegateCS was either called from 
        // the DPC, where cancel routine has been removed, or from 
        // the cancel routine itself.
        //
        SPBREQUEST spbRequest = requestPtr->SpbRequest;
        if (spbRequest != NULL) {

            ECSPI_ASSERT(
                devExtPtr->IfrLogHandle,
                csGpioPinPtr->GpioData ==
                    ECSPISpbGetCsNonActiveValue(requestPtr->SpbTargetPtr)
                );

            if (requestPtr->State == ECSPI_REQUEST_STATE::CANCELED) {

                status = STATUS_CANCELLED;

            } else {

                information = requestPtr->TotalBytesTransferred;
            }

            ECSPISpbCompleteTransferRequest(requestPtr, status, information);
        }

    } // Normal transfer...
}


#undef _ECSPI_DEVICE_CPP_
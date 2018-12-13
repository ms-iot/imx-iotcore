// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   file.cpp
//
// Abstract:
//
//  This module contains methods implementation for the file object create/close
//  callbacks.
//
// Environment:
//
//  Kernel mode only
//

#include "precomp.h"
#pragma hdrstop

#include "imxpwmhw.hpp"
#include "imxpwm.hpp"

#include "trace.h"
#include "file.tmh"

IMXPWM_PAGED_SEGMENT_BEGIN; //==================================================

_Use_decl_annotations_
VOID
ImxPwmEvtDeviceFileCreate (
    WDFDEVICE WdfDevice,
    WDFREQUEST WdfRequest,
    WDFFILEOBJECT WdfFileObject
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    UNICODE_STRING* filenamePtr = WdfFileObjectGetFileName(WdfFileObject);
    IMXPWM_DEVICE_CONTEXT* deviceContextPtr = ImxPwmGetDeviceContext(WdfDevice);
    NTSTATUS status;
    ULONG pinNumber = ULONG_MAX;

    //
    // Parse and validate the filename associated with the file object.
    //
    bool isPinInterface;
    if (filenamePtr == nullptr) {
        WdfRequestComplete(WdfRequest, STATUS_INVALID_DEVICE_REQUEST);
        return;
    } else if (filenamePtr->Length > 0) {
        //
        // A non-empty filename means to open a pin under the controller namespace.
        //
        status = PwmParsePinPath(filenamePtr, &pinNumber);
        if (!NT_SUCCESS(status)) {
            WdfRequestComplete(WdfRequest, status);
            return;
        }

        NT_ASSERT(deviceContextPtr->ControllerInfo.PinCount == 1);
        if (pinNumber >= deviceContextPtr->ControllerInfo.PinCount) {
            IMXPWM_LOG_INFORMATION(
                "Requested pin number out of bounds. (pinNumber = %lu)",
                pinNumber);
            WdfRequestComplete(WdfRequest, STATUS_NO_SUCH_FILE);
            return;
        }

        isPinInterface = true;
    } else {
        //
        // An empty filename means that the create is against the root controller.
        //
        isPinInterface = false;
    }

    ACCESS_MASK desiredAccess;
    ULONG shareAccess;
    ImxPwmCreateRequestGetAccess(WdfRequest, &desiredAccess, &shareAccess);

    //
    // ShareAccess will not be honored as it has no meaning currently in the
    // PWM DDI.
    //
    if (shareAccess != 0) {
        IMXPWM_LOG_INFORMATION(
            "Requested share access is not supported and request ShareAccess "
            "parameter should be zero. Access denied. (shareAccess = %lu)",
            shareAccess);
        WdfRequestComplete(WdfRequest, STATUS_SHARING_VIOLATION);
        return;
    }

    //
    // Verify request desired access.
    //
    const bool hasWriteAccess = ((desiredAccess & FILE_WRITE_DATA) != 0);

    if (isPinInterface) {
        IMXPWM_PIN_STATE* pinPtr = &deviceContextPtr->Pin;

        WdfWaitLockAcquire(pinPtr->Lock, NULL);
         
        if (hasWriteAccess) {
            if (pinPtr->IsOpenForWrite) {
                WdfWaitLockRelease(pinPtr->Lock);
                IMXPWM_LOG_ERROR("Pin access denied.");
                WdfRequestComplete(WdfRequest, STATUS_SHARING_VIOLATION);
                return;
            }
            pinPtr->IsOpenForWrite = true;
        }

        IMXPWM_LOG_INFORMATION(
            "Pin Opened. (IsOpenForWrite = %!bool!)",
            pinPtr->IsOpenForWrite);

        WdfWaitLockRelease(pinPtr->Lock);

    } else {

        WdfWaitLockAcquire(deviceContextPtr->ControllerLock, NULL);

        if (hasWriteAccess) {
            if (deviceContextPtr->IsControllerOpenForWrite) {
                WdfWaitLockRelease(deviceContextPtr->ControllerLock);
                IMXPWM_LOG_ERROR("Controller access denied.");
                WdfRequestComplete(WdfRequest, STATUS_SHARING_VIOLATION);
                return;
            }
            deviceContextPtr->IsControllerOpenForWrite = true;
        }

        IMXPWM_LOG_INFORMATION(
            "Controller Opened. (IsControllerOpenForWrite = %!bool!)",
            deviceContextPtr->IsControllerOpenForWrite);

        WdfWaitLockRelease(deviceContextPtr->ControllerLock);
    }

    //
    // Allocate and fill a file object context.
    //
    IMXPWM_FILE_OBJECT_CONTEXT* fileObjectContextPtr;
    {
        WDF_OBJECT_ATTRIBUTES wdfObjectAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &wdfObjectAttributes,
            IMXPWM_FILE_OBJECT_CONTEXT);

        void* contextPtr;
        status = WdfObjectAllocateContext(
                WdfFileObject,
                &wdfObjectAttributes,
                &contextPtr);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfObjectAllocateContext(...) failed. (status = %!STATUS!)",
                status);
            WdfRequestComplete(WdfRequest, status);
            return;
        }

        fileObjectContextPtr =
            static_cast<IMXPWM_FILE_OBJECT_CONTEXT*>(contextPtr);

        NT_ASSERT(fileObjectContextPtr != nullptr);
        fileObjectContextPtr->IsPinInterface = isPinInterface;
        fileObjectContextPtr->IsOpenForWrite = hasWriteAccess;
    }

    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
VOID
ImxPwmEvtFileClose (
    WDFFILEOBJECT WdfFileObject
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    WDFDEVICE wdfDevice = WdfFileObjectGetDevice(WdfFileObject);
    IMXPWM_DEVICE_CONTEXT* deviceContextPtr = ImxPwmGetDeviceContext(wdfDevice);
    IMXPWM_FILE_OBJECT_CONTEXT* fileObjectContextPtr = ImxPwmGetFileObjectContext(WdfFileObject);

    if (fileObjectContextPtr->IsPinInterface) {
        WdfWaitLockAcquire(deviceContextPtr->Pin.Lock, NULL);

        if (fileObjectContextPtr->IsOpenForWrite) {

            NTSTATUS status = ImxPwmResetPinDefaults(deviceContextPtr);
            if (!NT_SUCCESS(status)) {
                IMXPWM_LOG_ERROR(
                    "ImxPwmResetPinDefaults(...) failed. (status = %!STATUS!)",
                    status);
            }

            NT_ASSERT(deviceContextPtr->Pin.IsOpenForWrite);
            deviceContextPtr->Pin.IsOpenForWrite = false;
        }

        IMXPWM_LOG_TRACE(
            "Pin Closed. (IsOpenForWrite = %lu)",
            (deviceContextPtr->Pin.IsOpenForWrite ? 1 : 0));

        WdfWaitLockRelease(deviceContextPtr->Pin.Lock);

    } else {

        WdfWaitLockAcquire(deviceContextPtr->ControllerLock, NULL);

        if (fileObjectContextPtr->IsOpenForWrite) {
            NTSTATUS status = ImxPwmResetControllerDefaults(deviceContextPtr);
            if (!NT_SUCCESS(status)) {
                IMXPWM_LOG_ERROR(
                    "ImxPwmResetControllerDefaults(...) failed. (status = %!STATUS!)",
                    status);
            }

            NT_ASSERT(deviceContextPtr->IsControllerOpenForWrite);
            deviceContextPtr->IsControllerOpenForWrite = false;
        }

        IMXPWM_LOG_TRACE(
            "Controller Closed. (IsControllerOpenForWrite = %lu)",
            (deviceContextPtr->IsControllerOpenForWrite ? 1 : 0));

        WdfWaitLockRelease(deviceContextPtr->ControllerLock);
    }
}

IMXPWM_PAGED_SEGMENT_END; //===================================================
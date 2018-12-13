// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   ioctl.cpp
//
// Abstract:
//
//  This module contains PWM IOCTLs implementation.
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
#include "ioctl.tmh"

IMXPWM_NONPAGED_SEGMENT_BEGIN; //==============================================

_Use_decl_annotations_
VOID
ImxPwmEvtIoDeviceControl (
    WDFQUEUE WdfQueue,
    WDFREQUEST WdfRequest,
    size_t /*OutputBufferLength*/,
    size_t /*InputBufferLength*/,
    ULONG IoControlCode
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    WDFDEVICE wdfDevice = WdfIoQueueGetDevice(WdfQueue);
    WDFFILEOBJECT wdfFileObject = WdfRequestGetFileObject(WdfRequest);
    IMXPWM_DEVICE_CONTEXT *deviceContextPtr = ImxPwmGetDeviceContext(wdfDevice);
    IMXPWM_FILE_OBJECT_CONTEXT *fileObjectContextPtr = ImxPwmGetFileObjectContext(wdfFileObject);

    if (fileObjectContextPtr->IsPinInterface) {
        //
        // Is Pin Interface
        //
        switch (IoControlCode) {
        case IOCTL_PWM_CONTROLLER_GET_INFO:
        case IOCTL_PWM_CONTROLLER_GET_ACTUAL_PERIOD:
        case IOCTL_PWM_CONTROLLER_SET_DESIRED_PERIOD:
            IMXPWM_LOG_INFORMATION(
                "Controller IOCTL directed to a pin. (IoControlCode = 0x%x)",
                IoControlCode);
            WdfRequestComplete(WdfRequest, STATUS_INVALID_DEVICE_REQUEST);
            break;
        case IOCTL_PWM_PIN_GET_POLARITY:
            ImxPwmIoctlPinGetPolarity(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_PIN_SET_POLARITY:
            ImxPwmIoctlPinSetPolarity(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_PIN_GET_ACTIVE_DUTY_CYCLE_PERCENTAGE:
            ImxPwmIoctlPinGetActiveDutyCycle(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_PIN_SET_ACTIVE_DUTY_CYCLE_PERCENTAGE:
            ImxPwmIoctlPinSetActiveDutyCycle(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_PIN_START:
            ImxPwmIoctlPinStart(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_PIN_STOP:
            ImxPwmIoctlPinStop(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_PIN_IS_STARTED:
            ImxPwmIoctlPinIsStarted(deviceContextPtr, WdfRequest);
            break;
        default:
            IMXPWM_LOG_INFORMATION("IOCTL not supported. (IoControlCode = 0x%x)", IoControlCode);
            WdfRequestComplete(WdfRequest, STATUS_NOT_SUPPORTED);
            return;
        }
    } else {
        //
        // Is Controller Interface
        //
        switch (IoControlCode) {
        case IOCTL_PWM_CONTROLLER_GET_INFO:
            ImxPwmIoctlControllerGetInfo(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_CONTROLLER_GET_ACTUAL_PERIOD:
            ImxPwmIoctlControllerGetActualPeriod(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_CONTROLLER_SET_DESIRED_PERIOD:
            ImxPwmIoctlControllerSetDesiredPeriod(deviceContextPtr, WdfRequest);
            break;
        case IOCTL_PWM_PIN_GET_POLARITY:
        case IOCTL_PWM_PIN_SET_POLARITY:
        case IOCTL_PWM_PIN_GET_ACTIVE_DUTY_CYCLE_PERCENTAGE:
        case IOCTL_PWM_PIN_SET_ACTIVE_DUTY_CYCLE_PERCENTAGE:
        case IOCTL_PWM_PIN_START:
        case IOCTL_PWM_PIN_STOP:
        case IOCTL_PWM_PIN_IS_STARTED:
            IMXPWM_LOG_INFORMATION(
                "Pin IOCTL directed to a controller. (IoControlCode = 0x%x)",
                IoControlCode);
            WdfRequestComplete(WdfRequest, STATUS_INVALID_DEVICE_REQUEST);
            break;
        default:
            IMXPWM_LOG_INFORMATION("IOCTL not supported. (IoControlCode = 0x%x)", IoControlCode);
            WdfRequestComplete(WdfRequest, STATUS_NOT_SUPPORTED);
            return;
        }
    }
}

_Use_decl_annotations_
VOID
ImxPwmIoctlControllerGetInfo(
    const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    PWM_CONTROLLER_GET_INFO_OUTPUT* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveOutputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    RtlCopyMemory(
        outputBufferPtr,
        &DeviceContextPtr->ControllerInfo,
        sizeof(*outputBufferPtr));

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
VOID
ImxPwmIoctlControllerGetActualPeriod (
    const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    PWM_CONTROLLER_GET_ACTUAL_PERIOD_OUTPUT* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveOutputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    outputBufferPtr->ActualPeriod = DeviceContextPtr->ActualPeriod;

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
VOID
ImxPwmIoctlControllerSetDesiredPeriod (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    PWM_CONTROLLER_SET_DESIRED_PERIOD_INPUT* inputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        WdfRequest,
        sizeof(*inputBufferPtr),
        reinterpret_cast<PVOID*>(&inputBufferPtr),
        nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveInputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    PWM_CONTROLLER_SET_DESIRED_PERIOD_OUTPUT* outputBufferPtr;
    status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveOutputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    if ((inputBufferPtr->DesiredPeriod <
            DeviceContextPtr->ControllerInfo.MinimumPeriod) ||
        (inputBufferPtr->DesiredPeriod >
            DeviceContextPtr->ControllerInfo.MaximumPeriod)) {
        IMXPWM_LOG_INFORMATION(
            "DesiredPeriod %llu out of controller limits. "
            "(MinimumPeriod = %llu, MaximumPeriod = %llu)",
            inputBufferPtr->DesiredPeriod,
            DeviceContextPtr->ControllerInfo.MinimumPeriod,
            DeviceContextPtr->ControllerInfo.MaximumPeriod);
        WdfRequestComplete(WdfRequest, STATUS_INVALID_PARAMETER);
        return;
    }

    status =
        ImxPwmSetDesiredPeriod(
            DeviceContextPtr,
            inputBufferPtr->DesiredPeriod);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    outputBufferPtr->ActualPeriod = DeviceContextPtr->ActualPeriod;

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
VOID
ImxPwmIoctlPinGetActiveDutyCycle (
    const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    PWM_PIN_GET_ACTIVE_DUTY_CYCLE_PERCENTAGE_OUTPUT* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveOutputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    outputBufferPtr->Percentage = DeviceContextPtr->Pin.ActiveDutyCycle;

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
VOID
ImxPwmIoctlPinSetActiveDutyCycle (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    DeviceContextPtr->CurrentRequest = WdfRequest;

    PWM_PIN_SET_ACTIVE_DUTY_CYCLE_PERCENTAGE_INPUT* inputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        WdfRequest,
        sizeof(*inputBufferPtr),
        reinterpret_cast<PVOID*>(&inputBufferPtr),
        nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveInputBuffer(..) failed. (status = %!STATUS!)",
            status);

        DeviceContextPtr->CurrentRequest = NULL;
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    status =
        ImxPwmSetActiveDutyCycle(
            DeviceContextPtr,
            inputBufferPtr->Percentage);
    if (!NT_SUCCESS(status)) {
        DeviceContextPtr->CurrentRequest = NULL;
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    NT_ASSERT((status == STATUS_PENDING) || (status == STATUS_SUCCESS));

    if (status != STATUS_PENDING) {
        NT_ASSERT(status == STATUS_SUCCESS);
        DeviceContextPtr->CurrentRequest = NULL;
        WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
    }
}

_Use_decl_annotations_
VOID
ImxPwmIoctlPinGetPolarity (
    const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    PWM_PIN_GET_POLARITY_OUTPUT* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveOutputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    outputBufferPtr->Polarity = DeviceContextPtr->Pin.Polarity;

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
VOID
ImxPwmIoctlPinSetPolarity (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    PWM_PIN_SET_POLARITY_INPUT* inputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        WdfRequest,
        sizeof(*inputBufferPtr),
        reinterpret_cast<PVOID*>(&inputBufferPtr),
        nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveInputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    switch (inputBufferPtr->Polarity) {
    case PWM_ACTIVE_HIGH:
    case PWM_ACTIVE_LOW:
        break;

    default:
        WdfRequestComplete(WdfRequest, STATUS_INVALID_PARAMETER);
        return;
    }

    IMXPWM_PIN_STATE* pinPtr = &DeviceContextPtr->Pin;

    if (pinPtr->Polarity == inputBufferPtr->Polarity) {
        WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
        return;
    } else if (pinPtr->IsStarted != false) {
        WdfRequestComplete(WdfRequest, STATUS_INVALID_DEVICE_STATE);
        return;
    }

    status =
        ImxPwmSetPolarity(
            DeviceContextPtr,
            inputBufferPtr->Polarity);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
VOID
ImxPwmIoctlPinStart (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    if (DeviceContextPtr->Pin.IsStarted) {
        WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
        return;
    }

    NTSTATUS status = ImxPwmStart(DeviceContextPtr);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    //
    // Start always finish inline since the Fifo will be empty and hence
    // no ISR/DPC will be required to set duty cycle.
    //
    NT_ASSERT(status == STATUS_SUCCESS);

    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
VOID
ImxPwmIoctlPinStop (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    if (!DeviceContextPtr->Pin.IsStarted) {
        WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
        return;
    }

    NTSTATUS status = ImxPwmStop(DeviceContextPtr);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
VOID
ImxPwmIoctlPinIsStarted (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    IMXPWM_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
    IMXPWM_LOG_TRACE("()");

    PWM_PIN_IS_STARTED_OUTPUT* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfRequestRetrieveOutputBuffer(..) failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    outputBufferPtr->IsStarted = DeviceContextPtr->Pin.IsStarted;

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

IMXPWM_NONPAGED_SEGMENT_END; //=================================================
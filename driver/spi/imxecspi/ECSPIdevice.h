// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPIdevice.h
//
// Abstract:
//
//    This module contains all the enums, types, and functions related to
//    an IMX ECSPI controller device.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//

#ifndef _ECSPI_DEVICE_H_
#define _ECSPI_DEVICE_H_

WDF_EXTERN_C_START


//
// ECSPI_CS_GPIO_PIN.
//  A CS GPIO pin descriptor.
//
typedef struct _ECSPI_CS_GPIO_PIN
{
    //
    // The CS GPIO pin connection ID
    //
    LARGE_INTEGER GpioConnectionId;

    //
    // The CS GPIO pin device handle
    //
    WDFIOTARGET WdfIoTargetGpio;

    //
    // The CS GPIO data, and memory objects
    //
    ULONG GpioData;
    WDFMEMORY WdfMemoryGpio;

    //
    // The CS GPIO requests
    //
    WDFREQUEST WdfRequestGpioAssert;
    WDFREQUEST WdfRequestGpioNegate;

    //
    // Used for synchronous settings of GPIO pin.
    //
    KEVENT SyncCallEvent;

    //
    // Open count
    //
    ULONG OpenCount;

} ECSPI_CS_GPIO_PIN;


//
// ECSPI_DEVICE_EXTENSION.
//  Contains all The ECSPI device runtime parameters.
//  It is associated with the WDFDEVICE object.
//
typedef struct _ECSPI_DEVICE_EXTENSION
{
    //
    // The WDFDEVICE associated with this instance of the
    // controller driver.
    //
    WDFDEVICE WdfDevice;

    //
    // Device lock
    //
    KSPIN_LOCK DeviceLock;

    //
    // The LOG handle for this device
    //
    RECORDER_LOG IfrLogHandle;

    //
    // ECSPI mapped resources
    //

    //
    // ECSPI register file
    //
    volatile ECSPI_REGISTERS* ECSPIRegsPtr;

    //
    // ECSPI interrupt object
    //
    WDFINTERRUPT WdfSpiInterrupt;

    //
    //  Runtime...
    //

    //
    // If controller is locked
    //
    BOOLEAN IsControllerLocked;

    //
    // Lock/unlock request
    //
    SPBREQUEST LockUnlockRequest;

    //
    // The current request in progress
    //
    ECSPI_SPB_REQUEST CurrentRequest;

    //
    // The CS GPIO pin descriptors
    //
    ECSPI_CS_GPIO_PIN CsGpioPins[ECSPI_CHANNEL::COUNT];

} ECSPI_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ECSPI_DEVICE_EXTENSION, ECSPIDeviceGetExtension);


//
// ECSPIdevice WDF device event handlers
//
EVT_WDF_DEVICE_PREPARE_HARDWARE ECSPIEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE ECSPIEvtDeviceReleaseHardware;
EVT_WDF_INTERRUPT_ISR ECSPIEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC ECSPIEvtInterruptDpc;
EVT_WDF_REQUEST_CANCEL ECSPIEvtRequestCancel;

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
ECSPIDeviceEnableRequestCancellation (
    _In_ ECSPI_SPB_REQUEST* RequestPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
ECSPIDeviceDisableRequestCancellation (
    _In_ ECSPI_SPB_REQUEST* RequestPtr
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ECSPIDeviceOpenGpioTarget (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
ECSPIDeviceCloseGpioTarget (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr
    );

_When_(IsWait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(IsWait == 1, _IRQL_requires_max_(APC_LEVEL))
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
ECSPIDeviceAssertCS (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr,
    _In_ BOOLEAN IsWait
    );

_When_(IsWait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(IsWait == 1, _IRQL_requires_max_(APC_LEVEL))
NTSTATUS
ECSPIDeviceNegateCS (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr,
    _In_ BOOLEAN IsWait
    );

//
// Routine Description:
//
//  ECSPIDeviceGetGpio get the CS GPIO descriptor
//  based on the target device selection.
//
// Arguments:
//
//  TrgCtxPtr - The target context
//
// Return Value:
//
//  The CS GPIO device descriptor address.
//
__forceinline
ECSPI_CS_GPIO_PIN*
ECSPIDeviceGetCsGpio (
    _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;

    return &devExtPtr->CsGpioPins[TrgCtxPtr->Settings.DeviceSelection];
}


//
// ECSPIdevice private methods
//
#ifdef _ECSPI_DEVICE_CPP_

    _IRQL_requires_max_(PASSIVE_LEVEL)
    static VOID
    ECSPIpDeviceLogInit (
        _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr,
        _In_ ULONG DeviceLogId
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    static VOID
    ECSPIpDeviceLogDeinit (
        _In_ ECSPI_DEVICE_EXTENSION* DevExtPtr
        );

    _IRQL_requires_max_(DISPATCH_LEVEL)
    static NTSTATUS
    ECSPIpDeviceSetGpioPin (
        _In_ ECSPI_TARGET_CONTEXT* TrgCtxPtr,
        _In_ ULONG Value
        );

    static EVT_WDF_REQUEST_COMPLETION_ROUTINE ECSPIpGpioAssertCompletionRoutine;
    static EVT_WDF_REQUEST_COMPLETION_ROUTINE ECSPIpGpioNegateCompletionRoutine;

#endif // _ECSPI_DEVICE_CPP_

WDF_EXTERN_C_END

#endif // !_ECSPI_DEVICE_H_
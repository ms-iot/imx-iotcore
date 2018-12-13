// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   device.cpp
//
// Abstract:
//
//  This module contains methods implementation for the device initialization.
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
#include "device.tmh"

IMXPWM_PAGED_SEGMENT_BEGIN; //==================================================

_Use_decl_annotations_
NTSTATUS
ImxPwmEvtDevicePrepareHardware (
    WDFDEVICE WdfDevice,
    WDFCMRESLIST /*ResourcesRaw*/,
    WDFCMRESLIST ResourcesTranslated
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    const CM_PARTIAL_RESOURCE_DESCRIPTOR* memResourcePtr = nullptr;
    ULONG interruptResourceCount = 0;

    //
    // Look for single memory and interrupt resource.
    //
    const ULONG resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    for (ULONG i = 0; i < resourceCount; ++i) {
        const CM_PARTIAL_RESOURCE_DESCRIPTOR* resourcePtr =
            WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch (resourcePtr->Type) {
        case CmResourceTypeMemory:
            if (memResourcePtr != nullptr) {
                IMXPWM_LOG_ERROR(
                    "Received unexpected memory resource. (resourcePtr = 0x%p)",
                    resourcePtr);

                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            memResourcePtr = resourcePtr;
            break;

        case CmResourceTypeInterrupt:
            if (interruptResourceCount > 0) {
                IMXPWM_LOG_ERROR(
                    "Received unexpected interrupt resource. "
                    "(interruptResourceCount = %lu, resourcePtr = 0x%p)",
                    interruptResourceCount,
                    resourcePtr);

                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            ++interruptResourceCount;
            break;
        }
    }

    if ((memResourcePtr == nullptr) || (interruptResourceCount < 1)) {
        IMXPWM_LOG_ERROR(
            "Did not receive required memory resource and interrupt resource. "
            "(memResourcePtr = 0x%p, interruptResourceCount = %lu)",
            memResourcePtr,
            interruptResourceCount);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (memResourcePtr->u.Memory.Length < sizeof(IMXPWM_REGISTERS)) {
        IMXPWM_LOG_ERROR(
            "Memory resource is too small. "
            "(memResourcePtr->u.Memory.Length = %lu, "
            "sizeof(IMXPWM_REGISTERS) = %lu)",
            memResourcePtr->u.Memory.Length,
            sizeof(IMXPWM_REGISTERS));

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // ReleaseHardware is ALWAYS called, even if PrepareHardware fails, so
    // the cleanup of registersPtr is handled there
    //
    IMXPWM_DEVICE_CONTEXT* deviceContextPtr =
            ImxPwmGetDeviceContext(WdfDevice);

    NT_ASSERT(memResourcePtr->Type == CmResourceTypeMemory);
    deviceContextPtr->RegistersPtr = static_cast<IMXPWM_REGISTERS*>(
        MmMapIoSpaceEx(
            memResourcePtr->u.Memory.Start,
            sizeof(IMXPWM_REGISTERS),
            PAGE_READWRITE | PAGE_NOCACHE));

    if (deviceContextPtr->RegistersPtr == nullptr) {
        IMXPWM_LOG_LOW_MEMORY(
            "MmMapIoSpaceEx(...) failed. "
            "(memResourcePtr->u.Memory.Start = 0x%llx, "
            "sizeof(IMXPWM_REGISTERS) = %lu)",
            memResourcePtr->u.Memory.Start.QuadPart,
            sizeof(IMXPWM_REGISTERS));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmEvtDeviceReleaseHardware (
    WDFDEVICE WdfDevice,
    WDFCMRESLIST /*ResourcesTranslated*/
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMXPWM_DEVICE_CONTEXT* deviceContextPtr =
            ImxPwmGetDeviceContext(WdfDevice);

    if (deviceContextPtr->RegistersPtr != nullptr) {
        MmUnmapIoSpace(
            deviceContextPtr->RegistersPtr,
            sizeof(IMXPWM_REGISTERS));

        deviceContextPtr->RegistersPtr = nullptr;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmResetControllerDefaults (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMXPWM_LOG_TRACE("()");

    NTSTATUS status =
        ImxPwmSetDesiredPeriod(
            DeviceContextPtr,
            DeviceContextPtr->DefaultDesiredPeriod);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "ImxPwmSetDesiredPeriod(...) failed. (status = %!STATUS!)",
            status);
        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmResetPinDefaults (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMXPWM_LOG_TRACE("()");

    IMXPWM_PIN_STATE* pinPtr = &DeviceContextPtr->Pin;
    NTSTATUS status;

    if (pinPtr->IsStarted) {
        status = ImxPwmStop(DeviceContextPtr);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "ImxPwmStop(...) failed. (status = %!STATUS!)",
                status);
            return status;
        }
    }

    DeviceContextPtr->Pin.ActiveDutyCycle = 0;

    status = ImxPwmSetPolarity(DeviceContextPtr, PWM_ACTIVE_HIGH);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "ImxPwmSetPolarity(...) failed. (status = %!STATUS!)",
            status);
        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmEvtDeviceD0Entry (
    WDFDEVICE WdfDevice,
    WDF_POWER_DEVICE_STATE /* PreviousState */
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMXPWM_DEVICE_CONTEXT* deviceContextPtr = ImxPwmGetDeviceContext(WdfDevice);
    NTSTATUS status = ImxPwmSoftReset(deviceContextPtr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "ImxPwmSoftReset(...) failed. (status = %!STATUS!)",
            status);
        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmCreateDeviceInterface (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NTSTATUS status =
        WdfDeviceCreateDeviceInterface(
                DeviceContextPtr->WdfDevice,
                &GUID_DEVINTERFACE_PWM_CONTROLLER,
                nullptr);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "WdfDeviceCreateDeviceInterface(...) failed. (status = %!STATUS!)",
            status);

        return status;
    }

    //
    // Retrieve device interface symbolic link string
    //
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = DeviceContextPtr->WdfDevice;
        status =
            WdfStringCreate(
                nullptr,
                &attributes,
                &DeviceContextPtr->DeviceInterfaceSymlinkName);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfStringCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        status =
            WdfDeviceRetrieveDeviceInterfaceString(
                DeviceContextPtr->WdfDevice,
                &GUID_DEVINTERFACE_PWM_CONTROLLER,
                nullptr,
                DeviceContextPtr->DeviceInterfaceSymlinkName);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfDeviceRetrieveDeviceInterfaceString(...) failed. "
                "(status = %!STATUS!, GUID_DEVINTERFACE_PWM_CONTROLLER = %!GUID!)",
                status,
                &GUID_DEVINTERFACE_PWM_CONTROLLER);

            return status;
        }

        WdfStringGetUnicodeString(
            DeviceContextPtr->DeviceInterfaceSymlinkName,
            &DeviceContextPtr->DeviceInterfaceSymlinkNameWsz);
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmEvtDeviceAdd (
    WDFDRIVER /*WdfDriver*/,
    WDFDEVICE_INIT* DeviceInitPtr
    )
{
    PAGED_CODE();
    IMXPWM_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    //
    // Set PNP and Power callbacks
    //
    {
        WDF_PNPPOWER_EVENT_CALLBACKS wdfPnpPowerEventCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&wdfPnpPowerEventCallbacks);
        wdfPnpPowerEventCallbacks.EvtDevicePrepareHardware =
            ImxPwmEvtDevicePrepareHardware;

        wdfPnpPowerEventCallbacks.EvtDeviceReleaseHardware =
            ImxPwmEvtDeviceReleaseHardware;
        wdfPnpPowerEventCallbacks.EvtDeviceD0Entry =
            ImxPwmEvtDeviceD0Entry;

        WdfDeviceInitSetPnpPowerEventCallbacks(
            DeviceInitPtr,
            &wdfPnpPowerEventCallbacks);

    }

    //
    // Configure file create/close callbacks
    //
    {
        WDF_FILEOBJECT_CONFIG wdfFileObjectConfig;
        WDF_FILEOBJECT_CONFIG_INIT(
            &wdfFileObjectConfig,
            ImxPwmEvtDeviceFileCreate,
            ImxPwmEvtFileClose,
            WDF_NO_EVENT_CALLBACK); // not interested in Cleanup

        WdfDeviceInitSetFileObjectConfig(
            DeviceInitPtr,
            &wdfFileObjectConfig,
            WDF_NO_OBJECT_ATTRIBUTES);
    }

    NTSTATUS status;

    //
    // Create and initialize the WDF device
    //
    WDFDEVICE wdfDevice;
    IMXPWM_DEVICE_CONTEXT* deviceContextPtr;
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMXPWM_DEVICE_CONTEXT);

        status = WdfDeviceCreate(&DeviceInitPtr, &attributes, &wdfDevice);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfDeviceCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        WDF_OBJECT_ATTRIBUTES wdfObjectAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &wdfObjectAttributes,
            IMXPWM_DEVICE_CONTEXT);

        void* contextPtr;
        status =
            WdfObjectAllocateContext(
                wdfDevice,
                &wdfObjectAttributes,
                &contextPtr);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfObjectAllocateContext(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        deviceContextPtr = static_cast<IMXPWM_DEVICE_CONTEXT*>(contextPtr);
        deviceContextPtr->WdfDevice = wdfDevice;
    }

    //
    // Create an interrupt object with an associated context.
    //
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMXPWM_INTERRUPT_CONTEXT);

        WDF_INTERRUPT_CONFIG interruptConfig;
        WDF_INTERRUPT_CONFIG_INIT(
            &interruptConfig,
            ImxPwmEvtInterruptIsr,
            ImxPwmEvtInterruptDpc);

        WDFINTERRUPT wdfInterrupt;
        status =
            WdfInterruptCreate(
                wdfDevice,
                &interruptConfig,
                &attributes,
                &wdfInterrupt);

        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "Failed to create interrupt object. (status = %!STATUS!)",
                status);

            return status;
        }

        deviceContextPtr->WdfInterrupt = wdfInterrupt;
    }

    //
    // Create controller and pin locks
    //
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = wdfDevice;
        status =
            WdfWaitLockCreate(&attributes, &deviceContextPtr->ControllerLock);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfWaitLockCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        status = WdfWaitLockCreate(&attributes, &deviceContextPtr->Pin.Lock);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfWaitLockCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }
    }

    //
    // Set controller info and defaults
    //
    {
        //
        // These should be retrieved from ACPI _DSD, but for now will hardcode
        // clock source and assume clock is properly configured.
        //
        deviceContextPtr->ClockSource = IMXPWM_PWMCR_CLKSRC(IMXPWM_DEFAULT_CLKSRC);
        deviceContextPtr->ClockSourceFrequency = IMXPWM_DEFAULT_CLKSRC_FREQ;
        deviceContextPtr->RovEventCounterCompare = IMXPWM_ROV_EVT_COUNTER_COMPARE;

        auto &info = deviceContextPtr->ControllerInfo;

        info.Size = sizeof(PWM_CONTROLLER_INFO);
        info.PinCount = IMXPWM_PIN_COUNT;

        status =
            ImxPwmCalculatePeriod(
                deviceContextPtr,
                IMXPWM_PRESCALER_MIN,
                &info.MinimumPeriod);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "Failed to compute minimum period. (status = %!STATUS!)",
                status);

            return status;
        }

        status =
            ImxPwmCalculatePeriod(
                deviceContextPtr,
                IMXPWM_PRESCALER_MAX,
                &info.MaximumPeriod);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "Failed to compute maximum period. (status = %!STATUS!)",
                status);

            return status;
        }

        deviceContextPtr->DefaultDesiredPeriod = info.MinimumPeriod;

        NT_ASSERT(
            (info.MinimumPeriod > 0) &&
            (info.MinimumPeriod <= info.MaximumPeriod) &&
            (deviceContextPtr->DefaultDesiredPeriod > 0));
        IMXPWM_LOG_INFORMATION(
            "Controller Info: PinCount = %lu, MinimumPeriod = %llups(%lluHz), "
            "MaximumPeriod = %llups(%lluHz), DefaultPeriod = %llups(%lluHz)",
            info.PinCount,
            info.MinimumPeriod,
            ImxPwmPeriodToFrequency(info.MinimumPeriod),
            info.MaximumPeriod,
            ImxPwmPeriodToFrequency(info.MaximumPeriod),
            deviceContextPtr->DefaultDesiredPeriod,
            ImxPwmPeriodToFrequency(deviceContextPtr->DefaultDesiredPeriod));
    }

    //
    // Set PNP capabilities
    //
    {
        WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
        WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);

        pnpCaps.Removable = WdfFalse;
        pnpCaps.SurpriseRemovalOK = WdfFalse;
        WdfDeviceSetPnpCapabilities(wdfDevice, &pnpCaps);
    }

    //
    // Make the device disable-able
    //
    {
        WDF_DEVICE_STATE wdfDeviceState;
        WDF_DEVICE_STATE_INIT(&wdfDeviceState);

        wdfDeviceState.NotDisableable = WdfFalse;
        WdfDeviceSetDeviceState(wdfDevice, &wdfDeviceState);
    }

    //
    // Create default sequential dispatch queue
    //
    {
        WDF_IO_QUEUE_CONFIG wdfQueueConfig;
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
            &wdfQueueConfig,
            WdfIoQueueDispatchSequential);
        wdfQueueConfig.EvtIoDeviceControl = ImxPwmEvtIoDeviceControl;

        WDF_OBJECT_ATTRIBUTES wdfQueueAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&wdfQueueAttributes);
        wdfQueueAttributes.ExecutionLevel = WdfExecutionLevelPassive;

        WDFQUEUE wdfQueue;
        status = WdfIoQueueCreate(
                wdfDevice,
                &wdfQueueConfig,
                &wdfQueueAttributes,
                &wdfQueue);

        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "WdfIoQueueCreate(..) failed. (status=%!STATUS!)",
                status);

            return status;
        }
    }

    //
    // Publish controller device interface
    //
    status = ImxPwmCreateDeviceInterface(deviceContextPtr);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    IMXPWM_LOG_INFORMATION(
        "Published device interface %wZ",
        &deviceContextPtr->DeviceInterfaceSymlinkNameWsz);

    return STATUS_SUCCESS;
}

IMXPWM_PAGED_SEGMENT_END; //===================================================
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPIdriver.cpp
//
// Abstract:
//
//    This module contains the IMX ECSPI controller driver entry functions.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//
#include "precomp.h"
#pragma hdrstop

#define _ECSPI_DRIVER_CPP_

// Logging header files
#include "ECSPItrace.h"
#include "ECSPIdriver.tmh"

// Common driver header files
#include "ECSPIcommon.h"

// Module specific header files
#include "ECSPIhw.h"
#include "ECSPIspb.h"
#include "ECSPIdriver.h"
#include "ECSPIdevice.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, ECSPIEvtDriverUnload)
    #pragma alloc_text(PAGE, ECSPIEvtDeviceAdd)

    #pragma alloc_text(INIT, ECSPIpDriverReadConfig)
#endif


//
// Routine Description:
//
//  Installable driver initialization entry point.
//  This entry point is called directly by the I/O system.
//
// Arguments:
//
//  DriverObjectPtr - pointer to the driver object
//
//  RegistryPathPtr - pointer to a Unicode string representing the path,
//      to driver-specific key in the registry.
//
// Return Value:
//
//    STATUS_SUCCESS, or appropriate error code
//
_Use_decl_annotations_
NTSTATUS
DriverEntry (
    DRIVER_OBJECT* DriverObjectPtr,
    UNICODE_STRING* RegistryPathPtr
    )
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize tracing
    //
    RECORDER_LOG defLogHandle = NULL;
    {
        WPP_INIT_TRACING(DriverObjectPtr, RegistryPathPtr);

        RECORDER_CONFIGURE_PARAMS recorderConfigureParams;
        RECORDER_CONFIGURE_PARAMS_INIT(&recorderConfigureParams);

        WppRecorderConfigure(&recorderConfigureParams);
        #if DBG
            WPP_RECORDER_LEVEL_FILTER(ECSPI_TRACING_VERBOSE) = TRUE;
        #endif // DBG
        
        defLogHandle = WppRecorderLogGetDefault();

    } // Tracing

    //
    // Create the ECSPI WDF driver object
    //
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, ECSPI_DRIVER_EXTENSION);

        WDF_DRIVER_CONFIG wdfDriverConfig;
        WDF_DRIVER_CONFIG_INIT(&wdfDriverConfig, ECSPIEvtDeviceAdd);
        wdfDriverConfig.EvtDriverUnload = ECSPIEvtDriverUnload;

        //
        // Specify a pool tag for allocations WDF makes on our behalf
        //
        wdfDriverConfig.DriverPoolTag = ULONG(ECSPI_ALLOC_TAG::ECSPI_ALLOC_TAG_WDF);

        status = WdfDriverCreate(
            DriverObjectPtr,
            RegistryPathPtr,
            &attributes,
            &wdfDriverConfig,
            WDF_NO_HANDLE
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                defLogHandle,
                "WdfDriverCreate failed, status = %!STATUS!", 
                status
                );
            goto done;
        }
        ECSPIDriverGetDriverExtension()->DriverLogHandle = defLogHandle;

    } // WDF driver 

    //
    // Read driver configuration parameters from registry.
    //
    status = ECSPIpDriverReadConfig();
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            defLogHandle,
            "ECSPIpDriverReadConfig failed, status = %!STATUS!", 
            status
            );
        goto done;
    }

done:

    //
    // Cleanup...
    //
    if (!NT_SUCCESS(status)) {

        WPP_CLEANUP(DriverObjectPtr);
    }

    return status;
}


//
// Routine Description:
//
//  ECSPIEvtDriverUnload is called by the framework just before the driver 
//  is unloaded. We use it to call WPP_CLEANUP.
//
// Arguments:
//
//  WdfDriver - Handle to a framework driver object created in DriverEntry
//
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIEvtDriverUnload (
    WDFDRIVER WdfDriver
    )
{
    PAGED_CODE();

    WPP_CLEANUP(WdfDriverWdmGetDriverObject(WdfDriver));
}


//
// Routine Description:
//
//  ECSPIEvtDeviceAdd is called by the framework in response to AddDevice
//  call from the PnP manager.
//  The function creates and initializes a new ECSPI WDF device, and registers it
//  with the SpbCx framework.
//
// Arguments:
//
//  WdfDriver - The ECSPI WDF driver object.
//
//  DeviceInitPtr - Pointer to a framework-allocated WDFDEVICE_INIT structure.
//
// Return Value:
//
//   Device creation and SpbCx registration status.
//
_Use_decl_annotations_
NTSTATUS
ECSPIEvtDeviceAdd (
    WDFDRIVER /*WdfDriver*/,
    PWDFDEVICE_INIT DeviceInitPtr
    )
{
    PAGED_CODE();

    NTSTATUS status;

    //
    // Configure DeviceInit structure
    //
    status = SpbDeviceInitConfig(DeviceInitPtr);
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            DRIVER_LOG_HANDLE,
            "SpbDeviceInitConfig() failed. "
            "DeviceInitPtr = %p, status = %!STATUS!",
            DeviceInitPtr,
            status
            );
        return status;
    }

    //
    // Setup PNP/Power callbacks.
    //
    {
        WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

        pnpCallbacks.EvtDevicePrepareHardware = 
            ECSPIEvtDevicePrepareHardware;
        pnpCallbacks.EvtDeviceReleaseHardware =
            ECSPIEvtDeviceReleaseHardware;

        WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInitPtr, &pnpCallbacks);

    } // // Setup PNP/Power callbacks

    //
    // Create the device.
    //
    WDFDEVICE wdfDevice;
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes, 
            ECSPI_DEVICE_EXTENSION
            );

        status = WdfDeviceCreate(&DeviceInitPtr, &attributes, &wdfDevice);
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                DRIVER_LOG_HANDLE,
                "Failed to create WDFDEVICE. "
                "DeviceInitPtr = %p, status = %!STATUS!",
                DeviceInitPtr,
                status
                );
            return status;
        }

        //
        // For DMA...
        //
        WdfDeviceSetAlignmentRequirement(wdfDevice, FILE_LONG_ALIGNMENT);

    } // Create the device.

    //
    // Register ECSPI with SpbCx as an SPB controller.
    //
    {
        SPB_CONTROLLER_CONFIG spbControllerConfig;
        SPB_CONTROLLER_CONFIG_INIT(&spbControllerConfig);

        spbControllerConfig.ControllerDispatchType = WdfIoQueueDispatchSequential;

        spbControllerConfig.EvtSpbTargetConnect = ECSPIEvtSpbTargetConnect;
        spbControllerConfig.EvtSpbTargetDisconnect = ECSPIEvtSpbTargetDisconnect;
        spbControllerConfig.EvtSpbControllerLock = ECSPIEvtSpbControllerLock;
        spbControllerConfig.EvtSpbControllerUnlock = ECSPIEvtSpbControllerUnlock;
        spbControllerConfig.EvtSpbIoRead = ECSPIEvtSpbIoRead;
        spbControllerConfig.EvtSpbIoWrite = ECSPIEvtSpbIoWrite;
        spbControllerConfig.EvtSpbIoSequence = ECSPIEvtSpbIoSequence;

        status = SpbDeviceInitialize(wdfDevice, &spbControllerConfig);
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                DRIVER_LOG_HANDLE,
                "SpbDeviceInitialize failed, "
                "wdfDevice = %p, status = %!STATUS!",
                wdfDevice,
                status
                );
            return status;
        }

        //
        // Register for other to support custom IOctl(s), required 
        // for handling FULL_DUPLEX requests.
        //
        SpbControllerSetIoOtherCallback(
            wdfDevice,
            ECSPIEvtSpbIoOther,
            ECSPIEvtIoInCallerContext
            );

    } // Register ECSPI with SpbCx as an SPB controller.

    //
    // Set target object attributes.
    // Create our SPB target context
    //
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            ECSPI_TARGET_CONTEXT
            );

        SpbControllerSetTargetAttributes(wdfDevice, &attributes);

    } // Create our SPB target context

    //
    // Create an interrupt object
    //
    WDFINTERRUPT wdfInterrupt;
    {
        WDF_INTERRUPT_CONFIG interruptConfig;
        WDF_INTERRUPT_CONFIG_INIT(
            &interruptConfig,
            ECSPIEvtInterruptIsr,
            ECSPIEvtInterruptDpc
            );

        status = WdfInterruptCreate(
            wdfDevice,
            &interruptConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfInterrupt
            );
        if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                DRIVER_LOG_HANDLE,
                "Failed to create interrupt object. "
                "wdfDevice = %p, status = %!STATUS!",
                wdfDevice,
                status
                );
            return status;
        }

    } // Create an interrupt object

    //
    // Initialize the device extension
    //
    {
        ECSPI_DEVICE_EXTENSION* devExtPtr = ECSPIDeviceGetExtension(wdfDevice);
        devExtPtr->WdfDevice = wdfDevice;
        devExtPtr->WdfSpiInterrupt = wdfInterrupt;
        KeInitializeSpinLock(&devExtPtr->DeviceLock);

    } // Initialize the device extension

    return STATUS_SUCCESS;
}

//
// Routine Description:
//
//  ECSPIDriverGetDriverExtension return the driver extension.
//
// Arguments:
//
// Return Value:
//
//  The driver extension address.
//
ECSPI_DRIVER_EXTENSION*
ECSPIDriverGetDriverExtension ()
{
    static ECSPI_DRIVER_EXTENSION* drvExtPtr = nullptr;

    if (drvExtPtr == nullptr) {

        drvExtPtr = ECSPIDriverGetExtension(WdfGetDriver());
    }
    ECSPI_ASSERT(
        WppRecorderLogGetDefault(),
        drvExtPtr != nullptr
        );

    return drvExtPtr;
}


// 
// ECSPIdriver private methods.
// ----------------------------
//


//
// Routine Description:
//
//  PL011pDriverReadConfig is called by DriverEntry to
//  read driver configuration parameters from registry.
//  These configuration parameters are read from registry since there
//  is no standard way to get them through UEFI.
//
//  The configuration parameters reside at:
//  HKLM\System\CurrentControlSet\Services\imxecspi\Parameters
//
// Arguments:
//
// Return Value:
//
//  Registry open/read status code.
//
_Use_decl_annotations_
NTSTATUS
ECSPIpDriverReadConfig ()
{
    PAGED_CODE();

    //
    // Destination base address is the driver extension.
    //
    ECSPI_DRIVER_EXTENSION* drvExtPtr = ECSPIDriverGetDriverExtension();

    //
    // The list of registry value descriptors
    //
    struct ECSPI_REG_VALUE_DESCRIPTOR
    {
        // Value name
        PCWSTR ValueNameWsz;

        // Address of parameter
        _Field_size_bytes_(ParamSize)
        VOID* ParamAddrPtr;

        // Size of parameters
        size_t ParamSize;

        // Default value to use
        ULONG DefaultValue;

    } regValues[] = {

        {
            REGSTR_VAL_REFERENCE_CLOCK_HZ,
            &drvExtPtr->ReferenceClockHz,
            FIELD_SIZE(ECSPI_DRIVER_EXTENSION, ReferenceClockHz),
            ECSPI_DEFAULT_CLOCK_FREQ,

        },
        {
            REGSTR_VAL_REFERENCE_MAX_SPEED_HZ,
            &drvExtPtr->MaxConnectionSpeedHz,
            FIELD_SIZE(ECSPI_DRIVER_EXTENSION, MaxConnectionSpeedHz),
            0,
        },
        {
            REGSTR_VAL_FLAGS,
            &drvExtPtr->Flags,
            FIELD_SIZE(ECSPI_DRIVER_EXTENSION, Flags),
            0,
        },

    }; // regValues

    NTSTATUS status;
    WDFKEY driverRegkey = NULL;

    status = WdfDriverOpenParametersRegistryKey(
        WdfGetDriver(),
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &driverRegkey
        );
    if (!NT_SUCCESS(status)) {

        ECSPI_LOG_ERROR(
            DRIVER_LOG_HANDLE,
            "WdfDriverOpenParametersRegistryKey failed, status = %!STATUS!",
            status
            );
        goto done;
    }

    for (ULONG descInx = 0; descInx < ULONG(ARRAYSIZE(regValues)); ++descInx) {

        ECSPI_REG_VALUE_DESCRIPTOR* regValueDescPtr = &regValues[descInx];

        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, regValueDescPtr->ValueNameWsz);

        ULONG value;
        status = WdfRegistryQueryULong(driverRegkey, &valueName, &value);
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            //
            // Value not found, not an error, use default value.
            //
            value = regValueDescPtr->DefaultValue;
            status = STATUS_SUCCESS;

        } else if (!NT_SUCCESS(status)) {

            ECSPI_LOG_ERROR(
                DRIVER_LOG_HANDLE,
                "WdfRegistryQueryULong failed, status = %!STATUS!",
                status
                );
            goto done;
        }

        RtlCopyMemory(
            regValueDescPtr->ParamAddrPtr,
            &value,
            min(sizeof(value), regValueDescPtr->ParamSize)
            );

    } // for (descriptors)

    status = STATUS_SUCCESS;

done:

    if (driverRegkey != NULL) {

        WdfRegistryClose(driverRegkey);
    }

    return status;
}

#undef _ECSPI_DRIVER_CPP_
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   driver.cpp
//
// Abstract:
//
//   This module contains methods implementation for the driver initialization.
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
#include "driver.tmh"

IMXPWM_PAGED_SEGMENT_BEGIN; //==================================================

VOID
ImxPwmEvtDriverUnload (
    WDFDRIVER WdfDriver
    )
{
    PAGED_CODE();

    DRIVER_OBJECT* driverObjectPtr = WdfDriverWdmGetDriverObject(WdfDriver);
    WPP_CLEANUP(driverObjectPtr);
}

IMXPWM_PAGED_SEGMENT_END; //===================================================
IMXPWM_INIT_SEGMENT_BEGIN; //==================================================

_Use_decl_annotations_
NTSTATUS
DriverEntry (
    DRIVER_OBJECT* DriverObjectPtr,
    UNICODE_STRING* RegistryPathPtr
    )
{
    PAGED_CODE();

    //
    // Initialize logging
    //
    {
        WPP_INIT_TRACING(DriverObjectPtr, RegistryPathPtr);
        RECORDER_CONFIGURE_PARAMS recorderConfigureParams;
        RECORDER_CONFIGURE_PARAMS_INIT(&recorderConfigureParams);
        WppRecorderConfigure(&recorderConfigureParams);
#if DBG
        WPP_RECORDER_LEVEL_FILTER(IMXPWM_TRACING_DEFAULT) = TRUE;
#endif // DBG
    }

    WDF_DRIVER_CONFIG wdfDriverConfig;
    WDF_DRIVER_CONFIG_INIT(&wdfDriverConfig, ImxPwmEvtDeviceAdd);
    wdfDriverConfig.DriverPoolTag = IMXPWM_POOL_TAG;
    wdfDriverConfig.EvtDriverUnload = ImxPwmEvtDriverUnload;

    WDFDRIVER wdfDriver;
    NTSTATUS status = WdfDriverCreate(
            DriverObjectPtr,
            RegistryPathPtr,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfDriverConfig,
            &wdfDriver);

    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "Failed to create WDF driver object. (status = %!STATUS!, "
            "DriverObjectPtr = %p, RegistryPathPtr = %p)",
            status,
            DriverObjectPtr,
            RegistryPathPtr);

        return status;
    }

    return STATUS_SUCCESS;
}

IMXPWM_INIT_SEGMENT_END; //====================================================
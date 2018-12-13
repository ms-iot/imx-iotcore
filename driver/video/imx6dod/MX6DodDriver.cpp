// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "precomp.h"

#include "MX6DodLogging.h"
#include "MX6DodDriver.tmh"

#include "MX6DodCommon.h"
#include "MX6DodDevice.h"
#include "MX6DodDriver.h"

MX6DOD_NONPAGED_SEGMENT_BEGIN; //==============================================

//
// Placement new and delete operators
//

_Use_decl_annotations_
void* operator new ( size_t, void* Ptr ) throw ()
{
    return Ptr;
}

void operator delete ( void*, void* ) throw ()
{}

_Use_decl_annotations_
void* operator new[] ( size_t, void* Ptr ) throw ()
{
    return Ptr;
}

void operator delete[] ( void*, void* ) throw ()
{}

MX6DOD_NONPAGED_SEGMENT_END; //================================================
MX6DOD_PAGED_SEGMENT_BEGIN; //=================================================

namespace { // static

    // This must be stored as a global variable since DxgkDdiUnload does not
    // supply a driver object pointer or context pointer.
    DRIVER_OBJECT* mx6GlobalDriverObjectPtr;

} // namespace "static"

_Use_decl_annotations_
void MX6DodDdiUnload ()
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(mx6GlobalDriverObjectPtr);
    WPP_CLEANUP(mx6GlobalDriverObjectPtr);
    mx6GlobalDriverObjectPtr = nullptr;
}

MX6DOD_PAGED_SEGMENT_END; //=====================================================
MX6DOD_INIT_SEGMENT_BEGIN; //====================================================

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
        WPP_RECORDER_LEVEL_FILTER(MX6DOD_TRACING_DEFAULT) = TRUE;
#endif // DBG
    }

    MX6DOD_LOG_INFORMATION(
        "(DriverObjectPtr = 0x%p, RegistryPathPtr = 0x%p)",
        DriverObjectPtr,
        RegistryPathPtr);
    
    // Initialize DDI function pointers and dxgkrnl
    auto dodInit = KMDDOD_INITIALIZATION_DATA();
    dodInit.Version = DXGKDDI_INTERFACE_VERSION;

    dodInit.DxgkDdiUnload = MX6DodDdiUnload;

    dodInit.DxgkDdiAddDevice = MX6DOD_DEVICE::DdiAddDevice;
    dodInit.DxgkDdiStartDevice = MX6DOD_DEVICE::DdiStartDevice;
    dodInit.DxgkDdiStopDevice = MX6DOD_DEVICE::DdiStopDevice;
    dodInit.DxgkDdiResetDevice = MX6DOD_DEVICE::DdiResetDevice;
    dodInit.DxgkDdiRemoveDevice = MX6DOD_DEVICE::DdiRemoveDevice;
    dodInit.DxgkDdiDispatchIoRequest = MX6DOD_DEVICE::DdiDispatchIoRequest;

    dodInit.DxgkDdiQueryChildRelations = MX6DOD_DEVICE::DdiQueryChildRelations;
    dodInit.DxgkDdiQueryChildStatus = MX6DOD_DEVICE::DdiQueryChildStatus;
    dodInit.DxgkDdiQueryDeviceDescriptor = MX6DOD_DEVICE::DdiQueryDeviceDescriptor;
    dodInit.DxgkDdiSetPowerState = MX6DOD_DEVICE::DdiSetPowerState;

    dodInit.DxgkDdiQueryAdapterInfo = MX6DOD_DEVICE::DdiQueryAdapterInfo;
    dodInit.DxgkDdiSetPointerPosition = MX6DOD_DEVICE::DdiSetPointerPosition;
    dodInit.DxgkDdiSetPointerShape = MX6DOD_DEVICE::DdiSetPointerShape;

    dodInit.DxgkDdiIsSupportedVidPn = MX6DOD_DEVICE::DdiIsSupportedVidPn;
    dodInit.DxgkDdiRecommendFunctionalVidPn = MX6DOD_DEVICE::DdiRecommendFunctionalVidPn;
    dodInit.DxgkDdiEnumVidPnCofuncModality = MX6DOD_DEVICE::DdiEnumVidPnCofuncModality;
    dodInit.DxgkDdiSetVidPnSourceVisibility = MX6DOD_DEVICE::DdiSetVidPnSourceVisibility;
    dodInit.DxgkDdiCommitVidPn = MX6DOD_DEVICE::DdiCommitVidPn;
    dodInit.DxgkDdiUpdateActiveVidPnPresentPath = MX6DOD_DEVICE::DdiUpdateActiveVidPnPresentPath;

    dodInit.DxgkDdiRecommendMonitorModes = MX6DOD_DEVICE::DdiRecommendMonitorModes;
    dodInit.DxgkDdiQueryVidPnHWCapability = MX6DOD_DEVICE::DdiQueryVidPnHWCapability;
    dodInit.DxgkDdiPresentDisplayOnly = MX6DOD_DEVICE::DdiPresentDisplayOnly;
    dodInit.DxgkDdiSetPowerComponentFState = MX6DOD_DEVICE::DdiSetPowerComponentFState;
    dodInit.DxgkDdiStopDeviceAndReleasePostDisplayOwnership = MX6DOD_DEVICE::DdiStopDeviceAndReleasePostDisplayOwnership;
    dodInit.DxgkDdiSystemDisplayEnable = MX6DOD_DEVICE::DdiSystemDisplayEnable;
    dodInit.DxgkDdiSystemDisplayWrite = MX6DOD_DEVICE::DdiSystemDisplayWrite;
    dodInit.DxgkDdiPowerRuntimeControlRequest = MX6DOD_DEVICE::DdiPowerRuntimeControlRequest;

    NTSTATUS status = DxgkInitializeDisplayOnlyDriver(
            DriverObjectPtr,
            RegistryPathPtr,
            &dodInit);
            
    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DxgkInitializeDisplayOnlyDriver failed. (status = %!STATUS!, DriverObjectPtr = %p)",
            status,
            DriverObjectPtr);
            
        return status;
    }

    NT_ASSERT(mx6GlobalDriverObjectPtr == nullptr);
    mx6GlobalDriverObjectPtr = DriverObjectPtr;
    NT_ASSERT(status == STATUS_SUCCESS);
    return status;
} // DriverEntry (...)

MX6DOD_INIT_SEGMENT_END; //====================================================


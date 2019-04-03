/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    OpteeTrEE.c

Abstract:

    This file demonstrates a simple plugin for the TREE class extension driver.

Environment:

    Kernel mode

*/

#include <ntddk.h>
#include <wdf.h>
#include <wdmguid.h>
#include <initguid.h>
#include <ntstrsafe.h>
#include <TrustedRuntimeClx.h>
#include "inc\efidef.h"
#include <TrEEVariableService.h>
#include <TrEETPMService.h>
#include "OpteeTrEE.h"
#include "OpteeTrEEService.h"
#include "trace.h"

#ifdef WPP_TRACING
#include "OpteeTree.tmh"
#endif

KSPIN_LOCK ConcurrencyLock = 0;
volatile LONG CurrentConcurrency;
volatile LONG ConcurrencyHighMark;

//
// Driver entry point
//

DRIVER_INITIALIZE DriverEntry;

//
// Device callbacks
//

EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD OpteeTreeEvtAddDevice;

//
// Class Extension callbacks
//

EVT_TR_CREATE_SECURE_DEVICE_CONTEXT OpteeTreeCreateSecureDeviceContext;
EVT_TR_DESTROY_SECURE_DEVICE_CONTEXT OpteeTreeDestroySecureDeviceContext;
EVT_TR_PREPARE_HARDWARE_SECURE_ENVIRONMENT OpteeTreePrepareHardwareSecureEnvironment;
EVT_TR_RELEASE_HARDWARE_SECURE_ENVIRONMENT OpteeTreeReleaseHardwareSecureEnvironment;
EVT_TR_CONNECT_SECURE_ENVIRONMENT OpteeTreeConnectSecureEnvironment;
EVT_TR_DISCONNECT_SECURE_ENVIRONMENT OpteeTreeDisconnectSecureEnvironment;
EVT_TR_ENUMERATE_SECURE_SERVICES OpteeTreeEnumerateSecureServices;
EVT_TR_PROCESS_OTHER_DEVICE_IO OpteeTreeProcessOtherDeviceIo;
EVT_TR_CREATE_SECURE_SERVICE_CONTEXT OpteeTreeCreateSecureServiceContext;
EVT_TR_QUERY_SERVICE_CALLBACKS OpteeTreeQueryServiceCallbacks;

TR_SECURE_DEVICE_CALLBACKS OPTEE_TREE_CALLBACKS = {
    TR_DEVICE_NO_SERIALIZATION |
    TR_DEVICE_STACK_RESERVE_8K,

    &OpteeTreeCreateSecureDeviceContext,
    &OpteeTreeDestroySecureDeviceContext,

    &OpteeTreePrepareHardwareSecureEnvironment,
    &OpteeTreeReleaseHardwareSecureEnvironment,

    &OpteeTreeConnectSecureEnvironment,
    &OpteeTreeDisconnectSecureEnvironment,

    &OpteeTreeEnumerateSecureServices,
    &OpteeTreeProcessOtherDeviceIo,

    &OpteeTreeQueryServiceCallbacks
};

#ifndef WPP_TRACING
ULONG DefaultDebugLevel = DRVR_LVL_ERR;
ULONG DefaultDebugFlags = 0xFFFFFFFF;
#endif // !WPP_TRACING

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, OpteeTreeEvtAddDevice)

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

    Routine Description:

        This is the initialization routine for the device driver.
        This routine creates the device object for the TrEE device and performs
        all other driver initialization.

    Arguments:

        DriverObject - Pointer to driver object created by the system.

    Return Value:

        NTSTATUS code.

--*/
{

    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status;
    WDFDRIVER WdfDriver;


#ifdef WPP_TRACING
    {
        WPP_INIT_TRACING(DriverObject, RegistryPath);

        RECORDER_CONFIGURE_PARAMS RecorderConfigureParams;
        RECORDER_CONFIGURE_PARAMS_INIT(&RecorderConfigureParams);

        WppRecorderConfigure(&RecorderConfigureParams);
#if DBG
        WPP_RECORDER_LEVEL_FILTER(DRVR_LVL_DBG) = TRUE;
#endif // DBG
    } 
#endif // WPP_TRACING

    //
    // Create the WDF driver object
    //
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, OpteeTreeEvtAddDevice);
    DriverConfig.EvtDriverUnload = DriverUnload;

    Status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &DriverConfig,
        &WdfDriver);

    if (!NT_SUCCESS(Status)) {
#ifdef  WPP_TRACING
        WPP_CLEANUP(DriverObject);
#endif  // WPP_TRACING

        return Status;
    }

    return STATUS_SUCCESS;
}

VOID
DriverUnload(
    _In_ WDFDRIVER Driver
    )
{
#ifdef WPP_TRACING
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
#else
    UNREFERENCED_PARAMETER(Driver);
#endif
    
}

_Function_class_(EVT_WDF_DRIVER_DEVICE_ADD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreeEvtAddDevice(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )

/*++

    Routine Description:

        This routine is called when the driver is being attached to a specific
        device.

    Arguments:

        Driver - Supplies a handle to the framework driver object.

        DeviceInit - Supplies a pointer to the device initialization parameters.

    Return Value:

        NTSTATUS code.

--*/

{

    WDFDEVICE Device;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    Status = TrSecureDeviceHandoffMasterDeviceControl(
                DeviceInit,
                &OPTEE_TREE_CALLBACKS,
                &Device);

    TrSecureDeviceLogMessage(Device,
                             STATUS_SEVERITY_INFORMATIONAL,
                             "Master device discovered\n");

    return Status;
}

_Function_class_(EVT_TR_CREATE_SECURE_DEVICE_CONTEXT)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreeCreateSecureDeviceContext(
    _In_ WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment is first started.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to store any context information
                        required for future calls.

    Return Value:

        NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES CollectionAttributes;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    POPTEE_TREE_DEVICE_CONTEXT MasterContext;
    NTSTATUS Status;
    DECLARE_CONST_UNICODE_STRING(SymbolicLink, L"\\DosDevices\\OpteeTrEEDriver");

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            OPTEE_TREE_DEVICE_CONTEXT);

    Status = WdfObjectAllocateContext(MasterDevice, &ContextAttributes, &MasterContext);
    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeCreateSecureDeviceContextEnd;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&CollectionAttributes);
    CollectionAttributes.ParentObject = MasterDevice;
    Status = WdfCollectionCreate(&CollectionAttributes,
                                    &MasterContext->ServiceCollection);

    MasterContext->MasterDevice = MasterDevice;

    Status = WdfDeviceCreateSymbolicLink(MasterDevice, &SymbolicLink);
    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeCreateSecureDeviceContextEnd;
    }

OpteeTreeCreateSecureDeviceContextEnd:
    return Status;
}

_Function_class_(EVT_TR_DESTROY_SECURE_DEVICE_CONTEXT)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreeDestroySecureDeviceContext(
    _In_ WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment is no longer used.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

    Return Value:

        NTSTATUS code.

--*/

{

    POPTEE_TREE_DEVICE_CONTEXT MasterContext;

    MasterContext = TreeGetDeviceContext(MasterDevice);

    /*
    * No member needs cleanup here. ServiceCollection will be automatically
    * reclaimed by the framework, since the parent object is going away.
    */

    return STATUS_SUCCESS;
}

_Function_class_(EVT_TR_PREPARE_HARDWARE_SECURE_ENVIRONMENT)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreePrepareHardwareSecureEnvironment(
    _In_ WDFDEVICE MasterDevice,
    _In_ WDFCMRESLIST RawResources,
    _In_ WDFCMRESLIST TranslatedResources
    )

/*++

    Routine Description:

        This routine is called to handle any resources used by a secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        RawResources - Supplies a pointer to the raw resources.

        TranslatedResources - Supplies a pointer to the translated resources.

    Return Value:

        NTSTATUS code.

--*/

{

    ULONG NumMemResourcesFound;
    ULONG NumResources;
    POPTEE_TREE_DEVICE_CONTEXT MasterContext;
    ULONG Resource;

    UNREFERENCED_PARAMETER(RawResources);

    MasterContext = TreeGetDeviceContext(MasterDevice);
    NumResources = WdfCmResourceListGetCount(TranslatedResources);
    NumMemResourcesFound = 0;

    for (Resource = 0; Resource < NumResources; ++Resource) {

        const CM_PARTIAL_RESOURCE_DESCRIPTOR* ResDescPtr =
            WdfCmResourceListGetDescriptor(TranslatedResources, Resource);

        switch (ResDescPtr->Type) {
        case CmResourceTypeMemory:
            ++NumMemResourcesFound;
            if (NumMemResourcesFound > 1) {

                TrSecureDeviceLogMessage(MasterDevice,
                    STATUS_SEVERITY_WARNING,
                    "Unexpected memory resources %d\n", NumMemResourcesFound);

                break;
            }
            
            MasterContext->SharedMemoryBaseAddress = ResDescPtr->u.Memory.Start;
            MasterContext->SharedMemoryLength = ResDescPtr->u.Memory.Length;
            break;

        default:
            break;

        } // switch

    } // for (resource list)

    if (NumMemResourcesFound < 1) {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return STATUS_SUCCESS;
}

_Function_class_(EVT_TR_RELEASE_HARDWARE_SECURE_ENVIRONMENT)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreeReleaseHardwareSecureEnvironment(
    _In_ WDFDEVICE MasterDevice,
    _In_ WDFCMRESLIST TranslatedResources
    )

/*++

    Routine Description:

        This routine is called to handle the disposal of any resources used by
        a secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        TranslatedResources - Supplies a pointer to the translated resources.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(MasterDevice);
    UNREFERENCED_PARAMETER(TranslatedResources);

    return STATUS_SUCCESS;
}

_Function_class_(EVT_TR_CONNECT_SECURE_ENVIRONMENT)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreeConnectSecureEnvironment(
    _In_ WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment should be prepared
        for use either the first time or after a possible power state change.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(MasterDevice);

    return STATUS_SUCCESS;
}

_Function_class_(EVT_TR_DISCONNECT_SECURE_ENVIRONMENT)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreeDisconnectSecureEnvironment(
    _In_ WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment should be prepared
        for a possible power state change.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(MasterDevice);

    return STATUS_SUCCESS;
}

_Function_class_(EVT_TR_ENUMERATE_SECURE_SERVICES)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OpteeTreeEnumerateSecureServices(
    _In_ WDFDEVICE MasterDevice,
    _In_ ULONG Index,
    _Inout_updates_bytes_opt_(*DescriptionSize) PUCHAR SecureServiceDescription,
    _Inout_ ULONG *DescriptionSize
    )

/*++

    Routine Description:

        This routine is called to enumerate the supported secure services
        provided by this secure device. The zero-based Index parameter is used
        to determine which secure service information is being requested for.
        If Index is larger than the number of available services then
        STATUS_NO_MORE_ENTRIES is returned to indicate the end of the list has
        been reached.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        Index - Supplies the zero-based index for the secure service whose
                description is being requested.

        SecureServiceDescription - Supplies a pointer to a buffer to hold the
                                   secure service description. The description
                                   is of type TR_SECURE_SERVICE.

        DescriptionSize - Supplies a pointer to the size in bytes of
                          SecureServiceDescription on input, and holds the
                          number of bytes required on output.

    Return Value:

        NTSTATUS code.

--*/

{

    PGUID BusGuid;
    WDFMEMORY BusGuidMemory;
    size_t BusGuidSize;
    POPTEE_TREE_DEVICE_CONTEXT MasterContext;
    WDFMEMORY ServiceMemory;
    PTR_SECURE_SERVICE NextService;
    NTSTATUS Status;

    BusGuidMemory = NULL;
    MasterContext = TreeGetDeviceContext(MasterDevice);
    if (MasterContext->Enumerated == FALSE) {

        //
        // Only attempt enumeration once.
        //

        MasterContext->Enumerated = TRUE;
        Status = WdfDeviceAllocAndQueryProperty(MasterDevice,
                                                DevicePropertyBusTypeGuid,
                                                NonPagedPoolNx,
                                                WDF_NO_OBJECT_ATTRIBUTES,
                                                &BusGuidMemory);

        if (NT_SUCCESS(Status)) {
            BusGuid = (LPGUID)WdfMemoryGetBuffer(BusGuidMemory, &BusGuidSize);
            if (BusGuidSize == sizeof(*BusGuid) &&
                memcmp(BusGuid, &GUID_BUS_TYPE_ACPI, sizeof(GUID)) == 0) {

                //
                // This is an ACPI enumerated device -- look for services
                // described in the ACPI firmware.
                //

                Status = TreeEnumSecureServicesFromACPI(
                             MasterDevice,
                             MasterContext->ServiceCollection);
            }
        }
    
        if (!NT_SUCCESS(Status)) {

            //
            // Fallback to looking for services described in the registry.
            //

            Status = TreeEnumSecureServicesFromRegistry(
                         MasterDevice,
                         MasterContext->ServiceCollection);
        }

        if (!NT_SUCCESS(Status)) {
            goto OpteeTreeEnumerateSecureServicesEnd;
        }
    }

    if (Index >= WdfCollectionGetCount(MasterContext->ServiceCollection)) {
        Status = STATUS_NO_MORE_ENTRIES;
        goto OpteeTreeEnumerateSecureServicesEnd;
    }

    ServiceMemory = (WDFMEMORY)WdfCollectionGetItem(
                                   MasterContext->ServiceCollection,
                                   Index);

    NextService = WdfMemoryGetBuffer(ServiceMemory, NULL);
    if (*DescriptionSize < NextService->DescriptionSize) {
        Status = STATUS_BUFFER_TOO_SMALL;

    } else {

        NT_ASSERT(SecureServiceDescription != NULL);

        RtlCopyMemory(SecureServiceDescription,
                      NextService,
                      NextService->DescriptionSize);

        Status = STATUS_SUCCESS;
    }

    *DescriptionSize = NextService->DescriptionSize;

OpteeTreeEnumerateSecureServicesEnd:
    if (BusGuidMemory != NULL) {
        WdfObjectDelete(BusGuidMemory);
    }

    return Status;;
}

_Function_class_(EVT_TR_PROCESS_OTHER_DEVICE_IO)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
OpteeTreeProcessOtherDeviceIo(
    _In_ WDFDEVICE MasterDevice,
    _In_ WDFREQUEST Request
    )

/*++

    Routine Description:

        This routine is called when an unrecognized IO request is made to the
        device. This can be used to process private calls directly to the
        secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        Request - Supplies a pointer to the WDF request object.

    Return Value:

        NTSTATUS code.

--*/

{

    PWCHAR Buffer;
    size_t BufferSize;
    ULONG BytesWritten;
    WDF_REQUEST_PARAMETERS Parameters;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(MasterDevice);

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    BytesWritten = 0;
    switch (Parameters.Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SAMPLE_HELLOWORLD:
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                sizeof(HELLOWORLD_STRING),
                                                (PVOID*)&Buffer,
                                                NULL);

        if (!NT_SUCCESS(Status)) {
            goto OpteeTreeProcessOtherDeviceIoEnd;
        }

        RtlCopyMemory(Buffer, HELLOWORLD_STRING, sizeof(HELLOWORLD_STRING));
        BytesWritten = sizeof(HELLOWORLD_STRING);
        break;

    case IOCTL_SAMPLE_OutputDebugMessage:
        Status = WdfRequestRetrieveInputBuffer(Request,
                                               0,
                                               (PVOID*)&Buffer,
                                               &BufferSize);

        if (!NT_SUCCESS(Status)) {
            goto OpteeTreeProcessOtherDeviceIoEnd;
        }

        //
        // Must be NULL-terminated
        //
        if (Buffer[BufferSize / sizeof(WCHAR) - 1] != L'\0') {
            Status = STATUS_INVALID_PARAMETER;
            goto OpteeTreeProcessOtherDeviceIoEnd;
        }

        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_ERROR_LEVEL,
                   "OpteeTrEEDriver: %ws\n",
                   (PWSTR)Buffer);

        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

OpteeTreeProcessOtherDeviceIoEnd:
    WdfRequestCompleteWithInformation(Request, Status, BytesWritten);
}

_Function_class_(EVT_TR_QUERY_SERVICE_CALLBACKS)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
PTR_SECURE_SERVICE_CALLBACKS
OpteeTreeQueryServiceCallbacks(
    _In_ WDFDEVICE MasterDevice,
    _In_ LPGUID ServiceGuid
    )

/*++

    Routine Description:

        This routine is called when an unrecognized IO request is made to the
        device. This can be used to process private calls directly to the
        secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        Request - Supplies a pointer to the WDF request object.

    Return Value:

        NTSTATUS code.

--*/

{

    PTR_SECURE_SERVICE_CALLBACKS ServiceCallbacks;

    UNREFERENCED_PARAMETER(MasterDevice);

    if (IsEqualGUID(ServiceGuid, &GUID_EFI_VARIABLE_SERVICE)) {
    
        ServiceCallbacks = &VARIABLE_SERVICE_CALLBACKS;

    } else if (IsEqualGUID(ServiceGuid, &GUID_TREE_TPM_SERVICE)) {

        ServiceCallbacks = &FTPM_SERVICE_CALLBACKS;

    } else {

        ServiceCallbacks = &GEN_SERVICE_CALLBACKS;
    }

    return ServiceCallbacks;
}

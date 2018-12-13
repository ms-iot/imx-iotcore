/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <ntddk.h>
#include <wdf.h>
#include <wdmguid.h>
#include <initguid.h>
#include <TrustedRuntimeClx.h>
#include "OpteeTrEE.h"
#include "OpteeTrEEService.h"
#include <TrEEGenService.h>
#include <treerpmbservice.h>
#include "trace.h"

typedef struct _TREE_CHECKSUM_SERVICE_CONTEXT {
    POPTEE_TREE_DEVICE_CONTEXT DeviceContext;
    WDFDEVICE ServiceDevice;

    ULONG SessionsOpened;
    ULONG SessionsClosed;
    ULONG64 BytesProcessed;
} TREE_CHECKSUM_SERVICE_CONTEXT, *PTREE_CHECKSUM_SERVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_CHECKSUM_SERVICE_CONTEXT);

typedef struct _TREE_CHECKSUM_SESSION_CONTEXT {
    PTREE_CHECKSUM_SERVICE_CONTEXT ServiceContext;

    WDFTIMER CompletionTimer;
    ULONG RunningSum;
} TREE_CHECKSUM_SESSION_CONTEXT, *PTREE_CHECKSUM_SESSION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_CHECKSUM_SESSION_CONTEXT);

typedef struct _TREE_CHECKSUM_PENDED_FINALIZE {
    PVOID RequestHandle;
    PVOID OutputBuffer;
    PTREE_CHECKSUM_SESSION_CONTEXT SessionContext;
} TREE_CHECKSUM_PENDED_FINALIZE, *PTREE_CHECKSUM_PENDED_FINALIZE;

WDF_DECLARE_CONTEXT_TYPE(TREE_CHECKSUM_PENDED_FINALIZE);

EVT_TR_CREATE_SECURE_SERVICE_CONTEXT ChecksumServiceCreateSecureServiceContext;
EVT_TR_DESTROY_SECURE_SERVICE_CONTEXT ChecksumServiceDestroySecureServiceContext;
EVT_TR_CONNECT_SECURE_SERVICE ChecksumServiceConnectSecureService;
EVT_TR_DISCONNECT_SECURE_SERVICE ChecksumServiceDisconnectSecureService;
EVT_TR_CREATE_SECURE_SERVICE_SESSION_CONTEXT ChecksumServiceCreateSessionContext;
EVT_TR_DESTROY_SECURE_SERVICE_SESSION_CONTEXT ChecksumServiceDestroySessionContext;
EVT_TR_PROCESS_SECURE_SERVICE_REQUEST ChecksumServiceProcessSecureServiceRequest;
EVT_TR_CANCEL_SECURE_SERVICE_REQUEST ChecksumServiceCancelSecureServiceRequest;
EVT_TR_PROCESS_OTHER_SECURE_SERVICE_IO ChecksumServiceProcessOtherSecureServiceIo;

TR_SECURE_SERVICE_CALLBACKS CHECKSUM_SERVICE_CALLBACKS = {
    0,

    &ChecksumServiceCreateSecureServiceContext,
    &ChecksumServiceDestroySecureServiceContext,

    &ChecksumServiceConnectSecureService,
    &ChecksumServiceDisconnectSecureService,

    &ChecksumServiceCreateSessionContext,
    &ChecksumServiceDestroySessionContext,

    &ChecksumServiceProcessSecureServiceRequest,
    &ChecksumServiceCancelSecureServiceRequest,
    &ChecksumServiceProcessOtherSecureServiceIo
};

typedef
NTSTATUS
TREE_CHECKSUM_SERVICE_REQUEST_HANDLER(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_CHECKSUM_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    );

TREE_CHECKSUM_SERVICE_REQUEST_HANDLER ChecksumServiceInitialize;
TREE_CHECKSUM_SERVICE_REQUEST_HANDLER ChecksumServiceUpdate;
TREE_CHECKSUM_SERVICE_REQUEST_HANDLER ChecksumServiceFinalize;
//TREE_CHECKSUM_SERVICE_REQUEST_HANDLER ChecksumServiceTestBuiltinRPMB;
TREE_CHECKSUM_SERVICE_REQUEST_HANDLER ChecksumServiceDelayedCompletion;

TREE_CHECKSUM_SERVICE_REQUEST_HANDLER *ChecksumServiceDispatch[] = {
    ChecksumServiceInitialize,
    ChecksumServiceUpdate,
    ChecksumServiceFinalize,
    NULL,
    NULL, //ChecksumServiceTestBuiltinRPMB,
    ChecksumServiceDelayedCompletion
};

EVT_WDF_TIMER ChecksumServicePendedFinalizeWorker;
EVT_WDF_TIMER ChecksumServiceDelayedCompletionWorker;

VOID
ChecksumServiceDelayedCompletionComplete(
    _In_ WDFTIMER Timer,
    _In_ NTSTATUS Status
    );

_Use_decl_annotations_
NTSTATUS
ChecksumServiceCreateSecureServiceContext(
    WDFDEVICE MasterDevice,
    LPCGUID ServiceGuid,
    WDFDEVICE ServiceDevice
    )
{
    PTREE_CHECKSUM_SERVICE_CONTEXT ServiceContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ServiceGuid);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            TREE_CHECKSUM_SERVICE_CONTEXT);

    Status = WdfObjectAllocateContext(ServiceDevice,
                                      &ContextAttributes,
                                      (PVOID*)&ServiceContext);

    if (!NT_SUCCESS(Status)) {
        goto ChecksumServiceCreateSecureServiceContextEnd;
    }

    RtlZeroMemory(ServiceContext, sizeof(TREE_CHECKSUM_SERVICE_CONTEXT));
    ServiceContext->DeviceContext = TreeGetDeviceContext(MasterDevice);
    ServiceContext->ServiceDevice = ServiceDevice;

    Status = STATUS_SUCCESS;

ChecksumServiceCreateSecureServiceContextEnd:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
ChecksumServiceDestroySecureServiceContext(
    WDFDEVICE ServiceDevice
    )

/*++

    Routine Description:

        This routine is called when a secure service is being destroyed.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

    Return Value:

        NTSTATUS code.

--*/

{
    PTREE_CHECKSUM_SERVICE_CONTEXT ServiceContext;

    ServiceContext = WdfObjectGet_TREE_CHECKSUM_SERVICE_CONTEXT(ServiceDevice);

    /*
    * No member needs cleanup here.
    */

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ChecksumServiceConnectSecureService(
    WDFDEVICE ServiceDevice
    )

/*++

    Routine Description:

        This routine is called when a secure service is being used for the
        first time or has returned from a power-state change.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

    Return Value:

        NTSTATUS code.

--*/

{

    TrSecureDeviceLogMessage(ServiceDevice,
                             STATUS_SEVERITY_INFORMATIONAL,
                             "ChecksumService connect\n");

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ChecksumServiceDisconnectSecureService(
    WDFDEVICE ServiceDevice
    )

/*++

    Routine Description:

        This routine is called to disconnect a secure service in preparation
        for a possible power-state change.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

    Return Value:

        NTSTATUS code.

--*/

{

    TrSecureDeviceLogMessage(ServiceDevice,
                             STATUS_SEVERITY_INFORMATIONAL,
                             "ChecksumService disconnect\n");

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ChecksumServiceCreateSessionContext(
    WDFDEVICE ServiceDevice,
    WDFOBJECT *SessionContextObject
    )

/*++

    Routine Description:

        This routine is called on creation of a new session to a secure
        service. This can be used to track any state through multiple requests
        from the same client.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to hold any session state that may
                         be required for future calls.

    Return Value:

        NTSTATUS code.

--*/

{

    PTREE_CHECKSUM_SERVICE_CONTEXT ServiceContext;
    PTREE_CHECKSUM_SESSION_CONTEXT SessionContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    WDFOBJECT NewContext;
    NTSTATUS Status;
    WDF_OBJECT_ATTRIBUTES TimerAttributes;
    WDF_TIMER_CONFIG TimerConfig;

    UNREFERENCED_PARAMETER(ServiceDevice);

    ServiceContext = WdfObjectGet_TREE_CHECKSUM_SERVICE_CONTEXT(ServiceDevice);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            TREE_CHECKSUM_SESSION_CONTEXT);

    Status = WdfObjectCreate(&ContextAttributes, &NewContext);
    if (!NT_SUCCESS(Status)) {
        goto ChecksumServiceCreateSessionContextEnd;
    }

    SessionContext = WdfObjectGet_TREE_CHECKSUM_SESSION_CONTEXT(NewContext);
    SessionContext->ServiceContext = ServiceContext;
    SessionContext->RunningSum = 0;

    WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&TimerAttributes, TREE_CHECKSUM_PENDED_FINALIZE);
    TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    TimerAttributes.ParentObject = ServiceDevice;

    WDF_TIMER_CONFIG_INIT(&TimerConfig, ChecksumServicePendedFinalizeWorker);
    Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &SessionContext->CompletionTimer);
    if (!NT_SUCCESS(Status)) {
        goto ChecksumServiceCreateSessionContextEnd;
    }

    *SessionContextObject = NewContext;
    ++ServiceContext->SessionsOpened;
    Status = STATUS_SUCCESS;

ChecksumServiceCreateSessionContextEnd:
    if (!NT_SUCCESS(Status)) {
        if (NewContext != NULL) {
            WdfObjectDelete(NewContext);
        }

        *SessionContextObject = NULL;
    }

    return Status;
}

_Use_decl_annotations_
NTSTATUS
ChecksumServiceDestroySessionContext(
    WDFDEVICE ServiceDevice,
    WDFOBJECT *SessionContextObject
    )

/*++

    Routine Description:

        This routine is called on destruction a session.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the session context.

    Return Value:

        NTSTATUS code.

--*/

{

    PTREE_CHECKSUM_SERVICE_CONTEXT ServiceContext;
    PTREE_CHECKSUM_SESSION_CONTEXT SessionContext;

    ServiceContext = WdfObjectGet_TREE_CHECKSUM_SERVICE_CONTEXT(ServiceDevice);
    SessionContext = WdfObjectGet_TREE_CHECKSUM_SESSION_CONTEXT(*SessionContextObject);

    WdfObjectDelete(SessionContext->CompletionTimer);

    WdfObjectDelete(*SessionContextObject);
    *SessionContextObject = NULL;
    ++ServiceContext->SessionsClosed;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ChecksumServiceProcessSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine is called to process a request to the secure service.
        This is typically the only way communication would be done to a secure
        service.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the context.

        RequestHandle - Supplies a pointer to a request handle that will be
                        used if the operation completes asynchronously.

        Priority - Supplies the priority of the request.

        Request - Supplies a pointer to the data for the request.

        RequestorMode - Supplies where the request originated from.

        BytesWritten - Supplies a pointer to be filled out with the number of
                       bytes written.

    Return Value:

        NTSTATUS code.

--*/

{
    PTREE_CHECKSUM_SESSION_CONTEXT SessionContext;
    NTSTATUS Status;

    SessionContext = WdfObjectGet_TREE_CHECKSUM_SESSION_CONTEXT(SessionContextObject);
    *BytesWritten = 0;

    if ((Request->FunctionCode >= sizeof(ChecksumServiceDispatch)/sizeof(ChecksumServiceDispatch[0])) ||
        (ChecksumServiceDispatch[Request->FunctionCode] == NULL)) {

        return STATUS_INVALID_PARAMETER;
    }

    Status = (*ChecksumServiceDispatch[Request->FunctionCode])(
        ServiceDevice,
        SessionContext,
        RequestHandle,
        Priority,
        Request,
        Flags,
        BytesWritten,
        RequestContext
        );

    return Status;
}

_Use_decl_annotations_
VOID
ChecksumServiceCancelSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine is called to cancel a request made via a previous call to
        ChecksumServiceProcessSecureServiceRequest. Note that cancellation is
        best-effort, and on success would result in STATUS_CANCELLED being
        returned from the original request.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the context.

        RequestHandle - Supplies the request handle for which a cancellation
                        is being requested.

    Return Value:

        NTSTATUS code.

--*/

{

    PTREE_CHECKSUM_SESSION_CONTEXT SessionContext;
    WDFTIMER Timer;

    UNREFERENCED_PARAMETER(ServiceDevice);

    if ((RequestContext == NULL) || (*RequestContext == NULL)) {
        SessionContext = WdfObjectGet_TREE_CHECKSUM_SESSION_CONTEXT(
                            SessionContextObject);

        if (WdfTimerStop(SessionContext->CompletionTimer, FALSE)) {
            TrSecureDeviceCompleteAsyncRequest(RequestHandle,
                                               STATUS_CANCELLED,
                                               0);
        }
    } else {

        Timer = (WDFTIMER)*RequestContext;
        if (WdfTimerStop(Timer, FALSE)) {
            ChecksumServiceDelayedCompletionComplete(Timer, STATUS_CANCELLED);
        }

        *RequestContext = NULL;
    }

    return;
}

DECLARE_CONST_UNICODE_STRING(HelloWorld, L"Hello, world!");

_Use_decl_annotations_
VOID
ChecksumServiceProcessOtherSecureServiceIo(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    WDFREQUEST Request
    )

/*++

    Routine Description:

        This routine is called when an unrecognized IO request is made to a
        secure service. This can be used to process private calls.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the context.

        Request - Supplies a pointer to the WDF request object.

    Return Value:

        NTSTATUS code.

--*/

{

    PVOID OutputBuffer;
    size_t OutputSize;
    WDF_REQUEST_PARAMETERS Parameters;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);

    if (Parameters.Parameters.DeviceIoControl.IoControlCode
        == IOCTL_TR_CHECKSUM_HELLOWORLD) {

        OutputSize = HelloWorld.Length + sizeof(WCHAR);
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                OutputSize,
                                                &OutputBuffer,
                                                NULL);

        RtlCopyMemory(OutputBuffer, HelloWorld.Buffer, OutputSize);
        Status = STATUS_SUCCESS;

    } else {

        OutputSize = 0;
        Status = STATUS_INVALID_PARAMETER;
    }

    WdfRequestCompleteWithInformation(Request,
                                      Status,
                                      OutputSize);
}

NTSTATUS
ChecksumServiceInitialize(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_CHECKSUM_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    NT_ASSERT(IoGetRemainingStackSize() >= 8000);

    SessionContext->RunningSum = 0;
    *BytesWritten = 0;

    return STATUS_SUCCESS;
}


NTSTATUS
ChecksumServiceUpdate(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_CHECKSUM_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    ULONG_PTR i;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    if (Request->InputBufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < Request->InputBufferSize; ++i) {
        SessionContext->RunningSum += ((PUCHAR)(PVOID)Request->InputBuffer)[i];
    }

    SessionContext->ServiceContext->BytesProcessed += Request->InputBufferSize;
    *BytesWritten = 0;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID
ChecksumServicePendedFinalizeWorker(
    WDFTIMER Timer
    )
{
    PTREE_CHECKSUM_PENDED_FINALIZE FinalizeContext;
    PTREE_CHECKSUM_SESSION_CONTEXT SessionContext;

    FinalizeContext = WdfObjectGet_TREE_CHECKSUM_PENDED_FINALIZE(Timer);
    SessionContext = FinalizeContext->SessionContext;
    *(PULONG)FinalizeContext->OutputBuffer = (UCHAR)(
        (SessionContext->RunningSum >> 24) ^
        (SessionContext->RunningSum >> 16) ^
        (SessionContext->RunningSum >> 8) ^
        (SessionContext->RunningSum));

    SessionContext->RunningSum = 0;

    TrSecureDeviceCompleteAsyncRequest(FinalizeContext->RequestHandle,
                                       STATUS_SUCCESS,
                                       sizeof(ULONG));
}

NTSTATUS
ChecksumServiceFinalize(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_CHECKSUM_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    PTREE_CHECKSUM_PENDED_FINALIZE FinalizeContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(RequestContext);

    if (Request->OutputBufferSize < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Delay the completion, just because we can.
    //

    FinalizeContext = WdfObjectGet_TREE_CHECKSUM_PENDED_FINALIZE(
        SessionContext->CompletionTimer);

    FinalizeContext->RequestHandle = RequestHandle;
    FinalizeContext->OutputBuffer = Request->OutputBuffer;
    FinalizeContext->SessionContext = SessionContext;

    //
    // Delay 3 sec
    //

    WdfTimerStart(SessionContext->CompletionTimer, -3000 * WDF_TIMEOUT_TO_MS);

    Status = STATUS_PENDING;

    return Status;
}

BOOLEAN
IsRPMBAvailable()
{
    BOOLEAN Available;
    PWSTR Interfaces;
    NTSTATUS Status;

    Status = IoGetDeviceInterfaces(&GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                   NULL,
                                   0,
                                   &Interfaces);

    Available = (NT_SUCCESS(Status) && 
                 (Interfaces != NULL) &&
                 (Interfaces[0] != '\0'));

    if (Interfaces != NULL) {
        ExFreePool(Interfaces);
    }

    return Available;
}

NTSTATUS
ChecksumServiceTestBuiltinRPMB(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_CHECKSUM_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    PSFFDISK_DEVICE_PARTITION_ACCESS_DATA Access;
    TR_SERVICE_REQUEST OSRequest;
    ULONG_PTR OSServiceBytesWritten;
    NTSTATUS Status;
    ULONG Index;
    PTREE_RPMB_AUTHENTICATED_WRITE_EX_INPUT WriteExIn;
    PTREE_RPMB_AUTHENTICATED_WRITE_EX_OUTPUT WriteExOut;
    
    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(RequestContext);

    if (!IsRPMBAvailable()) {
        *BytesWritten = 0;
        return STATUS_SUCCESS;
    }

    Access = NULL;
    WriteExIn = NULL;
    WriteExOut = NULL;

    Access = ExAllocatePoolWithTag(PagedPool, sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA) + 512 * 2, 'EErT');
    if (Access == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ChecksumServiceTestBuiltinRPMBEnd;
    }

    WriteExIn = ExAllocatePoolWithTag(PagedPool, sizeof(TREE_RPMB_AUTHENTICATED_WRITE_EX_INPUT) + 512 * 2, 'EErT');
    if (Access == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ChecksumServiceTestBuiltinRPMBEnd;
    }

    WriteExOut = ExAllocatePoolWithTag(PagedPool, sizeof(TREE_RPMB_AUTHENTICATED_WRITE_EX_OUTPUT) + 512, 'EErT');
    if (Access == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ChecksumServiceTestBuiltinRPMBEnd;
    }

    OSRequest.FunctionCode = TREE_RPMB_SFFDISK_PARTITION_ACCESS;
    OSRequest.InputBuffer = Access;
    OSRequest.OutputBuffer = Access;

    //
    // Is supported?
    //
    Access->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
    Access->Command = SFFDISK_RPMB_IS_SUPPORTED;

    OSRequest.InputBufferSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
    OSRequest.OutputBufferSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
    Status = TrSecureDeviceCallOSService(ServiceDevice,
                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                         &OSRequest,
                                         &OSServiceBytesWritten);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID,
               DPFLTR_ERROR_LEVEL,
               "RPMB size = %d, reliable write length = %d\n",
               Access->Parameters.RpmbIsSupported.SizeInBytes,
               Access->Parameters.RpmbIsSupported.MaxReliableWriteSizeInBytes);

    if (!NT_SUCCESS(Status) ||
        (Access->Parameters.RpmbIsSupported.SizeInBytes == 0)) {

        goto ChecksumServiceTestBuiltinRPMBEnd;
    }

    Access->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
    Access->Command = SFFDISK_RPMB_AUTHENTICATED_READ;
    Access->Parameters.RpmbAuthenticatedRead.CountToRead = 3;
    //
    // Read from 0x0201
    //
    Access->Parameters.RpmbAuthenticatedRead.AuthenticatedReadFrame.Address[0] = 0x02;
    Access->Parameters.RpmbAuthenticatedRead.AuthenticatedReadFrame.Address[1] = 0x01;

    OSRequest.InputBufferSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
    OSRequest.OutputBufferSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA) + 512 * 2;
    Status = TrSecureDeviceCallOSService(ServiceDevice,
                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                         &OSRequest,
                                         &OSServiceBytesWritten);
    if (!NT_SUCCESS(Status)) {
        goto ChecksumServiceTestBuiltinRPMBEnd;
    }

    for (Index = 0; Index < 3; ++Index) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   DPFLTR_ERROR_LEVEL,
                   "Half-sector %02x%02x: %02x %02x %02x %02x %02x ...\n",
                   Access->Parameters.RpmbAuthenticatedRead.ReturnedFrameData[Index].Address[0],
                   Access->Parameters.RpmbAuthenticatedRead.ReturnedFrameData[Index].Address[1],
                   Access->Parameters.RpmbAuthenticatedRead.ReturnedFrameData[Index].Data[0],
                   Access->Parameters.RpmbAuthenticatedRead.ReturnedFrameData[Index].Data[1],
                   Access->Parameters.RpmbAuthenticatedRead.ReturnedFrameData[Index].Data[2],
                   Access->Parameters.RpmbAuthenticatedRead.ReturnedFrameData[Index].Data[3],
                   Access->Parameters.RpmbAuthenticatedRead.ReturnedFrameData[Index].Data[4]);
    }

    Access->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
    Access->Command = SFFDISK_RPMB_AUTHENTICATED_WRITE;
    Access->Parameters.RpmbAuthenticatedWrite.CountToWrite = 2;
    //
    // Write to 0x0306, 0x0307
    //
    Access->Parameters.RpmbAuthenticatedWrite.FrameDataToWrite[0].Address[0] = 0x03;
    Access->Parameters.RpmbAuthenticatedWrite.FrameDataToWrite[0].Address[1] = 0x06;
    Access->Parameters.RpmbAuthenticatedWrite.FrameDataToWrite[1].Address[0] = 0x03;
    Access->Parameters.RpmbAuthenticatedWrite.FrameDataToWrite[1].Address[1] = 0x07;

    OSRequest.InputBufferSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA) + 512;
    OSRequest.OutputBufferSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
    Status = TrSecureDeviceCallOSService(ServiceDevice,
                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                         &OSRequest,
                                         &OSServiceBytesWritten);
    if (!NT_SUCCESS(Status)) {
        goto ChecksumServiceTestBuiltinRPMBEnd;
    }

    OSRequest.FunctionCode = TREE_RPMB_AUTHENTICATED_WRITE_EX;
    OSRequest.InputBuffer = WriteExIn;
    OSRequest.OutputBuffer = WriteExOut;

    WriteExIn->WriteFrameCount = 3;
    //
    // Write to 0x010a, 0x010b, 0x010c
    //
    WriteExIn->Data[0].Address[0] = 0x01;
    WriteExIn->Data[0].Address[1] = 0x0a;
    WriteExIn->Data[1].Address[0] = 0x01;
    WriteExIn->Data[1].Address[1] = 0x0b;
    WriteExIn->Data[2].Address[0] = 0x01;
    WriteExIn->Data[2].Address[1] = 0x0c;

    OSRequest.InputBufferSize = sizeof(TREE_RPMB_AUTHENTICATED_WRITE_EX_INPUT) + 512 * 2;
    OSRequest.OutputBufferSize = sizeof(TREE_RPMB_AUTHENTICATED_WRITE_EX_OUTPUT) + 512;
    Status = TrSecureDeviceCallOSService(ServiceDevice,
                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                         &OSRequest,
                                         &OSServiceBytesWritten);
    if (!NT_SUCCESS(Status)) {
        goto ChecksumServiceTestBuiltinRPMBEnd;
    }

    *BytesWritten = 0;
    Status = STATUS_SUCCESS;

ChecksumServiceTestBuiltinRPMBEnd:
    if (Access != NULL) {
        ExFreePool(Access);
    }

    if (WriteExIn != NULL) {
        ExFreePool(WriteExIn);
    }

    if (WriteExOut != NULL) {
        ExFreePool(WriteExOut);
    }

    return Status;
}

VOID
ChecksumServiceDelayedCompletionComplete(
    _In_ WDFTIMER Timer,
    _In_ NTSTATUS Status
    )
{
    PTREE_CHECKSUM_PENDED_FINALIZE FinalizeContext;
    KIRQL OldIrql;
    
    KeAcquireSpinLock(&ConcurrencyLock, &OldIrql);
    --CurrentConcurrency;
    KeReleaseSpinLock(&ConcurrencyLock, OldIrql);

    FinalizeContext = WdfObjectGet_TREE_CHECKSUM_PENDED_FINALIZE(Timer);
    TrSecureDeviceCompleteAsyncRequest(FinalizeContext->RequestHandle,
                                       Status,
                                       0);

    WdfObjectDelete(Timer);
}

_Use_decl_annotations_
VOID
ChecksumServiceDelayedCompletionWorker(
    WDFTIMER Timer
    )
{
    ChecksumServiceDelayedCompletionComplete(Timer, STATUS_SUCCESS);
}

NTSTATUS
ChecksumServiceDelayedCompletion(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_CHECKSUM_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    PTREE_CHECKSUM_PENDED_FINALIZE FinalizeContext;
    KIRQL OldIrql;
    NTSTATUS Status;
    ULONG Timeout;
    WDFTIMER Timer;
    WDF_OBJECT_ATTRIBUTES TimerAttributes;
    WDF_TIMER_CONFIG TimerConfig;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(RequestContext);

    if (Request->InputBufferSize < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&TimerAttributes,
                                            TREE_CHECKSUM_PENDED_FINALIZE);

    TimerAttributes.ParentObject = ServiceDevice;
    TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    TimerAttributes.ParentObject = ServiceDevice;

    WDF_TIMER_CONFIG_INIT(&TimerConfig, ChecksumServiceDelayedCompletionWorker);
    Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &Timer);
    if (!NT_SUCCESS(Status)) {
        goto ChecksumServiceDelayedCompletionEnd;
    }

    KeAcquireSpinLock(&ConcurrencyLock, &OldIrql);
    if (++CurrentConcurrency > ConcurrencyHighMark) {
        ConcurrencyHighMark = CurrentConcurrency;
    }

    KeReleaseSpinLock(&ConcurrencyLock, OldIrql);
    FinalizeContext = WdfObjectGet_TREE_CHECKSUM_PENDED_FINALIZE(Timer);
    FinalizeContext->RequestHandle = RequestHandle;
    Timeout = *(PULONG)(PVOID)Request->InputBuffer;
    WdfTimerStart(Timer, WDF_REL_TIMEOUT_IN_MS(Timeout));
    if (RequestContext != NULL) {
        *RequestContext = Timer;
    }
    Status = STATUS_PENDING;

ChecksumServiceDelayedCompletionEnd:
    return Status;
}

/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <ntddk.h>
#include <wdf.h>
#include <TrustedRuntimeClx.h>
#include "OpteeTrEE.h"
#include "OpteeTrEEService.h"

typedef struct _TREE_RNG_SERVICE_CONTEXT {
    POPTEE_TREE_DEVICE_CONTEXT DeviceContext;
    WDFDEVICE ServiceDevice;
} TREE_RNG_SERVICE_CONTEXT, *PTREE_RNG_SERVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_RNG_SERVICE_CONTEXT);

#define LEFTOVER_ELEMENTS 32
#define LEFTOVER_SIZE (sizeof(ULONG) * LEFTOVER_ELEMENTS)

typedef struct _TREE_RNG_SESSION_CONTEXT {
    PTREE_RNG_SERVICE_CONTEXT ServiceContext;

    ULONG PreviousValue;
} TREE_RNG_SESSION_CONTEXT, *PTREE_RNG_SESSION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_RNG_SESSION_CONTEXT);

typedef struct _TREE_RNG_DELAYED_COMPLETION {
    PVOID RequestHandle;
} TREE_RNG_DELAYED_COMPLETION, *PTREE_RNG_DELAYED_COMPLETION;

WDF_DECLARE_CONTEXT_TYPE(TREE_RNG_DELAYED_COMPLETION);

EVT_TR_CREATE_SECURE_SERVICE_CONTEXT RNGServiceCreateSecureServiceContext;
EVT_TR_DESTROY_SECURE_SERVICE_CONTEXT RNGServiceDestroySecureServiceContext;
EVT_TR_CONNECT_SECURE_SERVICE RNGServiceConnectSecureService;
EVT_TR_DISCONNECT_SECURE_SERVICE RNGServiceDisconnectSecureService;
EVT_TR_CREATE_SECURE_SERVICE_SESSION_CONTEXT RNGServiceCreateSessionContext;
EVT_TR_DESTROY_SECURE_SERVICE_SESSION_CONTEXT RNGServiceDestroySessionContext;
EVT_TR_PROCESS_SECURE_SERVICE_REQUEST RNGServiceProcessSecureServiceRequest;
EVT_TR_CANCEL_SECURE_SERVICE_REQUEST RNGServiceCancelSecureServiceRequest;
EVT_TR_PROCESS_OTHER_SECURE_SERVICE_IO RNGServiceProcessOtherSecureServiceIo;

EVT_WDF_TIMER RNGServiceDelayedCompletionWorker;

TR_SECURE_SERVICE_CALLBACKS RNG_SERVICE_CALLBACKS = {
    0,

    &RNGServiceCreateSecureServiceContext,
    &RNGServiceDestroySecureServiceContext,

    &RNGServiceConnectSecureService,
    &RNGServiceDisconnectSecureService,

    &RNGServiceCreateSessionContext,
    &RNGServiceDestroySessionContext,

    &RNGServiceProcessSecureServiceRequest,
    &RNGServiceCancelSecureServiceRequest,
    &RNGServiceProcessOtherSecureServiceIo
};

typedef
NTSTATUS
TREE_RNG_SERVICE_REQUEST_HANDLER(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_RNG_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    );

TREE_RNG_SERVICE_REQUEST_HANDLER RNGServiceSeed;
TREE_RNG_SERVICE_REQUEST_HANDLER RNGServiceDiscard;
TREE_RNG_SERVICE_REQUEST_HANDLER RNGServiceGetRandom;
TREE_RNG_SERVICE_REQUEST_HANDLER RNGServiceDelayedCompletion;
TREE_RNG_SERVICE_REQUEST_HANDLER RNGServiceConcurrencyHighMark;
TREE_RNG_SERVICE_REQUEST_HANDLER RNGServiceClearHighMark;

TREE_RNG_SERVICE_REQUEST_HANDLER *RNGServiceDispatch[] = {
    RNGServiceSeed,
    RNGServiceDiscard,
    RNGServiceGetRandom,
    RNGServiceGetRandom,
    RNGServiceDelayedCompletion,
    RNGServiceConcurrencyHighMark,
    RNGServiceClearHighMark,
};

_Function_class_(EVT_WDF_TIMER)
VOID
RNGServiceDelayedCompletionComplete(
    _In_ WDFTIMER Timer,
    _In_ NTSTATUS Status
    );

_Use_decl_annotations_
NTSTATUS
RNGServiceCreateSecureServiceContext(
    WDFDEVICE MasterDevice,
    LPCGUID ServiceGuid,
    WDFDEVICE ServiceDevice
    )
{
    PTREE_RNG_SERVICE_CONTEXT ServiceContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ServiceGuid);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            TREE_RNG_SERVICE_CONTEXT);

    Status = WdfObjectAllocateContext(ServiceDevice,
                                      &ContextAttributes,
                                      (PVOID*)&ServiceContext);

    if (!NT_SUCCESS(Status)) {
        goto RNGServiceCreateSecureServiceContextEnd;
    }

    RtlZeroMemory(ServiceContext, sizeof(TREE_RNG_SERVICE_CONTEXT));
    ServiceContext->DeviceContext = TreeGetDeviceContext(MasterDevice);
    ServiceContext->ServiceDevice = ServiceDevice;

    Status = STATUS_SUCCESS;

RNGServiceCreateSecureServiceContextEnd:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
RNGServiceDestroySecureServiceContext(
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
    PTREE_RNG_SERVICE_CONTEXT ServiceContext;

    ServiceContext = WdfObjectGet_TREE_RNG_SERVICE_CONTEXT(ServiceDevice);

    /*
    * No member needs cleanup here.
    */

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
RNGServiceConnectSecureService(
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
                             "RNGService connect\n");

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
RNGServiceDisconnectSecureService(
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
                             "RNGService disconnect\n");

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
RNGServiceCreateSessionContext(
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

    PTREE_RNG_SERVICE_CONTEXT ServiceContext;
    PTREE_RNG_SESSION_CONTEXT SessionContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    WDFOBJECT NewContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ServiceDevice);

    ServiceContext = WdfObjectGet_TREE_RNG_SERVICE_CONTEXT(ServiceDevice);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            TREE_RNG_SESSION_CONTEXT);

    Status = WdfObjectCreate(&ContextAttributes, &NewContext);
    if (!NT_SUCCESS(Status)) {
        goto RNGServiceCreateSessionContextEnd;
    }

    SessionContext = WdfObjectGet_TREE_RNG_SESSION_CONTEXT(NewContext);
    SessionContext->ServiceContext = ServiceContext;
    SessionContext->PreviousValue = 2014;

    *SessionContextObject = NewContext;
    Status = STATUS_SUCCESS;

RNGServiceCreateSessionContextEnd:
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
RNGServiceDestroySessionContext(
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

    UNREFERENCED_PARAMETER(ServiceDevice);

    WdfObjectDelete(*SessionContextObject);
    *SessionContextObject = NULL;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
RNGServiceProcessSecureServiceRequest(
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
    PTREE_RNG_SESSION_CONTEXT SessionContext;
    NTSTATUS Status;

    SessionContext = WdfObjectGet_TREE_RNG_SESSION_CONTEXT(SessionContextObject);

    if (Request->FunctionCode >= sizeof(RNGServiceDispatch)/sizeof(RNGServiceDispatch[0])) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = (*RNGServiceDispatch[Request->FunctionCode])(
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
RNGServiceCancelSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine is called to cancel a request made via a previous call to
        RNGServiceProcessSecureServiceRequest. Note that cancellation is
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

    WDFTIMER Timer;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);

    if (*RequestContext != NULL) {
        Timer = (WDFTIMER)*RequestContext;
        if (WdfTimerStop(Timer, FALSE)) {
            RNGServiceDelayedCompletionComplete(Timer, STATUS_CANCELLED);
        }

        *RequestContext = NULL;
    }

    TrSecureDeviceCompleteAsyncRequest(RequestHandle, STATUS_CANCELLED, 0);
}

_Use_decl_annotations_
VOID
RNGServiceProcessOtherSecureServiceIo(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContext,
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

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContext);

    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}

NTSTATUS
RNGServiceSeed(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_RNG_SESSION_CONTEXT SessionContext,
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

    if (Request->InputBufferSize < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }

    SessionContext->PreviousValue = *(PULONG)(PVOID)Request->InputBuffer;
    *BytesWritten = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
RNGServiceDiscard(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_RNG_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    ULONG_PTR BytesToDiscard;
    ULONG Value;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(RequestContext);

    if ((Request->FunctionCode == RNG_FUNCTION_GET_RANDOM_KERNEL) &&
        ((Flags & TR_SERVICE_REQUEST_FROM_USERMODE) != 0)) {

        return STATUS_ACCESS_DENIED;
    }

    if (Request->InputBufferSize < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }

    BytesToDiscard = *(PULONG)(PVOID)Request->InputBuffer;
    Value = SessionContext->PreviousValue;
    while (BytesToDiscard--) {
        Value = (((ULONGLONG)Value) * ((ULONGLONG)Value)) % 12637769;
        Value = (((ULONGLONG)Value) * ((ULONGLONG)Value)) % 12637769;
    }

    SessionContext->PreviousValue = Value;

    *BytesWritten = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
RNGServiceGetRandom(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_RNG_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    UCHAR OutputByte;
    PUCHAR OutputPointer;
    ULONG64 OutputRemaining;
    ULONG Value;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(RequestContext);

    if ((Request->FunctionCode == RNG_FUNCTION_GET_RANDOM_KERNEL) &&
        ((Flags & TR_SERVICE_REQUEST_FROM_USERMODE) != 0)) {

        return STATUS_ACCESS_DENIED;
    }

    if (Request->OutputBufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    OutputPointer = (PUCHAR)(PVOID)Request->OutputBuffer;
    OutputRemaining = Request->OutputBufferSize;

    Value = SessionContext->PreviousValue;
    while (OutputRemaining--) {
        Value = (((ULONGLONG)Value) * ((ULONGLONG)Value)) % 12637769;
        OutputByte = (UCHAR)(Value & 0xf);

        Value = (((ULONGLONG)Value) * ((ULONGLONG)Value)) % 12637769;
        OutputByte |= (UCHAR)(Value << 4);

        *(OutputPointer++) = OutputByte;
    }

    SessionContext->PreviousValue = Value;
    *BytesWritten = (ULONG_PTR)Request->OutputBufferSize;
    NT_ASSERT(Request->OutputBufferSize ==
        ((ULONG64)OutputPointer) - ((ULONG64)Request->OutputBuffer));

    return STATUS_SUCCESS;
}

_Function_class_(EVT_WDF_TIMER)
VOID
RNGServiceDelayedCompletionComplete(
    _In_ WDFTIMER Timer,
    _In_ NTSTATUS Status
    )
{
    PTREE_RNG_DELAYED_COMPLETION FinalizeContext;
    KIRQL OldIrql;
    
    KeAcquireSpinLock(&ConcurrencyLock, &OldIrql);
    --CurrentConcurrency;
    KeReleaseSpinLock(&ConcurrencyLock, OldIrql);

    FinalizeContext = WdfObjectGet_TREE_RNG_DELAYED_COMPLETION(Timer);
    TrSecureDeviceCompleteAsyncRequest(FinalizeContext->RequestHandle,
                                       Status,
                                       0);

    WdfObjectDelete(Timer);
}

_Function_class_(EVT_WDF_TIMER)
VOID
RNGServiceDelayedCompletionWorker(
    _In_ WDFTIMER Timer
    )
{
    RNGServiceDelayedCompletionComplete(Timer, STATUS_SUCCESS);
}

NTSTATUS
RNGServiceDelayedCompletion(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_RNG_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    PTREE_RNG_DELAYED_COMPLETION FinalizeContext;
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
                                            TREE_RNG_DELAYED_COMPLETION);

    TimerAttributes.ParentObject = ServiceDevice;
    TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    TimerAttributes.ParentObject = ServiceDevice;

    WDF_TIMER_CONFIG_INIT(&TimerConfig, RNGServiceDelayedCompletionWorker);
    Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &Timer);
    if (!NT_SUCCESS(Status)) {
        goto RNGServiceDelayedCompletionEnd;
    }

    KeAcquireSpinLock(&ConcurrencyLock, &OldIrql);
    if (++CurrentConcurrency > ConcurrencyHighMark) {
        ConcurrencyHighMark = CurrentConcurrency;
    }

    KeReleaseSpinLock(&ConcurrencyLock, OldIrql);
    FinalizeContext = WdfObjectGet_TREE_RNG_DELAYED_COMPLETION(Timer);
    FinalizeContext->RequestHandle = RequestHandle;
    Timeout = *(PULONG)(PVOID)Request->InputBuffer;
    WdfTimerStart(Timer, WDF_REL_TIMEOUT_IN_MS(Timeout));
    *RequestContext = Timer;
    Status = STATUS_PENDING;

RNGServiceDelayedCompletionEnd:
    return Status;
}

NTSTATUS
RNGServiceConcurrencyHighMark(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_RNG_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(RequestContext);

    if (Request->OutputBufferSize < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }

    *(PULONG)(PVOID)Request->OutputBuffer = ConcurrencyHighMark;
    *BytesWritten = sizeof(ULONG);
    return STATUS_SUCCESS;
}

NTSTATUS
RNGServiceClearHighMark(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTREE_RNG_SESSION_CONTEXT SessionContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    )
{
    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(RequestContext);

    ConcurrencyHighMark = 0;
    *BytesWritten = 0;
    return STATUS_SUCCESS;
}
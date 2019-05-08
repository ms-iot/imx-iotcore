/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    GenService.c

Abstract:

    The OPTEE generic service implementation

Environment:

    Kernel mode.

*/

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <TrustedRuntimeClx.h>
#include <initguid.h>
#include <wdmguid.h>
#include "OpteeTrEE.h"
#include "OpteeTrEEService.h"
#include <TrEEGenService.h>
#include "OpteeClientLib\OpteeClientLib.h"
#include "OpteeClientLib\OpteeClientMemory.h"
#include "trace.h"

#ifdef WPP_TRACING
#include "GenService.tmh"
#endif

//
// Memory tags
//

#define GENSVC_SESSION_CONTEXT_TAG 'sTPO'
#define GENSVC_REQUEST_CONTEXT_TAG 'rTPO'

//
// TrEE Generic Service private types
//

typedef struct _TREE_GEN_SERVICE_CONTEXT {
    POPTEE_TREE_DEVICE_CONTEXT DeviceContext;
    WDFDEVICE ServiceDevice;

    TEEC_Context TEECContext;
    TEEC_UUID TeecUuid;

    ULONG SessionsOpened;
    ULONG SessionsClosed;

    WDFLOOKASIDE RequestContextPool;
} TREE_GEN_SERVICE_CONTEXT, *PTREE_GEN_SERVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TREE_GEN_SERVICE_CONTEXT, TreeGenGetServiceContext);

typedef struct _TREE_GEN_SESSION_CONTEXT {
    WDFOBJECT Handle;
    PTREE_GEN_SERVICE_CONTEXT ServiceContext;
    TEEC_Session ClientSession;

    FAST_MUTEX PendingRequestsLock;
    WDFCOLLECTION PendingRequests;
} TREE_GEN_SESSION_CONTEXT, *PTREE_GEN_SESSION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TREE_GEN_SESSION_CONTEXT, TreeGenGetSessionContext);

typedef enum _TREE_GEN_REQUEST_COMPLETION_TYPE {
    RequestCompletePartial = 0,
    RequestCompleteFinal,
} TREE_GEN_REQUEST_COMPLETION_TYPE;

typedef enum _TREE_GEN_REQUEST_STATE {
    RequestStateIdle = 0,
    RequestStatePending,
    RequestStateSetup,
    RequestStateSubmitted,
    RequestStateWaitRpcResponse,
    RequestStateRpcResponse,
    RequestStateCompleted,
    RequestStateCancelled
} TREE_GEN_REQUEST_STATE;

typedef struct _TREE_GEN_REQUEST_CONTEXT {
    WDFMEMORY Handle;
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    TREE_GEN_REQUEST_STATE State;
    KEVENT RpcCompletedEvent;

    ULONG Key;
    ULONG FunctionCode;
    PVOID TreeRequestHandle;

    _Field_size_bytes_(InputBufferSize) PVOID InputBuffer;
    SIZE_T InputBufferSize;
    _Field_size_bytes_(OutputBufferSize) PVOID OutputBuffer;
    SIZE_T OutputBufferSize;

    _Field_size_bytes_(RpcOutputSize) PVOID RpcResponseBuffer;
    SIZE_T RpcResponseBufferSize;

} TREE_GEN_REQUEST_CONTEXT, *PTREE_GEN_REQUEST_CONTEXT;

typedef struct _TREE_GEN_REQUEST_WORKER_CONTEXT
{
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    WDFMEMORY RequestContextHandle;
    ULONG RequestKey;

} TREE_GEN_REQUEST_WORKER_CONTEXT, *PTREE_GEN_REQUEST_WORKER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TREE_GEN_REQUEST_WORKER_CONTEXT, TreeGenGetRequestWorkerContext);

//
// TrEE Generic Service private methods
//

__requires_lock_held(SessionContext->PendingRequestsLock)
static PTREE_GEN_REQUEST_CONTEXT
GenServiceGetRequestContextByKey(
    _In_ PTREE_GEN_SESSION_CONTEXT SessionContext,
    _In_ ULONG Key
    );

__requires_lock_held(SessionContext->PendingRequestsLock)
static PTREE_GEN_REQUEST_CONTEXT
GenServiceGetRequestContextByHandle(
    _In_ PTREE_GEN_SESSION_CONTEXT SessionContext,
    _In_ WDFOBJECT WdfRequestContext
    );

static PTREE_GEN_SESSION_CONTEXT
GenServiceGetSessionContextById(
    _In_ UINT32 SessionId
    );

static NTSTATUS
GenServiceAddSession(
    _In_ WDFOBJECT Session
    );

static VOID
GenServiceRemoveSession(
    _In_ WDFOBJECT Session
    );

static NTSTATUS
GenServiceInvokeCommandDeferred(
    _In_ PTREE_GEN_SESSION_CONTEXT SessionContext,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ PVOID RequestHandle,
    _Out_ PVOID* MiniportRequestContext
    );

static NTSTATUS
GenServiceSendRpcResponse(
    _In_ PTREE_GEN_SESSION_CONTEXT SessionContext,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ PVOID RequestHandle,
    _Out_ PVOID* MiniportRequestContext
    );

static VOID
GenServiceCompleteRequestAsync(
    _In_ PTREE_GEN_REQUEST_CONTEXT RequestContext,
    _In_ NTSTATUS Status,
    _In_ ULONG_PTR BytesWritten,
    _In_ TREE_GEN_REQUEST_COMPLETION_TYPE CompletionType
    );

static EVT_WDF_WORKITEM GenServiceEvtRequestWorker;
static EVT_WDF_OBJECT_CONTEXT_DESTROY GenServiceEvtSessionDestroy;

//
// TrEEClx handlers
//

EVT_TR_CREATE_SECURE_SERVICE_CONTEXT GenServiceCreateSecureServiceContext;
EVT_TR_DESTROY_SECURE_SERVICE_CONTEXT GenServiceDestroySecureServiceContext;
EVT_TR_CONNECT_SECURE_SERVICE GenServiceConnectSecureService;
EVT_TR_DISCONNECT_SECURE_SERVICE GenServiceDisconnectSecureService;
EVT_TR_CREATE_SECURE_SERVICE_SESSION_CONTEXT GenServiceCreateSessionContext;
EVT_TR_DESTROY_SECURE_SERVICE_SESSION_CONTEXT GenServiceDestroySessionContext;
EVT_TR_PROCESS_SECURE_SERVICE_REQUEST GenServiceProcessSecureServiceRequest;
EVT_TR_CANCEL_SECURE_SERVICE_REQUEST GenServiceCancelSecureServiceRequest;
EVT_TR_PROCESS_OTHER_SECURE_SERVICE_IO GenServiceProcessOtherSecureServiceIo;

TR_SECURE_SERVICE_CALLBACKS GEN_SERVICE_CALLBACKS = {
    0,

    &GenServiceCreateSecureServiceContext,
    &GenServiceDestroySecureServiceContext,

    &GenServiceConnectSecureService,
    &GenServiceDisconnectSecureService,

    &GenServiceCreateSessionContext,
    &GenServiceDestroySessionContext,

    &GenServiceProcessSecureServiceRequest,
    &GenServiceCancelSecureServiceRequest,
    &GenServiceProcessOtherSecureServiceIo
};


//
// Session context list
//

WDFCOLLECTION GenSessionContextList = NULL;
FAST_MUTEX GenSessionContextListLock;


_Use_decl_annotations_
NTSTATUS
GenServiceCreateSecureServiceContext(
    WDFDEVICE MasterDevice,
    LPCGUID ServiceGuid,
    WDFDEVICE ServiceDevice
    )

/*++

Routine Description:

    This routine initializes and finalizes Secure Context by calling OpteeClient Library API

Arguments:

    ServiceDevice - Supplies a handle to the service device object.

Return Value:

    NTSTATUS code.

--*/

{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    TEEC_Result TeecResult;
    PTREE_GEN_SERVICE_CONTEXT GenServiceContext;
    TEEC_UUID* TeecUuidPtr;

    if (GenSessionContextList == NULL) {
        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = MasterDevice;

        Status = WdfCollectionCreate(&Attributes, &GenSessionContextList);
        if (!NT_SUCCESS(Status)) {
            TraceError("Failed to create the generic sessions collection, %!STATUS!", Status);
            goto Exit;
        }

        ExInitializeFastMutex(&GenSessionContextListLock);
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes,
        TREE_GEN_SERVICE_CONTEXT);

    Status = WdfObjectAllocateContext(ServiceDevice,
        &Attributes,
        (PVOID*)&GenServiceContext);
    if (!NT_SUCCESS(Status)) {
        TraceError("Failed to allocate service context, %!STATUS!", Status);
        goto Exit;
    }

    RtlZeroMemory(GenServiceContext, sizeof(TREE_GEN_SERVICE_CONTEXT));
    GenServiceContext->DeviceContext = TreeGetDeviceContext(MasterDevice);
    GenServiceContext->ServiceDevice = ServiceDevice;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = ServiceDevice;

    Status = WdfLookasideListCreate(
        &Attributes,
        sizeof(TREE_GEN_REQUEST_CONTEXT),
        NonPagedPool,
        WDF_NO_OBJECT_ATTRIBUTES,
        GENSVC_REQUEST_CONTEXT_TAG,
        &GenServiceContext->RequestContextPool);
    if (!NT_SUCCESS(Status)) {
        TraceError(
            "Failed to create request context lookaside list, %!STATUS!", 
            Status);

        goto Exit;
    }

    // Temporarily we are using the service UUID as the TA UUID.
    // Also make the adjustment to match how OPTEE expects the UUID.
    //

    TeecUuidPtr = &GenServiceContext->TeecUuid;
    RtlCopyMemory(TeecUuidPtr, ServiceGuid, sizeof(GUID));
    TeecUuidPtr->timeLow = SWAP_B32(TeecUuidPtr->timeLow);
    TeecUuidPtr->timeMid = SWAP_B16(TeecUuidPtr->timeMid);
    TeecUuidPtr->timeHiAndVersion = SWAP_B16(TeecUuidPtr->timeHiAndVersion);

    Status = OpteeClientApiLibInitialize(NULL, 
        ServiceDevice,
        GenServiceContext->DeviceContext->SharedMemoryBaseAddress,
        GenServiceContext->DeviceContext->SharedMemoryLength);

    if (!NT_SUCCESS(Status)) {
        TraceError(
            "OpteeClientApiLibInitialize failed, status %!STATUS!", Status);

        goto Exit;
    }

    TeecResult = TEEC_InitializeContext(NULL, &GenServiceContext->TEECContext);
    if (TeecResult == TEEC_SUCCESS) {
        Status = STATUS_SUCCESS;
    } else {
        TraceError("TEEC_InitializeContext failed, status 0x%X", TeecResult);
        Status = STATUS_UNSUCCESSFUL;
    }

Exit:

    return Status;
}

_Use_decl_annotations_
NTSTATUS
GenServiceDestroySecureServiceContext(
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
    PTREE_GEN_SERVICE_CONTEXT ServiceContext;
    ServiceContext = TreeGenGetServiceContext(ServiceDevice);

    TraceInformation("GenServiceCreateSecureServiceContext : Entered");

    OpteeClientApiLibDeinitialize(NULL, ServiceContext->ServiceDevice);

    TEEC_FinalizeContext(&ServiceContext->TEECContext);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
GenServiceConnectSecureService(
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

    UNREFERENCED_PARAMETER(ServiceDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
GenServiceDisconnectSecureService(
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

    UNREFERENCED_PARAMETER(ServiceDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
GenServiceCreateSessionContext(
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

    NTSTATUS Status;
    TEEC_Result TeecResult;
    PTREE_GEN_SERVICE_CONTEXT ServiceContext;
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFOBJECT SessionHandle;
    UINT32 ErrorOrigin;

    *SessionContextObject = NULL;

    ServiceContext = TreeGenGetServiceContext(ServiceDevice);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, TREE_GEN_SESSION_CONTEXT);
    Attributes.ParentObject = ServiceDevice;
    Attributes.EvtDestroyCallback = GenServiceEvtSessionDestroy;

    Status = WdfObjectCreate(&Attributes, &SessionHandle);
    if (!NT_SUCCESS(Status)) {
        TraceError("Failed to create session context,  %!STATUS!", Status);
        goto Exit;
    }

    SessionContext = TreeGenGetSessionContext(SessionHandle);
    SessionContext->Handle = SessionHandle;
    SessionContext->ServiceContext = ServiceContext;
    ExInitializeFastMutex(&SessionContext->PendingRequestsLock);
    RtlZeroMemory(&SessionContext->ClientSession,
        sizeof(SessionContext->ClientSession));

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = SessionHandle;

    Status = WdfCollectionCreate(&Attributes, &SessionContext->PendingRequests);
    if (!NT_SUCCESS(Status)) {
        TraceError("Failed to create pending requests collection, %!STATUS!", Status);
        goto Exit;
    }

    TeecResult = TEEC_OpenSession(&ServiceContext->TEECContext,
        &SessionContext->ClientSession,
        &ServiceContext->TeecUuid,
        TEEC_LOGIN_PUBLIC,
        NULL,
        NULL,
        &ErrorOrigin);
    if (TeecResult != TEEC_SUCCESS) {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    Status = GenServiceAddSession(SessionHandle);
    if (!NT_SUCCESS(Status)) {
        TraceError("Failed to add session to service sessions list, %!STATUS!", Status);
        goto Exit;
    }

    *SessionContextObject = SessionHandle;
    Status = STATUS_SUCCESS;   
    
Exit:

    if (!NT_SUCCESS(Status)) {
        if (SessionHandle != NULL) {
            WdfObjectDelete(SessionHandle);
        }
    }
    return Status;
}

_Use_decl_annotations_
VOID
GenServiceEvtSessionDestroy(
    WDFOBJECT SessionContextObject
    )

/*++

Routine Description:

    This routine is called just before the session context object is deleted. 
    This guaranties that all pending requests objects have been completed, 
    so we can safely close the session.

Arguments:

    SessionContextObject - The session context handle

Return Value:

    NTSTATUS code.

--*/

{

    PTREE_GEN_SESSION_CONTEXT SessionContext;

    SessionContext = TreeGenGetSessionContext(SessionContextObject);

    TraceWarning("Closing session 0x%X", 
        SessionContext->ClientSession.session_id);

    if (SessionContext->ClientSession.session_id != 0) {
        TEEC_CloseSession(&SessionContext->ClientSession);
    }
}

_Use_decl_annotations_
NTSTATUS
GenServiceDestroySessionContext(
    WDFDEVICE ServiceDevice,
    WDFOBJECT *SessionContextObject
    )

/*++

Routine Description:

    This routine is called on a destruction of a session.

Arguments:

    ServiceDevice - Supplies a handle to the service device object.

    SessionContext - Supplies a pointer to the session context.

Return Value:

    NTSTATUS code.

--*/

{

    PTREE_GEN_SERVICE_CONTEXT ServiceContext;
    PTREE_GEN_SESSION_CONTEXT SessionContext;

    ServiceContext = TreeGenGetServiceContext(ServiceDevice);

    if (*SessionContextObject != NULL) {
        SessionContext = TreeGenGetSessionContext(*SessionContextObject);
        
        GenServiceRemoveSession(*SessionContextObject);
        
        //
        // Session object will actually be deleted when all pending requests
        // objects have been deleted.
        //
        // The OPTEE session is closed just before the session object gets
        // deleted.
        //

        WdfObjectDelete(*SessionContextObject); 
        *SessionContextObject = NULL;
    }   

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
GenServiceProcessSecureServiceRequest(
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

    Flags - Request flags.

    BytesWritten - Supplies a pointer to be filled out with the number of
        bytes written.

    RequestContext - Request context that will be associated with the request 
        throughout the request life cycle.

Return Value:

    NTSTATUS code.

--*/

{
    
    PGENSVC_INPUT_BUFFER_HEADER InputHeader;
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    PTREE_GEN_SERVICE_CONTEXT ServiceContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);

    ServiceContext = TreeGenGetServiceContext(ServiceDevice);
    SessionContext = TreeGenGetSessionContext(SessionContextObject);
    *BytesWritten = 0;

    //
    // Validate parameters
    //

    InputHeader = (GENSVC_INPUT_BUFFER_HEADER*)((PVOID)Request->InputBuffer);

    if (Request->InputBufferSize < OPTEE_MINIMUM_COMMAND_INPUT_SIZE) {

        TraceError("Request input size %lld too small, expected %lld", 
            Request->InputBufferSize,
            OPTEE_MINIMUM_COMMAND_INPUT_SIZE);

        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Request->OutputBufferSize < OPTEE_MINIMUM_COMMAND_OUTPUT_SIZE) {

        TraceError("Request output size %lld too small, expected %lld",
            Request->OutputBufferSize,
            OPTEE_MINIMUM_COMMAND_OUTPUT_SIZE);

        return STATUS_BUFFER_TOO_SMALL;
    }

    if (InputHeader->InputDataSize >
        (Request->InputBufferSize - sizeof(GENSVC_INPUT_BUFFER_HEADER))) {

        TraceError("Invalid input data size %lld, cannot exceed %lld",
            InputHeader->InputDataSize,
            (Request->InputBufferSize - sizeof(GENSVC_INPUT_BUFFER_HEADER)));

        return STATUS_INVALID_PARAMETER;
    }

    if (InputHeader->OutputDataSize >
        (Request->OutputBufferSize - sizeof(GENSVC_OUTPUT_BUFFER_HEADER))) {

        TraceError("Invalid output data size %lld, cannot exceed %lld",
            InputHeader->OutputDataSize,
            (Request->OutputBufferSize - sizeof(GENSVC_OUTPUT_BUFFER_HEADER)));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Branch on request type
    //

    switch (InputHeader->Type) {
    case GenSvcInputTypeCommand:
        Status = GenServiceInvokeCommandDeferred(SessionContext,
            Request,
            RequestHandle,
            RequestContext);

        break;

    case GenSvcInputTypeRpcResponse:
        Status = GenServiceSendRpcResponse(SessionContext,
            Request,
            RequestHandle,
            RequestContext);
        
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return Status;
}

_Use_decl_annotations_
VOID
GenServiceCancelSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    PVOID* MiniportRequestContext
    )

/*++

Routine Description:

    This routine is called to cancel a request made via a previous call to
    GenServiceProcessSecureServiceRequest. 
    The callback makes sure the request is still active, updates the state 
    to canceled, and unblocks any pending threads.
    The request then continues its normal completion path.
    
    Keep in mind the request context only gets removed when it is completed 
    in the request completion path.

Arguments:

    ServiceDevice - Supplies a handle to the service device object.

    SessionContext - Supplies a pointer to the context.

    RequestHandle - The handle of the request to be canceled.

    MiniportRequestContext - The miniport context that will be associated with
        the request.

Return Value:

    NTSTATUS code.

--*/

{

    PTREE_GEN_SESSION_CONTEXT SessionContext;
    PTREE_GEN_REQUEST_CONTEXT RequestContext;
    WDFMEMORY WdfRequestContext;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(RequestHandle);

    SessionContext = TreeGenGetSessionContext(SessionContextObject);

    WdfRequestContext = (WDFMEMORY)(*MiniportRequestContext);
    NT_ASSERT(WdfRequestContext != NULL);

    ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

    RequestContext = GenServiceGetRequestContextByHandle(SessionContext, 
        WdfRequestContext);

    TraceWarning("Canceling request 0x%p, context 0x%p, key 0x%08X, session 0x%p",
        RequestHandle,
        RequestContext,
        (RequestContext == NULL ? -1 : RequestContext->Key),
        SessionContext);

    if (RequestContext != NULL) {
        RequestContext->State = RequestStateCancelled;
        KeSetEvent(&RequestContext->RpcCompletedEvent, IO_NO_INCREMENT, FALSE);
    }

    ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
}

_Use_decl_annotations_
VOID
GenServiceProcessOtherSecureServiceIo(
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

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);

    WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
}

_Use_decl_annotations_
VOID
GenServiceEvtRequestWorker(
    WDFWORKITEM WdfWorkItem
    )

/*++

Routine Description:

    This routine is the request submission worker thread.

Arguments:

    WdfWorkItem - The work item handle.

Return Value:


--*/

{

    ULONG_PTR BytesWritten;
    ULONG ErrorOrigin;
    BOOLEAN FreeMemory;
    size_t InputDataSize;
    ULONG OutputParamIndex;
    ULONG ParamCount;
    ULONG ParamsType[TEEC_CONFIG_PAYLOAD_REF_COUNT];
    size_t OutputDataSize;
    PGENSVC_OUTPUT_BUFFER_HEADER OutputHeader;
    PGENSVC_INPUT_BUFFER_HEADER InputHeader;
    PTREE_GEN_REQUEST_CONTEXT RequestContext;
    UCHAR *SharedInput;
    UCHAR *SharedOutput;
    NTSTATUS Status;
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    TEEC_Operation TeecOperation;
    TEEC_Result TeecResult;
    TEEC_SharedMemory TeecSharedMem;
    PTREE_GEN_REQUEST_WORKER_CONTEXT WorkItemContext;
    
    ParamsType[0] = ParamsType[1] = ParamsType[2] = TEEC_NONE;
    OutputParamIndex = (ULONG)-1;
    SharedInput = NULL;
    SharedOutput = NULL;
    ParamCount = 0;
    FreeMemory = FALSE;
    BytesWritten = 0;
    Status = STATUS_SUCCESS;

    RtlZeroMemory(&TeecOperation, sizeof(TeecOperation));
    RtlZeroMemory(&TeecSharedMem, sizeof(TeecSharedMem));

    WorkItemContext = TreeGenGetRequestWorkerContext(WdfWorkItem);
    SessionContext = WorkItemContext->SessionContext;

    RequestContext = (PTREE_GEN_REQUEST_CONTEXT)
        WdfMemoryGetBuffer(WorkItemContext->RequestContextHandle , NULL);

    NT_ASSERT(RequestContext != NULL);
    NT_ASSERT(RequestContext->Key == WorkItemContext->RequestKey);

    RequestContext->State = RequestStateSetup;

    InputHeader = (PGENSVC_INPUT_BUFFER_HEADER)RequestContext->InputBuffer;
    InputDataSize = InputHeader->InputDataSize;
    OutputDataSize = InputHeader->OutputDataSize;

    NT_ASSERT(InputHeader->Key == RequestContext->Key);


    //
    // param[0] is always the key.
    //

    ParamsType[ParamCount] = TEEC_VALUE_INPUT;
    TeecOperation.params[ParamCount].value.a = RequestContext->Key;
    ++ParamCount;

    //
    // Create shared memory copies of the buffers, if needed
    //

    TeecSharedMem.size = InputDataSize + OutputDataSize;

    if (TeecSharedMem.size != 0) {
        TeecResult = TEEC_AllocateSharedMemory(
            &SessionContext->ServiceContext->TEECContext,
            &TeecSharedMem);

        if (TeecResult != TEEC_SUCCESS) {
            TraceError("TEEC_AllocateSharedMemory failed, for %Id bytes", 
                TeecSharedMem.size);

            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
        FreeMemory = TRUE;
        SharedInput = (UINT8 *)TeecSharedMem.buffer;

        if (InputDataSize != 0) {
            NT_ASSERT(RequestContext->InputBuffer != NULL);
            
            RtlCopyMemory(SharedInput, InputHeader + 1, InputDataSize);

            ParamsType[ParamCount] = TEEC_MEMREF_TEMP_INPUT;
            TeecOperation.params[ParamCount].tmpref.buffer = 
                (PVOID)(ULONG_PTR)OpteeClientVirtualToPhysical(SharedInput);

            TeecOperation.params[ParamCount].tmpref.size = InputDataSize;
            ++ParamCount;
        }

        if (OutputDataSize != 0) {
            NT_ASSERT(RequestContext->OutputBuffer != NULL);

            OutputParamIndex = ParamCount;
            SharedOutput = SharedInput + InputDataSize;

            ParamsType[ParamCount] = TEEC_MEMREF_TEMP_INOUT;
            TeecOperation.params[ParamCount].tmpref.buffer = 
                (PVOID)(ULONG_PTR)OpteeClientVirtualToPhysical(SharedOutput);

            TeecOperation.params[ParamCount].tmpref.size = OutputDataSize;
            ++ParamCount;
        }
    }

    NT_ASSERT(ParamCount <= 3);

    TeecOperation.paramTypes = TEEC_PARAM_TYPES(ParamsType[0], 
        ParamsType[1], 
        ParamsType[2],
        TEEC_NONE);

    ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

    switch (RequestContext->State) {
    case RequestStateSetup:
        RequestContext->State = RequestStateSubmitted;
        Status = STATUS_SUCCESS;
        break;

    case RequestStateCancelled:
        Status = STATUS_CANCELLED;
        break;

    default:
        NT_ASSERT(FALSE);
        Status = STATUS_INVALID_DEVICE_STATE;
        break;
    }

    ExReleaseFastMutex(&SessionContext->PendingRequestsLock);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    TraceDebug("Submitting a new request, key %d, handle 0x%p, context 0x%p",
        RequestContext->Key,
        RequestContext->TreeRequestHandle,
        RequestContext);

    TeecResult = TEEC_InvokeCommand(&SessionContext->ClientSession,
        RequestContext->FunctionCode,
        &TeecOperation,
        (uint32_t *)&ErrorOrigin);

    if (TeecResult != TEEC_SUCCESS) {
        TraceError("TEEC_InvokeCommand failed, key %d, handle 0x%p, "
            "context 0x%p",
            RequestContext->Key,
            RequestContext->TreeRequestHandle,
            RequestContext);

        Status = STATUS_INVALID_PARAMETER;
    } else {
        TraceDebug("TEEC_InvokeCommand completed: key %d, handle 0x%p, "
            "context 0x%p",
            RequestContext->Key,
            RequestContext->TreeRequestHandle,
            RequestContext);

        Status = STATUS_SUCCESS;

        if ((RequestContext->OutputBuffer != NULL) &&
            (SharedOutput != NULL)) {

            NT_ASSERT(OutputParamIndex != (ULONG)-1);

            InputHeader =
                (PGENSVC_INPUT_BUFFER_HEADER)RequestContext->InputBuffer;

            OutputHeader =
                (PGENSVC_OUTPUT_BUFFER_HEADER)RequestContext->OutputBuffer;

            OutputHeader->Type = GenSvcOutputTypeCommandCompleted;

            OutputDataSize = InputHeader->OutputDataSize;

            BytesWritten =
                TeecOperation.params[OutputParamIndex].memref.size;

            if (OutputDataSize >= BytesWritten) {
                _Analysis_assume_(SharedOutput != NULL);
                RtlCopyMemory(OutputHeader + 1, SharedOutput, BytesWritten);
                Status = STATUS_SUCCESS;
            } else {
                TraceError("App buffer too small %Id, required size %Id",
                    OutputDataSize,
                    BytesWritten);

                NT_ASSERT(FALSE);
                Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            // Number of bytes reported to app
            //

            BytesWritten += sizeof(GENSVC_OUTPUT_BUFFER_HEADER);
        }
    }

Exit:

    GenServiceCompleteRequestAsync(RequestContext, Status, BytesWritten, RequestCompleteFinal);

    if (FreeMemory != FALSE) {
        TEEC_ReleaseSharedMemory(&TeecSharedMem);
    }

    WdfObjectDelete(WdfWorkItem);
}

_Use_decl_annotations_
static NTSTATUS
GenServiceInvokeCommandDeferred(
    PTREE_GEN_SESSION_CONTEXT SessionContext,
    PTR_SERVICE_REQUEST Request,
    PVOID RequestHandle,
    PVOID* MiniportRequestContext
    )

/*++

Routine Description:

    This routine is called when to send a command to the TEE asynchronously.
    The routine prepares the request context, add it to the pending request list,
    and schedules a worker thread to send it to the TEE.
    This is required in order to enable handling subsequent RPC calls that the 
    original call may trigger.

Arguments:

    SessionContext - Supplies a handle to the service device object.

    Request - Supplies a pointer to the context.

    RequestHandle - The framework request handle.

    MiniportRequestContext - The miniport context that will be associated with 
        the request.

Return Value:

    NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES Attributes;
    PTREE_GEN_REQUEST_CONTEXT RequestContext;
    ULONG RequestKey;
    PTREE_GEN_SERVICE_CONTEXT ServiceContext;
    NTSTATUS Status;
    PTREE_GEN_REQUEST_WORKER_CONTEXT WorkerContext;
    WDFMEMORY RequestContextHandle;
    WDF_WORKITEM_CONFIG WdfWorkItemConfig;
    WDFWORKITEM WdfWorkItem;

    *MiniportRequestContext = NULL;
    
    ServiceContext = SessionContext->ServiceContext;
    RequestContext = NULL;
    RequestContextHandle = NULL;
    WdfWorkItem = NULL;
    RequestKey = 
        ((GENSVC_INPUT_BUFFER_HEADER*)((PVOID)Request->InputBuffer))->Key;

    Status = WdfMemoryCreateFromLookaside(ServiceContext->RequestContextPool,
        &RequestContextHandle);

    if (!NT_SUCCESS(Status)) {
        TraceError("Failed to allocate request context, %!STATUS!", Status);
        goto Exit;
    }

    RequestContext = 
        (TREE_GEN_REQUEST_CONTEXT*)WdfMemoryGetBuffer(RequestContextHandle, NULL);

    TraceDebug("Starting a new request, key %d, request handle 0x%p, "
        "request context 0x%p",
        RequestKey,
        RequestHandle,
        RequestContext);

    RequestContext->Handle = RequestContextHandle;
    RequestContext->SessionContext = SessionContext;
    RequestContext->State = RequestStateIdle;

    RequestContext->Key = RequestKey;
    RequestContext->FunctionCode = Request->FunctionCode;
    RequestContext->SessionContext = SessionContext;
    RequestContext->TreeRequestHandle = RequestHandle;
    RequestContext->InputBuffer = Request->InputBuffer;
    RequestContext->InputBufferSize = (SIZE_T)Request->InputBufferSize;
    RequestContext->OutputBuffer = Request->OutputBuffer;
    RequestContext->OutputBufferSize = (SIZE_T)Request->OutputBufferSize;
    RequestContext->RpcResponseBuffer = NULL;
    RequestContext->RpcResponseBuffer = 0;

    KeInitializeEvent(&RequestContext->RpcCompletedEvent, 
        SynchronizationEvent, 
        FALSE);

    //
    // Add request to pending request list
    //

    ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

    if (GenServiceGetRequestContextByKey(SessionContext, RequestKey) != NULL) {
        TraceError("Key %d, is already associated with a pending request!", RequestKey);
        Status = STATUS_INVALID_PARAMETER;
    } else {
        Status = WdfCollectionAdd(SessionContext->PendingRequests, RequestContextHandle);
        if (!NT_SUCCESS(Status)) {
            TraceError("WdfCollectionAdd failed , %!STATUS!", Status);
        }
        RequestContext->State = RequestStatePending;
        WdfObjectReference(SessionContext->Handle);
    }

    ExReleaseFastMutex(&SessionContext->PendingRequestsLock);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Create a queue the work item
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&Attributes, 
        TREE_GEN_REQUEST_WORKER_CONTEXT);

    Attributes.ParentObject = SessionContext->ServiceContext->ServiceDevice;

    WDF_WORKITEM_CONFIG_INIT(&WdfWorkItemConfig, GenServiceEvtRequestWorker);

    Status = WdfWorkItemCreate(&WdfWorkItemConfig, &Attributes, &WdfWorkItem);
    if (!NT_SUCCESS(Status)) {
        if (!NT_SUCCESS(Status)) {
            TraceError("WdfWorkItemCreate failed , %!STATUS!", Status);
            goto Exit;
        }
    }

    WorkerContext = TreeGenGetRequestWorkerContext(WdfWorkItem);
    WorkerContext->SessionContext = SessionContext;
    WorkerContext->RequestContextHandle = RequestContextHandle;
    WorkerContext->RequestKey = RequestKey;

    WdfWorkItemEnqueue(WdfWorkItem);

    *MiniportRequestContext = RequestContextHandle;
    Status = STATUS_PENDING;

Exit:

    if (!NT_SUCCESS(Status)) {
        if (RequestContext != NULL) {
            if (RequestContext->State == RequestStatePending) {
                ExAcquireFastMutex(&SessionContext->PendingRequestsLock);
                WdfCollectionRemove(SessionContext->PendingRequests,
                    RequestContextHandle);

                ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
            }
            WdfObjectDelete(RequestContextHandle);
        }

        if (WdfWorkItem != NULL) {
            WdfObjectDelete(WdfWorkItem);
        }
    }

    return Status;
}

_Use_decl_annotations_
static NTSTATUS
GenServiceSendRpcResponse(
    PTREE_GEN_SESSION_CONTEXT SessionContext,
    PTR_SERVICE_REQUEST Request,
    PVOID RequestHandle,
    PVOID* MiniportRequestContext
    )

/*++

Routine Description:

    This routine is called when to send a a response to a TEE RPC call (OCALL).
    The routine updates the request context with the new framework request 
    parameters, and  awakes the thread that is waiting for the RPC response.

Arguments:

    SessionContext - Supplies a handle to the service device object.

    Request - Supplies a pointer to the context.

    RequestHandle - The framework request handle.

    MiniportRequestContext - The miniport context that will be associated with
        the request.

Return Value:

    NTSTATUS code.

--*/

{

    SIZE_T InputDataBytes;
    PGENSVC_INPUT_BUFFER_HEADER InputHeader;
    PTREE_GEN_REQUEST_CONTEXT RequestContext;
    ULONG RequestKey;
    PVOID RpcResponseBuffer;
    SIZE_T RpcResponseBufferSize;
    NTSTATUS Status;

    *MiniportRequestContext = NULL;
    InputHeader = (PGENSVC_INPUT_BUFFER_HEADER)((PVOID)Request->InputBuffer);
    RequestKey = InputHeader->Key;
    InputDataBytes = (SIZE_T)(Request->InputBufferSize - 
        sizeof(GENSVC_INPUT_BUFFER_HEADER));

    ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

    RequestContext = GenServiceGetRequestContextByKey(SessionContext, 
        RequestKey);

    if (RequestContext == NULL) {
        TraceError("Key 0x%X, is NOT associated with a pending request!", RequestKey);
        Status = STATUS_INVALID_PARAMETER;
    } else {
        switch (RequestContext->State) {
        case RequestStateWaitRpcResponse:
            RequestContext->State = RequestStateRpcResponse;
            Status = STATUS_SUCCESS;
            break;

        case RequestStateCancelled:
            Status = STATUS_CANCELLED;
            break;

        default:
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        //
        // Update the new framework request parameters
        //

        NT_ASSERT(RequestContext->TreeRequestHandle == NULL);

        if (NT_SUCCESS(Status)) {
            RequestContext->TreeRequestHandle = RequestHandle;
            RequestContext->InputBuffer = Request->InputBuffer;
            RequestContext->InputBufferSize = (SIZE_T)Request->InputBufferSize;
            RequestContext->OutputBuffer = Request->OutputBuffer;
            RequestContext->OutputBufferSize = 
                (SIZE_T)Request->OutputBufferSize;

            //
            // Copy RPC response to RPC buffer
            //

            RpcResponseBuffer = RequestContext->RpcResponseBuffer;
            RequestContext->RpcResponseBuffer = NULL;
            RpcResponseBufferSize = RequestContext->RpcResponseBufferSize;
            RequestContext->RpcResponseBufferSize = 0;

            if (RpcResponseBuffer != NULL) {
                NT_ASSERT(RpcResponseBufferSize != 0);
                RpcResponseBufferSize = min(RpcResponseBufferSize, InputDataBytes);

                RtlCopyMemory(RpcResponseBuffer,
                    InputHeader + 1,
                    RpcResponseBufferSize);
            }
        }

        *MiniportRequestContext = RequestContext->Handle;

        //
        // Unblock the thread waiting for RPC response 
        //

        TraceDebug("RPC response request, key 0x%X, request handle 0x%p, "
            "request context 0x%p, state %d, status %!STATUS!",
            RequestKey,
            RequestHandle,
            RequestContext,
            RequestContext->State,
            Status);

        Status = KeSetEvent(&RequestContext->RpcCompletedEvent, 
            IO_NO_INCREMENT,
            FALSE);

        NT_ASSERT(NT_SUCCESS(Status));

        Status = STATUS_PENDING;
    }

    ExReleaseFastMutex(&SessionContext->PendingRequestsLock);

    return Status;
}

TEEC_Result
GenServiceReceiveRpcCommand(
    UINT32 SessionId,
    ULONG RpcKey,
    ULONG RpcType,
    _In_reads_bytes_(RpcInputBufferSize) PVOID RpcInputBuffer,
    SIZE_T RpcInputBufferSize,
    _Out_writes_bytes_(RpcOutputBufferSize) PVOID RpcOutputBuffer,
    SIZE_T RpcOutputBufferSize
    )

/*++

Routine Description:

    This routine is called by the RPC command handler to send an OCALL RPC to
    the Windows APP.
    The forwards the information through the Windows APP pending requests, and 
    waits for the APP to respond. If the process crashes, the service cleanup routine will
    abort the wait.

Arguments:

    SessionId - The ID of the TA session associated with the request.

    RpcKey - The ID of the originating request.

    RpcType - GenSvcOutputTypeRpcCommand.

    RpcInputBuffer - Input buffer that carries the command parameters.

    RpcInputBufferSize - Size of the command parameter in bytes.

    RpcOutputBuffer - Response buffer address, where the Windows APP fills 
        the response.

    RpcOutputBufferSize - Response buffer size

Return Value:

    TEEC_Result

--*/

{

    ULONG_PTR BytesWritten;
    PGENSVC_OUTPUT_BUFFER_HEADER RpcCommandHeader;
    PTREE_GEN_REQUEST_CONTEXT RequestContext;
    TEEC_Result Result;
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    NTSTATUS Status;

    TraceDebug("RPC command: session = 0x%llX, RpcKey = 0x%X, RpcType = 0x%X, "
        "RpcInputBufferSize = 0x%IX, RpcInputBuffer = 0x%p, "
        "RpcOutputBufferSize = 0x%IX, RpcOutputBuffer = 0x%p\n",
        SessionId,
        RpcKey,
        RpcType,
        RpcInputBufferSize,
        RpcInputBuffer,
        RpcOutputBufferSize,
        RpcOutputBuffer);

    SessionContext = GenServiceGetSessionContextById(SessionId);
    if (SessionContext == NULL) {
        TraceError("Session context not found for session 0x%llX", SessionId);
        return TEEC_ERROR_COMMUNICATION;
    }

    ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

    RequestContext = GenServiceGetRequestContextByKey(SessionContext, RpcKey);
    if (RequestContext == NULL) {
        TraceError("Request not found for session 0x%llX, key 0x%X", 
            SessionId, 
            RpcKey);

        ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
        return TEEC_ERROR_COMMUNICATION;
    }

    BytesWritten = 0;

    switch (RequestContext->State) {
    case RequestStateSubmitted:
        RequestContext->State = RequestStateWaitRpcResponse;
        Status = STATUS_SUCCESS;
        Result = TEEC_SUCCESS;
        break;

    case RequestStateCancelled:
        Status = STATUS_CANCELLED;
        Result = TEEC_ERROR_EXTERNAL_CANCEL;
        break;

    default:
        NT_ASSERT(FALSE);
        Status = STATUS_INVALID_DEVICE_STATE;
        Result = TEEC_ERROR_BAD_STATE;
        break;
    }

    if (NT_SUCCESS(Status)) {
        if (RpcInputBufferSize > OPTEE_MAXIMUM_RPC_INPUT_SIZE) {
            TraceError("Request buffer too large %lld, maximum %lld",
                RpcInputBufferSize, OPTEE_MAXIMUM_RPC_INPUT_SIZE);

            ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
            return TEEC_ERROR_EXCESS_DATA;
        }

        NT_ASSERT(RequestContext->TreeRequestHandle != NULL);

        NT_ASSERT(RequestContext->OutputBuffer != NULL);
        NT_ASSERT(RequestContext->OutputBufferSize != 0);

        NT_ASSERT(RequestContext->RpcResponseBuffer == NULL);
        NT_ASSERT(RequestContext->RpcResponseBufferSize == 0);

        RpcCommandHeader = (PGENSVC_OUTPUT_BUFFER_HEADER)RequestContext->OutputBuffer;

        RequestContext->RpcResponseBuffer = RpcOutputBuffer;
        RequestContext->RpcResponseBufferSize = RpcOutputBufferSize;

        RpcCommandHeader->Type = GenSvcOutputTypeRpcCommand;
        RpcCommandHeader->RpcType = RpcType;

        RtlCopyMemory(
            RpcCommandHeader + 1,
            RpcInputBuffer,
            RpcInputBufferSize);

        BytesWritten = sizeof(GENSVC_OUTPUT_BUFFER_HEADER) + RpcInputBufferSize;

        TraceDebug("RPC command: copying data size %d into request 0x%p, "
            "RPC command header = 0x%p, RpcKey = 0x%X\n",
            (ULONG)RpcInputBufferSize + sizeof(GENSVC_OUTPUT_BUFFER_HEADER),
            RequestContext->TreeRequestHandle,
            RpcCommandHeader,
            RpcKey);

    } else {
        TraceError("Request 0x%p (context 0x%p) failed, status %!STATUS!",
            RequestContext->TreeRequestHandle,
            RequestContext,
            Status);
    }

    ExReleaseFastMutex(&SessionContext->PendingRequestsLock);

    TraceDebug("RPC command: completing request 0x%p, size = %lld\n",
        RequestContext->TreeRequestHandle,
        BytesWritten);

    //
    // Complete the APP request, i.e. send the RPC command
    //

    GenServiceCompleteRequestAsync(RequestContext,
        Status,
        BytesWritten,
        RequestCompletePartial);

    if (NT_SUCCESS(Status)) {

        //
        // Wait for RPC response
        //

        TraceDebug("RPC command: thread 0x%p waiting for event 0x%p, "
            "RpcKey = 0x%X, RpcType = 0x%X\n",
            KeGetCurrentThread(),
            &RequestContext->RpcCompletedEvent,
            RpcKey,
            RpcType);

        Status = KeWaitForSingleObject(
            &RequestContext->RpcCompletedEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        NT_ASSERT(NT_SUCCESS(Status));

        ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

        switch (RequestContext->State) {
        case RequestStateRpcResponse:
            RequestContext->State = RequestStateSubmitted;
            Status = STATUS_SUCCESS;
            Result = TEEC_SUCCESS;
            break;

        case RequestStateCancelled:
            Status = STATUS_CANCELLED;
            Result = TEEC_ERROR_EXTERNAL_CANCEL;
            break;

        default:
            NT_ASSERT(FALSE);
            Status = STATUS_INVALID_DEVICE_STATE;
            Result = TEEC_ERROR_BAD_STATE;
            break;
        }

        TraceDebug("RPC command: got response for RpcKey 0x%X, RpcType 0x%X, "
            "Status %!STATUS!\n",
            RpcKey,
            RpcType,
            Status);

        ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
    }

    return Result;
}

_Use_decl_annotations_
static VOID
GenServiceCompleteRequestAsync(
    PTREE_GEN_REQUEST_CONTEXT RequestContext,
    NTSTATUS Status,
    ULONG_PTR BytesWritten,
    TREE_GEN_REQUEST_COMPLETION_TYPE CompletionType
    )

/*++

Routine Description:

    This routine is called to complete a framework pending request.

Arguments:

    RequestContext - The request context.
    
    Status - Completion status.

    BytesWritten - Number of bytes written to request output buffer.

    CompletionType - If this is the final or partial completion:
        - RequestCompletePartial: Request context is not freed, since it
            will be still be used with subsequent RPC-calls.
        - RequestCompleteFinal: This is the completion of the originating request,
            request context is removed from the pending list and freed. 

Return Value:


--*/

{
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    WDFMEMORY WdfRequestContext;
    PVOID TreeRequestHandle;

    SessionContext = RequestContext->SessionContext;

    ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

    TreeRequestHandle = RequestContext->TreeRequestHandle;
    WdfRequestContext = RequestContext->Handle;

    //
    // Clear current request parameters
    //

    RequestContext->TreeRequestHandle = NULL;
    RequestContext->OutputBuffer = NULL;
    RequestContext->OutputBufferSize = 0;
    RequestContext->InputBuffer = NULL;
    RequestContext->InputBufferSize = 0;

    switch (CompletionType) {
    case RequestCompletePartial:
        TraceInformation("Partial completing request for key 0x%X, "
            "handle 0x%p, status %!STATUS!", 
            RequestContext->Key,
            TreeRequestHandle,
            Status);

        ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
        break;

    case RequestCompleteFinal:
        TraceInformation("Final completing request for key 0x%X, "
            "handle 0x%p, status %!STATUS!",
            RequestContext->Key,
            TreeRequestHandle,
            Status);

        WdfCollectionRemove(SessionContext->PendingRequests,
            WdfRequestContext);

        ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
        
        WdfObjectDelete(WdfRequestContext);
        WdfObjectDereference(SessionContext->Handle);
        break;

    default:
        NT_ASSERT(FALSE);
        ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
        break;
    }

    if (TreeRequestHandle != NULL) {
        TrSecureDeviceCompleteAsyncRequest(TreeRequestHandle, Status, BytesWritten);
    }
}

_Use_decl_annotations_
static NTSTATUS
GenServiceAddSession(
    WDFOBJECT Session
    )

/*++

Routine Description:

    This routine is called to add a new session object to the list of opened sessions.

Arguments:

    Session - The session object

    RequestKey - The request key

Return Value:

    NTSTATUS

--*/

{

    NTSTATUS Status;

    NT_ASSERT(GenSessionContextList != NULL);

    ExAcquireFastMutex(&GenSessionContextListLock);

    Status = WdfCollectionAdd(GenSessionContextList, Session);
    if (!NT_SUCCESS(Status)) {
        TraceError("Failed to add session to sessions list, %!STATUS!", Status);
    }

    ExReleaseFastMutex(&GenSessionContextListLock);

    return Status;
}

_Use_decl_annotations_
static VOID
GenServiceRemoveSession(
    WDFOBJECT Session
    )

/*++

Routine Description:

    This routine is called to add a new session object to the list of opened sessions.

Arguments:

    Session - The session object

Return Value:

--*/

{

    WDFCOLLECTION PendingRequests;
    WDFMEMORY RequestObject;
    PTREE_GEN_SESSION_CONTEXT SessionContext;

    NT_ASSERT(GenSessionContextList != NULL);

    //
    // First remove session from active session list
    //

    ExAcquireFastMutex(&GenSessionContextListLock);
    WdfCollectionRemove(GenSessionContextList, Session);
    ExReleaseFastMutex(&GenSessionContextListLock);

    SessionContext = TreeGenGetSessionContext(Session);
    PendingRequests = SessionContext->PendingRequests;

    //
    // Clear the pending request list, and wakeup all pending threads, 
    // if any...
    //

    ExAcquireFastMutex(&SessionContext->PendingRequestsLock);

    while ((RequestObject = WdfCollectionGetFirstItem(PendingRequests)) !=
           NULL) {

        PTREE_GEN_REQUEST_CONTEXT RequestContext;

        WdfCollectionRemoveItem(PendingRequests, 0);

        RequestContext = (PTREE_GEN_REQUEST_CONTEXT)WdfMemoryGetBuffer(RequestObject, NULL);

        TraceWarning("Aborting request 0x%p, handle 0x%p, key 0x%x",
            RequestContext,
            RequestContext->TreeRequestHandle,
            RequestContext->Key);

        RequestContext->State = RequestStateCancelled;
        KeSetEvent(&RequestContext->RpcCompletedEvent, LOW_PRIORITY, FALSE);
    }

    ExReleaseFastMutex(&SessionContext->PendingRequestsLock);
}

_Use_decl_annotations_
PTREE_GEN_REQUEST_CONTEXT
GenServiceGetRequestContextByKey(
    PTREE_GEN_SESSION_CONTEXT SessionContext,
    ULONG RequestKey
    )

/*++

Routine Description:

    This routine is called to get a pending request context given the request
    key.

Arguments:

    SessionContext - Supplies a handle to the service device object.

    RequestKey - The request key

Return Value:

    The request context, or NULL if no pending request that matches the given
    key.

--*/

{

    WDFCOLLECTION PendingRequests;
    ULONG Index;
    PTREE_GEN_REQUEST_CONTEXT RequestContext;

    PendingRequests = SessionContext->PendingRequests;
    RequestContext = NULL;

    for (Index = 0;
         Index < WdfCollectionGetCount(PendingRequests);
         ++Index) {

        PTREE_GEN_REQUEST_CONTEXT CurRequestContext;

        CurRequestContext = (PTREE_GEN_REQUEST_CONTEXT)WdfMemoryGetBuffer(
            (WDFMEMORY)WdfCollectionGetItem(PendingRequests, Index), 
            NULL);

        if (CurRequestContext->Key == RequestKey) {
            RequestContext = CurRequestContext;
            break;
        }
    }

    return RequestContext;
}

_Use_decl_annotations_
static PTREE_GEN_REQUEST_CONTEXT
GenServiceGetRequestContextByHandle(
    PTREE_GEN_SESSION_CONTEXT SessionContext,
    WDFOBJECT WdfRequestContext
    )

/*++

Routine Description:

    This routine is called to get a pending request context given the request
    handle.

Arguments:

    SessionContext - Supplies a handle to the service device object.

    WdfRequestContext - The request handle

Return Value:

    The request context, or NULL if no pending request that matches the given
    handle.

--*/

{
    WDFCOLLECTION PendingRequests;
    ULONG Index;
    PTREE_GEN_REQUEST_CONTEXT RequestContext;

    PendingRequests = SessionContext->PendingRequests;
    RequestContext = NULL;

    for (Index = 0;
         Index < WdfCollectionGetCount(PendingRequests);
         ++Index) {

        WDFMEMORY CurWdfRequestContext;

        CurWdfRequestContext = (WDFMEMORY)WdfCollectionGetItem(PendingRequests, Index);
        if (CurWdfRequestContext == WdfRequestContext) {
            RequestContext = (PTREE_GEN_REQUEST_CONTEXT)
                WdfMemoryGetBuffer(CurWdfRequestContext, NULL);

            break;
        }
    }

    return RequestContext;
}

_Use_decl_annotations_
static PTREE_GEN_SESSION_CONTEXT
GenServiceGetSessionContextById(
    UINT32 SessionId
    )

/*++

Routine Description:

    This routine is called to get a session context, given the session ID.

Arguments:

    SessionId- The session ID

Return Value:

    The session context, or NULL if no active session ID matches the desired
    ID.

--*/

{
    PTREE_GEN_SESSION_CONTEXT SessionContext;
    ULONG Index;

    ExAcquireFastMutex(&GenSessionContextListLock);

    SessionContext = NULL;

    for (Index = 0;
         Index < WdfCollectionGetCount(GenSessionContextList);
         ++Index) {

        PTREE_GEN_SESSION_CONTEXT CurSessionContext;

        CurSessionContext = TreeGenGetSessionContext(
            (WDFOBJECT)WdfCollectionGetItem(GenSessionContextList, Index));

        if (CurSessionContext->ClientSession.session_id == SessionId) {
            SessionContext = CurSessionContext;
            break;
        }
    }

    ExReleaseFastMutex(&GenSessionContextListLock);

    return SessionContext;
}

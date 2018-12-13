/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <TrustedRuntimeClx.h>
#include <initguid.h>
#include <wdmguid.h>
#include "OpteeTrEE.h"
#include "OpteeTrEEService.h"
#include "OpteeClientMemory.h"
#include "OpteeClientLib\OpteeClientLib.h"
#include "trace.h"

#ifdef WPP_TRACING
#include "OPMService.tmh"
#endif


// OPM TA Command ID for RSA Decryption
#define TA_OPM_RSA_DECRYPT                  200

typedef struct _TREE_OPM_SERVICE_CONTEXT {
    POPTEE_TREE_DEVICE_CONTEXT DeviceContext;
    TEEC_Context TEECContext;
    WDFDEVICE ServiceDevice;
    ULONG SessionsOpened;
    ULONG SessionsClosed;
} TREE_OPM_SERVICE_CONTEXT, *PTREE_OPM_SERVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_OPM_SERVICE_CONTEXT);

typedef struct _TREE_OPM_SESSION_CONTEXT {
    PTREE_OPM_SERVICE_CONTEXT ServiceContext;
    TEEC_Session ClientSession;
} TREE_OPM_SESSION_CONTEXT, *PTREE_OPM_SESSION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_OPM_SESSION_CONTEXT);

EVT_TR_CREATE_SECURE_SERVICE_CONTEXT OPMServiceCreateSecureServiceContext;
EVT_TR_DESTROY_SECURE_SERVICE_CONTEXT OPMServiceDestroySecureServiceContext;
EVT_TR_CONNECT_SECURE_SERVICE OPMServiceConnectSecureService;
EVT_TR_DISCONNECT_SECURE_SERVICE OPMServiceDisconnectSecureService;
EVT_TR_CREATE_SECURE_SERVICE_SESSION_CONTEXT OPMServiceCreateSessionContext;
EVT_TR_DESTROY_SECURE_SERVICE_SESSION_CONTEXT OPMServiceDestroySessionContext;
EVT_TR_PROCESS_SECURE_SERVICE_REQUEST OPMServiceProcessSecureServiceRequest;
EVT_TR_CANCEL_SECURE_SERVICE_REQUEST OPMServiceCancelSecureServiceRequest;
EVT_TR_PROCESS_OTHER_SECURE_SERVICE_IO OPMServiceProcessOtherSecureServiceIo;

TR_SECURE_SERVICE_CALLBACKS OPM_SERVICE_CALLBACKS = {
    0,

    &OPMServiceCreateSecureServiceContext,
    &OPMServiceDestroySecureServiceContext,

    &OPMServiceConnectSecureService,
    &OPMServiceDisconnectSecureService,

    &OPMServiceCreateSessionContext,
    &OPMServiceDestroySessionContext,

    &OPMServiceProcessSecureServiceRequest,
    &OPMServiceCancelSecureServiceRequest,
    &OPMServiceProcessOtherSecureServiceIo
};

_Use_decl_annotations_
NTSTATUS
OPMServiceCreateSecureServiceContext(
    WDFDEVICE MasterDevice,
    LPCGUID ServiceGuid,
    WDFDEVICE ServiceDevice
    )

/*++

Routine Description:

    This rountine initializes and finalizes Secure Context by calling OpteeClient Library API

Arguments:

    ServiceDevice - Supplies a handle to the service device object.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    TEEC_Result TeecResult;
    PTREE_OPM_SERVICE_CONTEXT OPMServiceContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    UNREFERENCED_PARAMETER(ServiceGuid);

    TraceDebug("OPMServiceCreateSecureServiceContext : Calling TEEC_InitializeContext\n");

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
        TREE_OPM_SERVICE_CONTEXT);

    Status = WdfObjectAllocateContext(ServiceDevice,
        &ContextAttributes,
        (PVOID*)&OPMServiceContext);
    if (!NT_SUCCESS(Status)) {
        goto OPMServiceCreateSecureServiceContextEnd;
    }

    RtlZeroMemory(OPMServiceContext, sizeof(TREE_OPM_SERVICE_CONTEXT));
    OPMServiceContext->DeviceContext = TreeGetDeviceContext(MasterDevice);
    OPMServiceContext->ServiceDevice = ServiceDevice;

    Status = OpteeClientApiLibInitialize(NULL,
        ServiceDevice,
        OPMServiceContext->DeviceContext->SharedMemoryBaseAddress,
        OPMServiceContext->DeviceContext->SharedMemoryLength);

    if (!NT_SUCCESS(Status)) {
        goto OPMServiceCreateSecureServiceContextEnd;
    }

    TeecResult = TEEC_InitializeContext(NULL, &OPMServiceContext->TEECContext);
    if (TeecResult == TEEC_SUCCESS) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

OPMServiceCreateSecureServiceContextEnd:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
OPMServiceDestroySecureServiceContext(
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
    PTREE_OPM_SERVICE_CONTEXT ServiceContext;
    ServiceContext = WdfObjectGet_TREE_OPM_SERVICE_CONTEXT(ServiceDevice);

    OpteeClientApiLibDeinitialize(NULL, ServiceContext->ServiceDevice);

    TEEC_FinalizeContext(&ServiceContext->TEECContext);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
OPMServiceConnectSecureService(
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
OPMServiceDisconnectSecureService(
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
OPMServiceCreateSessionContext(
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
    PTREE_OPM_SESSION_CONTEXT SessionContext;
    PTREE_OPM_SERVICE_CONTEXT ServiceContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    WDFOBJECT NewContext;
    UINT32 ErrorOrigin;

    // OPM TA GUID: 6b51f84e-a93d-456c-ab0e-29ad8f264a47
    TEEC_UUID TeecUuid = { 
        SWAP_B32(0x6b51f84e), SWAP_B16(0xa93d), SWAP_B16(0x456c),
        { 0xab, 0x0e, 0x29, 0xad, 0x8f, 0x26, 0x4a, 0x47 } 
    };

    ServiceContext = WdfObjectGet_TREE_OPM_SERVICE_CONTEXT(ServiceDevice);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes, TREE_OPM_SESSION_CONTEXT);

    Status = WdfObjectCreate(&ContextAttributes, &NewContext);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    SessionContext = WdfObjectGet_TREE_OPM_SESSION_CONTEXT(NewContext);
    SessionContext->ServiceContext = ServiceContext;

    TeecResult = TEEC_OpenSession(&ServiceContext->TEECContext,
        &SessionContext->ClientSession,
        &TeecUuid,
        TEEC_LOGIN_PUBLIC,
        NULL,
        NULL,
        &ErrorOrigin);
    if (TeecResult != TEEC_SUCCESS) {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    *SessionContextObject = NewContext;
    Status = STATUS_SUCCESS;

Exit:
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
OPMServiceDestroySessionContext(
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
    PTREE_OPM_SESSION_CONTEXT SessionContext;
    if (*SessionContextObject != NULL) {
        SessionContext = WdfObjectGet_TREE_OPM_SESSION_CONTEXT(*SessionContextObject);
        TEEC_CloseSession(&SessionContext->ClientSession);
        WdfObjectDelete(*SessionContextObject);
    }
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
OPMServiceProcessSecureServiceRequest(
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

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(RequestContext);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID
OPMServiceCancelSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    PVOID* RequestContext
    )

/*++

Routine Description:

    This routine is called to cancel a request made via a previous call to
    OPMServiceProcessSecureServiceRequest. Note that cancellation is
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

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(RequestContext);
}

_Use_decl_annotations_
VOID
OPMServiceProcessOtherSecureServiceIo(
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

    NTSTATUS Status;
    TEEC_Result TeecResult;
    PTREE_OPM_SESSION_CONTEXT SessionContext;
    TEEC_Operation TeecOperation;
    uint32_t ErrorOrigin;
    WDF_REQUEST_PARAMETERS Parameters;
    PVOID InputBuffer;
    UINTN InputBufferLength;
    PUCHAR OutputBuffer;
    UINTN OutputBufferLength;
    UINTN OpteeInputBuffer;
    PHYSICAL_ADDRESS OpteeInputBufferPhysical;
    UINTN OpteeOutputBuffer;
    PHYSICAL_ADDRESS OpteeOutputBufferPhysical;
    UINT32 BytesWritten;

    Status = STATUS_SUCCESS;
    TeecResult = TEEC_SUCCESS;
    OpteeInputBuffer = 0;
    OpteeOutputBuffer = 0;
    BytesWritten = 0;

    SessionContext = WdfObjectGet_TREE_OPM_SESSION_CONTEXT(SessionContextObject);

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);

    // Even though the OPM TA supports many operations, only RSA Decrypt
    // is called from Windows (all other operations are called from UEFI)
    switch (Parameters.Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_OPM_RSA_DECRYPT:
        Status = WdfRequestRetrieveInputBuffer(Request, 1, &InputBuffer, &InputBufferLength);
        if (!NT_SUCCESS(Status)) {
            ASSERT(FALSE);
            break;
        }

        Status = WdfRequestRetrieveOutputBuffer(Request, 1, &OutputBuffer, &OutputBufferLength);
        if (!NT_SUCCESS(Status)) {
            ASSERT(FALSE);
            break;
        }

        // Allocate OPTEE shared memory buffers and copy the OS input buffer into the OPTEE input buffer
        Status = OpteeClientMemAlloc((UINT32)(InputBufferLength), (PVOID*)&OpteeInputBuffer, &OpteeInputBufferPhysical);
        if (!NT_SUCCESS(Status)) {
            ASSERT(FALSE);
            break;
        }

        // The first 4 bytes of the OS output buffer are used to store the number of bytes decrypted. The remainder
        // will store the decrypted data. Therefore, the OS output buffer is length OutputBufferLength, and the OPTEE output
        // buffer is length OutputBufferLength - sizeof(ULONG).
        Status = OpteeClientMemAlloc((UINT32)(OutputBufferLength - sizeof(ULONG)), (PVOID*)&OpteeOutputBuffer, &OpteeOutputBufferPhysical);
        if (!NT_SUCCESS(Status)) {
            ASSERT(FALSE);
            break;
        }

        memcpy_s((PVOID)OpteeInputBuffer, InputBufferLength, InputBuffer, InputBufferLength);

        // Construct OPTEE operation parameters and call InvokeCommand
        memset(&TeecOperation, 0, sizeof(TEEC_Operation));
        TeecOperation.params[0].tmpref.buffer = (PVOID)OpteeInputBufferPhysical.LowPart;
        TeecOperation.params[0].tmpref.size = InputBufferLength;
        TeecOperation.params[1].tmpref.buffer = (PVOID)OpteeOutputBufferPhysical.LowPart;
        TeecOperation.params[1].tmpref.size = OutputBufferLength - sizeof(ULONG);

        TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
            TEEC_MEMREF_TEMP_OUTPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE);

        TeecResult = TEEC_InvokeCommand(
            &SessionContext->ClientSession,
            TA_OPM_RSA_DECRYPT,
            &TeecOperation,
            &ErrorOrigin);

        if (TeecResult != TEEC_SUCCESS) {
            ASSERT(FALSE);
            break;
        }

        BytesWritten = (UINT32)TeecOperation.params[2].value.a;

        if (BytesWritten == 0 || BytesWritten > OutputBufferLength - sizeof(ULONG)) {
            ASSERT(FALSE);
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        // Copy the OPTEE output buffer into the OS output buffer. Also return the number of decrypted bytes.
        *((PULONG)OutputBuffer) = BytesWritten;
        memcpy_s(OutputBuffer + sizeof(ULONG), OutputBufferLength - sizeof(ULONG), (PVOID)OpteeOutputBuffer, BytesWritten);
        BytesWritten += sizeof(ULONG);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (OpteeInputBuffer != 0) {
        OpteeClientMemFree((PVOID)OpteeInputBuffer);
    }

    if (OpteeOutputBuffer != 0) {
        OpteeClientMemFree((PVOID)OpteeOutputBuffer);
    }

    TraceDebug("Returning Status: 0x%x, BytesWritten: %d\n", 
        Status, 
        BytesWritten);

    WdfRequestCompleteWithInformation(Request, Status, BytesWritten);
}

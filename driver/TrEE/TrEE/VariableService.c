/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <ntddk.h>
#include <wdf.h>
#include "inc\efidef.h"
#include <ntstrsafe.h>
#include <TrustedRuntimeClx.h>
#include <initguid.h>
#include <TrEEVariableService.h>
#include <strsafe.h>
#include "OpteeTrEE.h"
#include "OpteeTrEEService.h"
#include "OpteeClientLib\OpteeClientLib.h"
#include "trace.h"
#include "OpteeVariableService.h"
#include "OpteeClientLib\OpteeClientMemory.h"

#ifdef WPP_TRACING
#include "VariableService.tmh"
#endif


//
// Variable Service TA GUID.
//
#define TA_AUTH_VAR_UUID { \
    SWAP_B32(0x2d57c0f7), SWAP_B16(0xbddf), SWAP_B16(0x48ea), \
    { 0x83, 0x2f, 0xd8, 0x4a, 0x1a, 0x21, 0x93, 0x01 } \
}

typedef struct _TREE_TEST_SESSION_CONTEXT {
    TEEC_Session ClientSession;
} TREE_VARIABLE_SESSION_CONTEXT, *PTREE_VARIABLE_SESSION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_VARIABLE_SESSION_CONTEXT);

typedef struct _TREE_VARIABLE_SERVICE_CONTEXT {
    POPTEE_TREE_DEVICE_CONTEXT DeviceContext;
    TEEC_Context TEECContext;
    WDFOBJECT SessionObject;
    WDFDEVICE ServiceDevice;
    ULONG SessionsOpened;
    ULONG SessionsClosed;
} TREE_VARIABLE_SERVICE_CONTEXT, *PTREE_VARIABLE_SERVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_VARIABLE_SERVICE_CONTEXT);

// 
// Define a single union of all the outbound parameter structures.
// Some are variable sized and thus overlay a single allocated buffer.
// 
typedef union _VARIABLE_PARAM {

    VARIABLE_GET_PARAM       GetParam;
    VARIABLE_GET_NEXT_PARAM  GetNextParam;
    VARIABLE_SET_PARAM       SetParam;
    VARIABLE_QUERY_PARAM     QueryParam;

} VARIABLE_PARAM, *PVARIABLE_PARAM;

// 
// Define a single union of all the inbound parameter structures.
// Some are variable sized and thus overlay a single allocated buffer.
// 
typedef union _VARIABLE_RESULT {

    VARIABLE_GET_RESULT       GetResult;
    VARIABLE_GET_NEXT_RESULT  GetNextResult;
    //  No VARIABLE_SET_RESULT since no data is returned
    VARIABLE_QUERY_RESULT     QueryResult;

} VARIABLE_RESULT, *PVARIABLE_RESULT;

//
// OP-TEE Auth. Var. TA transport buffers.
// These must be allocated using the OP-TEE client API set.
//
TEEC_SharedMemory  mVariableParamMem;
TEEC_SharedMemory  mVariableResultMem;

//
// Variable sizes as per UEFI
// PCD_MAX_VARIABLE_SIZE : from PcdMaxVariableSize in DescriptionFile.inc
// PCD_MAX_HARDWARE_ERROR_VARIABLE_SIZE: from PcdMaxHardwareErrorVariableSize in MdeModulePkg.dec and OvmfPkgIa32.dsc
//
#define PDC_MAX_VARIABLE_SIZE 0x2000
#define PCD_MAX_HARDWARE_ERROR_VARIABLE_SIZE 0x8000


EVT_TR_CREATE_SECURE_SERVICE_CONTEXT VariableServiceCreateSecureServiceContext;
EVT_TR_DESTROY_SECURE_SERVICE_CONTEXT VariableServiceDestroySecureServiceContext;
EVT_TR_CONNECT_SECURE_SERVICE VariableServiceConnectSecureService;
EVT_TR_DISCONNECT_SECURE_SERVICE VariableServiceDisconnectSecureService;
EVT_TR_CREATE_SECURE_SERVICE_SESSION_CONTEXT VariableServiceCreateSessionContext;
EVT_TR_DESTROY_SECURE_SERVICE_SESSION_CONTEXT VariableServiceDestroySessionContext;
EVT_TR_PROCESS_SECURE_SERVICE_REQUEST VariableServiceProcessSecureServiceRequest;
EVT_TR_CANCEL_SECURE_SERVICE_REQUEST VariableServiceCancelSecureServiceRequest;
EVT_TR_PROCESS_OTHER_SECURE_SERVICE_IO VariableServiceProcessOtherSecureServiceIo;

TR_SECURE_SERVICE_CALLBACKS VARIABLE_SERVICE_CALLBACKS = {
    0,

    &VariableServiceCreateSecureServiceContext,
    &VariableServiceDestroySecureServiceContext,

    &VariableServiceConnectSecureService,
    &VariableServiceDisconnectSecureService,

    &VariableServiceCreateSessionContext,
    &VariableServiceDestroySessionContext,

    &VariableServiceProcessSecureServiceRequest,
    &VariableServiceCancelSecureServiceRequest,
    &VariableServiceProcessOtherSecureServiceIo
};

typedef struct _TEST_VARIABLE_ENTRY {
    PWSTR Name;
    ULONG_PTR ValueSize;
    PVOID Value;
    BOOLEAN Allocated;
    GUID VendorGuid;
    struct _TEST_VARIABLE_ENTRY *Next;
} TEST_VARIABLE_ENTRY, *PTEST_VARIABLE_ENTRY;

#define TEST_VARIABLE_DATA(_Data) sizeof(_Data), _Data

_Use_decl_annotations_
NTSTATUS
VariableServiceCreateSecureServiceContext(
    WDFDEVICE MasterDevice,
    LPCGUID ServiceGuid,
    WDFDEVICE ServiceDevice
    )
{
    NTSTATUS Status;
    TEEC_Result TeecResult;
    PTREE_VARIABLE_SERVICE_CONTEXT VariableServiceContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    UNREFERENCED_PARAMETER(ServiceGuid);
    
    TraceDebug("TestServiceCreateSecureServiceContext : "
        "Calling TEEC_InitializeContext\n");

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
        TREE_VARIABLE_SERVICE_CONTEXT);

    Status = WdfObjectAllocateContext(ServiceDevice,
        &ContextAttributes,
        (PVOID*)&VariableServiceContext);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    RtlZeroMemory(VariableServiceContext, sizeof(TREE_VARIABLE_SERVICE_CONTEXT));
    VariableServiceContext->DeviceContext = TreeGetDeviceContext(MasterDevice);
    VariableServiceContext->ServiceDevice = ServiceDevice;

    Status = OpteeClientApiLibInitialize(NULL,
        ServiceDevice,
        VariableServiceContext->DeviceContext->SharedMemoryBaseAddress,
        VariableServiceContext->DeviceContext->SharedMemoryLength);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    TeecResult = TEEC_InitializeContext(NULL, &VariableServiceContext->TEECContext);
    if (TeecResult != TEEC_SUCCESS) {
        return NtstatusFromTeecResult(TeecResult);
    }

    //
    // Allocate the transport buffers. We don't have to worry about alignment because the
    // OP-TEE Client API will align the buffers for us.
    //
    ASSERT(mVariableParamMem.buffer == NULL);
    RtlZeroMemory(&mVariableParamMem, sizeof(mVariableParamMem));
    mVariableParamMem.size = max(PDC_MAX_VARIABLE_SIZE, PCD_MAX_HARDWARE_ERROR_VARIABLE_SIZE) +
        sizeof(VARIABLE_PARAM);

    ASSERT(mVariableResultMem.buffer == NULL);
    RtlZeroMemory(&mVariableResultMem, sizeof(mVariableResultMem));
    mVariableResultMem.size = mVariableParamMem.size;

    TeecResult = TEEC_AllocateSharedMemory(&VariableServiceContext->TEECContext, &mVariableParamMem);
    if (TeecResult != TEEC_SUCCESS) {
        return NtstatusFromTeecResult(TeecResult);
    }

    TeecResult = TEEC_AllocateSharedMemory(&VariableServiceContext->TEECContext, &mVariableResultMem);
    if (TeecResult != TEEC_SUCCESS) {
        return NtstatusFromTeecResult(TeecResult);
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
VariableServiceDestroySecureServiceContext(
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

    PTREE_VARIABLE_SERVICE_CONTEXT ServiceContext;
    ServiceContext = WdfObjectGet_TREE_VARIABLE_SERVICE_CONTEXT(ServiceDevice);

    //
    // Do the finalization in the reverse order of the initialization.
    //

    if (mVariableParamMem.buffer != NULL) {
        TEEC_ReleaseSharedMemory(&mVariableParamMem);
    }
    
    if (mVariableResultMem.buffer != NULL) {
        TEEC_ReleaseSharedMemory(&mVariableResultMem);
    }

    TEEC_FinalizeContext(&ServiceContext->TEECContext);

    OpteeClientApiLibDeinitialize(NULL, ServiceContext->ServiceDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
VariableServiceConnectSecureService(
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
VariableServiceDisconnectSecureService(
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
VariableServiceCreateSessionContext(
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
    PTREE_VARIABLE_SERVICE_CONTEXT ServiceContext;
    TEEC_Result TeecResult;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    PTREE_VARIABLE_SESSION_CONTEXT SessionContext;
    WDFOBJECT NewSession;
    UINT32 ErrorOrigin;
    const TEEC_UUID TeecUuid = TA_AUTH_VAR_UUID;

    ServiceContext = WdfObjectGet_TREE_VARIABLE_SERVICE_CONTEXT(ServiceDevice);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes, TREE_VARIABLE_SESSION_CONTEXT);
    ContextAttributes.ParentObject = ServiceDevice;

    Status = WdfObjectCreate(&ContextAttributes, &NewSession);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    SessionContext = WdfObjectGet_TREE_VARIABLE_SESSION_CONTEXT(NewSession);

    TeecResult = TEEC_OpenSession(&ServiceContext->TEECContext,
                                  &SessionContext->ClientSession,
                                  &TeecUuid,
                                  TEEC_LOGIN_PUBLIC,
                                  NULL,
                                  NULL,
                                  &ErrorOrigin);

    if (TeecResult != TEEC_SUCCESS) {
        return NtstatusFromTeecResult(TeecResult);
    }

    ServiceContext->SessionObject = NewSession;
    *SessionContextObject = NewSession;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
VariableServiceDestroySessionContext(
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

    if (*SessionContextObject != NULL) {
        PTREE_VARIABLE_SESSION_CONTEXT SessionContext =
            WdfObjectGet_TREE_VARIABLE_SESSION_CONTEXT(*SessionContextObject);

        TEEC_CloseSession(&SessionContext->ClientSession);
        WdfObjectDelete(*SessionContextObject);
        *SessionContextObject = NULL;
    }

    return STATUS_SUCCESS;
}

typedef
NTSTATUS
TREE_VARIABLE_SERVICE_REQUEST_HANDLER(
    _In_ TEEC_Session* Session,
    _In_ PVOID InputBuffer,
    _In_ ULONG64 InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG64 OutputBufferLength,
    _Out_ PULONG_PTR BytesWritten
    );

TREE_VARIABLE_SERVICE_REQUEST_HANDLER VariableServiceGetVariable;
TREE_VARIABLE_SERVICE_REQUEST_HANDLER VariableServiceGetNextVariableName;
TREE_VARIABLE_SERVICE_REQUEST_HANDLER VariableServiceSetVariable;
TREE_VARIABLE_SERVICE_REQUEST_HANDLER VariableServiceQueryInformation;

TREE_VARIABLE_SERVICE_REQUEST_HANDLER *VariableServiceDispatch[] = {
    VariableServiceGetVariable,
    VariableServiceGetNextVariableName,
    VariableServiceSetVariable,
    VariableServiceQueryInformation
};

_Use_decl_annotations_
NTSTATUS
VariableServiceProcessSecureServiceRequest(
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
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);
    UNREFERENCED_PARAMETER(SessionContextObject);

    PTREE_VARIABLE_SERVICE_CONTEXT ServiceContext = NULL;
    PTREE_VARIABLE_SESSION_CONTEXT SessionContext = NULL;

    if (ServiceDevice != NULL) {
        ServiceContext = WdfObjectGet_TREE_VARIABLE_SERVICE_CONTEXT(ServiceDevice);
        if (ServiceContext == NULL){
            return STATUS_DEVICE_NOT_READY;
        }
        SessionContext = WdfObjectGet_TREE_VARIABLE_SESSION_CONTEXT(ServiceContext->SessionObject);
    }

    if (Request->FunctionCode >= sizeof(VariableServiceDispatch) / sizeof(VariableServiceDispatch[0])) {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesWritten = 0;
    return (*VariableServiceDispatch[Request->FunctionCode])(
        &(SessionContext->ClientSession),
        Request->InputBuffer,
        Request->InputBufferSize,
        Request->OutputBuffer,
        Request->OutputBufferSize,
        BytesWritten);
}

_Use_decl_annotations_
VOID
VariableServiceCancelSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    PVOID* RequestContext
    )

/*++

Routine Description:

    This routine is called to cancel a request made via a previous call to
    VariableServiceProcessSecureServiceRequest. Note that cancellation is
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
VariableServiceProcessOtherSecureServiceIo(
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

EFI_STATUS
OpteeRuntimeVariableInvokeCommand(
    _In_ TEEC_Session* Session,
    _In_ VARIABLE_SERVICE_OPS Command,
    _In_ UINT32 VariableParamSize,
    _In_ TEEC_SharedMemory *VariableParam,
    _In_ UINT32 VariableResultSize,
    _In_ TEEC_SharedMemory *VariableResult,
    _Out_ UINT32 *ResultSize,
    _Out_ UINT32 *AuthVarStatus
    )

/*++

Routine Description:

    Populate a GP API parameter set and invoke a command on the Auth. Var. TA.

Arguments:

    Command            The command to invoke.
    VariableParamSize  Size of the parameter buffer.
    VariableParam      GP buffer describing the variable parameters.
    VariableResultSize Size of the result buffer.
    VariableResult     GP buffer describing the variable results.
    ResultSize         The actual size of the data in the result buffer.
    AuthVarStatus      The actual OP-TEE status result of the executed command.

Return Value:

    EFI_STATUS code:
    - EFI_SUCCESS                Found the specified variable.
    - EFI_NOT_FOUND              Variable not found.
    - EFI_BUFFER_TO_SMALL        DataSize is too small for the result.

--*/
{
    EFI_STATUS     Status = EFI_SUCCESS;
    TEEC_Operation TeecOperation = { 0 };
    TEEC_Result    TeecResult = TEEC_SUCCESS;
    uint32_t       ErrorOrigin = 0;

    TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
        TEEC_MEMREF_TEMP_OUTPUT,
        TEEC_VALUE_OUTPUT,
        TEEC_NONE);

    if (VariableParam != NULL) {

        TeecOperation.params[0].tmpref.buffer = (PVOID)(UINTN)OpteeClientVirtualToPhysical((PVOID)((UINTN)VariableParam->buffer));
        TeecOperation.params[0].tmpref.size = VariableParamSize;
    }

    if (VariableResult != NULL) {

        TeecOperation.params[1].tmpref.buffer = (PVOID)(UINTN)OpteeClientVirtualToPhysical((PVOID)((UINTN)VariableResult->buffer));
        TeecOperation.params[1].tmpref.size = VariableResultSize;
    }

    TeecResult = TEEC_InvokeCommand(Session,
        Command,
        &TeecOperation,
        &ErrorOrigin);

    *ResultSize = (UINT32)TeecOperation.params[2].value.a;
    *AuthVarStatus = (UINT32)TeecOperation.params[2].value.b;

    if (TeecResult != TEEC_SUCCESS) {

        //
        // We need to translate some errors into the EFI equivalents.
        //
        switch (TeecResult) {

        case TEEC_ERROR_ITEM_NOT_FOUND:
            Status = EFI_NOT_FOUND;
            break;

        case TEEC_ERROR_SHORT_BUFFER:
            Status = EFI_BUFFER_TOO_SMALL;
            break;

        default:
            Status = EFI_DEVICE_ERROR;
            TraceError("TEEC_InvokeCommand Failed : TeecResult=0x%X, ErrorOrigin=%d\n",
                TeecResult, ErrorOrigin);
        }

        goto Exit;
    }

Exit:
    return Status;
}

NTSTATUS
VariableServiceGetVariable(
    _In_ TEEC_Session* Session,
    _In_ PVOID InputBuffer,
    _In_ ULONG64 InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG64 OutputBufferLength,
    _Out_ PULONG_PTR BytesWritten
    )

{
    PEFI_GET_VARIABLE_IN Input = NULL;
    PEFI_GET_VARIABLE_OUT Output = NULL;
    EFI_STATUS EFIStatus = EFI_NOT_FOUND;
    NTSTATUS Status = STATUS_SUCCESS;
    UINTN LocalVariableNameSize;

    // testing 
    CHAR variableName[STRING_BUFFER_SIZE];

    if (InputBuffer == NULL){
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Input = (PEFI_GET_VARIABLE_IN)InputBuffer;
    Output = (PEFI_GET_VARIABLE_OUT)OutputBuffer;

    if (InputBufferLength < FIELD_OFFSET(EFI_GET_VARIABLE_IN, VariableName) ||
        Input->VariableName == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // This function is expected to return the min bytes written.
    //
    *BytesWritten = FIELD_OFFSET(EFI_GET_VARIABLE_OUT, Data);
    if (OutputBufferLength < FIELD_OFFSET(EFI_GET_VARIABLE_OUT, Data)) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    WCSTOMBS(variableName, Input->VariableName);
    TraceDebug("GET Variable %s \n", variableName);

    //
    // Security: make sure external pointer values are copied locally to prevent
    // concurrent modification after validation.
    //    
    LocalVariableNameSize = (ULONG64)STR_SIZE(Input->VariableName);

    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    PVARIABLE_RESULT VariableResult = (PVARIABLE_RESULT)mVariableResultMem.buffer;
    UINT32 VariableParamSize;
    UINT32 VariableResultSize;
    UINT32 ResultSize = 0;
    UINT32 AuthVarStatus = 0;

    //
    // Validate that the buffers will fit.
    // The first test validates that the variable fields are sensible
    // to start with and catches the case that they are some huge number
    // that will overflow when added to.
    //
    if ((LocalVariableNameSize >= mVariableParamMem.size) ||
        (OutputBufferLength >= mVariableResultMem.size)) {
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }

    //
    // A note about sizes. The correct calculations should be of the form:
    //   OFFSET_OF(VARIABLE_GET_PARAM, VariableName)
    //   OFFSET_OF(VARIABLE_GET_RESULT, Data)
    // However the Auth. Var. TA is coded based on sizeof() that is thus
    // including the first byte of the variable field and it's alignment.
    // The net effect is that the size calculation is too long by 4 bytes.
    // This doesn't matter in practice since the extra 4 bytes is not used.
    //

    VariableParamSize = (UINT32)(sizeof(VARIABLE_GET_PARAM) + LocalVariableNameSize);

    VariableResultSize = (UINT32)(sizeof(VARIABLE_GET_RESULT) +
        OutputBufferLength -
        FIELD_OFFSET(EFI_GET_VARIABLE_OUT, Data));


    //
    // The second test validates that the data will actually fit in the buffer.
    //
    if ((VariableParamSize > mVariableParamMem.size) ||
        (VariableResultSize > mVariableResultMem.size)) {
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }

    //
    // Clear the buffers as much as its needed
    //
    RtlZeroMemory(mVariableParamMem.buffer, (size_t)InputBufferLength);
    RtlZeroMemory(mVariableResultMem.buffer, (size_t)OutputBufferLength);


    //
    // Fill in the parameter fields.
    //
    VariableParam->GetParam.Size = sizeof(VARIABLE_GET_PARAM);
    VariableParam->GetParam.VariableNameSize = (UINT16)LocalVariableNameSize;

    RtlCopyMemory(&VariableParam->GetParam.VendorGuid,
        &Input->VendorGuid, sizeof(VariableParam->GetParam.VendorGuid));
    RtlCopyMemory(&VariableParam->GetParam.VariableName,
        &Input->VariableName, LocalVariableNameSize);

    EFIStatus = OpteeRuntimeVariableInvokeCommand(Session,
        VSGetOp,
        VariableParamSize,
        &mVariableParamMem,
        VariableResultSize,
        &mVariableResultMem,
        &ResultSize,
        &AuthVarStatus);

    TraceDebug("    EFI Status: 0x%IX\n", EFIStatus);

    if (EFIStatus == EFI_BUFFER_TOO_SMALL) {

        //
        // In this case the Auth. Var. TA returns the size of the *buffer* required
        // so in order to turn this into the size of the *data* required we need
        // to subtract header.
        //
        ASSERT(ResultSize >= sizeof(VARIABLE_GET_RESULT));
        *BytesWritten += ResultSize - sizeof(VARIABLE_GET_RESULT);
        Output->DataSize = ResultSize - sizeof(VARIABLE_GET_RESULT);
        goto Exit;

    } else if (EFIStatus == EFI_SUCCESS) {
        *BytesWritten += VariableResult->GetResult.DataSize;
    } else if (EFI_ERROR(EFIStatus)) {
        goto Exit;
    }

    //
    // Shouldn't happen, but...
    //
    if (VariableResult->GetResult.DataSize > OutputBufferLength) {

        TraceError("    Data buffer overflow, result DataSize=0x%IX\n",
                   VariableResult->GetResult.DataSize);

        //Status = STATUS_INVALID_BUFFER_SIZE;
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        goto Exit;
    }

    Output->Attributes = VariableResult->GetResult.Attributes;
    RtlCopyMemory(Output->Data, VariableResult->GetResult.Data, VariableResult->GetResult.DataSize);
    Output->DataSize = VariableResult->GetResult.DataSize;
    TraceDebug("    Output Size 0x%IX\n", Output->DataSize);

Exit:
    Output->EfiStatus = EFIStatus;
    return Status;
}

NTSTATUS
VariableServiceGetNextVariableName(
    _In_ TEEC_Session* Session,
    _In_ PVOID InputBuffer,
    _In_ ULONG64 InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG64 OutputBufferLength,
    _Out_ PULONG_PTR BytesWritten
    )

{

    PEFI_GET_NEXT_VARIABLE_NAME_IN Input = NULL;
    PEFI_GET_NEXT_VARIABLE_NAME_OUT Output = NULL;
    EFI_STATUS EFIStatus = EFI_NOT_FOUND;
    NTSTATUS Status = STATUS_SUCCESS;
    UINTN LocalVariableNameSize;

    // testing.
    CHAR variableName[STRING_BUFFER_SIZE];

    if (InputBuffer == NULL){
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Input = (PEFI_GET_NEXT_VARIABLE_NAME_IN)InputBuffer;
    Output = (PEFI_GET_NEXT_VARIABLE_NAME_OUT)OutputBuffer;

    if (InputBufferLength < FIELD_OFFSET(EFI_GET_NEXT_VARIABLE_NAME_IN, VariableName) ||
        Input->VariableName == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // The function is expected to return the minimum bytes.
    //
    *BytesWritten = FIELD_OFFSET(EFI_GET_NEXT_VARIABLE_NAME_OUT, VariableName);
    if (OutputBufferLength < FIELD_OFFSET(EFI_GET_NEXT_VARIABLE_NAME_OUT, VariableName)) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    //
    // Security: make sure external pointer values are copied locally to prevent
    // concurrent modification after validation.
    //
    // The Auth. Var. TA expects that input name size == 0 as the indicator that this
    // is the initial query for the first name that exists. Since the StrSize above
    // includes the NULL terminator we need to handle this case specially.
    //
    LocalVariableNameSize = (ULONG64)STR_SIZE(Input->VariableName);

    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    PVARIABLE_RESULT VariableResult = (PVARIABLE_RESULT)mVariableResultMem.buffer;
    UINT32 VariableParamSize;
    UINT32 VariableResultSize;
    UINT32 ResultSize = 0;
    UINT32 AuthVarStatus = 0;

    //
    // Validate that the buffers will fit.
    // The first test validates that the variable fields are sensible
    // to start with and catches the case that they are some huge number
    // that will overflow when added to.
    //
    if ((InputBufferLength >= mVariableParamMem.size) ||
        (OutputBufferLength >= mVariableResultMem.size)) {
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }
    //
    // A note about sizes. The correct calculations should be of the form:
    //   OFFSET_OF(VARIABLE_GET_NEXT_PARAM,  VariableName)
    //   OFFSET_OF(VARIABLE_GET_NEXT_RESULT, VariableName)
    // However the Auth. Var. TA is coded based on sizeof() that is thus
    // including the first byte of the variable field and it's alignment.
    // The net effect is that the size calculation is too long by 4 bytes.
    // This doesn't matter in practice since the extra 4 bytes is not used.
    //
    VariableParamSize = (UINT32)(sizeof(VARIABLE_GET_NEXT_PARAM) + LocalVariableNameSize);

    VariableResultSize = (UINT32)(sizeof(VARIABLE_GET_NEXT_RESULT) +
        OutputBufferLength -
        FIELD_OFFSET(EFI_GET_NEXT_VARIABLE_NAME_OUT, VariableName));


    //
    // The second test validates that the data will actually fit in the buffer.
    //
    if ((VariableParamSize > mVariableParamMem.size) ||
        (VariableResultSize > mVariableResultMem.size)) {
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }

    //
    // Clear the buffers as much as its needed
    //
    RtlZeroMemory(mVariableParamMem.buffer, mVariableParamMem.size);
    RtlZeroMemory(mVariableResultMem.buffer, mVariableResultMem.size);

    //
    // Fill in the parameter fields.
    //
    VariableParam->GetNextParam.Size = sizeof(VARIABLE_GET_NEXT_PARAM);
    VariableParam->GetNextParam.VariableNameSize = (UINT16)LocalVariableNameSize;

    RtlCopyMemory(&VariableParam->GetNextParam.VendorGuid,
        &Input->VendorGuid, sizeof(VariableParam->GetParam.VendorGuid));
    RtlCopyMemory(&VariableParam->GetNextParam.VariableName,
        &Input->VariableName, LocalVariableNameSize);

    EFIStatus = OpteeRuntimeVariableInvokeCommand(Session,
        VSGetNextVarOp,
        VariableParamSize,
        &mVariableParamMem,
        VariableResultSize,
        &mVariableResultMem,
        &ResultSize,
        &AuthVarStatus);

    WCSTOMBS(variableName, VariableResult->GetNextResult.VariableName);
    TraceDebug("GET NEXT Variable %s\n", variableName);
    TraceDebug("    EFI Status: 0x%IX\n", EFIStatus);

    if (EFIStatus == EFI_BUFFER_TOO_SMALL) {

        //
        // In this case the Auth. Var. TA returns the size of the *buffer* required
        // so in order to turn this into the size of the *name* required we need
        // to subtract header.
        //

        ASSERT(ResultSize >= sizeof(VARIABLE_GET_NEXT_RESULT));
        *BytesWritten += ResultSize - sizeof(VARIABLE_GET_NEXT_RESULT);
        Output->NameLength = VariableResult->GetNextResult.VariableNameSize;
        goto Exit;
    }
    else if (EFIStatus == EFI_SUCCESS) {
        *BytesWritten += VariableResult->GetNextResult.VariableNameSize;
    } else if (EFI_ERROR(EFIStatus)) {
        goto Exit;
    }

    //
    // Shouldn't happen, but...
    //
    if (VariableResult->GetNextResult.VariableNameSize > OutputBufferLength) {

        TraceError("Name buffer overflow, result VariableNameSize=0x%IX\n",
            VariableResult->GetNextResult.VariableNameSize);
        goto Exit;
    }

    Output->NameLength = VariableResult->GetNextResult.VariableNameSize;
    RtlCopyMemory(&Output->VendorGuid, &VariableResult->GetNextResult.VendorGuid, sizeof(VariableResult->GetNextResult.VendorGuid));
    RtlCopyMemory(&Output->VariableName, &VariableResult->GetNextResult.VariableName, VariableResult->GetNextResult.VariableNameSize);

Exit:
    Output->EfiStatus = EFIStatus;
    return Status;
}

NTSTATUS
VariableServiceSetVariable(
    _In_ TEEC_Session* Session,
    _In_ PVOID InputBuffer,
    _In_ ULONG64 InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG64 OutputBufferLength,
    _Out_ PULONG_PTR BytesWritten
    )
{
    EFI_STATUS EFIStatus;
    NTSTATUS Status = STATUS_SUCCESS;

    PEFI_SET_VARIABLE_IN Input = NULL;
    PEFI_SET_VARIABLE_OUT Output = NULL;
    PWSTR VariableName;
    UINTN VariableNameSize;

    Input = (PEFI_SET_VARIABLE_IN)InputBuffer;
    Output = (PEFI_SET_VARIABLE_OUT)OutputBuffer;

    if (InputBufferLength < FIELD_OFFSET(EFI_SET_VARIABLE_IN, Buffer)) {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesWritten = sizeof(EFI_SET_VARIABLE_OUT);
    if (OutputBufferLength < sizeof(EFI_SET_VARIABLE_OUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Input->DataSize == 0) {
        TraceDebug("OpteeTrEE: VariableServiceSetVariable: Delete Variable\n");
    }

    VariableName = EFI_SET_VARIABLE_GET_VARIABLE_NAME(Input);
    VariableNameSize = STR_SIZE(VariableName);

    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    UINTN VariableParamSize;
    UINT32 ResultSize = 0;
    UINT32 WtrStatus = 0;

    //
    // Validate that the buffers will fit.
    // The first test validates that the variable fields are sensible
    // to start with and catches the case that they are some huge number
    // that will overflow when added to.
    //
    if ((VariableNameSize >= mVariableParamMem.size) ||
        (InputBufferLength >= mVariableParamMem.size)) {
        Status = STATUS_INVALID_BUFFER_SIZE;
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        goto Exit;
    }

    VariableParamSize = sizeof(VARIABLE_SET_PARAM) + VariableNameSize + Input->DataSize;


    //
    // The second test validates that the data will actually fit in the buffer.
    //
    if ((VariableParamSize > mVariableParamMem.size)) {

        Status = STATUS_INVALID_BUFFER_SIZE;
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        goto Exit;
    }

    //
    // Clear the buffers entirely to start with.
    //
    RtlZeroMemory(mVariableParamMem.buffer, mVariableParamMem.size);
    RtlZeroMemory(mVariableResultMem.buffer, mVariableResultMem.size);

    //
    // Fill in the parameter fields.
    //
    VariableParam->SetParam.Size = sizeof(VARIABLE_SET_PARAM);
    VariableParam->SetParam.VariableNameSize = (UINT16)VariableNameSize;
    VariableParam->SetParam.Attributes = Input->Attributes;
    VariableParam->SetParam.DataSize = (UINT32)Input->DataSize;
    VariableParam->SetParam.OffsetVariableName = 0;
    VariableParam->SetParam.OffsetData = (UINT16)VariableNameSize /*Input->DataOffset*/;

    RtlCopyMemory(&VariableParam->SetParam.VendorGuid, &(Input->VendorGuid), sizeof(VariableParam->SetParam.VendorGuid));
    RtlCopyMemory(&VariableParam->SetParam.Payload[0], VariableName, VariableNameSize);
    RtlCopyMemory(&VariableParam->SetParam.Payload[VariableNameSize], EFI_SET_VARIABLE_GET_DATA(Input), Input->DataSize);

    EFIStatus = OpteeRuntimeVariableInvokeCommand(Session,
        VSSetOp,
        (UINT32)(VariableParamSize),
        &mVariableParamMem,
        sizeof(DWORD), // Just the comply with the parameters set
        &mVariableResultMem, // Just the comply with the parameters set
        &ResultSize,
        &WtrStatus);

    TraceDebug("    EFI Status: 0x%IX\n", EFIStatus);

Exit:
    TraceDebug("OpteeRuntimeSetVariable Exit : Status=0x%X, DataSize=%Id\n", 
        Status, Input->DataSize);

    Output->EfiStatus = EFIStatus;
    return Status;
}

NTSTATUS
VariableServiceQueryInformation(
    _In_ TEEC_Session* Session,
    _In_ PVOID InputBuffer,
    _In_ ULONG64 InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG64 OutputBufferLength,
    _Out_ PULONG_PTR BytesWritten
    )

{
    NTSTATUS Status = STATUS_SUCCESS;
    EFI_STATUS EFIStatus = EFI_SUCCESS;

    PEFI_QUERY_VARIABLE_INFO_IN Input = NULL;
    PEFI_QUERY_VARIABLE_INFO_OUT Output = NULL;

    TraceDebug("Variable QUERY Service");

    Input = (PEFI_QUERY_VARIABLE_INFO_IN)InputBuffer;
    Output = (PEFI_QUERY_VARIABLE_INFO_OUT)OutputBuffer;

    if (InputBufferLength < sizeof(EFI_QUERY_VARIABLE_INFO_IN)) {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesWritten = sizeof(EFI_QUERY_VARIABLE_INFO_OUT);
    if (OutputBufferLength < sizeof(EFI_QUERY_VARIABLE_INFO_OUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PVARIABLE_PARAM  VariableParam = (PVARIABLE_PARAM)mVariableParamMem.buffer;
    PVARIABLE_RESULT VariableResult = (PVARIABLE_RESULT)mVariableResultMem.buffer;
    UINT32 VariableParamSize;
    UINT32 VariableResultSize;
    UINT32 ResultSize = 0;
    UINT32 AuthVarStatus = 0;

    VariableParamSize = sizeof(VARIABLE_QUERY_PARAM);
    VariableResultSize = sizeof(VARIABLE_QUERY_RESULT);

    //
    // Validates that the data will actually fit in the buffer.
    //
    if ((VariableParamSize  > mVariableParamMem.size) ||
        (VariableResultSize > mVariableResultMem.size)) {
        EFIStatus = EFI_BAD_BUFFER_SIZE;
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }

    //
    // Clear the buffers entirely to start with.
    //
    RtlZeroMemory(mVariableParamMem.buffer, mVariableParamMem.size);
    RtlZeroMemory(mVariableResultMem.buffer, mVariableResultMem.size);

    //
    // Fill in the parameter fields.
    //
    VariableParam->QueryParam.Size = sizeof(VARIABLE_QUERY_PARAM);
    VariableParam->QueryParam.Attributes = Input->Attributes;

    EFIStatus = OpteeRuntimeVariableInvokeCommand(Session,
        VSQueryInfoOp,
        VariableParamSize,
        &mVariableParamMem,
        VariableResultSize,
        &mVariableResultMem,
        &ResultSize,
        &AuthVarStatus);

    TraceDebug("    EFI Status: 0x%IX", EFIStatus);

    if (EFI_ERROR(EFIStatus)) {
        goto Exit;
    }

    Output->MaximumVariableSize = VariableResult->QueryResult.MaximumVariableStorageSize;
    Output->MaximumVariableStorageSize = VariableResult->QueryResult.RemainingVariableStorageSize;
    Output->RemainingVariableStorageSize = VariableResult->QueryResult.MaximumVariableSize;

Exit:
    Output->EfiStatus = EFIStatus;
    return Status;
}


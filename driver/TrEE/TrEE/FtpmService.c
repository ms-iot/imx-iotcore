/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include <acpitabl.h>
#include <ntstrsafe.h>
#include <TrustedRuntimeClx.h>
#include <initguid.h>
#include <wdmguid.h>
#include <TrEETPMService.h>
#include "OpteeTrEE.h"
#include "OpteeTrEEService.h"
#include "OpteeClientLib\OpteeClientLib.h"
#include "OpteeClientLib\OpteeClientMemory.h"
#include "OpteeFtpmService.h"
#include "OpteeVariableService.h"
#include "trace.h"

#ifdef WPP_TRACING
#include "FtpmService.tmh"
#endif



TEEC_UUID TaUuid = { 
    SWAP_B32(0xBC50D971), SWAP_B16(0xD4C9), SWAP_B16(0x42C4),
    {0x82, 0xCB, 0x34, 0x3F, 0xB7, 0xF3, 0x78, 0x96}
};

//
// Buffers to/from Windows
//

typedef struct _PPI_PENDING_OPERATION {
    UINT32  ReturnCode;
    UINT32  Operation;
    UINT32  Argument;
} PPI_PENDING_OPERATION;

typedef struct _PPI_OPERATION_RESPONSE {
    UINT32  ReturnCode;
    UINT32  Operation;
    UINT32  Response;
} PPI_OPERATION_RESPONSE;

typedef struct _PPI_REQUEST {
    UINT32  FunctionIndex;
    UINT32  Param[2];
} PPI_REQUEST, *PPPI_REQUEST;

typedef union _PPI__RESPONSE {
    CHAR Version[1];
    PPI_PENDING_OPERATION PendingOperation;
    UINT32 TransitionAction;
    PPI_OPERATION_RESPONSE OperationResponse;
    UINT32 ReturnCode;
} PPI_RESPONSE, *PPPI_RESPONSE;

typedef struct _TREE_FTPM_SERVICE_CONTEXT {
    POPTEE_TREE_DEVICE_CONTEXT  DeviceContext;
    TEEC_Context                TEECContext;
    PFTPM_CONTROL_AREA          ControlArea;
    PUCHAR                      CommandBuffer;
    PUCHAR                      ResponseBuffer;
    ULONG                       State;
    WDFDEVICE                   ServiceDevice;
    ULONG                       SessionsOpened;
    ULONG                       SessionsClosed;
} TREE_FTPM_SERVICE_CONTEXT, *PTREE_FTPM_SERVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_FTPM_SERVICE_CONTEXT);

typedef struct _TREE_FTPM_SESSION_CONTEXT {
    PTREE_FTPM_SERVICE_CONTEXT  ServiceContext;
    TEEC_Session                ClientSession;
} TREE_FTPM_SESSION_CONTEXT, *PTREE_FTPM_SESSION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_FTPM_SESSION_CONTEXT);

EVT_TR_CREATE_SECURE_SERVICE_CONTEXT FtpmServiceCreateSecureServiceContext;
EVT_TR_DESTROY_SECURE_SERVICE_CONTEXT FtpmServiceDestroySecureServiceContext;
EVT_TR_CONNECT_SECURE_SERVICE FtpmServiceConnectSecureService;
EVT_TR_DISCONNECT_SECURE_SERVICE FtpmServiceDisconnectSecureService;
EVT_TR_CREATE_SECURE_SERVICE_SESSION_CONTEXT FtpmServiceCreateSessionContext;
EVT_TR_DESTROY_SECURE_SERVICE_SESSION_CONTEXT FtpmServiceDestroySessionContext;
EVT_TR_PROCESS_SECURE_SERVICE_REQUEST FtpmServiceProcessSecureServiceRequest;
EVT_TR_CANCEL_SECURE_SERVICE_REQUEST FtpmServiceCancelSecureServiceRequest;
EVT_TR_PROCESS_OTHER_SECURE_SERVICE_IO FtpmServiceProcessOtherSecureServiceIo;

TR_SECURE_SERVICE_CALLBACKS FTPM_SERVICE_CALLBACKS = {
    0,

    &FtpmServiceCreateSecureServiceContext,
    &FtpmServiceDestroySecureServiceContext,

    &FtpmServiceConnectSecureService,
    &FtpmServiceDisconnectSecureService,

    &FtpmServiceCreateSessionContext,
    &FtpmServiceDestroySessionContext,

    &FtpmServiceProcessSecureServiceRequest,
    &FtpmServiceCancelSecureServiceRequest,
    &FtpmServiceProcessOtherSecureServiceIo
};

NTSTATUS
FtpmAcpiGetControlArea(
    _Out_ PPHYSICAL_ADDRESS ControlAreaPA
    )

/*++

Routine Description:

    Obtain the physical address of the control area from the TPM2 ACPI table.

Arguments:

    ControlAreaPA - a pointer to the variable to receive the physical address
    value from the TPM2 ACPI table.

Return value:

    STATUS_SUCCESS - everything is fine.

    STATUS_UNSUCCESSFUL - something went wrong, such as obtaining the TPM2
    ACPI table. The value in the passed argument cannot be used.

--*/

{
    PDESCRIPTION_HEADER DescriptionHeader;

    NT_ASSERT(ControlAreaPA != NULL);

    DescriptionHeader = 
        (PDESCRIPTION_HEADER)HalGetCachedAcpiTable(ACPI_SIGNATURE_TPM20,
                                                   NULL,
                                                   NULL);
    if (DescriptionHeader == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    switch (DescriptionHeader->Revision) {
        case ACPI_TPM2_REVISION_3:
            __fallthrough;

        case ACPI_TPM2_REVISION_4:
        {
            PTPM20_TABLE table = (PTPM20_TABLE)DescriptionHeader;

            ControlAreaPA->QuadPart = table->ControlAreaPA;
            break;
        }

        default:
            return STATUS_UNSUCCESSFUL;
    }

    if (ControlAreaPA->QuadPart == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
FtpmServiceCreateSecureServiceContext(
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

    PUCHAR Buffer;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    PFTPM_CONTROL_AREA ControlArea;
    PHYSICAL_ADDRESS ControlAreaPA;
    PTREE_FTPM_SERVICE_CONTEXT FtpmServiceContext;
    PHYSICAL_ADDRESS Physical;
    NTSTATUS Status;
    TEEC_Result TeecResult;    
    
    UNREFERENCED_PARAMETER(ServiceGuid);

    TraceDebug("FtpmServiceCreateSecureServiceContext : "
        "Calling TEEC_InitializeContext\n");

    ControlArea = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            TREE_FTPM_SERVICE_CONTEXT);

    Status = WdfObjectAllocateContext(ServiceDevice,
                                      &ContextAttributes,
                                      (PVOID*)&FtpmServiceContext);
    if (!NT_SUCCESS(Status)) {
        goto FtpmServiceCreateSecureServiceContextEnd;
    }

    RtlZeroMemory(FtpmServiceContext, sizeof(TREE_FTPM_SERVICE_CONTEXT));
    FtpmServiceContext->DeviceContext = TreeGetDeviceContext(MasterDevice);
    FtpmServiceContext->ServiceDevice = ServiceDevice;

    Status = OpteeClientApiLibInitialize(NULL,
        ServiceDevice,
        FtpmServiceContext->DeviceContext->SharedMemoryBaseAddress,
        FtpmServiceContext->DeviceContext->SharedMemoryLength);

    if (!NT_SUCCESS(Status)) {
        goto FtpmServiceCreateSecureServiceContextEnd;
    }

    //
    // Get TPM control area from ACPI, and map it.
    //

    Status = FtpmAcpiGetControlArea(&ControlAreaPA);
    if (!NT_SUCCESS(Status)) {
        goto FtpmServiceCreateSecureServiceContextEnd;
    }

    ControlArea = (PFTPM_CONTROL_AREA)MmMapIoSpaceEx(ControlAreaPA,
                                                     sizeof(FTPM_CONTROL_AREA),
                                                     PAGE_READWRITE);

    if (ControlArea == NULL) {
        Status = STATUS_NO_MEMORY;
        goto FtpmServiceCreateSecureServiceContextEnd;
    }

    FtpmServiceContext->ControlArea = ControlArea;

    Physical.LowPart = ControlArea->CommandPALow;
    Physical.HighPart = ControlArea->CommandPAHigh;
    Buffer = (PUCHAR)MmMapIoSpaceEx(Physical,
                                    ControlArea->CommandBufferSize,
                                    PAGE_READWRITE);

    if (Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto FtpmServiceCreateSecureServiceContextEnd;
    }

    FtpmServiceContext->CommandBuffer = Buffer;

    Physical.LowPart = ControlArea->ResponsePALow;
    Physical.HighPart = ControlArea->ResponsePAHigh;
    Buffer = (PUCHAR)MmMapIoSpaceEx(Physical,
                                    ControlArea->ResponseBufferSize,
                                    PAGE_READWRITE);

    if (Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto FtpmServiceCreateSecureServiceContextEnd;
    }

    FtpmServiceContext->ResponseBuffer = Buffer;
    
    TeecResult = TEEC_InitializeContext(NULL, &FtpmServiceContext->TEECContext);
    if (TeecResult != TEEC_SUCCESS) {
        Status = STATUS_UNSUCCESSFUL;
        goto FtpmServiceCreateSecureServiceContextEnd;
    }

    Status = STATUS_SUCCESS;

 FtpmServiceCreateSecureServiceContextEnd:
    if (!NT_SUCCESS(Status)) {
        if (FtpmServiceContext->ControlArea != NULL) {
            if (FtpmServiceContext->CommandBuffer != NULL) {
                MmUnmapIoSpace(FtpmServiceContext->CommandBuffer,
                               ControlArea->CommandBufferSize);
            }

            if (FtpmServiceContext->ResponseBuffer != NULL) {
                MmUnmapIoSpace(FtpmServiceContext->ResponseBuffer,
                               ControlArea->ResponseBufferSize);
            }

            MmUnmapIoSpace(FtpmServiceContext->ControlArea,
                           sizeof(FTPM_CONTROL_AREA));
        }
    }

    return Status;
}

_Use_decl_annotations_
NTSTATUS
FtpmServiceDestroySecureServiceContext(
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

    PTREE_FTPM_SERVICE_CONTEXT ServiceContext;
    ServiceContext = WdfObjectGet_TREE_FTPM_SERVICE_CONTEXT(ServiceDevice);

    OpteeClientApiLibDeinitialize(NULL, ServiceContext->ServiceDevice);

    TEEC_FinalizeContext(&ServiceContext->TEECContext);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
FtpmServiceConnectSecureService(
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
FtpmServiceDisconnectSecureService(
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
FtpmServiceCreateSessionContext(
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
    PTREE_FTPM_SESSION_CONTEXT SessionContext;
    PTREE_FTPM_SERVICE_CONTEXT ServiceContext;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    WDFOBJECT NewContext;
    UINT32 ErrorOrigin;

    ServiceContext = WdfObjectGet_TREE_FTPM_SERVICE_CONTEXT(ServiceDevice);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes, TREE_FTPM_SESSION_CONTEXT);

    Status = WdfObjectCreate(&ContextAttributes, &NewContext);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    SessionContext = WdfObjectGet_TREE_FTPM_SESSION_CONTEXT(NewContext);
    SessionContext->ServiceContext = ServiceContext;    

    TeecResult = TEEC_OpenSession(&ServiceContext->TEECContext,
        &SessionContext->ClientSession,
        &TaUuid,
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
FtpmServiceDestroySessionContext(
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
    PTREE_FTPM_SESSION_CONTEXT SessionContext;

    if (*SessionContextObject != NULL) {
        SessionContext = WdfObjectGet_TREE_FTPM_SESSION_CONTEXT(*SessionContextObject);
        TEEC_CloseSession(&SessionContext->ClientSession);
        WdfObjectDelete(*SessionContextObject);
    }    
    return STATUS_SUCCESS;
}

NTSTATUS
FtpmOpteeSubmitCommand(
    PTREE_FTPM_SESSION_CONTEXT Context,
    ULONG Command,
    PUCHAR InputBuffer,
    ULONG InputBufferSize,
    PUCHAR OutputBuffer,
    ULONG OutputBufferSize,
    PULONG_PTR BytesWritten
    )
{

    ULONG ErrorOrigin;
    BOOLEAN FreeMemory;
    UCHAR *SharedInput;
    UCHAR *SharedOutput;
    NTSTATUS Status;
    TEEC_Operation TeecOperation;
    TEEC_Result TeecResult;
    TEEC_SharedMemory TeecSharedMem;

    FreeMemory = FALSE;
    Status = STATUS_SUCCESS;

    //
    // Create shared memory copies of the buffers.
    //

    RtlZeroMemory(&TeecSharedMem, sizeof(TeecSharedMem));
    TeecSharedMem.size = InputBufferSize + OutputBufferSize;
    TeecResult = TEEC_AllocateSharedMemory(
                    &Context->ServiceContext->TEECContext,
                    &TeecSharedMem);

    if (TeecResult != TEEC_SUCCESS) {
        Status = STATUS_NO_MEMORY;
        goto FtpmSubmitCommandEnd;
    }

    FreeMemory = TRUE;
    SharedInput = (UINT8 *)TeecSharedMem.buffer;
    SharedOutput = SharedInput + InputBufferSize;
    RtlCopyMemory(SharedInput, InputBuffer, InputBufferSize);

    RtlZeroMemory(&TeecOperation, sizeof(TeecOperation));
    TeecOperation.params[0].tmpref.buffer = (PVOID)(ULONG_PTR)OpteeClientVirtualToPhysical(SharedInput);
    TeecOperation.params[0].tmpref.size = InputBufferSize;
    TeecOperation.params[1].tmpref.buffer = (PVOID)(ULONG_PTR)OpteeClientVirtualToPhysical(SharedOutput);
    TeecOperation.params[1].tmpref.size = OutputBufferSize;
    TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                                TEEC_MEMREF_TEMP_INOUT,
                                                TEEC_NONE,
                                                TEEC_NONE);

    TeecResult = TEEC_InvokeCommand(&Context->ClientSession,
                                    Command,
                                    &TeecOperation,
                                    (uint32_t *)&ErrorOrigin);

    if (TeecResult != TEEC_SUCCESS) {
        Status = STATUS_INVALID_PARAMETER;
        goto FtpmSubmitCommandEnd;
    }

    RtlCopyMemory(OutputBuffer,
                  SharedOutput,
                  TeecOperation.params[1].memref.size);

    if (ARGUMENT_PRESENT(BytesWritten)) {
        *BytesWritten = TeecOperation.params[1].memref.size;
    }

FtpmSubmitCommandEnd:
    if (FreeMemory != FALSE) {
        TEEC_ReleaseSharedMemory(&TeecSharedMem);
    }

    return Status;
}

ULONG
FtpmValidateOperation(
    ULONG Operation,
    ULONG Flags
    )

/*++

Routine Description:

    This routine is called to validate the operation requested.

Arguments:

    Operation - Supplies the requested operation.

    Flags - Supplies the UEFI PPI flags.

Return Value:

    PPI_USER_CONFIRMATION_NOT_IMPLEMENTED
        The operation is not implemented.

    PPI_USER_CONFIRMATION_FIRMWARE 
        The operation is only available for firmware.

    PPI_USER_CONFIRMATION_BLOCKED
        The operation is blocked.

    PPI_USER_CONFIRMATION_PP_REQUIRED
        The operation requires the user's physical presence to acknowledge the
        operation. Normally, a prompt will be displayed to the user to accept
        the operation.

    PPI_USER_CONFIRMATION_PP_NOT_REQUIRED
        The operation is permitted and does not require the user's physical
        presence to acknowledge the operation.

--*/

{
    ULONG Status;

    UNREFERENCED_PARAMETER(Flags);

    switch (Operation) {
        case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
            Status = PPI_USER_CONFIRMATION_PP_NOT_REQUIRED;
            break;

        case TCG2_PHYSICAL_PRESENCE_CLEAR:
        case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
        case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
        case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3: {
            Status = (Flags & TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CLEAR) ?
                    PPI_USER_CONFIRMATION_PP_REQUIRED :
                    PPI_USER_CONFIRMATION_PP_NOT_REQUIRED;

            break;
            
        }

        default: {                    
            Status = PPI_USER_CONFIRMATION_NOT_IMPLEMENTED;
            break;
        }
    }

    return Status;
}

NTSTATUS
FtpmHandlePpiRequest (
    PPPI_REQUEST Request,
    ULONG RequestSize,
    PPPI_RESPONSE Response,
    ULONG ResponseSize,
    PULONG_PTR BytesWritten
    )

/*++

Routine Description:

    This routine handle's a Physical Presence (PP) request.

    High Level
    ----------
    The PP request is ultimately executed by UEFI. The OS will call this API to
    submit the request. The request will be stored in a UEFI variable
    (authenticated variable) and upon next reboot UEFI will execute the
    request. UEFI then stores the response in the UEFI variable and it is
    available for the OS to read.

    Details
    -------
    The PP interface specifies a Function Index that corresponds to different
    actions. The actions are specified by the PPI specification but most
    importantly the OS can:
        - Query the required action to transition to UEFI (eg. reboot).
        - Submit a PP request.
        - Query a pending PP request.
        - Query the response to the last PP request.
    

Arguments:

    Request - Supplies the requested operation. This is a function index
              followed by any arguments.

    RequestSize - Supplies the size of the request buffer.

    Response - Supplies a buffer to receive a response.

    ResponseSize - Supplies the size of the response buffer.

    BytesWritten - Supplies a pointer to receive the number of bytes written to
                   the response buffer.

Return Value:

    NTSTATUS

--*/

{
    ULONG OperationStatus;
    UINT32 PpiFlags;
    TCG2_PPI_VARIABLE PpiValue;
    ULONG PpiValueSize;
    UINTN ReturnSize;
    NTSTATUS Status;

    ReturnSize = 0;
    Status = STATUS_SUCCESS;

    //
    // Must supply a function index.
    //

    if (RequestSize < sizeof(UINT32) ||
        Request == NULL ||
        Response == NULL) {

        Status = STATUS_INVALID_PARAMETER;
        goto FtpmHandlePpiRequestEnd;
    }

    //
    // UEFI initializes the variables used by PPI, if there is a failure
    // reading the variables then PPI is not available.
    //

    PpiValueSize = sizeof(PpiFlags);
    PpiFlags = 0;
    Status = ExGetFirmwareEnvironmentVariable(&PpiFlagsVariableNameUnicode,
                                              &PpiVariableGuid,
                                              (PVOID)&PpiFlags,
                                              &PpiValueSize,
                                              NULL);

    if (!NT_SUCCESS(Status)) {
        goto FtpmHandlePpiRequestEnd;
    }

    PpiValueSize = sizeof(PpiValue);
    RtlZeroMemory(&PpiValue, PpiValueSize);
    Status = ExGetFirmwareEnvironmentVariable(&PpiVariableNameUnicode,
                                              &PpiVariableGuid,
                                              (PVOID)&PpiValue,
                                              &PpiValueSize,
                                              NULL);

    if (!NT_SUCCESS(Status)) {
        goto FtpmHandlePpiRequestEnd;
    }

    switch(Request->FunctionIndex) {
        case TCG_ACPI_FUNCTION_GET_PHYSICAL_PRESENCE_INTERFACE_VERSION: {
            ReturnSize = strlen(PPI_INTERFACE_VERSION) + 1;
            if (ResponseSize < ReturnSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }

            RtlCopyMemory(Response->Version, PPI_INTERFACE_VERSION, ReturnSize);
            break;
        }

        case TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS:
        case TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS_2: {

            //
            // Submit Operation 2 supportes an extra argument. However, only
            // single argument operations are supported.
            //

            if (RequestSize != 2 * sizeof(UINT32)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }

            //
            // Response is a single return code.
            //

            ReturnSize = sizeof(UINT32);
            if (ResponseSize < ReturnSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }

            OperationStatus = FtpmValidateOperation(Request->Param[0],
                                                    PpiFlags);

            if (OperationStatus == PPI_USER_CONFIRMATION_NOT_IMPLEMENTED) {

                //
                // PPI_SUBMIT_OPERATION_NOT_SUPPORTED and
                // PPI_SUMBIT_OPERATION2_NOT_IMPLEMENTED have the same value.
                //

                Response->ReturnCode = PPI_SUBMIT_OPERATION_NOT_SUPPORTED;

            } else if (OperationStatus < PPI_USER_CONFIRMATION_PP_REQUIRED) {
                if (Request->FunctionIndex == TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS) {
                    Response->ReturnCode = PPI_SUBMIT_OPERATION_ERROR;
                    
                } else {
                    Response->ReturnCode = PPI_SUMBIT_OPERATION2_BLOCKED;
                }

                break;
            }

            //
            // Store the operation in the UEFI variable and UEFI will execute
            // the operation on next boot.
            //

            PpiValue.Request = (UINT8)Request->Param[0];
            Status = ExSetFirmwareEnvironmentVariable(
                        &PpiVariableNameUnicode,
                        &PpiVariableGuid,
                        (PVOID)&PpiValue,
                        sizeof(PpiValue),
                        EFI_VARIABLE_NON_VOLATILE  |
                        EFI_VARIABLE_BOOTSERVICE_ACCESS |
                        EFI_VARIABLE_RUNTIME_ACCESS );

            if (!NT_SUCCESS(Status)) {
                Response->ReturnCode = PPI_SUBMIT_OPERATION_ERROR;
                Status = STATUS_SUCCESS;

            } else {
                Response->ReturnCode = PPI_SUCCESS;
            }

            break;
        }

        case TCG_ACPI_FUNCTION_GET_PENDING_REQUEST_BY_OS: {

            //
            // Returns the last requested executed operation.
            //

            ReturnSize = offsetof(PPI_PENDING_OPERATION, Argument);
            if (ResponseSize < ReturnSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }

            Response->PendingOperation.ReturnCode = PPI_SUCCESS;
            Response->PendingOperation.Operation = PpiValue.Request;
            break;
        }

        case TCG_ACPI_FUNCTION_GET_PLATFORM_ACTION_TO_TRANSITION_TO_BIOS: {

            //
            // Returns the action required for UEFI to execute the request.
            //

            ReturnSize = sizeof(UINT32);
            if (ResponseSize < ReturnSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }
            
            Response->TransitionAction = PPI_TRANSITION_ACTION_REBOOT;
            break;
        }

        case TCG_ACPI_FUNCTION_RETURN_REQUEST_RESPONSE_TO_OS: {

            //
            // Returns the response of the last executed request.
            //

            ReturnSize = sizeof(PPI_OPERATION_RESPONSE);
            if (ResponseSize < ReturnSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }

            Response->OperationResponse.ReturnCode = PPI_SUCCESS;
            Response->OperationResponse.Operation = PpiValue.LastRequest;
            Response->OperationResponse.Response = PpiValue.Response;
            break;
        }

        case TCG_ACPI_FUNCTION_SUBMIT_PREFERRED_USER_LANGUAGE: {

            //
            // Deprecated.
            //

            ReturnSize = sizeof(UINT32);
            if (ResponseSize < ReturnSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }
            
            Response->ReturnCode = PPI_LANGUAGE_NOT_IMPLEMENTED;
            break;
        }

        case TCG_ACPI_FUNCTION_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST: {

            //
            // Returns validation for a given operation.
            //

            ReturnSize = sizeof(UINT32);
            if (ResponseSize < ReturnSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto FtpmHandlePpiRequestEnd;
            }

            Response->ReturnCode = FtpmValidateOperation(Request->Param[0],
                                                         PpiFlags);

            break;
        }

        default: {
            Status = STATUS_INVALID_PARAMETER;
            goto FtpmHandlePpiRequestEnd;
            break;
        }
        
    }

    if (ARGUMENT_PRESENT(BytesWritten)) {
      *BytesWritten = ReturnSize;
    }

FtpmHandlePpiRequestEnd:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
FtpmServiceProcessSecureServiceRequest(
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
    PTREE_FTPM_SESSION_CONTEXT Context;
    PFTPM_CONTROL_AREA ControlArea;
    TPM_INVOKE_GENERIC_OUT *InvokeOutput;
    PTREE_FTPM_SERVICE_CONTEXT ServiceContext;
    ULONG State;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    Status = STATUS_SUCCESS;
    Context = WdfObjectGet_TREE_FTPM_SESSION_CONTEXT(SessionContextObject);
    ServiceContext = Context->ServiceContext;

    switch (Request->FunctionCode) {
        case TPM_FUNCTION_INVOKE: {
            if (Request->OutputBufferSize != sizeof(TPM_INVOKE_GENERIC_OUT)) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // This is the main TPM submit command function that follows the
            // TPM2 handshake spec.
            //

            State = ServiceContext->State;
            ControlArea = ServiceContext->ControlArea;
            switch (State) {
                case TpmStateReady: {
                    if (ControlArea->Start & CONTROL_AREA_START_START) {
                        Status = FtpmOpteeSubmitCommand(
                                    Context,
                                    TA_FTPM_SUBMIT_COMMAND,
                                    ServiceContext->CommandBuffer,
                                    ControlArea->CommandBufferSize,
                                    ServiceContext->ResponseBuffer,
                                    ControlArea->ResponseBufferSize,
                                    NULL);

                        ControlArea->Start &= (~CONTROL_AREA_START_START);

                    } else if (ControlArea->Request & CONTROL_AREA_REQUEST_GOIDLE) {
                        ServiceContext->State = TpmStateIdle;
                        ControlArea->Status |= CONTROL_AREA_STATUS_IDLE;
                        ControlArea->Request &= (~CONTROL_AREA_REQUEST_GOIDLE);

                    } else if (ControlArea->Request & CONTROL_AREA_REQUEST_CMD_READY) {
                        ControlArea->Request &= (~CONTROL_AREA_REQUEST_CMD_READY);
                    }

                    break;
                }    

                case TpmStateIdle: {
                    if (ControlArea->Start & CONTROL_AREA_START_START) {
                        ASSERT(FALSE);
                        Status = STATUS_UNSUCCESSFUL;
                        TraceError("ERR: Start received when idle\n");
                        
                    } else if (ControlArea->Request & CONTROL_AREA_REQUEST_CMD_READY) {
                        Context->ServiceContext->State = TpmStateReady;
                        ControlArea->Status &= (~CONTROL_AREA_STATUS_IDLE);
                        ControlArea->Request &= (~CONTROL_AREA_REQUEST_CMD_READY);
                    }
                    
                    break;
                }

                default: {
                    ASSERT(FALSE);
                    break;
                }
            }

            InvokeOutput = (TPM_INVOKE_GENERIC_OUT *)((PVOID)Request->OutputBuffer);
            InvokeOutput->Status = Status;
            *BytesWritten = sizeof(TPM_INVOKE_GENERIC_OUT);
            break;
        }

        case TPM_FUNCTION_PPI: {
            TraceDebug("TPM_FUNCTION_PPI received\n");
            Status = FtpmHandlePpiRequest(Request->InputBuffer,
                                          (ULONG)Request->InputBufferSize,
                                          Request->OutputBuffer,
                                          (ULONG)Request->OutputBufferSize,
                                          BytesWritten);

            break;
        }

        default: {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    return Status;
}

_Use_decl_annotations_
VOID
FtpmServiceCancelSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine is called to cancel a request made via a previous call to
        FtpmServiceProcessSecureServiceRequest. Note that cancellation is
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
FtpmServiceProcessOtherSecureServiceIo(
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


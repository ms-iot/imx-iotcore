/** @file
The OP-TEE SMC Interface that packs the SMC parameters and sends the SMC to
secure world for handling.
**/

/* Copyright(c) 2018, Microsoft Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met :
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *this list of conditions and the following disclaimer in the documentation
 * and / or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ntddk.h>
#include <ntstatus.h>
#include <wdmguid.h>
#include <sffdisk.h>
#include "types.h"
#include "OpteeClientLib.h"
#include "OpteeClientMemory.h"
#include "OpteeClientSMC.h"
#include "OpteeClientRPC.h"
#include "trace.h"

#ifdef WPP_TRACING
#include "OpteeClientLib.tmh"
#endif


WDFDEVICE LibServiceDevice = NULL;
KSEMAPHORE OpteeSMCLock;
KSEMAPHORE OpteeMemLock;

static BOOLEAN OpteeClientLibInitialized = FALSE;
static LONG OpteeClientApiLibRefCount = 0;

DRIVER_NOTIFICATION_CALLBACK_ROUTINE OpteeClientRpmbPnpNotification;
PVOID OpteeClientRpmbPnpNotificationHandle = NULL;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS OpteeRpcGetRpmbCid(
    _In_ WDFDEVICE WdfDevice,
    _In_ PUNICODE_STRING TargetSymbolicLink
    );
EVT_WDF_WORKITEM OpteeClientGetRpmbCidWorker;

#define HUNDRED_NANO_TO_SEC (10LL * 1000LL * 1000LL)

#define OPTEE_LIB_TRG_NAME_TAG 'grTO'

#define GUIDCOMPARE(g1, g2) \
    (RtlCompareMemory(&(g1),&(g2),sizeof(GUID)) == sizeof(GUID))


/*
 * Initialize the library
 */
NTSTATUS OpteeClientApiLibInitialize(
    _In_opt_ HANDLE ImageHandle,
    _In_ WDFDEVICE ServiceDevice,
    _In_ PHYSICAL_ADDRESS SharedMemoryBase,
    _In_ UINT32 SharedMemoryLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!OpteeClientLibInitialized){
        Status = OpteeClientMemInit(ImageHandle,
            SharedMemoryBase, SharedMemoryLength);

        if (!NT_SUCCESS(Status)) {
            TraceError("OpteeClientMemInit failed, status %!STATUS!",
                Status);

            goto Exit;
        }

        Status = OpteeClientRpcInit();
        if (!NT_SUCCESS(Status)) {
            TraceError("OpteeClientRpcInit failed, status %!STATUS!",
                Status);

            goto Exit;
        }

        LibServiceDevice = ServiceDevice;

        // Initializing the SMC lock.
        //

        KeInitializeSemaphore(&OpteeSMCLock, 1, 1);

        // Initialize the Memory lock.
        //

        KeInitializeSemaphore(&OpteeMemLock, 1, 1);

        NT_ASSERT(OpteeClientRpmbPnpNotificationHandle == NULL);

        // Block SMC calls until RPMB information has been
        // acquired, or failed.
        //

        Status = KeWaitForSingleObject(&OpteeSMCLock,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);
        ASSERT(Status == STATUS_SUCCESS);

        Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                    PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                    (PVOID)&GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                    WdfDriverWdmGetDriverObject(WdfGetDriver()),
                    OpteeClientRpmbPnpNotification,
                    NULL,
                    &OpteeClientRpmbPnpNotificationHandle);

        if (NT_SUCCESS(Status)) {
            OpteeClientLibInitialized = TRUE;
        } else {
            KeReleaseSemaphore(&OpteeSMCLock,
                IO_NO_INCREMENT,
                1,
                FALSE);
        }
    }

Exit:

    InterlockedIncrement(&OpteeClientApiLibRefCount);
    return Status;
}


typedef struct _GET_RPMB_CID_WORKER_CONTEXT {
    UNICODE_STRING TargetSymbolicLink;
} GET_RPMB_CID_WORKER_CONTEXT, *PGET_RPMB_CID_WORKER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(GET_RPMB_CID_WORKER_CONTEXT, GetRpmbCidWorkerContext);

_Use_decl_annotations_
VOID
OpteeClientGetRpmbCidWorker(
    WDFWORKITEM WdfWorkItem
    )
{
    PGET_RPMB_CID_WORKER_CONTEXT WorkerContext = GetRpmbCidWorkerContext(WdfWorkItem);

    OpteeRpcGetRpmbCid(LibServiceDevice,
        &WorkerContext->TargetSymbolicLink);

    ExFreePoolWithTag(WorkerContext->TargetSymbolicLink.Buffer,
        OPTEE_LIB_TRG_NAME_TAG);

    WdfObjectDelete(WdfWorkItem);
}

_Use_decl_annotations_
NTSTATUS
OpteeClientRpmbPnpNotification(
    PVOID NotificationStructure,
    PVOID Context
    )
{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification;
    NTSTATUS Status = STATUS_SUCCESS;
    WDFWORKITEM WdfWorkItem = NULL;

    UNREFERENCED_PARAMETER(Context);

    Notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;

    NT_ASSERT(GUIDCOMPARE(Notification->InterfaceClassGuid,
        GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB));

    if (GUIDCOMPARE(Notification->Event, GUID_DEVICE_INTERFACE_ARRIVAL)) {

        // We need to query the Cid in a worker thread, otherwise
        // we may risk a deadlock
        //

        WDF_OBJECT_ATTRIBUTES Attributes;
        WDF_WORKITEM_CONFIG WdfWorkitemConfig;
        PGET_RPMB_CID_WORKER_CONTEXT WorkerContext;

        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&Attributes,
                                               GET_RPMB_CID_WORKER_CONTEXT);
        Attributes.ParentObject = LibServiceDevice;

        WDF_WORKITEM_CONFIG_INIT(&WdfWorkitemConfig,
            OpteeClientGetRpmbCidWorker);

        Status = WdfWorkItemCreate(&WdfWorkitemConfig,
                    &Attributes,
                    &WdfWorkItem);

        if (!NT_SUCCESS(Status)) {
            TraceError("WdfWorkItemCreate failed, status 0x%08X\n", Status);
            goto Exit;
        }

        WorkerContext = GetRpmbCidWorkerContext(WdfWorkItem);

        // Copy the target symbolic link
        //

        WorkerContext->TargetSymbolicLink.Buffer =
            ExAllocatePoolWithTag(NonPagedPoolNx,
                Notification->SymbolicLinkName->MaximumLength,
                OPTEE_LIB_TRG_NAME_TAG);

        if (WorkerContext->TargetSymbolicLink.Buffer == NULL) {
            TraceError("OpteeTreeRpmbPnpNotification failed to allocate "
                "target name buffer, size %d\n",
                Notification->SymbolicLinkName->MaximumLength);

            goto Exit;
        }

        WorkerContext->TargetSymbolicLink.Length = 0;
        WorkerContext->TargetSymbolicLink.MaximumLength =
            Notification->SymbolicLinkName->MaximumLength;

        RtlCopyUnicodeString(&WorkerContext->TargetSymbolicLink,
                             Notification->SymbolicLinkName);

        WdfWorkItemEnqueue(WdfWorkItem);
    } // Device arrival

Exit:

    if (!NT_SUCCESS(Status)) {
        if (WdfWorkItem != NULL) {
            WdfObjectDelete(WdfWorkItem);
        }

        KeReleaseSemaphore(&OpteeSMCLock,
            IO_NO_INCREMENT,
            1,
            FALSE);
    }

    return STATUS_SUCCESS;
}


/*
* Release resources used by the library
*/
VOID
OpteeClientApiLibDeinitialize(
    _In_opt_ HANDLE ImageHandle,
    _In_ WDFDEVICE ServiceDevice
    )
{
    UNREFERENCED_PARAMETER(ImageHandle);
    UNREFERENCED_PARAMETER(ServiceDevice);

    if (InterlockedDecrement(&OpteeClientApiLibRefCount) == 0) {

        if (OpteeClientRpmbPnpNotificationHandle != NULL) {
            (VOID)IoUnregisterPlugPlayNotification(
                OpteeClientRpmbPnpNotificationHandle);

        }
        OpteeClientRpmbPnpNotificationHandle = NULL;

        OpteeClientMemDeinit();
	OpteeClientLibInitialized = FALSE;
    }
}


/*
* This function initializes a new TEE Context, connecting this Client
* application to the TEE identified by the name.
*
* name == NULL will give the default TEE.
*
* In this implementation only the default name is supported.
* If name != NULL then TEEC_ERROR_ITEM_NOT_FOUND is returned.
*/
uint32_t TEEC_InitializeContext(const char *name,
    TEEC_Context *context)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;

    TraceDebug("TEEC_InitializeContext Enter: name=%s, context=0x%p\n",
        name,
        context);

    if (context == NULL) {
        ASSERT(context != NULL);
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    if (name != NULL) {
        TeecResult = TEEC_ERROR_ITEM_NOT_FOUND;
        goto Exit;
    }

    RtlZeroMemory(context, sizeof(*context));

Exit:
    TraceDebug("TEEC_InitializeContext Exit : TeecResult=0x%X\n", TeecResult);
    return TeecResult;
}


/*
* This function destroys an initialized TEE Context, closing the connection
* between the Client and the TEE.
* The function implementation MUST do nothing if context is NULL
*
* There is nothing to do here since there is no context state.
*/
void TEEC_FinalizeContext(TEEC_Context *context)
{
    TraceDebug("TEEC_FinalizeContext Enter-Exit: context=0x%p\n", context);
}


/*
* Allocates or registers shared memory.
*
* Since EDK2 is configured flat with virtual memory == physical memory
* then we don't need to perform any special operations to get physical
* contiguous memory.
*/
TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context *context,
    TEEC_SharedMemory *shared_memory)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    NTSTATUS Status;

    TraceDebug("TEEC_AllocateSharedMemory Enter: context=0x%p, shared_memory=0x%p\n",
        context,
        shared_memory);

    if ((context == NULL) || (shared_memory == NULL)) {
        ASSERT((context != NULL) && (shared_memory != NULL));
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    // Currently not needed but ensure that it's properly set.
    //
    if (shared_memory->flags != 0) {
        ASSERT(shared_memory->flags == 0);
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    // Clear the outbound parameters in case we fail out.

    shared_memory->buffer = NULL;
    shared_memory->shadow_buffer = 0;

    TraceDebug("TEEC_AllocateSharedMemory: size=%Id, flags=0x%X\n",
        shared_memory->size,
        shared_memory->flags);

    Status = OpteeClientMemAlloc((UINT32)(shared_memory->size),
                &shared_memory->buffer,
                NULL);

    if (shared_memory->buffer == NULL || !NT_SUCCESS(Status)) {
        TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
        goto Exit;
    }

    // We need to keep the allocated buffer to pass to the free function
    // as a differentiation with a supplied (registered) buffer case.
    //
    shared_memory->shadow_buffer = shared_memory->buffer;

Exit:
    TraceDebug("TEEC_AllocateSharedMemory Exit : TeecResult=0x%X\n",
        TeecResult);

    return TeecResult;
}


/*
* Releases shared memory.
*
* The optee_client implementation allows this to be called with a null pointer
* and null buffer but we'll assert this is not the case for better debugging.
*/
void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *shared_memory)
{
    TraceDebug("TEEC_ReleaseSharedMemory Enter: shared_memory=0x%p\n",
        shared_memory);

    if (shared_memory == NULL) {
        ASSERT(shared_memory != NULL);
        goto Exit;
    }

    if (shared_memory->buffer == NULL) {
        ASSERT(shared_memory->buffer != NULL);
        goto Exit;
    }

    if (shared_memory->shadow_buffer != 0) {

        OpteeClientMemFree(shared_memory->shadow_buffer);
        shared_memory->shadow_buffer = 0;
    }

    shared_memory->buffer = NULL;
    shared_memory->size = 0;

Exit:
    return;
}


/*
* Register shared memory
*
* If the supplied buffer is compatible we can use it as supplied otherwise
* we'll need to allocate a copy buffer for the transfer instead.
*/
TEEC_Result TEEC_RegisterSharedMemory(TEEC_Context *context,
    TEEC_SharedMemory *shared_memory)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;

    TraceDebug("TEEC_RegisterSharedMemory Enter: context=0x%p, shared_memory=0x%p\n",
        context,
        shared_memory);

    if ((context == NULL) || (shared_memory == NULL)) {
        ASSERT((context != NULL) && (shared_memory != NULL));
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    if (shared_memory->buffer == NULL) {
        ASSERT(shared_memory->buffer != NULL);
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    shared_memory->shadow_buffer = 0;

    {
        UINT_PTR Start = (UINT_PTR)shared_memory->buffer;
        UINT32 Size = (UINT32)shared_memory->size;

        // If the supplied buffer is wholly page aligned we can use it as is
        // otherwise we'll need to allocate a copy buffer to use for the transfer
        // that is properly page aligned.
        //
        if (((Start % PAGE_SIZE) != 0) ||
            ((Size  % PAGE_SIZE) != 0)) {

            TEEC_SharedMemory TempSharedMemory;

            TempSharedMemory.size = shared_memory->size;
            TempSharedMemory.flags = shared_memory->flags;

            TeecResult = TEEC_AllocateSharedMemory(context, &TempSharedMemory);
            if (TeecResult != TEEC_SUCCESS) {
                goto Exit;
            }

            shared_memory->shadow_buffer = TempSharedMemory.shadow_buffer;
        }
    }

Exit:
    TraceDebug("TEEC_RegisterSharedMemory Exit : TeecResult=0x%X\n",
        TeecResult);

    return TeecResult;
}


/*
* This function opens a new Session between the Client application and the
* specified TEE application.
*
* Only connection_method == TEEC_LOGIN_PUBLIC is supported connection_data and
* operation shall be set to NULL.
*/
TEEC_Result TEEC_OpenSession(TEEC_Context *context,
    TEEC_Session *session,
    const TEEC_UUID *destination,
    uint32_t connection_method,
    const void *connection_data,
    TEEC_Operation *operation,
    uint32_t *error_origin)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    uint32_t TeecErrorOrigin = TEEC_ORIGIN_API;

    UNREFERENCED_PARAMETER(connection_data);

    TraceDebug("TEEC_OpenSession: context=0x%p, session=0x%p, ...\n",
        context,
        session);

    if ((context == NULL) || (session == NULL) || (destination == NULL)) {
        ASSERT((context != NULL) && (session != NULL) && (destination != NULL));
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    if (connection_method != TEEC_LOGIN_PUBLIC) {
        ASSERT(connection_method == TEEC_LOGIN_PUBLIC);
        TeecResult = TEEC_ERROR_NOT_SUPPORTED;
        goto Exit;
    }

    TraceDebug("TA=%8.8X-%4.4X-%4.4X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\n",
        destination->timeLow, destination->timeMid, destination->timeHiAndVersion,
        destination->clockSeqAndNode[0], destination->clockSeqAndNode[1],
        destination->clockSeqAndNode[2], destination->clockSeqAndNode[3],
        destination->clockSeqAndNode[4], destination->clockSeqAndNode[5],
        destination->clockSeqAndNode[6], destination->clockSeqAndNode[7]);

    {
        TEEC_Operation TeecNullOperation = { 0 };
        TEEC_Operation *TeecOperation;

        // The existing Linux client ensures that operation is not NULL
        // passing through so do the same and pass along an empty one instead.
        // We'll also do the same for error_origin.
        //
        if (operation == NULL) {
            TeecOperation = &TeecNullOperation;
        }
        else {
            TeecOperation = operation;
        }

        // How this is packed for the SMC call depends on the architecture
        // so call through to an architecture specific implementation.
        //
        TeecResult = TEEC_SMC_OpenSession(context, session, destination,
            TeecOperation, &TeecErrorOrigin);
    }

    TraceDebug("New TA session id 0x%X\n", session->session_id);

Exit:
    if (error_origin != NULL) {
        *error_origin = TeecErrorOrigin;
    }

    TraceDebug("TEEC_OpenSession Exit : TeecResult=0x%X, "
        "TeecErrorOrigin=0x%X\n",
        TeecResult,
        TeecErrorOrigin);

    return TeecResult;
}


/*
* This function closes a session which has been opened with a TEE
* application.
*/
void TEEC_CloseSession(TEEC_Session *session)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    uint32_t TeecErrorOrigin = TEEC_ORIGIN_API;

    TraceDebug("TEEC_CloseSession Enter: session=0x%p\n",
        session);

    if (session == NULL) {
        ASSERT(session != NULL);
        goto Exit;
    }

    TeecResult = TEEC_SMC_CloseSession(session, &TeecErrorOrigin);

Exit:
    TraceDebug("TEEC_CloseSession Exit : TeecResult=0x%X, "
        "TeecErrorOrigin=0x%X\n",
        TeecResult,
        TeecErrorOrigin);

    return;
}


/*
* Invokes a TEE command (secure service, sub-PA or whatever).
*/
TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
    uint32_t cmd_id,
    TEEC_Operation *operation,
    uint32_t *error_origin)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    uint32_t TeecErrorOrigin = TEEC_ORIGIN_API;

    TraceDebug("TEEC_InvokeCommand Enter: session=0x%p, cmd_id=0x%X\n",
        session,
        cmd_id);

    if (session == NULL) {
        ASSERT(session != NULL);
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    {
        TEEC_Operation TeecNullOperation = { 0 };
        TEEC_Operation *TeecOperation;

        // The existing Linux client ensures that operation is not NULL passing through
        // so do the same and pass along an empty one instead.
        // We'll also do the same for error_origin.
        //
        if (operation == NULL) {
            TeecOperation = &TeecNullOperation;
        }
        else {
            TeecOperation = operation;
        }

        // How this is packed for the SMC call depends on the architecture so call through
        // to an architecture specific implementation.
        //
        TeecResult = TEEC_SMC_InvokeCommand(session, cmd_id,
            TeecOperation, &TeecErrorOrigin);
    }

Exit:
    if (error_origin != NULL) {
        *error_origin = TeecErrorOrigin;
    }

    TraceDebug("TEEC_InvokeCommand Exit : TeecResult = 0x%X, TeecErrorOrigin = 0x%X\n",
                TeecResult,
                TeecErrorOrigin);

    return TeecResult;
}


/*
* Request a cancellation of a in-progress operation (best effort)
*/
void TEEC_RequestCancellation(TEEC_Operation *operation)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    uint32_t TeecErrorOrigin = TEEC_ORIGIN_API;

    TraceDebug("TEEC_RequestCancellation Enter: operation=0x%p\n",
               operation);

    if (operation == NULL) {
        ASSERT(operation != NULL);
        goto Exit;
    }

    TeecResult = TEEC_SMC_RequestCancellation(operation, &TeecErrorOrigin);

Exit:
    TraceDebug("TEEC_RequestCancellation Exit : TeecResult=0x%X, TeecErrorOrigin=0x%X\n",
               TeecResult,
               TeecErrorOrigin);

    return;
}

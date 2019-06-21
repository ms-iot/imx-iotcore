/** @file
  The OP-TEE RPC Interface that handles return calls from the OP-TEE OS in
  secure world that need to be processed by the normal world OS.
  **/

/*
 * Copyright (c) 2018, Microsoft Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <ntddk.h>
#include <ntstatus.h>
#include <ntstrsafe.h>
#include <initguid.h>
#include <sffdisk.h>
#include <sddef.h>
#include <wdf.h>
#include <TrustedRuntimeClx.h>
#include <TreeRpmbService.h>

#include "OpteeClientLib.h"
#include "OpteeClientRPC.h"
#include "OpteeClientMemory.h"
#include <TrEEGenService.h>
#include "trace.h"

#ifdef WPP_TRACING
#include "OpteeClientRpc.tmh"
#endif

// Taken from the optee_os implementation.
//
#include "teesmc.h"
#include "teesmc_optee.h"
#include "tee_rpc_types.h"
#include "tee_rpc.h"

//
// Imports
//

BOOLEAN
RtlTimeToSecondsSince1970(
    __in PLARGE_INTEGER Time,
    __out PULONG ElapsedSeconds
    );

TEEC_Result
GenServiceReceiveRpcCommand(
    UINT32 SessionId,
    ULONG RpcKey,
    ULONG RpcType,
    _In_reads_bytes_(RpcInputBufferSize) PVOID RpcInputBuffer,
    SIZE_T RpcInputBufferSize,
    _Out_writes_bytes_(RpcOutputBufferSize) PVOID RpcOutputBuffer,
    SIZE_T RpcOutputBufferSize
    );


#define OPTEE_CLIENT_RPC_WAIT_BLOCK_TAG 'wTPO'

#define NANOS_PER_SEC (1000LL * 1000LL * 1000LL)
#define HUNDRED_NANOS_TO_SEC (NANOS_PER_SEC / 100LL)

#define OS_TA_FILE_PATH_W_MAX_LENGTH 100

#define OPTEE_RPC_MEMORY_TAG 'ETPO'
#define OPTEE_RPC_CID_QUERY_TAG 'diCO'

#define RPMB_PACKET_DATA_TO_UINT16(d) ((d[0] << 8) + (d[1]))


//
// RPMB CID information
//

static UINT8 RpmbCid[RPMB_EMMC_CID_SIZE];
static BOOLEAN RpmbCidAquired = FALSE;

//
// Wait object resources
//

static WDFLOOKASIDE WaitBlockPool = NULL;
static WDFCOLLECTION WaitBlockList = NULL;
static FAST_MUTEX WaitBlockListLock;

//
// OPTEE sync object
//

typedef struct _OPTEE_WAIT_BLOCK
{
    WDFMEMORY Handle;
    uint64_t Key;
    KEVENT Event;
} OPTEE_WAIT_BLOCK, *POPTEE_WAIT_BLOCK;


static TEEC_Result OpteeRpcAlloc(UINTN Size, UINT64 *Address);
static TEEC_Result OpteeRpcFree(UINT64 Address);

static TEEC_Result OpteeRpcCmdLoadTa(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdRpmb(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdFs(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdGetTime(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdWaitQueue(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdSuspend(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdOcall(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdSharedMemAlloc(t_teesmc_arg *TeeSmcArg);
static TEEC_Result OpteeRpcCmdSharedMemFree(t_teesmc_arg *TeeSmcArg);

/*
 * Initialize RPC resources.
 */
NTSTATUS OpteeClientRpcInit()
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    WDFDRIVER WdfDriver;

    WdfDriver = WdfGetDriver();

    if (WaitBlockPool != NULL) {
        NT_ASSERT(WaitBlockList != NULL);
        return STATUS_SUCCESS;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = WdfDriver;

    Status = WdfCollectionCreate(&Attributes, &WaitBlockList);

    if (!NT_SUCCESS(Status)) {
        TraceError("WdfCollectionCreate failed, status %!STATUS!", Status);
        return Status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = WdfDriver;

    Status = WdfLookasideListCreate(
        &Attributes,
        sizeof(OPTEE_WAIT_BLOCK),
        NonPagedPool,
        WDF_NO_OBJECT_ATTRIBUTES,
        OPTEE_CLIENT_RPC_WAIT_BLOCK_TAG,
        &WaitBlockPool);

    if (!NT_SUCCESS(Status)) {
        TraceError(
            "Failed to create wait objects lookaside list, %!STATUS!",
            Status);

        return Status;
    }

    ExInitializeFastMutex(&WaitBlockListLock);

    return STATUS_SUCCESS;
}

/*
 * Handle the callback from secure world.
 */
TEEC_Result OpteeRpcCallback(ARM_SMC_ARGS *ArmSmcArgs)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;

    ASSERT(TEESMC_RETURN_IS_RPC(ArmSmcArgs->Arg0));

    TraceDebug("Enter: Arg0=0x%IX, Arg1=0x%IX, Arg2=0x%IX\n",
                ArmSmcArgs->Arg0,
                ArmSmcArgs->Arg1,
                ArmSmcArgs->Arg2);

    switch (TEESMC_RETURN_GET_RPC_FUNC(ArmSmcArgs->Arg0)) {

    // Supported RPC calls that need action.
    //

    case TEESMC_RPC_FUNC_ALLOC_ARG: {

        UINT64 Pa;

        TeecResult = OpteeRpcAlloc(ArmSmcArgs->Arg1, &Pa);

        // Used the physical address as the memory cookie.
        // Expected as 64bit in Arg4 (upper), Arg5 (lower)
        //
        if (TeecResult == TEEC_SUCCESS) {
            ArmSmcArgs->Arg1 = (UINTN)(Pa >> 32);
            ArmSmcArgs->Arg2 = (UINTN)(Pa & 0xFFFFFFFF);
            ArmSmcArgs->Arg4 = (UINTN)(Pa >> 32);
            ArmSmcArgs->Arg5 = (UINTN)(Pa & 0xFFFFFFFF);
        }
        break;
    }

    case TEESMC_RPC_FUNC_ALLOC_PAYLOAD: {

        UINT64 Pa;

        TeecResult = OpteeRpcAlloc(ArmSmcArgs->Arg1, &Pa);

        // Used the physical address as the memory cookie.
        // Expected as 64bit in Arg4 (upper), Arg5 (lower)
        //
        if (TeecResult == TEEC_SUCCESS) {
            ArmSmcArgs->Arg1 = (UINTN)(Pa >> 32);
            ArmSmcArgs->Arg2 = (UINTN)(Pa & 0xFFFFFFFF);
            ArmSmcArgs->Arg4 = (UINTN)(Pa >> 32);
            ArmSmcArgs->Arg5 = (UINTN)(Pa & 0xFFFFFFFF);
        }
        break;
    }

    case TEESMC_RPC_FUNC_FREE_ARG: {

        TeecResult = OpteeRpcFree((UINT64)ArmSmcArgs->Arg1 << 32 | (UINT64)ArmSmcArgs->Arg2);
        break;
    }

    case TEESMC_RPC_FUNC_FREE_PAYLOAD: {

        TeecResult = OpteeRpcFree((UINT64)ArmSmcArgs->Arg1 << 32 | (UINT64)ArmSmcArgs->Arg2);
        break;
    }

    // The IRQ would have been processed on return to normal world
    // so there is no further action to take other than to return.
    //
    case TEESMC_RPC_FUNC_IRQ: {

        break;
    }

    // This actually means an arg parameter block has come back that breaks
    // down further into various other commands.
    //
    case TEESMC_RPC_FUNC_CMD: {

        // Arg1 returned in the SMC call is Physical memory.
        //
        t_teesmc_arg *TeeSmcArg = (t_teesmc_arg *)OpteeClientPhysicalToVirtual((UINT64)ArmSmcArgs->Arg1 << 32 | (UINT64)ArmSmcArgs->Arg2);

        ASSERT(TeeSmcArg != NULL);

        TraceInformation("RPC CMD: cmd=0x%X, session=0x%X, num_params=%d, \n",
                         TeeSmcArg->cmd,
                         TeeSmcArg->session,
                         TeeSmcArg->num_params);

        switch (TeeSmcArg->cmd) {

        case TEE_RPC_CMD_LOAD_TA: {

            TeecResult = OpteeRpcCmdLoadTa(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_RPMB: {

            TeecResult = OpteeRpcCmdRpmb(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_FS: {

            TeecResult = OpteeRpcCmdFs(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_GET_TIME: {

            TeecResult = OpteeRpcCmdGetTime(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_SHM_ALLOC: {

            TeecResult = OpteeRpcCmdSharedMemAlloc(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_SHM_FREE: {

            TeecResult = OpteeRpcCmdSharedMemFree(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_WAIT_QUEUE: {

            TeecResult = OpteeRpcCmdWaitQueue(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_SUSPEND: {

            TeecResult = OpteeRpcCmdSuspend(TeeSmcArg);
            break;
        }

        case TEE_RPC_CMD_OCALL: {

            TeecResult = OpteeRpcCmdOcall(TeeSmcArg);
            break;
        }

        default: {

            TraceWarning("WRN: Unsupported RPC CMD: cmd=0x%X\n", TeeSmcArg->cmd);

            TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
            ASSERT(0);
            break;
        }
        }

        TeeSmcArg->ret = TeecResult;
        break;
    }

    // Additional supported RPC calls that need action.
    //

    case TEESMC_OPTEE_RPC_FUNC_ALLOC_PAYLOAD: {

        UINT64 Pa;

        TeecResult = OpteeRpcAlloc(ArmSmcArgs->Arg1, &Pa);

        // Used the physical address as the memory cookie.
        // Expected as 64bit in Arg4 (upper), Arg5 (lower)
        //
        if (TeecResult == TEEC_SUCCESS) {
            ArmSmcArgs->Arg1 = (UINTN)(Pa >> 32);
            ArmSmcArgs->Arg2 = (UINTN)(Pa & 0xFFFFFFFF);
            ArmSmcArgs->Arg4 = (UINTN)(Pa >> 32);
            ArmSmcArgs->Arg5 = (UINTN)(Pa & 0xFFFFFFFF);
        }
        break;
    }

    case TEESMC_OPTEE_RPC_FUNC_FREE_PAYLOAD: {

        TeecResult = OpteeRpcFree((UINT64)ArmSmcArgs->Arg1 << 32 | (UINT64)ArmSmcArgs->Arg2);
        break;
    }

    // Unsupported SMC calls
    //
    default: {

        TraceWarning("WRN: Unsupported RPC : Arg0=0x%IX\n", ArmSmcArgs->Arg0);

        TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
        ASSERT(0);
        break;
    }
    }

    // Send back the return code for the next call.
    //

    ArmSmcArgs->Arg0 = TEESMC32_CALL_RETURN_FROM_RPC;

    TraceDebug("Exit : TeecResult=0x%X\n", TeecResult);

    return TeecResult;
}


/*
 * Memory allocation.
 * Currently treated the same for both arguments & payloads.
 */
TEEC_Result OpteeRpcAlloc(UINTN Size, UINT64 *Address)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;

    *Address = 0;

    // The comment says to return 0 if the size was 0.
    // The memory allocated should be from the OP-TEE OS shared pool.
    //
    if (Size != 0) {
        PVOID AllocAddress = 0;
        NTSTATUS Status = OpteeClientMemAlloc((UINT32)Size, (PVOID*)&AllocAddress, NULL);
        if (AllocAddress == 0 || !NT_SUCCESS(Status)) {
            TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
        }
        else {

            // RPC Alloc is called from SMC call. We need to return the Physical address.
            //
            *Address = (UINT64)OpteeClientVirtualToPhysical(AllocAddress);
        }
    }
    return TeecResult;
}


/*
 * Memory free.
 * Currently treated the same for both arguments & payloads.
 */
TEEC_Result OpteeRpcFree(UINT64 Address)
{
    // The memory that we get from Optee is physical. Convert the memory to
    // Virtual address.
    //

    PVOID Va;
    Va = OpteeClientPhysicalToVirtual(Address);
    OpteeClientMemFree(Va);
    return TEEC_SUCCESS;
}

/*
 * Load a TA from storage into memory and provide it back to OP-TEE.
 * Param[0].value = IN: TA UUID
 * Param[1].tmem = OUT: TA Image allocated. If size is 0, return the required 
 *                      size.
 */
TEEC_Result OpteeRpcCmdLoadTa(t_teesmc_arg *TeeSmcArg)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    t_teesmc_param *TeeSmcParam = NULL;
    TEE_UUID TaUuid;
    PVOID TaBuffer;
    UINT64 TaBufferSize;

    // TA-load related-variables
    ULONG OsTaFileImageSize = 0;
    HANDLE OsTaFileHandle = NULL;
    OBJECT_ATTRIBUTES OsTaFileObjectAttributes;
    IO_STATUS_BLOCK OsTaFileIoStatus;
    FILE_STANDARD_INFORMATION OsTaFileInformation;
    LARGE_INTEGER OsTaByteOffset;
    WCHAR OsTaFilePathW[OS_TA_FILE_PATH_W_MAX_LENGTH];
    UNICODE_STRING OsTaFilePathU;

    if (TeeSmcArg->num_params != 2) {

        ASSERT(TeeSmcArg->num_params == 2);
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_VALUE_INPUT) ||
        (TeeSmcParam[1].attr != TEESMC_ATTR_TYPE_TMEM_OUTPUT)) {

        ASSERT(TeeSmcParam[0].attr == TEESMC_ATTR_TYPE_VALUE_INPUT);
        ASSERT(TeeSmcParam[1].attr == TEESMC_ATTR_TYPE_TMEM_OUTPUT);
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    RtlCopyMemory(&TaUuid, &TeeSmcParam[0].u.value,sizeof(TEEC_UUID));
    TaBufferSize = TeeSmcParam[1].u.tmem.size;

    TraceDebug("OpteeRpcCmdLoadTa Enter: buffer size=%d\n", 
        (UINT32)TaBufferSize);

    // OPTEE swaps UUID DWORD, and SHORTs, swap back... 
    //
    TaUuid.timeLow = SWAP_B32(TaUuid.timeLow);
    TaUuid.timeMid = SWAP_B16(TaUuid.timeMid);
    TaUuid.timeHiAndVersion = SWAP_B16(TaUuid.timeHiAndVersion);

    TraceDebug("TA uuid=%8.8X-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\n",
        TaUuid.timeLow, TaUuid.timeMid, TaUuid.timeHiAndVersion,
        TaUuid.clockSeqAndNode[0], TaUuid.clockSeqAndNode[1],
        TaUuid.clockSeqAndNode[2], TaUuid.clockSeqAndNode[3],
        TaUuid.clockSeqAndNode[4], TaUuid.clockSeqAndNode[5],
        TaUuid.clockSeqAndNode[6], TaUuid.clockSeqAndNode[7]);

    // Build the full TA path
    //
    Status = RtlStringCbPrintfW(
        OsTaFilePathW,
        sizeof(OsTaFilePathW),
        L"\\SystemRoot\\System32\\TrustedAppStore\\%8.8X-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X.ta",
        TaUuid.timeLow, TaUuid.timeMid, TaUuid.timeHiAndVersion,
        TaUuid.clockSeqAndNode[0], TaUuid.clockSeqAndNode[1],
        TaUuid.clockSeqAndNode[2], TaUuid.clockSeqAndNode[3],
        TaUuid.clockSeqAndNode[4], TaUuid.clockSeqAndNode[5],
        TaUuid.clockSeqAndNode[6], TaUuid.clockSeqAndNode[7]);

    if (!NT_SUCCESS(Status)) {
        TraceError("ERR: TA path build failed 0x%x\n", Status);
        TeecResult = TEEC_ERROR_GENERIC;
        goto Exit;
    }

    RtlInitUnicodeString(&OsTaFilePathU, OsTaFilePathW);

    InitializeObjectAttributes(
        &OsTaFileObjectAttributes,       // InitializedAttributes
        &OsTaFilePathU,                  // ObjectName
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, // Attributes
        NULL,                            // RootDirectory
        NULL);                           // SecurityDescriptor

    //
    // For containers: ensure that the openfile is executed in container host silo
    // so that we see the host's trusted app store
    //
    PESILO PreviousSilo = PsAttachSiloToCurrentThread(PsGetHostSilo());


    Status = ZwOpenFile(
        &OsTaFileHandle,                 // FileHandle
        GENERIC_READ,                    // DesiredAccess
        &OsTaFileObjectAttributes,       // ObjectAttributes
        &OsTaFileIoStatus,               // IoStatusBlock
        FILE_SHARE_READ,                 // ShareAccess
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT); // OpenOptions

    //
    // Restore previous container silo
    //
    PsDetachSiloFromCurrentThread(PreviousSilo);
    
    if (!NT_SUCCESS(Status)) {
        TraceError("ERR: TA file open failed 0x%x\n", Status);
        TeecResult = TEEC_ERROR_GENERIC;
        goto Exit;
    }

    Status = ZwQueryInformationFile(
        OsTaFileHandle,
        &OsTaFileIoStatus,
        &OsTaFileInformation,
        sizeof(OsTaFileInformation),
        FileStandardInformation);

    if (!NT_SUCCESS(Status)) {
        TraceError("ERR: TA file query info failed 0x%x\n", Status);
        TeecResult = TEEC_ERROR_GENERIC;
        goto Exit;
    }

    if (OsTaFileInformation.EndOfFile.HighPart != 0) {
        TraceError("ERR: TA file excess data failure.\n");
        TeecResult = TEEC_ERROR_EXCESS_DATA;
        goto Exit;
    }

    OsTaFileImageSize = (UINT64)OsTaFileInformation.EndOfFile.LowPart;

    // Make sure the OPTEE buffer is large enough for the TA image,
    // if not, let OPTEE know the required size.
    //
    if ((ULONG)TaBufferSize < OsTaFileImageSize) {

        // This is not an error, usually OPTEE uses the 1st call, 
        // to figure out the file size.
        //
        TeeSmcParam[1].u.tmem.size = OsTaFileImageSize;
        TeecResult = TEEC_SUCCESS;
        goto Exit;
    }
    
    TaBuffer = OpteeClientPhysicalToVirtual(TeeSmcParam[1].u.tmem.buf_ptr);

    if (TaBuffer == NULL) {
        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    OsTaByteOffset.HighPart = 0;
    OsTaByteOffset.LowPart = 0;

    Status = ZwReadFile(
        OsTaFileHandle,     // FileHandle
        NULL,               // Event
        NULL,               // ApcRoutine
        NULL,               // ApcContext
        &OsTaFileIoStatus,  // IoStatusBlock
        TaBuffer,           // Buffer
        OsTaFileImageSize,  // Length
        &OsTaByteOffset,    // ByteOffset
        NULL);              // Key
      
    if (!NT_SUCCESS(Status)) {
        TraceError("ERR: TA file read failed 0x%x\n", Status);
        TeecResult = TEEC_ERROR_GENERIC;
        goto Exit;
    }
    
    TraceDebug("TA loaded at VA/PA 0x%p / 0x%llx of size %d bytes\n",
        TaBuffer, 
        TeeSmcParam[1].u.tmem.buf_ptr, 
        OsTaFileImageSize);

Exit:
    TeeSmcArg->ret = TeecResult;
    TeeSmcArg->ret_origin = TEEC_ORIGIN_API;

    if (OsTaFileHandle != NULL) {
        ZwClose(OsTaFileHandle);
    }

    TraceDebug("OpteeRpcCmdLoadTa Exit : TeecResult=0x%X, Status=0x%X\n", 
        TeecResult, 
        Status);

    return TeecResult;
}


/*
 * Execute an RPMB storage operation.
 */
TEEC_Result OpteeRpcCmdRpmb(t_teesmc_arg *TeeSmcArg)
{
    ULONG AllocSize;
    ULONG BlockCount;
    ULONG_PTR BytesWritten;
    SFFDISK_DEVICE_RPMB_DATA_FRAME *Frame;
    ULONG InputSize;
    ULONG OutputSize;
    ULONGLONG Physical;
    UINT16 RequestMsgType;
    SFFDISK_DEVICE_RPMB_DATA_FRAME *RequestPackets;
    SFFDISK_DEVICE_RPMB_DATA_FRAME *ResponsePackets;
    tee_rpc_rpmb_cmd_t *RpmbRequest;
    NTSTATUS Status;
    TEEC_Result TeecResult;
    t_teesmc_param *TeeSmcParam;
    SFFDISK_DEVICE_PARTITION_ACCESS_DATA *TreeData;
    TR_SERVICE_REQUEST TreeRequest;

    if (TeeSmcArg->num_params != 2) {

        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    TeecResult = TEEC_SUCCESS;
    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_TMEM_INPUT) ||
        (TeeSmcParam[1].attr != TEESMC_ATTR_TYPE_TMEM_OUTPUT)) {

        TeecResult = TEEC_ERROR_BAD_PARAMETERS;
        goto Exit;
    }

    Physical = TeeSmcParam[0].u.tmem.buf_ptr;
    RpmbRequest = (tee_rpc_rpmb_cmd_t *)OpteeClientPhysicalToVirtual(Physical);
    TreeData = NULL;
    BytesWritten = 0;

    switch (RpmbRequest->cmd) {
        case TEE_RPC_RPMB_CMD_DATA_REQ: {
            RequestPackets = (SFFDISK_DEVICE_RPMB_DATA_FRAME *)
                                (RpmbRequest + 1);

            Physical = TeeSmcParam[1].u.tmem.buf_ptr;
            ResponsePackets = (SFFDISK_DEVICE_RPMB_DATA_FRAME *)
                                OpteeClientPhysicalToVirtual(Physical);

            ASSERT(ResponsePackets != NULL);

            RequestMsgType = RPMB_PACKET_DATA_TO_UINT16(
                                RequestPackets->RequestOrResponseType);

            TraceInformation("RPMB Data request %d\n", RequestMsgType);

            switch (RequestMsgType) {
                case SFFDISK_RPMB_PROGRAM_AUTH_KEY: {
                    InputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    OutputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    AllocSize = InputSize;
                    TreeData = ExAllocatePoolWithTag(PagedPool,
                                                     AllocSize,
                                                     OPTEE_RPC_MEMORY_TAG);

                    if (TreeData == NULL) {
                        TraceError("ERR: Could not allocate requested size %d\n",
                                   AllocSize);

                        TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    TreeRequest.FunctionCode = TREE_RPMB_SFFDISK_PARTITION_ACCESS;
                    TreeRequest.InputBuffer = TreeData;
                    TreeRequest.InputBufferSize = InputSize;
                    TreeRequest.OutputBuffer = TreeData;
                    TreeRequest.OutputBufferSize = OutputSize;
                    TreeData->Command = SFFDISK_RPMB_PROGRAM_AUTH_KEY;
                    TreeData->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    Frame = &TreeData->Parameters.RpmbProgramAuthKey.ProgramAuthKeyFrame;
                    RtlCopyMemory(Frame,
                                  RequestPackets,
                                  sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME));

                    Status = TrSecureDeviceCallOSService(LibServiceDevice,
                                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                                         &TreeRequest,
                                                         &BytesWritten);

                    if (!NT_SUCCESS(Status)) {
                        TraceError("ERR: RpmbQueryWriteCounter failed 0x%x\n",
                                   Status);

                        TeecResult = TEEC_ERROR_GENERIC;
                        break;
                    }

                    Frame = &TreeData->Parameters.RpmbProgramAuthKey.ResultFrame;
                    RtlCopyMemory(ResponsePackets,
                                  Frame,
                                  sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME));

                    break;
                }

                case SFFDISK_RPMB_QUERY_WRITE_COUNTER: {
                    InputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    OutputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    AllocSize = InputSize;
                    TreeData = ExAllocatePoolWithTag(PagedPool,
                                                     AllocSize,
                                                     OPTEE_RPC_MEMORY_TAG);

                    if (TreeData == NULL) {
                        TraceError("ERR: Could not allocate requested size %d\n",
                                   AllocSize);

                        TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    TreeRequest.FunctionCode = TREE_RPMB_SFFDISK_PARTITION_ACCESS;
                    TreeRequest.InputBuffer = TreeData;
                    TreeRequest.InputBufferSize = InputSize;
                    TreeRequest.OutputBuffer = TreeData;
                    TreeRequest.OutputBufferSize = OutputSize;
                    TreeData->Command = SFFDISK_RPMB_QUERY_WRITE_COUNTER;
                    TreeData->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    Frame = &TreeData->Parameters.RpmbQueryWriteCounter.QueryWriteCounterFrame;
                    RtlCopyMemory(Frame,
                                  RequestPackets,
                                  sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME));

                    Status = TrSecureDeviceCallOSService(LibServiceDevice,
                                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                                         &TreeRequest,
                                                         &BytesWritten);                    

                    if (!NT_SUCCESS(Status)) {
                        TraceError("ERR: RpmbQueryWriteCounter failed 0x%x\n",
                                   Status);

                        TeecResult = TEEC_ERROR_GENERIC;
                        break;
                    }

                    Frame = &TreeData->Parameters.RpmbQueryWriteCounter.ResultFrame;
                    RtlCopyMemory(ResponsePackets,
                                  Frame,
                                  sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME));

                    break;
                }

                case SFFDISK_RPMB_AUTHENTICATED_WRITE: {
                    OutputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);

                    // The input size is block_count * the size of a
                    // frame/block, however the ACCESS_DATA already contains
                    // one frame.
                    //

                    BlockCount = RPMB_PACKET_DATA_TO_UINT16(
                                    RequestPackets->BlockCount);

                    InputSize = sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME);
                    InputSize *= (BlockCount - 1);
                    InputSize += sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);

                    // Since the ACCESS_DATA contains the request and response
                    // frames then the allocation size is always the larger
                    // of the two sizes.
                    //

                    AllocSize = InputSize;
                    TreeData = ExAllocatePoolWithTag(PagedPool,
                                                     AllocSize,
                                                     OPTEE_RPC_MEMORY_TAG);

                    if (TreeData == NULL) {
                        TraceError("ERR: Could not allocate requested size %d\n",
                                   AllocSize);

                        TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    TreeRequest.FunctionCode = TREE_RPMB_SFFDISK_PARTITION_ACCESS;
                    TreeRequest.InputBuffer = TreeData;
                    TreeRequest.InputBufferSize = InputSize;
                    TreeRequest.OutputBuffer = TreeData;
                    TreeRequest.OutputBufferSize = OutputSize;
                    TreeData->Command = SFFDISK_RPMB_AUTHENTICATED_WRITE;
                    TreeData->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    TreeData->Parameters.RpmbAuthenticatedWrite.CountToWrite =
                        BlockCount;

                    InputSize = BlockCount * sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME);
                    Frame = TreeData->Parameters.RpmbAuthenticatedWrite.FrameDataToWrite;
                    RtlCopyMemory(Frame,
                                  RequestPackets,
                                  InputSize);

                    Status = TrSecureDeviceCallOSService(LibServiceDevice,
                                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                                         &TreeRequest,
                                                         &BytesWritten);                    

                    if (!NT_SUCCESS(Status)) {
                        TraceError("ERR: RpmbAuthenticatedWrite failed 0x%x\n",
                                   Status);

                        TeecResult = TEEC_ERROR_GENERIC;
                        break;
                    }

                    Frame = &TreeData->Parameters.RpmbAuthenticatedWrite.ResultFrame;
                    RtlCopyMemory(ResponsePackets,
                                  Frame,
                                  sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME));

                    break;
                }

                case SFFDISK_RPMB_AUTHENTICATED_READ: {

                    // The output size is block_count * the size of a
                    // frame/block, however the ACCESS_DATA already contains
                    // one frame.
                    //

                    BlockCount = RpmbRequest->block_count;
                    OutputSize = sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME);
                    OutputSize *= (BlockCount - 1);
                    OutputSize += sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);

                    // There seems to be a bug with the sdstor driver that
                    // requires InputSize = Output size for read.
                    //
                    
                    //InputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    InputSize = OutputSize;

                    // Since the ACCESS_DATA contains the request and response
                    // frames then the allocation size is always the larger
                    // of the two sizes.
                    //

                    AllocSize = OutputSize;
                    TreeData = ExAllocatePoolWithTag(PagedPool,
                                                     AllocSize,
                                                     OPTEE_RPC_MEMORY_TAG);

                    if (TreeData == NULL) {
                        TraceError("ERR: Could not allocate requested size %d\n",
                                   AllocSize);

                        TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    TreeRequest.FunctionCode = TREE_RPMB_SFFDISK_PARTITION_ACCESS;
                    TreeRequest.InputBuffer = TreeData;
                    TreeRequest.InputBufferSize = InputSize;
                    TreeRequest.OutputBuffer = TreeData;
                    TreeRequest.OutputBufferSize = OutputSize;
                    TreeData->Command = SFFDISK_RPMB_AUTHENTICATED_READ;
                    TreeData->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
                    TreeData->Parameters.RpmbAuthenticatedRead.CountToRead = BlockCount;
                    
                    Frame = &TreeData->Parameters.RpmbAuthenticatedRead.AuthenticatedReadFrame;
                    RtlCopyMemory(Frame,
                                  RequestPackets,
                                  sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME));

                    Status = TrSecureDeviceCallOSService(LibServiceDevice,
                                                         &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                                         &TreeRequest,
                                                         &BytesWritten);                    

                    if (!NT_SUCCESS(Status)) {
                        TraceError("ERR: RpmbAuthenticatedRead failed 0x%x\n",
                                     Status);

                        TeecResult = TEEC_ERROR_GENERIC;
                        break;
                    }

                    OutputSize = BlockCount * sizeof(SFFDISK_DEVICE_RPMB_DATA_FRAME);
                    Frame = TreeData->Parameters.RpmbAuthenticatedRead.ReturnedFrameData;
                    RtlCopyMemory(ResponsePackets, Frame, OutputSize);
                    break;
                }

                default:
                    TeecResult = TEEC_ERROR_BAD_PARAMETERS;
                    break;
            }

            if (TreeData != NULL) {
                ExFreePool(TreeData);
            }

            TraceInformation("Exit TeecResult %d\n", TeecResult);

            break;
        }

        case TEE_RPC_RPMB_CMD_GET_DEV_INFO: {
            
            tee_rpc_rpmb_dev_info* DeviceInfo;

            TraceInformation("RPMB Get Device Information request\n");

            if (!RpmbCidAquired) {
                NT_ASSERT(RpmbCidAquired);
                TeecResult = TEEC_ERROR_BAD_STATE;
                break;
            }

            Physical = TeeSmcParam[1].u.tmem.buf_ptr;
            DeviceInfo = (tee_rpc_rpmb_dev_info *)
                OpteeClientPhysicalToVirtual(Physical);

            InputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
            OutputSize = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);
            AllocSize = InputSize;
            TreeData = ExAllocatePoolWithTag(PagedPool,
                                             AllocSize,
                                             OPTEE_RPC_MEMORY_TAG);

            if (TreeData == NULL) {
                TraceError("ERR: Could not allocate requested size %d\n",
                           AllocSize);

                TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
                break;
            }

            TreeRequest.FunctionCode = TREE_RPMB_SFFDISK_PARTITION_ACCESS;
            TreeRequest.InputBuffer = TreeData;
            TreeRequest.InputBufferSize = InputSize;
            TreeRequest.OutputBuffer = TreeData;
            TreeRequest.OutputBufferSize = OutputSize;
            TreeData->Command = SFFDISK_RPMB_IS_SUPPORTED;
            TreeData->Size = sizeof(SFFDISK_DEVICE_PARTITION_ACCESS_DATA);

            Status = TrSecureDeviceCallOSService(LibServiceDevice,
                                                 &GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
                                                 &TreeRequest,
                                                 &BytesWritten);

            if (!NT_SUCCESS(Status)) {
                TraceError("ERR: RpmbSupported failed 0x%x\n",
                           Status);
                
                TeecResult = TEEC_ERROR_GENERIC;
                DeviceInfo->ret_code = TEE_RPC_RPMB_CMD_GET_DEV_INFO_RET_ERROR;

            } else {
                DeviceInfo->rel_wr_sec_c = (uint8_t)(TreeData->Parameters.RpmbIsSupported.MaxReliableWriteSizeInBytes / 512);
                DeviceInfo->rpmb_size_mult = (uint8_t)(TreeData->Parameters.RpmbIsSupported.SizeInBytes / (128 * 1024));
                RtlCopyMemory(DeviceInfo->cid, RpmbCid, RPMB_EMMC_CID_SIZE);
                DeviceInfo->ret_code = TEE_RPC_RPMB_CMD_GET_DEV_INFO_RET_OK;

                TeecResult = TEEC_SUCCESS;
            }

            DeviceInfo->ret_code = (uint8_t)TeecResult;
            ExFreePoolWithTag(TreeData, OPTEE_RPC_MEMORY_TAG);
            break;
        }

        default:
            TeecResult = TEEC_ERROR_BAD_PARAMETERS;
            break;
    }

Exit:

    TeeSmcArg->ret = TeecResult;
    TeeSmcArg->ret_origin = TEEC_ORIGIN_API;
    return TeecResult;
}


/*
 * Execute a normal world local file system operation.
 */
TEEC_Result OpteeRpcCmdFs(t_teesmc_arg *TeeSmcArg)
{
    UNREFERENCED_PARAMETER(TeeSmcArg);
    return TEEC_ERROR_NOT_IMPLEMENTED;
}


/*
 * Get current time.
 * Time is returned in Param[0].value:
 * Param[0].value.a = elapsed seconds
 * Param[0].value.b = elapsed nano-seconds
 */
TEEC_Result OpteeRpcCmdGetTime(t_teesmc_arg *TeeSmcArg)
{
    LARGE_INTEGER CurrentTime100Nanos;
    UINT64 ElapsedSeconds;
    UINT64 ElapsedNanos;
    ULONG ElapsedSecondsSinc1970;
    t_teesmc_param *TeeSmcParam;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcArg->num_params != 1) ||
        (TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_VALUE_OUTPUT)) {

        return TEEC_ERROR_BAD_PARAMETERS;
    }

    // System time is a count of 100-nanosecond intervals 
    // since January 1, 1601.

    KeQuerySystemTime(&CurrentTime100Nanos);

    // Convert to seconds since January 1 1970
    //

    RtlTimeToSecondsSince1970(&CurrentTime100Nanos, &ElapsedSecondsSinc1970);

    ElapsedSeconds = CurrentTime100Nanos.QuadPart / HUNDRED_NANOS_TO_SEC;
    ElapsedNanos =
        (CurrentTime100Nanos.QuadPart - ElapsedSeconds * HUNDRED_NANOS_TO_SEC)
        * 100LL;

    ASSERT(ElapsedNanos < NANOS_PER_SEC);

    TeeSmcParam[0].u.value.a = (uint64_t)ElapsedSecondsSinc1970;
    TeeSmcParam[0].u.value.b = ElapsedNanos;

    return TEEC_SUCCESS;
}

/*
 * Allocate and initialize a wait block object.
 *
 * On Entry:
 *  Key: Value identifying the queue to wait for.
 *
 */
static POPTEE_WAIT_BLOCK OpteeRpcWaitBlockAlloc(UINT64 Key)
{
    NTSTATUS Status;
    POPTEE_WAIT_BLOCK WaitBlock;
    WDFMEMORY WaitBlockObject;

    Status = WdfMemoryCreateFromLookaside(WaitBlockPool, &WaitBlockObject);
    if (!NT_SUCCESS(Status)) {
        TraceError("WdfMemoryCreateFromLookaside failed, status %!STATUS!\n",
                   Status);

        return NULL;
    }

    WaitBlock = (POPTEE_WAIT_BLOCK)WdfMemoryGetBuffer(WaitBlockObject, NULL);
    WaitBlock->Handle = WaitBlockObject;
    WaitBlock->Key = Key;
    KeInitializeEvent(&WaitBlock->Event, SynchronizationEvent, FALSE);

    Status = WdfCollectionAdd(WaitBlockList, WaitBlockObject);
    if (!NT_SUCCESS(Status)) {
        TraceError("WdfCollectionAdd failed, status %!STATUS!\n",
                   Status);

        WdfObjectDelete(WaitBlockObject);
        WaitBlock = NULL;
    }

    return WaitBlock;
}

/*
 * Find the block for a key, create the block if
 * it does not exist.
 *
 * On Entry:
 *  Key: Value identifying the queue to wait for.
 *
 */
static POPTEE_WAIT_BLOCK OpteeRpcGetWaitBlock(UINT64 Key)
{
    ULONG Index, ListCount;
    POPTEE_WAIT_BLOCK KeyWaitBlock = NULL;

    ExAcquireFastMutex(&WaitBlockListLock);

    ListCount = WdfCollectionGetCount(WaitBlockList);
    for (Index = 0; Index < ListCount; ++Index) {
        POPTEE_WAIT_BLOCK WaitBlock;

        WaitBlock = (POPTEE_WAIT_BLOCK)WdfMemoryGetBuffer(
            (WDFMEMORY)WdfCollectionGetItem(WaitBlockList, Index),
            NULL);

        if (WaitBlock->Key == Key) {
            KeyWaitBlock = WaitBlock;
            break;
        }
    }

    //
    // If the block has not been created yet, create it now.
    //
    if (KeyWaitBlock == NULL) {
        KeyWaitBlock = OpteeRpcWaitBlockAlloc(Key);
    }

    ExReleaseFastMutex(&WaitBlockListLock);

    return KeyWaitBlock;
}

/*
 * Free a wait block object.
 *
 * On Entry:
 *  WaitBlock: The wait block to be freed.
 *
 */
static VOID OpteeRpcWaitBlockFree(POPTEE_WAIT_BLOCK WaitBlock)
{
    ExAcquireFastMutex(&WaitBlockListLock);

    WdfCollectionRemove(WaitBlockList, WaitBlock->Handle);

    ExReleaseFastMutex(&WaitBlockListLock);

    WdfObjectDelete(WaitBlock->Handle);
}

/*
 * Wait until queue/synchronization object is signaled.
 *
 * On Entry:
 *  Key: Value identifying the queue to wait for.
 */
static TEEC_Result OpteeRpcCmdWaitQueueSleep(uint64_t Key)
{
    NTSTATUS Status;
    POPTEE_WAIT_BLOCK WaitBlock;

    TraceDebug("WaitQueueSleep: thread 0x%p, key = %llX\n", 
               KeGetCurrentThread(), 
               Key);

    //
    // Get or create a new block. The block may already exist
    // if the scheduler has inverted the order we receive the
    // wake/sleep RPC calls.
    //
    WaitBlock = OpteeRpcGetWaitBlock(Key);

    if (WaitBlock == NULL) {
        NT_ASSERT(FALSE);
        return TEEC_ERROR_OUT_OF_MEMORY;
    }

    Status = KeWaitForSingleObject(&WaitBlock->Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);

    NT_ASSERT(Status == STATUS_SUCCESS);

    TraceDebug("WaitQueueSleep: thread 0x%p, key = %llX, status %!STATUS!\n", 
               KeGetCurrentThread(), 
               Key, 
               Status);

    OpteeRpcWaitBlockFree(WaitBlock);

    return TEEC_SUCCESS;
}

/*
* Signal queue/synchronization object.
*
* On Entry:
*  Key: Unique value identifying the queue to signal for.
*
*/
static TEEC_Result OpteeRpcCmdWaitQueueWakeUp(uint64_t Key)
{
    POPTEE_WAIT_BLOCK WaitBlock;

    TraceDebug("WaitQueueWakeUp: thread 0x%p, key = %llX\n", 
               KeGetCurrentThread(),
               Key);

    //
    // Get or create a new block. The block may not exist
    // if the scheduler has inverted the order we receive the
    // wake/sleep RPC calls.
    //
    WaitBlock = OpteeRpcGetWaitBlock(Key);
    if (WaitBlock == NULL) {
        NT_ASSERT(FALSE);
        return TEEC_ERROR_OUT_OF_MEMORY;
    }

    TraceDebug("WaitQueueWakeUp: thread 0x%p, key = %llX, "
        "setting event 0x%p\n",
        KeGetCurrentThread(),
        Key,
        &WaitBlock->Event);
    KeSetEvent(&WaitBlock->Event, IO_NO_INCREMENT, FALSE);

    return TEEC_SUCCESS;
}

/*
 * Wait for or signal queue/synchronization object.
 *
 * On Entry:
 *  Params[0].attr: TEE_PARAM_TYPE_VALUE_INPUT;
 *  Params[0].u.value.a: 
 *      - OPTEE_MSG_RPC_WAIT_QUEUE_SLEEP
 *      - OPTEE_MSG_RPC_WAIT_QUEUE_WAKEUP
 *  Params[0].u.value.b: Mutex ID.
 *
 */
static TEEC_Result OpteeRpcCmdWaitQueue(t_teesmc_arg *TeeSmcArg)
{
    TEEC_Result Result;
    t_teesmc_param *TeeSmcParam;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcArg->num_params != 1) ||
        (TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_VALUE_INPUT)) {

        return TEEC_ERROR_BAD_PARAMETERS;
    }

    switch (TeeSmcParam[0].u.value.a) {
    case TEE_WAIT_QUEUE_SLEEP:
        Result = OpteeRpcCmdWaitQueueSleep(TeeSmcParam[0].u.value.b);
        break;

    case TEE_WAIT_QUEUE_WAKEUP:
        Result = OpteeRpcCmdWaitQueueWakeUp(TeeSmcParam[0].u.value.b);
        break;

    default:
        NT_ASSERT(FALSE);
        Result = TEEC_ERROR_BAD_PARAMETERS;
        break;
    }

    return Result;
}

/*
 * Suspend execution for a period in mSec.
 *
 * On Entry:
 *  Params[0].attr: TEE_PARAM_TYPE_VALUE_INPUT;
 *  Params[0].u.value.a: duration in mSec
 *
 */
static TEEC_Result OpteeRpcCmdSuspend(t_teesmc_arg *TeeSmcArg)
{
    t_teesmc_param *TeeSmcParam;
    LARGE_INTEGER Timeout;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcArg->num_params != 1) ||
        (TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_VALUE_INPUT)) {

        return TEEC_ERROR_BAD_PARAMETERS;
    }

    //
    // mSec -> 100 nSec
    //

    Timeout.QuadPart = -1 * TeeSmcParam[0].u.value.a * 1000LL * 10LL;

    KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

    return TEEC_SUCCESS;
}

/*
 * Process outbound call from OPTEE to normal world.
 *
 *  - Params[0].attr: TEE_PARAM_TYPE_VALUE_INPUT;
 *    Params[0].u.value.a: RPC type:
 *                         - GenSvcOutputTypeRpcCommand
 *    Params[0].u.value.b: TA sessions ID
 *    Params[0].u.value.c: RPC key, specified by the Normal World APP

 *  - Params[1].attr: TEESMC_ATTR_TYPE_TMEM_INPUT;
 *    Params[1].u.rmem: Input buffer (RPC command parameters).

 *  - Params[2].attr: TEESMC_ATTR_TYPE_TMEM_OUTPUT;
 *    Params[2].u.rmem: Output buffer (RPC command parameters).
 *
 */
TEEC_Result OpteeRpcCmdOcall(t_teesmc_arg *TeeSmcArg)
{
    PVOID RpcInputBuffer;
    SIZE_T RpcInputSize;
    PVOID RpcOutputBuffer;
    SIZE_T RpcOutputSize;
    t_teesmc_param *TeeSmcParam;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcArg->num_params != 3) ||
        (TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_VALUE_INPUT) ||
        (TeeSmcParam[1].attr != TEESMC_ATTR_TYPE_TMEM_INPUT) ||
        (TeeSmcParam[2].attr != TEESMC_ATTR_TYPE_TMEM_OUTPUT)) {

        NT_ASSERT(FALSE);
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    RpcInputBuffer = OpteeClientPhysicalToVirtual(TeeSmcParam[1].u.rmem.offs);
    RpcInputSize = (SIZE_T)TeeSmcParam[1].u.rmem.size;

    RpcOutputBuffer = OpteeClientPhysicalToVirtual(TeeSmcParam[2].u.rmem.offs);
    RpcOutputSize = (SIZE_T)TeeSmcParam[2].u.rmem.size;

    return GenServiceReceiveRpcCommand(
        (UINT32)TeeSmcParam[0].u.value.b,   // TA session ID.
        (ULONG)TeeSmcParam[0].u.value.c,    // RPC key, specified by the Normal World APP.
        (ULONG)TeeSmcParam[0].u.value.a,    // RPC type.
        RpcInputBuffer,
        RpcInputSize,
        RpcOutputBuffer,
        RpcOutputSize);
}


/*
 * Allocate shared memory with type and alignment.
 * On Entry:
 *  Params[0].attr: TEE_PARAM_TYPE_VALUE_INPUT;
 *  Params[0].u.value.a: Buffer type: 
 *                       - TEE_RPC_SHM_ALLOC_TYPE_APPL, or
 *                       - TEE_RPC_SHM_ALLOC_TYPE_HOST
 *                         Can be shared with a non-secure user space 
 *                         application.
 *                       - TEE_RPC_SHM_ALLOC_TYPE_KERNEL
 *                         Only shared with non-secure kernel.
 *  Params[0].u.value.b: Buffer size.
 *  Params[0].u.value.c: Buffer alignment.
 *
 * On Exit:
 *  Allocated buffer is returned in Param[0] as TEESMC_ATTR_TYPE_TMEM_OUTPUT
 */
TEEC_Result OpteeRpcCmdSharedMemAlloc(t_teesmc_arg *TeeSmcArg)
{
    UINT64 BufferAlignment;
    UINT64 BufferSize;
    UINT64 Pa;
    TEEC_Result TeecResult;
    t_teesmc_param *TeeSmcParam;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcArg->num_params != 1) ||
        (TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_VALUE_INPUT)) {

        return TEEC_ERROR_BAD_PARAMETERS;
    }

    // Currently we do not differentiate from application buffers and
    // kernel buffers, we only validate the parameters.
    //

    switch (TeeSmcParam[0].u.value.a) {
    case TEE_RPC_SHM_ALLOC_TYPE_APPL:
    case TEE_RPC_SHM_ALLOC_TYPE_HOST:
    case TEE_RPC_SHM_ALLOC_TYPE_KERNEL:
        break;

    default:
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    BufferSize = TeeSmcParam[0].u.value.b;
    BufferAlignment = TeeSmcParam[0].u.value.c;

    TeecResult = OpteeRpcAlloc((UINTN)(BufferSize + BufferAlignment), &Pa);

    if (TeecResult != TEEC_SUCCESS) {
        
        return TeecResult;
    }

    TeeSmcParam[0].attr = TEESMC_ATTR_TYPE_TMEM_OUTPUT;
    TeeSmcParam[0].u.tmem.buf_ptr = Pa & (~(BufferAlignment - 1));
    TeeSmcParam[0].u.tmem.size = BufferSize;
    TeeSmcParam[0].u.tmem.shm_ref = Pa; // Use PA as the cookie.

    return TEEC_SUCCESS;
}

/*
 * Free allocated shared memory with type.
 * On Entry:
 *  Params[0].attr: TEE_PARAM_TYPE_VALUE_INPUT;
 *  Params[0].u.value.a: Buffer type:
 *                       - TEE_RPC_SHM_ALLOC_TYPE_APPL, or
 *                       - TEE_RPC_SHM_ALLOC_TYPE_HOST
 *                         Can be shared with a non-secure user space
 *                         application.
 *                       - TEE_RPC_SHM_ALLOC_TYPE_KERNEL
 *                         Only shared with non-secure kernel.
 *  Params[0].u.value.b: Cookie.
 *  Params[0].u.value.c: 0.
 *
 */
TEEC_Result OpteeRpcCmdSharedMemFree(t_teesmc_arg *TeeSmcArg)
{
    t_teesmc_param *TeeSmcParam;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    if ((TeeSmcArg->num_params != 1) ||
        (TeeSmcParam[0].attr != TEESMC_ATTR_TYPE_VALUE_INPUT)) {

        return TEEC_ERROR_BAD_PARAMETERS;
    }

    // Currently we do not differentiate from application buffers and
    // kernel buffers, we only validate the parameters.
    //

    switch (TeeSmcParam[0].u.value.a) {
    case TEE_RPC_SHM_ALLOC_TYPE_APPL:
    case TEE_RPC_SHM_ALLOC_TYPE_HOST:
    case TEE_RPC_SHM_ALLOC_TYPE_KERNEL:
        break;

    default:
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    return OpteeRpcFree(TeeSmcParam[0].u.value.b);
}


/*
 * Get RPMB Cid completion routine
 */

typedef struct _OPTEE_GET_CID_CONTEXT
{
    size_t RequiredBufferSize;
    KEVENT CompletionEvent;
} OPTEE_GET_CID_CONTEXT, *POPTEE_GET_CID_CONTEXT;

VOID
OpteeRpcGetCidCompletionRoutine(
    _In_ WDFREQUEST WdfRequest,
    _In_ WDFIOTARGET WdfIoTarget,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS WdfReqCompletionParams,
    _In_ WDFCONTEXT WdfContext
    )
{
    POPTEE_GET_CID_CONTEXT CompletionContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(WdfIoTarget);

    CompletionContext = (POPTEE_GET_CID_CONTEXT)WdfContext;
    Status = WdfReqCompletionParams->IoStatus.Status;

    if (NT_SUCCESS(Status)) {
        size_t OutputBufferSize;
        PSFFDISK_DEVICE_COMMAND_DATA SffdiskCmdData;

        SffdiskCmdData = (PSFFDISK_DEVICE_COMMAND_DATA)WdfMemoryGetBuffer(
            WdfReqCompletionParams->Parameters.Ioctl.Output.Buffer,
            &OutputBufferSize);

        if ((SffdiskCmdData != NULL) &&
            (OutputBufferSize == CompletionContext->RequiredBufferSize)) {

            RtlCopyMemory(RpmbCid,
                          SffdiskCmdData->Data + sizeof(SDCMD_DESCRIPTOR),
                          sizeof(RpmbCid));
            
            RpmbCidAquired = TRUE;

        } else {
            NT_ASSERT(FALSE);
        }
    }

    WdfObjectDelete(WdfRequest);

    KeSetEvent(&CompletionContext->CompletionEvent,
               IO_NO_INCREMENT, 
               FALSE);

    // Unblock SMC calls
    //

    KeReleaseSemaphore(&OpteeSMCLock,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);
}


/*
 * An auxiliary function to get the RPMB Cid
 */
NTSTATUS 
OpteeRpcGetRpmbCid(
    _In_ WDFDEVICE WdfDevice,
    _In_ PUNICODE_STRING TargetSymbolicLink
    )
{
    WDFIOTARGET WdfIoTarget = NULL;
    NTSTATUS Status;

    if (RpmbCidAquired) {
        return TRUE;
    }

    // Create and open target that exposes the 
    // GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB interface
    //

    {
        WDF_OBJECT_ATTRIBUTES Attributes;
        WDF_IO_TARGET_OPEN_PARAMS OpenParams;

        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = WdfDevice;

        Status = WdfIoTargetCreate(WdfDevice,
                                   &Attributes,
                                   &WdfIoTarget);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams,
                                                    TargetSymbolicLink,
                                                    STANDARD_RIGHTS_READ);

        Status = WdfIoTargetOpen(WdfIoTarget, &OpenParams);

        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }
    }

    // Read the Cid:
    // We are using sdtor(eMMC)/IOCTL_SFFDISK_DEVICE_COMMAND / SFFDISK_DC_DEVICE_COMMAND to read
    // eMMC CID. 
    // Normally sdstor does not allow direct commands to be sent to the eMMC device, unless
    // the eMMC sdstor device HackFlags contain SDSTOR_HACK_FLAGS_ALLOW_DEVICE_COMMANDS (4).
    // Sdstor flags are located at SD\VID_<VID>&OID_0000&PID_<PID>&...\<Instance>\Device Parameters\HackFlags
    //
    // The more standard way to get CID information is to use: 
    // sdtor(eMMC)/IOCTL_STORAGE_QUERY_PROPERTY with StorageDeviceProperty/PropertyStandardQuery
    // But it only returns partial CID information.
    //

    {
        WDF_OBJECT_ATTRIBUTES Attributes;
        OPTEE_GET_CID_CONTEXT CompletionContext;
        size_t BufferSize;
        PSFFDISK_DEVICE_COMMAND_DATA SffdiskCmdData;
        PSDCMD_DESCRIPTOR SdCmdDescriptor;
        WDFREQUEST WdfRequest;
        WDFMEMORY WdfMemory;

        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = WdfIoTarget;

        Status = WdfRequestCreate(&Attributes,
                                  WdfIoTarget,
                                  &WdfRequest);

        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }

        BufferSize = sizeof(SFFDISK_DEVICE_COMMAND_DATA) +
                     sizeof(SDCMD_DESCRIPTOR) +
                     RPMB_EMMC_CID_SIZE;

        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = WdfRequest;

        Status = WdfMemoryCreate(&Attributes,
                                 NonPagedPool,
                                 OPTEE_RPC_CID_QUERY_TAG,
                                 BufferSize,
                                 &WdfMemory,
                                 &SffdiskCmdData);

        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }

        SffdiskCmdData->HeaderSize = sizeof(SFFDISK_DEVICE_COMMAND_DATA);
        SffdiskCmdData->Command = SFFDISK_DC_DEVICE_COMMAND;
        SffdiskCmdData->ProtocolArgumentSize = sizeof(SDCMD_DESCRIPTOR);
        SffdiskCmdData->DeviceDataBufferSize = RPMB_EMMC_CID_SIZE;
        SffdiskCmdData->Information = 0;

        SdCmdDescriptor = (PSDCMD_DESCRIPTOR)&SffdiskCmdData->Data;
        SdCmdDescriptor->Cmd = 10;
        SdCmdDescriptor->CmdClass = SDCC_STANDARD;
        SdCmdDescriptor->TransferDirection = SDTD_READ;
        SdCmdDescriptor->TransferType = SDTT_CMD_ONLY;
        SdCmdDescriptor->ResponseType = SDRT_2;

        Status = WdfIoTargetFormatRequestForIoctl(WdfIoTarget,
                                                  WdfRequest,
                                                  IOCTL_SFFDISK_DEVICE_COMMAND,
                                                  WdfMemory,
                                                  NULL,
                                                  WdfMemory,
                                                  NULL);

        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }

        CompletionContext.RequiredBufferSize = BufferSize;
        KeInitializeEvent(&CompletionContext.CompletionEvent, 
                          SynchronizationEvent, 
                          FALSE);

        WdfRequestSetCompletionRoutine(WdfRequest,
                                       OpteeRpcGetCidCompletionRoutine,
                                       (WDFCONTEXT)&CompletionContext);

        if (WdfRequestSend(WdfRequest,
                           WdfIoTarget,
                           WDF_NO_SEND_OPTIONS) == FALSE) {

            Status = WdfRequestGetStatus(WdfRequest);
            goto Exit;
        }

        Status = KeWaitForSingleObject(&CompletionContext.CompletionEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        NT_ASSERT(Status == STATUS_SUCCESS);

        Status = STATUS_SUCCESS;
    }

Exit:

    if (WdfIoTarget != NULL) {
        WdfObjectDelete(WdfIoTarget);
    }

    if (!NT_SUCCESS(Status)) {
        KeReleaseSemaphore(&OpteeSMCLock,
                           IO_NO_INCREMENT,
                           1,
                           FALSE);
    }

    return Status;
}
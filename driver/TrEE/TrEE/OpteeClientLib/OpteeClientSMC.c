/** @file
  The OP-TEE SMC Interface that packs the SMC parameters and sends the SMC to
  secure world for handling.
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
#include "OpteeClientMemory.h"
#include "OpteeClientSMC.h"
#include "OpteeClientRPC.h"
#include "trace.h"


// Taken from the optee_os implementation.
//
#include "teesmc.h"

#define TEEC_SMC_SERIALIZE_CALLS 0
#define TEEC_SMC_DEFAULT_CACHE_ATTRIBUTES (TEESMC_ATTR_CACHE_DEFAULT << TEESMC_ATTR_CACHE_SHIFT);

volatile float FloatingPointInit;

static void SetTeeSmc32Params(TEEC_Operation *operation, t_teesmc_param *TeeSmcParam);
static void GetTeeSmc32Params(t_teesmc_param *TeeSmcParam, TEEC_Operation *operation);

static TEEC_Result OpteeSmcCallInternal(PHYSICAL_ADDRESS *SmcAddrPA);
static TEEC_Result OpteeSmcCall(PHYSICAL_ADDRESS *SmcAddrPA);


/*
 * This function opens a new Session between the Client application and the
 * specified TEE application.
 *
 * Only connection_method == TEEC_LOGIN_PUBLIC is supported connection_data and
 * operation shall be set to NULL.
 */
TEEC_Result TEEC_SMC_OpenSession(TEEC_Context *context,
                                 TEEC_Session *session,
                                 const TEEC_UUID *destination,
                                 TEEC_Operation  *operation,
                                 uint32_t *error_origin)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    NTSTATUS Status;
    t_teesmc_arg *TeeSmcArg = NULL;
    t_teesmc_param *TeeSmcParam = NULL;
    static const UINT32 MetaParamNum = 2;
    PHYSICAL_ADDRESS TeeSmc32ArgPA;

    UNREFERENCED_PARAMETER(context);

    *error_origin = TEEC_ORIGIN_API;

    // Allocate the primary data packet from the OP-TEE OS shared pool,
    // with enough space to hold the meta parameters.
    //
    {
        UINT32 TeeSmc32ArgLength = TEESMC_GET_ARG_SIZE(TEEC_CONFIG_PAYLOAD_REF_COUNT + MetaParamNum);

        Status = OpteeClientMemAlloc(TeeSmc32ArgLength, &TeeSmcArg, &TeeSmc32ArgPA);
        if (TeeSmcArg == NULL || !NT_SUCCESS(Status)) {
            TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
            goto Exit;
        }
        RtlZeroMemory(TeeSmcArg, TeeSmc32ArgLength);
    }

    TeeSmcArg->cmd        = TEESMC_CMD_OPEN_SESSION;
    TeeSmcArg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT + MetaParamNum;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    // Set the first meta parameter: service UUID
    //
    TeeSmcParam[0].attr = TEESMC_ATTR_TYPE_VALUE_INPUT | TEESMC_ATTR_META;
    RtlCopyMemory(&TeeSmcParam[0].u.value, destination, sizeof(TEEC_UUID));

    // Set the second meta parameter: login
    //
    TeeSmcParam[1].attr = TEESMC_ATTR_TYPE_VALUE_INPUT | TEESMC_ATTR_META;
    TeeSmcParam[1].u.value.c = TEEC_LOGIN_PUBLIC;

    // Fill in the caller supplied operation parameters.
    //
    SetTeeSmc32Params(operation, TeeSmcParam + MetaParamNum);

    *error_origin = TEEC_ORIGIN_COMMS;

    TeecResult = OpteeSmcCall(&TeeSmc32ArgPA);
    if (TeecResult != TEEC_SUCCESS) {
        goto Exit;
    }

    session->session_id = TeeSmcArg->session;
    TeecResult    = TeeSmcArg->ret;
    *error_origin = TeeSmcArg->ret_origin;

    // Update the caller supplied operation parameters with those returned from the call.
    //
    GetTeeSmc32Params(TeeSmcParam, operation);

Exit:
    if (TeeSmcArg != NULL) {
        OpteeClientMemFree(TeeSmcArg);
    }

    return TeecResult;
}


/*
 * This function closes a session which has been opened with a TEE
 * application.
 *
 * Note that the GP specification does not allow for this API to fail and return
 * a failure code however we'll support this at the SMC level so we can get
 * see debug information about such failures.
 */
TEEC_Result TEEC_SMC_CloseSession(TEEC_Session *session,
                                  uint32_t *error_origin)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    NTSTATUS Status;
    t_teesmc_arg *TeeSmcArg = NULL;
    PHYSICAL_ADDRESS TeeSmc32ArgPA;

    *error_origin = TEEC_ORIGIN_API;

    // Allocate the primary data packet from the OP-TEE OS shared pool.
    //
    {
        UINT32 TeeSmc32ArgLength = TEESMC_GET_ARG_SIZE(0);

        Status = OpteeClientMemAlloc(TeeSmc32ArgLength, &TeeSmcArg , &TeeSmc32ArgPA);
        if (TeeSmcArg == NULL || !NT_SUCCESS(Status)) {
            TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
            goto Exit;
        }
        RtlZeroMemory(TeeSmcArg, TeeSmc32ArgLength);
    }

    TeeSmcArg->cmd = TEESMC_CMD_CLOSE_SESSION;
    TeeSmcArg->session = session->session_id;

    *error_origin = TEEC_ORIGIN_COMMS;
    
    TeecResult = OpteeSmcCall(&TeeSmc32ArgPA);
    if (TeecResult != TEEC_SUCCESS) {
        goto Exit;
    }

    TeecResult    = TeeSmcArg->ret;
    *error_origin = TeeSmcArg->ret_origin;

Exit:
    if (TeeSmcArg != NULL) {
        OpteeClientMemFree(TeeSmcArg);
    }

    return TeecResult;
}


/*
 * Invokes a TEE command (secure service, sub-PA or whatever).
 */
TEEC_Result TEEC_SMC_InvokeCommand(TEEC_Session *session,
                                   uint32_t cmd_id,
                                   TEEC_Operation *operation,
                                   uint32_t *error_origin)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    NTSTATUS Status;
    t_teesmc_arg *TeeSmcArg = NULL;
    t_teesmc_param *TeeSmcParam = NULL;
    PHYSICAL_ADDRESS TeeSmc32ArgPA;

    *error_origin = TEEC_ORIGIN_API;

    // Allocate the primary data packet from the OP-TEE OS shared pool.
    //
    {
        UINT32 TeeSmc32ArgLength = TEESMC_GET_ARG_SIZE(TEEC_CONFIG_PAYLOAD_REF_COUNT);

        Status = OpteeClientMemAlloc(TeeSmc32ArgLength, &TeeSmcArg, &TeeSmc32ArgPA);
        if (TeeSmcArg == NULL || !NT_SUCCESS(Status)) {
            TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
            goto Exit;
        }
        RtlZeroMemory(TeeSmcArg, TeeSmc32ArgLength);
    }

    TeeSmcArg->cmd        = TEESMC_CMD_INVOKE_COMMAND;
    TeeSmcArg->func       = cmd_id;
    TeeSmcArg->session    = session->session_id;
    TeeSmcArg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;

    TeeSmcParam = TEESMC_GET_PARAMS(TeeSmcArg);

    // Fill in the caller supplied operation parameters.
    //
    SetTeeSmc32Params(operation, TeeSmcParam);

    *error_origin = TEEC_ORIGIN_COMMS;

    TeecResult = OpteeSmcCall(&TeeSmc32ArgPA);
    if (TeecResult != TEEC_SUCCESS) {
        goto Exit;
    }

    TeecResult    = TeeSmcArg->ret;
    *error_origin = TeeSmcArg->ret_origin;

    // Update the caller supplied operation parameters with those returned from the call.
    //
    GetTeeSmc32Params(TeeSmcParam, operation);

Exit:
    if (TeeSmcArg != NULL) {
        OpteeClientMemFree(TeeSmcArg);
    }

    return TeecResult;
}


/*
 * Request a cancellation of a in-progress operation (best effort)
 *
 * Note that the GP specification does not allow for this API to fail and return
 * a failure code however we'll support this at the SMC level so we can get
 * see debug information about such failures.
 */
TEEC_Result TEEC_SMC_RequestCancellation(TEEC_Operation *operation,
                                         uint32_t *error_origin)
{
    UNREFERENCED_PARAMETER(operation);
    UNREFERENCED_PARAMETER(error_origin);
    return TEEC_ERROR_NOT_IMPLEMENTED;
}


/*
 * Set the call parameter blocks in the SMC call based on the TEEC parameter supplied.
 * This only handles the parameters supplied in the originating call and not those
 * considered internal meta parameters and is thus constrained by the build
 * constants exposed to callers.
 */
void SetTeeSmc32Params(TEEC_Operation *operation, t_teesmc_param *TeeSmcParam)
{
    UINT32 ParamCount;

    for (ParamCount = 0 ; ParamCount < TEEC_CONFIG_PAYLOAD_REF_COUNT ; ParamCount++) {

        uint32_t attr = TEEC_PARAM_TYPE_GET(operation->paramTypes, ParamCount);

        switch (attr)
        {
        case TEEC_NONE:
            attr = TEESMC_ATTR_TYPE_NONE;
            TeeSmcParam[ParamCount].attr = attr;
            break;

        case TEEC_VALUE_INPUT:
        case TEEC_VALUE_OUTPUT:
        case TEEC_VALUE_INOUT:
            attr = TEESMC_ATTR_TYPE_VALUE_INPUT + attr - TEEC_VALUE_INPUT;
            TeeSmcParam[ParamCount].attr = attr;
            TeeSmcParam[ParamCount].u.value.a = operation->params[ParamCount].value.a;
            TeeSmcParam[ParamCount].u.value.b = operation->params[ParamCount].value.b;
            break;

        case TEEC_MEMREF_TEMP_INPUT:
        case TEEC_MEMREF_TEMP_OUTPUT:
        case TEEC_MEMREF_TEMP_INOUT:
            attr = TEESMC_ATTR_TYPE_TMEM_INPUT + attr - TEEC_MEMREF_TEMP_INPUT;
            TeeSmcParam[ParamCount].attr = attr;
            TeeSmcParam[ParamCount].u.tmem.buf_ptr = (uintptr_t)operation->params[ParamCount].tmpref.buffer;
            TeeSmcParam[ParamCount].u.tmem.size = operation->params[ParamCount].tmpref.size;
            break;

        default:
            ASSERT(!"Unsupported TEEC value type");
            break;
        }
    }
}


/*
 * Get the return parameter blocks from the SMC call into the TEEC parameter supplied.
 * This only handles the parameters supplied in the originating call and not those
 * considered internal meta parameters and is thus constrained by the build
 * constants exposed to callers.
 */
void GetTeeSmc32Params (t_teesmc_param *TeeSmcParam, TEEC_Operation *operation)
{
    UINT32 ParamCount;

    for (ParamCount = 0; ParamCount < TEEC_CONFIG_PAYLOAD_REF_COUNT; ParamCount++) {
        // this assumes that operation was setup previously with SetTeeSmc32Params
        uint32_t attr = TEEC_PARAM_TYPE_GET(operation->paramTypes, ParamCount);

        switch (attr)
        {
        case TEEC_NONE:
            break;

        case TEEC_VALUE_INPUT:
        case TEEC_VALUE_OUTPUT:
        case TEEC_VALUE_INOUT:
            operation->params[ParamCount].value.a = (uint32_t)TeeSmcParam[ParamCount].u.value.a;
            operation->params[ParamCount].value.b = (uint32_t)TeeSmcParam[ParamCount].u.value.b;
            break;

        case TEEC_MEMREF_TEMP_INPUT:
        case TEEC_MEMREF_TEMP_OUTPUT:
        case TEEC_MEMREF_TEMP_INOUT:
            operation->params[ParamCount].tmpref.buffer = (void *)(uintptr_t)TeeSmcParam[ParamCount].u.tmem.buf_ptr;
            operation->params[ParamCount].tmpref.size = (UINTN)TeeSmcParam[ParamCount].u.tmem.size;
            break;

        default:
            ASSERT(!"Unsupported TEEC value type");
            break;
        }
    }
}


/*
 * Call into secure world...
 */
TEEC_Result OpteeSmcCall(PHYSICAL_ADDRESS *SmcAddrPA)
{
    TEEC_Result Result;
    XSTATE_SAVE XStateSave;

#if TEEC_SMC_SERIALIZE_CALLS
    NTSTATUS Status;

    Status = KeWaitForSingleObject(&OpteeSMCLock,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ASSERT(Status == STATUS_SUCCESS);
#endif // TEEC_SMC_SERIALIZE_CALLS

    // Force VFP initialization, if OPTEE is built with TA VFP support
    // it assumes VFP is enabled.
    //

    KeSaveExtendedProcessorState(XSTATE_MASK_LEGACY_SSE, &XStateSave);

    // Dummy, make a floating point opp to force thread VFP initialization
    //
    FloatingPointInit = 1.1f;

    Result = OpteeSmcCallInternal(SmcAddrPA);

    KeRestoreExtendedProcessorState(&XStateSave);

#if TEEC_SMC_SERIALIZE_CALLS
    KeReleaseSemaphore(&OpteeSMCLock,
                       LOW_PRIORITY,
                       1,
                       FALSE);
#endif // TEEC_SMC_SERIALIZE_CALLS
    return Result;
}


/*
 * Populate the SMC registers and make the call with OP-TEE specific
 * handling.
 */
TEEC_Result OpteeSmcCallInternal(PHYSICAL_ADDRESS *TeeSmc32ArgPA)
{
    TEEC_Result TeecResult = TEEC_SUCCESS;
    ARM_SMC_ARGS ArmSmcArgs = {0};
    
    // For now just use the normal call style.
    //
    ArmSmcArgs.Arg0 = TEESMC32_CALL_WITH_ARG;
    ArmSmcArgs.Arg1 = TeeSmc32ArgPA->u.HighPart;
    ArmSmcArgs.Arg2 = TeeSmc32ArgPA->u.LowPart;

    // This is a loop because the call may result in RPC's that will need
    // to be processed and may result in further calls until the originating
    // call is completed.
    //

    for (;;) {

        ArmCallSmc(&ArmSmcArgs);

        if (TEESMC_RETURN_IS_RPC(ArmSmcArgs.Arg0)) {

            // We must service the RPC even if it's processing failed
            // and let the OP-TEE OS unwind and return back to us with
            // it's error information.
            //
            (void) OpteeRpcCallback(&ArmSmcArgs);
        }
        else if (ArmSmcArgs.Arg0 == TEESMC_RETURN_UNKNOWN_FUNCTION) {
            TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
            break;
        }
        else if (ArmSmcArgs.Arg0 != TEESMC_RETURN_OK) {
            TeecResult = TEEC_ERROR_COMMUNICATION;
            break;
        }
        else {
            TeecResult = TEEC_SUCCESS;
            break;
        }
    }

    return TeecResult;
}

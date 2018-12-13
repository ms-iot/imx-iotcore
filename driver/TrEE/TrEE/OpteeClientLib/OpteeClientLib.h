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
#pragma once

//
// Include the OP-TEE OS Client API definitions
//
#include <wdf.h>
#include "tee_client_api.h"

#define PAGE_SIZE 0x1000

extern WDFDEVICE LibServiceDevice;
extern KSEMAPHORE OpteeSMCLock;
extern KSEMAPHORE OpteeMemLock;

NTSTATUS
OpteeClientApiLibInitialize(
    _In_opt_ HANDLE ImageHandle,
    _In_ WDFDEVICE ServiceDevice,
    _In_ PHYSICAL_ADDRESS SharedMemoryBase,
    _In_ UINT32 SharedMemoryLength
    );

VOID
OpteeClientApiLibDeinitialize(
    _In_opt_ HANDLE ImageHandle,
    _In_ WDFDEVICE ServiceDevice
    );

static inline
NTSTATUS NtstatusFromTeecResult(TEEC_Result TeecResult)
{
    //
    // Using the TEE Client API Specification V1.0c section 4.4.2 as guidline
    // for error mapping.
    //

    switch (TeecResult) {
    case TEEC_SUCCESS:
        return STATUS_SUCCESS;
    case TEEC_ERROR_GENERIC:
        return STATUS_UNSUCCESSFUL;
    case TEEC_ERROR_ACCESS_DENIED:
        return STATUS_ACCESS_DENIED;
    case TEEC_ERROR_CANCEL:
        return STATUS_CANCELLED;
    case TEEC_ERROR_ACCESS_CONFLICT:
        return STATUS_INVALID_PARAMETER;
    case TEEC_ERROR_EXCESS_DATA:
        return STATUS_PARAMETER_QUOTA_EXCEEDED;
    case TEEC_ERROR_BAD_FORMAT:
        return STATUS_BAD_DATA;
    case TEEC_ERROR_BAD_PARAMETERS:
        return STATUS_INVALID_PARAMETER;
    case TEEC_ERROR_BAD_STATE:
        return STATUS_UNEXPECTED_IO_ERROR;
    case TEEC_ERROR_ITEM_NOT_FOUND:
        return STATUS_NOT_FOUND;
    case TEEC_ERROR_NOT_IMPLEMENTED:
        return STATUS_NOT_IMPLEMENTED;
    case TEEC_ERROR_NOT_SUPPORTED:
        return STATUS_NOT_SUPPORTED;
    case TEEC_ERROR_NO_DATA:
        return STATUS_NO_DATA_DETECTED;
    case TEEC_ERROR_OUT_OF_MEMORY:
        return STATUS_INSUFFICIENT_RESOURCES;
    case TEEC_ERROR_BUSY:
        return STATUS_DEVICE_BUSY;
    case TEEC_ERROR_COMMUNICATION:
        return STATUS_PROTOCOL_UNREACHABLE;
    case TEEC_ERROR_SECURITY:
        return STATUS_FAIL_CHECK;
    case TEEC_ERROR_SHORT_BUFFER:
        return STATUS_BUFFER_TOO_SMALL;
    case TEEC_ERROR_EXTERNAL_CANCEL:
        return STATUS_CANCELLED;
    case TEEC_ERROR_TARGET_DEAD:
        return STATUS_DEVICE_UNREACHABLE;
    default:
        return STATUS_UNEXPECTED_IO_ERROR;
    }
}

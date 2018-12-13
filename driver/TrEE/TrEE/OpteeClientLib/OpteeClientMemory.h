/** @file
The OP-TEE Share Memory component is used to manage memory allocations
from the memory block used for parameters & buffers shared between the
client in normal world and the OP-TEE OS in secure world.
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

#pragma once

NTSTATUS 
OpteeClientMemInit(
    _In_ HANDLE ImageHandle,
    _In_ PHYSICAL_ADDRESS SharedMemoryBase,
    _In_ UINT32 SharedMemoryLength
    );

VOID
OpteeClientMemDeinit();

NTSTATUS 
OpteeClientMemAlloc(
    _In_ UINT32 Length,
    _Out_ PVOID *AllocatedMemory,
    _Out_opt_ PHYSICAL_ADDRESS *PhysicalMemory
    );

VOID 
OpteeClientMemFree(
    _In_ PVOID Mem
    );

ULONGLONG
OpteeClientVirtualToPhysical(
    _In_ PVOID Va
    );

PVOID
OpteeClientPhysicalToVirtual(
    _In_ ULONGLONG Pa
    );

#define SWAP_B16(u16)				 \
			((((UINT16)(u16)<<8) & 0xFF00)   | \
			 (((UINT16)(u16)>>8) & 0x00FF))

#define SWAP_B32(u32)					 \
			((((UINT32)(u32)<<24) & 0xFF000000)  | \
			 (((UINT32)(u32)<< 8) & 0x00FF0000)  | \
			 (((UINT32)(u32)>> 8) & 0x0000FF00)  | \
			 (((UINT32)(u32)>>24) & 0x000000FF))

#define SWAP_D64(u64) \
            (((UINT64)(u64) >> 32) | \
            (((UINT64)(u64) & 0xFFFFFFFF) << 32))


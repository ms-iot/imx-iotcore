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

#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <TrustedRuntimeClx.h>
#include "OpteeClientMemory.h"
#include "OpteeClientLib.h"
#include "OpteeTrEE.h"
#include "OpteeClientMM.h"
#include "types.h"


//
// Attributes for reserved memory.
// These seem to be missing from the general includes
//
#define MEMORY_PRESENT      0x0100000000000000ULL
#define MEMORY_INITIALIZED  0x0200000000000000ULL
#define MEMORY_TESTED       0x0400000000000000ULL


//
// OP-TEE reserved size on shared memory (Used for PL310 mutex)
//
#define OPTEE_SHM_RESERVED_SIZE 0x1000
#define MEMORY_GANULARITY PAGE_SIZE

/*
* Driver image handle to use for memory allocation.
*/
static HANDLE mDriverImageHandle;

/*
* The header contains the current memory mapping information.
*/
OPTEE_MM_HEADER g_OpteeMemoryHeader;

/*
* The UEFI allocation API's require the amount of memory to be freed in addition
* to the pointer to the memory to free. This header on the block contains the length
* allocated for that free.
*/
typedef struct _OPTEE_CLIENT_MEM_HEADER {

    UINT32 BitMapIndex;
    UINT32 Length;
    UINT64 Reserved64;
} OPTEE_CLIENT_MEM_HEADER;


NTSTATUS OpteeClientMemInit(
    _In_ HANDLE ImageHandle,
    _In_ PHYSICAL_ADDRESS SharedMemoryBase,
    _In_ UINT32 SharedMemoryLength
    )

/*
 * Initialize the memory component, for example providing the
 * containing drivers handle.
 */

{
    ULONG BitMapSizeBits;
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ImageHandle);

    g_OpteeMemoryHeader.BasePA.QuadPart =
        SharedMemoryBase.QuadPart + OPTEE_SHM_RESERVED_SIZE;

    g_OpteeMemoryHeader.Length = SharedMemoryLength - OPTEE_SHM_RESERVED_SIZE;


    //
    // Will use the unsafe function for the time being.
    //
    g_OpteeMemoryHeader.BaseVA = MmMapIoSpace(g_OpteeMemoryHeader.BasePA,
                                              g_OpteeMemoryHeader.Length,
                                              MmCached);

    if (g_OpteeMemoryHeader.BaseVA == NULL) {
        //
        // Report Error.
        //
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    BitMapSizeBits = g_OpteeMemoryHeader.Length / MEMORY_GANULARITY;

    g_OpteeMemoryHeader.BitMapAddress = ExAllocatePoolWithTag(PagedPool,
        (BitMapSizeBits + 7) / 8,
        OPTEE_TREE_POOL_TAG);

    if (g_OpteeMemoryHeader.BitMapAddress == NULL) {
        //
        // Report Error.
        //
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    RtlInitializeBitMap(&g_OpteeMemoryHeader.BitMapHeader,
        g_OpteeMemoryHeader.BitMapAddress,
        BitMapSizeBits);

    //
    // Clearing all bits to indicate that the memory is now unallocated.
    //
    RtlClearAllBits(&g_OpteeMemoryHeader.BitMapHeader);

Exit:
    return Status;
}


VOID
OpteeClientMemDeinit()

/*
 * Release any resources allocated by the memory component.
 */

{
    if (g_OpteeMemoryHeader.BaseVA != NULL) {
        MmUnmapIoSpace(g_OpteeMemoryHeader.BaseVA,
                       g_OpteeMemoryHeader.Length);
    }

    g_OpteeMemoryHeader.BasePA.QuadPart = 0;
    g_OpteeMemoryHeader.BaseVA = NULL;
    g_OpteeMemoryHeader.Length = 0;

    if (g_OpteeMemoryHeader.BitMapAddress != NULL) {
        ExFreePoolWithTag(g_OpteeMemoryHeader.BitMapAddress,
                          OPTEE_TREE_POOL_TAG);
    }

    g_OpteeMemoryHeader.BitMapAddress = NULL;
}


NTSTATUS
OpteeClientMemAlloc(
    _In_ UINT32 Length,
    _Out_ PVOID *AllocatedMemory,
    _Out_opt_ PHYSICAL_ADDRESS *PhysicalMemory
    )

/*
 * Allocate a page aligned block of memory from the TrustZone
 * shared memory block.
 */

{
    NTSTATUS Status;
    UINT32 ActualLength;
    ULONG NumPages;
    ULONG ClearIndex;

    *AllocatedMemory = NULL;
    if (PhysicalMemory != NULL) {
        PhysicalMemory->QuadPart = 0;
    }

    if (g_OpteeMemoryHeader.BaseVA == NULL) {
        //
        // Memory was not mapped during initialization
        //
        return STATUS_MEMORY_NOT_ALLOCATED;
    }

    Status = KeWaitForSingleObject(&OpteeMemLock,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ASSERT(Status == STATUS_SUCCESS);

    ActualLength = Length + sizeof(OPTEE_CLIENT_MEM_HEADER);
    NumPages = (ActualLength / PAGE_SIZE) + 1;

    if ((ActualLength % PAGE_SIZE) != 0) {
        ActualLength = NumPages * PAGE_SIZE;
    }

    //
    // Find the run that contain the set of clear bits and set the bit(s).
    //
    ClearIndex = RtlFindClearBitsAndSet(&g_OpteeMemoryHeader.BitMapHeader,
                                        NumPages,
                                        0);

    if (ClearIndex == 0xFFFFFFFF) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    *AllocatedMemory = (PVOID)((UINTN)g_OpteeMemoryHeader.BaseVA + ClearIndex * MEMORY_GANULARITY);

    //
    // Fill in the length into the header and account for the header at the start
    // of the memory block.
    //
    OPTEE_CLIENT_MEM_HEADER* Header = (OPTEE_CLIENT_MEM_HEADER*)(UINTN)(*AllocatedMemory);

    Header->BitMapIndex = ClearIndex;
    Header->Length = ActualLength;
    Header->Reserved64 = 0;

    *AllocatedMemory = (PVOID)((UINTN)(*AllocatedMemory) + sizeof(OPTEE_CLIENT_MEM_HEADER));

    if (PhysicalMemory != NULL) {
        PhysicalMemory->QuadPart = OpteeClientVirtualToPhysical(*AllocatedMemory);
    }

    Status = STATUS_SUCCESS;

Exit:

    KeReleaseSemaphore(&OpteeMemLock,
                       LOW_PRIORITY,
                       1,
                       FALSE);

    return Status;
}


VOID
OpteeClientMemFree(
    _In_ PVOID Mem
    )

/*
 * Free a block of memory previously allocated using the
 * OpteeClientMemAlloc function.
 */

{
    OPTEE_CLIENT_MEM_HEADER* Header;
    ULONG NumPages;

    Header = (OPTEE_CLIENT_MEM_HEADER*)(
        ((UINTN)(Mem)) - sizeof(OPTEE_CLIENT_MEM_HEADER));

    ASSERT(((UINTN)Header) % PAGE_SIZE == 0);

    NumPages = Header->Length  / PAGE_SIZE;

    ASSERT(RtlAreBitsSet(&g_OpteeMemoryHeader.BitMapHeader, Header->BitMapIndex, NumPages) == TRUE);

    RtlClearBits(&g_OpteeMemoryHeader.BitMapHeader, Header->BitMapIndex, NumPages);

    return;
}


ULONGLONG
OpteeClientVirtualToPhysical(
    _In_ PVOID Va
    )

/*
 * Converts the a Virtual address to Physical address in the
 * Optee Shared memory region.
 */

{
    ULONGLONG Pa;

    ASSERT(Va != NULL);
    Pa = g_OpteeMemoryHeader.BasePA.QuadPart +
        ((ULONGLONG)Va - (ULONGLONG)(g_OpteeMemoryHeader.BaseVA));

    return Pa;
}

PVOID
OpteeClientPhysicalToVirtual(
    _In_ ULONGLONG Pa
    )

/*
 * Converts the a physical address in the shared
 * Optee memory region to VA.
 */

{
    ULONGLONG Va;
    ASSERT(Pa >= (ULONGLONG)g_OpteeMemoryHeader.BasePA.QuadPart);

    Va = (ULONGLONG)g_OpteeMemoryHeader.BaseVA +
        (Pa - g_OpteeMemoryHeader.BasePA.QuadPart);

    return (PVOID)((UINTN)Va);

}
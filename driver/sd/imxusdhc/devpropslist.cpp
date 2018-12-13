// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   devpropslist.cpp
//
// Abstract:
//
//  This module contains the implementation of the Device Properties list 
//  abstraction
//
// Environment:
//
//  Kernel mode only
//

#include "precomp.hpp"
#pragma hdrstop

#include "devpropslist.hpp"

//
// Internal Helpers
//

VOID
DevicePropertiesListUnsafeRemoveEntry(
    _In_ USDHC_DEVICE_PROPERTIES* RecordPtr);

USDHC_DEVICE_PROPERTIES*
DevicePropertiesListUnsafeFindByPdo(
    _In_ const DEVICE_OBJECT* PdoPtr);

USDHC_DEVICE_PROPERTIES*
DevicePropertiesListUnsafeFindByKey(
    _In_ UINT32 Key);


VOID
DevicePropertiesListLock(
    _In_ KIRQL* oldIrqlPtr);

VOID
DevicePropertiesListUnlock(
    _In_ KIRQL oldIrql);

NONPAGED_SEGMENT_BEGIN; //=====================================================

USDHC_DEVICE_PROPERTIES SdhcDevicePropertiesList[USDHC_DEVICE_PROPERTIES_MAX];
KSPIN_LOCK SdhcDevicePropertiesLock;

_Use_decl_annotations_
VOID
DevicePropertiesListLock(
    KIRQL* OldIrqlPtr
    )
{
    NT_ASSERT(ARGUMENT_PRESENT(OldIrqlPtr));

    //
    // Crashdump environment will be running at IRQL > DISPATCH_LEVEL
    // Since we are not servicing interrupts during crashdump, it is safe to
    // assume critical section is implicit
    //
    if (KeGetCurrentIrql() <= DISPATCH_LEVEL) {
        KeAcquireSpinLock(&SdhcDevicePropertiesLock, OldIrqlPtr);
    }
}

_Use_decl_annotations_
VOID
DevicePropertiesListUnlock(
    KIRQL OldIrql
    ) 
{
    if (KeGetCurrentIrql() <= DISPATCH_LEVEL) {
        KeReleaseSpinLock(&SdhcDevicePropertiesLock, OldIrql);
    }
}

VOID
SdhcDevicePropertiesListInit()
{
    RtlZeroMemory(SdhcDevicePropertiesList, sizeof(SdhcDevicePropertiesList));
    KeInitializeSpinLock(&SdhcDevicePropertiesLock);
}

_Use_decl_annotations_
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListSafeAddEntry()
{
    KIRQL oldIrql;
    DevicePropertiesListLock(&oldIrql);

    USDHC_DEVICE_PROPERTIES* currentRecordPtr = SdhcDevicePropertiesList;
    const USDHC_DEVICE_PROPERTIES* recordEndPtr = currentRecordPtr + ARRAYSIZE(SdhcDevicePropertiesList);

    while (currentRecordPtr < recordEndPtr) {
        if (currentRecordPtr->IsAllocated == FALSE) {
            NT_ASSERT(currentRecordPtr->Key == 0);
            NT_ASSERT(currentRecordPtr->PdoPtr == 0);
            RtlZeroMemory(currentRecordPtr, sizeof(*currentRecordPtr));
            currentRecordPtr->IsAllocated = TRUE;
            break;
        }
        ++currentRecordPtr;
    }

    DevicePropertiesListUnlock(oldIrql);
    return currentRecordPtr;
}

_Use_decl_annotations_
VOID
DevicePropertiesListSafeRemoveEntry(
    USDHC_DEVICE_PROPERTIES* RecordPtr
    )
{
    if (RecordPtr == nullptr) {
        NT_ASSERT(FALSE);
        return;
    }

    KIRQL oldIrql;
    DevicePropertiesListLock(&oldIrql);

    NT_ASSERT(RecordPtr->IsAllocated);
    DevicePropertiesListUnsafeRemoveEntry(RecordPtr);
    NT_ASSERT(!RecordPtr->IsAllocated);

    DevicePropertiesListUnlock(oldIrql);
}

_Use_decl_annotations_
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListSafeFindByKey(
    UINT32 Key
    )
{
    if (Key == USDHC_DEVICE_PROPERTIES_NULL_KEY) {
        NT_ASSERT(FALSE);
        return nullptr;
    }

    KIRQL oldIrql;
    USDHC_DEVICE_PROPERTIES* recordPtr;
    DevicePropertiesListLock(&oldIrql);

    recordPtr = DevicePropertiesListUnsafeFindByKey(Key);

    DevicePropertiesListUnlock(oldIrql);
    return recordPtr;
}

_Use_decl_annotations_
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListSafeFindByPdo(
    const DEVICE_OBJECT* PdoPtr
    )
{
    if (PdoPtr == nullptr) {
        NT_ASSERT(FALSE);
        return nullptr;
    }

    KIRQL oldIrql;
    USDHC_DEVICE_PROPERTIES* recordPtr;

    DevicePropertiesListLock(&oldIrql);

    recordPtr = DevicePropertiesListUnsafeFindByPdo(PdoPtr);

    DevicePropertiesListUnlock(oldIrql);
    return recordPtr;
}

_Use_decl_annotations_
NTSTATUS
DevicePropertiesListSafeRemoveByKey(
    UINT32 Key
    )
{
    if (Key == USDHC_DEVICE_PROPERTIES_NULL_KEY) {
        NT_ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL oldIrql;
    NTSTATUS status;
    DevicePropertiesListLock(&oldIrql);

    USDHC_DEVICE_PROPERTIES* recordPtr = 
        DevicePropertiesListUnsafeFindByKey(Key);
    if (recordPtr == nullptr) {
        NT_ASSERTMSG("Can't remove from an empty list", FALSE);
        status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    NT_ASSERT(recordPtr->IsAllocated);
    DevicePropertiesListUnsafeRemoveEntry(recordPtr);
    NT_ASSERT(!recordPtr->IsAllocated);

    status = STATUS_SUCCESS;

Cleanup:

    DevicePropertiesListUnlock(oldIrql);
    return status;
}

//
// Non-protected/synchronized list manipulation internal helpers, should be
// called only inside a critical section
//

_Use_decl_annotations_
VOID
DevicePropertiesListUnsafeRemoveEntry(
    _In_ USDHC_DEVICE_PROPERTIES* RecordPtr
    )
{
    NT_ASSERT(RecordPtr->IsAllocated);
    RtlZeroMemory(RecordPtr, sizeof(*RecordPtr));
}

_Use_decl_annotations_
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListUnsafeFindByPdo(
    const DEVICE_OBJECT* PdoPtr
    )
{
    USDHC_DEVICE_PROPERTIES* currentRecordPtr = SdhcDevicePropertiesList;
    const USDHC_DEVICE_PROPERTIES* recordEndPtr = currentRecordPtr + ARRAYSIZE(SdhcDevicePropertiesList);

    while (currentRecordPtr < recordEndPtr) {
        if ((currentRecordPtr->IsAllocated) && (currentRecordPtr->PdoPtr == PdoPtr)) {
            return currentRecordPtr;
        }
        ++currentRecordPtr;
    }

    return nullptr;
}

_Use_decl_annotations_
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListUnsafeFindByKey(
    UINT32 Key
    )
{
    USDHC_DEVICE_PROPERTIES* currentRecordPtr = SdhcDevicePropertiesList;
    const USDHC_DEVICE_PROPERTIES* recordEndPtr = currentRecordPtr + ARRAYSIZE(SdhcDevicePropertiesList);

    while (currentRecordPtr < recordEndPtr) {
        if ((currentRecordPtr->IsAllocated) && (currentRecordPtr->Key == Key)) {
            return currentRecordPtr;
        }
        ++currentRecordPtr;
    }

    return nullptr;
}

NONPAGED_SEGMENT_END; //=======================================================
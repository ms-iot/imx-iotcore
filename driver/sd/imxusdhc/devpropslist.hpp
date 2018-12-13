// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   devpropslist.hpp
//
// Abstract:
//
//  This module contains the declaration of the Device Properties list abstraction
//  as well as the Device Properties record definition
//
//  The current implementation uses a fixed size array to represent the list in a
//  non-generic interface. Fixed size array was chosen over the kernel doubly
//  linked list (LIST_ENTRY) to simplify the implementation and reduce the bug
//  surface
//  A better reusable design would a generic class to abstract fixed-size array 
//  using templates
//
// Environment:
//
//  Kernel mode only
//

#ifndef __DEVPROPSLIST_H__
#define __DEVPROPSLIST_H__

//
// This is equal to the max number of uSDHC device instances that exist at any
// point of time. e.g iMX6Q platform has 4 uSDHC and thus the max number of
// uSDHC device instances will be 4
// We will use double that number to accommodate for possible new designs
// despite this code has been verified on iMX6 based platforms only
//
#define USDHC_DEVICE_PROPERTIES_MAX             8

//
// A special key to mark free entries in the Device Properties list
//
#define USDHC_DEVICE_PROPERTIES_NULL_KEY        0

//
// Used to store platform specific information for each SDHC known as
// Device Properties and populated from ACPI using _DSD method
//
typedef struct {
    //
    // Device Properties Key (Unique across the uSDHCs)
    // We will use the register base physical address as a
    // unique key for each uSDHCs
    //
    UINT32 Key;

    //
    // Device Properties Values
    //
    UINT32 BaseClockFrequencyHz;
    UINT32 Regulator1V8Exist;
    UINT32 SlotCount;

    //
    // Can power on/off SD/MMC slot supply voltage via
    // firmware
    //
    BOOLEAN SlotPowerControlSupported;

    //
    // Bookkeeping
    //
    // Associated PDO
    //
    DEVICE_OBJECT* PdoPtr;

    //
    // Used to track Device Properties list records allocation
    //
    BOOLEAN IsAllocated;
} USDHC_DEVICE_PROPERTIES;

//
// Device Properties list public interface
//

_IRQL_requires_max_(APC_LEVEL)
VOID
SdhcDevicePropertiesListInit();

_IRQL_requires_max_(DISPATCH_LEVEL)
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListSafeAddEntry();

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
DevicePropertiesListSafeRemoveEntry(
    _In_ USDHC_DEVICE_PROPERTIES* RecordPtr);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
DevicePropertiesListSafeRemoveByKey(
    _In_ UINT32 Key);

_IRQL_requires_max_(DISPATCH_LEVEL)
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListSafeFindByKey(
    _In_ UINT32 Key);

_IRQL_requires_max_(DISPATCH_LEVEL)
USDHC_DEVICE_PROPERTIES*
DevicePropertiesListSafeFindByPdo(
    _In_ const DEVICE_OBJECT* FdoPtr);

#endif // __DEVPROPSLIST_H__
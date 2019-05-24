/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

*/

#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif


NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    return STATUS_SUCCESS;
}

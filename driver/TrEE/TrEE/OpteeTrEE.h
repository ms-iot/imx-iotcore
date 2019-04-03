/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    OpteeTrEE.h

Abstract:

    The shared header file for the Optee TREE plugin driver.

Environment:

    Kernel mode

*/

#pragma once
#define OPTEE_TREE_POOL_TAG 'SerT'

NTSTATUS
TreeEnumSecureServicesFromACPI(
    _In_ WDFDEVICE Device,
    _In_ WDFCOLLECTION ServiceCollection
    );

NTSTATUS
TreeEnumSecureServicesFromRegistry(
    _In_ WDFDEVICE Device,
    _In_ WDFCOLLECTION ServiceCollection
    );

//
// Device context
//

struct _OPTEE_TREE_DEVICE_CONTEXT {
    BOOLEAN Enumerated;
    WDFCOLLECTION ServiceCollection;
    WDFDEVICE MasterDevice;
    PHYSICAL_ADDRESS SharedMemoryBaseAddress;
    ULONG SharedMemoryLength;
};

typedef struct _OPTEE_TREE_DEVICE_CONTEXT
    OPTEE_TREE_DEVICE_CONTEXT, *POPTEE_TREE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(OPTEE_TREE_DEVICE_CONTEXT, TreeGetDeviceContext);

TR_SECURE_SERVICE_CALLBACKS VARIABLE_SERVICE_CALLBACKS;
TR_SECURE_SERVICE_CALLBACKS FTPM_SERVICE_CALLBACKS;

TR_SECURE_SERVICE_CALLBACKS GEN_SERVICE_CALLBACKS;

extern KSPIN_LOCK ConcurrencyLock;
extern volatile LONG CurrentConcurrency;
extern volatile LONG ConcurrencyHighMark;
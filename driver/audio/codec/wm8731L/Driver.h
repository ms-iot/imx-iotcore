/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    User-mode Driver Framework 2

*/

#define INITGUID

#include <windows.h>
#include <wdf.h>

#include "device.h"
#include "queue.h"
#include "trace.h"

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD Wm8731LCodecEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP Wm8731LCodecEvtDriverContextCleanup;

EXTERN_C_END

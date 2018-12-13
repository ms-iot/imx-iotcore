/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imxi2cdriver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

*/

#ifndef _IMXI2CDRIVER_H_
#define _IMXI2CDRIVER_H_

//
// WDFDRIVER Events
//

EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;
EXTERN_C_END

EVT_WDF_DRIVER_DEVICE_ADD OnDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP OnDriverCleanup;

#endif // of _IMXI2CDRIVER_H_
/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    driver and application

*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_Wm8962Codec,
    0x558fcd57,0xb17a,0x4342,0xb4,0x9e,0x4d,0xf9,0x53,0x22,0xe2,0x0c);
// {558fcd57-b17a-4342-b49e-4df95322e20c}

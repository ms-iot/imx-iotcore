// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPIcommon.h
//
// Abstract:
//
//    This module contains common enum, types, and other definitions common
//    to the IMX ECSPI controller driver device driver.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//

#ifndef _ECSPI_COMMON_H_
#define _ECSPI_COMMON_H_

WDF_EXTERN_C_START


//
// ECSPI pool allocation tags
//
enum class ECSPI_ALLOC_TAG : ULONG {

    ECSPI_ALLOC_TAG_TEMP    = '0IPS', // Temporary be freed in the same routine
    ECSPI_ALLOC_TAG_WDF      = '@IPS'  // Allocations WDF makes on our behalf

}; // enum ECSPI_ALLOC_TAG


//
// Forward declaration of the ECSPI extension/contexts
//

typedef struct _ECSPI_DEVICE_EXTENSION ECSPI_DEVICE_EXTENSION;
typedef struct _ECSPI_TARGET_CONTEXT ECSPI_TARGET_CONTEXT;
typedef struct _ECSPI_TARGET_SETTINGS ECSPI_TARGET_SETTINGS;
typedef struct _ECSPI_SPB_REQUEST ECSPI_SPB_REQUEST;
typedef struct _ECSPI_SPB_TRANSFER ECSPI_SPB_TRANSFER;


//
// ECSPI common public methods
//

//
// Routine Description:
//
//  UlongWordSwap swaps the two 16bit words of a 32 bit value.
//
// Arguments:
//
//  Source - The original 32 bit value.
//
// Return Value:
//
//  A ULONG value with the 16bit words swapped.
//
__forceinline
ULONG
UlongWordSwap(
    _In_ ULONG Source
    )
{
    return ((Source & 0xFFFF) << 16) | (Source >> 16);
}


WDF_EXTERN_C_END


#endif // !_ECSPI_COMMON_H_

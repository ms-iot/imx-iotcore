/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    HalExtiMXDmaCfg.h

Abstract:

    This file includes the types/enums definitions associated with the
    i.MX SDMA HAL extension specific configuration.
    This file is intended to be used by external components that are iMX6 DMA aware
    like iMX device drivers for applying specific SDMA configuration settings.

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _HAL_EXT_IMX_SDMA_CFG
#define _HAL_EXT_IMX_SDMA_CFG

#ifdef __cplusplus
    extern "C" {
#endif // __cplusplus

//
// -------------------------------------------- Constants and Macro Definitions
//


//
// Max scatter-gather list size
//

#define SDMA_SG_LIST_MAX_SIZE 16UL

//
// Max bytes count per buffer descriptor
//

#define SDMA_BD_MAX_COUNT 0xFFFFUL

//
// Maximum transfer length (bytes)
//

#define SDMA_MAX_TRANSFER_LENGTH (SDMA_SG_LIST_MAX_SIZE * SDMA_BD_MAX_COUNT)

//
// Max watermark level value
//

#define SDMA_MAX_WATERMARK_LEVEL 0xFFFFUL


//
// ----------------------------------------------------------- Type Definitions
//


//
// SDMA Configuration Function IDs
//

enum _SDMA_CONFIG_FUNCTION_ID {

    SDMA_CFG_FUN_INVALID = 0,
    SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL = 0x8001,
    SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE = 0x8002,
    SDMA_CFG_FUN_RELEASE_REQUEST_LINE = 0x8003,
    SDMA_CFG_FUN_SET_CHANNEL_NOTIFICATION_THRESHOLD = 0x8004,

} SDMA_CONFIG_FUNCTION_ID;


//
// SDMA Configuration Function IDs data structures
//


//
// SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL input buffer:
// ULONG 
//  New watermark level value: 1..SDMA_MAX_WATERMARK_LEVEL, 0 (use default)
//

//
// SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE and 
// SDMA_CFG_FUN_RELEASE_REQUEST_LINE input buffer:
// ULONG 
//   The DMA request line as passed from 
//   CM_PARTIAL_RESOURCE_DESCRIPTOR.DmaV3.RequestLine
//

//
// SDMA_CFG_FUN_SET_CHANNEL_NOTIFICATION_THRESHOLD:
// ULONG 
//   The number of bytes transfered after which the caller
//   gets notified. 
//   Note:
//     It is NOT guarantied that the notification arrives after
//     the exact number of bytes have been transfered, but should be
//     'close enough'.
//     It is useful with auto initialize transfers where the caller
//     needs to be notified early enough to be able to
//     process the data in time.
//

#ifdef __cplusplus
    }
#endif // __cplusplus

#endif // !_HAL_EXT_IMX_SDMA_CFG

/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

UEFIVarServices.h

Abstract:

UEFI Variable Services definitions for accessing operations.

*/

#pragma once
#include <stddef.h>
#include <ntddk.h>
#include "OpteeClientLib\types.h"

//
// Function codes for calling UEFI Variable service
//

typedef enum _VARIABLE_SERVICE_OPS
{
    VSGetOp = 0,
    VSGetNextVarOp,
    VSSetOp,
    VSQueryInfoOp,
    VSSignalExitBootServicesOp,
} VARIABLE_SERVICE_OPS;

//
// In the WTR environment this was #pragma pack(1) however this does not
// work in the GP environment that requires alignment.
//

//
// Parameter Struct for Set Operations. No data returned in result
//

typedef struct _VARIABLE_SET_PARAM
{
    UINT32 Size;
    UINT16 VariableNameSize;
    GUID VendorGuid;
    UINT32 Attributes;
    UINT32 DataSize;
    UINT32 OffsetVariableName;
    UINT32 OffsetData;
    _Field_size_bytes_(DataSize + VariableNameSize) BYTE Payload[1];
} VARIABLE_SET_PARAM, *PVARIABLE_SET_PARAM;

//
//  Struct for Get  and GetNextVariable operation needs only name and Guid
//

typedef struct _VARIABLE_GET_PARAM
{
    UINT32 Size;
    UINT16 VariableNameSize;
    GUID VendorGuid;
    _Field_size_bytes_(VariableNameSize) WCHAR VariableName[1];
} VARIABLE_GET_PARAM, VARIABLE_GET_NEXT_PARAM, VARIABLE_GET_NEXT_RESULT, *PVARIABLE_GET_PARAM;
typedef struct _VARIABLE_GET_PARAM *PVARIABLE_GET_NEXT_PARAM, *PVARIABLE_GET_NEXT_RESULT;

//
// Result struct for Get operation
//

typedef struct _VARIABLE_GET_RESULT
{
    UINT32 Size;
    UINT32 Attributes;
    UINT32 DataSize;
    _Field_size_bytes_(DataSize) BYTE Data[1];
} VARIABLE_GET_RESULT, *PVARIABLE_GET_RESULT;

//
// Parameter struct for Query
//

typedef struct _VARIABLE_QUERY_PARAM
{
    UINT32 Size;
    UINT32 Attributes;
} VARIABLE_QUERY_PARAM, *PVARIABLE_QUERY_PARAM;

typedef struct _VARIABLE_QUERY_RESULT
{
    UINT32 Size;
    UINT64 MaximumVariableStorageSize;
    UINT64 RemainingVariableStorageSize;
    UINT64 MaximumVariableSize;
} VARIABLE_QUERY_RESULT, *PVARIABLE_QUERY_RESULT;

//
// Parameter struct for querying fragmentation statistics.
//
// NonVolatileSpaceUsed - Space marked as used in non volatile storage
// IdealNonVolatileSpaceUsed - Space that should have ideally been used considering name, attributes, guids, timestamps, keys,
//                              data, datasize, etc (gives idea of total frag)
// IdealWithOverheadSpace - Above + contribution due to variable pointers and blob pointers (gives an idea of external frag)
//

typedef struct _VS_GET_FRAG_STATS
{
    UINT32 NonVolatileSpaceUsed;
    UINT32 IdealNonVolatileSpaceUsed;
    UINT32 IdealWithOverheadSpace;
} VS_GET_FRAG_STATS, *PVS_GET_FRAG_STATS;

//
// ISSUE-2013/06/14-ARNOLDP: Refactor Ntefi.h
//

#define EFI_VARIABLE_NON_VOLATILE 0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE 0x00000040

#define EFI_KNOWN_ATTRIBUTES     (EFI_VARIABLE_NON_VOLATILE | \
                                  EFI_VARIABLE_BOOTSERVICE_ACCESS |\
                                  EFI_VARIABLE_RUNTIME_ACCESS |\
                                  EFI_VARIABLE_HARDWARE_ERROR_RECORD |\
                                  EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS |\
                                  EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS |\
                                  EFI_VARIABLE_APPEND_WRITE)

#define TEE_RESULT_FROM_NT(ntStatus)     (0xFF000000 | (ntStatus))
#define STR_SIZE(var) wcslen(var)==0 ? 0 : ((wcslen(var) + 1) * sizeof(WCHAR))


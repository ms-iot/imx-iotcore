/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

OpteeFtpmService.h

Abstract:

Firmware TPM physical presence interface for UEFI

*/

#pragma once
#include <stddef.h>
#include <ntddk.h>
#include "OpteeClientLib\types.h"

#define TA_FTPM_SUBMIT_COMMAND (0)

#define PPI_INTERFACE_VERSION "1.3"

//
// The preliminary official signature for the static ACPI table
//
#define ACPI_SIGNATURE_TPM20        0x324d5054 // "TPM2"
#define ACPI_SIGNATURE_TPM12        0x41504354 // "TCPA"

//
// Supported revisions of the static ACPI table layout
//
#define ACPI_TPM2_REVISION_3        3
#define ACPI_TPM2_REVISION_4        4

enum {
    TpmStateReady = 0,
    TpmStateIdle
};

#define CONTROL_AREA_REQUEST_CMD_READY  (0x00000001)
#define CONTROL_AREA_REQUEST_GOIDLE     (0x00000002)

#define CONTROL_AREA_STATUS_ERROR       (0x00000001)
#define CONTROL_AREA_STATUS_IDLE        (0x00000002)

#define CONTROL_AREA_START_START        (0x00000001)

typedef struct _FTPM_CONTROL_AREA
{
    //
    // This used to a Reserved field. This is the Miscellaneous field for the Command/Response interface.
    // 
    volatile UINT32  Request;
    
    //
    // The Status field of the Control area.
    //
    volatile UINT32  Status;
    
    //
    // The Cancel field of the Control area. TPM does not modify this field, hence it is not declared volatile.
    //
    UINT32  Cancel;
    
    //
    // The Start field of the Control area.
    //
    volatile UINT32  Start;

    //
    // The control area is in device memory. Device memory
    // often only supports word sized access. Split all
    // 64bit fields into High and Low parts.
    //
    UINT32 InterruptControlLow;
    UINT32 InterruptControlHigh;

    //
    // Command buffer size.
    //
    UINT32 CommandBufferSize;

    //
    // The control area is in device memory. Device memory
    // often only supports word sized access. Split all
    // 64bit fields into High and Low parts.
    //
    // Command buffer physical address.
    //
    UINT32 CommandPALow;
    UINT32 CommandPAHigh;

    //
    // Response Buffer size.
    //
    UINT32 ResponseBufferSize;

    //
    // The control area is in device memory. Device memory
    // often only supports word sized access. Split all
    // 64bit fields into High and Low parts.
    //
    // Response buffer physical address.
    //
    UINT32 ResponsePALow;
    UINT32 ResponsePAHigh;
} FTPM_CONTROL_AREA, *PFTPM_CONTROL_AREA;

//
// From UEFI Tcg2PhysicalPresenceData.h
//

//
// Non-volatile varaibles used to pass PPI data to UEFI
//
GUID PpiVariableGuid = {
    0xaeb9c5c1, 0x94f1, 0x4d02, { 0xbf, 0xd9, 0x46, 0x2, 0xdb, 0x2d, 0x3c, 0x54 }
};

#define PPI_VARIABLE_NAME L"Tcg2PhysicalPresence"
UNICODE_STRING PpiVariableNameUnicode = {
        40,
        42,
        PPI_VARIABLE_NAME
};

typedef struct _TCG2_PPI_VARIABLE {
    UINT8   Request;
    UINT32  RequestParameter;
    UINT8   LastRequest;
    UINT32  Response;
} TCG2_PPI_VARIABLE, *PTCG2_PPI_VARIABLE;

#define PPI_FLAGS_VARIABLE_NAME L"Tcg2PhysicalPresenceFlags"
UNICODE_STRING PpiFlagsVariableNameUnicode = {
        50,
        52,
        PPI_FLAGS_VARIABLE_NAME
};

//
// From UEFI Tcg2PhysicalPresenceLib.h
//

//
// UEFI TCG2 library definition bit of the BIOS TPM Management Flags
//
#define TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CLEAR 0x02

//
// From UEFI TcgPhysicalPresence.h
//

//
// TCG PP definition for physical presence ACPI function
//
#define TCG_ACPI_FUNCTION_GET_PHYSICAL_PRESENCE_INTERFACE_VERSION       1
#define TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS                        2
#define TCG_ACPI_FUNCTION_GET_PENDING_REQUEST_BY_OS                     3
#define TCG_ACPI_FUNCTION_GET_PLATFORM_ACTION_TO_TRANSITION_TO_BIOS     4
#define TCG_ACPI_FUNCTION_RETURN_REQUEST_RESPONSE_TO_OS                 5
#define TCG_ACPI_FUNCTION_SUBMIT_PREFERRED_USER_LANGUAGE                6
#define TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS_2                      7
#define TCG_ACPI_FUNCTION_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST      8

//
// TCG PP definition of physical presence operation actions for TPM2
// 
// This driver only verifies the expected success of clear commands, UEFI
// will determine if other commands (0->34) are valid at runtime.
//
#define TCG2_PHYSICAL_PRESENCE_NO_ACTION        0
#define TCG2_PHYSICAL_PRESENCE_CLEAR            5
#define TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR     14
#define TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2   21
#define TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3   22

//
// Selected return codes from the PPI spec, and TcgPhysicalPresence.h
//
#define PPI_SUCCESS 0

//
// Return codes for TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS
//
#define PPI_SUBMIT_OPERATION_NOT_SUPPORTED      1
#define PPI_SUBMIT_OPERATION_ERROR              2

//
// Return codes for TCG_ACPI_FUNCTION_GET_PLATFORM_ACTION_TO_TRANSITION_TO_BIOS
//
#define PPI_TRANSITION_ACTION_NONE              0
#define PPI_TRANSITION_ACTION_SHUTDOWN          1
#define PPI_TRANSITION_ACTION_REBOOT            2
#define PPI_TRANSITION_ACTION_VENDOR            3

//
// Return codes for TCG_ACPI_FUNCTION_RETURN_REQUEST_RESPONSE_TO_OS
//
#define PPI_OPERATION_RESPONSE_ERROR            1

//
// Return codes for TCG_ACPI_FUNCTION_SUBMIT_PREFERRED_USER_LANGUAGE (deprecated)
//
#define PPI_LANGUAGE_NOT_IMPLEMENTED            3

//
// Return codes for TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS_2
//
#define PPI_SUMBIT_OPERATION2_NOT_IMPLEMENTED   PPI_SUBMIT_OPERATION_NOT_SUPPORTED
#define PPI_SUMBIT_OPERATION2_ERROR             PPI_SUBMIT_OPERATION_ERROR
#define PPI_SUMBIT_OPERATION2_BLOCKED           3

//
// Return codes for TCG_ACPI_FUNCTION_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST
//
#define PPI_USER_CONFIRMATION_NOT_IMPLEMENTED   0
#define PPI_USER_CONFIRMATION_FIRMWARE          1
#define PPI_USER_CONFIRMATION_BLOCKED           2
#define PPI_USER_CONFIRMATION_PP_REQUIRED       3
#define PPI_USER_CONFIRMATION_PP_NOT_REQUIRED   4

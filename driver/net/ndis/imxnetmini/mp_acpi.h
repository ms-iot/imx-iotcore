/*
* Copyright 2018 NXP
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the disclaimer
* below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* * Neither the name of NXP nor the names of its contributors may be used to
* endorse or promote products derived from this software without specific prior
* written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
* LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#pragma once

EXTERN_C_START

#define IMX_ENET_DSM_REVISION 0
#define IMX_ENET_DSM_FUNCTION_SUPPORTED_FUNCTIONS_INDEX             0
#define IMX_ENET_DSM_FUNCTION_GET_MDIO_BUS_ENET_PHY_ADDRESS_INDEX   1
#define IMX_ENET_DSM_FUNCTION_GET_MAC_ADDRESS_INDEX                 2
#define IMX_ENET_DSM_FUNCTION_GET_MDIO_BASE_ADDRESS_INDEX           3
#define IMX_ENET_DSM_FUNCTION_GET_ENET_PHY_INTERFACE_TYPE_INDEX     4

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS Acpi_Init(_In_ PMP_ADAPTER pAdapter, _Out_ ULONG *pACPI_SupportedFunctions);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS Acpi_GetValue(_In_ PMP_ADAPTER pAdapter, _In_ ULONG FnIndex, _Out_ PVOID pBuffer, _In_ ULONG BufferSize);

EXTERN_C_END
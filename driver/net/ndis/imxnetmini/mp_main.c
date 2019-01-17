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

#include "precomp.h"

extern NDIS_OID ENETSupportedOids[];
extern ULONG    ENETSupportedOidsSize;

// Global var of ENET Ndis Handler
NDIS_HANDLE          NdisMiniportDriverHandle;
MP_MDIO_DRIVER      *pDriver = NULL;

NDIS_STATUS DriverEntry(_In_ PVOID DriverObject, _In_ PVOID RegistryPath);

#pragma NDIS_INIT_FUNCTION(DriverEntry)
#pragma NDIS_PAGEABLE_FUNCTION(MpUnloadDriver)
#pragma NDIS_PAGEABLE_FUNCTION(MpInitializeEx)

/*++
Routine Description:
    In the context of its this function, a miniport driver associates itself with NDIS,
    specifies the NDIS version that it is using, and registers its entry points.
Arguments:
    DriverObject    pointer to the driver object.
    RegistryPath    pointer to the driver registry path.
Return Value:
    NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
--*/
_Use_decl_annotations_
NDIS_STATUS DriverEntry(PVOID DriverObject, PVOID RegistryPath) {
    NDIS_STATUS                           Status;
    NDIS_MINIPORT_DRIVER_CHARACTERISTICS  MpChar;

    PAGED_CODE();
    DBG_DRV_METHOD_BEG_WITH_PARAMS("Driver: 0x%08X, '%S')", (unsigned int)DriverObject, ((PUNICODE_STRING)RegistryPath)->Buffer);
    DBG_DRV_PRINT_INFO("***********************************************************************************");
    DBG_DRV_PRINT_INFO("*** NXP ndis miniport driver, date: %s %s                        ***", __DATE__, __TIME__);
    DBG_DRV_PRINT_INFO("***********************************************************************************");
    do {
        if ((pDriver = (PMP_MDIO_DRIVER)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(MP_MDIO_DRIVER), MP_TAG_DRV)) == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            DBG_DRV_PRINT_ERROR_WITH_STATUS("ExAllocatePoolWithTag() failed.");
            break;
        }
        RtlZeroMemory(pDriver, sizeof(MP_MDIO_DRIVER));
        #if DBG
        RtlStringCbPrintfA(pDriver->MDIODrv_DeviceName, sizeof(pDriver->MDIODrv_DeviceName) - 1, "ENETx");
        #endif
        NdisAllocateSpinLock(&pDriver->MDIODrv_DriverSpinLock);
        // Fill-in adapter characteristics before calling NdisMRegisterMiniport
        NdisZeroMemory(&MpChar, sizeof(MpChar));
        {C_ASSERT(sizeof(MpChar) >= NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2); }
        MpChar.Header.Type                  = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS;
        MpChar.Header.Size                  = NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;
        MpChar.Header.Revision              = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;
        MpChar.MajorNdisVersion             = ENET_NDIS_MAJOR_VERSION;
        MpChar.MinorNdisVersion             = ENET_NDIS_MINOR_VERSION;
        MpChar.MajorDriverVersion           = NIC_MAJOR_DRIVER_VERSION;
        MpChar.MinorDriverVersion           = NIC_MINOR_DRIVER_VERISON;
        MpChar.InitializeHandlerEx          = MpInitializeEx;
        MpChar.HaltHandlerEx                = MpHaltEx;
        MpChar.UnloadHandler                = MpUnloadDriver;
        MpChar.PauseHandler                 = MpPause;
        MpChar.RestartHandler               = MpRestart;
        MpChar.OidRequestHandler            = MpOidRequest;
        MpChar.SendNetBufferListsHandler    = MpSendNetBufferLists;
        MpChar.ReturnNetBufferListsHandler  = MpReturnNetBufferLists;
        MpChar.CancelSendHandler            = MpCancelSendNetBufferLists;
        MpChar.CheckForHangHandlerEx        = MpCheckForHangEx;
        MpChar.ResetHandlerEx               = MpResetEx;
        MpChar.DevicePnPEventNotifyHandler  = MpDevicePnPEventNotify;
        MpChar.ShutdownHandlerEx            = MpShutdownEx;
        MpChar.CancelOidRequestHandler      = MpCancelOidRequest;
        // Now register Miniport
        Status = NdisMRegisterMiniportDriver(DriverObject, RegistryPath, (NDIS_HANDLE)pDriver, &MpChar, &NdisMiniportDriverHandle);
        if (NDIS_STATUS_SUCCESS != Status) {
            Status = NDIS_STATUS_FAILURE;
            DBG_DRV_PRINT_ERROR_WITH_STATUS("NdisMRegisterMiniportDriver() failed.");
            break;
        }
    }  while (0);
    DBG_DRV_METHOD_END_WITH_STATUS(Status);
    return (Status);
}

/*++
Routine Description:
    NDIS MiniportUnloadDriver callback handler.
    NDIS calls MiniportUnloadDriver at IRQL = PASSIVE_LEVEL.
Arguments:
    DriverObject Not used
Return Value:
    None
--*/
_Use_decl_annotations_
VOID MpUnloadDriver(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();
    DBG_DRV_METHOD_BEG();
    NdisMDeregisterMiniportDriver(NdisMiniportDriverHandle);  // Deregister Miniport driver
    if (pDriver) {
        ExFreePoolWithTag(pDriver, MP_TAG_DRV);
    }
    DBG_DRV_METHOD_END();
}

/*++
Routine Description:
    This function finds and initializes the ENET adapter. When the ENET driver calls
    the NdisMRegisterMiniport from its DriverEntry function, NDIS will call
    ENETInitialize in the context of NdisMRegisterMiniport.
    NDIS calls MiniportInitializeEx at IRQL = PASSIVE_LEVEL.
Parameters:
    MiniportAdapterHandle
        Specifies a handle identifying the ENET driver, which is assigned by
        the NDIS library. It is a required parameter in subsequent calls
        to NdisXXX functions.
    MiniportDriverContext
        Specifies a handle used only during initialization for calls to
        NdisXXX configuration and initialization functions. For example,
        this handle is passed to NDIS when we registered the driver
    MiniportInitParameters
        Initialization parameters
Returns:
    NDIS_STATUS_SUCCESS if initialization is success.
    NDIS_STATUS_FAILURE or NDIS_STATUS_UNSUPPORTED_MEDIA if not.

*/
_Use_decl_annotations_
NDIS_STATUS MpInitializeEx(NDIS_HANDLE MiniportAdapterHandle, NDIS_HANDLE MiniportDriverContext, PNDIS_MINIPORT_INIT_PARAMETERS  MiniportInitParameters)
{
    NDIS_STATUS                                     Status = NDIS_STATUS_SUCCESS;
    NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS         Interrupt;
    NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES   RegistrationAttributes;
    NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES        GeneralAttributes;
    NDIS_PM_CAPABILITIES                            PowerManagementCapabilities;
    PMP_ADAPTER                                     pAdapter = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR                 pResDesc;
    ULONG                                           index;
    PMP_MDIO_DRIVER                                 pMDIODriver = (PMP_MDIO_DRIVER)MiniportDriverContext;

    PAGED_CODE();

//    DBG_INIT_METHOD_BEG_WITH_DEV_NUM_AND_PARAMS(DeviceNumber, "Adapter handle: 0x%p", MiniportAdapterHandle);
    DBG_ENET_DEV_METHOD_BEG_WITH_PARAMS("Adapter handle: 0x%p", MiniportAdapterHandle);
    for(;;) {
        // Allocate MP_ADAPTER structure
        if ((Status = MpAllocAdapterBlock(&pAdapter, MiniportAdapterHandle)) != NDIS_STATUS_SUCCESS) {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("MpAllocAdapterBlock() failed.");
            break;
        }
        #if DBG
        RtlStringCbPrintfA(pAdapter->ENETDev_DeviceName, sizeof(pAdapter->ENETDev_DeviceName) - 1, "ENET%d", NdisInterlockedIncrement(&pMDIODriver->MDIODrv_EnetDeviceCount) - 1);
        RtlStringCbCopyA(pAdapter->ENETDev_PHYDevice.PHYDev_DeviceName, sizeof(pAdapter->ENETDev_PHYDevice.PHYDev_DeviceName) - 1, pAdapter->ENETDev_DeviceName);
        #endif
        // Register miniport
        NdisZeroMemory(&RegistrationAttributes, sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES));
        NdisZeroMemory(&GeneralAttributes, sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES));
        RegistrationAttributes.Header.Type               = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        RegistrationAttributes.Header.Revision           = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
        RegistrationAttributes.Header.Size               = sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES);
        RegistrationAttributes.MiniportAdapterContext    = (NDIS_HANDLE)pAdapter;
        RegistrationAttributes.AttributeFlags            = NDIS_MINIPORT_ATTRIBUTES_HARDWARE_DEVICE | NDIS_MINIPORT_ATTRIBUTES_BUS_MASTER | NDIS_MINIPORT_ATTRIBUTES_REGISTER_BUGCHECK_CALLBACK;
        RegistrationAttributes.CheckForHangTimeInSeconds = MAC_IS_ALIVE_POLL_SEC;
        RegistrationAttributes.InterfaceType             = NIC_INTERFACE_TYPE;
        if ((Status = NdisMSetMiniportAttributes(MiniportAdapterHandle, (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&RegistrationAttributes)) != NDIS_STATUS_SUCCESS) {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMSetMiniportAttributes() failed.");
            break;
        }
        (void)Acpi_Init(pAdapter, &pAdapter->ENETDev_bmACPISupportedFunctions);              // Initialize ACPI
        // Try to get MAC address from ACPI
        if (!NT_SUCCESS(Acpi_GetValue(pAdapter, IMX_ENET_DSM_FUNCTION_GET_MAC_ADDRESS_INDEX, pAdapter->FecMacAddress, sizeof(pAdapter->FecMacAddress)))) {
            DBG_ENET_DEV_PRINT_INFO("Acpi_GetValue(GET_MAC_ADDRESS) failed.");
        }  else {
            Status = STATUS_ACPI_INVALID_DATA;
            DBG_ENET_DEV_PRINT_INFO("ENET MAC address: %02X-%02X-%02X-%02X-%02X-%02X (from ACPI)", pAdapter->FecMacAddress[0], pAdapter->FecMacAddress[1], pAdapter->FecMacAddress[2], pAdapter->FecMacAddress[3], pAdapter->FecMacAddress[4], pAdapter->FecMacAddress[5]);
        }
        // Read the registry parameters
        if ((Status = NICReadRegParameters(pAdapter)) != NDIS_STATUS_SUCCESS) {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NICReadRegParameters() failed.");
            pAdapter->NdisInterruptHandle = NULL;  // SDV bug fix
            break;
        }
        #ifdef DBG
        if (!(pAdapter->FecMacAddress[0] || pAdapter->FecMacAddress[1] || pAdapter->FecMacAddress[2] || pAdapter->FecMacAddress[3] || pAdapter->FecMacAddress[4] || pAdapter->FecMacAddress[5])) {
            Status = NDIS_STATUS_FAILURE;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS("ENET MAC address is not valid"," %02X-%02X-%02X-%02X-%02X-%02X", pAdapter->FecMacAddress[0], pAdapter->FecMacAddress[1], pAdapter->FecMacAddress[2], pAdapter->FecMacAddress[3], pAdapter->FecMacAddress[4], pAdapter->FecMacAddress[5]);
            break;
        }
        #endif
        DBG_ENET_DEV_PRINT_INFO("ENET MAC address: %02X-%02X-%02X-%02X-%02X-%02X (used by device)", pAdapter->FecMacAddress[0], pAdapter->FecMacAddress[1], pAdapter->FecMacAddress[2], pAdapter->FecMacAddress[3], pAdapter->FecMacAddress[4], pAdapter->FecMacAddress[5]);
        // Set up generic attributes
        GeneralAttributes.Header.Type       = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        GeneralAttributes.Header.Revision   = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
        GeneralAttributes.Header.Size       = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
        GeneralAttributes.MediaType         = NIC_MEDIA_TYPE;
        GeneralAttributes.MtuSize           = ETHER_FRAME_MAX_PAYLOAD_LENGTH;
        GeneralAttributes.MaxXmitLinkSpeed  = NIC_MEDIA_MAX_SPEED;
        GeneralAttributes.MaxRcvLinkSpeed   = NIC_MEDIA_MAX_SPEED;
        GeneralAttributes.XmitLinkSpeed     = NDIS_LINK_SPEED_UNKNOWN;
        GeneralAttributes.RcvLinkSpeed      = NDIS_LINK_SPEED_UNKNOWN;
        GeneralAttributes.MediaConnectState = MediaConnectStateUnknown;
        GeneralAttributes.MediaDuplexState  = MediaDuplexStateUnknown;
        GeneralAttributes.LookaheadSize     = ETHER_FRAME_MAX_PAYLOAD_LENGTH;

        NdisZeroMemory(&PowerManagementCapabilities, sizeof(PowerManagementCapabilities));
        PowerManagementCapabilities.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        PowerManagementCapabilities.Header.Revision = NDIS_PM_CAPABILITIES_REVISION_2;
        PowerManagementCapabilities.Header.Size     = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2;
        GeneralAttributes.PowerManagementCapabilitiesEx = &PowerManagementCapabilities;

        GeneralAttributes.MacOptions              = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | NDIS_MAC_OPTION_TRANSFERS_NOT_PEND | NDIS_MAC_OPTION_NO_LOOPBACK;
        GeneralAttributes.SupportedPacketFilters  = NIC_SUPPORTED_FILTERS;
        GeneralAttributes.MaxMulticastListSize    = NIC_MAX_MCAST_LIST;
        GeneralAttributes.MacAddressLength        = ETH_LENGTH_OF_ADDRESS;
        NdisMoveMemory(GeneralAttributes.PermanentMacAddress, pAdapter->PermanentAddress, ETH_LENGTH_OF_ADDRESS);
        NdisMoveMemory(GeneralAttributes.CurrentMacAddress, pAdapter->CurrentAddress, ETH_LENGTH_OF_ADDRESS);

        GeneralAttributes.PhysicalMediumType     = NdisPhysicalMedium802_3;
        GeneralAttributes.RecvScaleCapabilities  = NULL;
        GeneralAttributes.AccessType             = NET_IF_ACCESS_BROADCAST;      // NET_IF_ACCESS_BROADCAST for a typical ethernet adapter
        GeneralAttributes.DirectionType          = NET_IF_DIRECTION_SENDRECEIVE; // NET_IF_DIRECTION_SENDRECEIVE for a typical ethernet adapter
        GeneralAttributes.ConnectionType         = NET_IF_CONNECTION_DEDICATED;  // NET_IF_CONNECTION_DEDICATED for a typical ethernet adapter
        GeneralAttributes.IfType                 = IF_TYPE_ETHERNET_CSMACD;      // IF_TYPE_ETHERNET_CSMACD for a typical ethernet adapter (regardless of speed)
        GeneralAttributes.IfConnectorPresent     = TRUE; // RFC 2665 TRUE if physical adapter
        GeneralAttributes.SupportedStatistics = NDIS_STATISTICS_RCV_DISCARDS_SUPPORTED |
                                                NDIS_STATISTICS_RCV_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_BYTES_RCV_SUPPORTED |
                                                NDIS_STATISTICS_DIRECTED_FRAMES_RCV_SUPPORTED |
                                                NDIS_STATISTICS_MULTICAST_FRAMES_RCV_SUPPORTED |
                                                NDIS_STATISTICS_BROADCAST_FRAMES_RCV_SUPPORTED |
                                                NDIS_STATISTICS_BYTES_XMIT_SUPPORTED |
                                                NDIS_STATISTICS_DIRECTED_FRAMES_XMIT_SUPPORTED |
                                                NDIS_STATISTICS_MULTICAST_FRAMES_XMIT_SUPPORTED |
                                                NDIS_STATISTICS_BROADCAST_FRAMES_XMIT_SUPPORTED |
                                                NDIS_STATISTICS_XMIT_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_XMIT_DISCARDS_SUPPORTED;
            //NDIS_STATISTICS_GEN_STATISTICS_SUPPORTED; // it is not in documentation
        GeneralAttributes.SupportedOidList       = ENETSupportedOids;
        GeneralAttributes.SupportedOidListLength = ENETSupportedOidsSize;

        if ((Status = NdisMSetMiniportAttributes(MiniportAdapterHandle, (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&GeneralAttributes)) != NDIS_STATUS_SUCCESS) {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMSetMiniportAttributes() failed.");
            break;
        }
        // Get HW resources
        NDIS_PHYSICAL_ADDRESS  ENETRegBase;
        BOOLEAN                GotInterrupt = FALSE;
        BOOLEAN                GotMemory = FALSE;
        ENETRegBase.QuadPart = 0; // suppress Warning C4701 potentially uninitialized local variable 'ENETRegBase'
        if (MiniportInitParameters->AllocatedResources) {
            for (index = 0; index < MiniportInitParameters->AllocatedResources->Count; index++) {
                pResDesc = &MiniportInitParameters->AllocatedResources->PartialDescriptors[index];
                switch (pResDesc->Type) {
                    case CmResourceTypeInterrupt:
                        DBG_CODE(if (!GotInterrupt) pAdapter->ENETDev_Irq = pResDesc->u.Interrupt.Level);
                        GotInterrupt = TRUE;
                        DBG_ENET_DEV_PRINT_INFO("InterruptLevel  = 0x%x", pResDesc->u.Interrupt.Level);
                        DBG_ENET_DEV_PRINT_INFO("InterruptVector = 0x%x", pResDesc->u.Interrupt.Vector);
                        break;
                    case CmResourceTypeMemory:
                        ENETRegBase = pResDesc->u.Memory.Start;
                        DBG_CODE(if (!GotMemory) pAdapter->ENETDev_RegsPhyAddress = ENETRegBase);
                        GotMemory = TRUE;
                        DBG_ENET_DEV_PRINT_INFO("MemPhysAddress(Low)  = 0x%08x", NdisGetPhysicalAddressLow(ENETRegBase));
                        DBG_ENET_DEV_PRINT_INFO("MemPhysAddress(High) = 0x%08x", NdisGetPhysicalAddressHigh(ENETRegBase));
                        break;
                    default:
                        break;
                }  // switch
            } // for
            if (!GotMemory || !GotInterrupt) {
                Status = NDIS_STATUS_RESOURCE_CONFLICT;
                DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("Interrupt vector or IO memory address not found.");
                break;
            }
        } else {
            Status = NDIS_ERROR_CODE_RESOURCE_CONFLICT;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("MiniportInitParameters->AllocatedResources == NULL.");
            break;
        }
        // Get registers base address
        if ((Status = NdisMMapIoSpace((PVOID *)&pAdapter->ENETRegBase, MiniportAdapterHandle, ENETRegBase, sizeof(CSP_ENET_REGS))) != NDIS_STATUS_SUCCESS) {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMMapIoSpace() failed.");
            pAdapter->NdisInterruptHandle = NULL;  // SDV bug fix
            break;
        }
        // Get MDIO registers base address
        MP_MDIO_ENET_PHY_CFG EnetPhyConfig;
        EnetPhyConfig.MDIOCfg_RegsPhyAddress               = ENETRegBase;  // Suppose same address for both ENET device and MDIO device.
        EnetPhyConfig.MDIOCfg_EnetPhyAddress               = 0;            // Suppose zero address (For backward compatibility).
        EnetPhyConfig.MDIOCfg_PhyInterfaceType             = RGMII;        // ENET PHY device interface (MII/RMII/RGMII)
        EnetPhyConfig.MDIOCfg_MDIOControllerInputClk_kHz   = 66000;        // TODO - read from ACPI?
        EnetPhyConfig.MDIOCfg_MDCFreq_kHz                  = 2500;         // TODO - read from PhyInfo? (802.3 max. value)
        EnetPhyConfig.MDIOCfg_STAHoldTime_ns               = 10;           // TODO - read from PhyInfo? (802.3 min. value)
        EnetPhyConfig.MDIOCfg_DisableFramePreamble         = FALSE;
        if (!NT_SUCCESS(Acpi_GetValue(pAdapter, IMX_ENET_DSM_FUNCTION_GET_MDIO_BASE_ADDRESS_INDEX, &EnetPhyConfig.MDIOCfg_RegsPhyAddress.LowPart, sizeof(ULONG)))) {
            DBG_ENET_DEV_PRINT_INFO("MDIO Controller address was not found in ACPI. ENET Controller address will be used.");
        }
        DBG_ENET_DEV_PRINT_INFO("ENET Controller Physical Address = 0x%08x_%08x", NdisGetPhysicalAddressHigh(ENETRegBase), NdisGetPhysicalAddressLow(ENETRegBase));
        DBG_ENET_DEV_PRINT_INFO("MDIO Controller Physical Address = 0x%08x_%08x", NdisGetPhysicalAddressHigh(EnetPhyConfig.MDIOCfg_RegsPhyAddress), NdisGetPhysicalAddressLow(EnetPhyConfig.MDIOCfg_RegsPhyAddress));
        if (!NT_SUCCESS(Acpi_GetValue(pAdapter, IMX_ENET_DSM_FUNCTION_GET_MDIO_BUS_ENET_PHY_ADDRESS_INDEX, &EnetPhyConfig.MDIOCfg_EnetPhyAddress, sizeof(EnetPhyConfig.MDIOCfg_EnetPhyAddress)))) {
            DBG_ENET_DEV_PRINT_INFO("MDIO bus ENET PHY address was not found in ACPI. Address 0 will be used as default.");
        } else {
            DBG_ENET_DEV_PRINT_INFO("MDIO bus ENET PHY Address: %d", EnetPhyConfig.MDIOCfg_EnetPhyAddress);
        }
        if (!NT_SUCCESS(Acpi_GetValue(pAdapter, IMX_ENET_DSM_FUNCTION_GET_ENET_PHY_INTERFACE_TYPE_INDEX, &EnetPhyConfig.MDIOCfg_PhyInterfaceType, sizeof(EnetPhyConfig.MDIOCfg_PhyInterfaceType)))) {
            DBG_ENET_DEV_PRINT_INFO("ENET PHY interface type was not found in ACPI. RGMII will be used as default.");
        } else {
            DBG_ENET_DEV_PRINT_INFO("ENET PHY interface type: %s", Dbg_GetEnetPhyInterfaceTypeName(EnetPhyConfig.MDIOCfg_PhyInterfaceType));
        }
        pAdapter->ENETDev_MDIODevice.MDIODev_pEnetAdapter = pAdapter;
        if ((Status = MDIODev_InitDevice(pMDIODriver, &EnetPhyConfig, &pAdapter->ENETDev_MDIODevice)) != NDIS_STATUS_SUCCESS) {
            pAdapter->NdisInterruptHandle = NULL;  // SDV bug fix
            break;
        }
        pAdapter->ENETDev_PHYDevice.PHYDev_pMDIODev = &pAdapter->ENETDev_MDIODevice;
        pAdapter->ENETDev_PHYDevice.PHYDev_MediaConnectState = MediaConnectStateUnknown;
        pAdapter->ENETDev_PHYDevice.PHYDev_DuplexMode = MediaDuplexStateUnknown;
        pAdapter->ENETDev_PHYDevice.PHYDev_LinkSpeed = 0;
        pAdapter->ENETDev_PHYDevice.PHYDev_LPApause = PHY_LPA_INIT;
        if ((Status = PHYDev_GetPhyId(&pAdapter->ENETDev_PHYDevice)) != NDIS_STATUS_SUCCESS) {
            pAdapter->NdisInterruptHandle = NULL;  // SDV bug fix
            break;
        }
        // Allocate all other memory blocks including shared memory
        if ((Status = NICAllocAdapterMemory(pAdapter)) != NDIS_STATUS_SUCCESS)  {
            pAdapter->NdisInterruptHandle = NULL;  // SDV bug fix
            break;
        }
        EnetInit(pAdapter, EnetPhyConfig.MDIOCfg_PhyInterfaceType);
        NdisZeroMemory(&Interrupt, sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS));
        Interrupt.Header.Type             = NDIS_OBJECT_TYPE_MINIPORT_INTERRUPT;
        Interrupt.Header.Revision         = NDIS_MINIPORT_INTERRUPT_REVISION_1;
        Interrupt.Header.Size             = sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS);
        Interrupt.InterruptHandler        = EnetIsr;
        Interrupt.InterruptDpcHandler     = EnetDpc;
        pAdapter->NdisInterruptHandle = NULL;  // SDV bug fix
        if ((Status = NdisMRegisterInterruptEx(pAdapter->AdapterHandle, pAdapter, &Interrupt, &pAdapter->NdisInterruptHandle)) != NDIS_STATUS_SUCCESS) {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMRegisterInterruptEx() failed.");
            break;
        }

        if (!PHYDev_ConfigurePhy(pAdapter)) {
            Status = NDIS_STATUS_DEVICE_FAILED;
            break;
        }
        break;
    }
    if (Status != NDIS_STATUS_SUCCESS)  {
        DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("MpInitializeEx() failed. Driver is getting unloaded.");
        MpFreeAdapter(pAdapter);
    } else {
        Dbg_PrintENETDeviceSettings(pAdapter);
    }
    DBG_ENET_DEV_METHOD_END_WITH_STATUS(Status);
    return Status;
}

/*++
Routine Description:
    NDIS MiniportHaltEx callback handler.
    Halt handler is called when NDIS receives IRP_MN_STOP_DEVICE, IRP_MN_SUPRISE_REMOVE or IRP_MN_REMOVE_DEVICE requests from the PNP manager.
    NDIS does not call MiniportHaltEx if there are outstanding OID requests or send requests.
    NDIS submits no further requests for the affected device after NDIS calls MiniportHaltEx.
    NDIS calls MiniportPause at IRQL = PASSIVE_LEVEL.
Arguments:
    MiniportAdapterContext
        Adapter context address
    HaltAction
        The reason for halting the adapter
Return Value:
    None
--*/
_Use_decl_annotations_
void MpHaltEx(NDIS_HANDLE MiniportAdapterContext, NDIS_HALT_ACTION HaltAction)
{
    PMP_ADAPTER  pAdapter = (PMP_ADAPTER)MiniportAdapterContext;

    DBG_SM_METHOD_BEG();
    UNREFERENCED_PARAMETER(HaltAction);
    ASSERT(pAdapter->Tx_PendingNBs == 0);
    NdisResetEvent(&pAdapter->StateMachine.SM_EnetHaltedEvent);                                            // Set event to not-signaled state
    (void)SmSetState(pAdapter, SM_STATE_HALTED, MP_SM_NEXT_STATE_IMMEDIATELY, SM_CALLED_BY_NDIS);          // Switch to the HALTED state immediately
    if (!NdisWaitEvent(&pAdapter->StateMachine.SM_EnetHaltedEvent, MP_SM_WAIT_FOR_SM_EXIT_TIMEOUT_MSEC)) { // Wait until the state is changed and Enet device is not running
        DBG_ENET_DEV_PRINT_ERROR("Failed to stop State engine. Adapter context will not be released.");
    } else {
        MpFreeAdapter(pAdapter);  // Free the entire adapter object, including the shared memory structures.
    }
 //   DBG_HALT_EX_METHOD_END_SM();
}

/*++
Routine Description:
    NDIS MiniportPause callback handler.
    NDIS calls MiniportPause at IRQL = PASSIVE_LEVEL.
Arguments:
    MiniportAdapterContext
        Adapter context address
    MiniportPauseParameters
        Additional information about the pause operation
Return Value:
    NDIS_STATUS_PENDING
--*/
_Use_decl_annotations_
NDIS_STATUS MpPause(NDIS_HANDLE MiniportAdapterContext, PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters)
{
    PMP_ADAPTER         pAdapter = (PMP_ADAPTER)MiniportAdapterContext;

    DBG_SM_METHOD_BEG();
    UNREFERENCED_PARAMETER(MiniportPauseParameters);
    (void)SmSetState(pAdapter, SM_STATE_PAUSING, MP_SM_NEXT_STATE_IMMEDIATELY, SM_CALLED_BY_NDIS);  // Switch to the PAUSING state immediately
    DBG_SM_METHOD_END_WITH_STATUS(NDIS_STATUS_PENDING);
    return NDIS_STATUS_PENDING;
}

/*++
Routine Description:
    NDIS MiniportRestart callback handler.
    NDIS calls MiniportRestart at IRQL = PASSIVE_LEVEL.
Arguments:
    MiniportAdapterContext
        Adapter context address
    MiniportRestartParameters
        Additional information about the restart operation
Return Value:
    NDIS_STATUS_PENDING
--*/
_Use_decl_annotations_
NDIS_STATUS MpRestart(NDIS_HANDLE MiniportAdapterContext, PNDIS_MINIPORT_RESTART_PARAMETERS MiniportRestartParameters)
{
    PMP_ADAPTER pAdapter = (PMP_ADAPTER)MiniportAdapterContext;

    DBG_SM_METHOD_BEG();
    UNREFERENCED_PARAMETER(MiniportRestartParameters);
    SmSetState(pAdapter, SM_STATE_RUNNING, MP_SM_NEXT_STATE_IMMEDIATELY, SM_CALLED_BY_NDIS);  // Switch to the RESTARTING state immediately
    DBG_SM_METHOD_END_WITH_STATUS(NDIS_STATUS_SUCCESS);
    return NDIS_STATUS_SUCCESS;
}

/*++
Routine Description:
    NDIS MiniportShutdownEx callback handler.
    MiniportShutdownEx runs at IRQL = PASSIVE_LEVEL  if ShutdownAction == NdisShutdownPowerOff
    MiniportShutdownEx runs at high IRQL             if ShutdownAction == NdisShutdownBugCheck
Arguments:
    MiniportAdapterContext
        Adapter context address
    ShutdownAction
        Additional information about the shutdown operation
Return Value:
    None
--*/
_Use_decl_annotations_
void MpShutdownEx(NDIS_HANDLE MiniportAdapterContext, NDIS_SHUTDOWN_ACTION ShutdownAction) {
    PMP_ADAPTER              pAdapter    = (PMP_ADAPTER)MiniportAdapterContext;

    DBG_SM_METHOD_BEG();
    if (ShutdownAction == NdisShutdownPowerOff) {
        EnetDeinit(pAdapter);
    } else {
        DBG_SM_PRINT_WARNING("Unexpected MpShutdownEx - status NdisShutdownBugCheck");
    }
    DBG_SM_METHOD_END();
}

/*
Routine Description:
    MiniportDevicePnPEvent handler
Argument:
    MiniportAdapterContext
        Our adapter context
    NetDevicePnPEventPtr
        The P&P event parameters
Return Value:
    None
*/
_Use_decl_annotations_
VOID MpDevicePnPEventNotify(NDIS_HANDLE MiniportAdapterContext, PNET_DEVICE_PNP_EVENT NetDevicePnPEvent)
{
    PMP_ADAPTER              pAdapter = (PMP_ADAPTER)MiniportAdapterContext;
    UNREFERENCED_PARAMETER(pAdapter);
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(NetDevicePnPEvent);
    DBG_SM_METHOD_BEG();
    #if DBG
    switch (NetDevicePnPEvent->DevicePnPEvent)  {
        case NdisDevicePnPEventQueryRemoved:
            DBG_SM_PRINT_INFO("PnP event: NdisDevicePnPEventQueryRemoved");
            break;
        case NdisDevicePnPEventRemoved:
            DBG_SM_PRINT_INFO("PnP event: NdisDevicePnPEventRemoved");
            break;
        case NdisDevicePnPEventSurpriseRemoved:
            DBG_SM_PRINT_INFO("PnP event: NdisDevicePnPEventSurpriseRemoved");
            break;
        case NdisDevicePnPEventQueryStopped:
            DBG_SM_PRINT_INFO("PnP event: NdisDevicePnPEventQueryStopped");
            break;
        case NdisDevicePnPEventStopped:
            DBG_SM_PRINT_INFO("PnP event: NdisDevicePnPEventStopped");
            break;
        case NdisDevicePnPEventPowerProfileChanged:
            DBG_SM_PRINT_INFO("PnP event: NdisDevicePnPEventPowerProfileChange");
            break;
        default:
            DBG_SM_PRINT_INFO("Unknown PnP event %x", NetDevicePnPEvent->DevicePnPEvent);
            break;
    }
    #endif
    DBG_SM_METHOD_END();
}

/*++
Routine Description:
    NDIS MiniportResetEx callback handler.
    NDIS calls MiniportResetEx at IRQL <= DISPATCH_LEVEL.
Arguments:
    AddressingReset
        To let NDIS know whether we need help from it with our reset
    MiniportAdapterContext
        Pointer to our adapter
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_RESET_IN_PROGRESS
    NDIS_STATUS_HARD_ERRORS
--*/
_Use_decl_annotations_
NDIS_STATUS MpResetEx(NDIS_HANDLE  MiniportAdapterContext, PBOOLEAN AddressingReset)
{
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER         pAdapter = (PMP_ADAPTER) MiniportAdapterContext;

    DBG_SM_METHOD_BEG();
    *AddressingReset = TRUE;                                                                       // Tell NDIS, to re-send the configuration parameters once reset is done.
    NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
    if (pAdapter->PendingNdisOperations.B.OP_RESET) {                                              // Reset pending?
        Status = NDIS_STATUS_RESET_IN_PROGRESS;                                                    // Yes, return pending status.
    } else {
        pAdapter->PendingNdisOperations.B.OP_RESET = 1;                                            // No, remember reset operation status.
        Status = NDIS_STATUS_PENDING;
    }
    NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
    if (Status == NDIS_STATUS_PENDING) {
        SmSetState(pAdapter, SM_STATE_RESET, MP_SM_NEXT_STATE_IMMEDIATELY, SM_CALLED_BY_NDIS);     // Switch to the RESET state immediately
    }
    DBG_SM_METHOD_END_WITH_STATUS(Status);
    return(Status);
}

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

// List of supported OIDs for the ENET driver
NDIS_OID ENETSupportedOids[] = {
    // General mandatory OIDs
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_INTERRUPT_MODERATION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_RCV_OK,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_STATISTICS,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_VENDOR_ID,
    OID_GEN_XMIT_OK,
    // Ethernet(802_3) mandatory OIDs
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,
    OID_802_3_XMIT_UNDERRUN,
    OID_PNP_SET_POWER,                             // Q: ""   S: "O"  RH
};

ULONG ENETSupportedOidsSize = sizeof(ENETSupportedOids);

/*++
Routine Description:
    Get the value for a statistics OID
Arguments:
    pAdapter    Pointer to our adapter
    Oid         Self-explanatory
    pCounter    Pointer to receive the value
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
--*/
_Use_decl_annotations_
NDIS_STATUS NICGetStatsCounters(PMP_ADAPTER pAdapter, NDIS_OID Oid, PULONG64 pCounter)
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    PNDIS_STATISTICS_INFO   pStatisticsInfo;
    volatile CSP_ENET_REGS *ENETRegBase = pAdapter->ENETRegBase;

    DBG_ENET_DEV_OIDS_METHOD_BEG();
    *pCounter = 0;
    switch (Oid) {
        case OID_GEN_XMIT_OK:
            *pCounter = pAdapter->TxdStatus.FramesXmitGood;
            break;

        case OID_GEN_RCV_OK:
            // Specifies the number of frames that are received without errors. However, the OID_GEN_STATISTICS does not include this information.
            *pCounter = pAdapter->RcvStatus.FrameRcvGood;
            break;

        case OID_802_3_XMIT_MAX_COLLISIONS:
            *pCounter = pAdapter->TxdStatus.FramesXmitCollisionErrors;
            break;

        case OID_802_3_RCV_OVERRUN:
            *pCounter = pAdapter->RcvStatus.FrameRcvOverrunErrors;
            break;

        case OID_802_3_XMIT_UNDERRUN:
            *pCounter = pAdapter->TxdStatus.FramesXmitUnderrunErrors;
            break;

        case OID_GEN_STATISTICS:
            #pragma prefast(disable:6386, "pStatisticInfo")
            pStatisticsInfo = (PNDIS_STATISTICS_INFO)pCounter;
            NdisZeroMemory(pStatisticsInfo, sizeof(NDIS_STATISTICS_INFO));
            pStatisticsInfo->Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
            pStatisticsInfo->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
            pStatisticsInfo->Header.Size = sizeof(NDIS_STATISTICS_INFO);
            pStatisticsInfo->SupportedStatistics = NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS |
                NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR |
                NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV |
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV |
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_RCV |
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_RCV |
                NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT |
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT |
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_XMIT |
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_XMIT |
                NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR |
                NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS;
            ENETRegBase->MIBC.U = ENET_MIBC_MIB_DIS_MASK;                   // Disable statistic counters
            while (!(ENETRegBase->MIBC.U & ENET_MIBC_MIB_IDLE_MASK));       // Wait for idle
            /* ifInDiscards - "The number of inbound packets which were chosen to be discarded even though no errors had been detected to prevent their being deliverable to a higher-layer protocol."
            OID_GEN_RCV_DISCARDS = OID_GEN_RCV_ERROR + OID_GEN_RCV_NO_BUFFER */
            pAdapter->StatisticsAcc.ifInDiscards += ENETRegBase->RMON_R_CRC_ALIGN + ENETRegBase->RMON_R_UNDERSIZE + ENETRegBase->RMON_R_OVERSIZE + ENETRegBase->IEEE_R_MACERR;
            /* ifInErrors - "For packet-oriented interfaces, the number of inbound packets that contained errors preventing them from being deliverable to a higher-layer protocol."
            OID_GEN_RCV_ERROR */
            pAdapter->StatisticsAcc.ifInErrors += ENETRegBase->RMON_R_CRC_ALIGN + ENETRegBase->RMON_R_UNDERSIZE + ENETRegBase->RMON_R_OVERSIZE;
            /* ifHCInOctets - "The total number of octets received on the interface, including framing characters."
            OID_GEN_BYTES_RCV = OID_GEN_DIRECTED_BYTES_RCV + OID_GEN_MULTICAST_BYTES_RCV + OID_GEN_BROADCAST_BYTES_RCV */
            pAdapter->StatisticsAcc.ifHCInOctets += ENETRegBase->IEEE_R_OCTETS_OK;
            /* ifHCInUcastPkts - "The number of packets, delivered by this sub-layer to a higher (sub-)layer, which were not addressed to a multicast or broadcast address at this sub-layer."
            OID_GEN_DIRECTED_FRAMES_RCV */
            pAdapter->StatisticsAcc.ifHCInUcastPkts += ENETRegBase->RMON_R_PACKETS - ENETRegBase->RMON_R_MC_PKT - ENETRegBase->RMON_R_BC_PKT;
            /* ifHCInMulticastPkts - "The number of packets, delivered by this sub-layer to a higher (sub-)layer, which were addressed to a multicast address at this sub-layer.
            For a MAC layer protocol, this includes both Group and Functional addresses."
            OID_GEN_MULTICAST_FRAMES_RCV */
            pAdapter->StatisticsAcc.ifHCInMulticastPkts += ENETRegBase->RMON_R_MC_PKT;
            /* ifHCInBroadcastPkts - "The number of packets, delivered by this sub-layer to a higher (sub-)layer, which were addressed to a broadcast address at this sub-layer"
            OID_GEN_BROADCAST_FRAMES_RCV */
            pAdapter->StatisticsAcc.ifHCInBroadcastPkts += ENETRegBase->RMON_R_BC_PKT;
            /* ifHCOutOctets - "The total number of octets transmitted out of the interface, including framing characters"
            OID_GEN_BYTES_XMIT = OID_GEN_DIRECTED_BYTES_XMIT + OID_GEN_MULTICAST_BYTES_XMIT + OID_GEN_BROADCAST_BYTES_XMIT */
            pAdapter->StatisticsAcc.ifHCOutOctets += ENETRegBase->IEEE_T_OCTETS_OK;
            /* ifHCOutUcastPkts - "The total number of packets that higher-level protocols requested be transmitted, and which were not addressed to a multicast or broadcast address at this sub-layer,
            including those that were discarded or not sent."
            OID_GEN_DIRECTED_FRAMES_XMIT */
            pAdapter->StatisticsAcc.ifHCOutUcastPkts += ENETRegBase->RMON_T_PACKETS - ENETRegBase->RMON_T_MC_PKT - ENETRegBase->RMON_T_BC_PKT;
            /* ifHCOutMulticastPkts - "The total number of packets that higher-level protocols requested be transmitted, and which were addressed to a multicast address at this sub-layer,
            including those that were discarded or not sent. For a MAC layer protocol, this includes both Group and Functional addresses."
            OID_GEN_MULTICAST_FRAMES_XMIT */
            pAdapter->StatisticsAcc.ifHCOutMulticastPkts += ENETRegBase->RMON_T_MC_PKT;
            /* ifHCOutBroadcastPkts - "The total number of packets that higher-level protocols requested be transmitted, and which were addressed to a broadcast address at this sub-layer,
            including those that were discarded or not sent."
            OID_GEN_BROADCAST_FRAMES_XMIT */
            pAdapter->StatisticsAcc.ifHCOutBroadcastPkts += ENETRegBase->RMON_T_BC_PKT;
            /* ifOutErrors - "For packet-oriented interfaces, the number of outbound packets that could not be transmitted because of errors."
            OID_GEN_XMIT_ERROR */
            pAdapter->StatisticsAcc.ifOutErrors += ENETRegBase->IEEE_T_EXCOL + ENETRegBase->IEEE_T_LCOL;
            /* ifOutDiscards - "The number of outbound packets which were chosen to be discarded even though no errors had been detected to prevent their being transmitted.
            One possible reason for discarding such a packet could be to free up buffer space."
            OID_GEN_XMIT_DISCARDS */
            pAdapter->StatisticsAcc.ifOutDiscards += ENETRegBase->IEEE_T_EXCOL;
            ENETRegBase->MIBC.U = (ENET_MIBC_MIB_DIS_MASK | ENET_MIBC_MIB_CLEAR_MASK);     // Clear statistic counters
            NdisStallExecution(2);
            ENETRegBase->MIBC.U = 0;                                                           // Enable statistic counters
            // Get accumulated values
            pStatisticsInfo->ifInDiscards = pAdapter->StatisticsAcc.ifInDiscards;
            pStatisticsInfo->ifInErrors = pAdapter->StatisticsAcc.ifInErrors;
            pStatisticsInfo->ifHCInOctets = pAdapter->StatisticsAcc.ifHCInOctets;
            pStatisticsInfo->ifHCInUcastPkts = pAdapter->StatisticsAcc.ifHCInUcastPkts;
            pStatisticsInfo->ifHCInMulticastPkts = pAdapter->StatisticsAcc.ifHCInMulticastPkts;
            pStatisticsInfo->ifHCInBroadcastPkts = pAdapter->StatisticsAcc.ifHCInBroadcastPkts;
            pStatisticsInfo->ifHCOutOctets = pAdapter->StatisticsAcc.ifHCOutOctets;
            pStatisticsInfo->ifHCOutUcastPkts = pAdapter->StatisticsAcc.ifHCOutUcastPkts;
            pStatisticsInfo->ifHCOutMulticastPkts = pAdapter->StatisticsAcc.ifHCOutMulticastPkts;
            pStatisticsInfo->ifHCOutBroadcastPkts = pAdapter->StatisticsAcc.ifHCOutBroadcastPkts;
            pStatisticsInfo->ifOutErrors = pAdapter->StatisticsAcc.ifOutErrors;
            pStatisticsInfo->ifOutDiscards = pAdapter->StatisticsAcc.ifOutDiscards;
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }
    DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS(Status);
    return(Status);
}

/*++
Routine Description:
    This routine will set up the adapter so that it accepts packets that match the specified packet filter.
    The only filter bits that can truly be toggled is multicast
Arguments:
    pAdapter         Pointer to our adapter
    PacketFilter    The new packet filter
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
--*/
_Use_decl_annotations_
NDIS_STATUS NICSetPacketFilter(PMP_ADAPTER pAdapter,  ULONG PacketFilter)
{
    NDIS_STATUS     StatusToReturn = NDIS_STATUS_SUCCESS;
    UINT            i;
    ULONG Filter;

    DBG_ENET_DEV_OIDS_METHOD_BEG_WITH_PARAMS("PacketFilter=0x%08X", PacketFilter);
    NdisMoveMemory((PUCHAR)&Filter, (PUCHAR)&PacketFilter, sizeof(ULONG));

    pAdapter->PacketFilter = Filter;

    if (Filter & NDIS_PACKET_TYPE_PROMISCUOUS )  {
        DBG_ENET_DEV_OIDS_PRINT_INFO("Promiscuous Filter being set..");
        pAdapter->ENETRegBase->RCR.U |= ENET_RCR_PROM_MASK;
    } else {
        DBG_ENET_DEV_OIDS_PRINT_INFO("Promiscuous Filter being cleared..");
        pAdapter->ENETRegBase->RCR.U &= ~ENET_RCR_PROM_MASK;
    }

    if (Filter & NDIS_PACKET_TYPE_MULTICAST ) {
        // clear all the content in hardware Hash Table
        ClearAllMultiCast(pAdapter);
        for (i = 0; i < pAdapter->MCAddressCount; i++)  {
            DBG_ENET_DEV_OIDS_PRINT_TRACE("MC(%d) = %02x-%02x-%02x-%02x-%02x-%02x", i, pAdapter->MCList[i][0], pAdapter->MCList[i][1], pAdapter->MCList[i][2], pAdapter->MCList[i][3], pAdapter->MCList[i][4], pAdapter->MCList[i][5]);
            AddMultiCast(pAdapter, &(pAdapter->MCList[i][0]));
        }
    } else {
        ClearAllMultiCast(pAdapter);
    }

    if (Filter & NDIS_PACKET_TYPE_DIRECTED)  {
        SetUnicast(pAdapter);
    }
    DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS(StatusToReturn);
    return(StatusToReturn);
}


/*++
Routine Description:
    This routine will set up the adapter for a specified multicast address list
Arguments:
    pAdapter         Pointer to our adapter
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_ACCEPTED
--*/
_Use_decl_annotations_
NDIS_STATUS NICSetMulticastList(PMP_ADAPTER pAdapter)
{
    UINT            i;

    DBG_ENET_DEV_OIDS_METHOD_BEG();
    for (i = 0; i < pAdapter->MCAddressCount; i++)  {
        DBG_ENET_DEV_OIDS_PRINT_TRACE("MC(%d) = %02x-%02x-%02x-%02x-%02x-%02x", i, pAdapter->MCList[i][0], pAdapter->MCList[i][1], pAdapter->MCList[i][2], pAdapter->MCList[i][3], pAdapter->MCList[i][4], pAdapter->MCList[i][5]);
        AddMultiCast(pAdapter, &(pAdapter->MCList[i][0]));
    }
    DBG_ENET_DEV_OIDS_METHOD_END();
    return(NDIS_STATUS_SUCCESS);
}

void MpIndicateLinkStatus(_In_ PMP_ADAPTER pAdapter);
/*++
Routine Description:
    This routine is called when the adapter receives a SetPower request.
     If requested to go to a low power state, the routine starts a 'Pause' process, which have similar behavior characteristics.
Arguments:
    pAdapter        Pointer to the adapter structure
    PowerState      NewPowerState
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_HARDWARE_ERRORS
--*/
_Use_decl_annotations_
NDIS_STATUS MpSetPower(PMP_ADAPTER pAdapter, NDIS_DEVICE_POWER_STATE NewPowerState)
{
    NDIS_STATUS       Status = NDIS_STATUS_SUCCESS;
    LONG              SendWaitCounter = 100;

    DBG_ENET_DEV_OIDS_METHOD_BEG_WITH_PARAMS("New power state: %s", Dbg_GetNdisPowerStateName(NewPowerState));

    ASSERT(NewPowerState >= NdisDeviceStateD0 && NewPowerState <= NdisDeviceStateD3);
    if ((NewPowerState < NdisDeviceStateD0 || NewPowerState > NdisDeviceStateD3)) {
        Status = NDIS_STATUS_INVALID_DATA;
        return Status;
    }
    NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
    do {
        if (pAdapter->CurrentPowerState == NewPowerState) {
            break;
        }
        pAdapter->NewPowerState = NewPowerState;
        if (NewPowerState == NdisDeviceStateD0) {
            DBG_ENET_DEV_PRINT_INFO("SET_POWER_OID D0");
            pAdapter->CurrentPowerState = NewPowerState;
            if (pAdapter->RestartEnetAfterResume) {
                pAdapter->RestartEnetAfterResume = FALSE;
                DBG_ENET_DEV_PRINT_INFO("Enet device was running before suspend. Start Enet now.");
                NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
                EnetStart(pAdapter);
                NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
            } else {
                DBG_ENET_DEV_PRINT_INFO("Enet device was not running before suspend. Do not start Enet now.");
            }
        } else {
            DBG_ENET_DEV_PRINT_INFO("SET_POWER_OID D3");
            if (pAdapter->EnetStarted) {                                                        // Is ENET running?
                DBG_ENET_DEV_PRINT_INFO("Enet device is running. Trying to stop Enet...");
                pAdapter->RestartEnetAfterResume = TRUE;
                NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);                                   
                EnetStop(pAdapter, NDIS_STATUS_LOW_POWER_STATE);                                // Stop the adapter and remember new NDIS status.
                (void)MpTxCancelAll(pAdapter);                                                  // Cancel all queued send requests.
                while (pAdapter->Tx_PendingNBs != 0) {                                          // Wait up to 100 ms untill all TX NBL are return to NDIS
                    DBG_ENET_DEV_PRINT_INFO("Waiting for pAdapter->Tx_PendingNBs == 0");
                    if (SendWaitCounter--) {
                        NdisMSleep(1000);                                                       // Wait 1 ms
                        (void)MpTxCancelAll(pAdapter);
                    } else {
                        DBG_ENET_DEV_PRINT_ERROR("Waiting for pAdapter->Tx_PendingNBs == 0 FAILED");
                        ASSERT(0);
                    }
                }
                NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
            } else {
                pAdapter->RestartEnetAfterResume = FALSE;
                DBG_ENET_DEV_PRINT_INFO("Enet device is stopped. No action needed.");
            }
            pAdapter->CurrentPowerState = NdisDeviceStateD3;
            pAdapter->LinkSpeed = 0;
            pAdapter->DuplexMode = MediaDuplexStateUnknown;
            pAdapter->MediaConnectState = MediaConnectStateUnknown;
            MpIndicateLinkStatus(pAdapter);
        }
    } while (0);
    NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
    DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS(Status);
    return Status; 
}

/*++
Routine Description:
    NDIS calls a miniport driver's MiniportOidRequest function to handle an OID request to query or set information in the driver.
    If MiniportOidRequest returns NDIS_STATUS_PENDING, NDIS will not call MiniportOidRequest with another request until the pending request is completed.
    NDIS calls MiniportOidRequest at IRQL == PASSIVE_LEVEL.
Arguments:
    MiniportAdapterContext
        Pointer to the adapter structure
    NdisRequest
        The OID request to handle
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_BUFFER_TOO_SHORT
--*/
_Use_decl_annotations_
NDIS_STATUS MpQueryInformation(NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST NdisRequest)
{
    NDIS_STATUS                           Status                  = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER                           pAdapter                = (PMP_ADAPTER)MiniportAdapterContext;
    NDIS_OID                              Oid                     = NdisRequest->DATA.QUERY_INFORMATION.Oid;
    PVOID                                 InformationBuffer       = NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
    ULONG                                 InformationBufferLength = NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
    ULONG                                 BytesWritten            = 0;
    ULONG                                 BytesNeeded             = 0;
    UCHAR                                 VendorDesc[]            = NIC_VENDOR_DESC;
    NDIS_INTERRUPT_MODERATION_PARAMETERS  ndisIntModParams;
    ULONG                                 ulInfo                  = 0;
    ULONG64                               ul64Info                = 0;
    PVOID                                 pInfo                   = (PVOID) &ulInfo;
    ULONG                                 ulInfoLen               = sizeof(ulInfo);
    ULONG                                 ulBytesAvailable        = ulInfoLen;
    BOOLEAN                               DoCopy                  = TRUE;

    DBG_ENET_DEV_OIDS_METHOD_BEG_WITH_PARAMS("%s",Dbg_GetNdisOidName(Oid));
    switch (Oid)  {    // Process different type of requests

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            // Specifies the maximum total packet length, in bytes, the NIC supports. This specification includes the header. (Typically 1514)
            // Tx: Specifies the largest packet that a protocol driver can pass to the NdisSendNetBufferLists function.
            // Rx: Specifies the largest packet that a miniport driver can indicate to NDIS.
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            // Specifies the minimum number of bytes that a single net packet occupies in the transmit buffer space of the NIC.
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            // Specifies the amount of storage, in bytes, that a single packet occupies in the receive buffer space of the NIC.
            ulInfo = (ULONG) ETHER_FRAME_MAX_LENGTH - ETHER_FRAME_CRC_LENGTH;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            // Specifies the amount of memory, in bytes, on the NIC that is available for buffering transmit data.
            ulInfo = ETHER_FRAME_MAX_LENGTH * pAdapter->Tx_DmaBDT_ItemCount;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            // Specifies the amount of memory on the NIC that is available for buffering receive data.
            ulInfo = ETHER_FRAME_MAX_LENGTH * pAdapter->Rx_DmaBDT_ItemCount;
            break;

        case OID_GEN_VENDOR_ID:
            // Specifies a three-byte IEEE-registered vendor code, followed by a single byte that the vendor assigns to identify a particular NIC.
            NdisMoveMemory(&ulInfo, pAdapter->PermanentAddress, 4);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            // Points to a null-terminated string describing the NIC.
            pInfo = VendorDesc;
            ulBytesAvailable = ulInfoLen = sizeof(VendorDesc);
            break;

        case OID_GEN_VENDOR_DRIVER_VERSION:
            // Specifies the vendor-assigned version number of the miniport driver. The low-order half of the return value specifies the minor version; the high-order half specifies the major version.
            ulInfo = NIC_VENDOR_DRIVER_VERSION;
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            // The address of the NIC encoded in the hardware.
            pInfo = pAdapter->PermanentAddress;
            ulBytesAvailable = ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            // The address the NIC is currently using.
            pInfo = pAdapter->CurrentAddress;
            ulBytesAvailable = ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            // Request to query or set the maximum number of 6-byte multicast addresses that the miniport pAdapter's multicast address list can hold.
            ulInfo = NIC_MAX_MCAST_LIST;
            break;

        case OID_GEN_XMIT_OK:
            // Specifies the number of frames that are transmitted without errors.
        case OID_GEN_RCV_OK:
            // Specifies the number of frames that are received without errors. However, the OID_GEN_STATISTICS does not include this information.
        case OID_802_3_XMIT_MAX_COLLISIONS:       // The number of frames not transmitted due to excessive collisions.
        case OID_802_3_RCV_OVERRUN:               // The number of frames not received due to overrun errors on the NIC.
        case OID_802_3_XMIT_UNDERRUN:             // The number of frames not transmitted due to underrun errors on the NIC.
            Status = NICGetStatsCounters(pAdapter, Oid, &ul64Info);
            ulBytesAvailable = ulInfoLen = sizeof(ul64Info);
            if (Status == NDIS_STATUS_SUCCESS) {
                if (InformationBufferLength < sizeof(ULONG)) {
                    Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                    BytesNeeded = ulBytesAvailable;
                    break;
                }
                ulInfoLen = MIN(InformationBufferLength, ulBytesAvailable);
                pInfo = &ul64Info;
            }
            break;

        case OID_GEN_STATISTICS:
            // NDIS and overlying drivers use the OID_GEN_STATISTICS OID to obtain statistics of an pAdapter or a miniport driver.
            DoCopy = FALSE;
            ulBytesAvailable = ulInfoLen = sizeof(NDIS_STATISTICS_INFO);
            if (InformationBufferLength < ulInfoLen) {
                break;
            }
            Status = NICGetStatsCounters(pAdapter, Oid, (PULONG64)InformationBuffer);
            break;

        case OID_GEN_INTERRUPT_MODERATION:
            // Miniport driver does not support interrupt moderation, the driver must specify NdisInterruptModerationNotSupported
            // in the InterruptModeration member of the NDIS_INTERRUPT_MODERATION_PARAMETERS structure.
            NdisZeroMemory(&ndisIntModParams, sizeof(ndisIntModParams));
            ndisIntModParams.Header.Type         = NDIS_OBJECT_TYPE_DEFAULT;
            ndisIntModParams.Header.Revision     = NDIS_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
            ndisIntModParams.Header.Size         = NDIS_SIZEOF_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
            ndisIntModParams.Flags               = 0;
            ndisIntModParams.InterruptModeration = NdisInterruptModerationNotSupported;
            pInfo = &ndisIntModParams;
            ulBytesAvailable = sizeof(ndisIntModParams);
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            DBG_ENET_DEV_OIDS_PRINT_INFO("%s not supported", Dbg_GetNdisOidName(Oid));
            break;
    }
    if (Status == NDIS_STATUS_SUCCESS) {
        BytesNeeded = ulBytesAvailable;
        if (ulInfoLen <= InformationBufferLength) {  // Copy result into InformationBuffer if there is space enough
            BytesWritten = ulInfoLen;
            if (ulInfoLen && DoCopy)  {
                NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
            }
        } else {
            BytesNeeded = ulInfoLen;                // Not enough space in provided buffer
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
    }
    NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = BytesWritten;
    NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = BytesNeeded;
    DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS(Status);
    return(Status);
}

/*++
Routine Description:
    NDIS calls a miniport driver's MiniportOidRequest function to handle an OID request to query or set information in the driver.
    If MiniportOidRequest returns NDIS_STATUS_PENDING, NDIS will not call MiniportOidRequest with another request until the pending request is completed.
    NDIS calls MiniportOidRequest at IRQL == PASSIVE_LEVEL.
Arguments:
    MiniportAdapterContext
        Pointer to the adapter structure
    NdisRequest
        The OID request to handle
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_BUFFER_TOO_SHORT
--*/
_Use_decl_annotations_
NDIS_STATUS MpSetInformation(NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST NdisRequest)
{
    NDIS_STATUS                 Status                  = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER                 pAdapter                = (PMP_ADAPTER) MiniportAdapterContext;
    NDIS_OID                    Oid                     = NdisRequest->DATA.SET_INFORMATION.Oid;
    PVOID                       InformationBuffer       = NdisRequest->DATA.SET_INFORMATION.InformationBuffer;;
    ULONG                       InformationBufferLength = NdisRequest->DATA.SET_INFORMATION.InformationBufferLength;;
    ULONG                       BytesRead               = 0;
    ULONG                       BytesNeeded             = 0;
    ULONG                       PacketFilter;
    ULONG                       MCAddressCount;

    DBG_ENET_DEV_OIDS_METHOD_BEG_WITH_PARAMS("%s",Dbg_GetNdisOidName(Oid));
    switch(Oid) {

        case OID_802_3_MULTICAST_LIST:
            // Request to replace the current multicast address list on a miniport pAdapter. If an address is present in the list, that address is enabled to receive multicast packets.
            if (InformationBufferLength % ETH_LENGTH_OF_ADDRESS != 0)  {  // Verify the length
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            MCAddressCount = InformationBufferLength / ETH_LENGTH_OF_ADDRESS;  // Save the number of MC list size
            if (MCAddressCount > NIC_MAX_MCAST_LIST) {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            pAdapter->MCAddressCount = MCAddressCount;
            NdisMoveMemory(pAdapter->MCList, InformationBuffer, MCAddressCount * ETH_LENGTH_OF_ADDRESS);   // Save the MC list
            BytesRead = InformationBufferLength;
            NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
            ClearAllMultiCast(pAdapter);                  // Clear all the content in hardware Hash Table
            Status = NICSetMulticastList(pAdapter);
            NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            if (InformationBufferLength != sizeof(ULONG)) {  // Verify the Length
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            BytesRead = InformationBufferLength;
            NdisMoveMemory(&PacketFilter, InformationBuffer, sizeof(ULONG));
            if (PacketFilter & ~NIC_SUPPORTED_FILTERS) {     // Any bits not supported?
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }
            if (PacketFilter == pAdapter->PacketFilter) {     // Any filtering changes?
                break;
            }
            NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
            Status = NICSetPacketFilter(pAdapter,PacketFilter);
            NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
            if (Status == NDIS_STATUS_SUCCESS) {
                pAdapter->PacketFilter = PacketFilter;
            }
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            if (InformationBufferLength < sizeof(ULONG)) {  // Verify the Length
                BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            if (*(UNALIGNED PULONG)InformationBuffer > ETHER_FRAME_MAX_PAYLOAD_LENGTH) {
                Status = NDIS_STATUS_INVALID_DATA;
                break;
            }
            NdisMoveMemory(&pAdapter->ulLookAhead, InformationBuffer, sizeof(ULONG));
            BytesRead = sizeof(ULONG);
            Status = NDIS_STATUS_SUCCESS;
            break;

      case OID_PNP_SET_POWER:
          if (InformationBufferLength != sizeof(NDIS_DEVICE_POWER_STATE)) {
              Status = NDIS_STATUS_INVALID_LENGTH;
              break;
          }
          Status = MpSetPower(pAdapter, *(PNDIS_DEVICE_POWER_STATE UNALIGNED)InformationBuffer);
          if ((Status == NDIS_STATUS_SUCCESS) || (Status == NDIS_STATUS_PENDING)) {
              BytesRead = sizeof(NDIS_DEVICE_POWER_STATE);
          }
          break;
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            DBG_ENET_DEV_OIDS_PRINT_INFO("%s not supported", Dbg_GetNdisOidName(Oid));
            break;

    }
    if (Status == NDIS_STATUS_SUCCESS) {
        NdisRequest->DATA.SET_INFORMATION.BytesRead = BytesRead;
    }
    NdisRequest->DATA.SET_INFORMATION.BytesNeeded = BytesNeeded;
    DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS(Status);
    return(Status);
}

/*++
Routine Description:
    NDIS calls a miniport driver's MiniportOidRequest function to handle an OID request to query or set information in the driver.
    If MiniportOidRequest returns NDIS_STATUS_PENDING, NDIS will not call MiniportOidRequest with another request until the pending request is completed.
    NDIS calls MiniportOidRequest at IRQL == PASSIVE_LEVEL.
Arguments:
    MiniportAdapterContext
        Pointer to the adapter structure
    NdisRequest
        The OID request to handle
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_BUFFER_TOO_SHORT
--*/
_Use_decl_annotations_
NDIS_STATUS MpOidRequest(NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST  NdisRequest)
{
    PMP_ADAPTER             pAdapter = (PMP_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS             Status;

    switch (NdisRequest->RequestType) {
        case NdisRequestSetInformation:
            Status = MpSetInformation(pAdapter, NdisRequest);
            break;
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
            Status = MpQueryInformation(pAdapter, NdisRequest);
            break;
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            DBG_ENET_DEV_OIDS_PRINT_INFO("OID Request type %s not supported", Dbg_GetNdisOidRequestTypeName(NdisRequest->RequestType));
            break;
    }
//    Dbg_PrintOidDescription(NdisRequest, Status);
    return Status;
}

/*++
Routine Description:
    NDIS MiniportCancelOidRequest callback handler.
    NDIS calls MiniportCancelOidRequest at IRQL <= DISPATCH_LEVEL.
Arguments:
    MiniportAdapterContext
        Adapter context address
    RestartParameters
        A cancellation identifier for the request. This identifier specifies the NDIS_OID_REQUEST structures that are being canceled.
Return Value:
    None
--*/
_Use_decl_annotations_
VOID MpCancelOidRequest(NDIS_HANDLE MiniportAdapterContext, PVOID RequestId)
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(RequestId);
}


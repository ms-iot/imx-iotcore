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

#if DBG

/*++
Routine Description:
    .
Argument:
    NdisRequest     OID
    Status          NDIS status
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_PrintOidDescription(PMP_ADAPTER pAdapter, PNDIS_OID_REQUEST NdisRequest, NTSTATUS Status) {
    NDIS_OID  Oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
    PCHAR     oidType = NULL;
    PCHAR     oidName = NULL;
    CHAR      Buffer[256];

    UNREFERENCED_PARAMETER(pAdapter);
    UNREFERENCED_PARAMETER(Status);
    if ((Oid == OID_GEN_STATISTICS) || (Oid == OID_IP6_OFFLOAD_STATS) || (Oid == OID_IP4_OFFLOAD_STATS) || (Oid == OID_GEN_NETWORK_LAYER_ADDRESSES)) {
        return;
    }
    switch (NdisRequest->RequestType) {
        case NdisRequestQueryInformation: oidType = "GET_INFO"; break;
        case NdisRequestSetInformation:   oidType = "SET_INFO"; break;
        case NdisRequestQueryStatistics:  oidType = "GET_STAT"; break;
        case NdisRequestMethod:           oidType = "REQ_MTHD"; break;
        default:                          oidType = "UNKNOWN "; break;
    }
    #undef MAKECASE
    #define MAKECASE(oidName, oidx) case oidx: oidName = #oidx; break;
    switch (Oid) {
        /* Operational OIDs */
        MAKECASE(oidName, OID_GEN_SUPPORTED_LIST)
        MAKECASE(oidName, OID_GEN_HARDWARE_STATUS)
        MAKECASE(oidName, OID_GEN_MEDIA_SUPPORTED)
        MAKECASE(oidName, OID_GEN_MEDIA_IN_USE)
        MAKECASE(oidName, OID_GEN_MAXIMUM_LOOKAHEAD)
        MAKECASE(oidName, OID_GEN_MAXIMUM_FRAME_SIZE)
        MAKECASE(oidName, OID_GEN_LINK_SPEED)
        MAKECASE(oidName, OID_GEN_TRANSMIT_BUFFER_SPACE)
        MAKECASE(oidName, OID_GEN_RECEIVE_BUFFER_SPACE)
        MAKECASE(oidName, OID_GEN_TRANSMIT_BLOCK_SIZE)
        MAKECASE(oidName, OID_GEN_RECEIVE_BLOCK_SIZE)
        MAKECASE(oidName, OID_GEN_VENDOR_ID)
        MAKECASE(oidName, OID_GEN_VENDOR_DESCRIPTION)
        MAKECASE(oidName, OID_GEN_VENDOR_DRIVER_VERSION)
        MAKECASE(oidName, OID_GEN_CURRENT_PACKET_FILTER)
        MAKECASE(oidName, OID_GEN_CURRENT_LOOKAHEAD)
        MAKECASE(oidName, OID_GEN_DRIVER_VERSION)
        MAKECASE(oidName, OID_GEN_MAXIMUM_TOTAL_SIZE)
        MAKECASE(oidName, OID_GEN_PROTOCOL_OPTIONS)
        MAKECASE(oidName, OID_GEN_MAC_OPTIONS)
        MAKECASE(oidName, OID_GEN_MEDIA_CONNECT_STATUS)
        MAKECASE(oidName, OID_GEN_MAXIMUM_SEND_PACKETS)
        MAKECASE(oidName, OID_GEN_SUPPORTED_GUIDS)
        MAKECASE(oidName, OID_GEN_NETWORK_LAYER_ADDRESSES)
        MAKECASE(oidName, OID_GEN_TRANSPORT_HEADER_OFFSET)
        MAKECASE(oidName, OID_GEN_MEDIA_CAPABILITIES)
        MAKECASE(oidName, OID_GEN_PHYSICAL_MEDIUM)
        MAKECASE(oidName, OID_GEN_MACHINE_NAME)
        MAKECASE(oidName, OID_GEN_VLAN_ID)
        MAKECASE(oidName, OID_GEN_RNDIS_CONFIG_PARAMETER)

        /* Operational OIDs for NDIS 6.0 */
        MAKECASE(oidName, OID_GEN_MAX_LINK_SPEED)
        MAKECASE(oidName, OID_GEN_LINK_STATE)
        MAKECASE(oidName, OID_GEN_LINK_PARAMETERS)
        MAKECASE(oidName, OID_GEN_MINIPORT_RESTART_ATTRIBUTES)
        MAKECASE(oidName, OID_GEN_ENUMERATE_PORTS)
        MAKECASE(oidName, OID_GEN_PORT_STATE)
        MAKECASE(oidName, OID_GEN_PORT_AUTHENTICATION_PARAMETERS)
        MAKECASE(oidName, OID_GEN_INTERRUPT_MODERATION)
        MAKECASE(oidName, OID_GEN_PHYSICAL_MEDIUM_EX)

        /* Statistical OIDs */
        MAKECASE(oidName, OID_GEN_XMIT_OK)
        MAKECASE(oidName, OID_GEN_RCV_OK)
        MAKECASE(oidName, OID_GEN_XMIT_ERROR)
        MAKECASE(oidName, OID_GEN_RCV_ERROR)
        MAKECASE(oidName, OID_GEN_RCV_NO_BUFFER)
        MAKECASE(oidName, OID_GEN_DIRECTED_BYTES_XMIT)
        MAKECASE(oidName, OID_GEN_DIRECTED_FRAMES_XMIT)
        MAKECASE(oidName, OID_GEN_MULTICAST_BYTES_XMIT)
        MAKECASE(oidName, OID_GEN_MULTICAST_FRAMES_XMIT)
        MAKECASE(oidName, OID_GEN_BROADCAST_BYTES_XMIT)
        MAKECASE(oidName, OID_GEN_BROADCAST_FRAMES_XMIT)
        MAKECASE(oidName, OID_GEN_DIRECTED_BYTES_RCV)
        MAKECASE(oidName, OID_GEN_DIRECTED_FRAMES_RCV)
        MAKECASE(oidName, OID_GEN_MULTICAST_BYTES_RCV)
        MAKECASE(oidName, OID_GEN_MULTICAST_FRAMES_RCV)
        MAKECASE(oidName, OID_GEN_BROADCAST_BYTES_RCV)
        MAKECASE(oidName, OID_GEN_BROADCAST_FRAMES_RCV)
        MAKECASE(oidName, OID_GEN_RCV_CRC_ERROR)
        MAKECASE(oidName, OID_GEN_TRANSMIT_QUEUE_LENGTH)

        /* Statistical OIDs for NDIS 6.0 */
        MAKECASE(oidName, OID_GEN_STATISTICS)
        MAKECASE(oidName, OID_GEN_BYTES_RCV)
        MAKECASE(oidName, OID_GEN_BYTES_XMIT)
        MAKECASE(oidName, OID_GEN_RCV_DISCARDS)
        MAKECASE(oidName, OID_GEN_XMIT_DISCARDS)

        /* Misc OIDs */
        MAKECASE(oidName, OID_GEN_GET_TIME_CAPS)
        MAKECASE(oidName, OID_GEN_GET_NETCARD_TIME)
        MAKECASE(oidName, OID_GEN_NETCARD_LOAD)
        MAKECASE(oidName, OID_GEN_DEVICE_PROFILE)
        MAKECASE(oidName, OID_GEN_INIT_TIME_MS)
        MAKECASE(oidName, OID_GEN_RESET_COUNTS)
        MAKECASE(oidName, OID_GEN_MEDIA_SENSE_COUNTS)

        /* PnP power management operational OIDs */
        MAKECASE(oidName, OID_PNP_CAPABILITIES)
        MAKECASE(oidName, OID_PNP_SET_POWER)
        MAKECASE(oidName, OID_PNP_QUERY_POWER)
        MAKECASE(oidName, OID_PNP_ADD_WAKE_UP_PATTERN)
        MAKECASE(oidName, OID_PNP_REMOVE_WAKE_UP_PATTERN)
        MAKECASE(oidName, OID_PNP_ENABLE_WAKE_UP)
        MAKECASE(oidName, OID_PNP_WAKE_UP_PATTERN_LIST)

        /* PnP power management statistical OIDs */
        MAKECASE(oidName, OID_PNP_WAKE_UP_ERROR)
        MAKECASE(oidName, OID_PNP_WAKE_UP_OK)

        /* Ethernet operational OIDs */
        MAKECASE(oidName, OID_802_3_PERMANENT_ADDRESS)
        MAKECASE(oidName, OID_802_3_CURRENT_ADDRESS)
        MAKECASE(oidName, OID_802_3_MULTICAST_LIST)
        MAKECASE(oidName, OID_802_3_MAXIMUM_LIST_SIZE)
        MAKECASE(oidName, OID_802_3_MAC_OPTIONS)

        /* Ethernet operational OIDs for NDIS 6.0 */
        MAKECASE(oidName, OID_802_3_ADD_MULTICAST_ADDRESS)
        MAKECASE(oidName, OID_802_3_DELETE_MULTICAST_ADDRESS)

        /* Ethernet statistical OIDs */
        MAKECASE(oidName, OID_802_3_RCV_ERROR_ALIGNMENT)
        MAKECASE(oidName, OID_802_3_XMIT_ONE_COLLISION)
        MAKECASE(oidName, OID_802_3_XMIT_MORE_COLLISIONS)
        MAKECASE(oidName, OID_802_3_XMIT_DEFERRED)
        MAKECASE(oidName, OID_802_3_XMIT_MAX_COLLISIONS)
        MAKECASE(oidName, OID_802_3_RCV_OVERRUN)
        MAKECASE(oidName, OID_802_3_XMIT_UNDERRUN)
        MAKECASE(oidName, OID_802_3_XMIT_HEARTBEAT_FAILURE)
        MAKECASE(oidName, OID_802_3_XMIT_TIMES_CRS_LOST)
        MAKECASE(oidName, OID_802_3_XMIT_LATE_COLLISIONS)

        /*  TCP/IP OIDs */
        MAKECASE(oidName, OID_TCP_TASK_OFFLOAD)
        MAKECASE(oidName, OID_TCP_TASK_IPSEC_ADD_SA)
        MAKECASE(oidName, OID_TCP_TASK_IPSEC_DELETE_SA)
        MAKECASE(oidName, OID_TCP_SAN_SUPPORT)
        MAKECASE(oidName, OID_TCP_TASK_IPSEC_ADD_UDPESP_SA)
        MAKECASE(oidName, OID_TCP_TASK_IPSEC_DELETE_UDPESP_SA)
        MAKECASE(oidName, OID_TCP4_OFFLOAD_STATS)
        MAKECASE(oidName, OID_TCP6_OFFLOAD_STATS)
        MAKECASE(oidName, OID_IP4_OFFLOAD_STATS)
        MAKECASE(oidName, OID_IP6_OFFLOAD_STATS)

        /* TCP offload OIDs for NDIS 6 */
        MAKECASE(oidName, OID_TCP_OFFLOAD_CURRENT_CONFIG)
        MAKECASE(oidName, OID_TCP_OFFLOAD_PARAMETERS)
        MAKECASE(oidName, OID_TCP_OFFLOAD_HARDWARE_CAPABILITIES)
        MAKECASE(oidName, OID_TCP_CONNECTION_OFFLOAD_CURRENT_CONFIG)
        MAKECASE(oidName, OID_TCP_CONNECTION_OFFLOAD_HARDWARE_CAPABILITIES)
        MAKECASE(oidName, OID_OFFLOAD_ENCAPSULATION)

        #if (NDIS_SUPPORT_NDIS620)
        /* VMQ OIDs for NDIS 6.20 */
        MAKECASE(oidName, OID_RECEIVE_FILTER_FREE_QUEUE)
        MAKECASE(oidName, OID_RECEIVE_FILTER_CLEAR_FILTER)
        MAKECASE(oidName, OID_RECEIVE_FILTER_ALLOCATE_QUEUE)
        MAKECASE(oidName, OID_RECEIVE_FILTER_QUEUE_ALLOCATION_COMPLETE)
        MAKECASE(oidName, OID_RECEIVE_FILTER_SET_FILTER)
        #endif

        #if (NDIS_SUPPORT_NDIS630)
        /* NDIS QoS OIDs for NDIS 6.30 */
        MAKECASE(oidName, OID_QOS_PARAMETERS)
        #endif
    }
    if (!oidType){
        RtlStringCbPrintfA(Buffer, sizeof(Buffer), "Unknown OID type: 0x%08X", Oid);
    } else if (!oidName) {
        RtlStringCbPrintfA(Buffer, sizeof(Buffer), "Unknown OID 0x%08X", Oid);
    } else  {
        RtlStringCbPrintfA(Buffer, sizeof(Buffer), "%s, %s", oidType, oidName);
    }
    DBG_ENET_DEV_OIDS_PRINT_INFO("%s, status: 0x%08X", Buffer, Status);
}


#define RETAILMSG(_zone_,...)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,DPFLTR_INFO_LEVEL,__VA_ARGS__)
#define WSTRINGER(_x)   L ## #_x
#define FORMAT_REGNAME_REGVALUE(_x)    "[%04X] %s\t= %#08x\n", offsetof(CSP_ENET_REGS,_x),#_x, (pAdapter->ENETRegBase->_x)

/*++
Routine Description:
    Print content of ENET device registers
Argument:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_DumpMACRegs(PMP_ADAPTER pAdapter)
{
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), "%s +++\r\n",__FUNCTION__);

    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(EIR.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(EIMR.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(RDAR));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(TDAR));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(ECR.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(MMFR.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(MSCR.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(MIBC.U));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(RCR.U));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(TCR.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(PALR));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(PAUR));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(OPD.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(IAUR));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(IALR));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(GAUR));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(GALR));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(TFWR.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(ERDSR));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(ETDSR));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(EMRBR));

    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(RSFL));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(RSEM));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(RAEM));
    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), FORMAT_REGNAME_REGVALUE(RAFL));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(TSEM));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(TAEM));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(TAFL));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(TIPG));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(FTRL));

    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(TACC.U));
    RETAILMSG( ZONE_REGDUMP,             FORMAT_REGNAME_REGVALUE(RACC.U));

    RETAILMSG((ZONE_REGDUMP||ZONE_INFO), "%s ---\r\n\r\n",__FUNCTION__);
}

/*++
Routine Description:
    Print transmit statistics
Argument:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_DumpTxStats(PMP_ADAPTER pAdapter)
{
    RETAILMSG(ZONE_REGDUMP, "%s +++\r\n",__FUNCTION__);

    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_DROP_NI));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_PACKETS));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_BC_PKT));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_MC_PKT));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_CRC_ALIGN));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_UNDERSIZE));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_FRAG));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_JAB));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_COL));

    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_P64));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_P65TO127));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_P128TO255));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_P256TO511));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_P512TO1023));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_P1024TO2047));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_P_GTE2048));

    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_T_OCTETS));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_DROP_NI));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_FRAME_OK));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_1COL));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_MCOL));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_DEF));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_LCOL));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_EXCOL));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_MACERR));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_CSERR));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_SQE_NI));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_FDXFC));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_T_OCTETS_OK));

    RETAILMSG(ZONE_REGDUMP, "%s ---\r\n\r\n",__FUNCTION__);
}

/*++
Routine Description:
    Print receive statistics
Argument:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_DumpRxStats(PMP_ADAPTER pAdapter)
{
    RETAILMSG(ZONE_REGDUMP, "%s +++\r\n",__FUNCTION__);

    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_PACKETS));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_BC_PKT));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_MC_PKT));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_CRC_ALIGN));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_UNDERSIZE));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_OVERSIZE));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_FRAG));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_JAB));

    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_P64));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_P65TO127));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_P128TO255));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_P256TO511));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_P512TO1023));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_P1024TO2047));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_P_GTE2048));

    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(RMON_R_OCTETS));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_R_DROP));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_R_FRAME_OK));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_R_CRC));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_R_ALIGN));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_R_MACERR));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_R_FDXFC));
    RETAILMSG(ZONE_REGDUMP, FORMAT_REGNAME_REGVALUE(IEEE_R_OCTETS_OK));

    RETAILMSG(ZONE_REGDUMP, "%s ---\r\n\r\n",__FUNCTION__);
}

/*++
Routine Description:
    Print transmit and receive statistics
Argument:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_DumpStatistics(PMP_ADAPTER pAdapter)
{
     Dbg_DumpTxStats(pAdapter);
     Dbg_DumpRxStats(pAdapter);
}

/*++
Routine Description:
    Print threshold and pause frame duration
Argument:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_DumpFifoTrasholdsAndPauseFrameDuration(PMP_ADAPTER pAdapter)
{
    CSP_ENET_REGS  *ENETRegBase = (CSP_ENET_REGS*)pAdapter->ENETRegBase;

    if (ENETRegBase->RSFL) { // Rx Cut-through mode?
        DBG_ENET_DEV_PRINT_INFO("< RX_SECTION_FULL:           %5d Bytes (RxDMA start threshold)",                             ENETRegBase->RSFL*8);
        DBG_ENET_DEV_PRINT_INFO("< RX_ALMOST_EMPTY:           %5d Bytes (RxDMA stop threshold)",                              ENETRegBase->RAEM*8);
        if (ENETRegBase->RAEM < 6) {
            DBG_ENET_DEV_PRINT_ERROR("RxDMA start threshold >= 48");
        }
        if (ENETRegBase->RSFL <= ENETRegBase->RAEM) {
            DBG_ENET_DEV_PRINT_ERROR("RxDMA start threshold > RxDMA stop threshold");
        }
    } else { //  Rx Store and forward mode
        DBG_ENET_DEV_PRINT_INFO("< RX_SECTION_FULL:           Store And Forward mode (Rx DMA starts only when a complete frame is stored in the Rx FIFO");
    }
    DBG_ENET_DEV_PRINT_INFO("< RX_SECTION_EMPTY:          %5d Bytes (RxFIFO slow down and XON/XOFF Tx packet threshold)", ENETRegBase->RSEM*8);
    DBG_ENET_DEV_PRINT_INFO("< RX_ALMOST_FULL:            %5d Bytes (RxFIFO overflow threshold)",                         ENETRegBase->RAFL*8);
    if (ENETRegBase->RAFL < 4) {
        DBG_ENET_DEV_PRINT_ERROR("RxFIFO overflow threshold >= 16");
    }
    DBG_ENET_DEV_PRINT_INFO("< Tx pause frame quanta:     %5d Bits  ",                                                   ENETRegBase->OPD.U & ENET_OPD_PAUSE_DUR_MASK);
    DBG_ENET_DEV_PRINT_INFO("< Pause frame reception:  %s",ENETRegBase->RCR.B.FCE?" Enabled":"Disabled");
    if(!(ENETRegBase->TFWR.U & ENET_TCR_STRFWD_MASK)) {  // Tx Cut-through mode?
        DBG_ENET_DEV_PRINT_INFO("< TX_TFWR:                   %5d Bytes (TxFIFO Tx start threshold)",                     ENETRegBase->TFWR.B.TFWR? ENETRegBase->TFWR.B.TFWR*64 : 64);
        DBG_ENET_DEV_PRINT_INFO("< TX_ALMOST_EMPTY:           %5d Bytes (TxFIFO underflow threshold)",                    ENETRegBase->TAEM*8);
        if (ENETRegBase->TAEM < 4) {
            DBG_ENET_DEV_PRINT_ERROR("TxFIFO underflow threshold >= 16");
        }
    } else { // Tx Store and forward mode
        DBG_ENET_DEV_PRINT_INFO("< TX_TFWR:                   Store And Forward mode (Tx start only when a complete frame is stored in the Transmit FIFO");
    }
    DBG_ENET_DEV_PRINT_INFO("< TX_SECTION_EMPTY:          %5d Bytes (TxDMA slow down threshold)",                     ENETRegBase->TSEM*8);
    DBG_ENET_DEV_PRINT_INFO("< TX_ALMOST_FULL:            %5d Bytes (TxDMA stop threshold)",                          ENETRegBase->TAFL*8);
    if (ENETRegBase->TAFL < 4) {
        DBG_ENET_DEV_PRINT_ERROR("TxDMA stop threshold >= 16");
    }

}

#undef MAKECASE
#undef MAKECASE1
#undef MAKEDEFAULT
#define MAKECASE(Value) case Value:  return #Value;
#define MAKECASE1(Value,Name) case Value:  return #Name;
#define MAKEDEFAULT(Message) default: return"!!! "Message" name unknown !!!";

/*++
Routine Description:
    Get state machine name
Argument:
    MpState     State machine state
Return Value:
    Pointer to state name
--*/
_Use_decl_annotations_
const char* Dbg_GetMpStateMachineStateName(MP_STATE i) {
    switch (i) {
        MAKECASE1(SM_STATE_HALTED,       HALTED)
        MAKECASE1(SM_STATE_SHUTDOWN,     SHUTDOWN)
        MAKECASE1(SM_STATE_INITIALIZING, INITIALIZING)
        MAKECASE1(SM_STATE_PAUSED,       PAUSED)
        MAKECASE1(SM_STATE_PAUSING,      PAUSING)
        MAKECASE1(SM_STATE_RESTARTING,   RESTARTING)
        MAKECASE1(SM_STATE_RUNNING,      RUNNING)
        MAKECASE1(SM_STATE_RESET,        RESET)
        MAKECASE1(SM_STATE_ERROR,        ERROR)
        MAKEDEFAULT("State machine state")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetMpMediaConnectStateName(NDIS_MEDIA_CONNECT_STATE i) {
    switch (i) {
        MAKECASE1(MediaConnectStateUnknown,      UNKNOWN)
        MAKECASE1(MediaConnectStateDisconnected, DISCONNECTED)
        MAKECASE1(MediaConnectStateConnected,    CONNECTED)
        MAKEDEFAULT("Media connect state")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetMpMediaDuplexStateName(NDIS_MEDIA_CONNECT_STATE i) {
    switch (i) {
        MAKECASE1(MediaDuplexStateUnknown, UNKNOWN)
        MAKECASE1(MediaDuplexStateHalf,    HALF DUPLEX)
        MAKECASE1(MediaDuplexStateFull,    FULL DUPLEX)
        MAKEDEFAULT("Media duplex state")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetNdisPowerStateName(NDIS_DEVICE_POWER_STATE i) {
    switch (i) {
        MAKECASE1(NdisDeviceStateUnspecified, NdisDeviceStateUnspecified)
        MAKECASE1(NdisDeviceStateD0,          NdisDeviceStateD0)
        MAKECASE1(NdisDeviceStateD1,          NdisDeviceStateD1)
        MAKECASE1(NdisDeviceStateD2,          NdisDeviceStateD2)
        MAKECASE1(NdisDeviceStateD3,          NdisDeviceStateD3)
        MAKECASE1(NdisDeviceStateMaximum,     NdisDeviceStateMaximum)
        MAKEDEFAULT("NDIS power state")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetNdisStatusName(NDIS_STATUS i) {
    switch (i) {
        MAKECASE(NDIS_STATUS_SUCCESS)
        MAKECASE(NDIS_STATUS_RESOURCES)
        MAKECASE(NDIS_STATUS_MEDIA_DISCONNECTED)
        MAKECASE(NDIS_STATUS_PAUSED)
        MAKECASE(NDIS_STATUS_REQUEST_ABORTED)
        MAKECASE(NDIS_STATUS_LOW_POWER_STATE)
        MAKECASE(NDIS_STATUS_BUFFER_TOO_SHORT)
        MAKECASE(NDIS_STATUS_NOT_SUPPORTED)
        MAKECASE(NDIS_STATUS_INVALID_DATA)
        MAKECASE(NDIS_STATUS_FAILURE)
        MAKECASE(NDIS_STATUS_RESOURCE_CONFLICT)
        MAKECASE(NDIS_ERROR_CODE_RESOURCE_CONFLICT)
        MAKECASE(STATUS_ACPI_INVALID_DATA)
        MAKECASE(STATUS_TIMEOUT)
        MAKECASE(STATUS_OBJECT_NAME_NOT_FOUND)
        MAKEDEFAULT("NDIS status")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetNdisOidRequestTypeName(NDIS_REQUEST_TYPE i) {
    switch (i) {
        MAKECASE(NdisRequestQueryInformation)
        MAKECASE(NdisRequestSetInformation)
        MAKECASE(NdisRequestQueryStatistics)
        MAKEDEFAULT("NDIS OID request type")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetNdisOidName(NDIS_OID i) {

    switch (i) {
        /* Operational OIDs */
        MAKECASE(OID_GEN_SUPPORTED_LIST)
        MAKECASE(OID_GEN_HARDWARE_STATUS)
        MAKECASE(OID_GEN_MEDIA_SUPPORTED)
        MAKECASE(OID_GEN_MEDIA_IN_USE)
        MAKECASE(OID_GEN_MAXIMUM_LOOKAHEAD)
        MAKECASE(OID_GEN_MAXIMUM_FRAME_SIZE)
        MAKECASE(OID_GEN_LINK_SPEED)
        MAKECASE(OID_GEN_TRANSMIT_BUFFER_SPACE)
        MAKECASE(OID_GEN_RECEIVE_BUFFER_SPACE)
        MAKECASE(OID_GEN_TRANSMIT_BLOCK_SIZE)
        MAKECASE(OID_GEN_RECEIVE_BLOCK_SIZE)
        MAKECASE(OID_GEN_VENDOR_ID)
        MAKECASE(OID_GEN_VENDOR_DESCRIPTION)
        MAKECASE(OID_GEN_VENDOR_DRIVER_VERSION)
        MAKECASE(OID_GEN_CURRENT_PACKET_FILTER)
        MAKECASE(OID_GEN_CURRENT_LOOKAHEAD)
        MAKECASE(OID_GEN_DRIVER_VERSION)
        MAKECASE(OID_GEN_MAXIMUM_TOTAL_SIZE)
        MAKECASE(OID_GEN_PROTOCOL_OPTIONS)
        MAKECASE(OID_GEN_MAC_OPTIONS)
        MAKECASE(OID_GEN_MEDIA_CONNECT_STATUS)
        MAKECASE(OID_GEN_MAXIMUM_SEND_PACKETS)
        MAKECASE(OID_GEN_SUPPORTED_GUIDS)
        MAKECASE(OID_GEN_NETWORK_LAYER_ADDRESSES)
        MAKECASE(OID_GEN_TRANSPORT_HEADER_OFFSET)
        MAKECASE(OID_GEN_MEDIA_CAPABILITIES)
        MAKECASE(OID_GEN_PHYSICAL_MEDIUM)
        MAKECASE(OID_GEN_MACHINE_NAME)
        MAKECASE(OID_GEN_VLAN_ID)
        MAKECASE(OID_GEN_RNDIS_CONFIG_PARAMETER)

        /* Operational OIDs for NDIS 6.0 */
        MAKECASE(OID_GEN_MAX_LINK_SPEED)
        MAKECASE(OID_GEN_LINK_STATE)
        MAKECASE(OID_GEN_LINK_PARAMETERS)
        MAKECASE(OID_GEN_MINIPORT_RESTART_ATTRIBUTES)
        MAKECASE(OID_GEN_ENUMERATE_PORTS)
        MAKECASE(OID_GEN_PORT_STATE)
        MAKECASE(OID_GEN_PORT_AUTHENTICATION_PARAMETERS)
        MAKECASE(OID_GEN_INTERRUPT_MODERATION)
        MAKECASE(OID_GEN_PHYSICAL_MEDIUM_EX)

        /* Statistical OIDs */
        MAKECASE(OID_GEN_XMIT_OK)
        MAKECASE(OID_GEN_RCV_OK)
        MAKECASE(OID_GEN_XMIT_ERROR)
        MAKECASE(OID_GEN_RCV_ERROR)
        MAKECASE(OID_GEN_RCV_NO_BUFFER)
        MAKECASE(OID_GEN_DIRECTED_BYTES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_BYTES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_BYTES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_BYTES_RCV)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_RCV)
        MAKECASE(OID_GEN_MULTICAST_BYTES_RCV)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_RCV)
        MAKECASE(OID_GEN_BROADCAST_BYTES_RCV)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_RCV)
        MAKECASE(OID_GEN_RCV_CRC_ERROR)
        MAKECASE(OID_GEN_TRANSMIT_QUEUE_LENGTH)

        /* Statistical OIDs for NDIS 6.0 */
        MAKECASE(OID_GEN_STATISTICS)
        MAKECASE(OID_GEN_BYTES_RCV)
        MAKECASE(OID_GEN_BYTES_XMIT)
        MAKECASE(OID_GEN_RCV_DISCARDS)
        MAKECASE(OID_GEN_XMIT_DISCARDS)

        /* Misc OIDs */
        MAKECASE(OID_GEN_GET_TIME_CAPS)
        MAKECASE(OID_GEN_GET_NETCARD_TIME)
        MAKECASE(OID_GEN_NETCARD_LOAD)
        MAKECASE(OID_GEN_DEVICE_PROFILE)
        MAKECASE(OID_GEN_INIT_TIME_MS)
        MAKECASE(OID_GEN_RESET_COUNTS)
        MAKECASE(OID_GEN_MEDIA_SENSE_COUNTS)

        /* PnP power management operational OIDs */
        MAKECASE(OID_PNP_CAPABILITIES)
        MAKECASE(OID_PNP_SET_POWER)
        MAKECASE(OID_PNP_QUERY_POWER)
        MAKECASE(OID_PNP_ADD_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_REMOVE_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_ENABLE_WAKE_UP)
        MAKECASE(OID_PNP_WAKE_UP_PATTERN_LIST)

        /* PnP power management statistical OIDs */
        MAKECASE(OID_PNP_WAKE_UP_ERROR)
        MAKECASE(OID_PNP_WAKE_UP_OK)

        /* Ethernet operational OIDs */
        MAKECASE(OID_802_3_PERMANENT_ADDRESS)
        MAKECASE(OID_802_3_CURRENT_ADDRESS)
        MAKECASE(OID_802_3_MULTICAST_LIST)
        MAKECASE(OID_802_3_MAXIMUM_LIST_SIZE)
        MAKECASE(OID_802_3_MAC_OPTIONS)

        /* Ethernet operational OIDs for NDIS 6.0 */
        MAKECASE(OID_802_3_ADD_MULTICAST_ADDRESS)
        MAKECASE(OID_802_3_DELETE_MULTICAST_ADDRESS)

        /* Ethernet statistical OIDs */
        MAKECASE(OID_802_3_RCV_ERROR_ALIGNMENT)
        MAKECASE(OID_802_3_XMIT_ONE_COLLISION)
        MAKECASE(OID_802_3_XMIT_MORE_COLLISIONS)
        MAKECASE(OID_802_3_XMIT_DEFERRED)
        MAKECASE(OID_802_3_XMIT_MAX_COLLISIONS)
        MAKECASE(OID_802_3_RCV_OVERRUN)
        MAKECASE(OID_802_3_XMIT_UNDERRUN)
        MAKECASE(OID_802_3_XMIT_HEARTBEAT_FAILURE)
        MAKECASE(OID_802_3_XMIT_TIMES_CRS_LOST)
        MAKECASE(OID_802_3_XMIT_LATE_COLLISIONS)

        /*  TCP/IP OIDs */
        MAKECASE(OID_TCP_TASK_OFFLOAD)
        MAKECASE(OID_TCP_TASK_IPSEC_ADD_SA)
        MAKECASE(OID_TCP_TASK_IPSEC_DELETE_SA)
        MAKECASE(OID_TCP_SAN_SUPPORT)
        MAKECASE(OID_TCP_TASK_IPSEC_ADD_UDPESP_SA)
        MAKECASE(OID_TCP_TASK_IPSEC_DELETE_UDPESP_SA)
        MAKECASE(OID_TCP4_OFFLOAD_STATS)
        MAKECASE(OID_TCP6_OFFLOAD_STATS)
        MAKECASE(OID_IP4_OFFLOAD_STATS)
        MAKECASE(OID_IP6_OFFLOAD_STATS)

        /* TCP offload OIDs for NDIS 6 */
        MAKECASE(OID_TCP_OFFLOAD_CURRENT_CONFIG)
        MAKECASE(OID_TCP_OFFLOAD_PARAMETERS)
        MAKECASE(OID_TCP_OFFLOAD_HARDWARE_CAPABILITIES)
        MAKECASE(OID_TCP_CONNECTION_OFFLOAD_CURRENT_CONFIG)
        MAKECASE(OID_TCP_CONNECTION_OFFLOAD_HARDWARE_CAPABILITIES)
        MAKECASE(OID_OFFLOAD_ENCAPSULATION)

        #if (NDIS_SUPPORT_NDIS620)
        /* VMQ OIDs for NDIS 6.20 */
        MAKECASE(OID_RECEIVE_FILTER_FREE_QUEUE)
        MAKECASE(OID_RECEIVE_FILTER_CLEAR_FILTER)
        MAKECASE(OID_RECEIVE_FILTER_ALLOCATE_QUEUE)
        MAKECASE(OID_RECEIVE_FILTER_QUEUE_ALLOCATION_COMPLETE)
        MAKECASE(OID_RECEIVE_FILTER_SET_FILTER)
        #endif

        #if (NDIS_SUPPORT_NDIS630)
        /* NDIS QoS OIDs for NDIS 6.30 */
        MAKECASE(OID_QOS_PARAMETERS)
        #endif

        MAKEDEFAULT("NDIS OID request")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetACPIFunctionName(ULONG i) {
    switch (i) {
        MAKECASE1(IMX_ENET_DSM_FUNCTION_SUPPORTED_FUNCTIONS_INDEX,           SUPPORTED_FUNCTIONS)
        MAKECASE1(IMX_ENET_DSM_FUNCTION_GET_MDIO_BUS_ENET_PHY_ADDRESS_INDEX, MDOI_BUS_ENET_PHY_ADDRES)
        MAKECASE1(IMX_ENET_DSM_FUNCTION_GET_MAC_ADDRESS_INDEX,               MAC_ADDRESS)
        MAKECASE1(IMX_ENET_DSM_FUNCTION_GET_MDIO_BASE_ADDRESS_INDEX,         MDIO_BASE_ADDRESS)
        MAKECASE1(IMX_ENET_DSM_FUNCTION_GET_ENET_PHY_INTERFACE_TYPE_INDEX,   ENET_PHY_INTERFACE_TYPE)
        MAKEDEFAULT("ACPI function")
    }
}

/*++
Routine Description:
    Converts input value to a string.
Argument:
    i  input value
Return Value:
    Pointer to the string
--*/
_Use_decl_annotations_
const char* Dbg_GetEnetPhyInterfaceTypeName(_In_ MP_MDIO_PHY_INTERFACE_TYPE i) {
    switch (i) {
        MAKECASE(RGMII)
        MAKECASE(RMII)
        MAKECASE(MII)
        MAKEDEFAULT("Enet Phy interface type")
    }
}

/*++
Routine Description:
    Prints MDIO bus info. It is supposed that MDIO bus spin lock is held by the caller.
Argument:
    pMDIOBus  MDIO bus handle
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_PrintMDIOBusInfo(MP_MDIO_BUS *pMDIOBus, char* prefix) {
    MP_MDIO_DEVICE *pMDIODev;
    char indent[32];

    indent[0] = '\0';
    strcat(indent, prefix);
    DBG_MDIO_BUS_PRINT_INFO("%s Dump begin:",                   indent);
    DBG_MDIO_BUS_PRINT_INFO("%s  First device:         0x%p",   indent, pMDIOBus->MDIOBus_pFirstDevice);
    DBG_MDIO_BUS_PRINT_INFO("%s  Active device:        0x%p",   indent, pMDIOBus->MDIOBus_pActiveDevice);
    DBG_MDIO_BUS_PRINT_INFO("%s  Transfer in progress: %d",     indent, pMDIOBus->MDIOBus_TransferInProgress);
    pMDIODev = pMDIOBus->MDIOBus_pFirstDevice;
    while (pMDIODev) {
        NdisAcquireSpinLock(&pMDIODev->MDIODev_DeviceSpinLock);
        DBG_MDIO_BUS_PRINT_INFO("%s  %s (0x%p) -  Head: 0x%p, Tail: 0x%p, Free: 0x%p", indent, pMDIODev->MDIODev_DeviceName, pMDIODev, pMDIODev->MDIODev_pPendingFrameListHead, pMDIODev->MDIODev_pPendingFrameListTail, pMDIODev->MDIODev_pFreeFrameListHead);
        NdisReleaseSpinLock(&pMDIODev->MDIODev_DeviceSpinLock);
        pMDIODev = pMDIODev->MDIODev_pNextDevice;
    }
    DBG_MDIO_BUS_PRINT_INFO("%s Dump end:", indent);
}

/*++
Routine Description:
    Prints ENET device info.
Argument:
    pMDIOBus  MDIO bus handle
Return Value:
    None
--*/
_Use_decl_annotations_
void Dbg_PrintENETDeviceSettings(MP_ADAPTER *pENETDev) {
    MP_MDIO_DEVICE *pMDIODev = &pENETDev->ENETDev_MDIODevice;
    MP_PHY_DEVICE  *pPHYDev  = &pENETDev->ENETDev_PHYDevice;
    MP_MDIO_BUS    *pMDIOBus = pMDIODev->MDIODev_pBus;

    DBG_PRINT_BASIC_ENET_CONFIGURATION("%s ENET device address: 0x%08x_%08x, Irq: 0x%x, MAC: %02X-%02X-%02X-%02X-%02X-%02X\n"
                                       "      MDIO device address: 0x%08x_%08x(%s), ENET_PHY address: %d, MDIO clock %d[kHz], MDIO hold time %d[ns], Preamble: %s\n"
                                       "      ENET PHY ID: 0x%08X, type: %s name: %s",\
        pENETDev->ENETDev_DeviceName,\
        NdisGetPhysicalAddressHigh(pENETDev->ENETDev_RegsPhyAddress),\
        NdisGetPhysicalAddressLow(pENETDev->ENETDev_RegsPhyAddress),\
        pENETDev->ENETDev_Irq,\
        pENETDev->FecMacAddress[0],pENETDev->FecMacAddress[1],pENETDev->FecMacAddress[2],pENETDev->FecMacAddress[3],pENETDev->FecMacAddress[4],pENETDev->FecMacAddress[5],\
        NdisGetPhysicalAddressHigh(pMDIOBus->MDIOBus_RegsPhyAddress),\
        NdisGetPhysicalAddressLow(pMDIOBus->MDIOBus_RegsPhyAddress),\
        pMDIOBus->MDIOBus_DeviceName, \
        pMDIODev->MDIODev_EnetPhyAddress >> ENET_MMFR_PA_SHIFT, \
        pMDIODev->MDIODev_Frequency_kHz, \
        pMDIODev->MDIODev_STAHoldTime_ns, \
        pMDIODev->MDIODev_DisableFramePreamble?"No":"Yes", \
        pPHYDev->PHYDev_PhyId, \
        Dbg_GetEnetPhyInterfaceTypeName(pMDIODev->MDIODev_PhyInterfaceType), \
        pPHYDev->PHYDev_PhySettings?pPHYDev->PHYDev_PhySettings->PhyName:"unknown"\
    );
}

#endif // DBG
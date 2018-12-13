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

#ifndef _MP_HW_H
#define _MP_HW_H

#ifdef __cplusplus
extern "C" {
#endif

// The Version of NDIS that the ENET driver is compatible with
#define ENET_NDIS_MAJOR_VERSION                    6
#define ENET_NDIS_MINOR_VERSION                   30

#define NDIS60_MINIPORT                            1

// NDIS 'is alive' poll interval in seconds
#define MAC_IS_ALIVE_POLL_SEC                      2

// Phy related constants

//Speed Defines
#define SPEED_1000MBPS                          1000 //bits per second
#define SPEED_100MBPS                            100 //bits per second
#define SPEED_10MBPS                              10 //bits per second
#define SPEED_FACTOR                         1000000 //Mega (10^6)
#define SPEED_AUTO                                 0 // Auto Negotiation
#define SPEED_HALF_DUPLEX_10M                      1 // 10Mbps/Half Duplex
#define SPEED_FULL_DUPLEX_10M                      2 // 10Mbps/Full Duplex
#define SPEED_HALF_DUPLEX_100M                     3 // 100Mbps/Half Duplex
#define SPEED_FULL_DUPLEX_100M                     4 // 100Mbps/Full Duplex
#define SPEED_HALF_DUPLEX_1G                       5 // 1.0Gbps/Half Duplex
#define SPEED_FULL_DUPLEX_1G                       6 // 1.0Gbps/Full Duplex

// Hash creation constants
#define CRC_PRIME                        0xFFFFFFFF
#define CRC_POLYNOMIAL                   0xEDB88320
#define HASH_BITS                                 6

#define MCAST_LIST_SIZE                          64 // number of multicast addresses supported

// Packet length definitions
#define ETHER_ADDR_LENGTH                         6  // Size of Ethernet address (source or destination)
#define ETHER_FRAME_HEADER_LENGTH                14  // Size of Ethernet frame MAC header
#define ETHER_FRAME_MAX_PAYLOAD_LENGTH         1500  // Maximal Ethernet frame data payload size
#define ETHER_FRAME_MIN_PAYLOAD_LENGTH           46  // Minimal Ethernet frame data payload size
#define ETHER_FRAME_CRC_LENGTH                    4  // Size of Ethernet frame CRC
#define ETHER_FRAME_MAX_LENGTH                 1518  // Maximal Ethernet frame size (sum of header, payload and CRC)
#define ETHER_FRAME_NIN_LENGTH                   64  // Minimal Ethernet frame size (sum of header, payload and CRC)

// Configuration from registry, should be the same as in INF file
#define RX_DESC_COUNT_DEFAULT                   128  // Number of Rx buffer descriptors
#define RX_DESC_COUNT_MIN                         3
#define RX_DESC_COUNT_MAX                       128
#define TX_DESC_COUNT_DEFAULT                    64  // Number of Tx buffer descriptors
#define TX_DESC_COUNT_MIN                         2
#define TX_DESC_COUNT_MAX                        64
#define SPEED_SELECT_DEFAULT             SPEED_AUTO  // Speed select
#define SPEED_SELECT_MIN                 SPEED_AUTO
#define SPEED_SELECT_MAX     SPEED_FULL_DUPLEX_100M

#define ENET_RX_FRAME_SIZE                     2048
#define ENET_TX_FRAME_SIZE                     2048

#define MMI_DATA_MASK                         0xFFFF

#define ENET_TX_ERR_INT_MASK (ENET_EIR_LC_MASK| ENET_EIR_RL_MASK | ENET_EIR_UN_MASK)
#define ENET_RX_INT_MASK     (ENET_EIR_RXF_MASK)
#define ENET_TX_INT_MASK     (ENET_EIR_TXF_MASK | ENET_TX_ERR_INT_MASK)
#define ENET_RX_TX_INT_MASK  (ENET_RX_INT_MASK | ENET_TX_INT_MASK | ENET_EIR_GRA_MASK)

// statistic counters for the frames which have been received by the ENET
typedef struct  _FRAME_RCV_STATUS
{
    ULONG    FrameRcvGood;
    ULONG    FrameRcvErrors;
    ULONG    FrameRcvExtraDataErrors;
    ULONG    FrameRcvShortDataErrors;
    ULONG    FrameRcvCRCErrors;
    ULONG    FrameRcvOverrunErrors;
    ULONG    FrameRcvAllignmentErrors;
    ULONG    FrameRcvLCErrors;
} FRAME_RCV_STATUS,  *PFRAME_RCV_STATUS;

// statistic counters for the frames which have been transmitted by the ENET
typedef struct  _FRAME_TXD_STATUS
{
    ULONG    FramesXmitGood;
    ULONG    FramesXmitBad;
    ULONG    FramesXmitHBErrors;
    ULONG    FramesXmitCollisionErrors;
    ULONG    FramesXmitAbortedErrors;
    ULONG    FramesXmitUnderrunErrors;
    ULONG    FramsXmitCarrierErrors;
} FRAME_TXD_STATUS,  *PFRAME_TXD_STATUS;

MINIPORT_ISR EnetIsr;
MINIPORT_SYNCHRONIZE_INTERRUPT EnetEnableRxAndTxInterrupts;
MINIPORT_SYNCHRONIZE_INTERRUPT EnetDisableRxAndTxInterrupts;

typedef struct _MP_ADAPTER MP_ADAPTER,*PMP_ADAPTER;

// ENET hardware related functions
void EnetInit  (_In_ PMP_ADAPTER pAdapter, _In_ MP_MDIO_PHY_INTERFACE_TYPE EnetPhyInterfaceType);
void EnetDeinit(_In_ PMP_ADAPTER pAdapter);
void EnetStop  (_In_ PMP_ADAPTER pAdapter, _In_ NDIS_STATUS NdisStatus);
void EnetStart (_In_ PMP_ADAPTER pAdapter);

// Multicast hash tables related functions
void ClearAllMultiCast(_In_ PMP_ADAPTER Adapter);
void AddMultiCast     (_In_ PMP_ADAPTER Adapter, _In_ UCHAR *pAddr);
void SetUnicast       (_In_ PMP_ADAPTER Adapter);

#endif // _MP_HW_H

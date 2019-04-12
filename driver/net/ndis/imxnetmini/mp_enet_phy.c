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

// --------------------------------------------------------------------------------------------------------------------
//  IEEE Standard PHY registers.
// --------------------------------------------------------------------------------------------------------------------

// Register definitions for the PHY
#define MII_REG_CR              0x00  // Control Register
#define MII_REG_SR              0x01  // Status Register
#define MII_REG_PHYIR1          0x02  // PHY Identification Register 1
#define MII_REG_PHYIR2          0x03  // PHY Identification Register 2
#define MII_REG_ANAR            0x04  // A-N Advertisement Register
#define MII_REG_ANLPAR          0x05  // A-N Link Partner Ability Register
#define MII_REG_ANER            0x06  // A-N Expansion Register
#define MII_REG_ANNPTR          0x07  // A-N Next Page Transmit Register
#define MII_REG_ANLPRNPR        0x08  // A-N Link Partner Received Next Page Register
#define MII_REG_1000BASETCR     0x09  // 1000 Base-T control Register
#define MII_REG_1000BASETST     0x0A  // 1000 Base-T status Register
#define MII_REG_EXTCR           0x0B  // Extended Register -- Control
#define MII_REG_EXTDW           0x0C  // Extended Register -- Data Write
#define MII_REG_EXTDR           0x0D  // Extended Register -- Data Read
#define MII_REG_EXTMIISR        0x0F  // Extended -- MII Status
#define MII_REG_MMD_CR          0x0D  // MMD Access -- Control
#define MII_REG_MMD_RD          0x0E  // MMD Access -- Register/Data
#define MII_REG_EXTSTATUS       0x0F  // Extended Status

//  MII_REG_CR
#define MII_REG_CR_RESET                    (1 << 15)
#define MII_REG_CR_LOOPBACK                 (1 << 14)
#define MII_REG_CR_SPEEDSELECT              (3 << 13)
#define MII_REG_CR_AN_ENABLE                (1 << 12)
#define MII_REG_CR_POWERDOWN                (1 << 11)
#define MII_REG_CR_ISOLATE                  (1 << 10)
#define MII_REG_CR_RESTART_AN               (1 << 9)
#define MII_REG_CR_DUPLEXMODE               (1 << 8)
#define MII_REG_CR_COLLISSION_TEST          (1 << 7)

//  MII_REG_SR
#define MII_REG_SR_100BASE_T4               (1 << 15)
#define MII_REG_SR_100BASE_TX_FULLDUPLEX    (1 << 14)
#define MII_REG_SR_100BASE_TX_HALFDUPLEX    (1 << 13)
#define MII_REG_SR_10BASE_TX_FULLDUPLEX     (1 << 12)
#define MII_REG_SR_10BASE_TX_HALFDUPLEX     (1 << 11)
#define MII_REG_SR_AN_COMPLETE              (1 << 5)
#define MII_REG_SR_REMOTEFAULT              (1 << 4)
#define MII_REG_SR_AN_ABILITY               (1 << 3)
#define MII_REG_SR_LINKSTATUS               (1 << 2)
#define MII_REG_SR_JABBER_DETECT            (1 << 1)
#define MII_REG_SR_EXTENDEDCAPABILITY       (1 << 0)

//  MII_REG_ANAR BitFields
#define MII_REG_ANAR_100BASETX_FD           (1 << 8)    // 100BASE-TX Full Duplex
#define MII_REG_ANAR_100BASETX_HD           (1 << 7)    // 100BASE-TX Half Duplex
#define MII_REG_ANAR_10BASETX_FD            (1 << 6)    // 10BASE-TX Full Duplex
#define MII_REG_ANAR_10BASETX_HD            (1 << 5)    // 10BASE-TX Half Duplex

//  MII_REG_ANLPAR BitFields
#define MII_REG_ANLPAR_PAUSE                (1 << 10)   // Pause
#define MII_REG_ANLPAR_100BASETX_FD         (1 << 8)    // 100BASE-TX Full Duplex
#define MII_REG_ANLPAR_100BASETX_HD         (1 << 7)    // 100BASE-TX Half Duplex
#define MII_REG_ANLPAR_10BASETX_FD          (1 << 6)    // 10BASE-TX Full Duplex
#define MII_REG_ANLPAR_10BASETX_HD          (1 << 5)    // 10BASE-TX Half Duplex

//  MII_REG_1000BASETCR BitFields
#define MII_REG_1000BASETCR_1000BT_FD       (1 << 9)    // 1000BASE-T Full Duplex
#define MII_REG_1000BASETCR_1000BT_HD       (1 << 8)    // 1000BASE-T Half Duplex

//  MII_REG_1000BASETST BitFields
#define MII_REG_1000BASETST_MsSl_Fault      (1 << 15)   // Master-Slave fault detected if set to 1
#define MII_REG_1000BASETST_MsSl_Set        (1 << 14)   // Master-Slave Configuration, Read 1 means PHY is Master, 0 for slave
#define MII_REG_1000BASETST_LRS             (1 << 13)   // Local Receiver Status, 1 means OK 0 means not OK
#define MII_REG_1000BASETST_RRS             (1 << 12)   // Remote Receiver Status, 1 means OK 0 means not OK
#define MII_REG_1000BASETST_LP1000FD        (1 << 11)   // Link partner is 1000Base T Full Duplex capable
#define MII_REG_1000BASETST_LP1000HD        (1 << 10)   // Link partner is 1000Base T Half  Duplex capable
#define MII_REG_1000BASETST_IEC_MASK        (0XFF)      // Idle Error Count


void EnetParse_MII_C       (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetParse_MII_S       (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetParse_MII_ANA     (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetParse_MII_ANA_1GB (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetParse_MII_LPA     (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetParse_MII_LPA_1GB (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
#if DBG
void EnetParse_MII_BASET   (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetParse_MII_SS      (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
#endif
void EnetPauseOff          (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetPauseOn           (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);

void EnetDispPHYCfg        (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void EnetParsePHYLink      (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);

void MDIODev_SavePHYID1    (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);
void MDIODev_SavePHYID2    (_In_ UINT32 RegVal, _In_ NDIS_HANDLE MiniportAdapterHandle);


ENET_PHY_CMD PHYCmdGetPhyId[] = {
    {MII_READ_COMMAND(MII_REG_PHYIR1), MDIODev_SavePHYID1},
    {MII_READ_COMMAND(MII_REG_PHYIR2), MDIODev_SavePHYID2},
    {ENET_MII_END,                     NULL}
};

ENET_PHY_CMD Start1000FDX[] = {
    {MII_WRITE_COMMAND(MII_REG_1000BASETCR, 0x0200), NULL},                   // Enable 1000BASE-T support
    {MII_WRITE_COMMAND(MII_REG_ANAR, 0x1C01),        NULL},                   // Disable 100BASE, 10BASE support
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1300),          EnetParse_MII_C},        // Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),                 EnetParse_MII_ANA},
    {MII_READ_COMMAND(MII_REG_1000BASETCR),          EnetParse_MII_ANA_1GB},
    {ENET_MII_END, NULL }
};

ENET_PHY_CMD Start100FDX[] = {
    {MII_WRITE_COMMAND(MII_REG_1000BASETCR, 0x0000), NULL},                   // Disable 1000BASE-T support
    {MII_WRITE_COMMAND(MII_REG_ANAR, 0x1D01),        NULL},                   // Enable only 100BASE full-duplex
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1300),          EnetParse_MII_C},        // Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),                 EnetParse_MII_ANA},
    {ENET_MII_END,                                   NULL}
};

ENET_PHY_CMD Start100HDX[] = {
    {MII_WRITE_COMMAND(MII_REG_1000BASETCR, 0x0000), NULL},                   // Disable 1000BASE-T support
    {MII_WRITE_COMMAND(MII_REG_ANAR, 0x1C81),        NULL},                   // Enable only 100BASE half-duplex
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1300),          EnetParse_MII_C},        // Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),                 EnetParse_MII_ANA},
    {ENET_MII_END,                                   NULL}
};

ENET_PHY_CMD Start10FDX[] = {
    {MII_WRITE_COMMAND(MII_REG_1000BASETCR, 0x0000), NULL},                   // Disable 1000BASE-T support
    {MII_WRITE_COMMAND(MII_REG_ANAR, 0x1C41),        NULL},                   // Enable only 10BASE full-duplex
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1300),          EnetParse_MII_C},        // Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),                 EnetParse_MII_ANA},
    {ENET_MII_END,                                   NULL}
};

ENET_PHY_CMD Start10HDX[] = {
    {MII_WRITE_COMMAND(MII_REG_1000BASETCR, 0x0000), NULL},                   // Disable 1000BASE-T support
    {MII_WRITE_COMMAND(MII_REG_ANAR, 0x1C21),        NULL},                   // Enable only 10BASE half-duplex
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1300),          EnetParse_MII_C},        // Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),                 EnetParse_MII_ANA},
    {ENET_MII_END,                                   NULL}
};

ENET_PHY_CMD PHYCmdCfg[] = {
    {MII_READ_COMMAND(MII_REG_CR), EnetParse_MII_C},
    {MII_READ_COMMAND(MII_REG_CR), EnetDispPHYCfg},
    {ENET_MII_END,                 NULL}
};

ENET_PHY_CMD PHYCmdLink[] = {
    {MII_READ_COMMAND(MII_REG_SR), EnetParse_MII_S},    // LINK_DETECTED[up/down], REMOTE_FAULT[yes/no] AN_COMPLETE_MASK[yes/no]
    {MII_READ_COMMAND(MII_REG_SR), EnetParsePHYLink},
    {ENET_MII_END,                 NULL}
};

ENET_PHY_CMD PHYCmdSuspend[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x800), NULL},
    {ENET_MII_END,                         NULL}
};

ENET_PHY_CMD PHYCmdResume[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1200), NULL},
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD PHYCmdReset[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x9000), NULL},
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD PHYCmd1000Off[] = {
    {MII_WRITE_COMMAND(MII_REG_1000BASETCR, 0x0000), EnetPauseOff},  // Switch off 1000BASE-T support
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1300),          NULL},          // Restart Auto-Negotiation
    {ENET_MII_END,                                   NULL}
};

ENET_PHY_CMD PHYCmd1000On[] = {
    {MII_WRITE_COMMAND(MII_REG_1000BASETCR, 0x0200), EnetPauseOn },  // Switch on 1000BASE-T support
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1300),          NULL},          // Restart Auto-Negotiation
    {ENET_MII_END,                                   NULL}
};

// --------------------------------------------------------------------------------------------------------------------
// PHY specific settings
// --------------------------------------------------------------------------------------------------------------------

//  AR8031/AR8033 PHY

//  IEEE Non-Standard PHY registers.
//      Register from 0x0 to 0xF should be standard across all Ethernet PHYs.
//      But, AR8031 and KSZ9021 PHYs seems to have differences in the registers from 0xB to 0xF
//      Hence, having separate #defines for registers from 0xB to 0xF
#define MII_REG_AR8031_RSVD1                            0xB  // IEEE Non-Standard -- Reserved
#define MII_REG_AR8031_RSVD2                            0xC  // IEEE Non-Standard -- Reserved
#define MII_REG_AR8031_MMD_ACR                          0xD  // IEEE Non-Standard -- MMD Access Control Register
#define MII_REG_AR8031_MMD_AADR                         0xE  // IEEE Non-Standard -- MMD Access Address Data Register
#define MII_REG_AR8031_ESR                              0xF  // IEEE Non-Standard -- Extended Status Register

//  Vendor specific PHY registers
#define MII_REG_AR8031_SS                               0x14  // Vendor -- Smart Speed
#define MII_REG_AR8031_DP_ADDR                          0x1D  // Vendor -- Debug Port (Address Offset Set)
#define MII_REG_AR8031_DP_RW                            0x1E  // Vendor -- Debug Port2 (R/W Port)

ENET_PHY_CMD AR8031Config[] = {
    //  Select 125MHz Clock
    {MII_WRITE_COMMAND(MII_REG_AR8031_MMD_ACR, 0x7),     NULL}, // Choose MMD7
    {MII_WRITE_COMMAND(MII_REG_AR8031_MMD_AADR, 0x8016), NULL}, // Choose CLK_25M Clock Select on MMD7
    {MII_WRITE_COMMAND(MII_REG_AR8031_MMD_ACR, 0x4007),  NULL}, // Retain MMD7
    {MII_WRITE_COMMAND(MII_REG_AR8031_MMD_AADR, 0x18),   NULL}, // Select 110 - 125Mhz from Local PLL Source
    //  Enable GTX_CLK delay
    {MII_WRITE_COMMAND(MII_REG_AR8031_DP_ADDR, 0x5),     NULL}, // Choose SerDes Test and System Mode Control
    {MII_WRITE_COMMAND(MII_REG_AR8031_DP_RW, 0x0100),    NULL}, // Select 1 - RGMII Tx Clock Delay Enable
    //  Specific
    {MII_WRITE_COMMAND(MII_REG_AR8031_SS, 0x000C),       NULL}, // Smart speed off
    {MII_WRITE_COMMAND(MII_REG_ANAR, 0x1DE1),            NULL}, // Enable Pause frames
    {ENET_MII_END,                                       NULL}
};

ENET_PHY_CMD AR8031Startup[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C},       // Enable and Restart Auto-Negotiation
    {MII_READ_COMMAND(MII_REG_ANAR),        EnetParse_MII_ANA},
    {MII_READ_COMMAND(MII_REG_1000BASETCR), EnetParse_MII_ANA_1GB},
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD AR8031Actint[] = {
    {MII_READ_COMMAND(MII_REG_SR),          EnetParse_MII_S},       // Read and parse Status register                (Offset: 0x01)
    {MII_READ_COMMAND(MII_REG_ANLPAR),      EnetParse_MII_LPA},     // Read and parse Link partner ability register  (Offset: 0x05)
    {MII_READ_COMMAND(MII_REG_1000BASETST), EnetParse_MII_LPA_1GB}, // Read and parse 1000 Base-T status             (Offset: 0x0A)
    {ENET_MII_END, NULL }
};

MP_PHY_INFO AR8031Info = {
    ENET_PHY_AR8031,
    TEXT("AR8031/AR8033"),
    AR8031Config,
    AR8031Startup,
    AR8031Actint,
    NULL
};

//  AR8035 PHY

//  IEEE Non-Standard PHY registers.
//      Register from 0x0 to 0xF should be standard across all Ethernet PHYs.
//      But, AR8031 and KSZ9021 PHYs seems to have differences in the registers from 0xB to 0xF
//      Hence, having separate #defines for registers from 0xB to 0xF
#define MII_REG_AR8035_RSVD1                            0xB  // IEEE Non-Standard -- Reserved
#define MII_REG_AR8035_RSVD2                            0xC  // IEEE Non-Standard -- Reserved
#define MII_REG_AR8035_MMD_ACR                          0xD  // IEEE Non-Standard -- MMD Access Control Register
#define MII_REG_AR8035_MMD_AADR                         0xE  // IEEE Non-Standard -- MMD Access Address Data Register
#define MII_REG_AR8035_ESR                              0xF  // IEEE Non-Standard -- Extended Status Register

//  Vendor specific PHY registers
#define MII_REG_AR8035_PHY_SS                           0x11  // Vendor -- PHY-specific status
#define MII_REG_AR8035_SS                               0x14  // Vendor -- Smart Speed
#define MII_REG_AR8035_DP_ADDR                          0x1D  // Vendor -- Debug Port (Address Offset Set)
#define MII_REG_AR8035_DP_RW                            0x1E  // Vendor -- Debug Port2 (R/W Port)

ENET_PHY_CMD AR8035Config[] = {
    //  Select 125MHz Clock
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_ACR, 0x7),     NULL}, // Choose MMD7
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_AADR, 0x8016), NULL}, // Choose CLK_25M Clock Select on MMD7
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_ACR, 0x4007),  NULL}, // Retain MMD7
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_AADR, 0x18),   NULL}, // Select 110 - 125Mhz from Local PLL Source
    // Enable GTX_CLK delay
    {MII_WRITE_COMMAND(MII_REG_AR8035_DP_ADDR, 0x5),     NULL}, // Choose SerDes Test and System Mode Control
    {MII_WRITE_COMMAND(MII_REG_AR8035_DP_RW, 0x0100),    NULL}, // Select 1 - RGMII Tx Clock Delay Enable
    // Specific
    {MII_WRITE_COMMAND(MII_REG_AR8035_SS, 0x080C),       NULL}, // Smart speed off
    {MII_WRITE_COMMAND(MII_REG_ANAR, 0x1DE1),            NULL}, // Enable Pause frames
    // Disable SmartEEE function
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_ACR, 0x3),     NULL}, // Choose MMD3
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_AADR, 0x805D), NULL}, // Choose Smart_EEE Control 3 on MMD3
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_ACR, 0x4003),  NULL}, // Retain MMD3
    {MII_WRITE_COMMAND(MII_REG_AR8035_MMD_AADR, 0x1000), NULL}, // Disable Smart EEE
    {ENET_MII_END,                                       NULL}
};

ENET_PHY_CMD AR8035Startup[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C},      // Enable and Restart Auto-Negotiation
    {MII_READ_COMMAND(MII_REG_ANAR),        EnetParse_MII_ANA},
    {MII_READ_COMMAND(MII_REG_1000BASETCR), EnetParse_MII_ANA_1GB},
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD AR8035Actint[] = {
    {MII_READ_COMMAND(MII_REG_SR),            EnetParse_MII_S},
    {MII_READ_COMMAND(MII_REG_ANLPAR),        EnetParse_MII_LPA},
    {MII_READ_COMMAND(MII_REG_1000BASETST),   EnetParse_MII_LPA_1GB},
    #if DBG
    {MII_READ_COMMAND(MII_REG_AR8035_PHY_SS), EnetParse_MII_SS},
    #endif
    {ENET_MII_END,                            NULL}
};

MP_PHY_INFO AR8035Info = {
    ENET_PHY_AR8035,
    TEXT("AR8035"),
    AR8035Config,
    AR8035Startup,
    AR8035Actint,
    NULL
};

// KSZ8081 PHY
ENET_PHY_CMD KSZ8081Config[] = {
    {MII_WRITE_COMMAND(0x00, 0x3140), NULL},
    {MII_WRITE_COMMAND(0x00, 0x3340), NULL},
    {MII_WRITE_COMMAND(0x1F, 0x8190), NULL},
    {ENET_MII_END,                    NULL}
};

ENET_PHY_CMD KSZ8081Startup[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C},       /* 100 Base T, Full Duplex */
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD KSZ8081Actint[] = {
    {MII_READ_COMMAND(MII_REG_SR),            EnetParse_MII_S},
    {MII_READ_COMMAND(MII_REG_ANLPAR),        EnetParse_MII_LPA},
    {ENET_MII_END,                            NULL}
};

MP_PHY_INFO KSZ8081Info = {
    ENET_PHY_KSZ8081,
    TEXT("KSZ8081"),
    KSZ8081Config,
    KSZ8081Startup,
    KSZ8081Actint,
    NULL
};

// KSZ8091 PHY
ENET_PHY_CMD KSZ8091Config[] = {
    {MII_WRITE_COMMAND(0x00, 0x3140), NULL},
    {MII_WRITE_COMMAND(0x00, 0x3340), NULL},
    {ENET_MII_END,                    NULL}
};

ENET_PHY_CMD KSZ8091Startup[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C},       /* Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),        EnetParse_MII_ANA},
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD KSZ8091Actint[] = {
    {MII_READ_COMMAND(MII_REG_SR),            EnetParse_MII_S},
    {MII_READ_COMMAND(MII_REG_ANLPAR),        EnetParse_MII_LPA},
    {ENET_MII_END,                            NULL}
};

MP_PHY_INFO KSZ8091Info = {
    ENET_PHY_KSZ8091,
    TEXT("KSZ8091"),
    KSZ8091Config,
    KSZ8091Startup,
    KSZ8091Actint,
    NULL
};

// KSZ9021 PHY
ENET_PHY_CMD KSZ9021Config[] = {
    { MII_WRITE_COMMAND(0x00, 0x3140), NULL },
    { MII_WRITE_COMMAND(0x00, 0x3340), NULL },
    { ENET_MII_END,                    NULL }
};

ENET_PHY_CMD KSZ9021Startup[] = {
    { MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C },       /* Enable and Restart Auto-Negotiation */
    { MII_READ_COMMAND(MII_REG_ANAR),        EnetParse_MII_ANA },
    { ENET_MII_END,                          NULL }
};

ENET_PHY_CMD KSZ9021Actint[] = {
    { MII_READ_COMMAND(MII_REG_SR),            EnetParse_MII_S },
    { MII_READ_COMMAND(MII_REG_ANLPAR),        EnetParse_MII_LPA },
    { ENET_MII_END,                            NULL }
};

MP_PHY_INFO KSZ9021Info = {
	ENET_PHY_KSZ9021,
	TEXT("KSZ9021"),
	KSZ9021Config,
	KSZ9021Startup,
	KSZ9021Actint,
	NULL
};

// RTL8211E Phy Add by kmslove

ENET_PHY_CMD RTL8211EConfig[] = {
    {MII_WRITE_COMMAND(0x00, 0x3140), NULL},
    {MII_WRITE_COMMAND(0x00, 0x3340), NULL},
    {ENET_MII_END,                    NULL}
};

ENET_PHY_CMD RTL8211EStartup[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C},       /* Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),        EnetParse_MII_ANA},
    {MII_READ_COMMAND(MII_REG_1000BASETCR), EnetParse_MII_ANA_1GB},
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD RTL8211EActint[] = {
    {MII_READ_COMMAND(MII_REG_SR),            EnetParse_MII_S},
    {MII_READ_COMMAND(MII_REG_ANLPAR),        EnetParse_MII_LPA},
    {MII_READ_COMMAND(MII_REG_1000BASETST),   EnetParse_MII_LPA_1GB},
    #if DBG 
    {MII_READ_COMMAND(MII_REG_AR8035_PHY_SS), EnetParse_MII_SS},
    #endif 
    {ENET_MII_END,                            NULL}
};

MP_PHY_INFO RTL8211EInfo = {
    ENET_PHY_RTL8211E,
    TEXT("RTL8211E"),
    RTL8211EConfig,
    RTL8211EStartup,
    RTL8211EActint,
    NULL
};

// RTL8211F Phy

ENET_PHY_CMD RTL8211FConfig[] = {
    {MII_WRITE_COMMAND(0x00, 0x3140), NULL},
    {MII_WRITE_COMMAND(0x00, 0x3340), NULL},
    {ENET_MII_END,                    NULL}
};

ENET_PHY_CMD RTL8211FStartup[] = {
    {MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C},       /* Enable and Restart Auto-Negotiation */
    {MII_READ_COMMAND(MII_REG_ANAR),        EnetParse_MII_ANA},
    {MII_READ_COMMAND(MII_REG_1000BASETCR), EnetParse_MII_ANA_1GB},
    {ENET_MII_END,                          NULL}
};

ENET_PHY_CMD RTL8211FActint[] = {
    {MII_READ_COMMAND(MII_REG_SR),            EnetParse_MII_S},
    {MII_READ_COMMAND(MII_REG_ANLPAR),        EnetParse_MII_LPA},
    {MII_READ_COMMAND(MII_REG_1000BASETST),   EnetParse_MII_LPA_1GB},
    {ENET_MII_END,                            NULL}
};

MP_PHY_INFO RTL8211FInfo = {
    ENET_PHY_RTL8211F,
    TEXT("RTL8211F"),
    RTL8211FConfig,
    RTL8211FStartup,
    RTL8211FActint,
    NULL
};

// KSZ9031 Phy

ENET_PHY_CMD KSZ9031Config[] = {
    /* control data pad skew - devaddr = 0x02, register = 0x04 */
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x2), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x4), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x4002), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x0), NULL },
    /* rx data pad skew - devaddr = 0x02, register = 0x05 */
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x2), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x5), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x4002), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x0), NULL },
    /* tx data pad skew - devaddr = 0x02, register = 0x06 */
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x2), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x6), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x4002), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x0), NULL },
    /* gtx and rx clock pad skew - devaddr = 0x02, register = 0x08 */
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x2), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x8), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_CR, 0x4002), NULL },
    { MII_WRITE_COMMAND(MII_REG_MMD_RD, 0x03ff), NULL },
    { ENET_MII_END, NULL }
};

ENET_PHY_CMD KSZ9031Startup[] = {
    { MII_WRITE_COMMAND(MII_REG_CR, 0x1200), EnetParse_MII_C },       /* Enable and Restart Auto-Negotiation */
    { MII_READ_COMMAND(MII_REG_ANAR),        EnetParse_MII_ANA },
    { MII_READ_COMMAND(MII_REG_1000BASETCR), EnetParse_MII_ANA_1GB },
    { ENET_MII_END, NULL }
};

ENET_PHY_CMD KSZ9031Actint[] = {
    { MII_READ_COMMAND(MII_REG_SR), EnetParse_MII_S },
    { MII_READ_COMMAND(MII_REG_ANLPAR), EnetParse_MII_LPA },
    { MII_READ_COMMAND(MII_REG_1000BASETST), EnetParse_MII_LPA_1GB },
    { ENET_MII_END, NULL }
};

MP_PHY_INFO KSZ9031Info = {
    ENET_PHY_KSZ9031,
    TEXT("KSZ9031"),
    KSZ9031Config,
    KSZ9031Startup,
    KSZ9031Actint,
    NULL
};

// if other PHY(s) need to be supported, add them to this array
MP_PHY_INFO *PhyInfo[] = {
    &AR8031Info,
    &AR8035Info,
    &RTL8211EInfo,
    &RTL8211FInfo,
    &KSZ8081Info,
    &KSZ8091Info,
    &KSZ9021Info,
    &KSZ9031Info,
    NULL
};

#if DBG

/*++
Routine Description:
    Get MDIO device frame callback name
Argument:
    RequestType  Callback address
Return Value:
    Pointer to MII command calback name
--*/
_Use_decl_annotations_
const char* Dbg_GetMIICallbackName(void * FunctionAddress) {
#undef MAKEIF
#define MAKEIF(FnName) if (FunctionAddress == FnName)  return #FnName;
    MAKEIF(EnetParse_MII_S)
        MAKEIF(EnetParse_MII_C)
        MAKEIF(EnetParse_MII_ANA)
        MAKEIF(EnetParse_MII_ANA_1GB)
        MAKEIF(EnetParse_MII_LPA)
        MAKEIF(EnetParse_MII_LPA_1GB)
        MAKEIF(EnetPauseOff)
        MAKEIF(EnetPauseOn)
        MAKEIF(EnetParsePHYLink)
        MAKEIF(MDIODev_SavePHYID1)
        MAKEIF(MDIODev_SavePHYID2)
        MAKEIF(EnetParse_MII_SS)
        MAKEIF(NULL)
        MAKEIF(EnetDispPHYCfg)
        return "MII CMD FUNCTION name unknown";
}

/*++
Routine Description:
    This function parses the 1000 BASE-T Control Register data of the
    external MII compatible PHY(s).
Arguments:
    RegVal
        The 1000 BASE-T Control Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetParse_MII_BASET(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_PHY_DEVICE*   pPHYDev = &((PMP_ADAPTER)MiniportAdapterHandle)->ENETDev_PHYDevice;
    UNREFERENCED_PARAMETER(RegVal);
    UNREFERENCED_PARAMETER(pPHYDev);
    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_BASET (Offset 0x%02X) Reg Value 0x%04X", 9, (UINT16)RegVal);
    DBG_PHY_DEV_METHOD_BEG();
}

/*++
Routine Description:
    This function parses the PHY-Specific Status Register data of the
    external MII compatible PHY(s).
Arguments:
    RegVal
        The PHY-Specific Status Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetParse_MII_SS(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_PHY_DEVICE*   pPHYDev = &((PMP_ADAPTER)MiniportAdapterHandle)->ENETDev_PHYDevice;

    UNREFERENCED_PARAMETER(RegVal);
    UNREFERENCED_PARAMETER(pPHYDev);

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_SS (Offset 0x%02X) Reg Value 0x%04X", 17, (UINT16)RegVal);
    DBG_PHY_DEV_METHOD_BEG();
}

#endif
/*++
Routine Description:
    This function parses the Control register (Offset 0x00) data of the external MII compatible PHY(s).
Arguments:
    RegVal
        The Control Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetParse_MII_C(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_PHY_DEVICE*   pPHYDev = &((PMP_ADAPTER)MiniportAdapterHandle)->ENETDev_PHYDevice;
    UINT             Status;
    volatile UINT*   s;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_C (Offset 0x%02X) Reg Value 0x%04X", MII_REG_CR, (UINT16)RegVal);
    s = &(pPHYDev->PHYDev_PhyConfig);
    Status = *s & ~(PHY_CONF_ANE | PHY_CONF_LOOP);
    if (RegVal & MII_REG_CR_AN_ENABLE)           // Is auto-negotiation enabled?
        Status |= PHY_CONF_ANE;
    if (RegVal & MII_REG_CR_LOOPBACK)            // Is loop-back enabled?
        Status |= PHY_CONF_LOOP;
    *s = Status;
    DBG_PHY_DEV_METHOD_BEG();
}

/*++
Routine Description:
    This function parses the Auto-Negotiation Advertisement Register data of the external MII compatible PHY(s).
Arguments:
    RegVal
        The Auto-Negotiation Advertisement Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetParse_MII_ANA(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_PHY_DEVICE*   pPHYDev = &((PMP_ADAPTER)MiniportAdapterHandle)->ENETDev_PHYDevice;
    UINT             Status;
    volatile UINT*   s;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_ANA (Offset 0x%02X) Reg Value 0x%04X", MII_REG_ANAR, (UINT16)RegVal);
    s = &(pPHYDev->PHYDev_PhyConfig);
    Status = *s & ~(PHY_CONF_SPMASK);
    if (RegVal & MII_REG_ANAR_10BASETX_HD)
        Status |= PHY_CONF_10HDX;
    if (RegVal & MII_REG_ANAR_10BASETX_FD)
        Status |= PHY_CONF_10FDX;
    if (RegVal & MII_REG_ANAR_100BASETX_HD)
        Status |= PHY_CONF_100HDX;
    if (RegVal & MII_REG_ANAR_100BASETX_FD)
        Status |= PHY_CONF_100FDX;
    *s = Status;
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    This function parses the 1000 BASE-T Auto-Negotiation Advertisement Register data of the
    external MII compatible PHY(s).
Arguments:
    RegVal
        The Auto-Negotiation Advertisement Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetParse_MII_ANA_1GB(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_PHY_DEVICE*   pPHYDev = &((PMP_ADAPTER)MiniportAdapterHandle)->ENETDev_PHYDevice;
    UINT             Status;
    volatile UINT*   s;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_ANA_1GB (Offset 0x%02X) Reg Value 0x%04X", MII_REG_1000BASETCR, (UINT16)RegVal);
    s = &(pPHYDev->PHYDev_PhyConfig);
    Status = *s & ~(PHY_CONF_SPMASK);
    if (RegVal & MII_REG_1000BASETCR_1000BT_FD)
        Status |= PHY_CONF_1000FDX;
    if (RegVal & MII_REG_1000BASETCR_1000BT_HD)
        Status |= PHY_CONF_1000HDX;
    *s = Status;
    DBG_PHY_DEV_METHOD_END();
}
/*
ECR_SPEED - Selects between 10/100-Mbit/s and 1000-Mbit/s modes of operation.
    0 - 10/100-Mbit/s mode
    1 - 1000-Mbit/s mode

RCR_RMII_MODE - RMII Mode Enable
    Specifies whether the MAC is configured for MII mode or RMII operation , when ECR_SPEED is cleared .
    0 - MAC configured for MII mode.
    1 - MAC configured for RMII operation.
RCR_RMII_10T - Enables 10-Mbit/s mode of the RMII or RGMII.
    0 - 100-Mbit/s operation.
    1 - 10-Mbit/s operation.

RCR_DRT - Disable Receive On Transmit
    0 - Receive path operates independently of transmit. Used for full-duplex or to monitor transmit activity in  half-duplex mode.
    1 - Disable reception of frames while transmitting. Normally used for half-duplex mode.
TCR_FDEN - Full-Duplex Enable
    If this field is set, frames transmit independent of carrier sense and collision inputs.
    NOTE: Only modify this bit when ECR[ETHEREN] is cleared.
*/

/*++
Routine Description:
    Converts the flags from MII compatible PHY(s) Status register (offset 0x01) to unified miniport MP_PHY_STATUS register
    Arguments:
    RegVal
        Status register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
Called by MpSmDispatcher()->PHYDev_UpdateLinkStatus()                            - PHYCmdLink(EnetParse_MII_S,EnetParsePHYLink)
                          ->PHYDev_UpdateLinkStatus(!MediaConnectStateConnected) - Actint(EnetParse_MII_S,EnetParse_MII_LPA,EnetParse_MII_LPA_1GB)
          MpInitializeEx()->PHYDev_ConfigurePhy()                                - Actint(EnetParse_MII_S,EnetParse_MII_LPA,EnetParse_MII_LPA_1GB)
--*/
_Use_decl_annotations_
void EnetParse_MII_S(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_PHY_DEVICE*          pPHYDev = &((PMP_ADAPTER)MiniportAdapterHandle)->ENETDev_PHYDevice;
    MP_PHY_STATUS           Status;
    volatile MP_PHY_STATUS  s;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_S (Offset 0x%02X) Reg value: 0x%04X (Link: %s, Remote fault: %s, AutoNego: %s)", MII_REG_SR, (UINT16)RegVal, \
        (RegVal & MII_REG_SR_LINKSTATUS)? "UP":"DOWN", (RegVal & MII_REG_SR_REMOTEFAULT)? "yes":"no", (RegVal & MII_REG_SR_AN_COMPLETE)? "Done":"Pending");
    s = pPHYDev->PHYDev_PhyStatus;
    Status.U = pPHYDev->PHYDev_PhyStatus.U & ~(MP_PHY_STATUS_LINK_DETECTED_MASK | MP_PHY_STATUS_REMOTE_FAULT_MASK | MP_PHY_STATUS_AN_COMPLETE_MASK);
    if (RegVal & MII_REG_SR_LINKSTATUS)
        Status.B.LINK_DETECTED = 1;
    if (RegVal & MII_REG_SR_REMOTEFAULT)
        Status.B.REMOTE_FAULT = 1;
    if (RegVal & MII_REG_SR_AN_COMPLETE)
        Status.B.AN_COMPLETE = 1;
    pPHYDev->PHYDev_PhyStatus = Status;
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    Parses the Auto-Negotiation Link Partner Ability Register data of the external MII compatible PHY(s).
Arguments:
    RegVal
        Auto-Negotiation Advertisement Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
Called by MpSmDispatcher()->PHYDev_UpdateLinkStatus(!MediaConnectStateConnected) - Actint(EnetParse_MII_S,EnetParse_MII_LPA,EnetParse_MII_LPA_1GB)
          MpInitializeEx()->PHYDev_ConfigurePhy()                             - Actint(EnetParse_MII_S,EnetParse_MII_LPA,EnetParse_MII_LPA_1GB)

--*/
_Use_decl_annotations_
void EnetParse_MII_LPA(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_ADAPTER*     pAdapter          = ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_MDIO_DEVICE* pMDIODev = &pAdapter->ENETDev_MDIODevice;
    MP_PHY_DEVICE*  pPHYDev =  &pAdapter->ENETDev_PHYDevice;
    BOOLEAN         bAutoNegSuccess;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_LPA (Offset 0x%02X) Reg Value 0x%04X", MII_REG_ANLPAR, (UINT16)RegVal);
    bAutoNegSuccess = pPHYDev->PHYDev_PhyStatus.B.AN_COMPLETE == 1;

    if (bAutoNegSuccess && pAdapter->SpeedSelect == SPEED_AUTO) {
        if (RegVal & MII_REG_ANLPAR_PAUSE) {
            // Link partner supports pause frames
            if (pPHYDev->PHYDev_LPApause == PHY_LPA_1GB_ON_WAIT) {
                pPHYDev->PHYDev_LPApause = PHY_LPA_1GB_ON;
            }
            if ((pPHYDev->PHYDev_LPApause == PHY_LPA_INIT) || (pPHYDev->PHYDev_LPApause == PHY_LPA_1GB_OFF_WAIT)) {
                pPHYDev->PHYDev_LPApause = PHY_LPA_PAUSE_DETECTED;
               MDIODev_QueueCommand(pMDIODev, PHYCmd1000On);            // Switch on 1000BASE-T support, call EnetPauseOn
            }
        } else {
            // Link partner does not support pause frames
            if (pPHYDev->PHYDev_LPApause == PHY_LPA_1GB_OFF_WAIT) {
                pPHYDev->PHYDev_LPApause = PHY_LPA_1GB_OFF;
            }
            if ((pPHYDev->PHYDev_LPApause == PHY_LPA_INIT) || (pPHYDev->PHYDev_LPApause == PHY_LPA_1GB_ON_WAIT)) {
                pPHYDev->PHYDev_LPApause = PHY_LPA_PAUSE_NOT_DETECTED;
               MDIODev_QueueCommand(pMDIODev, PHYCmd1000Off);           // Switch off 1000BASE-T support, call EnetPauseOff
            }
        }
        if (RegVal & MII_REG_ANLPAR_10BASETX_HD) {
            DBG_PHY_DEV_PRINT_INFO("Link Partner is  10 Mbps Half Duplex capable");
            pPHYDev->PHYDev_DuplexMode = MediaDuplexStateHalf;
            pPHYDev->PHYDev_LinkSpeed = SPEED_10MBPS;
        } if (RegVal & MII_REG_ANLPAR_10BASETX_FD) {
            DBG_PHY_DEV_PRINT_INFO("Link Partner is  10 Mbps Full Duplex capable");
            pPHYDev->PHYDev_DuplexMode = MediaDuplexStateFull;
            pPHYDev->PHYDev_LinkSpeed = SPEED_10MBPS;
        } if (RegVal & MII_REG_ANLPAR_100BASETX_HD) {
            DBG_PHY_DEV_PRINT_INFO("Link Partner is 100 Mbps Half Duplex capable");
            pPHYDev->PHYDev_DuplexMode = MediaDuplexStateHalf;
            pPHYDev->PHYDev_LinkSpeed = SPEED_100MBPS;
        } if (RegVal & MII_REG_ANLPAR_100BASETX_FD) {
            DBG_PHY_DEV_PRINT_INFO("Link Partner is 100 Mbps Full Duplex capable");
            pPHYDev->PHYDev_DuplexMode = MediaDuplexStateFull;
            pPHYDev->PHYDev_LinkSpeed = SPEED_100MBPS;
        }
    }
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    Parses the Auto-Negotiation 1000 Base-T status of the external MII compatible PHY(s).
Arguments:
    RegVal
        1000 Base-T status Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
Called by MpSmDispatcher()->PHYDev_UpdateLinkStatus(!MediaConnectStateConnected) - Actint(EnetParse_MII_S,EnetParse_MII_LPA,EnetParse_MII_LPA_1GB)
          MpInitializeEx()->PHYDev_ConfigurePhy()                             - Actint(EnetParse_MII_S,EnetParse_MII_LPA,EnetParse_MII_LPA_1GB)
--*/
_Use_decl_annotations_
void EnetParse_MII_LPA_1GB(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    BOOLEAN        bAutoNegSuccess;
    MP_ADAPTER*    pAdapter = ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_PHY_DEVICE* pPHYDev = &pAdapter->ENETDev_PHYDevice;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_LPA_1GB (Offset 0x%02X) Reg Value 0x%04X", MII_REG_1000BASETST, (UINT16)RegVal);
    bAutoNegSuccess = pPHYDev->PHYDev_PhyStatus.B.AN_COMPLETE == 1;

    if (bAutoNegSuccess && pAdapter->SpeedSelect == SPEED_AUTO) {
        if (pPHYDev->PHYDev_LPApause == PHY_LPA_1GB_ON) {
            if (RegVal & MII_REG_1000BASETST_LP1000HD) {
                DBG_PHY_DEV_PRINT_INFO("Link Partner is 1000 Mbps Half Duplex capable");
                pPHYDev->PHYDev_DuplexMode = MediaDuplexStateHalf;
                pPHYDev->PHYDev_LinkSpeed = SPEED_1000MBPS;
                pAdapter->ENETRegBase->ECR.U |= ENET_ECR_SPEED_MASK;
            }
            if (RegVal & MII_REG_1000BASETST_LP1000FD) {
                DBG_PHY_DEV_PRINT_INFO("Link Partner is 1000 Mbps Full Duplex capable");
                pPHYDev->PHYDev_DuplexMode = MediaDuplexStateFull;
                pPHYDev->PHYDev_LinkSpeed = SPEED_1000MBPS;
                pAdapter->ENETRegBase->ECR.U |= ENET_ECR_SPEED_MASK;
            }
        }
        if (pPHYDev->PHYDev_LinkSpeed == SPEED_10MBPS) {
            pAdapter->ENETRegBase->RCR.U |= ENET_RCR_RMII_10T_MASK;
        } else {
            pAdapter->ENETRegBase->RCR.U &= ~ENET_RCR_RMII_10T_MASK;
        }
        if (pPHYDev->PHYDev_DuplexMode == MediaDuplexStateHalf) {
            pAdapter->ENETRegBase->RCR.U |= ENET_RCR_DRT_MASK;
            pAdapter->ENETRegBase->TCR.U &= ~ENET_TCR_FEDN_MASK;
        } else {
            pAdapter->ENETRegBase->RCR.U &= ~ENET_RCR_DRT_MASK;
            pAdapter->ENETRegBase->TCR.U |= ENET_TCR_FEDN_MASK;
        }
    }
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    Parses the Link Partner Ability Register data of the external MII compatible PHY(s).
Arguments:
    RegVal
        Link Partner Ability Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetPauseOff(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_ADAPTER*     pAdapter = ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_PHY_DEVICE*  pPHYDev = &pAdapter->ENETDev_PHYDevice;

    UNREFERENCED_PARAMETER(RegVal);
    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("PHYCmd1000Off (Offset 0x%02X Reg Value 0x%04X)", MII_REG_1000BASETCR, (UINT16)RegVal);
    pPHYDev->PHYDev_LPApause = PHY_LPA_1GB_OFF_WAIT;
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    Parses the Link Partner Ability Register data of the external MII compatible PHY(s).
Arguments:
    RegVal
        Link Partner Ability Register value get from external MII compatible PHY(s)
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetPauseOn(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_ADAPTER*     pAdapter = ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_PHY_DEVICE*  pPHYDev = &pAdapter->ENETDev_PHYDevice;

    UNREFERENCED_PARAMETER(RegVal);
    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("PHYCmd1000On (Offset 0x%02X Reg Value 0x%04X)", MII_REG_1000BASETCR, (UINT16)RegVal);
    pPHYDev->PHYDev_LPApause = PHY_LPA_1GB_ON_WAIT;
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    This function will update the link status according to the Status register of the external PHY.
Arguments:
    RegVal
        The MII frame value which is read from external PHY registers
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by ENETInitialize.
Return Value:
    None
Called by MpSmDispatcher()->PHYDev_UpdateLinkStatus()->PHYCmdLink(EnetParse_MII_S,EnetParsePHYLink)
--*/
_Use_decl_annotations_
void EnetParsePHYLink(UINT RegVal, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_ADAPTER*     pAdapter = ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_PHY_DEVICE*  pPHYDev = &pAdapter->ENETDev_PHYDevice;

    UNREFERENCED_PARAMETER(RegVal);
    DBG_PHY_DEV_METHOD_BEG();
    NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
    if ((pPHYDev->PHYDev_LPApause == PHY_LPA_PAUSE_DETECTED) || (pPHYDev->PHYDev_LPApause == PHY_LPA_PAUSE_NOT_DETECTED)) {
        // do nothing
    } else {
        if (pPHYDev->PHYDev_PhyStatus.B.LINK_DETECTED) {
            pPHYDev->PHYDev_MediaConnectState = MediaConnectStateConnected;
            if (pAdapter->SpeedSelect == SPEED_FULL_DUPLEX_1G)
                pAdapter->ENETRegBase->ECR.U |= ENET_ECR_SPEED_MASK;   // Set gigabit speed
        } else {
            if ((pPHYDev->PHYDev_LPApause == PHY_LPA_1GB_ON_WAIT) || (pPHYDev->PHYDev_LPApause == PHY_LPA_1GB_OFF_WAIT)) {
                //
            } else {
                pPHYDev->PHYDev_LPApause = PHY_LPA_INIT;
            }
            pPHYDev->PHYDev_MediaConnectState = MediaConnectStateDisconnected;
            pAdapter->ENETRegBase->ECR.U &= ~ENET_ECR_SPEED_MASK;   //Clear gigabit speed mode so we can detect 100Mbps/10mbps links
        }
    }
    pPHYDev->PHYDev_MIISeqDone = TRUE;
    NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    This function will send the command to the external PHY to get the link status of the cable.
    The updated link status is stored in the context area designated by the parameter MiniportAdapterContext.
Arguments:
    MiniportAdapterContext
        Adapter context address
Return Value:
    None
--*/
_Use_decl_annotations_
void PHYDev_UpdateLinkStatus(NDIS_HANDLE MiniportAdapterHandle)
{
    MP_ADAPTER*     pAdapter= ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_MDIO_DEVICE* pMDIODev = &pAdapter->ENETDev_MDIODevice;
    MP_PHY_DEVICE*  pPHYDev  = &pAdapter->ENETDev_PHYDevice;

    DBG_PHY_DEV_METHOD_BEG();
    if (pPHYDev->PHYDev_MIISeqDone == FALSE) {
        return; // Previous command to the PHY is not done yet
    }
    if (pPHYDev->PHYDev_PhySettings) {
        pPHYDev->PHYDev_MIISeqDone = FALSE;
        if (pAdapter->MediaConnectState != MediaConnectStateConnected) {
            MDIODev_QueueCommand(pMDIODev, pPHYDev->PHYDev_PhySettings->PhyActint);
        }
        MDIODev_QueueCommand(pMDIODev, PHYCmdLink); // EnetParse_MII_S  EnetParsePHYLink - set MIISeqDone
    }
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    This function displays the current status of the external PHY
Arguments:
    MIIReg
        The MII frame value which is read from external PHY registers
    MiniportAdapterHandle
        Specifies the handle to the driver allocated context area in which the driver
        maintains ENET adapter state, set up by MpInitializeEx.
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetDispPHYCfg(_In_ UINT MIIReg, _In_ NDIS_HANDLE MiniportAdapterHandle)
{
    UINT           Status;
    MP_ADAPTER*    pAdapter = ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_PHY_DEVICE* pPHYDev = &pAdapter->ENETDev_PHYDevice;

    UNREFERENCED_PARAMETER(MIIReg);
    Status = pPHYDev->PHYDev_PhyConfig;
    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("PHYId2 Reg Value 0x%04X", Status);
    if (Status & PHY_CONF_ANE)
        DBG_PHY_DEV_PRINT_INFO("Auto-negotiation is on");
    else
        DBG_PHY_DEV_PRINT_INFO("Auto-negotiation is off");
    if (Status & PHY_CONF_1000FDX)
        DBG_PHY_DEV_PRINT_INFO("Report   1G Full Duplex Mode capability");
    if (Status & PHY_CONF_1000HDX)
        DBG_PHY_DEV_PRINT_INFO("Report   1G Half Duplex Mode capability");
    if (Status & PHY_CONF_100FDX)
        DBG_PHY_DEV_PRINT_INFO("Report 100M Full Duplex Mode capability");
    if (Status & PHY_CONF_100HDX)
        DBG_PHY_DEV_PRINT_INFO("Report 100M Half Duplex Mode capability");
    if (Status & PHY_CONF_10FDX)
        DBG_PHY_DEV_PRINT_INFO("Report  10M Full Duplex Mode capability");
    if (Status & PHY_CONF_10HDX)
        DBG_PHY_DEV_PRINT_INFO("Report  10M Half Duplex Mode capability");
    if (!(Status & PHY_CONF_SPMASK))
        DBG_PHY_DEV_PRINT_INFO("No speed/duplex selected");
    if (Status & PHY_CONF_LOOP)
        DBG_PHY_DEV_PRINT_INFO("Loop back mode is enabled");
    pPHYDev->PHYDev_MIISeqDone = TRUE;
    DBG_PHY_DEV_METHOD_END();
    return;
}

/*++
Routine Description:
    Initializes external PHY
Arguments:
    Adapter     Adapter context address
Return Value:
    None
--*/
_Use_decl_annotations_
BOOLEAN PHYDev_ConfigurePhy(NDIS_HANDLE MiniportAdapterHandle)
{
    MP_ADAPTER*     pAdapter = ((PMP_ADAPTER)(MiniportAdapterHandle));
    MP_PHY_DEVICE*  pPHYDev = &pAdapter->ENETDev_PHYDevice;
    MP_MDIO_DEVICE* pMDIODev = pPHYDev->PHYDev_pMDIODev;

    DBG_PHY_DEV_METHOD_BEG();
    // Wait for MII interrupt to set to TRUE
    if (pPHYDev->PHYDev_PhySettings) {
        // Set to auto-negotiation mode and restart the auto-negotiation process
        MDIODev_QueueCommand(pMDIODev, pPHYDev->PHYDev_PhySettings->PhyConfig);
        switch (pAdapter->SpeedSelect) {
            case SPEED_HALF_DUPLEX_10M:
                MDIODev_QueueCommand(pMDIODev, Start10HDX);
                pPHYDev->PHYDev_DuplexMode = MediaDuplexStateHalf;
                pPHYDev->PHYDev_LinkSpeed = SPEED_10MBPS;
                break;
            case SPEED_FULL_DUPLEX_10M:
                MDIODev_QueueCommand(pMDIODev, Start10FDX);
                pPHYDev->PHYDev_DuplexMode = MediaDuplexStateFull;
                pPHYDev->PHYDev_LinkSpeed = SPEED_10MBPS;
                break;
            case SPEED_HALF_DUPLEX_100M:
                MDIODev_QueueCommand(pMDIODev, Start100HDX);
                pPHYDev->PHYDev_DuplexMode = MediaDuplexStateHalf;
                pPHYDev->PHYDev_LinkSpeed = SPEED_100MBPS;
                break;
            case SPEED_FULL_DUPLEX_100M:
                MDIODev_QueueCommand(pMDIODev, Start100FDX);
                pPHYDev->PHYDev_DuplexMode = MediaDuplexStateFull;
                pPHYDev->PHYDev_LinkSpeed = SPEED_100MBPS;
                break;
            case SPEED_FULL_DUPLEX_1G:
                MDIODev_QueueCommand(pMDIODev, Start1000FDX);
                pPHYDev->PHYDev_DuplexMode = MediaDuplexStateFull;
                pPHYDev->PHYDev_LinkSpeed = SPEED_1000MBPS;
                break;
            default: // SPEED_AUTO
                MDIODev_QueueCommand(pMDIODev, pPHYDev->PHYDev_PhySettings->PhyStartup);
                break;
        }
        MDIODev_QueueCommand(pMDIODev, pPHYDev->PHYDev_PhySettings->PhyActint);
        MDIODev_QueueCommand(pMDIODev, PHYCmdCfg);
    } else {
        DBG_PHY_DEV_METHOD_END();
        return FALSE;
    }
    DBG_PHY_DEV_METHOD_END();
    return TRUE;
}

/*++
Routine Description:
    Scan all of the MII PHY addresses looking for someone to respond with a valid ID.
    This usually happens quickly.
Arguments:
    MIIReg
    The MII frame value which is read from external PHY registers
    MiniportAdapterHandle
    Specifies the handle to the driver allocated context area in which the driver
    maintains ENET adapter state, set up by MpInitializeEx.
Return Value:
    None
--*/
_Use_decl_annotations_
void MDIODev_SavePHYID1(UINT MIIReg, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_PHY_DEVICE* pPHYDev = &((PMP_ADAPTER)(MiniportAdapterHandle))->ENETDev_PHYDevice;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_PHY_ID (Offset 0x%02X) Reg Value 0x%04X", MII_REG_PHYIR1, (UINT16)MIIReg);
    pPHYDev->PHYDev_PhyId = (MIIReg & MMI_DATA_MASK) << 16;
    DBG_PHY_DEV_METHOD_END();
}
_Use_decl_annotations_
void MDIODev_SavePHYID2(UINT MIIReg, NDIS_HANDLE MiniportAdapterHandle)
{
    MP_MDIO_DEVICE* pMDIODev = &((PMP_ADAPTER)(MiniportAdapterHandle))->ENETDev_MDIODevice;
    MP_PHY_DEVICE*  pPHYDev = &((PMP_ADAPTER)(MiniportAdapterHandle))->ENETDev_PHYDevice;

    DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS("MII_PHY_ID2 (Offset 0x%02X) Reg Value 0x%04X", MII_REG_PHYIR2, (UINT16)MIIReg);
    pPHYDev->PHYDev_PhyId |= (MIIReg & MMI_DATA_MASK);
    KeSetEvent(&pMDIODev->MDIODev_CmdDoneEvent, 0, FALSE);
    DBG_PHY_DEV_METHOD_END();
}

/*++
Routine Description:
    Reads ENET PHY id from PHY device.
Arguments:
    pMDIODev - MDIO Enet PHY device data structure address.
Return Value:
    none
--*/
_Use_decl_annotations_
NTSTATUS PHYDev_GetPhyId(MP_PHY_DEVICE *pPHYDev) {
    NTSTATUS        Status;
    UINT            i;
    LARGE_INTEGER   TimeOut = { .QuadPart = -MDIO_GetPhyIdCmdTimout_ms * (10 * 1000) };
    MP_MDIO_DEVICE *pMDIODev = pPHYDev->PHYDev_pMDIODev;

    DBG_PHY_DEV_METHOD_BEG();
    do {
        if (!NT_SUCCESS(Status = MDIODev_QueueCommand(pMDIODev, PHYCmdGetPhyId))) {
            DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS("MDIODev_QueueCommand() failed.");
        }
        if ((Status = KeWaitForSingleObject(&pMDIODev->MDIODev_CmdDoneEvent, Executive, KernelMode, FALSE, &TimeOut)) != STATUS_SUCCESS) {
            DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS("KeWaitForMultipleObjects() failed.");
            break;
        }
        Status = NDIS_STATUS_FAILURE;
        #if DBG
        if (pPHYDev->PHYDev_PhyId == 0xFFFFFFFF) {
            DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS("Invalid Phy ID, probably bad PHY address provided.");
            break;
        }
        #endif
        for (i = 0; PhyInfo[i] != NULL; i++) {
            if (PhyInfo[i]->PhyId == pPHYDev->PHYDev_PhyId) {
                pPHYDev->PHYDev_PhySettings = PhyInfo[i];
                Status = NDIS_STATUS_SUCCESS;
                break;
            }
        }
        DBG_MDIO_DEV_CMD_PRINT_INFO("External PHY is %s", (NULL != PhyInfo[i]) ? PhyInfo[i]->PhyName : "not supported");
    } while (0);
    DBG_PHY_DEV_METHOD_END_WITH_STATUS(Status);
    return(Status);
}

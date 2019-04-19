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

#ifndef _MP_ENET_PHY_H
#define _MP_ENET_PHY_H

// values for PHY status

#define PHY_CONF_ANE        0x0001  /* 1 auto-negotiation enabled */
#define PHY_CONF_LOOP       0x0002  /* 1 loopback mode enabled */
#define PHY_CONF_SPMASK     0x00f0  /* mask for speed */
#define PHY_CONF_10HDX      0x0010  /* 10 Mbit half duplex supported */
#define PHY_CONF_10FDX      0x0020  /* 10 Mbit full duplex supported */
#define PHY_CONF_100HDX     0x0040  /* 100 Mbit half duplex supported */
#define PHY_CONF_100FDX     0x0080  /* 100 Mbit full duplex supported */
#define PHY_CONF_1000HDX    0x00A0  /* 1000 Mbit half duplex supported */
#define PHY_CONF_1000FDX    0x00C0  /* 1000 Mbit full duplex supported */

#define PHY_STAT_LINK       0x0001  /* 1 up - 0 down */
#define PHY_STAT_FAULT      0x0002  /* 1 remote fault */
#define PHY_STAT_ANC        0x0004  /* 1 auto-negotiation complete */
#define PHY_STAT_SPMASK     0x1f8   /* mask for speed */
#define PHY_STAT_10HDX      0x0008  /* 10 Mbit half duplex selected */
#define PHY_STAT_10FDX      0x0010  /* 10 Mbit full duplex selected */
#define PHY_STAT_100HDX     0x0020  /* 100 Mbit half duplex selected */
#define PHY_STAT_100FDX     0x0040  /* 100 Mbit full duplex selected */
#define PHY_STAT_1000HDX    0x0080  /* 1000 Mbit half duplex selected */
#define PHY_STAT_1000FDX    0x0100  /* 1000 Mbit full duplex selected */

/*
* MP_PHY_STATUS - Unified Miniport PHY status Register
*/
typedef union {
    UINT32 U;
    struct {
        unsigned LINK_DETECTED : 1; /* 1 = link detected */
        unsigned REMOTE_FAULT : 1; /* 1 = fault detected */
        unsigned AN_COMPLETE : 1; /* 1 = auto-negotiation process complete */
    } B;
} MP_PHY_STATUS;

#define MP_PHY_STATUS_LINK_DETECTED_MASK       0x00000001
#define MP_PHY_STATUS_REMOTE_FAULT_MASK        0x00000002
#define MP_PHY_STATUS_AN_COMPLETE_MASK         0x00000004

typedef enum {
    ENET_PHY_AR8031   = 0x004DD074,
    ENET_PHY_AR8035   = 0x004DD072,
    ENET_PHY_RTL8211E = 0x001cc915,
    ENET_PHY_RTL8211F = 0x001CC916,
    ENET_PHY_KSZ8091  = 0x00221560,
    ENET_PHY_KSZ9021  = 0x00221611,
    ENET_PHY_KSZ9031  = 0x00221622,
    ENET_PHY_KSZ8081  = 0x00221561
} MP_PHY_ID;

typedef enum {
    PHY_LPA_INIT = 0,                           // Unknown state
    PHY_LPA_PAUSE_DETECTED = 1,                 // Link partner supports pause frames
    PHY_LPA_PAUSE_NOT_DETECTED = 2,             // Link partner does not support pause frames
    PHY_LPA_1GB_ON_WAIT = 3,                    // 1000 BASE-T is ON, waiting for connect
    PHY_LPA_1GB_OFF_WAIT = 4,                   // 1000 BASE-T is OFF, waiting for connect
    PHY_LPA_1GB_ON = 5,                         // 1000 BASE-T is ON , connected
    PHY_LPA_1GB_OFF = 6                         // 1000 BASE-T is OFF, connected
} MP_PHY_LPA, *PMP_PHY_LPA;

typedef struct _MP_PHY_INFO {
    MP_PHY_ID                PhyId;
    TCHAR                    PhyName[16];
    PENET_PHY_CMD            PhyConfig;
    PENET_PHY_CMD            PhyStartup;
    PENET_PHY_CMD            PhyActint;
    PENET_PHY_CMD            PhyShutdown;
} MP_PHY_INFO, *PMP_PHY_INFO;

// The Enet Phy device driver structure
typedef struct _MP_PHY_DEVICE {
    MP_MDIO_DEVICE *         PHYDev_pMDIODev;
    MP_PHY_ID                PHYDev_PhyId;
    MP_PHY_INFO*             PHYDev_PhySettings;
    // Operational Speed & Mode
    NDIS_MEDIA_CONNECT_STATE PHYDev_MediaConnectState;
    NDIS_MEDIA_DUPLEX_STATE  PHYDev_DuplexMode;
    USHORT                   PHYDev_LinkSpeed;

    MP_PHY_STATUS            PHYDev_PhyStatus;
    UINT                     PHYDev_PhyConfig;
    BOOLEAN                  PHYDev_MIISeqDone;
    MP_PHY_LPA               PHYDev_LPApause;
    #if DBG
    char                     PHYDev_DeviceName[MAX_DEVICE_NAME + 1];         // MDIO bus device name.
    #endif
} MP_PHY_DEVICE, *PMP_PHY_DEVICE;


NTSTATUS PHYDev_GetPhyId(_In_ MP_PHY_DEVICE *pPHYDev);
BOOLEAN  PHYDev_ConfigurePhy(_In_ NDIS_HANDLE MiniportAdapterHandle);
void     PHYDev_UpdateLinkStatus(_In_ NDIS_HANDLE MiniportAdapterHandle);

#endif // _MP_ENET_PHY_H

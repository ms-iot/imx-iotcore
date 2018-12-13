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

#ifndef _MP_MDIO_H
#define _MP_MDIO_H

// Max device name length, used for debud support
#define MAX_DEVICE_NAME 32

#define MDIO_FRAME_LIST_SIZE        64
#define MDIO_TransferTimeout_ms      3
#define MDIO_GetPhyIdCmdTimout_ms  500

// MII read/write commands for the external PHY

#define MII_READ_COMMAND(reg)           BIT_FIELD_VAL(ENET_MMFR_ST, ENET_MMFR_ST_VALUE)|\
                                        BIT_FIELD_VAL(ENET_MMFR_OP, ENET_MMFR_OP_READ)|\
                                        BIT_FIELD_VAL(ENET_MMFR_TA, ENET_MMFR_TA_VALUE)|\
                                        BIT_FIELD_VAL(ENET_MMFR_RA, reg & 0x1F)|\
                                        BIT_FIELD_VAL(ENET_MMFR_DATA, 0x0 & 0xFFFF)

#define MII_WRITE_COMMAND(reg, val)     BIT_FIELD_VAL(ENET_MMFR_ST, ENET_MMFR_ST_VALUE)|\
                                        BIT_FIELD_VAL(ENET_MMFR_OP, ENET_MMFR_OP_WRITE)|\
                                        BIT_FIELD_VAL(ENET_MMFR_TA, ENET_MMFR_TA_VALUE)|\
                                        BIT_FIELD_VAL(ENET_MMFR_RA, reg & 0x1F)|\
                                        BIT_FIELD_VAL(ENET_MMFR_DATA, val & 0xFFFF)

#define ENET_MII_END                    0

// MII management structure
typedef struct _MP_MDIO_FRAME {
    struct _MP_MDIO_FRAME*         MDIOFrm_pNextFrame;
    UINT32                         MDIOFrm_MMFRRegVal;
    void                           (*MIIFunction)(UINT32 RegVal, NDIS_HANDLE  MiniportAdapterHandle);
} MP_MDIO_FRAME, *PMP_MDIO_FRAME;

// The Enet Phy device interface enumeration
typedef enum _MP_MDIO_PHY_INTERFACE_TYPE {
    RGMII,
    RMII,
    MII
} MP_MDIO_PHY_INTERFACE_TYPE, *PMP_MDIO_PHY_INTERFACE_TYPE;

// The MDIO Enet Phy device configuration structure
typedef struct _MP_MDIO_ENET_PHY_CFG {
    NDIS_PHYSICAL_ADDRESS          MDIOCfg_RegsPhyAddress;                          // MDIO bus controller registers physical base address.
    LONG                           MDIOCfg_EnetPhyAddress;                          // MDIO ENET PHY device address on MDIO bus.
    MP_MDIO_PHY_INTERFACE_TYPE     MDIOCfg_PhyInterfaceType;                        // MDIO ENET PHY device interface type (MII/RMMI/RGMII)
    ULONG                          MDIOCfg_MDIOControllerInputClk_kHz;              // MDIO controller input clock frequency [kHz].
    ULONG                          MDIOCfg_MDCFreq_kHz;                             // Requested MDC clock freqency [kHz].
    ULONG                          MDIOCfg_STAHoldTime_ns;                          // Requested MDIO STA output hold time [ns].
    BOOLEAN                        MDIOCfg_DisableFramePreamble;                    // Enables/disables prepending a preamble to the MII management frame.
} MP_MDIO_ENET_PHY_CFG, *PMP_MDIO_ENET_PHY_CFG;

// The MDIO Enet Phy device driver structure. This structure should be part of MP_ADAPTER structure.
typedef struct _MP_MDIO_DEVICE {
    struct _MP_MDIO_DEVICE*        MDIODev_pNextDevice;                             // Next MDIO device address, NOTE must be the firts item of this structure.
    struct _MP_MDIO_BUS*           MDIODev_pBus;                                    // MDIO bus structure address.
    LONG                           MDIODev_EnetPhyAddress;                          // MDIO ENET PHY address.
    NDIS_SPIN_LOCK                 MDIODev_DeviceSpinLock;                          // MDIO device spin lock.
    MP_MDIO_FRAME                  MDIODev_FrameListArray[MDIO_FRAME_LIST_SIZE];    // MDIO frame item array.
    MP_MDIO_FRAME*                 MDIODev_pFreeFrameListHead;                      // MDIO free frame item list head.
    MP_MDIO_FRAME*                 MDIODev_pPendingFrameListHead;                   // MDIO pending frame item list head .
    MP_MDIO_FRAME*                 MDIODev_pPendingFrameListTail;                   // MDIO pending frame item list tail.
    struct _MP_ADAPTER*            MDIODev_pEnetAdapter;                            // ENET device data address.
    MSCR_t                         MDIODev_MSCR;                                    // ENET MSCR register value for this Phy device.
    MP_MDIO_PHY_INTERFACE_TYPE     MDIODev_PhyInterfaceType;
    KEVENT                         MDIODev_CmdDoneEvent;                            // Event used to signal command done event.
    #if DBG
    char                           MDIODev_DeviceName[MAX_DEVICE_NAME + 1];         // MDIO device name.
    ULONG                          MDIODev_Frequency_kHz;                           // MDIO device clock ferquency.
    ULONG                          MDIODev_STAHoldTime_ns;                          // MDIO device hold time.
    BOOLEAN                        MDIODev_DisableFramePreamble;                    // MDIO device preamble enable/disable state.
    #endif
} MP_MDIO_DEVICE, *PMP_MDIO_DEVICE;

// The MDIO bus structure. This structure is allocated by MDIOBus_Open() method for each MDIO bus.
typedef struct _MP_MDIO_BUS {
    struct _MP_MDIO_BUS*           MDIOBus_pNextBus;                                // Next MDIO bus driver address, NOTE must be the firts item of this structure.
    NDIS_SPIN_LOCK                 MDIOBus_BusSpinLock;                             // MDIO bus spin lock.
    // Transfer varaibles
    MP_MDIO_DEVICE*                MDIOBus_pFirstDevice;                            // MDIO device list head.
    MP_MDIO_DEVICE*                MDIOBus_pActiveDevice;                           // MDIO device with active frame.
    BOOLEAN                        MDIOBus_TransferInProgress;
    // State machine variables
    KTIMER                         MDIOBus_Timer;                                   // MDIO bus timer.
    KEVENT                         MDIOBus_KillEvent;                               // Event used to stop MDIO bus.
    KEVENT                         MDIOBus_StartTransferEvent;                      // Event used to singnal transfer ready event.
    PKTHREAD                       MDIOBus_ThreadHandle;                            // MDIO bus thread handle.
    // HW variables
    volatile CSP_ENET_REGS*        MDIOBus_pRegBase;                                // MDIO bus controller registers virtual base address.
    NDIS_PHYSICAL_ADDRESS          MDIOBus_RegsPhyAddress;                          // MDIO bus controller registers physical base address.
    volatile LONG                  MDIOBus_ReferenceCount;
    struct _MP_MDIO_DRIVER        *MDIOBus_pMDIODriver;                             // ENET driver context data address.
    #if DBG
    char                           MDIOBus_DeviceName[MAX_DEVICE_NAME + 1];         // MDIO bus device name.
    #endif
} MP_MDIO_BUS, *PMP_MDIO_BUS;

// The MDIO driver structure. This structure is allocated by DriverEntry() method.
typedef struct _MP_MDIO_DRIVER {
    NDIS_SPIN_LOCK                 MDIODrv_DriverSpinLock;                          // MDIO driver spin lock.
    volatile long                  MDIODrv_EnetDeviceCount;                         // Number of Enet peripheral instances handles by this driver.
    MP_MDIO_BUS*                   MDIODrv_pFirstBus;                               // MDIO bus controller list head.
    #if DBG
    ULONG                          MDIODrv_BusCounter;
    char                           MDIODrv_DeviceName[MAX_DEVICE_NAME + 1];         // MDIO driver name.
#endif
} MP_MDIO_DRIVER, *PMP_MDIO_DRIVER;

// The MDIO device command structure.
typedef struct _ENET_PHY_CMD {
    UINT32 MIIData;
    void(*MIIFunct)(UINT RegVal, NDIS_HANDLE  MiniportAdapterHandle);
} ENET_PHY_CMD, *PENET_PHY_CMD;

NTSTATUS MDIODev_InitDevice  (_In_ MP_MDIO_DRIVER *pMDIODrv, _In_ MP_MDIO_ENET_PHY_CFG *pEnetPhyConfig, _In_ MP_MDIO_DEVICE *pMDIODev);
void     MDIODev_DeinitDevice(_In_ MP_MDIO_DEVICE *pMDIODev);
NTSTATUS MDIODev_QueueCommand(_In_ MP_MDIO_DEVICE *pMDIODev, _In_ PENET_PHY_CMD pCmd);

#endif // _MP_MDIO_H
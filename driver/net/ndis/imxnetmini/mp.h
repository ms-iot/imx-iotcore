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

#ifndef _MP_H
#define _MP_H

typedef struct _MP_ADAPTER           MP_ADAPTER,           *PMP_ADAPTER;
typedef struct _MP_STATE_MACHINE     MP_STATE_MACHINE,     *PMP_STATE_MACHINE;
typedef struct _MP_MII_STATE_MACHINE MP_MII_STATE_MACHINE, *PMP_MII_STATE_MACHINE;

// multicast list size
#define NIC_MAX_MCAST_LIST              32

// update the driver version number every time you release a new driver
// The high word is the major version. The low word is the minor version.
// let's start with 1.0 for NDIS 6.30 driver
#define NIC_MAJOR_DRIVER_VERSION        0x01
#define NIC_MINOR_DRIVER_VERISON        0x01

// update the driver version number every time you release a new driver
// The high word is the major version. The low word is the minor version.
// this should be the same as the version reported in miniport driver characteristics

#define NIC_VENDOR_DRIVER_VERSION       ((NIC_MAJOR_DRIVER_VERSION << 16) | NIC_MINOR_DRIVER_VERISON)

// NDIS version in use by the NIC driver.
// The high byte is the major version. The low byte is the minor version.
#define NIC_DRIVER_VERSION              0x0600

// media type, we use ethernet, change if necessary
#define NIC_MEDIA_TYPE                  NdisMedium802_3

// maximum link speed for send and receive in bps
#define NIC_MEDIA_MAX_SPEED             1000000000

// interface type
#define NIC_INTERFACE_TYPE              NdisInterfaceInternal

#define NIC_VENDOR_DESC                 "NXP"

// supported filters
#define NIC_SUPPORTED_FILTERS (     \
    NDIS_PACKET_TYPE_DIRECTED       | \
    NDIS_PACKET_TYPE_MULTICAST      | \
    NDIS_PACKET_TYPE_BROADCAST      | \
    NDIS_PACKET_TYPE_PROMISCUOUS)
// Threshold for a remove
#define NIC_HARDWARE_ERROR_THRESHOLD    8

// NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION
#define ERRLOG_INVALID_SPEED_DUPLEX     0x00000301L
#define ERRLOG_SET_SECONDARY_FAILED     0x00000302L

// NDIS_ERROR_CODE_OUT_OF_RESOURCES
#define ERRLOG_OUT_OF_MEMORY            0x00000401L
#define ERRLOG_OUT_OF_SHARED_MEMORY     0x00000402L
#define ERRLOG_OUT_OF_BUFFER_POOL       0x00000404L
#define ERRLOG_OUT_OF_NDIS_BUFFER       0x00000405L
#define ERRLOG_OUT_OF_PACKET_POOL       0x00000406L
#define ERRLOG_OUT_OF_NDIS_PACKET       0x00000407L
#define ERRLOG_OUT_OF_LOOKASIDE_MEMORY  0x00000408L
#define ERRLOG_OUT_OF_SG_RESOURCES      0x00000409L

// NDIS_ERROR_CODE_HARDWARE_FAILURE
#define ERRLOG_SELFTEST_FAILED          0x00000501L
#define ERRLOG_INITIALIZE_ADAPTER       0x00000502L
#define ERRLOG_REMOVE_MINIPORT          0x00000503L

// NDIS_ERROR_CODE_RESOURCE_CONFLICT
#define ERRLOG_MAP_IO_SPACE             0x00000601L
#define ERRLOG_QUERY_ADAPTER_RESOURCES  0x00000602L
#define ERRLOG_NO_IO_RESOURCE           0x00000603L
#define ERRLOG_NO_INTERRUPT_RESOURCE    0x00000604L
#define ERRLOG_NO_MEMORY_RESOURCE       0x00000605L

#ifndef MIN
    #define MIN(a, b)   ((a) > (b) ? b: a)
#endif

#define ETH_IS_LOCALLY_ADMINISTERED(Address)  (BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x02))

#define MP_ALIGNMEM(_p, _align) (((_align) == 0) ? (_p) : (PUCHAR)(((ULONG_PTR)(_p) + ((_align)-1)) & (~((ULONG_PTR)(_align)-1))))
#define MP_ALIGNMEM_PA(_p, _align) (((_align) == 0) ?  (_p).QuadPart : (((_p).QuadPart + ((_align)-1)) & (~((ULONGLONG)(_align)-1))))

// Memory tags for this driver
#define MP_TAG_DRV                      ((ULONG)'DecF')
#define MP_TAG_MDIO_DRV                 ((ULONG)'MecF')
#define MP_TAG_ACPI                     ((ULONG)'AecF')
#define MP_TAG_WAKEUP                   ((ULONG)'WecF')
#define MP_TAG_TX_NBL_AND_NB            ((ULONG)'NecF')
#define MP_TAG_TX_BD                    ((ULONG)'BecF')
#define MP_TAG_RX_PAYLOAD_DESC          ((ULONG)'RceF')
#define MP_TAG_RX_ADAPTER               ((ULONG)'AceF')
#define MP_TAG_RX_INT_FIFO_DESC         ((ULONG)'IceF')
#define MP_TAG_SM_TIMER_DESC            ((ULONG)'TceF')

//--------------------------------------
// Configuration
//--------------------------------------
typedef struct _MP_REG_VALUE_DESC
{
    NDIS_STRING ValueName;
    UINT        ParamOffset;            // offset to MP_ADAPTER field
    UINT        ParamSize;              // size (in bytes) of the field
    UINT        Default;
    UINT        Min;
    UINT        Max;
} MP_REG_VALUE_DESC, *PMP_REG_VALUE_DESC;

//--------------------------------------
// Structure for pended OIS query request
//--------------------------------------
typedef struct _MP_QUERY_REQUEST {
    IN NDIS_OID  Oid;
    IN PVOID     InformationBuffer;
    IN ULONG     InformationBufferLength;
    OUT PULONG   BytesWritten;
    OUT PULONG   BytesNeeded;
} MP_QUERY_REQUEST, *PMP_QUERY_REQUEST;

//--------------------------------------
// Structure for pended OIS set request
//--------------------------------------
typedef struct _MP_SET_REQUEST {
    IN NDIS_OID  Oid;
    IN PVOID     InformationBuffer;
    IN ULONG     InformationBufferLength;
    OUT PULONG   BytesRead;
    OUT PULONG   BytesNeeded;
} MP_SET_REQUEST, *PMP_SET_REQUEST;

//--------------------------------------
// Structure for Power Management Info
//--------------------------------------
typedef struct _MP_POWER_MGMT {
    LIST_ENTRY              PatternList;      // List of Wake Up Patterns
    UINT                    OutstandingRecv;  // Number of outstanding Rcv NetBufferLists.
    UINT                    PowerState;       // Current Power state of the adapter
    BOOLEAN                 PME_En;           // Is PME_En on this adapter
    BOOLEAN                 bWakeFromD0;      // Wake-up capabilities of the adapter
    BOOLEAN                 bWakeFromD1;
    BOOLEAN                 bWakeFromD2;
    BOOLEAN                 bWakeFromD3Hot;
    BOOLEAN                 bWakeFromD3Aux;
    BOOLEAN                 Pad[2];           // Pad

} MP_POWER_MGMT, *PMP_POWER_MGMT;

typedef struct _MP_WAKE_PATTERN {
    LIST_ENTRY      linkListEntry;      // Link to the next Pattern
    ULONG           Signature;          // signature of the pattern
    ULONG           AllocationSize;     // Size of this allocation
    UCHAR           Pattern[1];         // Pattern - This contains the NDIS_PM_PACKET_PATTERN
} MP_WAKE_PATTERN , *PMP_WAKE_PATTERN ;

//--------------------------------------
// Macros specific to miniport adapter structure
//--------------------------------------

#define MP_OFFSET(field)   ((UINT)FIELD_OFFSET(MP_ADAPTER,field))
#define MP_SIZE(field)     sizeof(((PMP_ADAPTER)0)->field)

// A generic queue
typedef struct _MP_QUEUE {
    NDIS_SPIN_LOCK  Lock;       // Sync object
    LIST_ENTRY      Head;       // The list head
    LONG            Count;      // Number of members
} MP_QUEUE, *PMP_QUEUE;

//--------------------------------------
// Miniport tx net buffer descriptor
//--------------------------------------
typedef struct _MP_TX_BD {
    LIST_ENTRY            Link;            // Queable
    PMP_ADAPTER           pAdapter;        // Adapter private data address
    PNET_BUFFER           pNB;             // NB address
    PNET_BUFFER_LIST      pNBL;            // MBL address
    LONG                  NBId;            // For debug only
    PSCATTER_GATHER_LIST  pSGList;         // The scatter gather list address
    SCATTER_GATHER_LIST   SGList;          // SG list passed to MpProcessSGList
} MP_TX_BD, *PMP_TX_BD;

// ------------------------------------------------------------------------------------------------
// The TX payload data buffer descriptor.
// ------------------------------------------------------------------------------------------------
typedef struct _MP_TX_PAYLOAD_BD {
    PMP_TX_BD               pMpBD;
    PMDL                    pMdl;
    PUCHAR                  pBuffer;
    NDIS_PHYSICAL_ADDRESS   BufferPa;
    ULONG                   BufferSize;
} MP_TX_PAYLOAD_BD, *PMP_TX_PAYLOAD_BD;

// ------------------------------------------------------------------------------------------------
// The RX frame buffer descriptor. Free RX payload buffers are maintained in a list.
// ------------------------------------------------------------------------------------------------
typedef struct _MP_RX_FRAME_BD {
    LIST_ENTRY              Link;
    PNET_BUFFER_LIST        pNBL;           //
    PMDL                    pMdl;           // Address of the MDL describing buffer
    PUCHAR                  pBuffer;        // Address of the buffer (in the context of miniport driver)
    NDIS_PHYSICAL_ADDRESS   BufferPa;       // Physical address of the buffer
} MP_RX_FRAME_BD, *PMP_RX_FRAME_BD;


typedef struct _MP_ENET_BD_SW_EXT_ {
    PMP_RX_FRAME_BD         pRxFrameBD;
} MP_ENET_BD_SW_EXT, *PMP_ENET_BD_SW_EXT;

// Next state delay periods...
#define MP_SM_NEXT_STATE_IMMEDIATELY                 -1
#define MP_SM_NEXT_STATE_SAMPLE_DEALY_MSEC          100
#define MP_SM_NEXT_STATE_NORMAL_DEALY_MSEC         1000
#define MP_SM_CABLE_CONNECTED_CHECK_PERIOD_MSEC    1000
#define MP_SM_WAIT_FOR_SM_EXIT_TIMEOUT_MSEC        2000

typedef union {
    UINT32  U;
    struct {
        unsigned OP_RESTART :  1;
        unsigned OP_POWER   :  1;
        unsigned OP_RESET   :  1;
        unsigned OP_PAUSING :  1;
        unsigned RSRVD_4_31 : 28;
    } B;
} MP_NDIS_PEND_OPS, *PMP_NDIS_PEND_OPS;

#define MP_NDIS_PEND_OP_RESTART_MASK   0x00000001
#define MP_NDIS_PEND_OP_POWER_MASK     0x00000002
#define MP_NDIS_PEND_OP_RESET_MASK     0x00000004

// Set next state caller modes
typedef enum {
    SM_CALLED_BY_NDIS,
    SM_CALLED_BY_DISPATCHER
} MP_SM_NEXT_STATE_CALLER_MODE;

_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS SmSetState(_In_ PMP_ADAPTER pAdapter, _In_ LONG State, _In_ LONG DelayMsec, _In_ MP_SM_NEXT_STATE_CALLER_MODE CallerMode);
BOOLEAN SmIsAdapterRunning(_In_ PMP_ADAPTER pAdapter);

typedef enum {
    SM_STATE_HALTED         = 0,
    SM_STATE_SHUTDOWN       = 1,
    SM_STATE_INITIALIZING   = 2,
    SM_STATE_PAUSED         = 3,
    SM_STATE_PAUSING        = 4,
    SM_STATE_RESTARTING     = 5,
    SM_STATE_RUNNING        = 6,
    SM_STATE_RESET          = 7,
    SM_STATE_ERROR          = 8
} MP_STATE, *PMP_STATE;

typedef VOID (MP_SM_STATE_HANDLER)(PMP_ADAPTER pAdapter);

//--------------------------------------
// The miniport adapter structure
//--------------------------------------
typedef struct _MP_ADAPTER {
    NDIS_HANDLE             AdapterHandle;     // Handle given by NDIS when the Adapter registered itself.
    struct _MP_STATE_MACHINE {
        NDIS_SPIN_LOCK      SM_SpinLock;              // State machine spin lock
        NDIS_HANDLE         SM_hTimer;                // State machine timer handle
        NDIS_EVENT          SM_EnetHaltedEvent;       // Enet miniport driver halted event, if signaled, no ISR/DPC are planned and all resources can be freed.
        MP_STATE            SM_PreviousState;
        MP_STATE            SM_CurrentState;
        MP_STATE            SM_NextState;
        MP_STATE            SM_OnResetPreviousState;  // State machine state at the moment when OnReset state handler is called.
    } StateMachine;
    volatile BOOLEAN        DpcQueued;                // Set to TRUE by ISR id DPC is queued
    BOOLEAN                 DpcRunning;               // Set to TRUE at the begin of DPC and to FALSE ad the and of DPC
    MP_NDIS_PEND_OPS        PendingNdisOperations;
    NDIS_STATUS             NdisStatus;
    BOOLEAN                 EnetStarted;              // Set to TRUE(data path is running) by EnetStart() and to FALSE in EnetStop() method. 
    BOOLEAN                 RestartEnetAfterResume;
    volatile CSP_ENET_REGS *ENETRegBase;              // ENET peripheral registers virtual base address
    UINT32                  InterruptFlags;
    UCHAR                   PermanentAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR                   CurrentAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR                   FecMacAddress[ETH_LENGTH_OF_ADDRESS];
    PDEVICE_OBJECT          Pdo;
    PDEVICE_OBJECT          Fdo;
    PDEVICE_OBJECT          NextDeviceObject;

    USHORT                  SpeedSelect;                           // Selected Speed Mode from registry
    USHORT                  TheMostPowefullSpeedAndDuplexMode;     // The most powerful speed and duplex mode for both partners are capable
    NDIS_HANDLE             Tx_DmaHandle;                          // Scatter/Gather DMA handle
    ULONG                   Tx_SGListSize;
    NPAGED_LOOKASIDE_LIST   Tx_MpTxBDLookasideList;                // Tx buffer descriptor lookaside list
    ULONG                   Tx_CheckForHangCounter;
    LONG                    Tx_PendingNBs;                         // Number of TX frames (NET_BUFFERs) that are owned by the miniport. Total number of queued TX frames and frames that are already setup for DMA transfers.
    MP_QUEUE                Tx_qMpOwnedBDs;                        // Pending (owned by driver) TX ethernet frames (NET_BUFFERs) queue
    MP_QUEUE                Tx_qDmaOwnedBDs;                       // In progress (owned by Enet DMA) TX ethernet frames (NET_BUFFERs) queue
    NDIS_SPIN_LOCK          Tx_SpinLock;                           // Tx path spin lock
    LONG                    Tx_EnetFreeBDCount;                    // Number of unused Enet Buffer Descriptors
    LONG                    Tx_EnetFreeBDIdx;                      // Index of the first free BD
    LONG                    Tx_EnetPendingBDIdx;                   // Index of first BD submitted to ENET DMA
    MP_TX_PAYLOAD_BD        Tx_EnetSwExtBDT[TX_DESC_COUNT_MAX];    // Table containing Sw related data for each Enet BD
    PUCHAR                  Tx_DataBuffer_Va;                      // TxBufAlloc points to the block of numTCB*NIC_PACKET_SIZE
    ULONG                   Tx_DataBuffer_Size;                    // numTCB*NIC_PACKET_SIZE
    NDIS_PHYSICAL_ADDRESS   Tx_DataBuffer_Pa;

    volatile ENET_BD       *Tx_DmaBDT;                             // ENET peripheral Tx Dma buffer descriptor table (BDT) address
    LONG                    Tx_DmaBDT_ItemCount;                   // ENET peripheral Tx Dma buffer descriptor table (BDT) item count
    ULONG                   Tx_DmaBDT_Size;                        // Size of the Tx_DmaBDT [Bytes]
    NDIS_PHYSICAL_ADDRESS   Tx_DmaBDT_Pa;                          // Tx_DmaBDT physical address
    #if DBG
    LONG                    Tx_NBCounter;                          // For debug only
    LONG                    Tx_NBLCounter;                         // For debug only
    #endif

    // RECV
    NDIS_HANDLE             Rx_NBAndNBLPool;                       // NB and NBL pool handle
    PMP_RX_FRAME_BD         Rx_FrameBDT;                           // Rx payload data buffer descriptor table address
    NDIS_SPIN_LOCK          Rx_SpinLock;                           // Rx path spin lock
    LONG                    Rx_EnetFreeBDIdx;                      // Index of the first free BD
    LONG                    Rx_EnetPendingBDIdx;                   // Index of first BD submitted to ENET DMA
    LONG                    Rx_NdisOwnedBDsCount;                  // Number of buffers owned by NDIS
    LONG                    Rx_DmaBDT_DmaOwnedBDsCount;            // Number of BDs owned by ENET DMA (ready to receive data)
    LONG                    Rx_DmaBDT_DmaOwnedBDsLowWatterMark;    // Number of BDs that must be ready for data reception
    PENET_BD                Rx_DmaBDT;                             // ENET peripheral Dma buffer descriptor table (BDT) address
    PMP_ENET_BD_SW_EXT      Rx_DmaBDT_SwExt;                       // SW extension of Rx_DmaBDT
    LONG                    Rx_DmaBDT_ItemCount;                   // ENET peripheral Dma buffer descriptor table (BDT) item count
    ULONG                   Rx_DmaBDT_Size;                        // Size of Rx_DmaBDT in bytes
    NDIS_PHYSICAL_ADDRESS   Rx_DmaBDT_Pa;                          // Physical address of Rx_DmaBDT
    LONG                    Rx_NBLCounter;                         // For debug only
    NDIS_SPIN_LOCK          Dev_SpinLock;                          // spin locks
    // Packet Filter and look ahead size.
    ULONG                   PacketFilter;
    ULONG                   OldPacketFilter;
    ULONG                   ulLookAhead;
    // multicast list
    UINT                    MCAddressCount;
    UINT                    OldMCAddressCount;
    UCHAR                   MCList[NIC_MAX_MCAST_LIST][ETH_LENGTH_OF_ADDRESS];
    UCHAR                   OldMCList[NIC_MAX_MCAST_LIST][ETH_LENGTH_OF_ADDRESS];

    NDIS_HANDLE             NdisInterruptHandle;

    ULONG                   CacheFillSize;

    FRAME_RCV_STATUS        RcvStatus;
    FRAME_TXD_STATUS        TxdStatus;
    // new fields for NDIS 6.0 version to report unknown  states and speed
    NDIS_MEDIA_CONNECT_STATE    MediaConnectState;
    NDIS_MEDIA_DUPLEX_STATE     DuplexMode;
    ULONG64                     LinkSpeed;

    NDIS_DEVICE_POWER_STATE     CurrentPowerState;
    NDIS_DEVICE_POWER_STATE     NewPowerState;


    BOOLEAN                 bResetPending;
    PNDIS_OID_REQUEST       PendingRequest;
    MP_POWER_MGMT           PoMgmt;
    ULONG                   WakeUpEnable;
    // Accumulated HW statistics
    struct {
        ULONG64  ifInDiscards;
        ULONG64  ifInErrors;
        ULONG64  ifHCInOctets;
        ULONG64  ifHCInUcastPkts;
        ULONG64  ifHCInMulticastPkts;
        ULONG64  ifHCInBroadcastPkts;
        ULONG64  ifHCOutOctets;
        ULONG64  ifHCOutUcastPkts;
        ULONG64  ifHCOutMulticastPkts;
        ULONG64  ifHCOutBroadcastPkts;
        ULONG64  ifOutErrors;
        ULONG64  ifOutDiscards;
    } StatisticsAcc;

    #if DBG
    // Threshold values from registry
    USHORT                  OPD_Value;          // "Opcode/Pause Duration"
    USHORT                  RSFL_Value;         // "Rx FIFO Section Full Threshold"
    USHORT                  RSEM_Value;         // "Rx FIFO Section Empty Threshold"
    USHORT                  RAEM_Value;         // "Rx FIFO Almost Empty Threshold"
    USHORT                  RAFL_Value;         // "Rx FIFO Almost Full Threshold"
    USHORT                  TFWR_Value;         // "Tx FIFO Watermark"
    USHORT                  TAEM_Value;         // "Tx FIFO Almost Empty Threshold"
    USHORT                  TAFL_Value;         // "Tx FIFO Almost Full Threshold"
    USHORT                  TSEM_Value;         // "Tx FIFO Section Empty Threshold"

    LARGE_INTEGER           EchoReqRxTime;
    LARGE_INTEGER           EchoReqTxTime;
    #endif

    ULONG                   ENETDev_bmACPISupportedFunctions;         // ACPI _DSM supported methods bitmask.
    MP_PHY_DEVICE           ENETDev_PHYDevice;                        // ENET PHY device data structure.
    MP_MDIO_DEVICE          ENETDev_MDIODevice;                       // MDIO device (ENET PHY) data structure.
    #if DBG
    char                    ENETDev_DeviceName[MAX_DEVICE_NAME + 1];  // ENET device name.
    NDIS_PHYSICAL_ADDRESS   ENETDev_RegsPhyAddress;                   // ENET device registers physical address.
    ULONG                   ENETDev_Irq;                              // ENET device interrupt vector.
    #endif
} MP_ADAPTER, *PMP_ADAPTER;

//--------------------------------------
// Miniport routines in ENET.C
//--------------------------------------


MINIPORT_INTERRUPT_DPC EnetDpc;

NDIS_STATUS MpSetPower           (_In_ PMP_ADAPTER pAdapter, _In_ NDIS_DEVICE_POWER_STATE PowerState);

NDIS_TIMER_FUNCTION MpSmDispatcher;

VOID MpCompleteOids(_In_ PMP_ADAPTER pAdapter, _In_ NDIS_STATUS Status);

// MP_INIT.C
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS MpAllocAdapterBlock(_Out_ PMP_ADAPTER *pAdapter, _In_ NDIS_HANDLE MiniportAdapterHandle);
_IRQL_requires_max_(PASSIVE_LEVEL)
void MpFreeAdapter(_In_opt_ PMP_ADAPTER Adapter);
_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS NICReadRegParameters(_In_ PMP_ADAPTER Adapter);
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS NICAllocAdapterMemory(_In_ PMP_ADAPTER Adapter);

// MP_REQ.C
_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS NICGetStatsCounters(_In_ PMP_ADAPTER Adapter, _In_ NDIS_OID Oid, _Out_ PULONG64 pCounter);
_Requires_lock_held_(&Adapter->Dev_SpinLock)
_IRQL_requires_(DISPATCH_LEVEL)
NDIS_STATUS NICSetPacketFilter(_In_ PMP_ADAPTER Adapter, _In_ ULONG PacketFilter);
_Requires_lock_held_(&Adapter->Dev_SpinLock)
_IRQL_requires_(DISPATCH_LEVEL)
NDIS_STATUS NICSetMulticastList(_In_ PMP_ADAPTER Adapter);

// NDIS Miniport interfaces
_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS        MpQueryInformation(NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST NdisRequest);
_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS        MpSetInformation  (NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST NdisRequest);

// Prototypes for standard NDIS miniport entry points
MINIPORT_INITIALIZE                 MpInitializeEx;
MINIPORT_HALT                       MpHaltEx;
MINIPORT_UNLOAD                     MpUnloadDriver;
MINIPORT_PAUSE                      MpPause;
MINIPORT_RESTART                    MpRestart;
MINIPORT_RESET                      MpResetEx;
MINIPORT_DEVICE_PNP_EVENT_NOTIFY    MpDevicePnPEventNotify;
MINIPORT_SHUTDOWN                   MpShutdownEx;
MINIPORT_OID_REQUEST                MpOidRequest;
MINIPORT_CANCEL_OID_REQUEST         MpCancelOidRequest;

#endif  // _MP_H



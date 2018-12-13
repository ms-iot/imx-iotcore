/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    HalExtiMXDma.h

Abstract:

    This file includes the types/enums definitions associated with the
    i.MX SDMA HAL extension

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _HAL_EXT_IMX_SDMA
#define _HAL_EXT_IMX_SDMA

//
// -------------------------------------------- Constants and Macro Definitions
//


//
// Max number of SDMA events that can trigger 
// a transfer.
//

#define SDMA_MAX_TRIGGER_EVENTS 2


//
// Invalid request ID mark
//

#define SDMA_UNSUPPORTED_REQUEST_ID 0xFFFFFFFFUL


//
// Device flags
//

#define SDMA_DEVICE_FLAG_FIXED_ADDRESS  0x00000001UL
#define SDMA_DEVICE_FLAG_EXT_ADDRESS    0x00000002UL
#define SDMA_DEVICE_FLAG_P2P            0x00000004UL

#define SDMA_DEVICE_FLAG_ON(_Value, _Flags) (((_Value) & (_Flags)) != 0)


//
// Getting the logical address of a channel member
//

#define SDMA_CHN_LOGICAL_ADDR(_ChannelPtr, _Field) (ULONG)(\
    (((SDMA_CHANNEL*)(_ChannelPtr))->This.QuadPart + \
    FIELD_OFFSET(SDMA_CHANNEL, _Field)))

#define SDMA_CHN0_LOGICAL_ADDR(_Channel0Ptr, _Field) (ULONG)(\
    ((_Channel0Ptr)->SdmaChannel.This.QuadPart + \
    FIELD_OFFSET(SDMA_CHANNEL0, _Field)))


//
// 'Channel Done' retry count, since we cannot use
// any time/stall services.
//

#define IMX_MAX_CHANNEL_DONE_WAIT_RETRY 1000UL


#define SDMA_READ_REGISTER_ULONG(_Address) \
    READ_REGISTER_NOFENCE_ULONG((_Address))

#define SDMA_WRITE_REGISTER_ULONG(_Address, _Data) \
    WRITE_REGISTER_NOFENCE_ULONG((_Address), (_Data))


#ifdef DBG
    #define SDMA_DUMP_REGS(_SdmaCOntrollerPtr_) SdmaHwDumpRegs((_SdmaCOntrollerPtr_));
#else
    #define SDMA_DUMP_REGS(a)
#endif

//
// ----------------------------------------------------------- Type Definitions
//

//
// SDMA request line IDs.
// These IDs should match ACPI table DMA request line values.
// These value do not match the datasheet but rather a union of
// all DMA events.
// These values uniquely identify the device, since DMA events
// can be shared by more than one device, where different devices
// may need different handling (a different script). 
//
typedef enum _SDMA_REQ_LINE_ID {
    SDMA_REQ_VPU = 0,
    SDMA_REQ_IPU2 = 1,
    SDMA_REQ_IPU1 = 2,
    SDMA_REQ_HDMI_AUDIO = 3,
    SDMA_REQ_ECSPI1_RX = 4,
    SDMA_REQ_ECSPI1_TX = 5,
    SDMA_REQ_ECSPI2_RX = 6,
    SDMA_REQ_ECSPI2_TX = 7,
    SDMA_REQ_ECSPI3_RX = 8,
    SDMA_REQ_ECSPI3_TX = 9,
    SDMA_REQ_ECSPI4_RX = 10,
    SDMA_REQ_ECSPI4_TX = 11,
    SDMA_REQ_ECSPI5_RX = 12,
    SDMA_REQ_ECSPI5_TX = 13,
    SDMA_REQ_I2C1_RX = 14,
    SDMA_REQ_I2C1_TX = 15,
    SDMA_REQ_I2C2_RX = 16,
    SDMA_REQ_I2C2_TX = 17,
    SDMA_REQ_I2C3_RX = 18,
    SDMA_REQ_I2C3_TX = 19,
    SDMA_REQ_UART1_RX = 20,
    SDMA_REQ_UART1_TX = 21,
    SDMA_REQ_UART2_RX = 22,
    SDMA_REQ_UART2_TX = 23,
    SDMA_REQ_UART3_RX = 24,
    SDMA_REQ_UART3_TX = 25,
    SDMA_REQ_UART4_RX = 26,
    SDMA_REQ_UART4_TX = 27,
    SDMA_REQ_UART5_RX = 28,
    SDMA_REQ_UART5_TX = 29,
    SDMA_REQ_SPDIF_RX = 30,
    SDMA_REQ_SPDIF_TX = 31,
    SDMA_REQ_EPIT1 = 32,
    SDMA_REQ_EPIT2 = 33,
    SDMA_REQ_GPT1 = 34,
    SDMA_REQ_ASRC_RXA = 35,
    SDMA_REQ_ASRC_RXB = 36,
    SDMA_REQ_ASRC_RXC = 37,
    SDMA_REQ_ASRC_TXA = 38,
    SDMA_REQ_ASRC_TXB = 39,
    SDMA_REQ_ASRC_TXC = 40,
    SDMA_REQ_ESAI_RX = 41,
    SDMA_REQ_ESAI_TX = 42,
    SDMA_REQ_ASRC_TXA_2_ESAI_TX = 43,
    SDMA_REQ_ASRC_TXB_2_ESAI_TX = 44,
    SDMA_REQ_ASRC_TXC_2_ESAI_TX = 45,
    SDMA_REQ_SSI1_RX1 = 46,
    SDMA_REQ_SSI1_TX1 = 47,
    SDMA_REQ_SSI1_RX0 = 48,
    SDMA_REQ_SSI1_TX0 = 49,
    SDMA_REQ_SSI2_RX1 = 50,
    SDMA_REQ_SSI2_TX1 = 51,
    SDMA_REQ_SSI2_RX0 = 52,
    SDMA_REQ_SSI2_TX0 = 53,
    SDMA_REQ_SSI3_RX1 = 54,
    SDMA_REQ_SSI3_TX1 = 55,
    SDMA_REQ_SSI3_RX0 = 56,
    SDMA_REQ_SSI3_TX0 = 57,
    SDMA_REQ_EXT1 = 58,
    SDMA_REQ_EXT2 = 59,

    SDMA_REQ_UART6_RX = 60,
    SDMA_REQ_UART6_TX = 61,
    SDMA_REQ_ADC1 = 62,
    SDMA_REQ_ADC2 = 63,
    SDMA_REQ_I2C4_RX = 64,
    SDMA_REQ_I2C4_TX = 65,
    SDMA_REQ_CSI1 = 66,
    SDMA_REQ_CSI2 = 67,
    SDMA_REQ_PXP = 68,
    SDMA_REQ_LCDIF1 = 69,
    SDMA_REQ_LCDIF2 = 70,
    SDMA_REQ_QSPI1_RX = 71,
    SDMA_REQ_QSPI1_TX = 72,
    SDMA_REQ_QSPI2_RX = 73,
    SDMA_REQ_QSPI2_TX = 74,
    SDMA_REQ_SAI1_TX = 75,
    SDMA_REQ_SAI1_RX = 76,
    SDMA_REQ_SAI2_TX = 77,
    SDMA_REQ_SAI2_RX = 78,

    SDMA_REQ_SAI3_TX = 79,
    SDMA_REQ_SAI3_RX = 80,
    SDMA_REQ_SAI4_TX = 81,
    SDMA_REQ_SAI4_RX = 82,
    SDMA_REQ_SAI5_TX = 83,
    SDMA_REQ_SAI5_RX = 84,
    SDMA_REQ_SAI6_TX = 85,
    SDMA_REQ_SAI6_RX = 86,
    SDMA_REQ_UART7_RX = 87,
    SDMA_REQ_UART7_TX = 88,
    SDMA_REQ_GPT2 = 89,
    SDMA_REQ_GPT3 = 90,
    SDMA_REQ_GPT4 = 91,
    SDMA_REQ_GPT5 = 92,
    SDMA_REQ_GPT6 = 93,
    SDMA_REQ_ENET1_EV0 = 94,
    SDMA_REQ_ENET1_EV1 = 95,
    SDMA_REQ_ENET2_EV0 = 96,
    SDMA_REQ_ENET2_EV1 = 97,
    SDMA_REQ_EXT3 = 98,
    SDMA_REQ_EXT4 = 99,
    SDMA_REQ__MAX,

} SDMA_REQ_LINE_ID;

// 
// The iMX8M SOC has two SDMA controllers so the request line ids must
// be partitioned across the two controllers. The lower 10 bits encode
// the request line id
//

#define SDMA_REQ_LINE_ID_MASK 0x3FF
#define SDMA_INSTANCE_ID_SHIFT 10

//
// Unsupported request ID
//

#define SDMA_REQ_UNSUPPORTED ((ULONG)-1)


#pragma pack(push, 1)

//
// DMA / SDMA Core Clock Ratio
//

typedef enum _IMX_SDMA_CORE_CLOCK_RATIO {
    IMX_SDMA_CORE_CLOCK_TWICE_CORE_FREQ = 0,
    IMX_SDMA_CORE_CLOCK_EQUALS_CORE_FREQ = 1,
} IMX_SDMA_CORE_CLOCK_RATIO;


//
// Shared DMA event selection
//

typedef enum _IMX_IOMUXC_GPR0_DMA_EVENT_MUX_SEL {
    DMA_EVENT_EXCLUSIVE = 0,
    DMA_EVENT_MUX_SEL0 = (1 << 0),
    DMA_EVENT_MUX_SEL1 = (1 << 1),
    DMA_EVENT_MUX_SEL2 = (1 << 2),
    DMA_EVENT_MUX_SEL3 = (1 << 3),
    DMA_EVENT_MUX_SEL4 = (1 << 4),
    DMA_EVENT_MUX_SEL5 = (1 << 5),
    DMA_EVENT_MUX_SEL6 = (1 << 6),
    DMA_EVENT_MUX_SEL7 = (1 << 7),
    DMA_EVENT_MUX_SEL8 = (1 << 8),
    DMA_EVENT_MUX_SEL9 = (1 << 9),
    DMA_EVENT_MUX_SEL10 = (1 << 10),
    DMA_EVENT_MUX_SEL11 = (1 << 11),
    DMA_EVENT_MUX_SEL12 = (1 << 12),
    DMA_EVENT_MUX_SEL13 = (1 << 13),
    DMA_EVENT_MUX_SEL14 = (1 << 14),
    DMA_EVENT_MUX_SEL15 = (1 << 15),
    DMA_EVENT_MUX_SEL16 = (1 << 16),
    DMA_EVENT_MUX_SEL17 = (1 << 17),
    DMA_EVENT_MUX_SEL18 = (1 << 18),
    DMA_EVENT_MUX_SEL19 = (1 << 19),
    DMA_EVENT_MUX_SEL20 = (1 << 20),
    DMA_EVENT_MUX_SEL21 = (1 << 21),
    DMA_EVENT_MUX_SEL22 = (1 << 22),
} IMX_IOMUXC_GPR0_DMA_EVENT_MUX_SEL;


//
// CSRT SDMA controller descriptor
// Matches the CSRT resource descriptor (RD_SDMA).
//

typedef struct _CSRT_RESOURCE_DESCRIPTOR_SDMA_CONTROLLER {

    CSRT_RESOURCE_DESCRIPTOR_HEADER Header;

    ULONG ChipType;
    UINT64 RegistersBaseAddress;
    UINT64 IoMuxGPR0Address;
    ULONG Interrupt;
    ULONG SdmaCoreClockRatio;

} CSRT_RESOURCE_DESCRIPTOR_SDMA_CONTROLLER;

#pragma pack(pop)


//
// SDMA event configuration
//

typedef struct _SDMA_EVENT_CONFIG {

    //
    // DMA Event ID
    // The actual SOC specific DMA event ID
    //
    ULONG SdmaEventId;

    //
    // DMA event selection position
    //

    IMX_IOMUXC_GPR0_DMA_EVENT_MUX_SEL DmaEventSelect;

    //
    // The selection (0/1)
    //

    ULONG SelectConfig;

} SDMA_EVENT_CONFIG;


//
// SDMA channel configuration based on 
// the SMA request line.
// 

typedef struct _SDMA_CHANNEL_CONFIG {

    //
    // SDMA Script address
    //

    ULONG SdmaScriptAddr;

    //
    // DMA request ID
    //

    ULONG DmaRequestId;

    //
    // Transfer width
    //

    SDMA_TRANSFER_WIDTH TransferWidth;

    //
    // The channel index that owns the request line.
    // Used for managing mutual exclusion for shared DMA requests.
    // Value of 0 indicates no owner.
    //

    ULONG OwnerChannel;

    //
    // Device flags.
    // Check SDMA_DEVICE_FLAG_xxx values
    // 

    ULONG DeviceFlags;

    //
    // Other peripheral address,
    // used for peripheral to peripheral transfers.
    //

    UINT32 Peripheral2Address;

    //
    // Watermark Level scale (percent)
    //
    // We apply the scale factor to the watermark level
    // supplied by the client.
    // We for special cases like UART RX, where we need lower
    // the SDMA controller watermark (loop chunk size), so
    // the transfer always ends by the aging timer.
    //

    ULONG WatermarkLevelScale;

    //
    // Trigger events list
    //

    ULONG TriggerDmaEventCount;
    SDMA_EVENT_CONFIG TriggerDmaEvents[SDMA_MAX_TRIGGER_EVENTS];

    //
    // SDMA instance
    //

    ULONG SdmaInstance;

} SDMA_CHANNEL_CONFIG ;


//
// SMA channel state
//

typedef enum _SDMA_CHANNEL_STATE {
    CHANNEL_IDLE = 0,
    CHANNEL_RUNNING,
    CHANNEL_ABORTING,
    CHANNEL_ERROR
} SDMA_CHANNEL_STATE;


//
// SDMA buffer descriptor extension
//

typedef struct _SDMA_BD_EXT {

    //
    // Buffer logical base address
    //

    ULONG Address;

    //
    // Buffer length (bytes)
    //

    ULONG ByteLength;

    //
    // Number of bytes transferred
    //

    ULONG BytesTransferred;

} SDMA_BD_EXT;


//
// SDMA channel descriptor 1..31
//

typedef struct _SDMA_CHANNEL {

    //
    // A buffer descriptor associated with the channel
    //

    volatile SDMA_BD SdmaBD[SDMA_SG_LIST_MAX_SIZE];

    //
    // Buffer descriptors extension
    // For keeping track of SdmaBD that is modified by the HW in runtime.
    //

    SDMA_BD_EXT SdmaBdExt[SDMA_SG_LIST_MAX_SIZE];

    //
    // The logical address of this structure
    //

    PHYSICAL_ADDRESS This;

    //
    // If set to auto initialize
    //

    BOOLEAN IsAutoInitialize;

    //
    // Address of the assigned channel configuration,
    // or NULL if channel is not active.
    //

    SDMA_CHANNEL_CONFIG* ChannelConfigPtr;

    //
    // Watermark level.
    // By default it is taken from ChannelConfigPtr, and it can be
    // overwritten through SdmaConfigureChannel 
    // (SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL).
    //
    
    ULONG WatermarkLevel;

    //
    // Notification threshold (bytes)
    //

    ULONG NotificationThreshold;

    //
    // Number of active buffers
    //

    ULONG ActiveBufferCount;

    //
    // Transfer length (bytes)
    //

    ULONG TransferLength;

    //
    // Device address
    //

    PHYSICAL_ADDRESS DeviceAddress;

    //
    // Channel State
    //

    SDMA_CHANNEL_STATE State;

    //
    // Next buffer to process (Auto Initialize transfers)
    //

    ULONG AutoInitNextBufferIndex;

    //
    // Bytes transferred (Auto Initialize transfers)
    //

    ULONG AutoInitBytesTransferred;

    //
    // DMA word size in bytes
    //

    ULONG DmaWordSize;

} SDMA_CHANNEL;


//
// SDMA channel 0 descriptor
//

typedef struct _SDMA_CHANNEL0 {

    //
    // Standard SDMA channel descriptor
    //
    
    SDMA_CHANNEL SdmaChannel;

    //
    // Channel control blocks.
    // All channel CCBs need to reside as a contiguous block.
    //

    volatile SDMA_CCB SdmaCCBs[SDMA_NUM_CHANNELS];

    //
    // Buffer for holding scripts code
    // to be loaded to SDMA RAM.
    //

    UCHAR ScriptsCode[SDMA_MAX_RAM_CODE_SIZE];

    //
    // Size of script code 
    //

    ULONG ScriptsCodeSize;

    //
    // Channel context for scripts parameters
    //

    SDMA_CHANNEL_CONTEXT ChannelContext;

} SDMA_CHANNEL0;


//
// SDMA controller internal data, that is
// passed by the framework to each DMA HAL extension callback routine.
//

typedef struct _SDMA_CONTROLLER {

    //
    // SDMA base address logical address.
    //

    PHYSICAL_ADDRESS SdmaRegsAddress;

    //
    // IoMuX GPR0 register logical address.
    //

    PHYSICAL_ADDRESS IoMuxGPR0RegAddress;

    //
    // SDMA controller instance
    // 
    
    UINT32 SdmaInstance;

    //
    // DMA / SDMA Core Clock Ratio
    //

    ULONG CoreClockRatio;

    //
    // SDMA request ID configuration mapping
    //

    SDMA_CHANNEL_CONFIG* SdmaReqToChannelConfigPtr;
    ULONG SdmaReqMaxId;

    //
    // SDMA code block
    //
    const short* SdmaCodeBlock;
    ULONG SdmaCodeSize;

    // Script references to distinguish mem->mem, periph->periph and other
    ULONG SdmaAp2ApScript;
    ULONG SdmaPer2PerScript;

    //
    // SDMA registers mapped (virtual) address
    //

    volatile SDMA_REGS* SdmaRegsPtr;

    //
    // IoMuX GPR0 register mapped (virtual) address
    //

    volatile ULONG* IoMuxGPR0RegPtr;

    //
    // All channel descriptors
    //

    SDMA_CHANNEL* ChannelsPtr[SDMA_NUM_CHANNELS];

    //
    // Pending channel interrupts
    //

    ULONG PendingInterrupts;

    //
    // The SDMA controller status
    //

    NTSTATUS ControllerStatus;

} SDMA_CONTROLLER;


//
// ------------------------------------------------ HAL extension DDI Functions
//

DMA_INITIALIZE_CONTROLLER SdmaInitializeController;
DMA_VALIDATE_REQUEST_LINE_BINDING SdmaValidateRequestLineBinding;
DMA_QUERY_MAX_FRAGMENTS SdmaQueryMaxFragments;
DMA_PROGRAM_CHANNEL SdmaProgramChannel;
DMA_CONFIGURE_CHANNEL SdmaConfigureChannel;
DMA_CANCEL_TRANSFER SdmaCancelTransfer;
DMA_FLUSH_CHANNEL SdmaFlushChannel;
DMA_HANDLE_INTERRUPT SdmaHandleInterrupt;
DMA_READ_DMA_COUNTER SdmaReadDmaCounter;
DMA_REPORT_COMMON_BUFFER SdmaReportCommonBuffer;

//
// ------------------------------------------------------- HW support Functions
//

NTSTATUS
SdmaInitDmaReqMapping (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ IMX_CPU CpuType,
    _In_ ULONG Instance
    );

VOID
SdmaAcquireChannel0 (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr
    );

VOID
SdmaReleaseChannel0 (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr
    );

VOID
SdmaHwInitialize (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr
    );

NTSTATUS
SdmaHwStartChannel (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG Channel
    );

NTSTATUS
SdmaHwResumeChannel (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

BOOLEAN
SdmaHwIsChannelRunning (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG Channel
    );

NTSTATUS
SdmaHwStopTransfer (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

NTSTATUS
SdmaHwStopChannel (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

VOID
SdmaHwSetChannelOverride (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG Channel,
    _In_ BOOLEAN IsEventOverride
    );

VOID
SdmaHwSetChannelPriority (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG Channel,
    _In_ SDMA_CHANNEL_PRIORITY Priority
    );

NTSTATUS
SdmaHwBindDmaEvents (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber,
    _In_ ULONG RequestLine
    );

VOID
SdmaHwUnbindDmaEvents (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

NTSTATUS
SdmaHwConfigureSgList (
    _In_ SDMA_CHANNEL* SdmaChannelPtr,
    _In_ const DMA_SCATTER_GATHER_LIST* MemoryAddressesPtr,
    _In_ PHYSICAL_ADDRESS DeviceAddress,
    _In_ BOOLEAN IsLoopTransfer
    );

VOID
SdmaHwInitBufferDescriptor (
    _In_ SDMA_CHANNEL* SdmaChannelPtr,
    _In_ ULONG BufferIndex,
    _In_ ULONG Length,
    _In_ PHYSICAL_ADDRESS Address1,
    _In_ PHYSICAL_ADDRESS Address2,
    _In_ BOOLEAN IsLast,
    _In_ BOOLEAN IsAutoInitialize
    );

ULONG
SdmaGetTransferLength (
    _In_ const DMA_SCATTER_GATHER_LIST* MemoryAddressesPtr
    );

BOOLEAN
SdmaHwUpdateBufferDescriptorForAutoInitialize (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber,
    _In_ ULONG BufferIndex
    );

NTSTATUS
SdmaHwSetupChannel (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

NTSTATUS
SdmaHwSetupChannelContext (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

NTSTATUS
SdmaHwLoadRamPM (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG SourceAddress,
    _In_ ULONG DestinationAddress,
    _In_ ULONG ByteCount
    );

NTSTATUS
SdmaHwLoadChannelContext (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

VOID
SdmaHwGetDmaEventMask (
    _In_ const SDMA_CHANNEL_CONFIG* ChannelConfigPtr,
    _Out_ LARGE_INTEGER* DmaEventMaskPtr
    );

VOID
SdmaHwUpdateChannelForAutoInitialize (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

NTSTATUS
SdmaHwSetRequestLineOwnership (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber,
    _In_ ULONG DmaRequestLine,
    _In_ BOOLEAN IsAcquire
    );

NTSTATUS
SdmaHwWaitChannelDone (
    _In_ SDMA_CONTROLLER* SdmaControllerPtr,
    _In_ ULONG ChannelNumber
    );

ULONG
SdmaHwGetDataWidth (
    _In_ SDMA_TRANSFER_WIDTH SdmaTransferWidth
    );

VOID
SdmaHwDumpRegs (
    _In_ const SDMA_CONTROLLER* SdmaControllerPtr
    );

#endif // !_HAL_EXT_IMX_SDMA

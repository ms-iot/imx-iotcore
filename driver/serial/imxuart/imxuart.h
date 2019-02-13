// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   imxuart.h
//
// Abstract:
//
//   IMX6 UART Driver Declarations
//
#ifndef _IMX_UART_H_
#define _IMX_UART_H_

//
// Macros to be used for proper PAGED/NON-PAGED code placement
//

#define IMX_UART_NONPAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    //__pragma(code_seg(.text))

#define IMX_UART_NONPAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMX_UART_PAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("PAGE"))

#define IMX_UART_PAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMX_UART_INIT_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("INIT"))

#define IMX_UART_INIT_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMX_UART_ASSERT_MAX_IRQL(Irql) NT_ASSERT(KeGetCurrentIrql() <= (Irql))

enum : ULONG { IMX_UART_POOL_TAG = 'RUXM' };

enum : ULONG { IMX_UART_STATE_ACTIVE = 0x100 };
enum IMX_UART_STATE : ULONG {
    STOPPED = 0x00,
    IDLE = 0x01,
    CANCELLED = 0x02,
    WAITING_FOR_INTERRUPT = (IMX_UART_STATE_ACTIVE | 0x03),
    WAITING_FOR_DPC = (IMX_UART_STATE_ACTIVE | 0x04),
    STOPPING = (IMX_UART_STATE_ACTIVE | 0x05),
    COMPLETE = 0x06,
};

enum : ULONG { IMX_UART_RX_DMA_MIN_BUFFER_SIZE = 4096UL };

//
// Placement new and delete operators
//

_Notnull_
void* __cdecl operator new ( size_t, _In_ void* Ptr ) throw ();

void __cdecl operator delete ( void*, void* ) throw ();

_Notnull_
void* __cdecl operator new[] ( size_t, _In_ void* Ptr ) throw ();

void __cdecl operator delete[] ( void*, void* ) throw ();

//
// Single-reader, single-writer circular buffer.
//
struct IMX_UART_RING_BUFFER {
    ULONG HeadIndex;                    // points to next available slot
    ULONG TailIndex;                    // points to least recently filled slot
    ULONG Size;                         // number of slots in buffer
    _Field_size_(Size) UCHAR* BufferPtr;

    FORCEINLINE IMX_UART_RING_BUFFER () :
        HeadIndex(0),
        TailIndex(0),
        Size(0),
        BufferPtr(nullptr)
    {}

    FORCEINLINE void SetBuffer (_In_reads_(Size) UCHAR* InBufferPtr, ULONG InBufferSize)
    {
        this->HeadIndex = 0;
        this->TailIndex = 0;
        this->Size = InBufferSize;
        this->BufferPtr = InBufferPtr;
    }

    FORCEINLINE bool IsEmpty () const
    {
        return this->HeadIndex == this->TailIndex;
    }

    FORCEINLINE ULONG Count () const
    {
        return Count(this->HeadIndex, this->TailIndex, this->Size);
    }

    FORCEINLINE static ULONG Count (ULONG HeadIndex, ULONG TailIndex, ULONG Size)
    {
        if (HeadIndex >= TailIndex)
        {
            return HeadIndex - TailIndex;
        }
        else
        {
            return Size + HeadIndex - TailIndex;
        }
    }

    FORCEINLINE ULONG Capacity () const
    {
        return this->Size - 1;
    }

    //
    // Empties the buffer and returns the number of bytes that were discarded.
    // Caller is responsible to synchronize.
    //
    FORCEINLINE ULONG Reset ()
    {
        const ULONG count = this->Count();
        this->HeadIndex = 0;
        this->TailIndex = 0;
        return count;
    }

    ULONG
    EnqueueBytes (
        _In_reads_(InputBufferSize) const UCHAR* InputBufferPtr,
        ULONG InputBufferSize
        );

    ULONG
    DequeueBytes (
        _Out_writes_to_(OutputBufferSize, return) UCHAR* OutputBufferPtr,
        ULONG OutputBufferSize
        );
};

struct IMX_UART_WDFKEY {
    WDFKEY Handle;

    FORCEINLINE IMX_UART_WDFKEY (WDFKEY _Handle = WDF_NO_HANDLE) : Handle(_Handle) {}

    _IRQL_requires_max_(DISPATCH_LEVEL)
    FORCEINLINE ~IMX_UART_WDFKEY ()
    {
        if (this->Handle == WDF_NO_HANDLE) return;
        WdfObjectDelete(this->Handle);
        this->Handle = WDF_NO_HANDLE;
    }
};

struct IMX_UART_INTERRUPT_CONTEXT;

struct IMX_UART_DEVICE_CONTEXT {
    IMX_UART_REGISTERS* RegistersPtr;
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr;

    ULONG CurrentBaudRate;
    ULONG CurrentFlowReplace;

    BYTE* RxBufferStorage;
    BYTE* TxBufferStorage;
    WDFDEVICE WdfDevice;
    WDFINTERRUPT WdfInterrupt;

    //
    // Has IMXUartEvtSerCx2ApplyConfig() been called at least once?
    //
    bool Initialized;

    //
    // Were RTS and CTS specified in the SerialLinesEnabled field of the
    // connection descriptor? Output CTS flow control (SERIAL_CTS_HANDSHAKE)
    // is only allowed if RTS and CTS were specified in connection
    // descriptor
    //
    bool RtsCtsLinesEnabled;

    //
    // Select UART as data terminal equipment (DTE mode)
    // or as data communication equipment (DCE mode)
    //
    bool DTEModeSelected;

    struct {
        ULONG RxIntermediateBufferSize;
        ULONG RxDmaIntermediateBufferSize;
        ULONG TxIntermediateBufferSize;
        ULONG RxFifoThresholdUs;
        ULONG TxFifoThresholdUs;
        ULONG TxDpcThresholdUs;
        ULONG RtsTriggerLevel;
        ULONG ModuleClockFrequency;
        ULONG RxDmaMinTransactionLength;
        ULONG TxDmaMinTransactionLength;
    } Parameters;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMX_UART_DEVICE_CONTEXT,
    IMXUartGetDeviceContext);

struct IMX_UART_RX_DMA_TRANSACTION_CONTEXT;
struct IMX_UART_TX_DMA_TRANSACTION_CONTEXT;

struct IMX_UART_INTERRUPT_CONTEXT {
    IMX_UART_REGISTERS* RegistersPtr;

    ULONG Usr1EnabledInterruptsMask;
    ULONG Usr2EnabledInterruptsMask;

    IMX_UART_STATE RxState;
    IMX_UART_STATE RxDmaState;
    IMX_UART_STATE TxState;
    IMX_UART_STATE TxDrainState;
    IMX_UART_STATE TxPurgeState;
    IMX_UART_STATE TxDmaState;
    IMX_UART_STATE TxDmaDrainState;

    IMX_UART_RING_BUFFER RxBuffer;
    IMX_UART_RING_BUFFER TxBuffer;

    ULONG Ucr1Copy;
    ULONG Ucr2Copy;
    ULONG Ucr3Copy;
    ULONG Ucr4Copy;
    ULONG UbirCopy;
    ULONG UbmrCopy;
    ULONG UfcrCopy;

    //
    // Stores errors supplied to SERIAL_STATUS::Errors member
    //
    ULONG CommStatusErrors;

    //
    // Stores current wait mask (IOCTL_SERIAL_SET_WAIT_MASK)
    //
    ULONG WaitMask;

    //
    // Stores pending wait events
    //
    ULONG WaitEvents;

    //
    // Number of bytes purged from the TX intermediate FIFO
    //
    ULONG PurgedByteCount;

    //
    // If intermediate TX buffer falls below this threshold, call
    // SerCx2PioTransmitReady() to request more bytes.
    //
    ULONG TxDpcThreshold;

    WDFINTERRUPT WdfInterrupt;
    SERCX2PIORECEIVE SerCx2PioReceive;
    SERCX2PIOTRANSMIT SerCx2PioTransmit;

    //
    // DMA related objects
    //
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr;
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* TxDmaTransactionContextPtr;
    bool IsRxDmaStarted;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMX_UART_INTERRUPT_CONTEXT,
    IMXUartGetInterruptContext);

struct IMX_UART_RX_CONTEXT {
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(IMX_UART_RX_CONTEXT, IMXUartGetRxContext);

struct IMX_UART_TX_CONTEXT {
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(IMX_UART_TX_CONTEXT, IMXUartGetTxContext);

struct IMX_UART_DMA_TRANSACTION_CONTEXT {
    //
    // Transaction associated objects
    //
    WDFDEVICE WdfDevice;
    WDFDMAENABLER WdfDmaEnabler;
    PDMA_ADAPTER DmaAdapterPtr;
    WDFDMATRANSACTION WdfDmaTransaction;

    //
    // Configuration
    //
    ULONG DmaRequestLine;

    //
    // Container
    //
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr;

    //
    // Runtime
    //
    WDFSPINLOCK Lock;
    bool IsDeferredCancellation;
    WDFREQUEST WdfRequest;
    size_t TransferLength;
    size_t BytesTransferred;
};

struct IMX_UART_RX_DMA_TRANSACTION_CONTEXT
    : public IMX_UART_DMA_TRANSACTION_CONTEXT {
    //
    // SerCx2 associated handles
    //
    SERCX2CUSTOMRECEIVE SerCx2CustomRx;
    SERCX2CUSTOMRECEIVETRANSACTION SerCx2CustomRxTransaction;

    //
    // DMA RX buffer information
    //
    UCHAR* DmaBufferPtr;
    size_t DmaBufferSize;
    PMDL DmaBufferMdlPtr;
    size_t DmaBufferReadPos;
    size_t DmaBufferPendingBytes;
    size_t LastDmaCounter;

    //
    // Caller request buffer parameters
    //
    PMDL BufferMdlPtr;
    size_t BufferMdlOffset;

    //
    // Progress information
    //
    WDFTIMER WdfProgressTimer;
    ULONG CurrentFrameTimeUsec;
    ULONG CurrentBaudRate;
    LONG UnreportedBytes;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT,
    IMXUartGetRxDmaTransactionContext);

struct IMX_UART_RX_DMA_TIMER_CONTEXT {
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionPtr;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMX_UART_RX_DMA_TIMER_CONTEXT,
    IMXUartGetRxDmaTimerContext);

struct IMX_UART_TX_DMA_TRANSACTION_CONTEXT
    : public IMX_UART_DMA_TRANSACTION_CONTEXT {
    //
    // SerCx2 associated handles
    //
    SERCX2CUSTOMTRANSMIT SerCx2CustomTx;
    SERCX2CUSTOMTRANSMITTRANSACTION SerCx2CustomTxTransaction;

    //
    // Runtime
    //
    bool IsDeferredCancellation;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT,
    IMXUartGetTxDmaTransactionContext);

//
// NONPAGED
//

EVT_WDF_INTERRUPT_ISR IMXUartEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC IMXUartEvtInterruptDpc;

EVT_SERCX2_FILEOPEN IMXUartEvtSerCx2FileOpen;
EVT_SERCX2_FILECLOSE IMXUartEvtSerCx2FileClose;
EVT_SERCX2_SET_WAIT_MASK IMXUartEvtSerCx2SetWaitMask;
EVT_SERCX2_CONTROL IMXUartEvtSerCx2Control;

EVT_SERCX2_PIO_RECEIVE_READ_BUFFER IMXUartEvtSerCx2PioReceiveReadBuffer;
EVT_SERCX2_PIO_RECEIVE_ENABLE_READY_NOTIFICATION IMXUartEvtSerCx2PioReceiveEnableReadyNotification;
EVT_SERCX2_PIO_RECEIVE_CANCEL_READY_NOTIFICATION IMXUartEvtSerCx2PioReceiveCancelReadyNotification;

EVT_SERCX2_PIO_TRANSMIT_WRITE_BUFFER IMXUartEvtSerCx2PioTransmitWriteBuffer;
EVT_SERCX2_PIO_TRANSMIT_ENABLE_READY_NOTIFICATION IMXUartEvtSerCx2PioTransmitEnableReadyNotification;
EVT_SERCX2_PIO_TRANSMIT_CANCEL_READY_NOTIFICATION IMXUartEvtSerCx2PioTransmitCancelReadyNotification;
EVT_SERCX2_PIO_TRANSMIT_DRAIN_FIFO IMXUartEvtSerCx2PioTransmitDrainFifo;
EVT_SERCX2_PIO_TRANSMIT_CANCEL_DRAIN_FIFO IMXUartEvtSerCx2PioTransmitCancelDrainFifo;
EVT_SERCX2_PIO_TRANSMIT_PURGE_FIFO IMXUartEvtSerCx2PioTransmitPurgeFifo;

EVT_SERCX2_CUSTOM_RECEIVE_TRANSACTION_START IMXUartEvtSerCx2CustomReceiveTransactionStart;
EVT_SERCX2_CUSTOM_RECEIVE_TRANSACTION_CLEANUP IMXUartEvtSerCx2CustomReceiveTransactionCleanup;
EVT_SERCX2_CUSTOM_RECEIVE_TRANSACTION_QUERY_PROGRESS IMXUartEvtSerCx2CustomReceiveTransactionQueryProgress;
EVT_WDF_REQUEST_CANCEL IMXUartEvtCustomReceiveTransactionRequestCancel;

EVT_WDF_DMA_TRANSACTION_CONFIGURE_DMA_CHANNEL IMXUartEvtWdfRxDmaTransactionConfigureDmaChannel;
EVT_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE IMXUartEvtWdfRxDmaTransactionTransferComplete;
EVT_WDF_PROGRAM_DMA IMXUartEvtWdfProgramRxDma;
EVT_WDF_TIMER IMXUartEvtTimerRxDmaProgress;

EVT_SERCX2_CUSTOM_TRANSMIT_TRANSACTION_INITIALIZE IMXUartEvtSerCx2CustomTransmitTransactionInitialize;
EVT_SERCX2_CUSTOM_TRANSMIT_TRANSACTION_START IMXUartEvtSerCx2CustomTransmitTransactionStart;
EVT_SERCX2_CUSTOM_TRANSMIT_TRANSACTION_CLEANUP IMXUartEvtSerCx2CustomTransmitTransactionCleanup;
EVT_WDF_REQUEST_CANCEL IMXUartEvtCustomTransmitTransactionRequestCancel;

EVT_WDF_DMA_TRANSACTION_CONFIGURE_DMA_CHANNEL IMXUartEvtWdfTxDmaTransactionConfigureDmaChannel;
EVT_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE IMXUartEvtWdfTxDmaTransactionTransferComplete;
EVT_WDF_PROGRAM_DMA IMXUartEvtWdfProgramTxDma;

EVT_SERCX2_PURGE_FIFOS IMXUartEvtSerCx2PurgeFifos;
EVT_SERCX2_APPLY_CONFIG IMXUartEvtSerCx2ApplyConfig;

NTSTATUS
IMXUartCheckValidStateForConfigIoctl (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr
    );

NTSTATUS
IMXUartResetHardwareAndPreserveShadowedRegisters (
    const IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

NTSTATUS
IMXUartResetHardwareAndClearShadowedRegisters (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

void
IMXUartRestoreHardwareFromShadowedRegisters (
    const IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

void
IMXUartComputeFifoThresholds (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    ULONG BaudRate,
    _In_range_(9, 11) ULONG BitsPerFrame,
    _Out_range_(2, IMX_UART_FIFO_COUNT) ULONG* TxFifoThresholdPtr,
    _Out_range_(1, IMX_UART_FIFO_COUNT - 1) ULONG* RxFifoThresholdPtr
    );

ULONG
IMXUartComputeCharactersPerDuration (
    ULONG DurationUs,
    ULONG BaudRate,
    ULONG BitsPerFrame
    );

NTSTATUS
IMXUartComputeDividers (
    ULONG UartModuleClock,
    ULONG DesiredBaudRate,
    _Out_ ULONG* RfDivPtr,
    _Out_ ULONG* UbirPtr,
    _Out_ ULONG* UbmrPtr
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
IMXUartConfigureUart (
    WDFDEVICE WdfDevice,
    ULONG BaudRate,
    const SERIAL_LINE_CONTROL *LineControl,
    const SERIAL_HANDFLOW *Handflow,
    bool RtsCtsEnabled
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartSetBaudRate (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    ULONG BaudRate
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartSetLineControl (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    const SERIAL_LINE_CONTROL* LineControlPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartSetHandflow (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    const SERIAL_HANDFLOW* LineControlPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartSetBreakOnOff (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    bool BreakOn
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartSetWaitMask (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    ULONG WaitMask
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartUpdateDmaSettings (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartStartRxDma (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartStopRxDma (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE ULONG
IMXUartDmaReadCounter (
    PDMA_ADAPTER DmaAdapterPtr
    )
{
    return DmaAdapterPtr->DmaOperations->ReadDmaCounter(DmaAdapterPtr);
}

FORCEINLINE bool
IMXUartIsRxDmaActive (
    const IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    const ULONG rxDmaState = InterruptContextPtr->RxDmaState;
    return (rxDmaState & IMX_UART_STATE_ACTIVE) != 0;
}

FORCEINLINE bool
IMXUartIsTxDmaActive (
    const IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    const ULONG txDmaState = InterruptContextPtr->TxDmaState |
        InterruptContextPtr->TxDmaDrainState;

    return (txDmaState & IMX_UART_STATE_ACTIVE) != 0;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartAcquireDmaRequestLineOwnership (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartReleaseDmaRequestLineOwnership (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
IMXUartRxDmaGetBytesTransferred (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IMXUartRxDmaStartProgressTimer (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
bool
IMXUartRxDmaCopyToUserBuffer (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr,
    size_t NewBytesTransferred
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
IMXUartPioDequeueDmaBytes (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr,
    _Out_writes_(Length) PUCHAR Buffer,
    ULONG Length
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IMXUartCompleteCustomRxTransactionRequest (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr,
    NTSTATUS RequestStatus
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IMXUartTxDmaDrainFifo (
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* TxDmaTransactionContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IMXUartCompleteCustomTxTransactionRequest (
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* TxDmaTransactionContextPtr,
    NTSTATUS RequestStatus
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartGetErrorInformation (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IMXUartNotifyEventsDuringDma (
    IMX_UART_DMA_TRANSACTION_CONTEXT* DmaTransactionContextPtr,
    const ULONG WaitEvents,
    const ULONG CommStatusInfo
    );

//
// IOCTL Handlers
//

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlSetBaudRate (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetBaudRate (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlSetLineControl (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetLineControl (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlSetHandflow (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetHandflow (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetProperties (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetCommStatus (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetModemStatus (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlSetClrRts (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest,
    bool RtsState
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetDtrRts (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlSetBreakOnOff (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest,
    bool BreakOn
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
IMXUartIoctlGetModemControl (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    );

//
// ACPI - Device Properties
//

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS IMXUartReadDeviceProperties(
    WDFDEVICE WdfDevice,
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr
    );

//
// PAGED
//

EVT_WDF_DEVICE_PREPARE_HARDWARE IMXUartEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE IMXUartEvtDeviceReleaseHardware;
EVT_WDF_DRIVER_DEVICE_ADD IMXUartEvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD IMXUartEvtDriverUnload;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
IMXUartCreateDeviceInterface (
    WDFDEVICE WdfDevice,
    LARGE_INTEGER ConnectionId
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
IMXUartParseConnectionDescriptor (
    const RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER* RhBufferPtr,
    _Out_ ULONG* BaudRatePtr,
    _Out_ SERIAL_LINE_CONTROL* LineControlPtr,
    _Out_ SERIAL_HANDFLOW* HandflowPtr,
    _Out_ bool* RtsCtsEnabledPtr
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
IMXUartInitializeDma (
    WDFDEVICE WdfDevice,
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* RegistersResourcePtr,
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* RxDmaResourcePtr,
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* TxDmaResourcePtr
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
IMXUartDeinitializeDma (
    WDFDEVICE WdfDevice
    );

extern "C" DRIVER_INITIALIZE DriverEntry;

#endif // _IMX_UART_H_

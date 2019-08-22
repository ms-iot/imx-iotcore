/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    CSoc class implementation.

*/

#include "soc.h"

#pragma code_seg()
volatile ULONG cntRxSyncError = 0;
volatile ULONG cntRxFifoError = 0;
volatile ULONG cntRxFifoWarning = 0;
volatile ULONG cntRxFifoRequest = 0;
volatile ULONG cntTxSyncError = 0;
volatile ULONG cntTxFifoError = 0;
volatile ULONG cntTxFifoWarning = 0;
volatile ULONG cntTxFifoRequest = 0;
volatile ULONG cntIsr = 0;

#pragma code_seg()
VOID
CDmaBuffer::ResetTxFifo()
{
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;

    TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
    TransmitControlRegister.AsUlong &= ~SAI_CTRL_ERRORFLAGS;    // don't reset error flags
    TransmitControlRegister.FifoReset = 1;
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

#ifdef _ARM64_
    SAI_TRANSMIT_CONFIGURATION_REGISTER_3 TransmitConfigReg3;

    TransmitConfigReg3.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitConfigRegister3.AsUlong);
    TransmitConfigReg3.ChannelFifoReset |= 1;
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitConfigRegister3.AsUlong, TransmitConfigReg3.AsUlong);
#endif
    m_ulChannel = 0;
}

#pragma code_seg()
VOID
CDmaBuffer::ResetRxFifo()
{
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;

    ReceiveControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong);
    ReceiveControlRegister.AsUlong &= ~SAI_CTRL_ERRORFLAGS;    // don't reset error flags
    ReceiveControlRegister.FifoReset = 1;
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);

    m_ulChannel = 0;
}

#pragma code_seg()
VOID
CDmaBuffer::FillFifos()
{
    SAI_TRANSMIT_FIFO_REGISTER FifoControl;
    ULONG sampleIndex;

    FifoControl.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitFifoRegister.AsUlong);

    // If Read Fifo Pointer and Write Fifo Pointer are identical except for MSB, the FIFO is full
    while(!((FIFO_PTR_NOMSB(FifoControl.ReadFifoPointer) == FIFO_PTR_NOMSB(FifoControl.WriteFifoPointer)) &&
            (FIFO_PTR_MSB(FifoControl.ReadFifoPointer) != FIFO_PTR_MSB(FifoControl.WriteFifoPointer))))
    {
        // Samples consist of n values where n is number of channels, Left first.
        ULONG sample;

        sampleIndex = GetSampleIndex();
        sample = m_DataBuffer[sampleIndex + m_ulChannel];

        WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitDataRegister.AsUlong, sample);

        m_ulChannel += 1;
        if (m_ulChannel >= m_pWfExt->Format.nChannels) {
            m_ulChannel = 0;
            m_ulSamplesTransferred += 1;
        }

        FifoControl.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitFifoRegister.AsUlong);
    }

    m_pRtStream->UpdateVirtualPositionRegisters(m_ulSamplesTransferred);
}

#pragma code_seg()
VOID
CDmaBuffer::DrainFifos()
{
    SAI_RECEIVE_FIFO_REGISTER FifoControl;

    FifoControl.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveFifoRegister.AsUlong);

    ULONG sampleIndex;
    ULONG sample;

    while(FifoControl.ReadFifoPointer != FifoControl.WriteFifoPointer) 
    {
        // pull sample out of the fifo
        sample = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveDataRegister.AsUlong);
        sampleIndex = GetSampleIndex();

        // Mask to 24-bit depth, MSB at bit 31
        sample = sample & 0xffffff00;

        if (m_ulChannel == 0) {
            m_DataBuffer[sampleIndex] = sample;
         
            m_ulSamplesTransferred += 1;
        }

        m_ulChannel = 0x01 & (m_ulChannel + 1);

        FifoControl.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveFifoRegister.AsUlong);
    } 

    m_pRtStream->UpdateVirtualPositionRegisters(m_ulSamplesTransferred);
}


#pragma code_seg()
VOID
CDmaBuffer::RegisterStream
(
    _In_        CMiniportWaveRTStream* Stream,
                eDeviceType DeviceType
)
{
    m_pRtStream = Stream;
    m_DeviceType = DeviceType;
    m_ulSamplesTransferred = 0;
    m_ulChannel = 0;
    m_DataBuffer = Stream->GetDmaBuffer();
    m_ulDmaBufferSize = Stream->GetDmaBufferSize();
    m_pWfExt = Stream->GetDataFormat();
}

#pragma code_seg()
VOID
CDmaBuffer::UnregisterStream
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    UNREFERENCED_PARAMETER(Stream);
    ASSERT(m_pRtStream == Stream);

    m_pRtStream = NULL;
    m_ulSamplesTransferred = 0;
    m_ulChannel = 0;
    m_DataBuffer = NULL;
    m_ulDmaBufferSize = 0;
    m_pWfExt = NULL;
}


CSoc::CSoc()
{

}

CSoc::~CSoc()
{
    if (m_pSaiRegisters != NULL)
    {
        MmUnmapIoSpace(m_pSaiRegisters, sizeof(SaiRegisters));
    }

    IO_DISCONNECT_INTERRUPT_PARAMETERS params;

    if (m_pInterruptObject != NULL)
    {
        params.Version = CONNECT_FULLY_SPECIFIED;
        params.ConnectionContext.InterruptObject = m_pInterruptObject;

        IoDisconnectInterruptEx(&params);
    }
}

NTSTATUS
CSoc::RegisterStream
(
    _In_        CMiniportWaveRTStream* Stream,
                eDeviceType DeviceType
)
{
    m_Buffer[DeviceType].RegisterStream(Stream, DeviceType);

    if (DeviceType == eSpeakerHpDevice)
    {
        m_bIsRenderActive = TRUE;
    }
    else
    {
        m_bIsCaptureActive = TRUE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CSoc::UnregisterStream
(
    _In_        CMiniportWaveRTStream* Stream,
                eDeviceType DeviceType
)
{
    m_Buffer[DeviceType].UnregisterStream(Stream);

    if (DeviceType == eSpeakerHpDevice)
    {
        m_bIsRenderActive = FALSE;
    }
    else
    {
        m_bIsCaptureActive = FALSE;
    }

    EnableInterrupts();

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
NTSTATUS
CSoc::SetupClocks()
{
    SAI_TRANSMIT_CONFIGURATION_REGISTER_1 TransmitConfigReg1;
    SAI_RECEIVE_CONFIGURATION_REGISTER_1 ReceiveConfigReg1;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_2 TransmitConfigReg2;
    SAI_RECEIVE_CONFIGURATION_REGISTER_2 ReceiveConfigReg2;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_3 TransmitConfigReg3;
    SAI_RECEIVE_CONFIGURATION_REGISTER_3 ReceiveConfigReg3;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_4 TransmitConfigReg4;
    SAI_RECEIVE_CONFIGURATION_REGISTER_4 ReceiveConfigReg4;
    SAI_TRANSMIT_CONFIGURATION_REGISTER_5 TransmitConfigReg5;
    SAI_RECEIVE_CONFIGURATION_REGISTER_5 ReceiveConfigReg5;

    SAI_TRANSMIT_MASK_REGISTER TransmitMaskRegister;
    SAI_RECEIVE_MASK_REGISTER ReceiveMaskRegister;

    IO_CONNECT_INTERRUPT_PARAMETERS parameters;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Reset SAI controller
    //

    TxSoftwareReset();
    RxSoftwareReset();

    //
    // Configure the RX/TX FIFO watermark settings in Config Reg 1
    //

    TransmitConfigReg1.AsUlong = 0;
    ReceiveConfigReg1.AsUlong = 0;

#ifdef _ARM64_
    TransmitConfigReg1.TransmitFifoWatermark = 0x2C;
    ReceiveConfigReg1.ReceiveFifoWatermark = 0xC;
#else
    TransmitConfigReg1.TransmitFifoWatermark = 0xC;
    ReceiveConfigReg1.ReceiveFifoWatermark = 0xC;
#endif

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitConfigRegister1.AsUlong, TransmitConfigReg1.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveConfigRegister1.AsUlong, ReceiveConfigReg1.AsUlong);

    //
    // Configure the RX/TX bit clock settings in Config Reg 2
    //

    TransmitConfigReg2.AsUlong = 0;
    ReceiveConfigReg2.AsUlong = 0;

    TransmitConfigReg2.SynchronousMode = 0; // Asynchronous Mode.
#ifdef _ARM64_
    TransmitConfigReg2.BitClockDivide = 0x1;  // bitclk = mclk / 4
#else
    TransmitConfigReg2.BitClockDivide = 0x3;  // bitclk = mclk / 16
#endif
    TransmitConfigReg2.BitClockDirection = 1; // internal bit clock
    TransmitConfigReg2.BitClockParity = 1;
    TransmitConfigReg2.MasterClockSelect = 1; // 0 or 1 makes no difference according to NXP.

    ReceiveConfigReg2.SynchronousMode = 0x1;  // Synchronous Mode, dependant on Transmitter.
#ifdef _ARM64_
    ReceiveConfigReg2.BitClockDivide = 0x1;
#else
    ReceiveConfigReg2.BitClockDivide = 0x3;
#endif
    ReceiveConfigReg2.BitClockDirection = 1;
    ReceiveConfigReg2.BitClockPolarity = 1;
    ReceiveConfigReg2.MasterClockSelect = 1; // 0 or 1 makes no difference according to NXP.

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitConfigRegister2.AsUlong, TransmitConfigReg2.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveConfigRegister2.AsUlong, ReceiveConfigReg2.AsUlong);

    //
    // Configure the RX/TX bit clock settings in Config Reg 3
    //

    TransmitConfigReg3.AsUlong = 0;
    ReceiveConfigReg3.AsUlong = 0;

    TransmitConfigReg3.TransmitChannelEnable = 1;
    TransmitConfigReg3.WordFlagConfiguration = 0; // word 0 set start of word flag

    ReceiveConfigReg3.ReceiveChannelEnable = 1;
    ReceiveConfigReg3.WordFlagConfiguration = 0; // word 0 set start of word flag

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitConfigRegister3.AsUlong, TransmitConfigReg3.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveConfigRegister3.AsUlong, ReceiveConfigReg3.AsUlong);

    //
    // Configure the RX/TX Config Reg 4
    //

    TransmitConfigReg4.AsUlong = 0;
    ReceiveConfigReg4.AsUlong = 0;

    TransmitConfigReg4.FrameSize = 1;       // two words per frame (n-1)
#ifdef _ARM64_
    TransmitConfigReg4.ChannelMode = 1;     // don't tristate outputs when masked
#endif
    TransmitConfigReg4.SyncWidth = 0x1f;    // 32 bit word lengths
    TransmitConfigReg4.MSBFirst = 1;        // I2S MSB-First left-1 Justified
    TransmitConfigReg4.FrameSyncEarly = 1;
    TransmitConfigReg4.FrameSyncDirection = 1; // internally generated
    TransmitConfigReg4.FrameSyncPolarity = 1; // active low

    ReceiveConfigReg4.FrameSize = 1;        // two words per frame (n-1)
    ReceiveConfigReg4.SyncWidth = 0x1f;     // 32 bit word lengths
    ReceiveConfigReg4.MSBFirst = 1;         // I2S MSB-First left-1 Justified
    ReceiveConfigReg4.FrameSyncEarly = 1;
    ReceiveConfigReg4.FrameSyncDirection = 1;
    ReceiveConfigReg4.FrameSyncPolarity = 1;

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitConfigRegister4.AsUlong, TransmitConfigReg4.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveConfigRegister4.AsUlong, ReceiveConfigReg4.AsUlong);

    //
    // Configure the  RX/TX Config Reg 5
    //

    TransmitConfigReg5.AsUlong = 0;
    ReceiveConfigReg5.AsUlong = 0;

    TransmitConfigReg5.FirstBitShifted = 0x1f; // MSB in bit position 31
    TransmitConfigReg5.Word0Width = 0x1f;   // 32 bit word lengths
    TransmitConfigReg5.WordNWidth = 0x1f;

    ReceiveConfigReg5.FirstBitShifted = 0x1f;
    ReceiveConfigReg5.Word0Width = 0x1f;
    ReceiveConfigReg5.WordNWidth = 0x1f;

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitConfigRegister5.AsUlong, TransmitConfigReg5.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveConfigRegister5.AsUlong, ReceiveConfigReg5.AsUlong);

    //
    // Configure the RX/TX Mask register
    //
    
    TransmitMaskRegister.AsUlong = ~0x3u;   // mask off all but first two words
    ReceiveMaskRegister.AsUlong = ~0x3u;

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitMaskRegister.AsUlong, TransmitMaskRegister.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveMaskRegister.AsUlong, ReceiveMaskRegister.AsUlong);

    ASSERT(m_pDescriptor);

    parameters.Version = CONNECT_FULLY_SPECIFIED;
    parameters.FullySpecified.PhysicalDeviceObject = m_pPDO;
    parameters.FullySpecified.InterruptObject = &m_pInterruptObject;
    parameters.FullySpecified.ServiceRoutine = &CSoc::ISR;
    parameters.FullySpecified.ServiceContext = this;
    parameters.FullySpecified.SpinLock = NULL;
    parameters.FullySpecified.SynchronizeIrql = (KIRQL) m_pDescriptor->u.Interrupt.Level;
    parameters.FullySpecified.FloatingSave = FALSE;
    parameters.FullySpecified.ShareVector = FALSE;
    parameters.FullySpecified.Vector = m_pDescriptor->u.Interrupt.Vector;
    parameters.FullySpecified.Irql = (KIRQL) m_pDescriptor->u.Interrupt.Level;
    parameters.FullySpecified.InterruptMode = LevelSensitive;
    parameters.FullySpecified.ProcessorEnableMask = m_pDescriptor->u.Interrupt.Affinity;
    parameters.FullySpecified.Group = 0;

    status = IoConnectInterruptEx(&parameters);

    EnableInterrupts();

    ASSERT(NT_SUCCESS(status));
    ASSERT(m_pInterruptObject);

    return status;
}

#pragma code_seg()
VOID
CSoc::DisableInterruptsNoLock()
{
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;

    TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
    ReceiveControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong);

    // Clear the interrupt flags and don't reset any error flags yet
    TransmitControlRegister.AsUlong &= ~(SAI_CTRL_INTERRUPTMASK | SAI_CTRL_ERRORFLAGS);
    ReceiveControlRegister.AsUlong &= ~(SAI_CTRL_INTERRUPTMASK | SAI_CTRL_ERRORFLAGS);

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);
}

#pragma code_seg()
VOID
CSoc::DisableInterrupts()
{
    KIRQL irql;

    irql = AcquireIsrSpinLock();

    DisableInterruptsNoLock();

    ReleaseIsrSpinLock(irql);
}

#pragma code_seg()
VOID
CSoc::EnableInterruptsNoLock()
{
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;

    TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
    ReceiveControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong);

    if (m_bIsRenderActive == TRUE)
    {
        TransmitControlRegister.FifoRequestInterruptEnable = 1;
    }

    if (m_bIsCaptureActive == TRUE)
    {
        ReceiveControlRegister.FifoRequestInterruptEnable = 1;
    }

    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);
}


#pragma code_seg()
VOID
CSoc::EnableInterrupts()
{
    KIRQL irql;

    irql = AcquireIsrSpinLock();

    EnableInterruptsNoLock();

    ReleaseIsrSpinLock(irql);
}

VOID
CSoc::TxSoftwareReset()
{
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;

    TransmitControlRegister.AsUlong = 0;

    TransmitControlRegister.SoftwareReset = 1;
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

    TransmitControlRegister.SoftwareReset = 0;
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);
}

VOID
CSoc::RxSoftwareReset()
{
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;

    ReceiveControlRegister.AsUlong = 0;

    ReceiveControlRegister.SoftwareReset = 1;
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);

    ReceiveControlRegister.SoftwareReset = 0;
    WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);
}

NTSTATUS
CSoc::StartDma
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;
    KIRQL irql;

    irql = AcquireIsrSpinLock();

    cntIsr = 0;
    cntTxFifoError = 0;
    cntTxFifoWarning = 0;
    cntTxFifoRequest = 0;
    cntTxSyncError = 0;
    cntRxFifoError = 0;
    cntRxFifoWarning = 0;
    cntRxFifoRequest = 0;
    cntRxSyncError = 0;

    if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        RxSoftwareReset();
        TxSoftwareReset();
    }
    else if (m_Buffer[eSpeakerHpDevice].IsMyStream(Stream))
    {
        TxSoftwareReset();
    }

    if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        m_Buffer[eMicInDevice].ResetRxFifo();
        ReceiveControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong);
        ReceiveControlRegister.ReceiverEnable = 1;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);

        // Rx is synchronous on Tx, so Tx must be enabled
        TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
        TransmitControlRegister.TransmitterEnable = 1;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

        m_bIsCaptureActive = TRUE;
    }
    else if (m_Buffer[eSpeakerHpDevice].IsMyStream(Stream))
    {
        m_Buffer[eSpeakerHpDevice].ResetTxFifo();
        m_Buffer[eSpeakerHpDevice].FillFifos();

        TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
        TransmitControlRegister.TransmitterEnable = 1;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

        m_bIsRenderActive = TRUE;
    }

    ReleaseIsrSpinLock(irql);

    EnableInterrupts();


    return STATUS_SUCCESS;
}

NTSTATUS
CSoc::StopDma
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;
    KIRQL irql;

    irql = AcquireIsrSpinLock();

    if (m_Buffer[eSpeakerHpDevice].IsMyStream(Stream))
    {
        TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
        TransmitControlRegister.TransmitterEnable = 0;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

        m_bIsRenderActive = FALSE;
    } 
    else if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        // Rx is synchronous on Tx, so Tx must be disabled first.
        TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
        TransmitControlRegister.TransmitterEnable = 0;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

        ReceiveControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong);
        ReceiveControlRegister.ReceiverEnable = 0;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);

        m_Buffer[eMicInDevice].DrainFifos();

        m_bIsCaptureActive = FALSE;
    }

    ReleaseIsrSpinLock(irql);

    // EnableInterrupts will properly set the enables based on the active streams (if any.)
    EnableInterrupts();

    return STATUS_SUCCESS;
}


#pragma code_seg()
NTSTATUS
CSoc::PauseDma
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    KIRQL irql;
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;

    DisableInterrupts();

    irql = AcquireIsrSpinLock();

    if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        m_bIsCaptureActive = FALSE;
    }
    else if (m_Buffer[eSpeakerHpDevice].IsMyStream(Stream))
    {
        TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
        TransmitControlRegister.TransmitterEnable = 0;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

        m_bIsRenderActive = FALSE;
    }

    if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
        TransmitControlRegister.TransmitterEnable = 0;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong, TransmitControlRegister.AsUlong);

        ReceiveControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong);
        ReceiveControlRegister.ReceiverEnable = 0;
        WRITE_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong, ReceiveControlRegister.AsUlong);

        m_Buffer[eMicInDevice].DrainFifos();

        m_bIsCaptureActive = FALSE;
    }

    ReleaseIsrSpinLock(irql);

    return STATUS_SUCCESS;
}

#pragma code_seg()
BOOLEAN
CSoc::DoIsrWork ()
{

    BOOLEAN isMe;
    SAI_TRANSMIT_CONTROL_REGISTER TransmitControlRegister;
    SAI_RECEIVE_CONTROL_REGISTER ReceiveControlRegister;

    TransmitControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->TransmitControlRegister.AsUlong);
    ReceiveControlRegister.AsUlong = READ_REGISTER_ULONG(&m_pSaiRegisters->ReceiveControlRegister.AsUlong);

    isMe = ((ReceiveControlRegister.FifoWarningFlag == 1 || ReceiveControlRegister.FifoRequestFlag == 1 ||
             ReceiveControlRegister.FifoErrorFlag == 1 || ReceiveControlRegister.SyncErrorFlag == 1) ||
            (TransmitControlRegister.FifoWarningFlag == 1 || TransmitControlRegister.FifoRequestFlag == 1 ||
             TransmitControlRegister.FifoErrorFlag == 1 || TransmitControlRegister.SyncErrorFlag == 1));

    DisableInterruptsNoLock();

    cntIsr ++;

    if (ReceiveControlRegister.FifoErrorFlag)
    {
        cntRxFifoError ++;
        m_Buffer[eMicInDevice].ResetRxFifo();
    }

    if (TransmitControlRegister.FifoErrorFlag)
    {
        cntTxFifoError ++;
        m_Buffer[eSpeakerHpDevice].ResetTxFifo();
    }

    if (TransmitControlRegister.SyncErrorFlag)
        cntTxSyncError ++;

    if (TransmitControlRegister.FifoRequestFlag)
        cntTxFifoRequest ++;

    if (TransmitControlRegister.FifoWarningFlag)
        cntTxFifoWarning ++;

    if (ReceiveControlRegister.SyncErrorFlag)
        cntRxSyncError ++;

    if (ReceiveControlRegister.FifoRequestFlag)
        cntRxFifoRequest ++;

    if (ReceiveControlRegister.FifoWarningFlag)
        cntRxFifoWarning ++;

    if ((ReceiveControlRegister.FifoWarningFlag == 1 || ReceiveControlRegister.FifoRequestFlag == 1) && m_bIsCaptureActive == TRUE)
    {
        m_Buffer[eMicInDevice].DrainFifos();
    }

    if ((TransmitControlRegister.FifoWarningFlag == 1 || TransmitControlRegister.FifoRequestFlag == 1) && m_bIsRenderActive == TRUE)
    {
        m_Buffer[eSpeakerHpDevice].FillFifos();
    }

    // Enable interrupts.  Also clears any error flags
    EnableInterruptsNoLock();

    return isMe;
}

#pragma code_seg()
BOOLEAN
CSoc::ISR (
    _In_ struct _KINTERRUPT *Interrupt,
    _In_opt_ PVOID ServiceContext
)
{
    UNREFERENCED_PARAMETER(Interrupt);

    CSoc* me = (CSoc*) ServiceContext;
    return me->DoIsrWork();
}


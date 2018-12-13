/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    CSoc class implementation.

*/

#include "soc.h"


#pragma code_seg()
VOID
CDmaBuffer::FillFifos()
{
    FIFO_CONTROL_REGISTER FifoControl;
    ULONG sampleIndex;

    FifoControl.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->FifoControlRegister.u.AsUlong);

    while(FifoControl.u.Fields.TransmitFifoCounter0 != 0xf && FifoControl.u.Fields.TransmitFifoCounter1 != 0xf)
    {
        //
        // We must interleave samples in between FIFO 0 and FIFO 1.
        // 
        // WASAPI requires that both left and right channels be handled at a time and we don't support mono playback.
        //

        sampleIndex = GetSampleIndex();

        WRITE_REGISTER_ULONG(&m_pSsiRegisters->TransmitDataRegister0, m_DataBuffer[sampleIndex] >> 8);
        
        WRITE_REGISTER_ULONG(&m_pSsiRegisters->TransmitDataRegister1, m_DataBuffer[sampleIndex + 1] >> 8);
     
        m_ulSamplesTransferred += 1;

        FifoControl.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->FifoControlRegister.u.AsUlong);
    }

    m_pRtStream->UpdateVirtualPositionRegisters(m_ulSamplesTransferred);
}

#pragma code_seg()
VOID
CDmaBuffer::DrainFifos()
{
    FIFO_CONTROL_REGISTER FifoControl;

    FifoControl.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->FifoControlRegister.u.AsUlong);

    ULONG sampleIndex;
    ULONG audiosample[4];
    ULONG sample1, sample2;

    //
    // We capture in mono, but we must drain both fifos since we render in stereo and the tx clock and rx clock are shared.
    // For some reason, the soc appears to populate the fifos in an odd fashion, so we are heuristically looking for two
    // consequtive non-zero samples (or a non-zero sample paired with a zero sample) every 4 samples.
    // 

    while(FifoControl.u.Fields.ReceiveFifoCounter0 > 1 && FifoControl.u.Fields.ReceiveFifoCounter1 > 1) 
    {
        // Step 1: pull 4 samples out of the fifos.

        audiosample[0] = READ_REGISTER_ULONG(&m_pSsiRegisters->ReceiveDataRegister0);
        audiosample[1] = READ_REGISTER_ULONG(&m_pSsiRegisters->ReceiveDataRegister1);
        audiosample[2] = READ_REGISTER_ULONG(&m_pSsiRegisters->ReceiveDataRegister0);
        audiosample[3] = READ_REGISTER_ULONG(&m_pSsiRegisters->ReceiveDataRegister1);

        // Step 2a: if two consecutive samples are non-zero, we have found our sample pair.
        // Step 2b: look for the first non-zero sample.
        // Step 2c: we must have 2 samples of zero.

        if (audiosample[0] != 0 && audiosample[1] != 0)
        {
            sample1 = audiosample[0];
            sample2 = audiosample[1];
        } 
        else if (audiosample[1] != 0 && audiosample[2] != 0)
        {
            sample1 = audiosample[1];
            sample2 = audiosample[2];
        } 
        else if (audiosample[2] != 0 && audiosample[3] != 0)
        {
            sample1 = audiosample[2];
            sample2 = audiosample[3];
        } 
        else if (audiosample[0] != 0 && audiosample[3] != 0)
        {
            // wrap case - samples need to be reordered.
            sample1 = audiosample[3];
            sample2 = audiosample[0];
        } 
        else if (audiosample[0] != 0)   // begin step 2b - find the first non-zero sample.
        {
            sample1 = audiosample[0];
            sample2 = 0;
        } 
        else if (audiosample[1] != 0)
        {
            sample1 = audiosample[0];
            sample2 = 0;
        } 
        else if (audiosample[2] != 0)
        {
            sample1 = audiosample[0];
            sample2 = 0;
        } 
        else if (audiosample[3] != 0)
        {
            sample1 = audiosample[3];
            sample2 = 0;
        } 
        else
        {
            // step 2c - two zero samples.
            sample1 = 0;
            sample2 = 0;
        }

        sampleIndex = GetSampleIndex();

        sample1 = sample1 << 8;
        sample1 = sample1 & 0xffffff00;

        m_DataBuffer[sampleIndex] = sample1;
     
        m_ulSamplesTransferred += 1;

        sampleIndex = GetSampleIndex();

        sample2 = sample2 << 8;
        sample2 = sample2 & 0xffffff00;

        m_DataBuffer[sampleIndex] = sample2;
     
        m_ulSamplesTransferred += 1;
    
        FifoControl.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->FifoControlRegister.u.AsUlong);     
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
    m_DataBuffer = NULL;
    m_ulDmaBufferSize = 0;
    m_pWfExt = NULL;
}


CSoc::CSoc()
{

}

CSoc::~CSoc()
{
    if (m_pSsiRegisters != NULL)
    {
        MmUnmapIoSpace(m_pSsiRegisters, sizeof(SsiRegisters));
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
    TRANSMIT_CLOCK_CONTROL_REGISTER TransmitClockControlReg;
    TRANSMIT_CONFIG_REGISTER TransmitConfig;
    RECEIVE_CONFIG_REGISTER ReceiveConfig;
    FIFO_CONTROL_REGISTER FifoControl;
    SSI_CONTROL_REGISTER ControlRegister;
    INTERRUPT_ENABLE_REGISTER InterruptEnableRegister;

    IO_CONNECT_INTERRUPT_PARAMETERS parameters;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Clear interrupt enables that are powered on by default that we don't care about.
    //
    InterruptEnableRegister.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->InterruptEnableRegister.u.AsUlong);

    InterruptEnableRegister.u.Fields.TxFifoEmpty1En = 0;
    InterruptEnableRegister.u.Fields.TxDataRegisterEmpty0En = 0;
    InterruptEnableRegister.u.Fields.TxDataRegisterEmpty1En = 0;

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->InterruptEnableRegister.u.AsUlong, InterruptEnableRegister.u.AsUlong);

    //
    // Setup the transmit clock register in SSI
    //

    TransmitClockControlReg.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->TransmitClockControlRegister.u.AsUlong);

    TransmitClockControlReg.u.Fields.DivideByTwo = 0;
    TransmitClockControlReg.u.Fields.PrescalerRange = 0;
    TransmitClockControlReg.u.Fields.WordLengthControl = WLC_24_BIT;
    TransmitClockControlReg.u.Fields.FrameRateDivider = 3; 
    TransmitClockControlReg.u.Fields.PrescalerModulusSelect = 5;

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->TransmitClockControlRegister.u.AsUlong, TransmitClockControlReg.u.AsUlong);

    // We only enable the interrupt for FIFO 0 since FIFO 1 should fire at approximately the same time.

    FifoControl.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->FifoControlRegister.u.AsUlong);

    FifoControl.u.Fields.TransmitFifoWaterMark0 = 8; // 50%
    FifoControl.u.Fields.ReceiveFifoWaterMark0 = 8; // 50%

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->FifoControlRegister.u.AsUlong, FifoControl.u.AsUlong);

    TransmitConfig.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->TransmitConfigRegister.u.AsUlong);

    TransmitConfig.u.Fields.TxBit0 =  MSB_ALIGNED;
    TransmitConfig.u.Fields.FifoEnable1 =  1;
    TransmitConfig.u.Fields.FifoEnable0 =  1;
    TransmitConfig.u.Fields.FrameDirection =  TCRDIR_INTERNAL;
    TransmitConfig.u.Fields.ClockDirection =  TCRDIR_INTERNAL;
    TransmitConfig.u.Fields.ShiftDirection =  MSB_FIRST;
    TransmitConfig.u.Fields.ClockPolarity =  FALLING_EDGE;
    TransmitConfig.u.Fields.FrameSyncInvert =  1;
    TransmitConfig.u.Fields.FrameSyncLength =  0;
    TransmitConfig.u.Fields.EarlyFrameSync =  ONE_BIT_BEFORE;

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->TransmitConfigRegister.u.AsUlong, TransmitConfig.u.AsUlong);

    ReceiveConfig.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->ReceiveConfigRegister.u.AsUlong);

    ReceiveConfig.u.Fields.RxBit0 = MSB_ALIGNED;
    ReceiveConfig.u.Fields.FifoEnable0 = 1;
    ReceiveConfig.u.Fields.FifoEnable1 = 1;
    ReceiveConfig.u.Fields.ShiftDirection =  MSB_FIRST;
    ReceiveConfig.u.Fields.ClockPolarity =  FALLING_EDGE;
    ReceiveConfig.u.Fields.FrameSyncInvert =  1;
    ReceiveConfig.u.Fields.FrameSyncLength =  0;
    ReceiveConfig.u.Fields.EarlyFrameSync =  ONE_BIT_BEFORE;

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->ReceiveConfigRegister.u.AsUlong, ReceiveConfig.u.AsUlong);

    ControlRegister.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->ControlRegister.u.AsUlong);

    ControlRegister.u.Fields.SsiEnable = 1;
    ControlRegister.u.Fields.NetworkMode = 0;
    ControlRegister.u.Fields.SynchronousMode = 1;
    ControlRegister.u.Fields.TwoChannelEnable = 1;
    ControlRegister.u.Fields.I2sMode = I2S_MODE_MASTER;

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->ControlRegister.u.AsUlong, ControlRegister.u.AsUlong);

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
  	
    ASSERT(NT_SUCCESS(status));
    ASSERT(m_pInterruptObject);

    return status;
}

#pragma code_seg()
VOID
CSoc::DisableInterruptsNoLock()
{
    WRITE_REGISTER_ULONG(&m_pSsiRegisters->InterruptEnableRegister.u.AsUlong, 0);
}

#pragma code_seg()
VOID
CSoc::DisableInterrupts()
{
    KIRQL irql;

    irql = AcquireIsrSpinLock();

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->InterruptEnableRegister.u.AsUlong, 0);

    ReleaseIsrSpinLock(irql);
}

#pragma code_seg()
VOID
CSoc::EnableInterruptsNoLock()
{
    INTERRUPT_ENABLE_REGISTER InterruptEnableRegister;
    InterruptEnableRegister.u.AsUlong = 0;

    if (m_bIsRenderActive == TRUE)
    {
        InterruptEnableRegister.u.Fields.TxFifoEmpty0En = 1;
        InterruptEnableRegister.u.Fields.TxInterruptEn = 1;    
    }

    if (m_bIsCaptureActive == TRUE)
    {
        InterruptEnableRegister.u.Fields.RxFifoFull0En = 1;
        InterruptEnableRegister.u.Fields.RxInterruptEn = 1;    
    }

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->InterruptEnableRegister.u.AsUlong, InterruptEnableRegister.u.AsUlong);
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



NTSTATUS
CSoc::StartDma
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    SSI_CONTROL_REGISTER ControlRegister;
    KIRQL irql;

    irql = AcquireIsrSpinLock();

    ControlRegister.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->ControlRegister.u.AsUlong);

    if (m_Buffer[eSpeakerHpDevice].IsMyStream(Stream))
    {
        m_Buffer[eSpeakerHpDevice].FillFifos();
        ControlRegister.u.Fields.TransmitEnable = 1;
        WRITE_REGISTER_ULONG(&m_pSsiRegisters->ControlRegister.u.AsUlong, ControlRegister.u.AsUlong);
        m_bIsRenderActive = TRUE;
        
    }

    if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        ControlRegister.u.Fields.ReceiveEnable = 1;
        WRITE_REGISTER_ULONG(&m_pSsiRegisters->ControlRegister.u.AsUlong, ControlRegister.u.AsUlong);
        m_bIsCaptureActive = TRUE;
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
    SSI_CONTROL_REGISTER ControlRegister;
    KIRQL irql;

    irql = AcquireIsrSpinLock();

    ControlRegister.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->ControlRegister.u.AsUlong);

    if (m_Buffer[eSpeakerHpDevice].IsMyStream(Stream))
    {
        ControlRegister.u.Fields.TransmitEnable = 0;

        m_bIsRenderActive = FALSE;
    } 
    else if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        ControlRegister.u.Fields.ReceiveEnable = 0;

        m_Buffer[eMicInDevice].DrainFifos();

        m_bIsCaptureActive = FALSE;
    }

    WRITE_REGISTER_ULONG(&m_pSsiRegisters->ControlRegister.u.AsUlong, ControlRegister.u.AsUlong);

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

    DisableInterrupts();

    irql = AcquireIsrSpinLock();

    if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        m_bIsCaptureActive = FALSE;
    }
    else
    {
        m_bIsRenderActive = FALSE;
    }

    if (m_Buffer[eMicInDevice].IsMyStream(Stream))
    {
        m_Buffer[eMicInDevice].DrainFifos();
    }

    ReleaseIsrSpinLock(irql);

    return STATUS_SUCCESS;
}

#pragma code_seg()
BOOLEAN
CSoc::DoIsrWork ()
{

    BOOLEAN isMe;
    INTERRUPT_STATUS_REGISTER IsrReg;

    IsrReg.u.AsUlong = READ_REGISTER_ULONG(&m_pSsiRegisters->InterruptStatusRegister.u.AsUlong);

    isMe = (IsrReg.u.Fields.RxFifoFull0 == 1) || (IsrReg.u.Fields.TxFifoEmpty0 == 1);

    DisableInterruptsNoLock();

    if (IsrReg.u.Fields.RxFifoFull0 == 1 && m_bIsCaptureActive == TRUE)
    {
        m_Buffer[eMicInDevice].DrainFifos();
    }

    if (IsrReg.u.Fields.TxFifoEmpty0 == 1 && m_bIsRenderActive == TRUE)
    {
        m_Buffer[eSpeakerHpDevice].FillFifos();
    }

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


/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

//4127: conditional expression is constant
//4595: illegal inline operator
#pragma warning (disable : 4127)
#pragma warning (disable : 4595)

#include "imx_audio.h"
#include <limits.h>
#include <ks.h>
#include <ntddk.h>
#include "simple.h"
#include "minwavert.h"
#include "minwavertstream.h"
#define MINWAVERTSTREAM_POOLTAG 'SBRB'

//=============================================================================
// CMiniportWaveRTStream
//=============================================================================

//=============================================================================
#pragma code_seg("PAGE")
CMiniportWaveRTStream::~CMiniportWaveRTStream
( 
    VOID 
)
/*++

Routine Description:

    Destructor for CMiniportWaveRTStream 

Arguments:

    None

Return Value:

    None

--*/
{
    PAGED_CODE();

    if (NULL != m_pMiniport)
    {
        if (m_bUnregisterStream)
        {
            m_pMiniport->StreamClosed(m_ulPin, this);
            m_bUnregisterStream = FALSE;
        }

        m_pMiniport->Release();
        m_pMiniport = NULL;
    }

    if (m_pWfExt)
    {
        ExFreePoolWithTag( m_pWfExt, MINWAVERTSTREAM_POOLTAG );
        m_pWfExt = NULL;
    }

    //
    // Wait for all queued DPCs.
    //
    KeFlushQueuedDpcs();

    DPF_ENTER(("[CMiniportWaveRTStream::~CMiniportWaveRTStream]"));
} 

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS
CMiniportWaveRTStream::Init
( 
    PCMiniportWaveRT           Miniport,
    PPORTWAVERTSTREAM          PortStream,
    ULONG                      Pin,
    BOOLEAN                    Capture,
    PKSDATAFORMAT              DataFormat,
    GUID                       SignalProcessingMode
)
/*++

Routine Description:

    Initializes the stream object.

Arguments:

    Miniport - pointer to the miniport class this stream belongs to

    Pin - pin number of this stream

    Capture - flag if this is capture or playback stream

    DataFormat - dataformat of this stream

    SignalProcessingMode - used to configure hardware specific processing

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    PWAVEFORMATEX pWfEx = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DECLARE_UNICODE_STRING_SIZE(pwmDeviceName, 128);

    DPF_ENTER(("[CMiniportWaveRTStream::Init]"));

    m_pMiniport = NULL;
    m_ulPin = 0;
    m_bUnregisterStream = FALSE;
    m_ulDmaBufferSize = 0;
    m_DataBuffer = NULL;
    m_KsState = KSSTATE_STOP;
    m_ulNotificationsPerBuffer = 0;

    m_ulSamplesTransferred = 0;
    m_pWfExt = NULL;
    m_SignalProcessingMode = SignalProcessingMode;

    m_pPortStream = PortStream;

    m_pAdapterCommon = Miniport->GetAdapterCommObj();

    KeQueryPerformanceCounter(&m_PerformanceCounterFrequency);

    InitializeListHead(&m_NotificationList);

    pWfEx = GetWaveFormatEx(DataFormat);
    if (NULL == pWfEx) 
    { 
        return STATUS_UNSUCCESSFUL; 
    }

    m_pMiniport = reinterpret_cast<CMiniportWaveRT*>(Miniport);
    if (m_pMiniport == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }
    m_pMiniport->AddRef();

    m_ulPin = Pin;

    m_bCapture = Capture;

    KeInitializeDpc(&m_Dpc, &CMiniportWaveRTStream::DpcCallback, this);

    KeInitializeSpinLock(&m_Lock);

    m_pWfExt = (PWAVEFORMATEXTENSIBLE)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(WAVEFORMATEX) + pWfEx->cbSize, MINWAVERTSTREAM_POOLTAG);
    if (m_pWfExt == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(m_pWfExt, pWfEx, sizeof(WAVEFORMATEX) + pWfEx->cbSize);

    //
    // Register this stream.
    //
    ntStatus = m_pMiniport->StreamCreated(m_ulPin, this);
    if (NT_SUCCESS(ntStatus))
    {
        m_bUnregisterStream = TRUE;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CMiniportWaveRTStream::NonDelegatingQueryInterface
( 
    REFIID  Interface,
    PVOID * Object
)
/*++

Routine Description:

    Returns the interface, if it is supported.

Arguments:

    Interface - interface GUID

    Object - interface pointer to be returned

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTWAVERTSTREAM(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveRTStream))
    {
        *Object = PVOID(PMINIPORTWAVERTSTREAM(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveRTStreamNotification))
    {
        *Object = PVOID(PMINIPORTWAVERTSTREAMNOTIFICATION(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
CMiniportWaveRTStream::AllocateBufferCommon
(
    ULONG               NotificationCount,
    ULONG               RequestedSize,
    PMDL                *AudioBufferMdl,
    ULONG               *ActualSize,
    ULONG               *OffsetFromFirstPage,
    MEMORY_CACHING_TYPE *CacheType
)
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    *AudioBufferMdl = NULL;
    *ActualSize = 0;
    *OffsetFromFirstPage = 0;
    *CacheType = MmNotMapped;

    RequestedSize -= RequestedSize % (m_pWfExt->Format.nBlockAlign);

    PHYSICAL_ADDRESS highAddress;
    highAddress.HighPart = 0;
    highAddress.LowPart = MAXULONG;

    PMDL pBufferMdl = m_pPortStream->AllocatePagesForMdl (highAddress, RequestedSize);

    if (NULL == pBufferMdl)
    {
        return STATUS_UNSUCCESSFUL;
    }

    m_DataBuffer = (ULONG*)m_pPortStream->MapAllocatedPages(pBufferMdl, MmCached);
    if (m_DataBuffer)
    {
        m_ulNotificationsPerBuffer = NotificationCount;
        m_ulDmaBufferSize = RequestedSize;

        ntStatus = m_pAdapterCommon->RegisterStream(this, m_pMiniport->GetDeviceType());

        if (NT_SUCCESS(ntStatus))
        {
            *AudioBufferMdl = pBufferMdl;
            *ActualSize = RequestedSize;
            *OffsetFromFirstPage = 0;
            *CacheType = MmCached;
        }
        else
        {
            m_pPortStream->FreePagesFromMdl(pBufferMdl);
            m_ulDmaBufferSize = 0;
            m_DataBuffer = NULL;
            DPF(D_ERROR, ("[CMiniportWaveRTStream::AllocateBufferWithNotification] failed to register stream with CSoc class."));
        }

    }
    else
    {
        DPF(D_ERROR, ("[CMiniportWaveRTStream::AllocateBufferWithNotification] Could not allocate buffer for audio."));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;

}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::AllocateBufferWithNotification
(
    ULONG               NotificationCount,
    ULONG               RequestedSize,
    PMDL                *AudioBufferMdl,
    ULONG               *ActualSize,
    ULONG               *OffsetFromFirstPage,
    MEMORY_CACHING_TYPE *CacheType
)
/*++

Routine Description:

    Allocates a buffer the audio stack is writing the data in.

Arguments:

    NotificationCount - number of notifications to generate per buffer

    RequestedSize - size of the buffer to allocate

    AudioBufferMdl - ptr to a MDL describing the buffer

    ActualSize - size of the allocated buffer

    OffsetFromFirstPage - offset of the buffer start in the first page of the MDL

    CacheType - caching used for the buffer

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    if ( (0 == RequestedSize) || (RequestedSize < m_pWfExt->Format.nBlockAlign) )
    { 
        return STATUS_UNSUCCESSFUL; 
    }
    
   if ((NotificationCount == 0) || (RequestedSize % NotificationCount != 0))
   {
       return STATUS_INVALID_PARAMETER;
   }

   return AllocateBufferCommon(NotificationCount, RequestedSize, AudioBufferMdl, ActualSize, OffsetFromFirstPage, CacheType);
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
VOID 
CMiniportWaveRTStream::FreeBufferWithNotification
(
    PMDL    Mdl,
    ULONG   Size
)
/*++

Routine Description:

    Frees the audio buffer.

Arguments:

    Mdl - MDL of the buffer to free
    
    Size - size of the buffer to free
    
Return Value:

    None

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Size);

    m_pAdapterCommon->UnregisterStream(this, m_pMiniport->GetDeviceType());

    if (Mdl != NULL)
    {
        if (m_DataBuffer != NULL)
        {
            m_pPortStream->UnmapAllocatedPages(m_DataBuffer, Mdl);
            m_DataBuffer = NULL;
        }
        
        m_pPortStream->FreePagesFromMdl(Mdl);
        Mdl = NULL;
    }

    m_ulNotificationsPerBuffer = 0;
    m_ulDmaBufferSize = 0;

    return;
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::RegisterNotificationEvent
(
    PKEVENT NotificationEvent
)
/*++

Routine Description:

    Registers a notification event.

Arguments:

    NotificationEvent - Notification event which should be signaled

Return Value:

    NT status code

--*/
{
 

    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;

    NOTIFICATION_LIST_ENTRY *notification = (PNOTIFICATION_LIST_ENTRY)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof(NOTIFICATION_LIST_ENTRY),
        MINWAVERT_POOLTAG);
    if (NULL == notification)
    {
        DPF(D_VERBOSE, ("[CMiniportWaveRTStream::RegisterNotificationEvent] Insufficient resources for notification"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        notification->NotificationEvent = NotificationEvent;

        KeAcquireSpinLock(&m_Lock, &irql);

        if (!IsListEmpty(&m_NotificationList))
        {
            PLIST_ENTRY currentNotificationListEntry = m_NotificationList.Flink;
            while (currentNotificationListEntry != &m_NotificationList)
            {
                PNOTIFICATION_LIST_ENTRY currentNotification = CONTAINING_RECORD(currentNotificationListEntry, NOTIFICATION_LIST_ENTRY, ListEntry);
                if (currentNotification->NotificationEvent == NotificationEvent)
                {
                    RemoveEntryList(currentNotificationListEntry);
                    ExFreePoolWithTag(notification, MINWAVERT_POOLTAG);
                    KeReleaseSpinLock(&m_Lock, irql);
                    return STATUS_UNSUCCESSFUL;
                }
                currentNotificationListEntry = currentNotificationListEntry->Flink;
            }
        }
        InsertTailList(&m_NotificationList, &(notification->ListEntry));

        DPF(D_VERBOSE, ("[CMiniportWaveRTStream::RegisterNotificationEvent] Notification event registered: 0x%p", NotificationEvent));

        KeReleaseSpinLock(&m_Lock, irql);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::UnregisterNotificationEvent
(
    PKEVENT NotificationEvent
)
/*++

Routine Description:

    Unregisters a notification event.

Arguments:

    NotificationEvent - Notification event to unregister

Return Value:

    NT status code

--*/
{
   

    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;

    KeAcquireSpinLock(&m_Lock, &irql);

    if (!IsListEmpty(&m_NotificationList))
    {
        PLIST_ENTRY currentNotificationListEntry = m_NotificationList.Flink;
        PLIST_ENTRY nextNotificationListEntry = NULL;
        while (currentNotificationListEntry != &m_NotificationList)
        {
            PNOTIFICATION_LIST_ENTRY currentNotfication = CONTAINING_RECORD(currentNotificationListEntry, NOTIFICATION_LIST_ENTRY, ListEntry);
            if (currentNotfication->NotificationEvent == NotificationEvent)
            {
                nextNotificationListEntry = currentNotificationListEntry->Flink;
                RemoveEntryList(currentNotificationListEntry);
                ExFreePoolWithTag(currentNotfication, MINWAVERT_POOLTAG);
                DPF(D_VERBOSE, ("[CMiniportWaveRTStream::UnregisterNotificationEvent] Notification event (0x%p) unregistered", NotificationEvent));
                currentNotificationListEntry = nextNotificationListEntry;
            }
            else
            {
                currentNotificationListEntry = currentNotificationListEntry->Flink;
            }
        }
    }

    KeReleaseSpinLock(&m_Lock, irql);

    return ntStatus;
}


//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::GetClockRegister
(
    PKSRTAUDIO_HWREGISTER Register
)
/*++

Routine Description:

    Provides hardware clock regster information.

Arguments:

    Register - HW clock register info

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Register);

    DPF(D_TERSE, ("[CMiniportWaveRTStream::GetClockRegister] Not supported"));

    return STATUS_NOT_IMPLEMENTED;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::GetPositionRegister
(
    PKSRTAUDIO_HWREGISTER Register
)
/*++

Routine Description:

    Provides hardware position regster information.

    Register->Accuracy is the worst case (maximum) value that the byte position can increment.

Arguments:

    Register - HW position register info

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRTStream::GetPositionRegister]"));

    Register->Register = &m_ulVirtualPosition;
    Register->Width = 32; // bits.
    Register->Accuracy = 15 * m_pWfExt->Format.nBlockAlign;

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
VOID 
CMiniportWaveRTStream::GetHWLatency
(
    PKSRTAUDIO_HWLATENCY  Latency
)
/*++

Routine Description:

    Provides info on the latency introduced by the hardware.

Arguments:

    Latency - HW latency info

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    ASSERT(Latency);

    Latency->ChipsetDelay = 0;
    Latency->CodecDelay = 1 * _100NS_PER_MILLISECOND; // MSDN says to 'guess'.
    Latency->FifoSize = m_pWfExt->Format.nBlockAlign * 15; // Number of bytes in the fifo. This is block align * 15.
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
VOID 
CMiniportWaveRTStream::FreeAudioBuffer
(
    PMDL        Mdl,
    ULONG       Size
)
/*++

Routine Description:

    Frees a memory buffer.

Arguments:

    Mdl - MDL of the buffer to free
    
    Size - Size of the buffer to free

Return Value:

    None

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Mdl);
    UNREFERENCED_PARAMETER(Size);

    DPF(D_TERSE, ("[CMiniportWaveRTStream::FreeAudioBuffer] Not supported"));

    return;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::AllocateAudioBuffer
(
    ULONG                   RequestedSize,
    PMDL                   *AudioBufferMdl,
    ULONG                  *ActualSize,
    ULONG                  *OffsetFromFirstPage,
    MEMORY_CACHING_TYPE    *CacheType
)
/*++

Routine Description:

    Allocates a buffer the audio stack is writing the data in.

Arguments:

    RequestedSize - size of the buffer to allocate

    AudioBufferMdl - ptr to a MDL describing the buffer

    ActualSize - size of the allocated buffer

    OffsetFromFirstPage - offset of the buffer start in the first page of the MDL

    CacheType - caching used for the buffer

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    return AllocateBufferCommon(0, RequestedSize, AudioBufferMdl, ActualSize, OffsetFromFirstPage, CacheType);
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
NTSTATUS
CMiniportWaveRTStream::GetPosition
(
    KSAUDIO_POSITION    *Position
)
/*++

Routine Description:

    Returns the current stream playback/recording position as byte offset from the beginning of the buffer.

Arguments:

    Position - position of playback/recording

Return Value:

    NT status code

--*/
{
    //
    // We only support render and capture, so update PlayOffset and WriteOffset.
    //

    Position->PlayOffset = m_ulVirtualPosition;
    Position->WriteOffset = m_ulVirtualPosition;

    return STATUS_SUCCESS;
}

#pragma code_seg()
VOID 
CMiniportWaveRTStream::NotifyRegisteredEvents
(
    VOID
)
/*++

Routine Description:

    Notifies all registered events, which requests the next packets.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Notify all registered listeners.
    //

    KeAcquireSpinLockAtDpcLevel(&m_Lock);

    if (!IsListEmpty(&m_NotificationList))
    {
        PLIST_ENTRY currentNotificationListEntry = m_NotificationList.Flink;
        while (currentNotificationListEntry != &m_NotificationList)
        {
            PNOTIFICATION_LIST_ENTRY currentNotification = CONTAINING_RECORD(currentNotificationListEntry, NOTIFICATION_LIST_ENTRY, ListEntry);
            KeSetEvent(currentNotification->NotificationEvent, 0, FALSE);
            currentNotificationListEntry = currentNotificationListEntry->Flink;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&m_Lock);

}


//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::SetState
(
    KSSTATE State
)
/*++

Routine Description:

    Sets the stream state.

Arguments:

    State - new stream state to set

Return Value:

    NT status code

--*/
{
    NTSTATUS        ntStatus        = STATUS_SUCCESS;

    // Spew an event for a pin state change request from portcls
    // Event type: eMINIPORT_PIN_STATE
    // Parameter 1: Current linear buffer position	
    // Parameter 2: Current WaveRtBufferWritePosition	
    // Parameter 3: Pin State 0->KS_STOP, 1->KS_ACQUIRE, 2->KS_PAUSE, 3->KS_RUN 
    //Parameter 4: 0
    PADAPTERCOMMON  pAdapterComm = m_pMiniport->GetAdapterCommObj();
    pAdapterComm->WriteEtwEvent(eMINIPORT_PIN_STATE,
                                (m_ulSamplesTransferred * m_pWfExt->Format.nBlockAlign / 2) %  m_ulDmaBufferSize, 
                                0,
                                State,
                                0); 

    //
    // Synchronize execution with the ISR to ensure that the notification DPC won't get queued when
    // changing the state of the stream to paused or stop.
    //

    switch (State)
    {
        case KSSTATE_STOP:
            DPF(D_TERSE, ("[CMiniportWaveRTStream::SetState] KSSTATE_STOP requested"));

            // Reset DMA
            ntStatus = m_pAdapterCommon->StopDma(this);

            m_ulSamplesTransferred = 0;
            m_ulVirtualPosition = 0;

            break;

        case KSSTATE_ACQUIRE:
            DPF(D_TERSE, ("[CMiniportWaveRTStream::SetState] KSSTATE_ACQUIRE requested"));
            break;

        case KSSTATE_PAUSE:
            DPF(D_TERSE, ("[CMiniportWaveRTStream::SetState] KSSTATE_PAUSE requested"));

            //
            // Pause DMA
            //


            if (m_KsState == KSSTATE_RUN)
            {
                ntStatus = m_pAdapterCommon->PauseDma(this);
            }
            break;

        case KSSTATE_RUN:
            DPF(D_TERSE, ("[CMiniportWaveRTStream::SetState] KSSTATE_RUN requested"));

            //
            // Start DMA.
            //

            if (m_KsState < KSSTATE_RUN)
            {
                ntStatus = m_pAdapterCommon->StartDma(this);
            }
            break;
    }

    if (NT_SUCCESS(ntStatus))
    {
        m_KsState = State;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS 
CMiniportWaveRTStream::SetFormat(
    KSDATAFORMAT    *DataFormat
    )
/*++

Routine Description:

    Sets the data format for the stream.

Arguments:

    DataFormat - data format to set

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DataFormat);

    DPF(D_TERSE, ("[CMiniportWaveRTStream::SetFormat] Not supported"));

    return STATUS_NOT_SUPPORTED;
}

//=============================================================================
#pragma code_seg()
VOID 
CMiniportWaveRTStream::UpdateVirtualPositionRegisters
(
    ULONG TotalSamplesTransferred
)
/*++

Routine Description:

    Updates the playback position.  The virtual position points to the last sample sent to the codec.

Arguments:

    None

Return Value:

    None

--*/
{
    DPF_ENTER(("[CMiniportWaveRTStream::UpdateVirtualPositionRegisters]"));


    ULONG oldPosition, newPosition;
    BOOLEAN updateClients = FALSE;

    //
    // Compute if we need to notify clients.
    //
    // m_ulNotificationsPerBuffer can be set to either 1 or 2.  If set to 1, we notify when the DMA pointer wraps around 
    // the end of the buffer.  If set to two, we notify when the new position wraps around the end of the buffer or passes
    // the 50% marker.
    //

    m_ulSamplesTransferred = TotalSamplesTransferred;

    oldPosition = m_ulVirtualPosition;
    newPosition = (m_ulSamplesTransferred * m_pWfExt->Format.nBlockAlign) % m_ulDmaBufferSize;

    if ((m_ulNotificationsPerBuffer == 2) && 
        (newPosition >= m_ulDmaBufferSize / 2) && 
        (oldPosition < (m_ulDmaBufferSize / 2)))
    {
        updateClients = TRUE;
    }

    if (newPosition < oldPosition)
    {
        // We have wrapped - always notify.
        updateClients = TRUE;
    }

    m_ulVirtualPosition = newPosition;

    if (updateClients == TRUE && m_KsState == KSSTATE_RUN)
    {
        KeInsertQueueDpc(&m_Dpc, NULL, NULL);
    }
}
#pragma code_seg()
VOID
CMiniportWaveRTStream::DpcCallback
(
    _In_ struct _KDPC *Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArg1,
    _In_opt_ PVOID SystemArg2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArg1);
    UNREFERENCED_PARAMETER(SystemArg2);

    CMiniportWaveRTStream* me = (CMiniportWaveRTStream*) DeferredContext;

    me->NotifyRegisteredEvents();

    return;
}

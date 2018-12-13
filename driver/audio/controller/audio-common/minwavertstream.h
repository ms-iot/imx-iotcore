/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Definition of wavert stream class.

*/

#pragma once

#include "common.h"

#define _100NS_PER_MILLISECOND           (10000)        // number of 100ns units per millisecond

//=============================================================================
// Referenced Forward
//=============================================================================
class CMiniportWaveRT;
typedef CMiniportWaveRT *PCMiniportWaveRT;

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveRTStream 
// 
class CMiniportWaveRTStream :
        public IMiniportWaveRTStreamNotification,
        public CUnknown
{
protected:
    PPORTWAVERTSTREAM           m_pPortStream;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveRTStream);
    ~CMiniportWaveRTStream();

    IMP_IMiniportWaveRTStream;
    IMP_IMiniportWaveRTStreamNotification;

    NTSTATUS                    Init
        (
        _In_  PCMiniportWaveRT    Miniport,
        _In_  PPORTWAVERTSTREAM   Stream,
        _In_  ULONG               Channel,
        _In_  BOOLEAN             Capture,
        _In_  PKSDATAFORMAT       DataFormat,
        _In_  GUID                SignalProcessingMode
        );


    // Friends
    friend class                CMiniportWaveRT;
    
    VOID UpdateVirtualPositionRegisters
    (
        ULONG TotalSamplesTransferred
    );

    ULONG GetDmaBufferSize() { return m_ulDmaBufferSize; }
    ULONG* GetDmaBuffer() { return m_DataBuffer; }
    PWAVEFORMATEXTENSIBLE       GetDataFormat() { return m_pWfExt; }

protected:
    CMiniportWaveRT*            m_pMiniport;
    ULONG                       m_ulPin;
    BOOLEAN                     m_bUnregisterStream;
    ULONG                       m_ulDmaBufferSize;
    ULONG*                      m_DataBuffer;
    LIST_ENTRY                  m_NotificationList;
    ULONG                       m_ulNotificationsPerBuffer;
    LARGE_INTEGER               m_PerformanceCounterFrequency;
    BOOLEAN                     m_bCapture;

    ULONG                       m_ulSamplesTransferred;

    KSSTATE                     m_KsState;
    KDPC                        m_Dpc;
    KSPIN_LOCK                  m_Lock;
    PWAVEFORMATEXTENSIBLE       m_pWfExt;

    GUID                        m_SignalProcessingMode;
    PADAPTERCOMMON              m_pAdapterCommon;

    //
    // Our virtual registers.
    //

    ULONG                       m_ulVirtualPosition;           // byte offset into the buffer


    public:

    GUID GetSignalProcessingMode()
    {
        return m_SignalProcessingMode;
    }

    NTSTATUS 
    AllocateBufferCommon
    (
        ULONG               NotificationCount,
        ULONG               RequestedSize,
        PMDL                *AudioBufferMdl,
        ULONG               *ActualSize,
        ULONG               *OffsetFromFirstPage,
        MEMORY_CACHING_TYPE *CacheType
    );

private:

    //
    // List for notification events
    //

    typedef struct _NOTIFICATION_LIST_ENTRY
    {
        LIST_ENTRY  ListEntry;
        PKEVENT     NotificationEvent;
    } NOTIFICATION_LIST_ENTRY, *PNOTIFICATION_LIST_ENTRY;

    static KDEFERRED_ROUTINE DpcCallback;

    VOID NotifyRegisteredEvents
    (
        VOID
    );


};

typedef CMiniportWaveRTStream *PCMiniportWaveRTStream;


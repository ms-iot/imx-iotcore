/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    CSoc class declaration.

*/
#pragma once

#include "imx_audio.h"
#include "common.h"
#include "imx_sairegs.h"
#include "minwavertstream.h"

class CSoc;

class CDmaBuffer
{
public:
    CDmaBuffer() { }
    ~CDmaBuffer() { }

    VOID Init(_In_ PSAI_REGISTERS SaiRegisters)
    {
        m_pSaiRegisters = SaiRegisters;
    }

    BOOLEAN IsMyStream(CMiniportWaveRTStream* stream)
    {
        if (stream == m_pRtStream)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    VOID ResetTxFifo();

    VOID ResetRxFifo();

    VOID FillFifos();

    VOID DrainFifos();

    VOID RegisterStream
    (
        _In_        CMiniportWaveRTStream* Stream,
                    eDeviceType DeviceType
    );

    VOID UnregisterStream
    (
        _In_        CMiniportWaveRTStream* Stream
    );

    ULONG GetSampleIndex()
    {
        ULONG byteOffset;

        byteOffset = m_ulSamplesTransferred * m_pWfExt->Format.nBlockAlign;
        if (byteOffset >= m_ulDmaBufferSize)
        {
            byteOffset = byteOffset % m_ulDmaBufferSize;
        }

        return (byteOffset >> 2); // byteOffset / sizeof(ULONG)
    }

private:

    ULONG                  m_ulSamplesTransferred;
    ULONG                  m_ulChannel;
    CMiniportWaveRTStream* m_pRtStream;
    ULONG*                 m_DataBuffer;
    PWAVEFORMATEXTENSIBLE  m_pWfExt;
    ULONG                  m_ulDmaBufferSize;
    eDeviceType            m_DeviceType;

    volatile PSAI_REGISTERS          m_pSaiRegisters;
};

class CSoc
{
public:
    CSoc();
    ~CSoc();

public:
    NTSTATUS InitSsiBlock
    (
        _In_ PSAI_REGISTERS SaiRegisters, 
        _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor,
        _In_ PDEVICE_OBJECT PDO
    )
    {
        m_pSaiRegisters = SaiRegisters;
        m_pDescriptor = descriptor;
        m_pPDO = PDO;

        m_Buffer[0].Init(SaiRegisters);
        m_Buffer[1].Init(SaiRegisters);

        return SetupClocks();
    }

    NTSTATUS RegisterStream
    (
        _In_        CMiniportWaveRTStream* Stream,
                    eDeviceType DeviceType
    );

    NTSTATUS UnregisterStream
    (
        _In_        CMiniportWaveRTStream* Stream,
                    eDeviceType DeviceType
    );

    NTSTATUS StartDma
    (
        _In_        CMiniportWaveRTStream* Stream
    );

    NTSTATUS PauseDma
    (
        _In_        CMiniportWaveRTStream* Stream
    );

    NTSTATUS StopDma
    (
        _In_        CMiniportWaveRTStream* Stream
    );

private:

    CDmaBuffer m_Buffer[eMaxDeviceType];

    BOOLEAN m_bIsRenderActive;
    BOOLEAN m_bIsCaptureActive;

    volatile PSAI_REGISTERS          m_pSaiRegisters;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  m_pDescriptor;
    PDEVICE_OBJECT                   m_pPDO;

    PKINTERRUPT                         m_pInterruptObject;
    static KSERVICE_ROUTINE             ISR;

    NTSTATUS SetupClocks();

    VOID DisableInterrupts();
    VOID EnableInterrupts();

    VOID DisableInterruptsNoLock();
    VOID EnableInterruptsNoLock();

    VOID TxSoftwareReset();
    VOID RxSoftwareReset();

    BOOLEAN DoIsrWork();

    KIRQL AcquireIsrSpinLock(void)
    {
        return KeAcquireInterruptSpinLock(m_pInterruptObject);
    }

    VOID ReleaseIsrSpinLock(KIRQL oldIrql)
    {
        KeReleaseInterruptSpinLock(m_pInterruptObject, oldIrql);
    }

};


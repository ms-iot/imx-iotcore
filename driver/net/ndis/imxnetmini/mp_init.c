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

#include "precomp.h"

/*++
Routine Description:
    Allocate MP_ADAPTER data block and do some initialization
Arguments:
    ppAdapter
        Pointer to receive pointer to our adapter
    MiniportAdapterContext
        Adapter context address
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
--*/
_Use_decl_annotations_
NDIS_STATUS MpAllocAdapterBlock(PMP_ADAPTER *ppAdapter, NDIS_HANDLE MiniportAdapterHandle) {
    NDIS_STATUS     Status;
    MP_ADAPTER     *pAdapter = NULL;

    DBG_ENET_DEV_METHOD_BEG_WITH_PARAMS("Adapter handle: 0x%p", MiniportAdapterHandle);
    for(;;)  {
        if ((pAdapter = NdisAllocateMemoryWithTagPriority(MiniportAdapterHandle, sizeof(MP_ADAPTER), MP_TAG_RX_ADAPTER, NormalPoolPriority)) == NULL) { // Allocate MP_ADAPTER block
            Status = NDIS_STATUS_RESOURCES;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisAllocateMemoryWithTagPriority() failed to allocate memory for ENET device data.");
            break;
        }
        Status = NDIS_STATUS_SUCCESS;
        NdisZeroMemory(pAdapter, sizeof(MP_ADAPTER));          // Clean up the memory block
        pAdapter->AdapterHandle = MiniportAdapterHandle;
        // Initialize lists, spinlocks, etc.
        InitializeListHead(&pAdapter->PoMgmt.PatternList);
        NdisAllocateSpinLock(&pAdapter->Dev_SpinLock);

        MpQueueInit(&pAdapter->Tx_qMpOwnedBDs);                // Initialize pending Tx Ethernet frames (NET_BUFFERs) queue
        MpQueueInit(&pAdapter->Tx_qDmaOwnedBDs);               // Initialize in-progress Tx Ethernet frames (NET_BUFFERs) queue
        NdisAllocateSpinLock(&pAdapter->Tx_SpinLock);          // Initialize Tx path spin lock
        NdisAllocateSpinLock(&pAdapter->Rx_SpinLock);          // Initialize Rx path spin lock

        // State machine initialization
        PMP_STATE_MACHINE pSM = &pAdapter->StateMachine;
        pSM->SM_CurrentState = SM_STATE_PAUSED;
        pSM->SM_NextState = SM_STATE_PAUSED;
        NdisAllocateSpinLock(&pSM->SM_SpinLock);               // Initialize state machine spin lock
        NdisInitializeEvent(&pSM->SM_EnetHaltedEvent);         // Initialize Halted state event

        NDIS_TIMER_CHARACTERISTICS  Timer;
        NdisZeroMemory(&Timer, sizeof(Timer));
        ASSERT(NDIS_SIZEOF_TIMER_CHARACTERISTICS_REVISION_1 <= sizeof(Timer));
        Timer.Header.Type     = NDIS_OBJECT_TYPE_TIMER_CHARACTERISTICS;
        Timer.Header.Revision = NDIS_TIMER_CHARACTERISTICS_REVISION_1;
        Timer.Header.Size     = NDIS_SIZEOF_TIMER_CHARACTERISTICS_REVISION_1;
        Timer.AllocationTag   = MP_TAG_SM_TIMER_DESC;
        Timer.TimerFunction   = MpSmDispatcher;
        Timer.FunctionContext = pAdapter;
        // Allocate State machine timer
        if ((Status = NdisAllocateTimerObject(MiniportAdapterHandle, &Timer, &pSM->SM_hTimer)) != NDIS_STATUS_SUCCESS) {
            Status = NDIS_STATUS_FAILURE;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisAllocateTimerObject() failed.");
            break;
        }
        break;
    }
    *ppAdapter = pAdapter;
    DBG_ENET_DEV_METHOD_END_WITH_STATUS(Status);
    return Status;
}

/*++
Routine Description:
    Allocate all the memory blocks for send, receive and others
Arguments:
    pAdapter    Pointer to our adapter
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_RESOURCES
--*/
_Use_decl_annotations_
NDIS_STATUS NICAllocAdapterMemory(PMP_ADAPTER pAdapter)
{
    NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
    PMP_TX_PAYLOAD_BD               pEnetSwExtBD;
    LONG                            index;
    NDIS_SG_DMA_DESCRIPTION         DmaDescription;
    PUCHAR                          AllocVa;
    NDIS_PHYSICAL_ADDRESS           AllocPa;
    NET_BUFFER_LIST_POOL_PARAMETERS PoolParameters;

    DBG_ENET_DEV_METHOD_BEG();
    for(;;) {

        // Initialize DMA system
        NdisZeroMemory(&DmaDescription, sizeof(DmaDescription));
        DmaDescription.Header.Type                      = NDIS_OBJECT_TYPE_SG_DMA_DESCRIPTION;
        DmaDescription.Header.Revision                  = NDIS_SG_DMA_DESCRIPTION_REVISION_1;
        DmaDescription.Header.Size                      = sizeof(NDIS_SG_DMA_DESCRIPTION);
        DmaDescription.Flags                            = 0;                    // we don't do 64 bit DMA
        DmaDescription.MaximumPhysicalMapping           = ENET_TX_FRAME_SIZE;   // Even if offload is enabled, the packet size for mapping shouldn't change
        DmaDescription.ProcessSGListHandler             = MpProcessSGList;      //
        DmaDescription.SharedMemAllocateCompleteHandler = NULL;                 // ENET does not call NdisMAllocateSharedMemoryAsyncEx, hence no need for complete handler
        if ((Status = NdisMRegisterScatterGatherDma(pAdapter->AdapterHandle, &DmaDescription, &pAdapter->Tx_DmaHandle)) == NDIS_STATUS_SUCCESS) {
            pAdapter->Tx_SGListSize = DmaDescription.ScatterGatherListSize;
        } else {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMRegisterScatterGatherDma() failed.");
            break;
        }
        pAdapter->CacheFillSize = NdisMGetDmaAlignment(pAdapter->AdapterHandle);

        //  Allocates a pool of NBL(and NB) structures for Rx path. Each allocated NBL structure is initialized with one NB structure.
        NdisZeroMemory(&PoolParameters, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));
        PoolParameters.Header.Type        = NDIS_OBJECT_TYPE_DEFAULT;
        PoolParameters.Header.Revision    = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        PoolParameters.Header.Size        = sizeof(PoolParameters);
        PoolParameters.fAllocateNetBuffer = TRUE;                     // Allocate one NB for each NBL
        // PoolParameters.DataSize        = 0;                        // Do not allocate data buffer
        // PoolParameters.ProtocolId      = NDIS_PROTOCOL_ID_DEFAULT; // NDIS_PROTOCOL_ID_DEFAULT = 0;
        PoolParameters.PoolTag            = MP_TAG_TX_NBL_AND_NB;
        pAdapter->Rx_NBAndNBLPool         = NdisAllocateNetBufferListPool(pAdapter->AdapterHandle,&PoolParameters);
        if (pAdapter->Rx_NBAndNBLPool == NULL)  {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Initialize Tx Lookaside lists
        NdisInitializeNPagedLookasideList(&pAdapter->Tx_MpTxBDLookasideList, NULL, NULL, 0, sizeof(MP_TX_BD) - sizeof(SCATTER_GATHER_LIST) + pAdapter->Tx_SGListSize, MP_TAG_TX_BD, 0);

        pAdapter->Rx_DmaBDT_DmaOwnedBDsLowWatterMark = (pAdapter->Rx_DmaBDT_ItemCount * MAC_RX_BUFFER_LOW_WATER_PERCENT) / 100;

        /* ************************************************************************************************************************************ */
        /* Allocated memory for ENET DMA Receive Descriptors Table(Rx_DmaBDT). Note: This memory must be 8 bytes aligned!                       */
        /* ************************************************************************************************************************************ */
        pAdapter->Rx_DmaBDT_Size = pAdapter->Rx_DmaBDT_ItemCount * sizeof(ENET_BD);
        NdisMAllocateSharedMemory(pAdapter->AdapterHandle, pAdapter->Rx_DmaBDT_Size, FALSE, (PVOID) &pAdapter->Rx_DmaBDT, &pAdapter->Rx_DmaBDT_Pa);
        ASSERT(!((uintptr_t)pAdapter->Rx_DmaBDT & 0x7));   // This memory must be 8 bytes aligned!
        if (!pAdapter->Rx_DmaBDT) {
            Status = NDIS_STATUS_RESOURCES;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMAllocateSharedMemory() failed to allocate memory for Rx_DmaBDT.");
            break;
        }
        NdisZeroMemory((PVOID)pAdapter->Rx_DmaBDT, pAdapter->Rx_DmaBDT_Size);

        /* ************************************************************************************************************************************ */
        /* Allocated memory for ENET DMA Transmit Descriptors Table(Tx_DmaBDT). Note: This memory must be 8 bytes aligned!                      */
        /* ************************************************************************************************************************************ */
        pAdapter->Tx_DmaBDT_Size = pAdapter->Tx_DmaBDT_ItemCount * sizeof(ENET_BD);
        NdisMAllocateSharedMemory(pAdapter->AdapterHandle, pAdapter->Tx_DmaBDT_Size, FALSE, (PVOID) &pAdapter->Tx_DmaBDT, &pAdapter->Tx_DmaBDT_Pa);
        ASSERT(!((uintptr_t)pAdapter->Tx_DmaBDT & 0x7));   // This memory must be 8 bytes aligned!
        if (!pAdapter->Tx_DmaBDT) {
            Status = NDIS_STATUS_RESOURCES;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMAllocateSharedMemory() failed to allocate memory for Tx_DmaBDT.");
            break;
        }
        NdisZeroMemory((PVOID)pAdapter->Tx_DmaBDT, pAdapter->Tx_DmaBDT_Size);

        // Allocate RX DMA SW extension buffer descriptors array.
        ULONG Rx_DmaBDT_SwExtSize =  sizeof(PMP_RX_FRAME_BD) * pAdapter->Rx_DmaBDT_ItemCount;
        if ((pAdapter->Rx_DmaBDT_SwExt = NdisAllocateMemoryWithTagPriority(pAdapter->AdapterHandle, Rx_DmaBDT_SwExtSize, MP_TAG_RX_PAYLOAD_DESC, NormalPoolPriority)) == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMAllocateSharedMemory() failed to allocated RX Dma SW extension descriptors table.");
            break;
        }
        NdisZeroMemory(pAdapter->Rx_DmaBDT_SwExt, Rx_DmaBDT_SwExtSize);
        // Allocate RX frame buffer descriptors array.
        ULONG Rx_FrameBDTSize =  sizeof(MP_RX_FRAME_BD) * pAdapter->Rx_DmaBDT_ItemCount;
        if ((pAdapter->Rx_FrameBDT = NdisAllocateMemoryWithTagPriority(pAdapter->AdapterHandle, Rx_FrameBDTSize, MP_TAG_RX_PAYLOAD_DESC, NormalPoolPriority)) == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisAllocateMemoryWithTagPriority() failed to allocated RX frame descriptors table.");
            break;
        }
        NdisZeroMemory(pAdapter->Rx_FrameBDT, Rx_FrameBDTSize);
        // Allocate RX frame date buffers. Allocate buffer memory, MDL, NBL, NB
        for (LONG RxBuffIdx = 0; RxBuffIdx < pAdapter->Rx_DmaBDT_ItemCount; ++RxBuffIdx) {
            MP_RX_FRAME_BD *pRxFrameBD = &pAdapter->Rx_FrameBDT[RxBuffIdx];
            #if 0 //MVa
            NdisMAllocateSharedMemory(pAdapter->AdapterHandle, pAdapter->ENET_RX_FRAME_SIZE, TRUE, &pRxFrameBD->pBuffer, &pRxFrameBD->BufferPa);
            if (pRxFrameBD->pBuffer == NULL) {
                DBG_PRINT_ERROR(ZONE_INIT, "Failed to allocate memory for ENET receive buffer descriptor.");
                Status = NDIS_STATUS_RESOURCES;
                break;
            #endif
            { // MS-temp fix begin
                #pragma prefast(disable:30030, "Temporary fix")
                PHYSICAL_ADDRESS highestAcceptableAddress; highestAcceptableAddress.QuadPart = (LONGLONG)-1;
                PHYSICAL_ADDRESS lowestAcceptableAddress; lowestAcceptableAddress.QuadPart = 0;
                PHYSICAL_ADDRESS boundaryAddress; boundaryAddress.QuadPart = 0;
                pRxFrameBD->pBuffer = (PUCHAR)MmAllocateContiguousMemorySpecifyCache(ENET_RX_FRAME_SIZE,lowestAcceptableAddress,highestAcceptableAddress,boundaryAddress,MmCached);
                if (pRxFrameBD->pBuffer == NULL) {
                    DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("MmAllocateContiguousMemorySpecifyCache() failed to allocate memory for receive buffer.");
                    Status = NDIS_STATUS_RESOURCES;
                    break;
                }
                pRxFrameBD->BufferPa = MmGetPhysicalAddress(pRxFrameBD->pBuffer);
            } // MS-temp fix end
            // Allocate MDL
            if ((pRxFrameBD->pMdl = NdisAllocateMdl(pAdapter->AdapterHandle, pRxFrameBD->pBuffer, ENET_RX_FRAME_SIZE)) == NULL) {
                Status = NDIS_STATUS_RESOURCES;
                DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisAllocateMdl() failed to allocate Mdl for receive buffer.");
                break;
            }
            // Allocate NBL and NB
            if ((pRxFrameBD->pNBL = NdisAllocateNetBufferAndNetBufferList(pAdapter->Rx_NBAndNBLPool, 0, 0, pRxFrameBD->pMdl, 2, 0)) == NULL) {
                Status = NDIS_STATUS_RESOURCES;
                DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisAllocateNetBufferAndNetBufferList() failed to allocate NBL and NB for receive buffer.");
                break;
            }
            MP_NBL_SET_RX_FRAME_BD(pRxFrameBD->pNBL, pRxFrameBD);       // Associate NBL and payload buffer descriptor
        }
        if (Status != NDIS_STATUS_SUCCESS) {
            break;
        }

        /* ************************************************************************************************************************************ */
        // Allocate memory for tx Ethernet frames
        /* ************************************************************************************************************************************ */
        pAdapter->Tx_DataBuffer_Size = pAdapter->Tx_DmaBDT_ItemCount * (ENET_TX_FRAME_SIZE/*+ pAdapter->CacheFillSize*/ );
        NdisMAllocateSharedMemory(pAdapter->AdapterHandle, pAdapter->Tx_DataBuffer_Size, TRUE, &pAdapter->Tx_DataBuffer_Va, &pAdapter->Tx_DataBuffer_Pa);
        if (pAdapter->Tx_DataBuffer_Va == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisMAllocateSharedMemory() failed to allocate a big Tx data buffer");
            break;
        }
        // For each Tx buffer initialize buffer description
        AllocVa = pAdapter->Tx_DataBuffer_Va;
        AllocPa = pAdapter->Tx_DataBuffer_Pa;
        for (index = 0; index < pAdapter->Tx_DmaBDT_ItemCount; index++) {
            pEnetSwExtBD = &pAdapter->Tx_EnetSwExtBDT[index];
            pEnetSwExtBD->BufferSize        = ENET_TX_FRAME_SIZE;
            pEnetSwExtBD->pBuffer           = MP_ALIGNMEM(AllocVa, pAdapter->CacheFillSize); // Align the buffer on the cache line boundary
            pEnetSwExtBD->BufferPa.QuadPart = MP_ALIGNMEM_PA(AllocPa, pAdapter->CacheFillSize);
            #pragma prefast(disable:6385, "pEnetSwExtBD->pBuffer")
            pEnetSwExtBD->pMdl = NdisAllocateMdl(pAdapter->AdapterHandle, pEnetSwExtBD->pBuffer, pEnetSwExtBD->BufferSize);
            if (pEnetSwExtBD->pMdl == NULL) {
                DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisAllocateMdl() failed to allocate MDL for a big Tx buffer.");
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
            AllocVa += ENET_TX_FRAME_SIZE;
            AllocPa.QuadPart += ENET_TX_FRAME_SIZE;
        }
        break;
    }
    DBG_ENET_DEV_METHOD_END_WITH_STATUS(Status);
    return Status;
}

/*++
Routine Description:
    Free all the resources and MP_ADAPTER data block
Arguments:
    pAdapter    Pointer to our adapter
Return Value:
    None
Note: Must be called at IRQL = PASSIVE_LEVEL.
--*/
_Use_decl_annotations_
void MpFreeAdapter(PMP_ADAPTER  pAdapter)
{
    DBG_ENET_DEV_METHOD_BEG();
    ASSERT(NDIS_CURRENT_IRQL() == PASSIVE_LEVEL);

    if (pAdapter) {
        // Free hardware resources

        if (pAdapter->NdisInterruptHandle)  {
            NdisMDeregisterInterruptEx(pAdapter->NdisInterruptHandle);
        }
        if (pAdapter->ENETRegBase) {
            NdisMUnmapIoSpace(pAdapter->AdapterHandle, (PVOID)pAdapter->ENETRegBase, sizeof(CSP_ENET_REGS));
        }

        MDIODev_DeinitDevice(&pAdapter->ENETDev_MDIODevice);

        for (int i = 0; i < pAdapter->Tx_DmaBDT_ItemCount; i++) { // Free all Tx packet buffer MDL
            if (pAdapter->Tx_EnetSwExtBDT[i].pMdl != NULL) {
                 NdisFreeMdl(pAdapter->Tx_EnetSwExtBDT[i].pMdl);
                 pAdapter->Tx_EnetSwExtBDT[i].pMdl = NULL;
            }
        }
        if (pAdapter->Tx_DataBuffer_Va != NULL)  { // Free Tx packets memory
            NdisMFreeSharedMemory(pAdapter->AdapterHandle, pAdapter->Tx_DataBuffer_Size, TRUE, pAdapter->Tx_DataBuffer_Va, pAdapter->Tx_DataBuffer_Pa);
            pAdapter->Tx_DataBuffer_Va = NULL;
        }
        if (pAdapter->Tx_DmaBDT) { // Free Tx_DmaBDT
            NdisMFreeSharedMemory(pAdapter->AdapterHandle, pAdapter->Tx_DmaBDT_Size, FALSE, (PVOID)pAdapter->Tx_DmaBDT, pAdapter->Tx_DmaBDT_Pa);
            pAdapter->Tx_DmaBDT = NULL;
        }
        if (pAdapter->Rx_DmaBDT) { // Free ENET Rx_DmaBDT
            NdisMFreeSharedMemory(pAdapter->AdapterHandle, pAdapter->Rx_DmaBDT_Size, FALSE, (PVOID)pAdapter->Rx_DmaBDT, pAdapter->Rx_DmaBDT_Pa);
            pAdapter->Rx_DmaBDT = NULL;
        }
        // Free Rx_DmaBDT_SwExt
        if (pAdapter->Rx_DmaBDT_SwExt != NULL) {
            NdisFreeMemory(pAdapter->Rx_DmaBDT_SwExt, 0, 0);
            pAdapter->Rx_DmaBDT_SwExt = NULL;
        }
        // Free RX payload buffer descriptors
        if (pAdapter->Rx_FrameBDT != NULL) {
            for (LONG RxBuffIdx = 0; RxBuffIdx < pAdapter->Rx_DmaBDT_ItemCount; ++RxBuffIdx) {
                MP_RX_FRAME_BD *pRxFrameBD = &pAdapter->Rx_FrameBDT[RxBuffIdx];
                if (pRxFrameBD != NULL) {
                    if (pRxFrameBD->pMdl != NULL) {
                        NdisFreeMdl(pRxFrameBD->pMdl);
                    }
                    if (pRxFrameBD->pNBL != NULL) {
                        NdisFreeNetBufferList(pRxFrameBD->pNBL);
                    }
                    if (pRxFrameBD->pBuffer != NULL) {
                        // MVa NdisMFreeSharedMemory(pAdapter->AdapterHandle, ENET_RX_FRAME_SIZE, TRUE, pRxFrameBD->pBuffer, pRxFrameBD->BufferPa);
                        /* MS temp fix*/ MmFreeContiguousMemory(pRxFrameBD->pBuffer);
                    }
                }
            }
            NdisFreeMemory(pAdapter->Rx_FrameBDT, 0, 0);
            pAdapter->Rx_FrameBDT = NULL;

        }
        // Free NB and NBL pool
        if (pAdapter->Rx_NBAndNBLPool) {
            NdisFreeNetBufferListPool(pAdapter->Rx_NBAndNBLPool);
            NdisDeleteNPagedLookasideList(&pAdapter->Tx_MpTxBDLookasideList);
        }
        // Deregister the DMA handle after freeing all shared memory
        if (pAdapter->Tx_DmaHandle != NULL)  {
            NdisMDeregisterScatterGatherDma(pAdapter->Tx_DmaHandle);
        }
        // Free timer object
        if (pAdapter->StateMachine.SM_hTimer) {
            NdisFreeTimerObject(pAdapter->StateMachine.SM_hTimer);
            pAdapter->StateMachine.SM_hTimer = NULL;
        }
        NdisFreeMemory(pAdapter, 0, 0);
        DBG_ENET_DEV_CODE(pAdapter = NULL);
    }
    DBG_ENET_DEV_METHOD_END();
}

/*++
Routine Description:
    Read the following from the registry
        1. All the parameters
        2. NetworkAddres
Arguments:
    Adapter     Pointer to our adapter
Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
--*/
_Use_decl_annotations_
NDIS_STATUS NICReadRegParameters(PMP_ADAPTER pAdapter)
{
    NDIS_STATUS                     Status;
    NDIS_HANDLE                     ConfigurationHandle;
    PUCHAR                          NetworkAddress;
    UINT                            Length;
    NDIS_CONFIGURATION_OBJECT       ConfigObject;

    MP_REG_VALUE_DESC static regValues[] = {
        {
            NDIS_STRING_CONST("*ReceiveBuffers"),
            MP_OFFSET(Rx_DmaBDT_ItemCount),
            MP_SIZE(Rx_DmaBDT_ItemCount),
            RX_DESC_COUNT_DEFAULT,
            RX_DESC_COUNT_MIN,
            RX_DESC_COUNT_MAX
        },
        {
            NDIS_STRING_CONST("*TransmitBuffers"),
            MP_OFFSET(Tx_DmaBDT_ItemCount),
            MP_SIZE(Tx_DmaBDT_ItemCount),
            TX_DESC_COUNT_DEFAULT,
            TX_DESC_COUNT_MIN,
            TX_DESC_COUNT_MAX
        },
        {
            NDIS_STRING_CONST("*SpeedDuplex"),
            MP_OFFSET(SpeedSelect),
            MP_SIZE(SpeedSelect),
            SPEED_SELECT_DEFAULT,
            SPEED_SELECT_MIN,
            SPEED_SELECT_MAX
        },
#if DBG
        {
            NDIS_STRING_CONST("OpcodePauseDuration"),
            MP_OFFSET(OPD_Value),
            MP_SIZE(OPD_Value),
            ENET_MAC_RX_OPD_DEFAULT_VALUE,
            ENET_MAC_RX_OPD_MIN_VALUE,
            ENET_MAC_RX_OPD_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("RxSectionFullTreshold"),
            MP_OFFSET(RSFL_Value),
            MP_SIZE(RSFL_Value),
            ENET_MAC_RX_SECTION_FULL_DEFAULT_VALUE,
            ENET_MAC_RX_SECTION_FULL_MIN_VALUE,
            ENET_MAC_RX_SECTION_FULL_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("RxSectionEmptyTreshold"),
            MP_OFFSET(RSEM_Value),
            MP_SIZE(RSEM_Value),
            ENET_MAC_RX_SECTION_EMPTY_DEFAULT_VALUE,
            ENET_MAC_RX_SECTION_EMPTY_MIN_VALUE,
            ENET_MAC_RX_SECTION_EMPTY_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("RxAlmostEmptyTreshold"),
            MP_OFFSET(RAEM_Value),
            MP_SIZE(RAEM_Value),
            ENET_MAC_RX_ALMOST_EMPTY_DEFAULT_VALUE,
            ENET_MAC_RX_ALMOST_EMPTY_MIN_VALUE,
            ENET_MAC_RX_ALMOST_EMPTY_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("RxAlmostFullTreshold"),
            MP_OFFSET(RAFL_Value),
            MP_SIZE(RAFL_Value),
            ENET_MAC_RX_ALMOST_FULL_DEFAULT_VALUE,
            ENET_MAC_RX_ALMOST_FULL_MIN_VALUE,
            ENET_MAC_RX_ALMOST_FULL_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("TxWatermark"),
            MP_OFFSET(TFWR_Value),
            MP_SIZE(TFWR_Value),
            BF_ENET_MAC_TFW_TFWR_DEFAULT_VALUE,
            BF_ENET_MAC_TFW_TFWR_MIN_VALUE,
            BF_ENET_MAC_TFW_TFWR_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("TxAlmostEmptyTreshold"),
            MP_OFFSET(TAEM_Value),
            MP_SIZE(TAEM_Value),
            ENET_MAC_TX_ALMOST_EMPTY_DEFAULT_VALUE,
            ENET_MAC_TX_ALMOST_EMPTY_MIN_VALUE,
            ENET_MAC_TX_ALMOST_EMPTY_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("TxAlmostFullTreshold"),
            MP_OFFSET(TAFL_Value),
            MP_SIZE(TAFL_Value),
            ENET_MAC_TX_ALMOST_FULL_DEFAULT_VALUE,
            ENET_MAC_TX_ALMOST_FULL_MIN_VALUE,
            ENET_MAC_TX_ALMOST_FULL_MAX_VALUE
        },
        {
            NDIS_STRING_CONST("TxSectiontEmptyTreshold"),
            MP_OFFSET(TSEM_Value),
            MP_SIZE(TSEM_Value),
            ENET_MAC_TX_SECTION_EMPTY_DEFAULT_VALUE,
            ENET_MAC_TX_SECTION_EMPTY_MIN_VALUE,
            ENET_MAC_TX_SECTION_EMPTY_MAX_VALUE
        },
#endif
    };

    DBG_ENET_DEV_METHOD_BEG();
    do {
        Status = NDIS_STATUS_SUCCESS;
        ConfigurationHandle = NULL;
        NdisZeroMemory(&ConfigObject, NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1);
        ConfigObject.Header.Type     = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
        ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
        ConfigObject.Header.Size     = NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1;
        ConfigObject.NdisHandle      = pAdapter->AdapterHandle;
        ConfigObject.Flags           = 0;
        if ((Status = NdisOpenConfigurationEx(&ConfigObject, &ConfigurationHandle)) != NDIS_STATUS_SUCCESS) {
            DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS("NdisOpenConfiguration() failed.");
            break;
        }
        // Read configuration parameters from registry
        for (ULONG regDescInx = 0; regDescInx < ARRAYSIZE(regValues); ++regDescInx) {
            PMP_REG_VALUE_DESC             regValueDescPtr = &regValues[regDescInx];
            UINT                           value           = regValueDescPtr->Default;
            PNDIS_CONFIGURATION_PARAMETER  returnedValuePtr;
            // All configuration parameters are only integer
            NdisReadConfiguration(&Status, &returnedValuePtr, ConfigurationHandle, &regValueDescPtr->ValueName, NdisParameterInteger);
            // Validate the returned value or used the default...
            if (Status == NDIS_STATUS_SUCCESS) {
                if (returnedValuePtr->ParameterData.IntegerData >= regValueDescPtr->Min && returnedValuePtr->ParameterData.IntegerData <= regValueDescPtr->Max)  {
                    value = returnedValuePtr->ParameterData.IntegerData;
                }
            }
            NdisMoveMemory((PUCHAR)pAdapter + regValueDescPtr->ParamOffset, &value, regValueDescPtr->ParamSize);
            Status = NDIS_STATUS_SUCCESS;
        }
        // If there is a MAC address in registry, use it.
        NdisReadNetworkAddress(&Status, &NetworkAddress, &Length, ConfigurationHandle);
        if ((Status == NDIS_STATUS_SUCCESS) && (Length == ETH_LENGTH_OF_ADDRESS)) {
            if ((ETH_IS_MULTICAST(NetworkAddress) || ETH_IS_BROADCAST(NetworkAddress)) || !ETH_IS_LOCALLY_ADMINISTERED (NetworkAddress)) {
                Status = NDIS_STATUS_FAILURE;
                DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS("NdisReadNetworkAddress() failed.", "NetworkAddress in registry is not invalid - %02X-%02X-%02X-%02X-%02X-%02X", NetworkAddress[0], NetworkAddress[1], NetworkAddress[2], NetworkAddress[3], NetworkAddress[4], NetworkAddress[5]);
            } else {
                ETH_COPY_NETWORK_ADDRESS(pAdapter->FecMacAddress, NetworkAddress);
                DBG_ENET_DEV_PRINT_INFO("ENET MAC address: %02X-%02X-%02X-%02X-%02X-%02X (from registry)", pAdapter->FecMacAddress[0], pAdapter->FecMacAddress[1], pAdapter->FecMacAddress[2], pAdapter->FecMacAddress[3], pAdapter->FecMacAddress[4], pAdapter->FecMacAddress[5]);
            }
        }
        ETH_COPY_NETWORK_ADDRESS(pAdapter->CurrentAddress, pAdapter->FecMacAddress);
        ETH_COPY_NETWORK_ADDRESS(pAdapter->PermanentAddress, pAdapter->FecMacAddress);
        Status = NDIS_STATUS_SUCCESS;
        // Close the registry
        NdisCloseConfiguration(ConfigurationHandle);
    } while (0);
    DBG_ENET_DEV_METHOD_END_WITH_STATUS(Status);
    return Status;
}

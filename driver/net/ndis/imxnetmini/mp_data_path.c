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
    It is called to initialize a generic queue.
Arguments:
    pQueue  Address of a caller allocated ST_MP_QUEUE object to be initialized.
Return Value:
    None
--*/
_Use_decl_annotations_
void MpQueueInit(PMP_QUEUE pQueue)
{
    RtlZeroMemory(pQueue, sizeof(MP_QUEUE));
    NdisAllocateSpinLock(&pQueue->Lock);
    InitializeListHead(&pQueue->Head);
}

/*++
Routine Description:
    It is called to add a new entry to the beginning of the queue.
Arguments:
    pQueue      The target queue address.
    pListEntry  The address of the new entry to be added.
Return Value:
    None
--*/
_Use_decl_annotations_
void MpQueueAdd(PMP_QUEUE pQueue, PLIST_ENTRY pListEntry)
{
    NdisAcquireSpinLock(&pQueue->Lock);
    InsertHeadList(&pQueue->Head, pListEntry);
    pQueue->Count++;
    NdisReleaseSpinLock(&pQueue->Lock);
}

/*++
Routine Description:
    It is called to get the next (oldest) entry of the queue.
    The entry is removed from the queue. If the queue is empty, NULL is returned.
Arguments:
    pQueue      The target queue address.
Return Value:
    The address of the oldest entry in the queue, or NULL if the queue is empty.
--*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PLIST_ENTRY MpQueueGetNext(PMP_QUEUE pQueue)
{
    PLIST_ENTRY pNextEntry = NULL;

    NdisAcquireSpinLock(&pQueue->Lock);
    ASSERT(pQueue->Count >= 0);
    do {
        if (IsListEmpty(&pQueue->Head)) {
            ASSERT(pQueue->Count == 0);
            break;
        }
        pNextEntry = RemoveTailList(&pQueue->Head);
        NdisZeroMemory(pNextEntry, sizeof(LIST_ENTRY));   // Mark that it is not queued
        pQueue->Count--;
    } while (0);
    NdisReleaseSpinLock(&pQueue->Lock);
    return  pNextEntry;
}

/*++
Routine Description:
    Called to remove an entry from the queue.
Arguments:
    pQueue      The target queue address.
    pListEntry  The address of the entry to be removed.
Return Value:
    TRUE if after removing the entry the queue is empty, otherwise FALSE.
--*/
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN MpQueueRemoveEntry(PMP_QUEUE pQueue, PLIST_ENTRY pListEntry)
{
    BOOLEAN isRemoved = FALSE;

    NdisAcquireSpinLock(&pQueue->Lock);
    do {
        if (IsListEmpty(&pQueue->Head)) {
            ASSERT(pQueue->Count == 0);
            break;
        }
        ASSERT(pQueue->Count > 0);
        isRemoved = RemoveEntryList(pListEntry);
        pQueue->Count--;
        if (isRemoved) {
            ASSERT(pQueue->Count == 0);
            ASSERT(IsListEmpty(&pQueue->Head));
        }
        NdisZeroMemory(pListEntry, sizeof(LIST_ENTRY)); // Mark that it is not queued
    } while (0);
    NdisReleaseSpinLock(&pQueue->Lock);
    return isRemoved;
}

/*++
Routine Description:
    It returns the number of items currently in the queue.
Arguments:
    pQueue    The target queue address.
Return Value:
    The number of items currently in the queue.
--*/
_Use_decl_annotations_
LONG MpQueueGetDepth(PMP_QUEUE pQueue)
{
    NdisAcquireSpinLock(&pQueue->Lock);
    LONG ulDepth = pQueue->Count;
    NdisReleaseSpinLock(&pQueue->Lock);
    return ulDepth;
}

/*++
Routine Description:
    Peeks at the first (oldest) entry in the queue, without removing it
Arguments:
    pQueue      The target queue address.
Return Value:
    Address of the first (oldest) entry in the queue, or NULL if queue is empty
--*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PLIST_ENTRY MpQueuePeekFirst(PMP_QUEUE pQueue)
{
    PLIST_ENTRY firstEntryPtr = NULL;

    NdisAcquireSpinLock(&pQueue->Lock);
    do {
        if (IsListEmpty(&pQueue->Head)) {
            ASSERT(pQueue->Count == 0);
            break;
        }
        ASSERT(pQueue->Count > 0);
        firstEntryPtr = pQueue->Head.Blink;
    } while (0);
    NdisReleaseSpinLock(&pQueue->Lock);
    return firstEntryPtr;
}

/*++
Routine Description:
   Peeks at the next entry in the queue, without removing it given a current entry
Arguments:
    pQueue      The target queue address.
    pListEntry  The current entry
Return Value:
    Address of the next entry in the queue, or NULL if queue is empty
--*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PLIST_ENTRY MpQueuePeekNext(PMP_QUEUE pQueue, PLIST_ENTRY pListEntry)
{
    PLIST_ENTRY nextEntryPtr = NULL;
    NdisAcquireSpinLock(&pQueue->Lock);
    do {
        if (IsListEmpty(&pQueue->Head))  {
            ASSERT(pQueue->Count == 0);
            break;
        }
        ASSERT(pQueue->Count > 0);
        nextEntryPtr = pListEntry->Blink;
        if (nextEntryPtr == &pQueue->Head) {
            nextEntryPtr = NULL;
        }
    } while (0);
    NdisReleaseSpinLock(&pQueue->Lock);
    return nextEntryPtr;
}

/*++
Routine Description:
    Checks if the transmission process is stalled.
    Check is there are Tx Ethernet frame in the 'in progress' queue that have timed out.
    NDIS calls MiniportCheckForHangEx at IRQL = PASSIVE_LEVEL.
Arguments:
    MiniportAdapterContext
        Address of the adapter context
Return Value:
    TRUE - If transmission process is hang, or FALSE if it working properly.
--*/
_Use_decl_annotations_
BOOLEAN MpCheckForHangEx(NDIS_HANDLE MiniportAdapterContext)
{
    PMP_ADAPTER pAdapter = (PMP_ADAPTER)MiniportAdapterContext;
    BOOLEAN     isTxHang = TRUE;

    NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
    for(;;) {
        if (MpQueueGetDepth(&pAdapter->Tx_qDmaOwnedBDs) > 0) {  // Any Tx frame pending in HW?
            if (++pAdapter->Tx_CheckForHangCounter > 2) {       // Second call of this function without successful Tx transfer?
                DBG_ENET_DEV_PRINT_ERROR("TX is hang!");
                break;
            }
        }
        isTxHang = FALSE;
        break;
    }
    NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
    if (TRUE == isTxHang)
        DBG_ENET_DEV_PRINT_ERROR("Requesting Reset by NDIS");
    return isTxHang;
}

/*++
Routine Description:
    It is called to unwind an Ethernet frame (NET_BUFFER). All the resources (MpTxBD, scatter gather list) associated
    with the Ethernet frame, are released, and if the Ethernet frame is the last Ethernet frame of the packet,
    the packet is completed.
Arguments:
    pAdapter            Address of the adapter context.
    pNBL                The container Tx packet (NET_BUFFER_LIST).
    pNB                 The Ethernet frame (NET_BUFFER) to complete.
    CompletionStatus    The completion status
    SendCompleteFlags   Flags specifying if the caller is at DISPATCH_LEVEL.
Return Value:
    None
--*/
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID MpTxUnwindNetBuffer(PMP_ADAPTER pAdapter, PNET_BUFFER_LIST pNBL, PNET_BUFFER pNB, NDIS_STATUS CompletionStatus, ULONG SendCompleteFlags)
{
    PMP_TX_BD      pMpTxBD = MP_NB_pMpTxBD(pNB);

    if (pMpTxBD != NULL) {
        if (pMpTxBD->pSGList != NULL) {
          NdisMFreeNetBufferSGList(pAdapter->Tx_DmaHandle, pMpTxBD->pSGList, pMpTxBD->pNB);
        }
        DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d), returning MpTxBD 0x%08X to Tx_MpTxBDLookasideList list", pMpTxBD->NBId ,pMpTxBD);
        NdisFreeToNPagedLookasideList(&pAdapter->Tx_MpTxBDLookasideList, pMpTxBD);
    }
    if (NdisInterlockedDecrement(&MP_NBL_NB_Counter(pNBL)) == 0) {
        #ifdef DBG
        LARGE_INTEGER  CurrentSystemTime;
        LARGE_INTEGER  NBLStartTime;
        KeQuerySystemTimePrecise(&CurrentSystemTime);
        NBLStartTime.QuadPart = MP_NB_Time(pNB).QuadPart;
        DBG_ENET_DEV_TX_PRINT_TRACE("####### Copmleting NBL(%d), Status: 0x%X, %d us", MP_NBL_ID(pNBL), CompletionStatus, (LONG)((CurrentSystemTime.QuadPart - NBLStartTime.QuadPart)/10));
        #endif
        NET_BUFFER_LIST_STATUS(pNBL)   = CompletionStatus;
        NET_BUFFER_LIST_NEXT_NBL(pNBL) = NULL;
        NdisMSendNetBufferListsComplete(pAdapter->AdapterHandle, pNBL, SendCompleteFlags);
    }
    NdisInterlockedDecrement(&pAdapter->Tx_PendingNBs);
    ASSERT(pAdapter->Tx_PendingNBs >= 0);
}

/*++
Routine Description:
    It is called to cancel all queued TX frames.
Arguments:
    pAdapter    Address of the adapter context
Return Value:
    TRUE: at least one TX frame was canceled, otherwise FALSE.
--*/
_Use_decl_annotations_
BOOLEAN MpTxCancelAll(PMP_ADAPTER pAdapter)
{
    LIST_ENTRY  CanceledNetBufferList;                                // The list of NET_BUFFERs associated with NET_BUFFER_LISTs that should be canceled.
    NDIS_STATUS CompletionStatus;                                     // Completion status to use
    PLIST_ENTRY pListEntry;

    InitializeListHead(&CanceledNetBufferList);
    NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
    while ((pListEntry = MpQueueGetNext(&pAdapter->Tx_qDmaOwnedBDs)) != NULL)
        InsertHeadList(&CanceledNetBufferList, pListEntry);
    while ((pListEntry = MpQueueGetNext(&pAdapter->Tx_qMpOwnedBDs)) != NULL)
        InsertHeadList(&CanceledNetBufferList, pListEntry);
    CompletionStatus = pAdapter->NdisStatus;                                 // Get the completion status to use
    BOOLEAN isAnyTxFrameCanceled = !IsListEmpty(&CanceledNetBufferList);     // The status to return...
    NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
    // Unwind all canceled NET_BUFFERs
    while (!IsListEmpty(&CanceledNetBufferList)) {
        pListEntry = RemoveTailList(&CanceledNetBufferList);
        ASSERT(pListEntry != NULL);
        PMP_TX_BD pMpTxBD = CONTAINING_RECORD(pListEntry, MP_TX_BD, Link);   // Get pMpTxBD address
        MpTxUnwindNetBuffer(pAdapter, pMpTxBD->pNBL, pMpTxBD->pNB, CompletionStatus, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
    }
    return isAnyTxFrameCanceled;
}

/*++
Routine Description:
    CancelSendHandler handler.
    NDIS will call This method() to cancel a group of pending TX packets.  This method will dequeue all Tx packets that match the CancelId and complete.
    TX packets that are already mapped into DMA descriptors will NOT be cancelled, those will be completed once the DMA transfer is done.
Argument:
    MiniportAdapterContext
        Our adapter context
    CancelId
        The NET_BUFFER_LIST cancel filter.
Return Value:
    None
--*/
_Use_decl_annotations_
void MpCancelSendNetBufferLists(NDIS_HANDLE MiniportAdapterContext, PVOID CancelId)
{
    PMP_ADAPTER       pAdapter = (PMP_ADAPTER)MiniportAdapterContext;
    LIST_ENTRY        CanceledNBList;       // The list of NET_BUFFERs associated with NET_BUFFER_LISTs that should be cancelled.

    InitializeListHead(&CanceledNBList);
    NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
    PLIST_ENTRY pListEntry = MpQueuePeekFirst(&pAdapter->Tx_qMpOwnedBDs);
    while (pListEntry != NULL) {
        PMP_TX_BD pMpTxBD = CONTAINING_RECORD(pListEntry, MP_TX_BD, Link);         // Get pMpTxBD address
        if (NDIS_GET_NET_BUFFER_LIST_CANCEL_ID(pMpTxBD->pNBL) == CancelId) {       // Compare CancelIds
            PLIST_ENTRY curpListEntry = pListEntry;                                // Remember current pMpTxBD
            pListEntry = MpQueuePeekNext(&pAdapter->Tx_qMpOwnedBDs, pListEntry);   // Move to next pMpTxBD, before removing...
            MpQueueRemoveEntry(&pAdapter->Tx_qMpOwnedBDs, curpListEntry);          // Remove pMpTxBD from Tx_qMpOwnedBDs queue
            InsertHeadList(&CanceledNBList, &pMpTxBD->Link);                       // Add pMpTxBD to the cancel ready queue
        } else {
            pListEntry = MpQueuePeekNext(&pAdapter->Tx_qMpOwnedBDs, pListEntry);   // CancelIds are different, move to the next pMpTxBD
        }
    }
    NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
    while (!IsListEmpty(&CanceledNBList)) {                                        // Unwind all cancelled NET_BUFFERs
        pListEntry = RemoveTailList(&CanceledNBList);
        ASSERT(pListEntry != NULL);
        PMP_TX_BD pMpTxBD = CONTAINING_RECORD(pListEntry, MP_TX_BD, Link);
        MpTxUnwindNetBuffer(pAdapter, pMpTxBD->pNBL, pMpTxBD->pNB, NDIS_STATUS_SEND_ABORTED, 0);
    }
}

/*++
Routine Description:
   Copy data in a packet to the specified location
Arguments:
    pMpTxBD         A pointer to the source buffer
    pEnetSwExtBD    A pointer to the destination buffer
Return Value:
    The number of bytes actually copied
--*/
ULONG MpCopyNetBuffer(_In_ PMP_TX_BD pMpTxBD, _Inout_ PMP_TX_PAYLOAD_BD pEnetSwExtBD)
{
    ULONG          CurrLength=0;
    PUCHAR         pSrc=NULL;
    PUCHAR         pDest;
    ULONG          BytesCopied = 0;
    ULONG          Offset;
    PMDL           CurrentMdl;
    ULONG          DataLength;
    PNET_BUFFER    NetBuffer = pMpTxBD->pNB;
    PSCATTER_GATHER_LIST sgListPtr = pMpTxBD->pSGList;

//TODO    DBG_ENET_DEV_TX_METHOD_BEG();
    pDest = pEnetSwExtBD->pBuffer;
    CurrentMdl = NET_BUFFER_FIRST_MDL(NetBuffer);
    Offset = NET_BUFFER_DATA_OFFSET(NetBuffer);
    DataLength = NET_BUFFER_DATA_LENGTH(NetBuffer);

    while (CurrentMdl && DataLength > 0) {
        NdisQueryMdl(CurrentMdl, &pSrc, &CurrLength, NormalPagePriority);
        if (pSrc == NULL) {
            BytesCopied = 0;
            break;
        }
        //  Current buffer length is greater than the offset to the buffer
        if (CurrLength > Offset)  {
            pSrc += Offset;
            CurrLength -= Offset;

            if (CurrLength > DataLength) {
                CurrLength = DataLength;
            }
            DataLength -= CurrLength;
            NdisMoveMemory(pDest, pSrc, CurrLength);
            BytesCopied += CurrLength;

            pDest += CurrLength;
            Offset = 0;
        } else {
            Offset -= CurrLength;
        }
        NdisGetNextMdl(CurrentMdl, &CurrentMdl);

    }
    if ((BytesCopied != 0) && (BytesCopied < ETHER_FRAME_NIN_LENGTH))  {
        NdisZeroMemory(pDest, ETHER_FRAME_NIN_LENGTH - BytesCopied);
    }
    NdisAdjustMdlLength(pEnetSwExtBD->pMdl, BytesCopied);
    ASSERT(BytesCopied <= pEnetSwExtBD->BufferSize);

    sgListPtr->Elements[0].Address = pEnetSwExtBD->BufferPa;
    sgListPtr->Elements[0].Length = BytesCopied;
//TODO    DBG_ENET_DEV_TX_METHOD_END();
    return BytesCopied;
}

/*++
Routine Description:
    It is called to map all NET_BUFFER scatter gather elements into TX DMA descriptors.
Arguments:
    pAdapter    Address of the adapter context
    pMpTxBD     Address of the TCB to be freed
Return Value:
    None
--*/
void MpTxFillEnetTxBD(_In_ PMP_ADAPTER pAdapter, _In_ PMP_TX_BD pMpTxBD)
{
    PSCATTER_GATHER_LIST sgListPtr = pMpTxBD->pSGList;
    LONG                 EnetFreeBDIdx = pAdapter->Tx_EnetFreeBDIdx;            // First free Ethernet packet hw buffer descriptor index
    volatile ENET_BD    *pFreeEnetBD = &pAdapter->Tx_DmaBDT[EnetFreeBDIdx];    // First free Ethernet packet hw buffer descriptor address
    USHORT               ControlStatus;
    ULONG                bytesToSent;

    ASSERT(sgListPtr != NULL);
    ASSERT(sgListPtr->NumberOfElements > 0);

    DBG_ENET_DEV_TX_METHOD_BEG();
    bytesToSent = MpCopyNetBuffer(pMpTxBD, &pAdapter->Tx_EnetSwExtBDT[EnetFreeBDIdx]);          // Copy data to driver provided buffer
    ASSERT(bytesToSent);
    ASSERT(!(pFreeEnetBD->ControlStatus & ENET_TX_BD_R_MASK));
    pAdapter->Tx_EnetSwExtBDT[EnetFreeBDIdx].pMpBD = pMpTxBD;                                   // Associate sw MP_TxBD with current hw ENET_TxBD
    ControlStatus = ENET_TX_BD_L_MASK | ENET_TX_BD_TC_MASK | ENET_TX_BD_R_MASK;                 // Prepare transfer flags
    if (++EnetFreeBDIdx == pAdapter->Tx_DmaBDT_ItemCount) {                                     // Update Free BD index
        ControlStatus |= ENET_TX_BD_W_MASK;                                                     // Last BD in BDT must have WRAP bit set
        EnetFreeBDIdx = 0;                                                                      // Free BD is the first item of Tx_DmaBDT
    }
    pAdapter->Tx_EnetFreeBDIdx = EnetFreeBDIdx;
    pAdapter->Tx_EnetFreeBDCount--;

    pFreeEnetBD->DataLen        = (USHORT)sgListPtr->Elements[0].Length;                       // Set ENET_TxBD data length
    pFreeEnetBD->BufferAddress  = NdisGetPhysicalAddressLow(sgListPtr->Elements[0].Address);   // Set ENET_TxBD data address
    pFreeEnetBD->ControlStatus  = ControlStatus;                                               // Write ControlStatus word of BD as last step
    _DataSynchronizationBarrier();                                                             // Wait for read is finished
    ControlStatus = pFreeEnetBD->ControlStatus;                                                // Read ControlStatus back
    _DataSynchronizationBarrier();                                                             // Wait for read is finished
    DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d): Added to ENET_BD, Size: %5d.",pMpTxBD->NBId,(USHORT)sgListPtr->Elements[0].Length);
    if (pAdapter->ENETRegBase->TDAR == 0) {
        _DataSynchronizationBarrier();                                                         // Wait for read is finished
        if (pFreeEnetBD->ControlStatus & ENET_TX_BD_R_MASK) {                                  // Transfer not started yet?
            DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d): Starting transfer. TDAR: 0x%08X, EIR: 0x%08X", pMpTxBD->NBId, pAdapter->ENETRegBase->TDAR, pAdapter->ENETRegBase->EIR.U);
            pAdapter->ENETRegBase->TDAR = 0x00000000;                                          // No, start transfer
        }
    }
    DBG_ENET_DEV_TX_METHOD_END();
}

/*++
Routine Description:
    Tries to send the next pending Ethernet frame (Net buffer). If an outgoing frame is pending, it makes sure we have enough
    free BDs to accommodate for the frame fragments. If we do, the frame is taken out from
    the queue, the required TFD list is setup to map the frame fragments and the transmission is
    initiated.
    The above process continues until there are no more TX frames to send or we exhausted all
    our free TFDs, and we need to wait for a TX frame to complete before we can send the next
    pending frames.
Arguments:
    pAdapter    Address of the adapter context
Return Value:
    None
--*/
void MpSendNextNB(_In_ PMP_ADAPTER pAdapter)
{
    DBG_ENET_DEV_TX_METHOD_BEG();
    NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
    do {
        if (pAdapter->NdisStatus != NDIS_STATUS_SUCCESS)  {                       // Make sure the adapter is ready
            DBG_SM_PRINT_TRACE("NIC is not ready ");
            break;
        }
        for (;;) {
            if ((pAdapter->Tx_EnetFreeBDCount == 0)) {                            // No Dma BD empty?
//TODO                DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d) - OUT of ENET_TxBD", pAdapter->Tx_pCurrentMpBD->NBId);
                break;                                                            // Do nothing, Tx DPC will dequeue NB from Tx_qMpOwnedBDs
            }
            PLIST_ENTRY pListEntry = MpQueueGetNext(&pAdapter->Tx_qMpOwnedBDs);   // Get NB from remove from Miniport queue
            if (pListEntry == NULL) {                                             // Queue empty?
                DBG_ENET_DEV_TX_PRINT_TRACE("Tx_qMpOwnedBDs EMPTY");
                break;                                                            // Yes, no more NBs to send.
            }
            PMP_TX_BD Tx_pCurrentMpBD = Tx_pCurrentMpBD = CONTAINING_RECORD(pListEntry, MP_TX_BD, Link);  // Get NB address
            MpQueueAdd(&pAdapter->Tx_qDmaOwnedBDs, &Tx_pCurrentMpBD->Link);                               // Add NB to the DMA queue
            MpTxFillEnetTxBD(pAdapter, Tx_pCurrentMpBD);                                                  // Put data to HW add start transfer
        } // Keep processing queued TX frames
    } while (0);
    NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
    DBG_ENET_DEV_TX_METHOD_END();
}

/*++
Routine Description:
    It is called by NDIS to process a scatter/gather list for a NET_BUFFER.
    The routine makes sure we have enough TXDs to satisfy Ethernet NET_BUFFER scatter/gather list
    requirements, and if we do, the buffer is queued for transmission.
Arguments:
    DeviceObjectPtr Address of the related device object.
    Reserved        Not used
    SGListPtr       The scatter gather list for the related buffer
    ContextPtr      Address of the related ST_MP_TCB object
Return Value:
    None
--*/
_Use_decl_annotations_
void MpProcessSGList(PDEVICE_OBJECT DeviceObjectPtr, PVOID Reserved, PSCATTER_GATHER_LIST SGListPtr, PVOID ContextPtr)
{
    PMP_TX_BD         pMpTxBD = (PMP_TX_BD)(ContextPtr);

    UNREFERENCED_PARAMETER(DeviceObjectPtr);
    UNREFERENCED_PARAMETER(Reserved);

//TODO    DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d) Adding to Tx_qMpOwnedBDs queue", pMpTxBD->NBId);
    pMpTxBD->pSGList = SGListPtr;
    MpQueueAdd(&pMpTxBD->pAdapter->Tx_qMpOwnedBDs, &pMpTxBD->Link);
}

/*++
Routine Description:
    NDIS calls this method to send a list of Tx Ethernet frames through the network. Each NET_BUFFER structure
    that is linked to a NET_BUFFER_LIST structure describes a single Ethernet frame.
Argument:
    MiniportAdapterContext  Adapter context.
    NetBufferListPtr        A linked list of NET_BUFFER_LIST objects that we previously indicated to NDIS.
    PortNumber              Not used.
    SendFlags               Flags specifying if the caller is at DISPATCH_LEVEL.
Return Value:
    None
--*/
_Use_decl_annotations_
void MpSendNetBufferLists(NDIS_HANDLE MiniportAdapterContext, PNET_BUFFER_LIST NetBufferListPtr, NDIS_PORT_NUMBER PortNumber, ULONG SendFlags)
{
    ULONG             uQueuedPacketCounter = 0;
    NDIS_STATUS       status               = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER       pAdapter             = (PMP_ADAPTER)MiniportAdapterContext;
    PNET_BUFFER_LIST  pNextNBL             = NetBufferListPtr;
    PNET_BUFFER_LIST  pCurrentNBL;
    PNET_BUFFER       pCurrentNB;
    PMP_TX_BD         pMpTxBD;

    UNREFERENCED_PARAMETER(PortNumber);
    DBG_ENET_DEV_TX_METHOD_BEG();

    for(;;) {
        NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
        status = pAdapter->NdisStatus;
        NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
        if (status != NDIS_STATUS_SUCCESS)  {                                                 // Make sure the adapter is ready
            DBG_SM_PRINT_TRACE("NIC is not ready ");
            break;
        }
        while (pNextNBL != NULL) {                                                            // For all NBL in NBL chain do
            pCurrentNBL = pNextNBL;                                                           // Save current NBL address
            pNextNBL = NET_BUFFER_LIST_NEXT_NBL(pNextNBL);                                    // Save next NBL address
            NET_BUFFER_LIST_NEXT_NBL(pCurrentNBL) = NULL;                                     // Detach current NBL from NBL chain
            #if DBG
            MP_NBL_SET_ID(pCurrentNBL, NdisInterlockedIncrement(&pAdapter->Tx_NBLCounter));   // Save NBL sequence number
            #endif
            // For all NB in current NBL do:
            for (pCurrentNB = NET_BUFFER_LIST_FIRST_NB(pCurrentNBL); pCurrentNB != NULL; pCurrentNB = NET_BUFFER_NEXT_NB(pCurrentNB)) {
                NdisInterlockedIncrement(&MP_NBL_NB_Counter(pCurrentNBL));                     // Increment pending NB counter in NBL
                pMpTxBD = (PMP_TX_BD)NdisAllocateFromNPagedLookasideList(&pAdapter->Tx_MpTxBDLookasideList);
                if (pMpTxBD == NULL) {
                    DBG_ENET_DEV_TX_PRINT_TRACE("Fail to allocate memory for MpTxBD");
                    status = NDIS_STATUS_RESOURCES;                                           // Fail to allocate memory from non-paged pool
                    break;
                }
                MP_NB_SET_pMpTxBD(pCurrentNB, pMpTxBD);
                #if DBG
                KeQuerySystemTimePrecise(&MP_NB_Time(pCurrentNB));
                pMpTxBD->NBId                   = NdisInterlockedIncrement(&pAdapter->Tx_NBCounter);   // Save NB sequence number
                DBG_ENET_DEV_TX_PRINT_TRACE("### Sending NBL(%d), NB(%d)", MP_NBL_ID(pCurrentNBL),pMpTxBD->NBId);
                #endif
                DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d) MpTxBD allocated ", pMpTxBD->NBId);
                pMpTxBD->pAdapter  = pAdapter;
                pMpTxBD->pNBL      = pCurrentNBL;                          // Associate NBL with MpTxBD
                pMpTxBD->pNB       = pCurrentNB;                           // Associate NB with MpTxBD
                pMpTxBD->pSGList   = NULL;
                // Map the buffer to its physically contiguous fragments. NdisMAllocateNetBufferSGList needs to be called at DISPATCH_LEVEL
                DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d) Calling AllocSGList()", pMpTxBD->NBId);
                NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
                status = NdisMAllocateNetBufferSGList(pAdapter->Tx_DmaHandle, pCurrentNB, pMpTxBD, NDIS_SG_LIST_WRITE_TO_DEVICE, &pMpTxBD->SGList, pAdapter->Tx_SGListSize);
                NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
                if (status != NDIS_STATUS_SUCCESS) {                       // Fail to allocate memory from non-paged pool for SGList
                    DBG_ENET_DEV_PRINT_ERROR("NB(%d) NdisMAllocateNetBufferSGList() failed. Status: 0x%08X", pMpTxBD->NBId, status);
                    break;
                }
                NdisInterlockedIncrement(&pAdapter->Tx_PendingNBs);
            }
            if (status == NDIS_STATUS_SUCCESS) {
                uQueuedPacketCounter++;
            } else {  // handle error case
                if (pCurrentNB != NULL) {
                    pMpTxBD = MP_NB_pMpTxBD(pCurrentNB);
                    if (pMpTxBD != NULL) {
                        DBG_ENET_DEV_TX_PRINT_TRACE("Discarding NB(%d) 0x%08X", pCurrentNB, pMpTxBD->NBId);
                        if (pMpTxBD->pSGList != NULL) {
                            NdisMFreeNetBufferSGList(pAdapter->AdapterHandle, pMpTxBD->pSGList, pCurrentNB);   // Free SG list
                        }
                        NdisFreeToNPagedLookasideList(&pAdapter->Tx_MpTxBDLookasideList, pMpTxBD);             // Free TxBD
                    }
                    if (NdisInterlockedDecrement(&MP_NBL_NB_Counter(pCurrentNBL)) == 0) {
                        ULONG sendCompleteFlags = 0;
                        if (NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags)) {
                            NDIS_SET_SEND_COMPLETE_FLAG(sendCompleteFlags, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
                        }
                        NET_BUFFER_LIST_STATUS(NetBufferListPtr) = status;
                        DBG_ENET_DEV_TX_PRINT_TRACE("Completing NBL: 0x%08X, Status: 0x%08X", pCurrentNBL, status);
                        NdisMSendNetBufferListsComplete(pAdapter->AdapterHandle, pCurrentNBL, sendCompleteFlags);
                    }
                }
            }
        } // More NBLs
        break;
    }
    if (status != NDIS_STATUS_SUCCESS) {    // On failure, complete (fail) the remaining packets (NBLs)
        ULONG sendCompleteFlags = 0;
        if (NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags)) {
            NDIS_SET_SEND_COMPLETE_FLAG(sendCompleteFlags, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
        }
        if (pNextNBL) { // Complete remaining packets (NBLs)
            for (PNET_BUFFER_LIST tmp_pNBL = pNextNBL; tmp_pNBL != NULL;  tmp_pNBL = NET_BUFFER_LIST_NEXT_NBL(tmp_pNBL)) {
                NET_BUFFER_LIST_STATUS(tmp_pNBL) = status;
                DBG_ENET_DEV_TX_PRINT_TRACE("Completing NBL: 0x%08, Status: 0x%08", tmp_pNBL, status);
            }
            NdisMSendNetBufferListsComplete(pAdapter->AdapterHandle, pNextNBL, sendCompleteFlags);
        }
    }
    if (uQueuedPacketCounter != 0) // If at lease one NBL (packet) was queued, kick off the transmission, if it had not been already started.
        MpSendNextNB(pAdapter);    // Send queued NBs (Ethernet frames)
    DBG_ENET_DEV_TX_METHOD_END();
}

/*++
Routine Description:
    It is called from EnetDpc() to handle 'frame transmission complete' interrupts.
    MpHandleTxInterrupt() scans the TFD list for transmitted frames, notifies NDIS,
    and tries to send the next queued outgoing frame.
Arguments:
    pAdapter        The miniport adapter context
    InterruptEvent  The Interrupt Event Register image.
Return Value:
    None
--*/
_Use_decl_annotations_
void MpHandleTxInterrupt(PMP_ADAPTER pAdapter, UINT32 InterruptEvent)
{
    LIST_ENTRY         completedNetBufferList;
    LONG               EnetPendingBDIdx;
    volatile ENET_BD  *pDmaTxBD;
    PMP_TX_BD          pMpTxBD = NULL;

    UNREFERENCED_PARAMETER(InterruptEvent);
    InitializeListHead(&completedNetBufferList);
    DBG_ENET_DEV_DPC_TX_METHOD_BEG();

    NdisDprAcquireSpinLock(&pAdapter->Tx_SpinLock);
    EnetPendingBDIdx = pAdapter->Tx_EnetPendingBDIdx;
    DBG_ENET_DEV_TX_PRINT_TRACE("**** ISR,  Tx_EnetPendingBDIdx: %d, Tx_EnetFreeBDIdx: %d, flags: 0x%08X, TDAR: 0x%08X ****", pAdapter->Tx_EnetPendingBDIdx, pAdapter->Tx_EnetFreeBDIdx, InterruptEvent, pAdapter->ENETRegBase->TDAR);
    do {
        pMpTxBD = pAdapter->Tx_EnetSwExtBDT[EnetPendingBDIdx].pMpBD;                     // Get Mp NB Tx BD
        if (pMpTxBD == NULL) {                                                           // Mp NB Tx BD already processed as the first item in this loop?
            break;                                                                       // Break the loop
        }
        pDmaTxBD = &pAdapter->Tx_DmaBDT[EnetPendingBDIdx];                               // Get Dma Tx BD
        if (pDmaTxBD->ControlStatus & ENET_TX_BD_R_MASK) {                               // Dma Tx BD owned by DMA engine?
            if (pAdapter->ENETRegBase->TDAR == 0) {                                      // DMA stopped? (ERR006358 bug fix)
                pAdapter->ENETRegBase->TDAR = 0x0000000;                                 // Restart DMA
            }
            break;                                                                       // Break the loop
        }
        pAdapter->Tx_EnetSwExtBDT[EnetPendingBDIdx].pMpBD = NULL;                        // Mark Mp NB Tx BD as "already processed"
        if (++EnetPendingBDIdx >= pAdapter->Tx_DmaBDT_ItemCount)                         // Updated ENET_BDT index
            EnetPendingBDIdx = 0;
        pAdapter->Tx_EnetFreeBDCount ++;                                                 // Update Free ENET_TxBD counter
        pAdapter->Tx_EnetPendingBDIdx = EnetPendingBDIdx;                                // Update pending BD index
        DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d) 0x%08X done, adding it to the complete queue.", pMpTxBD->NBId, pMpTxBD->pNB);
        (void)MpQueueGetNext(&pAdapter->Tx_qDmaOwnedBDs);           // Remove the TX BD from the 'in progress' queue
        InsertHeadList(&completedNetBufferList, &pMpTxBD->Link);                         // Put BD to the completed BD queue
        pAdapter->Tx_CheckForHangCounter = 0;                                            // Restart "check for hang" counter
    } while (EnetPendingBDIdx != pAdapter->Tx_EnetFreeBDIdx);
    DBG_ENET_DEV_TX_PRINT_TRACE("**** ISR, Before release spin lock, Tx_EnetPendingBDIdx: %d, Tx_EnetFreeBDIdx: %d", pAdapter->Tx_EnetPendingBDIdx, pAdapter->Tx_EnetFreeBDIdx);
    NdisDprReleaseSpinLock(&pAdapter->Tx_SpinLock);

    while (!IsListEmpty(&completedNetBufferList)) {                                      // Unwind all completed NET_BUFFERs
        PLIST_ENTRY pListEntry = RemoveTailList(&completedNetBufferList);
        ASSERT(pListEntry != NULL);
        pMpTxBD = CONTAINING_RECORD(pListEntry, MP_TX_BD, Link);
        DBG_ENET_DEV_TX_PRINT_TRACE("NB(%d) 0x%08X removed from complete queue.", pMpTxBD->NBId,pMpTxBD->pNB);
        NDIS_STATUS completionStatus = (InterruptEvent & ENET_TX_ERR_INT_MASK)? NDIS_STATUS_FAILURE : NDIS_STATUS_SUCCESS;   // Get the completion status
        MpTxUnwindNetBuffer(pAdapter, pMpTxBD->pNBL, pMpTxBD->pNB, completionStatus, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
    } // More completed TX frames
    MpSendNextNB(pAdapter);            // Send next waiting TX frames, if any...
    DBG_ENET_DEV_DPC_TX_METHOD_END();
    return;
}

/*++
Routine Description:
    Initialize send data structures
Arguments:
    pAdapter     Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
VOID MpTxInit(PMP_ADAPTER pAdapter)
{
    pAdapter->Tx_CheckForHangCounter = 0;
    pAdapter->Tx_EnetFreeBDCount     = pAdapter->Tx_DmaBDT_ItemCount;    // Initialize number of unused Ethernet Buffer Descriptors
    pAdapter->Tx_EnetFreeBDIdx       = 0;
    pAdapter->Tx_EnetPendingBDIdx    = 0;

    pAdapter->TxdStatus.FramesXmitGood            = 0;
    pAdapter->TxdStatus.FramesXmitBad             = 0;
    pAdapter->TxdStatus.FramesXmitHBErrors        = 0;
    pAdapter->TxdStatus.FramesXmitUnderrunErrors  = 0;
    pAdapter->TxdStatus.FramesXmitCollisionErrors = 0;
    pAdapter->TxdStatus.FramesXmitAbortedErrors   = 0;
    pAdapter->TxdStatus.FramsXmitCarrierErrors    = 0;
    NdisZeroMemory((VOID*)pAdapter->Tx_DmaBDT, pAdapter->Tx_DmaBDT_Size);      // Zero TxBDT
}

/*++
Routine Description:
    Initialize receive data structures.
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void MpRxInit(PMP_ADAPTER pAdapter)
{
    PENET_BD             pDmaBD = NULL;

    ASSERT(pAdapter->Rx_DmaBDT_ItemCount);                                                // There must be at least one Rx buffer
    pAdapter->Rx_EnetFreeBDIdx           = 0;                                             // Initialize HW Dma buffer descriptor ring index
    pAdapter->Rx_EnetPendingBDIdx        = 0;                                             
    pAdapter->Rx_NBLCounter              = 0;
    pAdapter->Rx_NdisOwnedBDsCount       = 0;                                             // No buffer is owned by NDIS
    pAdapter->Rx_DmaBDT_DmaOwnedBDsCount = pAdapter->Rx_DmaBDT_ItemCount;                 // All Rx BDs are owned by ENET DMA
    NdisZeroMemory(&pAdapter->RcvStatus, sizeof(pAdapter->RcvStatus));
    for (LONG Idx = 0; Idx < pAdapter->Rx_DmaBDT_ItemCount; ++Idx) {                      // For each DmaBD do:
        MP_RX_FRAME_BD *pRxFrameBD = &pAdapter->Rx_FrameBDT[Idx];
        pDmaBD = &pAdapter->Rx_DmaBDT[Idx];                                               // Get DmaBD address
        pAdapter->Rx_DmaBDT_SwExt[Idx].pRxFrameBD = pRxFrameBD;                           // Create link between Rx frame descriptor and DmaBD
        NET_BUFFER_LIST_NEXT_NBL(pRxFrameBD->pNBL) = NULL;                                // Not necessary consider removing
        pDmaBD->BufferAddress = pRxFrameBD->BufferPa.LowPart;                             // Fill DmaBD data buffer address
        pDmaBD->ControlStatus = ENET_RX_BD_E_MASK | ENET_RX_BD_L_MASK;                    // Fill DMaBD Status (Mark DmaBD as ready to receive data)
        /* MS-temp */ NdisAdjustMdlLength(pRxFrameBD->pMdl, ENET_RX_FRAME_SIZE);
    }
    if (pDmaBD) {
        pDmaBD->ControlStatus |= ENET_RX_BD_W_MASK;                                       // Mark last DmaBD
    }
}

/*++
Routine Description:
    Returns TRUE if ndis owns at lease one RX buffer.
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    TRUE - Ndis owns at lease one RX buffer.
    FALSE - Ndis owns no RX buffer.
--*/
_Use_decl_annotations_
BOOLEAN IsRxFramePandingInNdis(PMP_ADAPTER pAdapter) {
    BOOLEAN RxFramePanding;
    NdisAcquireSpinLock(&pAdapter->Rx_SpinLock);
    RxFramePanding = pAdapter->Rx_NdisOwnedBDsCount != 0;
    NdisReleaseSpinLock(&pAdapter->Rx_SpinLock);
    return RxFramePanding;
}

/*++
Routine Description:
    MiniportReturnNetBufferLists handler. NDIS will call this function once it is done with the RX frames
    indicated using NdisMIndicateReceiveNetBufferLists(),  so the miniport can re-use them for new RX frames.
Argument:
    MiniportAdapterContext
        Our adapter context
    pNBL
        A linked list of NET_BUFFER_LIST objects that we previously indicated to NDIS.
    ReturnFlags
        Flags specifying if the caller is at DISPATCH_LEVEL.
Return Value:
    None
--*/
_Use_decl_annotations_
void MpReturnNetBufferLists(NDIS_HANDLE MiniportAdapterContext, PNET_BUFFER_LIST pNBL, ULONG ReturnFlags)
{
    PMP_ADAPTER       pAdapter = (PMP_ADAPTER)MiniportAdapterContext;
    PNET_BUFFER_LIST  pNextNBL;
    PMP_RX_FRAME_BD   pRxFrameBD;
    PENET_BD          pCurrentDmaBD;
    PENET_BD          pFirstDmaBD = NULL;
    USHORT            CurrentControlStatus, FirtsControlStatus = 0;
    LONG              Rx_EnetFreeBDIdx;

    UNREFERENCED_PARAMETER(ReturnFlags);
    DBG_ENET_DEV_RX_METHOD_BEG();
    NdisAcquireSpinLock(&pAdapter->Rx_SpinLock);
    Rx_EnetFreeBDIdx = pAdapter->Rx_EnetFreeBDIdx;
    // Mark all returned frames as active, so adapter DMA can use them for future RX frames.
    ASSERT(pNBL);
    for (PNET_BUFFER_LIST pCurrentNBL = pNBL; pCurrentNBL != NULL; pCurrentNBL = pNextNBL) {
        pNextNBL = NET_BUFFER_LIST_NEXT_NBL(pCurrentNBL);
        pRxFrameBD = MP_NBL_RX_FRAME_BD(pCurrentNBL);                               // Get Frame BD address from the current NBL.
        /* MS-temp */ NdisAdjustMdlLength(pRxFrameBD->pMdl, ENET_RX_FRAME_SIZE);
        pAdapter->Rx_DmaBDT_DmaOwnedBDsCount++;                                     // Increment counter of Rx BDs owned by ENET DMA.
        pAdapter->Rx_NdisOwnedBDsCount--;
        if (!pAdapter->EnetStarted) {
            continue;
        }
        ASSERT(!pAdapter->Rx_DmaBDT_SwExt[Rx_EnetFreeBDIdx].pRxFrameBD);
        /* Reuse frame descriptor */
        pAdapter->Rx_DmaBDT_SwExt[Rx_EnetFreeBDIdx].pRxFrameBD = pRxFrameBD;    // Association current Frame BD and the first free Dma BD
        DBG_ENET_DEV_RX_PRINT_TRACE("NBL(%4d, 0x%08X) returned,       DmaIdx: %4d, NewDmaIdx: %4d DmaBD ready: %4d, PhyAddr: 0x%08X", MP_NBL_ID(pCurrentNBL), pCurrentNBL, MP_NB_DmaIdx(pCurrentNBL->FirstNetBuffer), Rx_EnetFreeBDIdx, pAdapter->Rx_DmaBDT_DmaOwnedBDsCount, pRxFrameBD->BufferPa.LowPart);
        pCurrentDmaBD                = &pAdapter->Rx_DmaBDT[Rx_EnetFreeBDIdx];  // Get address of the first free Dma BD
        pCurrentDmaBD->BufferAddress = pRxFrameBD->BufferPa.LowPart;            // Fill Dma BD data buffer address
        CurrentControlStatus = ENET_RX_BD_E_MASK;                               // Set EMPTY bit
        CurrentControlStatus |= ENET_RX_BD_L_MASK;                              // Set LAST bit
        if (++Rx_EnetFreeBDIdx == pAdapter->Rx_DmaBDT_ItemCount) {              // Compute next Rx_EnetFreeBDIdx
            Rx_EnetFreeBDIdx = 0;
            CurrentControlStatus |= ENET_RX_BD_W_MASK;                          // Set WRAP bit in the last Dma BD
        }
        if (pFirstDmaBD == NULL) {                                              // For the first returned BD do not set Dma BD control and status word now, do it as the last step
            pFirstDmaBD = pCurrentDmaBD;                                        // Remember the first free Dma BD address
            FirtsControlStatus = CurrentControlStatus;                          // Remember Dma BD control and status word for the first free Dma BD
        } else {
            pCurrentDmaBD->ControlStatus = CurrentControlStatus;                // Fill Dma BD control and status word
        }
    } // More free buffers
    pAdapter->Rx_EnetFreeBDIdx = Rx_EnetFreeBDIdx;                              // Update Rx_EnetFreeBDIdx
    if (pFirstDmaBD != NULL) {
        pFirstDmaBD->ControlStatus = FirtsControlStatus;                        // Mark first Dma BD as empty = ready to receive data
        _DataSynchronizationBarrier();                                          // Wait until write is finished
        FirtsControlStatus = pFirstDmaBD->ControlStatus;                        // Read ControlStatus back
        DBG_ENET_DEV_RX_PRINT_TRACE("NBL(%4d): Added to ENET_BD.",MP_NBL_ID(pNBL));
        if (pAdapter->ENETRegBase->RDAR == 0) {                                 // Receive in progress?
            if (pFirstDmaBD->ControlStatus & ENET_RX_BD_E_MASK) {               // No, Transfer not started yet?
                DBG_ENET_DEV_RX_PRINT_TRACE("NBL(%4d): Starting transfer. TDAR: 0x%08X, EIR: 0x%08X", MP_NBL_ID(pNBL), pAdapter->ENETRegBase->RDAR, pAdapter->ENETRegBase->EIR.U);
                pAdapter->ENETRegBase->RDAR = 0x00000000;                       // No, start transfer
            }
        }
    }
    NdisReleaseSpinLock(&pAdapter->Rx_SpinLock);
    DBG_ENET_DEV_RX_METHOD_END();
}

/*++
Routine Description:
    Interrupt handler for receive processing. Put the received packets into an array and call
    NdisMIndicateReceivePacket. If we run low on RFDs, allocate another one
    Assumption: Receive spin lock has been acquired
Arguments:
    pAdapter
        Pointer to the adapter structure.
    pMaxNBLsToIndicate
        A pointer to the maximal number of RX frames we indicate to NDIS
    pRecvThrottleParameters
        A pointer to an NDIS_RECEIVE_THROTTLE_PARAMETERS structure. This structure specifies
        the maximum number of NET_BUFFER_LIST structures that a miniport driver should indicate in a DPC.
Return Value:
    None
--*/
_Use_decl_annotations_
void MpHandleRecvInterrupt(PMP_ADAPTER pAdapter, PULONG pMaxNBLsToIndicate, PNDIS_RECEIVE_THROTTLE_PARAMETERS pRecvThrottleParameters)
{
    PNET_BUFFER_LIST *ppNBLTail;
    PNET_BUFFER_LIST pErrorNBLHead     = NULL;
    ULONG            ErrorNBLItemCount = 0;
    PNET_BUFFER_LIST pAsyncNBLHead     = NULL;
    PNET_BUFFER_LIST pAsyncNBLTail     = NULL;
    ULONG            AsyncNBLItemCount = 0;
    PNET_BUFFER_LIST pSyncNBLHead      = NULL;
    PNET_BUFFER_LIST pSyncNBLTail      = NULL;
    ULONG            SyncNBLItemCount = 0;
    LONG             Rx_EnetPendingBDIdx;

    DBG_ENET_DEV_DPC_RX_METHOD_BEG();
    NdisDprAcquireSpinLock(&pAdapter->Rx_SpinLock);
    if (pAdapter->NdisStatus != NDIS_STATUS_SUCCESS) {                        // Mp ready to indicate Rx packets?
       NdisDprReleaseSpinLock(&pAdapter->Rx_SpinLock);                        // No, do nothing
       DBG_ENET_DEV_DPC_RX_METHOD_END();
       return;
    }
    Rx_EnetPendingBDIdx = pAdapter->Rx_EnetPendingBDIdx;
    for (LONG Idx = 0; Idx < pAdapter->Rx_DmaBDT_ItemCount; ++Idx) {          // One call of MpHandleRecvInterrupt() will indicate up to pAdapter->Rx_DmaBDT_ItemCount NBLs
        PENET_BD pDmaBD = &pAdapter->Rx_DmaBDT[Rx_EnetPendingBDIdx];          // Get address of the first not checked BD
        if (pDmaBD->ControlStatus & ENET_RX_BD_E_MASK) {                      // No data received or reception in progress?
            break;                                                            // Stop BD checking
        }
        if (pAdapter->Rx_DmaBDT_DmaOwnedBDsCount == 0) {                      // All NBL has been already indicated to NDIS, next packet will be lost
            break;
        }
        if ((*pMaxNBLsToIndicate) == 0) {                                     // Did we reach the max number of RX frames we are allowed to indicate to NDIS?
            DBG_ENET_DEV_PRINT_WARNING("NDIS RX frame throttle applied %d RX frames will be indicated", AsyncNBLItemCount + SyncNBLItemCount);
            pRecvThrottleParameters->MoreNblsPending = TRUE;                  // No, inform NDIS about it
            break;
        }
        pAdapter->Rx_DmaBDT_DmaOwnedBDsCount--;                                                    // Decrement counter of Rx BDs owned by ENET DMA
        PMP_RX_FRAME_BD pRxFrameBD = pAdapter->Rx_DmaBDT_SwExt[Rx_EnetPendingBDIdx].pRxFrameBD;    // Get frame descriptor
        ASSERT(pRxFrameBD != NULL);
        pAdapter->Rx_DmaBDT_SwExt[Rx_EnetPendingBDIdx].pRxFrameBD = NULL;                          // Disconnect Rx Frame BD from ENET DMA BD
        PNET_BUFFER_LIST  pCurrentNBL     = pRxFrameBD->pNBL;                                      // Get NBL
        ULONG             realFrameLength = (ULONG)pDmaBD->DataLen - ETHER_FRAME_CRC_LENGTH - 2;   // Compute real data length
        #if DBG
        MP_NBL_SET_ID(pCurrentNBL, NdisInterlockedIncrement(&pAdapter->Rx_NBLCounter) - 1);        // for debug only
        MP_NB_SET_DmaIdx(pCurrentNBL->FirstNetBuffer, Rx_EnetPendingBDIdx);                        // for debug only
        #endif
        NET_BUFFER_DATA_LENGTH(pCurrentNBL->FirstNetBuffer) = realFrameLength;                     // Save real data length
        // Is this packet completed and has error bits set?
        if (pDmaBD->ControlStatus & (ENET_RX_BD_TR_MASK | ENET_RX_BD_OV_MASK | ENET_RX_BD_NO_MASK | ENET_RX_BD_CR_MASK)) {
            /* MS-temp */ NdisAdjustMdlLength(pRxFrameBD->pMdl, ENET_RX_FRAME_SIZE);
            NET_BUFFER_LIST_NEXT_NBL(pCurrentNBL) = pErrorNBLHead;        // Append this NBL to the had of the error NBL list
            pErrorNBLHead = pCurrentNBL;
            pAdapter->RcvStatus.FrameRcvErrors++;
            ErrorNBLItemCount++;
            if (pDmaBD->ControlStatus & ENET_RX_BD_TR_MASK) {             // Truncated frame?
                DBG_ENET_DEV_PRINT_ERROR(" NBL(%4d) data received, DmaIdx: %4d, DmaOwnedBDs: %4d:, !!! ERROR Truncated frame !!!, status: 0x%08X, Size: %4d, PhyAddr: 0x%08X", MP_NBL_ID(pCurrentNBL), Rx_EnetPendingBDIdx, pAdapter->Rx_DmaBDT_DmaOwnedBDsCount, pDmaBD->ControlStatus, realFrameLength, pRxFrameBD->BufferPa.LowPart);
                pAdapter->RcvStatus.FrameRcvLCErrors++;
            } else if (pDmaBD->ControlStatus & ENET_RX_BD_OV_MASK) {      // Receive FIFO overrun?
                DBG_ENET_DEV_PRINT_ERROR(" NBL(%4d) data received, DmaIdx: %4d, DmaOwnedBDs: %4d:, !!! ERROR Receive FIFO overrun !!!, status: 0x%08X, Size: %4d, PhyAddr: 0x%08X", MP_NBL_ID(pCurrentNBL), Rx_EnetPendingBDIdx, pAdapter->Rx_DmaBDT_DmaOwnedBDsCount, pDmaBD->ControlStatus, realFrameLength, pRxFrameBD->BufferPa.LowPart);
                pAdapter->RcvStatus.FrameRcvOverrunErrors++;
            } else if (pDmaBD->ControlStatus & ENET_RX_BD_NO_MASK) {      // No-octet aligned frame
                DBG_ENET_DEV_PRINT_ERROR(" NBL(%4d) data received, DmaIdx: %4d, DmaOwnedBDs: %4d:, !!! ERROR No-octet aligned frame !!!, status: 0x%08X, Size: %4d, PhyAddr: 0x%08X", MP_NBL_ID(pCurrentNBL), Rx_EnetPendingBDIdx, pAdapter->Rx_DmaBDT_DmaOwnedBDsCount, pDmaBD->ControlStatus,realFrameLength,  pRxFrameBD->BufferPa.LowPart);
                pAdapter->RcvStatus.FrameRcvAllignmentErrors++;
            } else if (pDmaBD->ControlStatus & ENET_RX_BD_CR_MASK) {      // CRC error?
                DBG_ENET_DEV_PRINT_ERROR(" NBL(%4d) data received, DmaIdx: %4d, DmaOwnedBDs: %4d:, !!! ERROR CRC !!!, status: 0x%08X, Size: %4d, PhyAddr: 0x%08X", MP_NBL_ID(pCurrentNBL), Rx_EnetPendingBDIdx, pAdapter->Rx_DmaBDT_DmaOwnedBDsCount, pDmaBD->ControlStatus, realFrameLength, pRxFrameBD->BufferPa.LowPart);
                pAdapter->RcvStatus.FrameRcvCRCErrors++;
            } else {                                                      // Too long frame
                DBG_ENET_DEV_PRINT_ERROR(" NBL(%4d) data received, DmaIdx: %4d, DmaOwnedBDs: %4d:, !!! ERROR Frame too long !!!, status: 0x%08X, Size: %4d, PhyAddr: 0x%08X", MP_NBL_ID(pCurrentNBL), Rx_EnetPendingBDIdx, pAdapter->Rx_DmaBDT_DmaOwnedBDsCount, pDmaBD->ControlStatus, realFrameLength, pRxFrameBD->BufferPa.LowPart);
                pAdapter->RcvStatus.FrameRcvExtraDataErrors++;
            }
        } else {
            (*pMaxNBLsToIndicate)--;                                                   // Decrement MaxNBLsToIndicate counter
            NdisFlushBuffer(pRxFrameBD->pMdl, FALSE);                                  // Flush Rx buffer
            /* MS-temp */NdisAdjustMdlLength(pRxFrameBD->pMdl, realFrameLength + 2);   // Update real length in MDL
            DBG_ENET_DEV_RX_PRINT_TRACE(" NBL(%4d) data received, DmaIdx: %4d, DmaOwnedBDs: %4d:, Size: %d, PhyAddr: 0x%08X", MP_NBL_ID(pCurrentNBL), Rx_EnetPendingBDIdx, pAdapter->Rx_DmaBDT_DmaOwnedBDsCount, realFrameLength, pRxFrameBD->BufferPa.LowPart);
            // Decide how we are going to indicate the RX buffer to NDIS. If we are running low on RX buffers, we will do in synchronously, otherwise we do it asynchronously.
            if (pAdapter->Rx_DmaBDT_DmaOwnedBDsCount <= pAdapter->Rx_DmaBDT_DmaOwnedBDsLowWatterMark) {
                ppNBLTail = &pSyncNBLTail;                            // Low RX buffers level, use synchronous RX buffer indication
                if (pSyncNBLTail == NULL) {                           // Synchronous NBL list empty?
                    pSyncNBLHead = pCurrentNBL;                       // Current NBL is the first item of the Synchronous NBL list
                }
                SyncNBLItemCount++;                                   // Increment SyncNBLItemCount
                DBG_ENET_DEV_RX_PRINT_TRACE(" NBL(%4d, 0x%08X) indicate  SYNC, DmaIdx: %4d, Size: %d", MP_NBL_ID(pCurrentNBL), pCurrentNBL, MP_NB_DmaIdx(pCurrentNBL->FirstNetBuffer), NET_BUFFER_DATA_LENGTH(pCurrentNBL->FirstNetBuffer));
            } else {
                ppNBLTail = &pAsyncNBLTail;                           // Normal RX buffers, use asynchronous RX buffer indication
                if (pAsyncNBLTail == NULL) {                          // Asynchronous NBL list empty?
                    pAsyncNBLHead = pCurrentNBL;                      // Current NBL is the first item of the Asynchronous NBL list
                }
                AsyncNBLItemCount++;                                  // Increment AsyncNBLItemCount
                DBG_ENET_DEV_RX_PRINT_TRACE(" NBL(%4d, 0x%08X) indicate ASYNC, DmaIdx: %4d, Size: %d", MP_NBL_ID(pCurrentNBL), pCurrentNBL, MP_NB_DmaIdx(pCurrentNBL->FirstNetBuffer), NET_BUFFER_DATA_LENGTH(pCurrentNBL->FirstNetBuffer));
            }
            if (*ppNBLTail != NULL) {                                 // List empty?
                NET_BUFFER_LIST_NEXT_NBL(*ppNBLTail) = pCurrentNBL;   // No, attach current NBL to the tail of the list
            }
            *ppNBLTail = pCurrentNBL;                                 // Remember current tail of the list
            NET_BUFFER_LIST_NEXT_NBL(pCurrentNBL) = NULL;             // Current NBL is the last NBL in the list
            pCurrentNBL->SourceHandle = pAdapter->AdapterHandle;      // Set NBL source handle
        }
        if (++Rx_EnetPendingBDIdx == pAdapter->Rx_DmaBDT_ItemCount) { // Compute next Rx_EnetPendingBDIdx
            Rx_EnetPendingBDIdx = 0;
        }
    } // More RFDs
    pAdapter->Rx_EnetPendingBDIdx = Rx_EnetPendingBDIdx;              // Update Ethernet Dma Rx empty buffer index
    pAdapter->Rx_NdisOwnedBDsCount += AsyncNBLItemCount + SyncNBLItemCount + ErrorNBLItemCount;
    NdisDprReleaseSpinLock(&pAdapter->Rx_SpinLock);
    if (pErrorNBLHead) {
        DBG_ENET_DEV_RX_PRINT_ERROR(" NBL(%4d) received with error, returning back", MP_NBL_ID(pErrorNBLHead));
        MpReturnNetBufferLists(pAdapter, pErrorNBLHead, 0);
    }
    // Indicate received RX frames to NDIS, if any...
    if (pAsyncNBLHead) {    // Asynchronous list not empty?
        NdisMIndicateReceiveNetBufferLists(pAdapter->AdapterHandle, pAsyncNBLHead, NDIS_DEFAULT_PORT_NUMBER, AsyncNBLItemCount, NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL);
    } // Sync list
    if (pSyncNBLHead) {     // Sync list not empty?
        ULONG tcr = ENET_TCR_TFC_PAUSE_MASK | pAdapter->ENETRegBase->TCR.U;
        pAdapter->ENETRegBase->TCR.U = tcr;
        NdisMIndicateReceiveNetBufferLists(pAdapter->AdapterHandle, pSyncNBLHead, NDIS_DEFAULT_PORT_NUMBER, SyncNBLItemCount, NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL | NDIS_RECEIVE_FLAGS_RESOURCES);
        MpReturnNetBufferLists(pAdapter, pSyncNBLHead, 0);
    } // Sync list
    DBG_ENET_DEV_DPC_RX_METHOD_END();
    return;
}

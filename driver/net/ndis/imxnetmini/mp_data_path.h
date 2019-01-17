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

#ifndef _MP_DATA_PATH_H
#define _MP_DATA_PATH_H

#define MAC_RX_BUFFER_LOW_WATER_PERCENT  20


#define MP_NB_pMpTxBD(_NetBuffer)             ((PMP_TX_BD)((_NetBuffer)->MiniportReserved[0]))
#define MP_NB_SET_pMpTxBD(_NetBuffer,_pTxBD)  (_NetBuffer)->MiniportReserved[0] = (PVOID)(_pTxBD)

#define MP_NB_DmaIdx(_NetBuffer)              ((UINT)((_NetBuffer)->MiniportReserved[1]))
#define MP_NB_SET_DmaIdx(_NetBuffer,_DmaIdx)  (_NetBuffer)->MiniportReserved[1] = (PVOID)(_DmaIdx)

#define MP_NB_Time(_NetBuffer)                (*(LARGE_INTEGER*)&((_NetBuffer)->MiniportReserved[2]))        // NOTE: Uses MiniportReserved[2] and MiniportReserved[3] (LARGE_INTEGER)

#define MP_NBL_NB_Counter(_NetBufferList)                          (*(LONG *)&((_NetBufferList)->MiniportReserved[0]))
#define MP_NBL_NB_SET_Counter(_NetBufferList, _Counter)            *(LONG *)&((_NetBufferList)->MiniportReserved[0]) = (_Counter)

#define MP_NBL_RX_FRAME_BD(_NetBufferList)                         ((PMP_RX_FRAME_BD)((_NetBufferList)->MiniportReserved[0]))
#define MP_NBL_SET_RX_FRAME_BD(_NetBufferList, _BufferDescriptor)  (_NetBufferList)->MiniportReserved[0] = (PVOID)(_BufferDescriptor)

#define MP_NBL_ID(_NetBufferList)                                  (*(LONG *)&((_NetBufferList)->MiniportReserved[1]))
#define MP_NBL_SET_ID(_NetBufferList, _ID)                         *(LONG *)&((_NetBufferList)->MiniportReserved[1]) = (_ID)


MINIPORT_SEND_NET_BUFFER_LISTS      MpSendNetBufferLists;
MINIPORT_CANCEL_SEND                MpCancelSendNetBufferLists;
MINIPORT_CHECK_FOR_HANG             MpCheckForHangEx;
MINIPORT_RETURN_NET_BUFFER_LISTS    MpReturnNetBufferLists;

_IRQL_requires_(DISPATCH_LEVEL)
BOOLEAN MpTxCancelAll(_In_ PMP_ADAPTER pAdapter);
_IRQL_requires_max_(DISPATCH_LEVEL)
void MpQueueInit(PMP_QUEUE QueuePtr);
MINIPORT_PROCESS_SG_LIST MpProcessSGList;
_IRQL_requires_max_(DISPATCH_LEVEL)
void MpQueueAdd(_In_ PMP_QUEUE QueuePtr, PLIST_ENTRY ListEntryPtr);
_IRQL_requires_max_(DISPATCH_LEVEL)
LONG MpQueueGetDepth(PMP_QUEUE pQueue);
_IRQL_requires_max_(DISPATCH_LEVEL)
void MpHandleTxInterrupt(_In_ PMP_ADAPTER pAdapter, _In_ UINT32 InterruptEvent);
_IRQL_requires_max_(DISPATCH_LEVEL)
void MpHandleRecvInterrupt(_In_ PMP_ADAPTER pAdapter, _Inout_ PULONG pMaxNBLsToIndicate, _Inout_ PNDIS_RECEIVE_THROTTLE_PARAMETERS pRecvThrottleParameters);
void MpTxInit(_In_ PMP_ADAPTER pAdapter);
void MpRxInit(_In_ PMP_ADAPTER pAdapter);
BOOLEAN IsRxFramePandingInNdis(PMP_ADAPTER pAdapter);

#endif // _MP_DATA_PATH_H

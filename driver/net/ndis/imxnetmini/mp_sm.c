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
    Updates following sw Enet device sw variable according to the values got from PHY:
     pAdapter->LinkSpeed   [10 Mbs | 100 Mbs | 1 Gbs]
     pAdapter->DuplexMode  [MediaDuplexStateHalf | MediaDuplexStateFull | MediaDuplexStateUnknown]
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
void MpUpdateLinkStatus(_In_ PMP_ADAPTER pAdapter)
{
//    DBG_METHOD_BEGIN_CFG();
    MP_PHY_DEVICE  *pPHYDev = &pAdapter->ENETDev_PHYDevice;

    NdisDprAcquireSpinLock(&pAdapter->Dev_SpinLock);
    if (pAdapter->MediaConnectState != pPHYDev->PHYDev_MediaConnectState) {
        // Media state changed
        pAdapter->MediaConnectState = pPHYDev->PHYDev_MediaConnectState;
        switch (pAdapter->MediaConnectState) {
            case MediaConnectStateConnected:
                pAdapter->LinkSpeed  = pPHYDev->PHYDev_LinkSpeed * SPEED_FACTOR;
                pAdapter->DuplexMode = pPHYDev->PHYDev_DuplexMode;
                if (pPHYDev->PHYDev_DuplexMode == MediaDuplexStateHalf) {
                    DBG_SM_PRINT_INFO("ENET.LinkState: HALF-DUP, %d Mbps", pPHYDev->PHYDev_LinkSpeed);
                } else if (pPHYDev->PHYDev_DuplexMode == MediaDuplexStateFull) {
                    DBG_SM_PRINT_INFO("ENET.LinkState: FULL-DUP, %d Mbps", pPHYDev->PHYDev_LinkSpeed);
                } else {
                    DBG_SM_PRINT_INFO("ENET.LinkState: UNKWOWN-DUP, %d Mbps", pPHYDev->PHYDev_LinkSpeed);
                }
                break;
            case MediaConnectStateDisconnected:
                pAdapter->DuplexMode = MediaDuplexStateUnknown;
                pAdapter->LinkSpeed  = NDIS_LINK_SPEED_UNKNOWN;
                DBG_SM_PRINT_INFO("ENET.LinkState: DISCONNECTED");
                break;
            default:
                pAdapter->DuplexMode = MediaDuplexStateUnknown;
                pAdapter->LinkSpeed  = NDIS_LINK_SPEED_UNKNOWN;
                DBG_SM_PRINT_INFO("ENET.LinkState: UNKNOWN");
                break;
        }
    }
    NdisDprReleaseSpinLock(&pAdapter->Dev_SpinLock);
//    DBG_METHOD_END_CFG();
    return;
}

/*++
Routine Description:
    This routine sends a NDIS_STATUS_LINK_STATE status up to NDIS
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
void MpIndicateLinkStatus(_In_ PMP_ADAPTER pAdapter) {
    NDIS_LINK_STATE                LinkState;
    NDIS_STATUS_INDICATION         StatusIndication;

    NdisZeroMemory(&LinkState, sizeof(NDIS_LINK_STATE));
    LinkState.Header.Revision         = NDIS_LINK_STATE_REVISION_1;
    LinkState.Header.Type             = NDIS_OBJECT_TYPE_DEFAULT;
    LinkState.Header.Size             = NDIS_SIZEOF_LINK_STATE_REVISION_1;
    LinkState.MediaConnectState       = pAdapter->MediaConnectState;
    LinkState.MediaDuplexState        = pAdapter->DuplexMode;
    LinkState.XmitLinkSpeed           = pAdapter->LinkSpeed;
    LinkState.RcvLinkSpeed            = pAdapter->LinkSpeed;
    NdisZeroMemory(&StatusIndication, sizeof(NDIS_STATUS_INDICATION));
    StatusIndication.Header.Type      = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    StatusIndication.Header.Revision  = NDIS_STATUS_INDICATION_REVISION_1;
    StatusIndication.Header.Size      = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    StatusIndication.SourceHandle     = pAdapter->AdapterHandle;
    StatusIndication.StatusCode       = NDIS_STATUS_LINK_STATE;
    StatusIndication.StatusBuffer     = (PVOID)&LinkState;
    StatusIndication.StatusBufferSize = sizeof(LinkState);
    DBG_SM_PRINT_TRACE("Media connect state: %s, Duplex mode %s: , Link speed: %d", Dbg_GetMpMediaConnectStateName(LinkState.MediaConnectState), Dbg_GetMpMediaDuplexStateName(LinkState.MediaDuplexState), (int)LinkState.XmitLinkSpeed/1000000);
    NdisMIndicateStatusEx(pAdapter->AdapterHandle, &StatusIndication);  // Report Link status
}

/*++
Routine Description:
    State machine SM_STATE_RUNNING handler.
    Called at DISPATCH_LEVEL with SM_SpinLock held.
Arguments:
    pAdapter        Pointer to adapter data
    PreviousState   Previous state machine state
Return Value:
    None
--*/
void MpOnRunningStateHandler(_In_ PMP_ADAPTER pAdapter, _In_ MP_STATE PreviousState) {
    NDIS_MEDIA_CONNECT_STATE  PreviousMediaConnectState = pAdapter->MediaConnectState;        // Remember previous media state.
    LONG NextDelayMsec = MP_SM_NEXT_STATE_NORMAL_DEALY_MSEC;

    DBG_SM_METHOD_BEG();
    ASSERT(NDIS_CURRENT_IRQL() == DISPATCH_LEVEL);
    MpUpdateLinkStatus(pAdapter);                                                             // Get the current link status.
    switch (PreviousState) {
        case SM_STATE_PAUSED:                                                                 // PAUSED -> RUNNING (We have no "RESTARTING" state)
            if (pAdapter->MediaConnectState == MediaConnectStateConnected) {                  // Cable connected?
                DBG_SM_PRINT_TRACE("PAUSED -> RUNNING done. Media connected, starting ENET");
                EnetStart(pAdapter);                                                          // Yes, start Enet Rx and Tx operations.
                if (PreviousMediaConnectState != pAdapter->MediaConnectState) {               // Link state changed?
                    MpIndicateLinkStatus(pAdapter);                                           // Indicate the link status change to NDIS.
                }
            } else {
                DBG_SM_PRINT_TRACE("PAUSED -> RUNNING done. Media not connected");
            }
            break;
        case SM_STATE_RUNNING:                                                                // RUNNING -> RUNNING
            if (PreviousMediaConnectState != pAdapter->MediaConnectState) {                   // Link state changed?
                if (pAdapter->MediaConnectState == MediaConnectStateConnected) {              // Yes, Cable connected?
                    DBG_SM_PRINT_TRACE("RUNNING: Media connected, starting ENET");
                    EnetStart(pAdapter);                                                      // Yes, Start Data reception.
                    MpIndicateLinkStatus(pAdapter);                                           // Indicate the link status change to NDIS.
                } else {                                                                      // Cable disconnected.
                    DBG_SM_PRINT_TRACE("RUNNING: Media not connected, stopping ENET");
                    EnetStop(pAdapter, NDIS_STATUS_MEDIA_DISCONNECTED);                       // Stop data transfer.
                    (void)MpTxCancelAll(pAdapter);                                            // Cancel all pending TX requests.
                    MpIndicateLinkStatus(pAdapter);                                           // Indicate the link status change to NDIS.
                }
            }
            break;
        default:
            DBG_ENET_DEV_PRINT_ERROR("RUNNING: Unexpected previous state: %s", Dbg_GetMpStateMachineStateName(PreviousState));
            ASSERT(0);
            break;
    }
    if (pAdapter->MediaConnectState != MediaConnectStateConnected) {
        NextDelayMsec = MP_SM_CABLE_CONNECTED_CHECK_PERIOD_MSEC;
    }
    (void)SmSetState(pAdapter, SM_STATE_RUNNING, NextDelayMsec, SM_CALLED_BY_DISPATCHER);  // Set next state
    DBG_SM_METHOD_END();
}

/*++
Routine Description:
    State machine SM_STATE_PAUSING handler.
    Called at DISPATCH_LEVEL with SM_SpinLock held.
Arguments:
    pAdapter        Pointer to adapter data
    PreviousState   Previous state machine state
Return Value:
    None
--*/
void MpOnPausingStateHandler(_In_ PMP_ADAPTER pAdapter, _In_ MP_STATE PreviousState) {
    LONG NextState     = SM_STATE_PAUSING;
    LONG NextDelayMsec = MP_SM_NEXT_STATE_SAMPLE_DEALY_MSEC;

    DBG_SM_METHOD_BEG();
    switch (PreviousState) {
        case SM_STATE_RUNNING:                                                              // RUNNING -> PAUSING
            DBG_SM_PRINT_TRACE("RUNNING -> PAUSING done, stopping ENET");
            EnetStop(pAdapter, NDIS_STATUS_PAUSED);                                         // Stop the adapter and remember new NDIS status
            (void)MpTxCancelAll(pAdapter);                                                  // Cancel all queued send requests
            break;
        case SM_STATE_PAUSING:                                                              // PAUSING -> PAUSING
            if (MpTxCancelAll(pAdapter)) {                                                  // Check if all queued send requests have been canceled
                DBG_SM_PRINT_TRACE("PAUSING -> PAUSING, canceling Tx again");
                break;                                                                      // Keep waiting...
            }
            ASSERT(pAdapter->Tx_PendingNBs == 0);
            if (IsRxFramePandingInNdis(pAdapter)) {                                         // Wait until NDIS returns all indicated NBLs
                DBG_SM_PRINT_TRACE("PAUSING -> PAUSING, waiting for NDIS to return Rx");
                break;                                                                      // Keep waiting...
            }
            DBG_SM_PRINT_TRACE("PAUSING -> PAUSING, all buffer handled, switching to PAUSED");
            NextState = SM_STATE_PAUSED;                                                    // Switch immediately to the PAUSED state
            NextDelayMsec = MP_SM_NEXT_STATE_IMMEDIATELY;
            break;
        case SM_STATE_PAUSED:                                                               // PAUSED -> PAUSING (when OID_PNP_SET_POWER)
            DBG_SM_PRINT_TRACE("PAUSED -> PAUSING done, stopping ENET");
            EnetStop(pAdapter, NDIS_STATUS_PAUSED);                                         // Stop the adapter and remember new NDIS status
            (void)MpTxCancelAll(pAdapter);                                                  // Cancel all queued send requests
            break;
        case SM_STATE_RESET:                                                                // RESET -> PAUSING
            DBG_SM_PRINT_TRACE("RESET -> PAUSING, stopping ENET");
            EnetStop(pAdapter, NDIS_STATUS_REQUEST_ABORTED);                                // Stop ENET Rx and Tx
            (void)MpTxCancelAll(pAdapter);                                                  // Cancel all queued Tx Ethernet frames
            break;
        default:
            DBG_ENET_DEV_PRINT_ERROR("PAUSING: Unexpected previous state: %s", Dbg_GetMpStateMachineStateName(PreviousState));
            ASSERT(0);
            break;
    }
    (void)SmSetState(pAdapter, NextState, NextDelayMsec, SM_CALLED_BY_DISPATCHER);          // Set next state
    DBG_SM_METHOD_END();
    return;
}

/*++
Routine Description:
    State machine SM_STATE_PAUSED handler.
    Called at DISPATCH_LEVEL with SM_SpinLock held.
Arguments:
    pAdapter        Pointer to adapter data
    PreviousState   Previous state machine state
Return Value:
    None
--*/
void MpOnPausedStateHandler(_In_ PMP_ADAPTER pAdapter, _In_ MP_STATE PreviousState) {

    DBG_SM_METHOD_BEG();
    switch (PreviousState) {
        case SM_STATE_PAUSING:                                    // PAUSING -> PAUSED
            NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
            if (pAdapter->PendingNdisOperations.B.OP_RESET) {
                DBG_SM_PRINT_TRACE("PAUSING(RSTART) -> PAUSED, calling NdisMResetComplete()");
                pAdapter->PendingNdisOperations.B.OP_RESET = 0;
                NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
                if (pAdapter->StateMachine.SM_OnResetPreviousState == SM_STATE_RUNNING) { // Previous state was RUNNING?
                    EnetStart(pAdapter);                                     // Yes, start Enet Rx and Tx operations.
                }
                NdisMResetComplete(pAdapter->AdapterHandle, NDIS_STATUS_SUCCESS, TRUE);
                (void)SmSetState(pAdapter, pAdapter->StateMachine.SM_OnResetPreviousState, MP_SM_NEXT_STATE_IMMEDIATELY, SM_CALLED_BY_DISPATCHER);   // Set next state
                break;
            }
            DBG_SM_PRINT_TRACE("PAUSING -> PAUSED, calling NdisMPauseComplete()");
            NdisMPauseComplete(pAdapter->AdapterHandle);          // Complete pause request.
            NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
            break;
        default:
            DBG_ENET_DEV_PRINT_ERROR("PAUSING: Unexpected previous state: %s", Dbg_GetMpStateMachineStateName(PreviousState));
            ASSERT(0);
            break;
    }
    DBG_SM_METHOD_END();
}

/*++
Routine Description:
    State machine SM_STATE_RESET handler.
    Called at DISPATCH_LEVEL with SM_SpinLock held.
Arguments:
    pAdapter        Pointer to adapter data
    PreviousState   Previous state machine state
Return Value:
    None
--*/
void MpOnResetStateHandler(_In_ PMP_ADAPTER pAdapter, _In_ MP_STATE PreviousState) {

    DBG_SM_METHOD_BEG();
    pAdapter->StateMachine.SM_OnResetPreviousState = PreviousState;                                        // Remember previous state
    (void)SmSetState(pAdapter, SM_STATE_PAUSING, MP_SM_NEXT_STATE_IMMEDIATELY, SM_CALLED_BY_DISPATCHER);   // Set next state
    DBG_SM_METHOD_END();
}

/*++
Routine Description:
    State machine SM_STATE_HALTED handler.
    Called at DISPATCH_LEVEL with SM_SpinLock held.
Arguments:
    pAdapter        Pointer to adapter data
    PreviousState   Previous state machine state
Return Value:
    None
--*/
void MpOnHaltedStateHandler(_In_ PMP_ADAPTER pAdapter, _In_ MP_STATE PreviousState, _Out_ BOOLEAN *EnetStopped) {

    UNREFERENCED_PARAMETER(pAdapter);
    DBG_SM_METHOD_BEG();
    switch (PreviousState) {
        case SM_STATE_PAUSED:                                   // PAUSED -> HALTED
            EnetStop(pAdapter, NDIS_STATUS_REQUEST_ABORTED);    //
            *EnetStopped = TRUE;
            break;
        default:
            DBG_ENET_DEV_PRINT_ERROR("HALTED: Unexpected previous state: %s", Dbg_GetMpStateMachineStateName(PreviousState));
            ASSERT(0);
            *EnetStopped = FALSE;
            break;
    }
    DBG_SM_METHOD_END();
}

/*++
Routine Description:
    Timer function for State machine service.
    Called at DISPATCH_LEVEL as soon as accociated NDIS timer expires.
Arguments:
    SystemSpecific1     Not used
    FunctionContext     Pointer to our adapter
    SystemSpecific2     Not used
    SystemSpecific3     Not used
Return Value:
    None
--*/
_Use_decl_annotations_
void MpSmDispatcher(PVOID SystemSpecific1, PVOID FunctionContext, PVOID SystemSpecific2, PVOID SystemSpecific3)
{
    PMP_ADAPTER       pAdapter = (PMP_ADAPTER)FunctionContext;
    PMP_STATE_MACHINE pSM      = &pAdapter->StateMachine;
    MP_STATE          NextState, PreviousState;
    BOOLEAN           EnetStopped = FALSE;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);
    do {
        NdisDprAcquireSpinLock(&pSM->SM_SpinLock);
        if (pSM->SM_NextState != SM_STATE_HALTED) {
            PHYDev_UpdateLinkStatus(pAdapter);                     // Start new read of PHY registers
        }
        PreviousState = pSM->SM_CurrentState;               // Remember current state
        NextState     = pSM->SM_NextState;                  // Remember new state
        pSM->SM_PreviousState = PreviousState;
        pSM->SM_CurrentState  = NextState;
        if (NextState == -1) {
            DBG_ENET_DEV_PRINT_ERROR("No state (-1)");
            break;
        }
        switch (NextState) {
            case SM_STATE_RUNNING:
                MpOnRunningStateHandler(pAdapter, PreviousState);
                break;
            case SM_STATE_HALTED:
                MpOnHaltedStateHandler(pAdapter, PreviousState, &EnetStopped);
                break;
            case SM_STATE_SHUTDOWN:
                DBG_SM_PRINT_TRACE("New state: SM_STATE_SHUTDOWN - no handler for this state");
                break;
            case SM_STATE_PAUSING:
                MpOnPausingStateHandler(pAdapter, PreviousState);
                break;
            case SM_STATE_PAUSED:
                MpOnPausedStateHandler(pAdapter, PreviousState);
                break;
            case SM_STATE_RESET:
                MpOnResetStateHandler(pAdapter, PreviousState);
                break;
            case SM_STATE_ERROR:
                DBG_SM_PRINT_TRACE("New state: SM_STATE_ERROR - no handler for this state");
                break;
            default:
                DBG_ENET_DEV_PRINT_ERROR("New state: UNKNOWN");
                break;
        }
    } while (0);
    NdisDprReleaseSpinLock(&pSM->SM_SpinLock);
    if (EnetStopped) {
        DBG_SM_PRINT_TRACE("Setting SM_EnetHaltEvent");
        NdisSetEvent(&pSM->SM_EnetHaltedEvent); // Mark Enet device as halted
    }
}

/*++
Routine Description:
    .
Arguments:
    pAdapter    Pointer to adapter data
    State       Required state machine state
    DelayMsec   Required delay time in milliseconds
    Caller      Next state caller mode
Return Value:
    NDIS_STATUS_SUCCESS or NDIS_STATUS_INVALID_PARAMETER or NDIS_STATUS_INVALID_STATE
--*/
_Use_decl_annotations_
NDIS_STATUS SmSetState(PMP_ADAPTER pAdapter, LONG State, LONG DelayMsec, MP_SM_NEXT_STATE_CALLER_MODE Caller) {
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;

    if (Caller == SM_CALLED_BY_NDIS) {
        NdisAcquireSpinLock(&pAdapter->StateMachine.SM_SpinLock);
    }
    do {
        if (pAdapter->StateMachine.SM_hTimer == NULL) {
            DBG_ENET_DEV_PRINT_ERROR("State machine not ready");
            status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }
        // Make sure that DISPATCHER state handlers do not override preceding NDIS commands.
        if (Caller == SM_CALLED_BY_DISPATCHER) {
            if (pAdapter->StateMachine.SM_NextState != pAdapter->StateMachine.SM_CurrentState) {
                DBG_SM_PRINT_WARNING("State change request - denied");
                status = NDIS_STATUS_INVALID_STATE;
                break;
            }
        }
 //       ASSERT(pAdapter->StateMachine.SM_CurrentState == SM_STATE_SHUTDOWN);
        pAdapter->StateMachine.SM_NextState = State;   // Remember dispatcher next state
        LARGE_INTEGER delayPeriod100nSec;              // Get the relative period in 100 nSec units
        switch (DelayMsec) {
            case MP_SM_NEXT_STATE_IMMEDIATELY:         // Switch to the next state without delay
                delayPeriod100nSec.QuadPart = -1;
                #ifdef DBG
                if (pAdapter->StateMachine.SM_CurrentState != State) {
                    DBG_SM_PRINT_TRACE(" %s -> %s requested immediately (after 1 ns)", Dbg_GetMpStateMachineStateName(pAdapter->StateMachine.SM_CurrentState), Dbg_GetMpStateMachineStateName(State));
                }
                #endif
                break;
            default:
                delayPeriod100nSec.QuadPart = -DelayMsec * (10 * 1000);
                #ifdef DBG
                if (pAdapter->StateMachine.SM_CurrentState != State) {
                    DBG_SM_PRINT_TRACE(" %s -> %s requested after %d ms", Dbg_GetMpStateMachineStateName(pAdapter->StateMachine.SM_CurrentState), Dbg_GetMpStateMachineStateName(State), DelayMsec);
                }
                #endif
                break;
        }
        // If the timer object was already in the timer queue, it is implicitly canceled before being set to the new expiration time.
        (void)NdisSetTimerObject(pAdapter->StateMachine.SM_hTimer, delayPeriod100nSec, 0, pAdapter);
    } while (0);
    if (Caller == SM_CALLED_BY_NDIS) {
        NdisReleaseSpinLock(&pAdapter->StateMachine.SM_SpinLock);
    }
    return status;
}

/*++
Routine Description:
    It checks if the adapter is running.
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    TRUE, the adapter is running, otherwise FALSE.
--*/
_Use_decl_annotations_
BOOLEAN SmIsAdapterRunning(PMP_ADAPTER pAdapter)
{
    BOOLEAN isRunning;
    NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
    isRunning = pAdapter->NdisStatus == NDIS_STATUS_SUCCESS;
    NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
    return isRunning;
}

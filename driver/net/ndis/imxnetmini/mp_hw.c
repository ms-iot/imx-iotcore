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
    This function calculates the 6-bit Hash value for multicasting.
Arguments:
    pAddr: pointer to a Ethernet address
Return Value:
    Returns the calculated 6-bit Hash value.
--*/
UCHAR CalculateHashValue(_In_ UCHAR *pAddr)
{
    ULONG CRC;
    UCHAR HashValue = 0;
    UCHAR AddrByte;
    int byte, bit;

//    DBG_METHOD_BEGIN_NIC_CFG();
    CRC = CRC_PRIME;
    for (byte=0; byte < ETHER_ADDR_LENGTH; byte++) {
        AddrByte = *pAddr++;
        for(bit = 0; bit < 8; bit++, AddrByte >>= 1) {
            CRC = (CRC >> 1) ^ (((CRC ^ AddrByte) & 1) ? CRC_POLYNOMIAL : 0);
        }
    }
    // Only upper 6 bits (HASH_BITS) are used which point to specific bit in the hash registers
    HashValue = (UCHAR)((CRC >> (32 - HASH_BITS)) & 0x3f);
//    DBG_METHOD_END_WITH_PARAMS_NIC_CFG("HashValue: [0x%.2X]", HashValue);
    return HashValue;
}

/*++
Routine Description:
    This function adds the Hash value to the Hash table (Descriptor Individual Address Register).
Arguments:
    pAdapter     Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void SetUnicast(PMP_ADAPTER pAdapter)
{
    UCHAR HashValue = 0;

    DBG_ENET_DEV_OIDS_PRINT_TRACE("UCast        = %02x-%02x-%02x-%02x-%02x-%02x", pAdapter->FecMacAddress[0], pAdapter->FecMacAddress[1], pAdapter->FecMacAddress[2], pAdapter->FecMacAddress[3], pAdapter->FecMacAddress[4], pAdapter->FecMacAddress[5]);
    pAdapter->ENETRegBase->IAUR = 0;
    pAdapter->ENETRegBase->IALR = 0;
    HashValue = CalculateHashValue(pAdapter->FecMacAddress);
    if (HashValue > 31) {
        pAdapter->ENETRegBase->IAUR = 1 << (HashValue - 32);
    } else {
        pAdapter->ENETRegBase->IALR = 1 << HashValue;
    }
}

/*++
Routine Description:
    This function adds the Hash value to the Hash table (Descriptor Group Address Register).
Arguments:
    pAdapter    Pointer to adapter data
    pAddr       Pointer to multicast list
Return Value:
    None
--*/
_Use_decl_annotations_
void AddMultiCast(PMP_ADAPTER pAdapter, UCHAR *pAddr)
{
    UCHAR HashValue = 0;
    HashValue = CalculateHashValue( pAddr );
    if (HashValue > 31) {
        pAdapter->ENETRegBase->GAUR = 1 << (HashValue - 32);
    } else {
        pAdapter->ENETRegBase->GALR = 1 << HashValue;
    }
}

/*++
Routine Description:
    This function clears the Hash table (Descriptor Group Address Register).
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void ClearAllMultiCast(PMP_ADAPTER pAdapter)
{
    DBG_ENET_DEV_OIDS_PRINT_TRACE("Clearing all multicasts addresses");
    pAdapter->ENETRegBase->GAUR = 0;    // clear all GAUR and GALR bits
    pAdapter->ENETRegBase->GALR = 0;
}

/*++
Routine Description:
    MiniportHandleInterrupt handler
Arguments:
    MiniportInterruptContext
        Pointer to the interrupt context. In this is a pointer to the adapter structure.
    MiniportDpcContext
        Not used
    ReceiveThrottleParameters
        A pointer to an NDIS_RECEIVE_THROTTLE_PARAMETERS structure. This structure specifies the maximum number of
        NET_BUFFER_LIST structures that a miniport driver should indicate in a DPC.
    NdisReserved2
        Not used
Return Value:
    None
--*/
_Use_decl_annotations_
VOID EnetDpc(NDIS_HANDLE  MiniportInterruptContext, PVOID MiniportDpcContext, PVOID ReceiveThrottleParameters, PVOID NdisReserved2)
{
    PMP_ADAPTER                        pAdapter = (PMP_ADAPTER)MiniportInterruptContext;
    UINT32                             InterruptEvent, InterruptFlags;
    PNDIS_RECEIVE_THROTTLE_PARAMETERS  pRecvThrottleParameters = (PNDIS_RECEIVE_THROTTLE_PARAMETERS)ReceiveThrottleParameters;
    ULONG                              MaxNBLsToIndicate;

    UNREFERENCED_PARAMETER(MiniportDpcContext);
    UNREFERENCED_PARAMETER(NdisReserved2);

    DBG_ENET_DEV_DPC_METHOD_BEG();
    MaxNBLsToIndicate = pRecvThrottleParameters->MaxNblsToIndicate;
    if (MaxNBLsToIndicate == NDIS_INDICATE_ALL_NBLS) {
        MaxNBLsToIndicate = MAXULONG;
    }
    pRecvThrottleParameters->MoreNblsPending = FALSE;
    do {
        NdisDprAcquireSpinLock(&pAdapter->Dev_SpinLock);
        pAdapter->DpcQueued          = FALSE;
        pAdapter->DpcRunning         = TRUE;
        InterruptFlags               = pAdapter->ENETRegBase->EIR.U & ENET_RX_TX_INT_MASK;  // Get current interrupt flags from HW register
        pAdapter->ENETRegBase->EIR.U = InterruptFlags;                                      // Clear current interrupt flags in HW register
        InterruptFlags              |= pAdapter->InterruptFlags;                            // Add saved interrupt flags
        pAdapter->InterruptFlags     = 0;                                                   // Clear saved interrupt flags
        NdisDprReleaseSpinLock(&pAdapter->Dev_SpinLock);
        InterruptEvent               = InterruptFlags;  // Compute new interrupt flags
        if (!InterruptEvent)                                            // An interrupt ready to be served?
            break;                                                      // No, finish DPC
        if (InterruptEvent & ENET_EIR_EBERR_MASK) {                     // Handle Bus error interrupt
            DBG_ENET_DEV_DPC_PRINT_ERROR("Bus error, reset required.");
            return;
        }
        if (InterruptEvent & ENET_TX_INT_MASK) {                        // Handle frame(s) sent or sent error interrupt
            MpHandleTxInterrupt(pAdapter, InterruptEvent);
        }
        if (InterruptEvent & ENET_RX_INT_MASK) {                        // Handle frame(s) received or receive error interrupt
            MpHandleRecvInterrupt(pAdapter, &MaxNBLsToIndicate, pRecvThrottleParameters);
            if (pRecvThrottleParameters->MoreNblsPending) {
                NdisDprAcquireSpinLock(&pAdapter->Dev_SpinLock);
                pAdapter->InterruptFlags |= (InterruptEvent & ENET_RX_INT_MASK); // We have to prepare interrupt flags for NDIS called EnetDPC
                NdisDprReleaseSpinLock(&pAdapter->Dev_SpinLock);
                break;
            }
        }
        if (InterruptEvent & ENET_EIR_GRA_MASK) {                       // Restart Tx path after pause frame transmit
            pAdapter->ENETRegBase->TDAR = 0x0000000;
        }
    } while (0);
    if (!pRecvThrottleParameters->MoreNblsPending) {
      NdisMSynchronizeWithInterruptEx(pAdapter->NdisInterruptHandle, 0, EnetEnableRxAndTxInterrupts, pAdapter);
    }
    NdisDprAcquireSpinLock(&pAdapter->Dev_SpinLock);
    pAdapter->DpcRunning = FALSE;
    NdisDprReleaseSpinLock(&pAdapter->Dev_SpinLock);
    DBG_ENET_DEV_DPC_METHOD_END();
}

/*++
Routine Description:
    Miniport ISR callback function.
    Disables all ENET peripheral interrupts and queues DPC.
Arguments:
    MiniportInterruptContext  Pointer to the miniport adapter data structure.
    QueueDefaultInterruptDpc  Set to TRUE value to queue DPC on default(this) CPU.
    TargetProcessors          Pointer to a bitmap specifying Target processors which should run the DPC if QueueDefaultInterruptDpc = FALSE.
Return Value:
    TRUE   The miniport recognizes the interrupt as ENET peripheral interrupt.
    FALSE  Not ENET peripheral interrupt, other ISR will be called by OS if registered.
--*/
_Use_decl_annotations_
BOOLEAN EnetIsr(NDIS_HANDLE MiniportInterruptContext, PBOOLEAN QueueDefaultInterruptDpc, PULONG TargetProcessors)
{
    PMP_ADAPTER pAdapter = (PMP_ADAPTER)MiniportInterruptContext;

    UNREFERENCED_PARAMETER(TargetProcessors);
    DBG_ENET_DEV_ISR_METHOD_BEG();
    if (pAdapter->ENETRegBase->EIMR.U != 0U) {
        pAdapter->ENETRegBase->EIMR.U = 0x00;  // Disable all ENET interrupts. (EnetIsr will not be called again until interrupts are enabled in EnetDpc)
        pAdapter->DpcQueued = TRUE;            // Remember that EnetDpc is queued
        *QueueDefaultInterruptDpc = TRUE;      // Schedule EnetDpc on the current CPU to complete the operation
    } else {
        *QueueDefaultInterruptDpc = FALSE;     // Do not schedule Dpc
        *TargetProcessors = 0;
        DBG_ENET_DEV_ISR_PRINT_WARNING("Spurious Interrupt.");
    }
    DBG_ENET_DEV_ISR_METHOD_END();
    return *QueueDefaultInterruptDpc;
}

/*++
Routine Description:
    Enables TxF, RxF and MII interrupts
Arguments:
    SynchronizeContext  The handle to the driver allocated context area.
    Return Value:
        Always returns TRUE
--*/
_Use_decl_annotations_
BOOLEAN EnetEnableRxAndTxInterrupts(NDIS_HANDLE SynchronizeContext) {
    PMP_ADAPTER  pAdapter = (PMP_ADAPTER)SynchronizeContext;
    volatile CSP_ENET_REGS  *ENETRegBase = pAdapter->ENETRegBase;

    ENETRegBase->EIMR.U |= ENET_RX_TX_INT_MASK;   // Enable Rx and Tx interrupts
    return TRUE;
}

/*++
Routine Description:
    Disables all ENET peripheral interrupt and returns TRUE if Dpc is queued or running.
    It is supposed that this function is called synchronised with EnetIsr by NdisMSynchronizeWithInterruptEx()
Arguments:
    SynchronizeContext  The handle to the driver allocated context area.
Return Value:
    TRUE   Dpc is queued or running.
    FALSE  Dpc is neither queued nor running.
--*/
_Use_decl_annotations_
BOOLEAN EnetDisableRxAndTxInterrupts(NDIS_HANDLE SynchronizeContext) {
    PMP_ADAPTER  pAdapter = (PMP_ADAPTER)SynchronizeContext;
    volatile CSP_ENET_REGS  *ENETRegBase = pAdapter->ENETRegBase;

    ENETRegBase->EIMR.U &= ~((ENET_RX_TX_INT_MASK));         // Disable Rx and Tx interrupts
    ENETRegBase->EIR.U = (ENET_RX_TX_INT_MASK);              // Clear Rx and Tx interrupts flags
    return pAdapter->DpcQueued;
}

/*++
Routine Description:
    This function stops the ENET hardware.
    Called at DISPATCH_LEVEL with SM_SpinLock held from state machine handler(s).
Arguments:
    pAdapter    Pointer to adapter data
    NdisStatus  Actual NDIS status
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetStop(PMP_ADAPTER pAdapter, NDIS_STATUS NdisStatus)
{
    volatile CSP_ENET_REGS  *ENETRegBase = pAdapter->ENETRegBase;

    NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
    NdisAcquireSpinLock(&pAdapter->Rx_SpinLock);
    NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
    DBG_SM_PRINT_TRACE("Stopping ENET, all spinlocks acquired");
    NdisMSynchronizeWithInterruptEx(pAdapter->NdisInterruptHandle, 0, EnetDisableRxAndTxInterrupts, pAdapter);
    ENETRegBase->ECR.U &= ~ENET_ECR_ETHER_EN_MASK;        // Disable Enet MAC (Clear "Enable" bit)
    while (ENETRegBase->ECR.U & ENET_ECR_ETHER_EN_MASK);  // Wait until Enet MAC is disabled
    pAdapter->NdisStatus = NdisStatus;                    // Remember new NDIS status
    pAdapter->EnetStarted = FALSE;                        // Remember new Enet state
    DBG_SM_PRINT_TRACE("ENET stopped, status: %s, releasing all spinlocks", Dbg_GetNdisStatusName(NdisStatus));
    NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
    NdisReleaseSpinLock(&pAdapter->Rx_SpinLock);
    NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
}

/*++
Routine Description:
    This function starts the frame reception.
Arguments:
    pAdapter    Pointer to adapter data
    NdisStatus  Actual NDIS status
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetStart(PMP_ADAPTER pAdapter)
{
    volatile CSP_ENET_REGS  *ENETRegBase = pAdapter->ENETRegBase;

    NdisAcquireSpinLock(&pAdapter->Dev_SpinLock);
    NdisAcquireSpinLock(&pAdapter->Rx_SpinLock);
    NdisAcquireSpinLock(&pAdapter->Tx_SpinLock);
    DBG_SM_PRINT_TRACE("Starting ENET, all spinlocks acquired");
    MpTxInit(pAdapter);                                               // Initialize Tx data structures
    MpRxInit(pAdapter);                                               // Initialize Rx data structures
    pAdapter->EnetStarted = TRUE;                                     // Remember new Enet state
    pAdapter->NdisStatus = NDIS_STATUS_SUCCESS;                       // Remember new NDIS status
    pAdapter->InterruptFlags = 0;                                     // No interrupt flags pending from previous call of DPC
    ENETRegBase->ERDSR = (ULONG)pAdapter->Rx_DmaBDT_Pa.QuadPart;      // Set the Rx_DmaBDT physical address
    ENETRegBase->ETDSR = (ULONG)pAdapter->Tx_DmaBDT_Pa.QuadPart;      // Set the Tx_DmaBDT physical address
    ENETRegBase->EMRBR = 0x7f0;                                       //
    ENETRegBase->EIR.U = (ENET_RX_TX_INT_MASK);                       // Clear Rx and Tx interrupts flags
    NdisMSynchronizeWithInterruptEx(pAdapter->NdisInterruptHandle, 0, EnetEnableRxAndTxInterrupts, pAdapter);
    _DataSynchronizationBarrier();                                    // Wait until mem-io accesses are finished 
    ENETRegBase->ECR.U |= ENET_ECR_ETHER_EN_MASK;                     // Start Enet (ENET must be running in order to invoke MII interrupt)
    ENETRegBase->RDAR = 0x00000000;                                   // Start data reception
    DBG_SM_PRINT_TRACE("ENET started, releasing all spinlocks");
    NdisReleaseSpinLock(&pAdapter->Tx_SpinLock);
    NdisReleaseSpinLock(&pAdapter->Rx_SpinLock);
    NdisReleaseSpinLock(&pAdapter->Dev_SpinLock);
}

/*++
Routine Description:
    This function disables the ENET hardware.
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetDeinit(PMP_ADAPTER pAdapter)
{
    volatile CSP_ENET_REGS  *ENETRegBase = pAdapter->ENETRegBase;
    DBG_SM_PRINT_TRACE("Disabling ENET");
    NdisMSynchronizeWithInterruptEx(pAdapter->NdisInterruptHandle, 0, EnetDisableRxAndTxInterrupts, pAdapter);
    ENETRegBase->ECR.U &= ~ENET_ECR_ETHER_EN_MASK;        // Disable Enet MAC (Clear "Enable" bit)
    while (ENETRegBase->ECR.U & ENET_ECR_ETHER_EN_MASK);  // Wait until Enet MAC is disabled
    DBG_SM_PRINT_TRACE("ENET reset done");
}

/*++
Routine Description:
    Initialize the adapter and set up everything
Arguments:
    pAdapter    Pointer to adapter data
Return Value:
    None
--*/
_Use_decl_annotations_
void EnetInit(PMP_ADAPTER pAdapter, MP_MDIO_PHY_INTERFACE_TYPE EnetPhyInterfaceType)
{
    volatile CSP_ENET_REGS* ENETRegBase = pAdapter->ENETRegBase;
    UINT32                  ECR_RegMask = ENET_ECR_DBSW_MASK;
    UINT32                  RCR_RegMask = ETHER_FRAME_MAX_LENGTH << ENET_RCR_MAX_FL_SHIFT | ENET_RCR_MII_MODE_MASK | ENET_RCR_FCE_MASK;  // Set maximum Ethernet frame length and enable MII mode and Flow control;
    UINT32                  TCR_RegMask = 0;

    DBG_SM_METHOD_BEG();
    pAdapter->MediaConnectState    = MediaConnectStateUnknown;
    pAdapter->CurrentPowerState    = NdisDeviceStateD0;
    pAdapter->NewPowerState        = NdisDeviceStateD0;

    ENETRegBase->ECR.U &= ~ENET_ECR_ETHER_EN_MASK;        // Disable Enet MAC (Clear "Enable" bit)
    while (ENETRegBase->ECR.U & ENET_ECR_ETHER_EN_MASK);  // Wait until Enet MAC is disabled

    if (EnetPhyInterfaceType == RGMII) {
        RCR_RegMask |= ENET_RCR_RGMII_EN_MASK;         // Set RGMII mode;
    } else if (EnetPhyInterfaceType == RMII) {
        RCR_RegMask |= ENET_RCR_RMII_MODE_MASK;        // Set RMII mode;
    } else {
    }

    NdisZeroMemory(&pAdapter->StatisticsAcc, sizeof(pAdapter->StatisticsAcc)); // Reset accumulated values of HW statistic counters
    if (pAdapter->SpeedSelect == SPEED_AUTO) {
        TCR_RegMask = ENET_TCR_FEDN_MASK;                                      // Enable full duplex
    } else {
        if ((pAdapter->SpeedSelect == SPEED_HALF_DUPLEX_100M) || (pAdapter->SpeedSelect == SPEED_HALF_DUPLEX_10M)) {
            RCR_RegMask |= ENET_RCR_DRT_MASK;                                  // Enable Half duplex
        } else {
            TCR_RegMask = ENET_TCR_FEDN_MASK;                                  // Enable Full duplex
        }
        if ((pAdapter->SpeedSelect == SPEED_HALF_DUPLEX_10M) || (pAdapter->SpeedSelect == SPEED_FULL_DUPLEX_10M)) {
            RCR_RegMask |= ENET_RCR_RMII_10T_MASK;                             // Select 10 MBs
        }
    }
    ENETRegBase->ECR.U = ECR_RegMask;
    ENETRegBase->RCR.U = RCR_RegMask;
    ENETRegBase->TCR.U = TCR_RegMask;
    ENETRegBase->MIBC.U = 0;                                                                         // Enable statistic counters
    ENETRegBase->RACC.U = ENET_RACC_SHIFT16_MASK;                                                    // Instructs the MAC to write two additional bytes in front of each frame received into the RX FIFO.
    ENETRegBase->PALR = pAdapter->FecMacAddress[3] | pAdapter->FecMacAddress[2] << 8 | pAdapter->FecMacAddress[1] << 16 | pAdapter->FecMacAddress[0] << 24;
    ENETRegBase->PAUR = pAdapter->FecMacAddress[5] << 16 | pAdapter->FecMacAddress[4] << 24;           // Set the station address for the ENET Adapter
                                                                                                       // Set FIFO thresholds and Pause frame duration
#if DBG
    ENETRegBase->RSFL = pAdapter->RSFL_Value;
    ENETRegBase->RSEM = pAdapter->RSEM_Value;
    ENETRegBase->RAEM = pAdapter->RAEM_Value;
    ENETRegBase->RAFL = pAdapter->RAFL_Value;
    ENETRegBase->OPD.U = pAdapter->OPD_Value;
    ENETRegBase->TFWR.U = pAdapter->TFWR_Value;
    ENETRegBase->TAEM = pAdapter->TAEM_Value;   // min 4
    ENETRegBase->TAFL = pAdapter->TAFL_Value;   // min 4
    ENETRegBase->TSEM = pAdapter->TSEM_Value;   // 8~480?
#else
    ENETRegBase->RSFL = ENET_MAC_RX_SECTION_FULL_DEFAULT_VALUE;
    ENETRegBase->RSEM = ENET_MAC_RX_SECTION_EMPTY_DEFAULT_VALUE;
    ENETRegBase->RAEM = ENET_MAC_RX_ALMOST_EMPTY_DEFAULT_VALUE;
    ENETRegBase->RAFL = ENET_MAC_RX_ALMOST_FULL_DEFAULT_VALUE;
    ENETRegBase->OPD.U = ENET_MAC_RX_OPD_DEFAULT_VALUE;
    ENETRegBase->TFWR.U = BF_ENET_MAC_TFW_TFWR_DEFAULT_VALUE;
    ENETRegBase->TAEM = ENET_MAC_TX_ALMOST_EMPTY_DEFAULT_VALUE;  // min 4
    ENETRegBase->TAFL = ENET_MAC_TX_ALMOST_FULL_DEFAULT_VALUE;   // min 4
    ENETRegBase->TSEM = ENET_MAC_TX_SECTION_EMPTY_DEFAULT_VALUE; // 8~480?
#endif
    SetUnicast(pAdapter);
    //Dbg_DumpFifoTrasholdsAndPauseFrameDuration(pAdapter);
    DBG_SM_METHOD_END();
}
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPIhw.cpp
//
// Abstract:
//
//    This module contains methods for accessing the IMX ECSPI
//    controller hardware.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//
#include "precomp.h"
#pragma hdrstop

#define _ECSPI_HW_CPP_

// Logging header files
#include "ECSPItrace.h"
#include "ECSPIhw.tmh"

// Common driver header files
#include "ECSPIcommon.h"

// Module specific header files
#include "ECSPIhw.h"
#include "ECSPIspb.h"
#include "ECSPIdriver.h"
#include "ECSPIdevice.h"


//
// Routine Description:
//
//  ECSPIHwInitController initializes the ARM ECSPI controller, and  
//  sets it to a known state.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the ECSPI this instance of
//      the ECSPI controller.
//
// Return Value:
//
//  Controller initialization status.
//
_Use_decl_annotations_
NTSTATUS
ECSPIHwInitController (
    ECSPI_DEVICE_EXTENSION* DevExtPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;

    //
    // Reset the block
    //
    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG, 0);

    //
    // Set default configuration
    //
    {
        ECSPI_CONREG ctrlReg = { 
            READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG) 
            };
        ECSPI_ASSERT(
            DevExtPtr->IfrLogHandle, 
            ctrlReg.AsUlong == 0
            );

        ctrlReg.CHANNEL_MODE = ECSPI_CHANNEL_MODE::ALL_MASTERS;
        WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG, ctrlReg.AsUlong);
    }

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPIHwSetTargetConfiguration calculates the controller HW configuration
//  according to the target required settings.
//  The routine save the control and configuration HW registers image on the 
//  target context so it can be applied when the target is referenced.
//
// Arguments:
//
//  TrgCtxPtr - The target context.
//
// Return Value:
//
//  STATUS_SUCCESS, or STATUS_NOT_SUPPORTED if the desired speed yields
//  an error that more than the allowed range.
//
_Use_decl_annotations_
NTSTATUS
ECSPIHwSetTargetConfiguration (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr
    )
{
    ECSPI_TARGET_SETTINGS* trgSettingsPtr = &TrgCtxPtr->Settings;
    ECSPI_CHANNEL spiChannel = static_cast<ECSPI_CHANNEL>(
        trgSettingsPtr->DeviceSelection
        );

    //
    // Control register
    //
    {
        ECSPI_CONREG* ctrlRegPtr = &trgSettingsPtr->CtrlReg;
        ctrlRegPtr->AsUlong = 0;

        ctrlRegPtr->CHANNEL_SELECT = trgSettingsPtr->DeviceSelection;
        ctrlRegPtr->CHANNEL_MODE = ECSPI_CHANNEL_MODE::ALL_MASTERS;
        ctrlRegPtr->DRCTL = 0; // We do not use SPI_RDY.
        ctrlRegPtr->SMC = ECSPI_START_MODE::XCH; // Use Exchange Bit (XCH)
                                                 // to start the transfer.

        //
        // Calculate the SPI clock settings.
        //
        NTSTATUS status = ECSPIpHwCalcFreqDivider(
            TrgCtxPtr->DevExtPtr,
            trgSettingsPtr->ConnectionSpeed, 
            ctrlRegPtr
            );
        if (!NT_SUCCESS(status)) {

            return status;
        }

    } // Control register

    //
    // Configuration register
    //
    {
        ECSPI_CONFIGREG* configRegPtr = &trgSettingsPtr->ConfigReg;
        configRegPtr->AsUlong = 0;

        //
        // SCLK polarity
        //
        if (trgSettingsPtr->Polarity) {
            
            configRegPtr->SCLK_POL = 
                ECSPI_CH_ATTR(spiChannel, ECSPI_SCLK_POL::SCLK_POL1);
            configRegPtr->SCLK_CTL =
                ECSPI_CH_ATTR(spiChannel, ECSPI_INACTIVE_SCLK_CTL::SCLK_HIGH);

        } else {

            configRegPtr->SCLK_POL =
                ECSPI_CH_ATTR(spiChannel, ECSPI_SCLK_POL::SCLK_POL0);
            configRegPtr->SCLK_CTL =
                ECSPI_CH_ATTR(spiChannel, ECSPI_INACTIVE_SCLK_CTL::SCLK_LOW);
        }

        //
        // SCLK phase
        //
        if (trgSettingsPtr->Phase) {

            configRegPtr->SCLK_PHA = 
                ECSPI_CH_ATTR(spiChannel, ECSPI_SCLK_PHA::SCLK_PHASE1);

        }  else {

            configRegPtr->SCLK_PHA =
                ECSPI_CH_ATTR(spiChannel, ECSPI_SCLK_PHA::SCLK_PHASE0);
        }

        //
        // Device polarity, configure the CS (SS) polarity.
        //
        if (trgSettingsPtr->CsActiveValue == 1) {

            configRegPtr->SS_POL = 
                ECSPI_CH_ATTR(spiChannel, ECSPI_SS_POL::SS_ACTIVE_HIGH);

        } else {

            configRegPtr->SS_POL =
                ECSPI_CH_ATTR(spiChannel, ECSPI_SS_POL::SS_ACTIVE_LOW);
        }

        //
        // Data is always active HIGH
        //
        configRegPtr->DATA_CTL =
            ECSPI_CH_ATTR(spiChannel, ECSPI_INACTIVE_DATA_CTL::DATA_LOW);

        //
        // We use single burst instead of multi-bursts mode for the
        // following reasons:
        // 1. Eliminate a delay that controller adds between bursts.
        //      At 4MBPS the delay is 1.2uSec.
        // 2. When multi-bursts mode is used, the controller negates the 
        //    CS line between each burst, which is undesirable and breaks 
        //    many devices.
        //
        //  Single burst is limited to 512 bytes, so the driver starts a 
        //  new burst on the fly for transfers that are longer than 512 bytes.
        //  In these cases, the CS is negated between bursts.
        //  To overcome this limitation we use a GPIO pin to drive the CS line.
        //
        configRegPtr->SS_CTL = 
            ECSPI_CH_ATTR(spiChannel, ECSPI_SS_CTL::SS_SINGLE_BURST);

    } // Configuration register

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPIHwSelectTarget is from EvtSpbControllerLock callback
//  to select a given target.
//  The routine resets the block and apply the configuration 
//  saved on the target context.
//
// Arguments:
//
//  TrgCtxPtr - The context to target to select.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIHwSelectTarget (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;
    volatile ECSPI_REGISTERS* ecspiRegsPtr = devExtPtr->ECSPIRegsPtr;
    const ECSPI_TARGET_SETTINGS* trgSettingsPtr = &TrgCtxPtr->Settings;

    //
    // Reset the block
    //
    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG, 0);

    //
    // Configure the block and select the channel.
    // When the block is disabled no register other than
    // ECSPI_CONREG can be accessed
    //
    {
        ECSPI_CONREG ctrlReg = { trgSettingsPtr->CtrlReg.AsUlong };

        ctrlReg.EN = 1;
        WRITE_REGISTER_NOFENCE_ULONG(
            &ecspiRegsPtr->CONREG, 
            ctrlReg.AsUlong
            );

    } // Configure the block and select the channel

    //
    // Disable and ACK all ECSPI interrupt
    //
    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG, 0);
    WRITE_REGISTER_NOFENCE_ULONG(
        &ecspiRegsPtr->STATREG, 
        ECSPI_INTERRUPT_GROUP::ECSPI_ACKABLE_INTERRUPTS
        );

    //
    // Update the delay/wait states register 
    //
    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->PERIODREG, 0);

    //
    // Configuration register
    //
    WRITE_REGISTER_NOFENCE_ULONG(
        &ecspiRegsPtr->CONFIGREG,
        trgSettingsPtr->ConfigReg.AsUlong
        );
}


//
// Routine Description:
//
//  ECSPIHwUnselectTarget is from ECSPIEvtSpbControllerUnlock callback
//  to un-select a given target.
//  
//
// Arguments:
//
//  TrgCtxPtr - The context to target to select.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIHwUnselectTarget (
    ECSPI_TARGET_CONTEXT* TrgCtxPtr
    )
{
    ECSPI_DEVICE_EXTENSION* devExtPtr = TrgCtxPtr->DevExtPtr;
    volatile ECSPI_REGISTERS* ecspiRegsPtr = devExtPtr->ECSPIRegsPtr;

    //
    // Disable and ACK all ECSPI interrupt
    //
    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG, 0);
    WRITE_REGISTER_NOFENCE_ULONG(
        &ecspiRegsPtr->STATREG, 
        ECSPI_INTERRUPT_GROUP::ECSPI_ACKABLE_INTERRUPTS
        );

    //
    // Reset the block
    //
    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG, 0);
}


//
// Routine Description:
//
//  ECSPIHwGetClockRange gets the min and max supported clock frequencies.
//
// Arguments:
//
//  MinClockPtr - Caller min clock address.
//
//  MaxClockPtr - Caller max clock address.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIHwGetClockRange (
    ULONG* MinClockHzPtr,
    ULONG* MaxClockHzPtr
    )
{
    *MinClockHzPtr = ECSPIDriverGetReferenceClock() / ECSPI_MAX_DIVIDER;
    *MaxClockHzPtr = ECSPIDriverGetMaxSpeed();
}


//
// Routine Description:
//
//  ECSPIHwInterruptControl is called to enable/disable interrupts
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
//  IsEnable - TRUE enable interrupts, otherwise FALSE
//
//  InterruptMask -  A combination of ECSPI_INTERRUPT_TYPE values.
//
// Return Value:
//
//  The new interrupt mask
//
_Use_decl_annotations_
ULONG
ECSPIHwInterruptControl (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    BOOLEAN IsEnable,
    ULONG InterruptMask
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;

    ECSPI_ASSERT(
        DevExtPtr->IfrLogHandle,
        InterruptMask <= ECSPI_INTERRUPT_TYPE::ALL
        );

    //
    // Update interrupt control
    //
    ECSPI_INTREG intReg;
    {
        intReg.AsUlong = READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG);
        if (IsEnable) {

            intReg.AsUlong |= InterruptMask;

        } else {

            intReg.AsUlong &= ~InterruptMask;
        }
        WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG, intReg.AsUlong);

    } // // Update interrupt control

    return intReg.AsUlong;
}

//
// Routine Description:
//
//  ECSPIHwConfigureTransfer is called to calculate the HW configuration
//  based on the given transfer information.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
//  TransferPtr - The transfer descriptor
//
//  IsInitialSetup - If this is the transfer initial setup.
//      When IsInitialSetup is TRUE the routine configure the HW based
//      on the transfer parameters.
//      When IsFirstTransfer is FALSE, the routine adds the transfer configuration to
//      the existing settings.
//
//  InterruptRegPtr - Address of the ECSPI_INTREG image
//      to receive the required interrupt control settings.
//      After all transfers have been configured, caller uses this value 
//      when calling ECSPIHwInterruptControl.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIHwConfigureTransfer (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ECSPI_SPB_TRANSFER* TransferPtr,
    BOOLEAN IsInitialSetup,
    PULONG InterruptRegPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;

    ECSPI_INTREG intReg = { 0 };
    ECSPI_DMAREG dmaReg = { 0 };
    if (IsInitialSetup) {

        ECSPI_CONREG ctrlReg = {
            READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG)
            };
        ctrlReg.BURST_LENGTH = (TransferPtr->BurstLength * 8) - 1;
        WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG, ctrlReg.AsUlong);
        TransferPtr->IsStartBurst = TRUE;

        #ifdef DBG
            ECSPIpHwEnableLoopbackIf(DevExtPtr);
        #endif // DBG

    } else {

        intReg.AsUlong = *InterruptRegPtr;
        dmaReg.AsUlong = READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->DMAREG);
    }

    //
    // Set/update RX/TX thresholds
    //
    {
        if (ECSPISpbIsWriteTransfer(TransferPtr)) {
            //
            // Write configuration
            //
            size_t wordsLeftToTransfer = ECSPISpbWordsLeftToTransfer(TransferPtr);

            intReg.AsUlong &= ~ECSPI_TX_INTERRUPTS;
            if (wordsLeftToTransfer > 0) {

                if (wordsLeftToTransfer > ECSPI_FIFO_DEPTH) {

                    dmaReg.TX_THRESHOLD = ECSPI_FIFO_DEPTH * 1 / 4;
                    intReg.TDREN = 1; // TX threshold interrupt

                } else {
                    //
                    // We enable TX FIFO empty, since we are
                    // polling the XCH bit, and cannot rely only on TC.
                    //
                    intReg.TEEN = 1;
                }
            }
            intReg.TCEN = 1; // Transfer Complete

        }  else {
            //
            // Read configuration
            //
            size_t wordsLeftToTransfer = ECSPISpbWordsLeftInBurst(TransferPtr);

            ECSPI_ASSERT(
                DevExtPtr->IfrLogHandle,
                wordsLeftToTransfer > 0
                );

            intReg.AsUlong &= ~ECSPI_RX_INTERRUPTS;
            if (wordsLeftToTransfer > ECSPI_FIFO_DEPTH) {
                dmaReg.RX_THRESHOLD = ECSPI_FIFO_DEPTH * 3 / 4;

            } else {

                dmaReg.RX_THRESHOLD = wordsLeftToTransfer - 1;
            }
            intReg.RDREN = 1; // RX threshold interrupt
        }

    } // Set RX/TX thresholds

    WRITE_REGISTER_NOFENCE_ULONG(
        &ecspiRegsPtr->DMAREG,
        dmaReg.AsUlong
        );
    *InterruptRegPtr = intReg.AsUlong;
}


//
// Routine Description:
//
//  ECSPIHwUpdateTransferConfiguration is called to update transfer 
//  RX/TX thresholds configuration due to transfer progress.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
//  TransferPtr - The transfer descriptor
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIHwUpdateTransferConfiguration (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ECSPI_SPB_TRANSFER* TransferPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;
    ECSPI_INTREG intReg = {
        READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG)
        };

    ECSPIHwConfigureTransfer(
        DevExtPtr,
        TransferPtr,
        FALSE, // Not initial setup
        &intReg.AsUlong
        );

    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG, intReg.AsUlong);
}


//
// Routine Description:
//
//  ECSPIHwWriteTxFIFO is called to write outgoing data from MDL to
//  TX FIFO.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
//  TransferPtr - The transfer descriptor
//
// Return Value:
//
//  TRUE: transfer is done, otherwise FALSE.
//
_Use_decl_annotations_
BOOLEAN
ECSPIHwWriteTxFIFO (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ECSPI_SPB_TRANSFER* TransferPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;
    volatile ULONG* txDataRegPtr = &ecspiRegsPtr->TXDATA;
    ECSPI_SPB_REQUEST* requestPtr = TransferPtr->AssociatedRequestPtr;

    ULONG maxWordsToWrite = ECSPIHwQueryTxFifoSpace(ecspiRegsPtr);
    ULONG totalBytesRead = 0;
    while ((maxWordsToWrite != 0) &&
           !ECSPISpbIsAllDataTransferred(TransferPtr)) {

        ULONG txFifoWord;
        ULONG bytesRead = ECSPIpHwReadWordFromMdl(TransferPtr, &txFifoWord);

        WRITE_REGISTER_NOFENCE_ULONG(txDataRegPtr, txFifoWord);
        --TransferPtr->BurstWords;

        ECSPIpHwUpdateTransfer(TransferPtr, bytesRead);

        ECSPIpHwStartBurstIf(DevExtPtr, TransferPtr);

        totalBytesRead += bytesRead;
        --maxWordsToWrite;
    }
    requestPtr->TotalBytesTransferred += totalBytesRead;

    ECSPIpHwStartBurstIf(DevExtPtr, TransferPtr);

    if (ECSPISpbIsTransferDone(TransferPtr)) {

        return TRUE;

    } else {

        ECSPIHwUpdateTransferConfiguration(DevExtPtr, TransferPtr);
        return FALSE;
    }
}


//
// Routine Description:
//
//  ECSPIHwReadRxFIFO is called to read incoming data from RX FIFO
//  to MDL.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
//  TransferPtr - The transfer descriptor
//
// Return Value:
//
//  TRUE: transfer is done, otherwise FALSE.
//
_Use_decl_annotations_
BOOLEAN
ECSPIHwReadRxFIFO (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ECSPI_SPB_TRANSFER* TransferPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;
    volatile ULONG* rxDataRegPtr = &ecspiRegsPtr->RXDATA;
    ECSPI_SPB_REQUEST* requestPtr = TransferPtr->AssociatedRequestPtr;

    ULONG totalBytesWritten = 0;
    while (!ECSPIHwIsRxFifoEmpty(ecspiRegsPtr) && 
           !ECSPISpbIsAllDataTransferred(TransferPtr)) {

        ULONG rxFifoWord = READ_REGISTER_NOFENCE_ULONG(rxDataRegPtr);
        ULONG bytesWritten = ECSPIpHwWriteWordToMdl(TransferPtr, rxFifoWord);

        ECSPIpHwUpdateTransfer(TransferPtr, bytesWritten);

        ECSPIpHwStartBurstIf(DevExtPtr, TransferPtr);

        totalBytesWritten += bytesWritten;
    }
    requestPtr->TotalBytesTransferred += totalBytesWritten;

    if (ECSPISpbIsTransferDone(TransferPtr)) {

        return TRUE;

    } else {
        ECSPIHwUpdateTransferConfiguration(DevExtPtr, TransferPtr);

        if (requestPtr->Type != ECSPI_REQUEST_TYPE::FULL_DUPLEX) {

            return ECSPIHwWriteZerosTxFIFO(DevExtPtr, TransferPtr);
        }
        return FALSE;
    }
}


//
// Routine Description:
//
//  ECSPIHwWriteZerosTxFIFO is called to write 0 to TX FIFO for
//  clocking in RX data.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
//  TransferPtr - The transfer object
//
// Return Value:
//  TRUE if burst scheduled but not started
//
_Use_decl_annotations_
BOOLEAN
ECSPIHwWriteZerosTxFIFO (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ECSPI_SPB_TRANSFER* TransferPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;
    volatile ULONG* txDataRegPtr = &ecspiRegsPtr->TXDATA;
    BOOLEAN burstStarted = FALSE;
    
    size_t wordsToWrite = TransferPtr->BurstWords;
    ULONG maxWordsToWrite = ECSPIHwQueryTxFifoSpace(ecspiRegsPtr);

    wordsToWrite = min(wordsToWrite, maxWordsToWrite);
    while (wordsToWrite != 0) {
        if (ECSPIHwIsTxFifoFull(ecspiRegsPtr)) {
            break;
        }
        WRITE_REGISTER_NOFENCE_ULONG(txDataRegPtr, 0);
        --TransferPtr->BurstWords;
        --wordsToWrite;

        burstStarted |= ECSPIpHwStartBurstIf(DevExtPtr, TransferPtr);
    }
    burstStarted |= ECSPIpHwStartBurstIf(DevExtPtr, TransferPtr);

    if (TransferPtr->IsStartBurst && !burstStarted) {
        // Need to start a new burst, but still processing current one.
        return TRUE;
    }
    return FALSE;
}


//
// Routine Description:
//
//  ECSPIHwClearFIFOs clears RX FIFO.
//  TX FIFO should be cleared.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIHwClearFIFOs (
    ECSPI_DEVICE_EXTENSION* DevExtPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;
    volatile ULONG* rxDataRegPtr = &ecspiRegsPtr->RXDATA;

    while (!ECSPIHwIsRxFifoEmpty(ecspiRegsPtr)) {

        (void)READ_REGISTER_NOFENCE_ULONG(rxDataRegPtr);
    }

    ECSPI_ASSERT(
        DevExtPtr->IfrLogHandle,
        ECSPIHwIsTxFifoEmpty(ecspiRegsPtr)
        );
}

//
// Routine Description:
//
//  ECSPIHwpDisableTransferInterrupts disables the interrupts associated with 
//  the given transfer.
//
// Arguments:
//
//  DevExtPtr - The device extension
//
//  TransferPtr - The transfer to start.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIHwDisableTransferInterrupts (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    const ECSPI_SPB_TRANSFER* TransferPtr
    )
{
    volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;
    ECSPI_INTREG intReg = {
        READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG)
        };

    if (ECSPISpbIsWriteTransfer(TransferPtr)) {

        intReg.AsUlong &= ~ECSPI_TX_INTERRUPTS;

    } else {

        intReg.AsUlong &= ~ECSPI_RX_INTERRUPTS;
    }
    WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->INTREG, intReg.AsUlong);
}


//
// Routine Description:
//
//  ECSPIHwIsSupportedDataBitLength checked if the given data bit length 
//  is supported by the driver.
//
// Arguments:
//
//  DataBitLength - Desired bit length
//
// Return Value:
//
//  TRUE - The give data bit length is supported by the driver, otherwise
//      FALSE.
//
_Use_decl_annotations_
BOOLEAN
ECSPIHwIsSupportedDataBitLength (
    ULONG DataBitLength
    )
{
    switch (DataBitLength) {
    case 8:
    case 16:
    case 32:
        return TRUE;

    default:
        return FALSE;
    }
}


// 
// ECSPIhw private methods
// -----------------------
//


//
// Routine Description:
//
//  ECSPIpHwCalcFreqDivider calculate the frequency divider settings
//  based on the required connection speed.
//
// Arguments:
//
//  DevExtPtr - The device extension.
//  
//  ConnectionSpeedHz - Desired connection speed in Hz.
//
//  CtrlRegPtr - Address of a ECSPI_CONREG register image to be configured to
//      the desired speed. 
//      Only CtrlRegPtr->PRE_DIVIDER, CtrlRegPtr->POST_DIVIDER are modified.
//
// Return Value:
//  
//  NTSTATUS
//
_Use_decl_annotations_
NTSTATUS
ECSPIpHwCalcFreqDivider (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ULONG ConnectionSpeedHz,
    ECSPI_CONREG* CtrlRegPtr
    )
{
    //
    // Connection speed has been validated during target connection.
    //
    ULONG refClockHz = ECSPIDriverGetReferenceClock();
    ULONG divider = refClockHz / ConnectionSpeedHz;

    ULONG preDivider = 0;
    ULONG postDivider = 0;
    while (divider >= ECSPI_PRE_DIVIDER_MAX) {

        ++postDivider;
        divider >>= 1;
    }
    preDivider = divider;

    if (postDivider > ECSPI_POST_DIVIDER_MAX) {
        ECSPI_LOG_ERROR(
            DevExtPtr->IfrLogHandle,
            "Connection post divider %lu%% is out of range. "
            "Max post divider value %lu%%",
            postDivider,
            ECSPI_POST_DIVIDER_MAX
            );
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Verify connection speed error is in range
    //
    {
        ULONG actualSpeedHz = (refClockHz / preDivider) >> postDivider;
        ULONG speedErrorHz = ConnectionSpeedHz > actualSpeedHz ?
            ConnectionSpeedHz - actualSpeedHz :
            actualSpeedHz - ConnectionSpeedHz;
        ULONG speedErrorPercent = (speedErrorHz * 100) / ConnectionSpeedHz;

        if (speedErrorPercent > MAX_SPEED_ERROR_PERCENT) {

            ECSPI_LOG_ERROR(
                DevExtPtr->IfrLogHandle,
                "Connection speed error %lu%% is out of range. "
                "Max connection error %lu%%",
                speedErrorPercent,
                MAX_SPEED_ERROR_PERCENT
                );
            return STATUS_NOT_SUPPORTED;
        }

        ECSPI_LOG_INFORMATION(
            DevExtPtr->IfrLogHandle,
            "Connection speed set to %luHz, reference clock %luHz. "
            "PREDIVIDER %lu, POSTDIVIDER %lu, error %lu%%",
            ConnectionSpeedHz,
            refClockHz,
            preDivider,
            postDivider,
            speedErrorPercent
            );

    } // Verify connection speed error is in range

    CtrlRegPtr->PRE_DIVIDER = preDivider > 0 ? preDivider - 1 : preDivider;
    CtrlRegPtr->POST_DIVIDER = postDivider;
    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  ECSPIpHwReadWordFromMdl reads a single word from transfer MDL, 
//  and updates the transfer.
//
// Arguments:
//
//  TransferPtr - The transfer descriptor
//
//  DataPtr - Address of a caller ULONG to receive the word to be
//      written to TX FIFO.
//
// Return Value:
//  
//  Number of bytes read from MDL
//
_Use_decl_annotations_
ULONG
ECSPIpHwReadWordFromMdl (
    ECSPI_SPB_TRANSFER* TransferPtr,
    PULONG DataPtr
    )
{
    PMDL mdlPtr = TransferPtr->CurrentMdlPtr;
    size_t mdlOffset = TransferPtr->CurrentMdlOffset;
    ULONG bytesToRead = sizeof(ULONG);
    ULONG readData = 0;
    UCHAR* dataBytePtr = reinterpret_cast<UCHAR*>(&readData);

    //
    // LSBytes are in the first WORD of burst
    //
    if (ECSPISpbIsBurstStart(TransferPtr)) {

        bytesToRead = TransferPtr->BurstLength % sizeof(ULONG);
        if (bytesToRead == 0) {

            bytesToRead = sizeof(ULONG);
        }
    }

    ULONG bytesLeftToRead = bytesToRead;
    while (mdlPtr != nullptr) {

        const UCHAR* mdlAddr =
            reinterpret_cast<const UCHAR*>(mdlPtr->MappedSystemVa) + mdlOffset;
        size_t byteCount = MmGetMdlByteCount(mdlPtr) - mdlOffset;

        while (byteCount > 0) {

            *dataBytePtr = *mdlAddr;

            ++dataBytePtr;
            ++mdlAddr;
            ++mdlOffset;

            --bytesLeftToRead;
            if (bytesLeftToRead == 0) {

                goto done;
            }

            --byteCount;

        } // More bytes to write

        mdlPtr = mdlPtr->Next;
        mdlOffset = 0;

    } // More MDLs

done:

    NT_ASSERT(bytesLeftToRead == 0);

    *DataPtr = ECSPIpHwDataSwap(readData, bytesToRead, TransferPtr->BufferStride);

    TransferPtr->CurrentMdlPtr = mdlPtr;
    TransferPtr->CurrentMdlOffset = mdlOffset;
    return bytesToRead - bytesLeftToRead;
}


//
// Routine Description:
//
//  ECSPIpHwWriteWordToMdl writes a single word to transfer MDL, 
//  and updates the transfer.
//
// Arguments:
//
//  TransferPtr - The transfer descriptor
//
//  DataPtr - Data to be written to MDL.
//
// Return Value:
//  
//  Number of bytes written to MDL
//
_Use_decl_annotations_
ULONG
ECSPIpHwWriteWordToMdl (
    ECSPI_SPB_TRANSFER* TransferPtr,
    ULONG Data
    )
{
    PMDL mdlPtr = TransferPtr->CurrentMdlPtr;
    size_t mdlOffset = TransferPtr->CurrentMdlOffset;
    ULONG bytesToWrite = sizeof(ULONG);
    ULONG dataToWrite;
    const UCHAR* dataBytePtr = reinterpret_cast<const UCHAR*>(&dataToWrite);

    //
    // LSBytes are in the first WORD of burst
    //
    if (ECSPISpbIsBurstStart(TransferPtr)) {

        bytesToWrite = TransferPtr->BurstLength % sizeof(ULONG);
        if (bytesToWrite == 0) {

            bytesToWrite = sizeof(ULONG);
        }
    }

    dataToWrite = ECSPIpHwDataSwap(Data, bytesToWrite, TransferPtr->BufferStride);

    ULONG bytesLeftToWrite = bytesToWrite;
    while (mdlPtr != nullptr) {

        UCHAR* mdlAddr = 
            reinterpret_cast<UCHAR*>(mdlPtr->MappedSystemVa) + mdlOffset;
        size_t byteCount = MmGetMdlByteCount(mdlPtr) - mdlOffset;

        while (byteCount > 0) {

            *mdlAddr = *dataBytePtr;

            ++dataBytePtr;
            ++mdlAddr;
            ++mdlOffset;

            --bytesLeftToWrite;
            if (bytesLeftToWrite == 0) {

                goto done;
            }

            --byteCount;

        } // More bytes to write

        mdlPtr = mdlPtr->Next;
        mdlOffset = 0;

    } // More MDLs

done:

    NT_ASSERT(bytesLeftToWrite == 0);

    TransferPtr->CurrentMdlPtr = mdlPtr;
    TransferPtr->CurrentMdlOffset = mdlOffset;
    return bytesToWrite - bytesLeftToWrite;
}


//
// Routine Description:
//  ECSPIpHwUpdateTransfer updates the transfer/burst progress with the number
//  of bytes transferred, and figures out if a new burst needs to be started. 
//
// Arguments:
//
//  TransferPtr - The transfer object.
//
//  BytesTransferred - Number of bytes transfered.
//
// Return Value:
//
_Use_decl_annotations_
VOID
ECSPIpHwUpdateTransfer (
    ECSPI_SPB_TRANSFER* TransferPtr,
    ULONG BytesTransferred
    )
{
    TransferPtr->BytesTransferred += BytesTransferred;
    TransferPtr->BytesLeftInBurst -= BytesTransferred;
    
    if (TransferPtr->BytesLeftInBurst == 0) {

        TransferPtr->BytesLeftInBurst =
            ECSPISpbBytesLeftToTransfer(TransferPtr);
        TransferPtr->BytesLeftInBurst =
            min(TransferPtr->BytesLeftInBurst, ECSPI_MAX_BURST_LENGTH_BYTES);
        
        if (TransferPtr->BytesLeftInBurst > 0) {

            TransferPtr->BurstLength = TransferPtr->BytesLeftInBurst;
            TransferPtr->BurstWords = ECSPISpbWordsLeftInBurst(TransferPtr);
            TransferPtr->IsStartBurst = TRUE;
        }
    }
}


//
// Routine Description:
//
//  ECSPIpHwStartBurstIf starts a burst if it has not been started,
//  by toggling the CONREG.XCH bit.
//  The routine verifies that the previous burst is done before starting 
//  a new one.
//  The driver uses this routine to poll the XCH and start a new burst when 
//  required. This way we can minimize the delay between bursts.
//
// Arguments:
//
//  DevExtPtr - The device extension
//
//  TransferPtr - The transfer to start.
//
// Return Value:
//  Returns TRUE if burst started.
//
_Use_decl_annotations_
BOOLEAN
ECSPIpHwStartBurstIf (
    ECSPI_DEVICE_EXTENSION* DevExtPtr,
    ECSPI_SPB_TRANSFER* TransferPtr
    )
{
    if (TransferPtr->IsStartBurst) {

        volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;

        ECSPI_CONREG ctrlReg = {
            READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG)
            };
        if (ctrlReg.XCH != 0) {
            //
            // Controller is still busy with previous burst
            //
            return FALSE;
        }

        //
        // In order for the new burst to start, TX FiFo needs to be not
        // empty
        //
        ECSPI_STATREG statReg = {
            READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->STATREG)
            };
        if (statReg.TE != 0) {

            return FALSE;
        }
        TransferPtr->IsStartBurst = FALSE;

        ctrlReg.BURST_LENGTH = (TransferPtr->BurstLength * 8) - 1;
        WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG, ctrlReg.AsUlong);

        ctrlReg.XCH = 1;
        WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->CONREG, ctrlReg.AsUlong);

        return TRUE;
    }

    return FALSE;
}


#if DBG
    //
    // Routine Description:
    //
    //  ECSPIpHwEnableLoopbackIf enables the controller loop back mode, if 
    //  the relevant driver flag is set.
    //
    // Arguments:
    //
    //  DevExtPtr - The device extension
    //
    // Return Value:
    //
    _Use_decl_annotations_
    VOID
    ECSPIpHwEnableLoopbackIf (
        ECSPI_DEVICE_EXTENSION* DevExtPtr
        )
    {
        //
        // Enabled loop back, if needed
        //
        if (ECSPIDriverIsLoopback()) {

            volatile ECSPI_REGISTERS* ecspiRegsPtr = DevExtPtr->ECSPIRegsPtr;

            ECSPI_TESTREG testReg = {
                READ_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->TESTREG)
                };
            testReg.LBC = 1;
            WRITE_REGISTER_NOFENCE_ULONG(&ecspiRegsPtr->TESTREG, testReg.AsUlong);

        } // Enabled loop back, if needed
    }
#endif 

#undef _ECSPI_HW_CPP_
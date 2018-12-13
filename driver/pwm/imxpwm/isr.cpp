// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   isr.cpp
//
// Abstract:
//
//  This module contains methods implementation for the ISR/DPC callbacks.
//
// Environment:
//
//  Kernel mode only
//

#include "precomp.h"
#pragma hdrstop

#include "imxpwmhw.hpp"
#include "imxpwm.hpp"

#include "trace.h"
#include "isr.tmh"

_Use_decl_annotations_
BOOLEAN
ImxPwmEvtInterruptIsr (
    WDFINTERRUPT WdfInterrupt,
    ULONG /*MessageID*/
    )
{
    IMXPWM_LOG_TRACE("()");

    IMXPWM_DEVICE_CONTEXT* deviceContextPtr = 
        ImxPwmGetDeviceContext(WdfInterruptGetDevice(WdfInterrupt));
    IMXPWM_REGISTERS* registersPtr = deviceContextPtr->RegistersPtr;
    IMXPWM_INTERRUPT_CONTEXT* interruptContextPtr =
        ImxPwmGetInterruptContext(WdfInterrupt);
    IMXPWM_PWMSR_REG pwmSr = { READ_REGISTER_ULONG(&registersPtr->PWMSR) };

    if ((pwmSr.AsUlong & IMXPWM_PWMSR_INTERRUPT_MASK) == 0) {
        //
        // This interrupt is not ours.
        //
        return FALSE;
    }

    //
    // Disable interrupts. It will be re-enabled later on-demand.
    //
    ImxPwmInterruptDisable(deviceContextPtr);

    //
    // Save the reason for the interrupt so that the DPC will process it.
    //
    interruptContextPtr->InterruptStatus = pwmSr;

    //
    // Acknowledge interrupts and clear status bits.
    //
    WRITE_REGISTER_ULONG(&registersPtr->PWMSR, pwmSr.AsUlong);

    //
    // Continue in DPC...
    //
    WdfInterruptQueueDpcForIsr(WdfInterrupt);

    return TRUE;
}

_Use_decl_annotations_
VOID
ImxPwmEvtInterruptDpc (
    WDFINTERRUPT WdfInterrupt,
    WDFOBJECT /*AssociatedWdfObject*/
    )
{
    IMXPWM_LOG_TRACE("()");

    IMXPWM_DEVICE_CONTEXT* deviceContextPtr =
        ImxPwmGetDeviceContext(WdfInterruptGetDevice(WdfInterrupt));
    IMXPWM_INTERRUPT_CONTEXT* interruptContextPtr =
        ImxPwmGetInterruptContext(WdfInterrupt);
    WDFREQUEST currentRequest = deviceContextPtr->CurrentRequest;

    if (currentRequest == NULL) {
        IMXPWM_LOG_ERROR("Spurious interrupt for a request that doesn't exist");
        ImxPwmDumpRegisters(deviceContextPtr);
        return;
    }

    IMXPWM_PWMSR_REG interruptStatus;

    //
    // Using WdfInterruptAcquireLock to guard the interrupt context is not
    // required here since the interrupt is disabled before queuing the DPC.
    //
    interruptStatus = interruptContextPtr->InterruptStatus;

    if (interruptStatus.FE != 0) {

        //
        // There is no dedicated interrupt for Fifo errors, but we don't expect
        // any Fifo error to happen since we always write when there is enough
        // space available in the Fifo, indicated by the FE interrupt.
        //
        NT_ASSERTMSG("Unexpected Fifo write error", interruptStatus.FWE == 0);

        //
        // There is a space in the Fifo, write the last sample and complete
        // the request successfully.
        //
        ImxPwmFifoWrite(
            deviceContextPtr,
            deviceContextPtr->CmpEventCounterCompare);

        deviceContextPtr->CurrentRequest = NULL;
        WdfRequestComplete(
            currentRequest,
            STATUS_SUCCESS);

    } else {
        NT_ASSERT(FALSE);
        IMXPWM_LOG_WARNING(
            "Unexpected interrupt source. (InterruptStatus = 0x%X)",
            interruptStatus.AsUlong);
    }
}
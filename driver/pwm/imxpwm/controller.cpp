// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   controller.cpp
//
// Abstract:
//
//  This module contains methods implementation for the PWM controller.
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
#include "controller.tmh"

IMXPWM_NONPAGED_SEGMENT_BEGIN; //==============================================

_Use_decl_annotations_
void
ImxPwmDumpRegisters (
    const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMXPWM_REGISTERS* registersPtr = DeviceContextPtr->RegistersPtr;

    IMXPWM_PWMCR_REG pwmCr = { READ_REGISTER_ULONG(&registersPtr->PWMCR) };
    IMXPWM_LOG_INFORMATION(
        "PWMCR (EN = 0x%X, REPEAT = 0x%X, SWR = 0x%X, PRESCALER=0x%X, "
        "CLKSRC = 0x%X, POUTC = 0x%X, HCTR = 0x%X, BCTR = 0x%X, DBGEN = 0x%X, "
        "WAITEN = 0x%X, DOZEN = 0x%X, STOPEN = 0x%X, FWM = 0x%X)",
        pwmCr.EN,
        pwmCr.REPEAT,
        pwmCr.SWR,
        pwmCr.PRESCALER,
        pwmCr.CLKSRC,
        pwmCr.POUTC,
        pwmCr.HCTR,
        pwmCr.BCTR,
        pwmCr.DBGEN,
        pwmCr.WAITEN,
        pwmCr.DOZEN,
        pwmCr.STOPEN,
        pwmCr.FWM);

    IMXPWM_PWMSR_REG pwmSr = { READ_REGISTER_ULONG(&registersPtr->PWMSR) };
    IMXPWM_LOG_INFORMATION(
        "PWMSR (FIFOAV = 0x%X, FE = 0x%X, ROV = 0x%X, CMP = 0x%X, FWE = 0x%X)",
        pwmSr.FIFOAV,
        pwmSr.FE,
        pwmSr.ROV,
        pwmSr.CMP,
        pwmSr.FWE);

    IMXPWM_PWMPR_REG pwmPr = { READ_REGISTER_ULONG(&registersPtr->PWMPR) };
    IMXPWM_LOG_INFORMATION(
        "PWMPR (PERIOD = 0x%X)",
        pwmPr.PERIOD);

    //
    // Reading from the Fifo is only allowed if PWM is disabled
    //
    if (pwmCr.EN == 0) {
        IMXPWM_PWMSAR_REG pwmSar = { READ_REGISTER_ULONG(&registersPtr->PWMSAR) };
        IMXPWM_LOG_INFORMATION("PWMPR (SAMPLE = 0x%X)", pwmSar.SAMPLE);
    }
}

//
// Period and Prescaler Calculation
//
// From the datasheet:
// PCLK = CLKSRC / PRESCALER -- Eq1
// POUT = PCLK / (PWMPR + 2) -- Eq2
//
// Where:
// CLKSRC: Clock source frequency
// PRESCALER: 12-bit prescaler for CLKSRC
// PCLK: Prescaler Clock Output
// POUT: Pwm Output Frequency
//
// Writing POUT in terms of the period T (the input DesiredPeriod), and
// substituting PCLK in Eq2 from Eq1:
// 10^12 / T = (CLKSRC / PRESCALER) / (PWMPR + 2)
//
// Solving for period T:
// (10^12 * (PWMPR + 2)) / T = (CLKSRC / PRESCALER)
// (10^12 * (PWMPR + 2) * PRESCALER) / T = CLKSRC
//  T = (10^12 * (PWMPR + 2) * PRESCALER) / CLKSRC -- Eq3
//
// Solving for PRESCALER:
//  PRESCALER = (T * CLKSRC) / (10^12 * (PWMPR + 2)) -- Eq4
//

_Use_decl_annotations_
NTSTATUS
ImxPwmCalculatePeriod (
    const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    USHORT ClockPrescaler,
    PWM_PERIOD* PeriodPtr
    )
{
    NT_ASSERT(ARGUMENT_PRESENT(PeriodPtr));
    NT_ASSERT(ClockPrescaler > 0);
    PWM_PERIOD period;
    NTSTATUS status =
        SafeMulDivRound64x32x64(
            PICOSECONDS_IN_1_SECOND,
            ULONG(ClockPrescaler) *
                ULONG(DeviceContextPtr->RovEventCounterCompare + 1),
            DeviceContextPtr->ClockSourceFrequency,
            &period);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "SafeMulDivRound64x32x64(%llu, %lu, %llu) failed. (status = %!STATUS!)",
            PICOSECONDS_IN_1_SECOND,
            ULONG(ClockPrescaler) *
                ULONG(DeviceContextPtr->RovEventCounterCompare + 1),
            DeviceContextPtr->ClockSourceFrequency,
            status);
    }

    *PeriodPtr = period;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmCalculatePrescaler (
    const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    PWM_PERIOD DesiredPeriod,
    USHORT* PrescalerPtr
    )
{
    NT_ASSERT(ARGUMENT_PRESENT(PrescalerPtr));
    NT_ASSERT(DesiredPeriod > 0);

    ULONGLONG prescaler;
    NTSTATUS status =
        SafeMulDivRound64x32x64(
            DesiredPeriod,
            DeviceContextPtr->ClockSourceFrequency,
            PICOSECONDS_IN_1_SECOND *
                (DeviceContextPtr->RovEventCounterCompare + 1),
            &prescaler);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "MulDivRound64x32x64(%llu, %lu, %llu) failed. (status = %!STATUS!)",
            DesiredPeriod,
            DeviceContextPtr->ClockSourceFrequency,
            PICOSECONDS_IN_1_SECOND *
                (DeviceContextPtr->RovEventCounterCompare + 1),
            status);
        return status;
    }

    NT_ASSERT(prescaler > 0);
    if (prescaler > IMXPWM_PRESCALER_MAX) {
        IMXPWM_LOG_ERROR(
            "Required prescaler %llu exceeds the max allowed value %lu. "
            "(Period = %llups)",
            prescaler,
            ULONG(IMXPWM_PRESCALER_MAX),
            DesiredPeriod);
        return STATUS_INTEGER_OVERFLOW;
    }

    static_assert(
        IMXPWM_PRESCALER_MAX <= 0xFFFF,
        "Prescaler should fit in 16-bit.");
    *PrescalerPtr = static_cast<USHORT>(prescaler);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmSoftReset (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    IMXPWM_PWMCR_REG pwmCr = { READ_REGISTER_ULONG(&registersPtr->PWMCR) };

    //
    // Controller soft-reset will result in resetting all registers to their
    // default values except for WAITEN, DBGEN, DOZEN and STOPEN registers
    // and we want them to be always 1 to keep the clock ungated all the time.
    //
    pwmCr.WAITEN = 1;
    pwmCr.DBGEN = 1;
    pwmCr.DOZEN = 1;
    pwmCr.STOPEN = 1;
    pwmCr.SWR = 1;

    WRITE_REGISTER_ULONG(&registersPtr->PWMCR, pwmCr.AsUlong);

    ULONG retry = IMXPWM_POLL_COUNT;
    pwmCr.AsUlong = READ_REGISTER_ULONG(&registersPtr->PWMCR);

    while ((pwmCr.SWR != 0) && (retry > 0)) {
        pwmCr.AsUlong = READ_REGISTER_ULONG(&registersPtr->PWMCR);
        KeStallExecutionProcessor(IMXPWM_POLL_WAIT_US);
        retry -= 1;
    }

    if (retry == 0) {
        IMXPWM_LOG_ERROR("Time-out waiting for a soft-reset to complete");
        ImxPwmDumpRegisters(DeviceContextPtr);
        return STATUS_IO_TIMEOUT;
    }

    //
    // The controller comes out of soft-reset clean of errors, with flushed Fifo
    // and PWM disabled, need to reconfigure its clock source and ROV counter according
    // to its last known state which is preserved in the device context.
    //

    //
    // Set Fifo Water Mark and PWM clock source.
    //
    pwmCr.AsUlong = READ_REGISTER_ULONG(&registersPtr->PWMCR);
    pwmCr.FWM = IMXPWM_PWMCR_FWM_2;
    pwmCr.CLKSRC = DeviceContextPtr->ClockSource;
    WRITE_REGISTER_ULONG(&registersPtr->PWMCR, pwmCr.AsUlong);

    //
    // Set Roll Over Event (ROV) counter compare. The datasheet calls it PERIOD
    // which can be confusing, since this alone does not decide PWM output period.
    // Note that the counter counts from 0 to PWMPR + 1 inclusive, and resets after
    // its value equals PWMPR + 1, which RovEventCounterCompare represents
    //
    IMXPWM_PWMPR_REG pwmPr = { DeviceContextPtr->RovEventCounterCompare - 1ul };
    WRITE_REGISTER_ULONG(&registersPtr->PWMPR, pwmPr.AsUlong);

    //
    // Reset software maintained state
    //

    NTSTATUS status =
        ImxPwmSetDesiredPeriod(
            DeviceContextPtr,
            DeviceContextPtr->DefaultDesiredPeriod);
    if (!NT_SUCCESS(status)) {
        IMXPWM_LOG_ERROR(
            "ImxPwmSetDesiredPeriod(...) failed. (status = %!STATUS!)",
            status);
        return status;
    }

    DeviceContextPtr->Pin.ActiveDutyCycle = 0;
    DeviceContextPtr->Pin.Polarity = PWM_ACTIVE_HIGH;
    DeviceContextPtr->Pin.IsStarted = false;

    ImxPwmDumpRegisters(DeviceContextPtr);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmSetDesiredPeriod (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    PWM_PERIOD DesiredPeriod
    )
{
    NT_ASSERT(
        (DesiredPeriod >= DeviceContextPtr->ControllerInfo.MinimumPeriod) &&
        (DesiredPeriod <= DeviceContextPtr->ControllerInfo.MaximumPeriod));

    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    IMXPWM_PWMCR_REG pwmCr = { READ_REGISTER_ULONG(&registersPtr->PWMCR) };

    USHORT prescaler;
    NTSTATUS status =
        ImxPwmCalculatePrescaler(DeviceContextPtr, DesiredPeriod, &prescaler);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    NT_ASSERT(prescaler > 0);

    pwmCr.PRESCALER = prescaler - 1;
    WRITE_REGISTER_ULONG(&registersPtr->PWMCR, pwmCr.AsUlong);

    PWM_PERIOD actualPeriod;
    status =
        ImxPwmCalculatePeriod(
            DeviceContextPtr,
            prescaler,
            &actualPeriod);
    if (!NT_SUCCESS(status)) {
        NT_ASSERTMSG("Unexpected actual period computation failure.", FALSE);
        IMXPWM_LOG_ERROR(
            "Unexpected failure computing the actual period. "
            "(prescaler = %llu DesiredPeriod = %llups, status = %!STATUS!)",
            prescaler,
            DesiredPeriod,
            status);
        return status;
    }

    DeviceContextPtr->DesiredPeriod = DesiredPeriod;
    DeviceContextPtr->ActualPeriod = actualPeriod;

    IMXPWM_LOG_INFORMATION(
        "Setting new period. (DesiredPeriod = %llups(%lluHz), ActualPeriod = %llups(%lluHz))",
        DesiredPeriod,
        PICOSECONDS_IN_1_SECOND / DesiredPeriod,
        actualPeriod,
        PICOSECONDS_IN_1_SECOND / actualPeriod);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmSetActiveDutyCycle (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    PWM_PERCENTAGE ActiveDutyCycle
    )
{
    IMXPWM_PIN_STATE* pinPtr = &DeviceContextPtr->Pin;
    //
    // Delay hardware setting of duty cycle till PWM starts. That avoids filling the
    // Fifo unnecessarily with stale duty cycles. If PWM is stopped, then do nothing
    // on the hardware level, otherwise do the duty cycle hardware setting.
    //
    if (!pinPtr->IsStarted) {
        pinPtr->ActiveDutyCycle = ActiveDutyCycle;
        return STATUS_SUCCESS;
    }

    //
    // Scale down the desired duty cycle from 64-bit to 32-bit to avoid overflow
    // errors that may result from multiplying 64-bit by 32-bit operands, which will
    // require a 96-bit intermediate value, otherwise the multiply divide operation
    // will overflow.
    //
    ULONGLONG cmpEventCounterCompare;

    if (ActiveDutyCycle == 0) {
        cmpEventCounterCompare = 0;
    } else if (ActiveDutyCycle == PWM_PERCENTAGE_MAX) {
        cmpEventCounterCompare = DeviceContextPtr->RovEventCounterCompare + 1;
    } else {
        NTSTATUS status =
            SafeDivRound64x64(
                (ActiveDutyCycle >> 32) *
                    DeviceContextPtr->RovEventCounterCompare,
                PWM_PERCENTAGE_MAX >> 32,
                &cmpEventCounterCompare);
        if (!NT_SUCCESS(status)) {
            IMXPWM_LOG_ERROR(
                "Unexpected failure computing CmpEventCounterCompare. "
                "(ActiveDutyCycle = %llu, status = %!STATUS!)",
                ActiveDutyCycle,
                status);
            return status;
        }
    }

    if (cmpEventCounterCompare >
            DeviceContextPtr->RovEventCounterCompare + 1) {
        IMXPWM_LOG_ERROR(
            "CMP Event Counter Compare overflow its max %lu. "
            "(ActiveDutyCycle = %llu, cmpEventCounterCompare = %llu)",
            DeviceContextPtr->RovEventCounterCompare,
            ActiveDutyCycle,
            cmpEventCounterCompare);
        return STATUS_INTEGER_OVERFLOW;
    }

    static_assert(
        IMXPWM_ROV_EVT_COUNTER_COMPARE <= 0xFFFF,
        "Sample should fit in 16-bit");
    NT_ASSERT(cmpEventCounterCompare <= IMXPWM_ROV_EVT_COUNTER_COMPARE + 1);

    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    IMXPWM_PWMSR_REG pwmSr = { READ_REGISTER_ULONG(&registersPtr->PWMSR) };

    if (pwmSr.FWE != 0) {
        IMXPWM_LOG_ERROR("Unexpected Fifo write error.");
        ImxPwmDumpRegisters(DeviceContextPtr);
        return STATUS_IO_DEVICE_ERROR;
    }

    pinPtr->ActiveDutyCycle = ActiveDutyCycle;
    DeviceContextPtr->CmpEventCounterCompare =
            static_cast<USHORT>(cmpEventCounterCompare);

    NTSTATUS status;

    //
    // Clear status bits
    //
    WRITE_REGISTER_ULONG(&registersPtr->PWMSR, pwmSr.AsUlong);

    //
    // Fifo is not full, we can safely write the sample to the Fifo and complete
    // the request inline.
    //
    if (pwmSr.FIFOAV < IMXPWM_FIFO_SAMPLE_COUNT) {
        ImxPwmFifoWrite(
            DeviceContextPtr,
            DeviceContextPtr->CmpEventCounterCompare);
        status = STATUS_SUCCESS;
    } else {
        //
        // There is no space in the Fifo for the current sample.
        //
        // We need FE interrupt to write the sample to the Fifo and complete
        // the request. Enable the interrupts here and disable them later in
        // the ISR.
        //
        IMXPWM_LOG_TRACE("Fifo is full. Waiting for FE interrupt.");
        ImxPwmInterruptEnable(DeviceContextPtr);
        status = STATUS_PENDING;
    }

    IMXPWM_LOG_TRACE(
        "(ActiveDutyCycle = %llu, CmpEventCounterCompare (Sample) = %lu, FIFOAV = %lu)",
        ActiveDutyCycle,
        ULONG(DeviceContextPtr->CmpEventCounterCompare),
        pwmSr.FIFOAV);

    return status;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmSetPolarity (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    PWM_POLARITY Polarity
    )
{
    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    IMXPWM_PWMCR_REG pwmCr = { READ_REGISTER_ULONG(&registersPtr->PWMCR) };

    switch (Polarity) {
    case PWM_ACTIVE_HIGH:
        pwmCr.POUTC = IMXPWM_PWMCR_POUTC_SET_ROV_CLR_CMP;
        break;

    case PWM_ACTIVE_LOW:
        pwmCr.POUTC = IMXPWM_PWMCR_POUTC_CLR_ROV_SET_CMP;
        break;

    default:
        NT_ASSERTMSG("Invalid polarity", FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    WRITE_REGISTER_ULONG(&registersPtr->PWMCR, pwmCr.AsUlong);

    DeviceContextPtr->Pin.Polarity = Polarity;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmStart (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMXPWM_PIN_STATE* pinPtr = &DeviceContextPtr->Pin;
    NT_ASSERT(!pinPtr->IsStarted);

    NTSTATUS status =
    ImxPwmSetActiveDutyCycle(
        DeviceContextPtr,
        pinPtr->ActiveDutyCycle);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // When the PWM is about to start, the Fifo should be empty and the request
    // does not need additional processing in the ISR/DPC for which STATUS_PENDING
    // is returned instead.
    //
    NT_ASSERT(status == STATUS_SUCCESS);

    //
    // Enable PWM
    //
    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    IMXPWM_PWMCR_REG pwmCr = { READ_REGISTER_ULONG(&registersPtr->PWMCR) };
    NT_ASSERTMSG("PWM is expected to be disabled", pwmCr.EN == 0);
    pwmCr.EN = 1;
    WRITE_REGISTER_ULONG(&registersPtr->PWMCR, pwmCr.AsUlong);

    pinPtr->IsStarted = true;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ImxPwmStop (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMXPWM_PIN_STATE* pinPtr = &DeviceContextPtr->Pin;
    NT_ASSERT(pinPtr->IsStarted);

    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    IMXPWM_PWMCR_REG pwmCr = { READ_REGISTER_ULONG(&registersPtr->PWMCR) };

    NT_ASSERTMSG("PWM is expected to be enabled", pwmCr.EN == 1);
    pwmCr.EN = 0;
    WRITE_REGISTER_ULONG(&registersPtr->PWMCR, pwmCr.AsUlong);

    //
    // If there are stale samples in the Fifo after stopping PWM, the only way
    // to flush the Fifo is to reset the controller and reprogram it. Flushing
    // the Fifo is required, otherwise the next time the PWM start it may output
    // 1 to 4 stale samples from the Fifo.
    //
    IMXPWM_PWMSR_REG pwmSr = { READ_REGISTER_ULONG(&registersPtr->PWMSR) };
    if (pwmSr.FIFOAV > 0) {
        NTSTATUS status = ImxPwmSoftReset(DeviceContextPtr);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // The controller comes clean out of the software reset, we only need to
        // program the period and polarity, while duty cycle gets programmed in
        // the PWM start callback ImxPwmStart
        //

        status =
            ImxPwmSetDesiredPeriod(
                DeviceContextPtr,
                DeviceContextPtr->DesiredPeriod);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status =
            ImxPwmSetPolarity(DeviceContextPtr, DeviceContextPtr->Pin.Polarity);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    pinPtr->IsStarted = false;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void
ImxPwmInterruptDisable (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    WRITE_REGISTER_ULONG(&registersPtr->PWMIR, 0);
}

_Use_decl_annotations_
void
ImxPwmInterruptEnable (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMXPWM_REGISTERS *registersPtr = DeviceContextPtr->RegistersPtr;
    IMXPWM_PWMIR_REG pwmIr = { 0 };

    //
    // We use only the Fifo Empty interrupt which is reflected in PWMSR.FE bit.
    //
    pwmIr.FIE = 1;
    WRITE_REGISTER_ULONG(&registersPtr->PWMIR, pwmIr.AsUlong);
}

IMXPWM_NONPAGED_SEGMENT_END; //=================================================
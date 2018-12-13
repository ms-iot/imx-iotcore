// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   imxpwm.hpp
//
// Abstract:
//
//   i.MX PWM Driver Declarations
//

#ifndef _IMXPWM_HPP_
#define _IMXPWM_HPP_

//
// Macros to be used for proper PAGED/NON-PAGED code placement
//

#define IMXPWM_NONPAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    //__pragma(code_seg(.text))

#define IMXPWM_NONPAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMXPWM_PAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("PAGE"))

#define IMXPWM_PAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMXPWM_INIT_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("INIT"))

#define IMXPWM_INIT_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMXPWM_ASSERT_MAX_IRQL(Irql) NT_ASSERT(KeGetCurrentIrql() <= (Irql))

enum : ULONG { IMXPWM_POOL_TAG = 'WPXM' };

enum : ULONG {
    //
    // i.MX PWM controller have 1 channel only
    //
    IMXPWM_PIN_COUNT = 1,

    IMXPWM_POLL_WAIT_US = 10,
    IMXPWM_POLL_COUNT = 5,

    IMXPWM_DEFAULT_CLKSRC = IMXPWM_PWMCR_CLKSRC_IPG_CLK,
    IMXPWM_DEFAULT_CLKSRC_FREQ = 66000000
};

enum : USHORT {
    IMXPWM_PRESCALER_MIN = 1,
    IMXPWM_PRESCALER_MAX = IMXPWM_PWMCR_PRESCALER_MAX + 1,
    //
    // With a 66MHz clock source, 12-bit prescaler and a 16-bit counter, fixing
    // the ROV event counter compare to 4096 will allow for a PWM output
    // that ranges in frequency from 4Hz and up to 16KHz by only varying the
    // 12-bit prescaler.
    //
    IMXPWM_ROV_EVT_COUNTER_COMPARE = 4096,
    IMXPWM_CMP_EVT_COUNTER_COMPARE_MAX = IMXPWM_ROV_EVT_COUNTER_COMPARE,
};

static_assert(
    IMXPWM_ROV_EVT_COUNTER_COMPARE <= IMXPWM_PWMPR_PERIOD_MAX,
    "Counter compare for ROV should be less than or equal PWMPR max value");

enum : ULONGLONG {
    PICOSECONDS_IN_1_SECOND = 1000000000000
};

struct IMXPWM_PIN_STATE {
    PWM_POLARITY Polarity;
    PWM_PERCENTAGE ActiveDutyCycle;
    bool IsStarted;
    bool IsOpenForWrite;
    //
    // A lock to protect IsOpenForWrite during file create/close callbacks
    //
    WDFWAITLOCK Lock;
}; // struct IMXPWM_PIN_STATE

struct IMXPWM_DEVICE_CONTEXT {
    IMXPWM_REGISTERS* RegistersPtr;
    WDFDEVICE WdfDevice;
    WDFINTERRUPT WdfInterrupt;
    WDFREQUEST CurrentRequest;
    WDFSTRING DeviceInterfaceSymlinkName;
    UNICODE_STRING DeviceInterfaceSymlinkNameWsz;

    //
    // HW State
    //

    IMXPWM_PWMCR_CLKSRC ClockSource;
    ULONG ClockSourceFrequency;
    //
    // Roll-over counter compare, equals to PWMPR + 1
    //
    USHORT RovEventCounterCompare;
    USHORT ClockPrescaler;
    //
    // Last sample value in the closed range [0, PWMPR + 1]
    //
    USHORT CmpEventCounterCompare;

    PWM_PERIOD DefaultDesiredPeriod;

    //
    // Controller and Pin State
    //

    bool IsControllerOpenForWrite;
    //
    // A lock to protect the controller IsOpenForWrite during file create/close
    // callbacks
    //
    WDFWAITLOCK ControllerLock;
    PWM_PERIOD DesiredPeriod;
    PWM_PERIOD ActualPeriod;
    IMXPWM_PIN_STATE Pin;

    //
    // Controller Info
    //

    PWM_CONTROLLER_INFO ControllerInfo;

}; // struct IMXPWM_DEVICE_CONTEXT

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMXPWM_DEVICE_CONTEXT,
    ImxPwmGetDeviceContext);

struct IMXPWM_FILE_OBJECT_CONTEXT {
    bool IsOpenForWrite;
    bool IsPinInterface;
}; // struct IMXPWM_FILE_OBJECT_CONTEXT

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMXPWM_FILE_OBJECT_CONTEXT,
    ImxPwmGetFileObjectContext);

struct IMXPWM_INTERRUPT_CONTEXT {
    IMXPWM_PWMSR_REG InterruptStatus;
}; // struct IMXPWM_INTERRUPT_CONTEXT

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    IMXPWM_INTERRUPT_CONTEXT,
    ImxPwmGetInterruptContext);

//
// NONPAGED
//

EVT_WDF_DEVICE_D0_ENTRY ImxPwmEvtDeviceD0Entry;
EVT_WDF_INTERRUPT_ISR ImxPwmEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC ImxPwmEvtInterruptDpc;

void
ImxPwmInterruptDisable (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

void
ImxPwmInterruptEnable (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

__forceinline
void
ImxPwmFifoWrite (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ USHORT Sample
    )
{
    IMXPWM_REGISTERS* registersPtr = DeviceContextPtr->RegistersPtr;

    IMXPWM_PWMSR_REG pwmSr = { READ_REGISTER_ULONG(&registersPtr->PWMSR) };
    NT_ASSERTMSG("Fifo shouldn't be full", pwmSr.FIFOAV < IMXPWM_FIFO_SAMPLE_COUNT);

    IMXPWM_PWMSAR_REG pwmSar = { 0 };
    pwmSar.SAMPLE = Sample;
    WRITE_REGISTER_ULONG(&registersPtr->PWMSAR, pwmSar.AsUlong);

    pwmSr.AsUlong = READ_REGISTER_ULONG(&registersPtr->PWMSR);
    NT_ASSERTMSG("Fifo write error unexpected", pwmSr.FWE == 0);
}

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ImxPwmEvtIoDeviceControl;

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlControllerGetInfo(
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlControllerGetActualPeriod (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlControllerSetDesiredPeriod (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlPinGetActiveDutyCycle (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlPinSetActiveDutyCycle (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlPinGetPolarity (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlPinSetPolarity (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlPinStart (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlPinStop (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ImxPwmIoctlPinIsStarted (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ WDFREQUEST WdfRequest
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmSoftReset (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmSetDesiredPeriod (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ PWM_PERIOD DesiredPeriod
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmSetActiveDutyCycle (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ PWM_PERCENTAGE ActiveDutyCycle
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmSetPolarity (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ PWM_POLARITY Polarity
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmStart (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmStop (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmCalculatePrescaler (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ PWM_PERIOD DesiredPeriod,
    _Out_ USHORT* PrescalerPtr
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmCalculatePeriod (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr,
    _In_ USHORT ClockPrescaler,
    _Out_ PWM_PERIOD* PeriodPtr
    );

_IRQL_requires_same_
void
ImxPwmDumpRegisters (
    _In_ const IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

//
// PAGED
//

//
// In safe-arithmetic, perform integer division (LeftAddend + RightAddend)/Divisor
// and return the result rounded up or down to the nearest integer where 3.5 and
// 3.75 are near 4, while 3.25 is near 3.
//
// LeftAddend: First 64-bit unsigned integer addend.
// RightAddend: Second 64-bit unsigned integer addend.
// ResultPtr: A pointer to a 64-bit unsigned integer that receives the result.
//
// Returns error in case of overflow, otherwise returns STATUS_SUCCESS.
//
__forceinline
NTSTATUS
SafeAddDivRound64x64x64 (
    _In_ ULONGLONG LeftAddend,
    _In_ ULONGLONG RightAddend,
    _In_ ULONGLONG Divisor,
    _Out_ ULONGLONG* ResultPtr)
{
    ASSERT(ARGUMENT_PRESENT(ResultPtr));
    ASSERT(Divisor > 0);

    //
    // Calculate the following in safe-arithmetic to avoid overflows:
    // return ((LeftAddend + RightAddend) + (Divisor / 2)) / Divisor;
    //

    ULONGLONG dividend;
    NTSTATUS status = RtlULongLongAdd(LeftAddend, RightAddend, &dividend);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONGLONG result;
    status = RtlULongLongAdd(dividend, Divisor / 2, &result);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    *ResultPtr = result / Divisor;

    return STATUS_SUCCESS;
}

//
// Return the result of Dividend / Divisor rounded up or down to the nearest intege
// where 3.5 and 3.75 are near 4, while 3.25 is near 3. Note that the implementation
// is subject to overflow if the intermediate value of adding Dividend + (Divisor / 2)
// overflows 64-bit. Use only if you are sure that an overflow is not possible.
//
// Dividend: A 64-bit unsigned integer dividend.
// Divisor: A 64-bit unsigned integer divisor.
//
// Returns the rounded division 64-bit result
//
__forceinline
ULONGLONG
UnsafeDivRound64x64 (
    _In_ ULONGLONG Dividend,
    _In_ ULONGLONG Divisor
    )
{
    ASSERT(Divisor > 0);
    return (Dividend + (Divisor / 2)) / Divisor;
}

//
// In safe-arithmetic, perform integer division Dividend/Divisor and return the
// result rounded up or down to the nearest integer where 3.5 and 3.75 are near
// 4, while 3.25 is near 3.
//
// Dividend: A 64-bit unsigned integer dividend.
// Divisor: A 64-bit unsigned integer divisor.
// ResultPtr: A pointer to a 64-bit unsigned integer that receives the result.
//
// Returns error in case of overflow, otherwise returns STATUS_SUCCESS.
//
__forceinline
NTSTATUS
SafeDivRound64x64 (
    _In_ ULONGLONG Dividend,
    _In_ ULONGLONG Divisor,
    _Out_ ULONGLONG* ResultPtr
    )
{
    ASSERT(ARGUMENT_PRESENT(ResultPtr));
    ASSERT(Divisor > 0);

    //
    // Calculate the following in safe-arithmetic to avoid overflows:
    // return (Dividend + (Divisor / 2)) / Divisor;
    //

    ULONGLONG result;
    NTSTATUS status = RtlULongLongAdd(Dividend, Divisor / 2, &result);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    *ResultPtr = result / Divisor;

    return STATUS_SUCCESS;
}

//
// In safe-arithmetic, perform a multiplication followed by division in the form
// (Mul64 * Mul32) / Div64 where the result is rounded to the nearest integer.
//
// Mul64: A 64-bit unsigned integer multiplicand.
// Mul32: A 32-bit unsigned multiplier.
// Div64: A 64-bit unsigned integer divisor.
// ResultPtr: A pointer to a 64-bit unsigned integer that receives the result.
//
// Returns error in case of overflow, otherwise returns STATUS_SUCCESS.
//
__forceinline
NTSTATUS
SafeMulDivRound64x32x64 (
    _In_ ULONGLONG Mul64,
    _In_ ULONG Mul32,
    _In_ ULONGLONG Div64,
    _Out_ ULONGLONG* Result64Ptr
    )
{
    ASSERT(ARGUMENT_PRESENT(Result64Ptr));

    ULONGLONG q = Mul64 / Div64;
    ULONGLONG r = Mul64 % Div64;
    NTSTATUS status;

    //
    // Calculate the following in safe-arithmetic to avoid overflows:
    // return (q * Mul32) + ((r * Mul32) / Div64);
    //

    ULONGLONG qMul32;
    status = RtlULongLongMult(q, Mul32, &qMul32);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONGLONG rMul32;
    status = RtlULongLongMult(r, Mul32, &rMul32);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONGLONG rMul32OverDiv64;
    status = SafeDivRound64x64(rMul32, Div64, &rMul32OverDiv64);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONGLONG result;
    status = RtlULongLongAdd(qMul32, rMul32OverDiv64, &result);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    *Result64Ptr = result;

    return STATUS_SUCCESS;
}

__forceinline
ULONGLONG
ImxPwmPeriodToFrequency(
    _In_ PWM_PERIOD DesiredPeriod
    )
{
    return UnsafeDivRound64x64(PICOSECONDS_IN_1_SECOND, DesiredPeriod);
}

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ImxPwmCreateDeviceInterface (
    _In_ IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

__forceinline
void
ImxPwmCreateRequestGetAccess(
    _In_ WDFREQUEST WdfRequest,
    _Out_ ACCESS_MASK* DesiredAccessPtr,
    _Out_ ULONG* ShareAccessPtr
    )
{
    NT_ASSERT(ARGUMENT_PRESENT(DesiredAccessPtr));
    NT_ASSERT(ARGUMENT_PRESENT(ShareAccessPtr));

    WDF_REQUEST_PARAMETERS wdfRequestParameters;
    WDF_REQUEST_PARAMETERS_INIT(&wdfRequestParameters);
    WdfRequestGetParameters(WdfRequest, &wdfRequestParameters);

    NT_ASSERTMSG(
        "Expected create request",
        wdfRequestParameters.Type == WdfRequestTypeCreate);

    *DesiredAccessPtr =
        wdfRequestParameters.Parameters.Create.SecurityContext->DesiredAccess;
    *ShareAccessPtr = wdfRequestParameters.Parameters.Create.ShareAccess;
}

_IRQL_requires_same_
NTSTATUS
ImxPwmResetControllerDefaults (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

_IRQL_requires_same_
NTSTATUS
ImxPwmResetPinDefaults (
    IMXPWM_DEVICE_CONTEXT* DeviceContextPtr
    );

EVT_WDF_DEVICE_PREPARE_HARDWARE ImxPwmEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE ImxPwmEvtDeviceReleaseHardware;
EVT_WDF_DRIVER_DEVICE_ADD ImxPwmEvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD ImxPwmDriverUnload;
EVT_WDF_DEVICE_FILE_CREATE ImxPwmEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE ImxPwmEvtFileClose;

extern "C" DRIVER_INITIALIZE DriverEntry;

#endif // _IMXPWM_H_
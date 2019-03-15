// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   imxgpio.cpp
//
// Abstract:
//
//  This module contains implementation of i.MX Series GPIO controller driver.
//
// Environment:
//
//  Kernel mode only
//

#include "precomp.hpp"

#include "trace.h"
#include "imxgpio.tmh"

#include <ImxCpuRev.h>
#include "imxutility.hpp"
#include "imxgpio.hpp"

#include "imx6sx.hpp"
#include "imx6dq.hpp"
#include "imx6sdl.hpp"
#include "imx7d.hpp"
#include "imx6ull.hpp"
#include "imx8m.hpp"
#include "imx8m-mini.hpp"

IMX_NONPAGED_SEGMENT_BEGIN; //====================================================

namespace {

    LPCSTR PullModeToString (
        IMX_SOC soc,
        IMX_GPIO_PULL mode
        )
    {
        switch (soc) {
        case IMX_SOC_MX8M:
            switch (mode) {
            case IMX_GPIO_PULL_DEFAULT:
                return "PullDefault";
            case IMX8M_GPIO_PULL_UP:
            case IMX8MM_GPIO_PULL_UP:
                return "PullUp";
            case IMX8MM_GPIO_PULL_DOWN:
                return "PullDown";
            case IMX_GPIO_PULL_DISABLE:
                return "PullNone";
            case IMX_GPIO_PULL_INVALID:
            default:
                NT_ASSERT(!"Invalid pull mode");
                return "Invalid";
            }
            break;

        case IMX_SOC_MX6:
        case IMX_SOC_MX7:
            switch (mode) {
            case IMX_GPIO_PULL_DEFAULT:
                return "PullDefault";
            case IMX6_GPIO_PULL_UP:
            case IMX7_GPIO_PULL_UP:
                return "PullUp";
            case IMX6_GPIO_PULL_DOWN:
            case IMX7_GPIO_PULL_DOWN:
                return "PullDown";
            case IMX_GPIO_PULL_DISABLE:
                return "PullNone";
            case IMX_GPIO_PULL_INVALID:
            default:
                NT_ASSERT(!"Invalid pull mode");
                return "Invalid";
            }
            break;

        default:
            NT_ASSERT(!"Invalid SOC type");
            return "Invalid";
        }
    }

    LPCSTR InterruptConfigToString (
        IMX_GPIO_INTERRUPT_CONFIG config
        )
    {
        switch (config)
        {
        case IMX_GPIO_INTERRUPT_CONFIG_HIGH_LEVEL:
            return "HighLevel";
        case IMX_GPIO_INTERRUPT_CONFIG_LOW_LEVEL:
            return "LowLevel";
        case IMX_GPIO_INTERRUPT_CONFIG_RISING_EDGE:
            return "RisingEdge";
        case IMX_GPIO_INTERRUPT_CONFIG_FALLING_EDGE:
            return "FallingEdge";
        default:
            NT_ASSERT(!"Invalid interrupt config");
            return "Invalid";
        }
    }

    NTSTATUS ParsePadCtlFromVendorData (
        _In_reads_bytes_opt_(VendorDataLength) PVOID VendorData,
        ULONG VendorDataLength,
        _Out_ IMX_VENDOR_DATA_ELEMENT *Element
        )
    {
        Element->Tag = IMX_TAG_EMPTY;

        if ((VendorData == nullptr) || (VendorDataLength == 0)) {
            return STATUS_SUCCESS;
        }

        const UCHAR* bytes = reinterpret_cast<const UCHAR *>(VendorData);

        switch (*bytes) {
        case 0x00:  // no-op
            break;

        case 0x01:  // PAD_CTL ULONG
            VendorDataLength -= sizeof(UCHAR);
            if (VendorDataLength < sizeof(ULONG)) {
                LogError("VendorData element too short");
                return STATUS_INVALID_PARAMETER;
            }

            Element->Tag = static_cast<IMX_VENDOR_DATA_TAG>(*bytes);
            ++bytes;
            Element->PadCtl = *reinterpret_cast<const ULONG *>(bytes);
            bytes += sizeof(ULONG);
            break;

        default:
            LogError("Invalid VendorData tag 0x%x", (ULONG)*bytes);
            return STATUS_INVALID_PARAMETER;
        }

        return STATUS_SUCCESS;
    }
} // namespace static

UINT32 IMX_GPIO::cpuRev;
UINT32 IMX_GPIO::bankStride;
UINT32 IMX_GPIO::bankCount;
USHORT IMX_GPIO::pinCount;
UINT32 IMX_GPIO::pullShift;
UINT32 IMX_GPIO::pullMask;
UINT32 IMX_GPIO::pullUp;
UINT32 IMX_GPIO::pullDown;
const IMX_PIN_DATA* IMX_GPIO::sparsePinMap;
UINT32 IMX_GPIO::sparsePinMapLength;
const IMX_PIN_INPUT_DATA* IMX_GPIO::inputSelectMap;
UINT32 IMX_GPIO::inputSelectMapLength;

NTSTATUS IMX_GPIO::GpioPullModeToImxPullMode(
    UCHAR pullConfiguration,
    _Out_ IMX_GPIO_PULL *pullMode
)
{
    switch (pullConfiguration) {
    case GPIO_PIN_PULL_CONFIGURATION_PULLUP:
        if (pullUp == IMX_GPIO_PULL_INVALID) {
            NT_ASSERT(!"Soc doesn't support GPIO pull up");
            return STATUS_NOT_SUPPORTED;
        }
        *pullMode = (IMX_GPIO_PULL)pullUp;
        break;
    case GPIO_PIN_PULL_CONFIGURATION_PULLDOWN:
        if (pullDown == IMX_GPIO_PULL_INVALID) {
            NT_ASSERT(!"Soc doesn't support GPIO pull down");
            return STATUS_NOT_SUPPORTED;
        }
        *pullMode = (IMX_GPIO_PULL)pullDown;
        break;
    case GPIO_PIN_PULL_CONFIGURATION_NONE:
        *pullMode = IMX_GPIO_PULL_DISABLE;
        break;
    case GPIO_PIN_PULL_CONFIGURATION_DEFAULT:
        *pullMode = IMX_GPIO_PULL_DEFAULT;
        break;
    default:
        NT_ASSERT(!"Invalid GPIO pullMode");
        return STATUS_INVALID_PARAMETER;
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS IMX_GPIO::configureInterrupt (
    BANK_ID BankId,
    PIN_NUMBER PinNumber,
    KINTERRUPT_MODE InterruptMode,
    KINTERRUPT_POLARITY Polarity
    )
{
    IMX_GPIO_INTERRUPT_CONFIG interruptConfig;

    switch (InterruptMode) {
    case LevelSensitive:
        switch (Polarity) {
        case InterruptActiveHigh:
            interruptConfig = IMX_GPIO_INTERRUPT_CONFIG_HIGH_LEVEL;
            break;
        case InterruptActiveLow:
            interruptConfig = IMX_GPIO_INTERRUPT_CONFIG_LOW_LEVEL;
            break;
        default:
            return STATUS_NOT_SUPPORTED;
        } // switch (Polarity)
        break;
    case Latched:
        switch (Polarity) {
        case InterruptRisingEdge:
            interruptConfig = IMX_GPIO_INTERRUPT_CONFIG_RISING_EDGE;
            break;
        case InterruptFallingEdge:
            interruptConfig = IMX_GPIO_INTERRUPT_CONFIG_FALLING_EDGE;
            break;
        case InterruptActiveBoth:
        default:
            return STATUS_NOT_SUPPORTED;
        } // switch (Polarity)
        break;
    default:
        return STATUS_NOT_SUPPORTED;
    } // switch (InterruptMode)

    IMX_GPIO_BANK_REGISTERS* const bank = gpioBankAddr[BankId];
    ULONG* bankICRSrc;
    ULONG* bankICRDst;
    ULONG shift;

    NT_ASSERT(
        (PinNumber < IMX_GPIO_PINS_PER_BANK) &&
        "Pin number expected to be in range [0..31]");

    // no need to interlock the following operations since they align to
    // the logical banks exposed to GpioClx (14 banks of 16 pins instad of
    // 7 banks of 32 pins)

    // Use ICR1 for pins [0..15]
    if (PinNumber < 16) {
        bankICRDst = &bank->InterruptConfig1;
        bankICRSrc = &banksInterruptConfig[BankId].ICR1;
    }
    // Use ICR2 for pins [16..31]
    else {
        bankICRDst = &bank->InterruptConfig2;
        bankICRSrc = &banksInterruptConfig[BankId].ICR2;
    } // iff

    shift = (PinNumber % 16) * 2;

    *bankICRSrc &= ~(IMX_GPIO_INTERRUPT_CONFIG_MASK << shift);
    *bankICRSrc |= (interruptConfig << shift);

    WRITE_REGISTER_NOFENCE_ULONG(bankICRDst, *bankICRSrc);

    LogInfo(
        "Configure Interrupt#%u: %s",
        IMX_MAKE_PIN_0(BankId, PinNumber),
        InterruptConfigToString(interruptConfig));

    return STATUS_SUCCESS;
} // IMX_GPIO::configureInterrupt (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::MaskInterrupts (
    PVOID ContextPtr,
    PGPIO_MASK_INTERRUPT_PARAMETERS MaskParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankValid(MaskParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(MaskParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
    const ULONG mask = static_cast<ULONG>(MaskParametersPtr->PinMask) << getPhysicalPinShift(MaskParametersPtr->BankId);

    volatile ULONG* const bankIMR = &thisPtr->banksInterruptConfig[bankId].IMR;

#ifdef DBG
    LogInfo(
        "PRE: IMR=%08x, ISR=%08x, 'IMR'=%08x, mask=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus),
        *bankIMR,
        mask);
#endif

    (void)InterlockedAnd(bankIMR, ~mask);

    WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptMask, *bankIMR);

#ifdef DBG
    LogInfo(
        "POST: IMR=%08x, ISR=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

    MaskParametersPtr->FailedMask = 0x0;

    return STATUS_SUCCESS;
} // IMX_GPIO::MaskInterrupts (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::UnmaskInterrupt (
    PVOID ContextPtr,
    PGPIO_ENABLE_INTERRUPT_PARAMETERS UnmaskParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankAndPinValid(UnmaskParametersPtr->BankId, UnmaskParametersPtr->PinNumber)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(UnmaskParametersPtr->BankId);
    const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(UnmaskParametersPtr->BankId, UnmaskParametersPtr->PinNumber);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
    const ULONG mask = 1 << bankPinNumber;

    volatile ULONG* const bankIMR = &thisPtr->banksInterruptConfig[bankId].IMR;

#ifdef DBG
    LogInfo(
        "PRE: IMR=%08x, ISR=%08x, 'IMR'=%08x, mask=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus),
        *bankIMR,
        mask);
#endif

    (void)InterlockedOr(bankIMR, mask);

    WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus, mask);
    WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptMask, *bankIMR);

#ifdef DBG
    LogInfo(
        "POST: IMR=%08x, ISR=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

    return STATUS_SUCCESS;
} // IMX_GPIO::UnmaskInterrupt (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::QueryActiveInterrupts (
    PVOID ContextPtr,
    PGPIO_QUERY_ACTIVE_INTERRUPTS_PARAMETERS QueryActiveParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankValid(QueryActiveParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(QueryActiveParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];

#ifdef DBG
    LogInfo(
        "PRE: IMR=%08x, ISR=%08x, EnabledMask=%08x, IMR&ISR=%0x8, EnabledMask&ISR=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus),
        static_cast<ULONG>(QueryActiveParametersPtr->EnabledMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask) & READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus),
        static_cast<ULONG>(QueryActiveParametersPtr->EnabledMask) & READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

    // need to shift output results depending on bank ID.
    QueryActiveParametersPtr->ActiveMask = static_cast<ULONG64>(
        (READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus) &
         READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask))
        >> getPhysicalPinShift(QueryActiveParametersPtr->BankId)) & 0x0FFFF;

    return STATUS_SUCCESS;
} // IMX_GPIO::QueryActiveInterrupts (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::ClearActiveInterrupts (
    PVOID ContextPtr,
    PGPIO_CLEAR_ACTIVE_INTERRUPTS_PARAMETERS ClearParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankValid(ClearParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(ClearParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
    const ULONG mask = static_cast<ULONG>(ClearParametersPtr->ClearActiveMask) << getPhysicalPinShift(ClearParametersPtr->BankId);

#ifdef DBG
    LogInfo(
        "PRE: IMR=%08x, ISR=%08x, ClearActiveMask=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus),
        mask);
#endif

    WRITE_REGISTER_NOFENCE_ULONG(
        &bank->InterruptStatus,
        mask);

#ifdef DBG
    LogInfo(
        "POST: IMR=%08x, ISR=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

    ClearParametersPtr->FailedClearMask = 0x0;

    return STATUS_SUCCESS;
} // IMX_GPIO::ClearActiveInterrupts (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::QueryEnabledInterrupts (
    PVOID ContextPtr,
    PGPIO_QUERY_ENABLED_INTERRUPTS_PARAMETERS QueryEnabledParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankValid(QueryEnabledParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(QueryEnabledParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];

    volatile ULONG* const bankIMR = &thisPtr->banksInterruptConfig[bankId].IMR;

#ifdef DBG
    LogInfo(
        "PRE: IMR=%08x, ISR=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

    QueryEnabledParametersPtr->EnabledMask =
        (*bankIMR >> getPhysicalPinShift(QueryEnabledParametersPtr->BankId)) & 0x0FFFF;

    // only used in LogInfo() calls, suppress compiler warning for unused vars in retail builds
    UNREFERENCED_PARAMETER(bank);

    return STATUS_SUCCESS;
} // IMX_GPIO::QueryEnabledInterrupts (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::ReconfigureInterrupt (
    PVOID ContextPtr,
    PGPIO_RECONFIGURE_INTERRUPTS_PARAMETERS ReconfigureParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankAndPinValid(ReconfigureParametersPtr->BankId, ReconfigureParametersPtr->PinNumber)) {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status;
    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(ReconfigureParametersPtr->BankId);
    const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(ReconfigureParametersPtr->BankId, ReconfigureParametersPtr->PinNumber);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];

#ifdef DBG
    LogInfo(
        "PRE: IMR=%08x, ISR=%08x",
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
        READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

    status = thisPtr->configureInterrupt(
        bankId,
        bankPinNumber,
        ReconfigureParametersPtr->InterruptMode,
        ReconfigureParametersPtr->Polarity);

#ifdef DBG
    if (NT_SUCCESS(status)) {
        LogInfo(
            "POST: IMR=%08x, ISR=%08x",
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
    } // if
#endif

    // only used in LogInfo() calls, suppress compiler warning for unused vars in retail builds
    UNREFERENCED_PARAMETER(bank);

    return status;
} // IMX_GPIO::ReconfigureInterrupt (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::ReadGpioPinsUsingMask (
    PVOID ContextPtr,
    PGPIO_READ_PINS_MASK_PARAMETERS ReadParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankValid(ReadParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(ReadParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];

    ULONG mask;

    if (ReadParametersPtr->Flags.WriteConfiguredPins) {
        // read both input and output values
        mask = 0xFFFFFFFF;
    }
    else {
        // mask off output pins
        mask = ~thisPtr->banksDirectionReg[bankId];
    }

    *ReadParametersPtr->PinValues = (mask & READ_REGISTER_NOFENCE_ULONG(&bank->Data)) >> getPhysicalPinShift(ReadParametersPtr->BankId) & 0x0FFFF;

    return STATUS_SUCCESS;
} // IMX_GPIO::ReadGpioPinsUsingMask (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::WriteGpioPinsUsingMask (
    PVOID ContextPtr,
    PGPIO_WRITE_PINS_MASK_PARAMETERS WriteParametersPtr
    )
{
#ifdef DBG
    LogEnter();
#endif

    if (!isBankValid(WriteParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(WriteParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
    volatile ULONG* const bankDR = thisPtr->banksDataReg + bankId;
    ULONG setMask = static_cast<ULONG>(WriteParametersPtr->SetMask << getPhysicalPinShift(WriteParametersPtr->BankId));
    ULONG clearMask = static_cast<ULONG>(WriteParametersPtr->ClearMask << getPhysicalPinShift(WriteParametersPtr->BankId));

    // update our shadow copy of the data register first
    (void)InterlockedAnd(bankDR, ~clearMask);
    (void)InterlockedOr(bankDR, setMask);

    // write the value to the IMX DR
    WRITE_REGISTER_NOFENCE_ULONG(&bank->Data, *bankDR);

    return STATUS_SUCCESS;
} // IMX_GPIO::WriteGpioPinsUsingMask (...)


// Although the CLIENT_Start/StopController callback function is called
// at IRQL = PASSIVE_LEVEL, you should not make this function pageable.
// The CLIENT_Start/StopController callback is in the critical timing path
// for restoring power to the devices in the hardware platform and, for
// performance reasons, it should not be delayed by page faults
// See MSDN CLIENT_Start/StopController Remarks
_Use_decl_annotations_
NTSTATUS IMX_GPIO::StartController (
    PVOID ContextPtr,
    BOOLEAN /*RestoreContext*/,
    WDF_POWER_DEVICE_STATE /*PreviousPowerState*/
    )
{
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);

    for (ULONG bankId = 0; bankId < bankCount; ++bankId) {
        IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];

        // Reset bank interrupts configurations
        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptMask, 0);
        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptConfig1, 0);
        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptConfig2, 0);
        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus, 0xFFFFFFFF);   // clear all interrupts
    } // for (ULONG bankId = ...)

    return STATUS_SUCCESS;
} // IMX_GPIO::StartController (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::StopController (
    PVOID ContextPtr,
    BOOLEAN /*SaveContext*/,
    WDF_POWER_DEVICE_STATE /*TargetState*/
    )
{
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    UNREFERENCED_PARAMETER(ContextPtr);

    return STATUS_SUCCESS;
} // IMX_GPIO::StopController (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::EnableInterrupt (
    PVOID ContextPtr,
    PGPIO_ENABLE_INTERRUPT_PARAMETERS EnableParametersPtr
    )
{
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    if (!isBankAndPinValid(EnableParametersPtr->BankId, EnableParametersPtr->PinNumber)) {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status;
    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(EnableParametersPtr->BankId);
    const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(EnableParametersPtr->BankId, EnableParametersPtr->PinNumber);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
    const ULONG mask = 1 << bankPinNumber;
    IMX_GPIO_PULL pullMode;
    IMX_VENDOR_DATA_ELEMENT padCtl;

    volatile ULONG* const bankIMR = &thisPtr->banksInterruptConfig[bankId].IMR;

    status = ParsePadCtlFromVendorData(EnableParametersPtr->VendorData,
                                       EnableParametersPtr->VendorDataLength,
                                       &padCtl);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = GpioPullModeToImxPullMode(EnableParametersPtr->PullConfiguration, &pullMode);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    {
        INTERRUPT_BANK_LOCK lock(ContextPtr, EnableParametersPtr->BankId);

#ifdef DBG
        LogInfo(
            "PRE: IMR=%08x, ISR=%08x, 'IMR'=%08x, mask=%08x",
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus),
            *bankIMR,
            mask);
#endif

        if (NT_SUCCESS(status)) {
            status = thisPtr->configureInterrupt(
                bankId,
                bankPinNumber,
                EnableParametersPtr->InterruptMode,
                EnableParametersPtr->Polarity);
        }

        if (!NT_SUCCESS(status)) {
            return status;
        }

        NT_ASSERT((thisPtr->openInterruptPins[bankId] & mask) == 0);

        (void)InterlockedOr(bankIMR, mask);

        // set pad config if pin is not currently in use for input or output.
        if ((thisPtr->openIoPins[bankId] & mask) == 0)
        {
            const ULONG absolutePinNumber = IMX_MAKE_PIN_0(bankId, bankPinNumber);
            volatile ULONG* const bankDirection = thisPtr->banksDirectionReg + bankId;

            // pin is currently unused, make sure it's marked as input
            (void)InterlockedAnd(bankDirection, ~mask);
            WRITE_REGISTER_NOFENCE_ULONG(
                &bank->Direction,
                *bankDirection);

            // set the VendorData PadControl if specified before setting pull mode
            if (padCtl.Tag == IMX_TAG_PADCTL) {
                status = thisPtr->setPinPadCtl(absolutePinNumber, padCtl.PadCtl);
            }

            // Set correct pull mode
            if (NT_SUCCESS(status)) {
                status = thisPtr->setPullMode(absolutePinNumber, pullMode);
            }
        }

        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus, mask);
        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptMask, *bankIMR);

#ifdef DBG
        LogInfo(
            "POST: IMR=%08x, ISR=%08x",
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

        // mark in-use flag
        (void)InterlockedOr(&thisPtr->openInterruptPins[bankId], mask);
    }

    return STATUS_SUCCESS;
} // IMX_GPIO::EnableInterrupt (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::DisableInterrupt (
    PVOID ContextPtr,
    PGPIO_DISABLE_INTERRUPT_PARAMETERS DisableParametersPtr
    )
{
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    if (!isBankAndPinValid(DisableParametersPtr->BankId, DisableParametersPtr->PinNumber)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(DisableParametersPtr->BankId);
    const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(DisableParametersPtr->BankId, DisableParametersPtr->PinNumber);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
    const ULONG mask = 1 << bankPinNumber;

    {
        INTERRUPT_BANK_LOCK lock(ContextPtr, DisableParametersPtr->BankId);

        volatile ULONG* const bankIMR = &thisPtr->banksInterruptConfig[bankId].IMR;

#ifdef DBG
        LogInfo(
            "PRE: IMR=%08x, ISR=%08x, 'IMR'=%08x, mask=%08x",
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus),
            *bankIMR,
            mask);
#endif
        NT_ASSERT((thisPtr->openInterruptPins[bankId] & mask) != 0);

        (void)InterlockedAnd(bankIMR, ~mask);

        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptMask, *bankIMR);
        WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus, mask);

        // No need to configure the disabled interrupt to some state,
        // it will be reconfigured properly next time before enabled

#ifdef DBG
        LogInfo(
            "POST: IMR=%08x, ISR=%08x",
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptMask),
            READ_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus));
#endif

        // check to see if pin is now unused, if so reset the pin to
        // hw default
        if ((thisPtr->openIoPins[bankId] & mask) == 0) {
            const ULONG absolutePinNumber = IMX_MAKE_PIN_0(bankId, bankPinNumber);

            (void)thisPtr->resetPinFunction(absolutePinNumber);

            if ((thisPtr->gpioDirectionDefaultValue[bankId] & mask) == 0) {
                (void)InterlockedAnd(&thisPtr->banksDirectionReg[bankId], ~mask);
            }
            else {
                (void)InterlockedOr(&thisPtr->banksDirectionReg[bankId], mask);
            }

            WRITE_REGISTER_NOFENCE_ULONG(
                &bank->Direction,
                thisPtr->banksDirectionReg[bankId]);
        }

        // clear in-use flag
        (void)InterlockedAnd(&thisPtr->openInterruptPins[bankId], ~mask);
    } // release lock

    return STATUS_SUCCESS;
} // IMX_GPIO::DisableInterrupt (...)

// Resets the pin pull up, muxing function and input select registers of the
// selected pin to the HW defaults. It releases ownership of any input select
// registers. It cannot ensure that the InputSelect of the default muxing value
// is set to HW default as another pin may own it and it cannot be changed
// without the current pin owning the InputSelect register.
NTSTATUS IMX_GPIO::resetPinFunction (
    ULONG AbsolutePinNumber)
{
    NT_ASSERT(AbsolutePinNumber < ARRAYSIZE(gpioAbsolutePinDataMap));

    IMX_IOMUXC_REGISTERS* const hw = iomuxcRegsPtr;
    const IMX_PIN_DATA & pinData = gpioAbsolutePinDataMap[AbsolutePinNumber];

    if (AbsolutePinNumber != pinData.PadGpioAbsolutePin) {
        LogError("No entry for pin#%u in PinDataMap", AbsolutePinNumber);
        return STATUS_NOT_SUPPORTED;
    }

    // read current pad mux setting and input select config
    const ULONG padMuxRegIndex = pinData.PadMuxByteOffset / sizeof(ULONG);
    IMX_GPIO_FUNCTION curPadMux =
        static_cast<IMX_GPIO_FUNCTION>(READ_REGISTER_NOFENCE_ULONG(hw->Reg + padMuxRegIndex) & IMX_GPIO_FUNCTION_MASK);

    const IMX_PIN_INPUT_DATA *curInpSel = findPinAltInputSetting(AbsolutePinNumber, curPadMux);
    ULONG curInpSelRegIndex = curInpSel == nullptr ? 0 :
         curInpSel->PadSelInpByteOffset / sizeof(ULONG);

    // reset the current select input register value and release ownership
    if (curInpSel != nullptr) {
        NT_ASSERT(curInpSel->PadGpioAbsolutePin == AbsolutePinNumber);

        ULONG owningPin = gpioInputSelectOwningPin[curInpSelRegIndex];
        if (owningPin == AbsolutePinNumber) {
            WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + curInpSelRegIndex, gpioInputSelectDefaultValue[curInpSelRegIndex]);
            (void)InterlockedExchange(
               reinterpret_cast<long *>(&gpioInputSelectOwningPin[curInpSelRegIndex]),
               static_cast<long>(IMX_GPIO_INVALID_PIN));
            LogInfo("Pin#%u released ownership of InputSelect %x", AbsolutePinNumber, curInpSel->PadSelInpByteOffset);
        }
        else {
            LogInfo("Pin#%u doesn't own InputSelect %x, cannot release", AbsolutePinNumber, curInpSel->PadSelInpByteOffset);
        }
    }

    // restore pad mux register
    const ULONG padMuxDefault = pinData.PadMuxDefault;
    WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + padMuxRegIndex, padMuxDefault);

    // restore pad ctl register
    const ULONG padCtlDefault = pinData.PadCtlDefault;
    const ULONG padCtlRegIndex = pinData.PadCtlByteOffset / sizeof(ULONG);
    WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + padCtlRegIndex, padCtlDefault);

    LogInfo("Reset Pin#%u", AbsolutePinNumber);

#ifdef DBG
    for (int i = 0; i < ARRAYSIZE(gpioInputSelectOwningPin); ++i) {
        // leaked a pin ownership?
        NT_ASSERT(gpioInputSelectOwningPin[i] != AbsolutePinNumber);
    }
#endif

    return STATUS_SUCCESS;

}

NTSTATUS IMX_GPIO::setPullMode (
    ULONG AbsolutePinNumber,
    IMX_GPIO_PULL PullMode
    )
{
    NT_ASSERT(AbsolutePinNumber < ARRAYSIZE(gpioAbsolutePinDataMap));

    IMX_IOMUXC_REGISTERS* const hw = iomuxcRegsPtr;
    const IMX_PIN_DATA & pinData = gpioAbsolutePinDataMap[AbsolutePinNumber];

    if (AbsolutePinNumber != pinData.PadGpioAbsolutePin) {
        LogError("No entry for pin#%u in PinDataMap", AbsolutePinNumber);
        return STATUS_NOT_SUPPORTED;
    }

    ULONG padCtlRegIndex = pinData.PadCtlByteOffset / sizeof(ULONG);

    // set pad control register
    if (PullMode == IMX_GPIO_PULL_DEFAULT) {
        PullMode = static_cast<IMX_GPIO_PULL>(
            (pinData.PadCtlDefault & pullMask) >> pullShift);
    }

    ULONG newPadCtl = READ_REGISTER_NOFENCE_ULONG(hw->Reg + padCtlRegIndex);
    newPadCtl &= ~pullMask;
    newPadCtl |= (PullMode << pullShift);
    WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + padCtlRegIndex, newPadCtl);

    LogInfo("Set Pin#%u: pull mode %s", AbsolutePinNumber, PullModeToString(IMX_SOC_TYPE(cpuRev), PullMode));

    return STATUS_SUCCESS;
} // IMX_GPIO::setPullMode (...)

NTSTATUS IMX_GPIO::setPinPadCtl (
    ULONG AbsolutePinNumber,
    ULONG PadCtl
    )
{
    NT_ASSERT(AbsolutePinNumber < ARRAYSIZE(gpioAbsolutePinDataMap));

    IMX_IOMUXC_REGISTERS* const hw = iomuxcRegsPtr;
    const IMX_PIN_DATA & pinData = gpioAbsolutePinDataMap[AbsolutePinNumber];

    if (AbsolutePinNumber != pinData.PadGpioAbsolutePin) {
        LogError("No entry for pin#%u in PinDataMap", AbsolutePinNumber);
        return STATUS_NOT_SUPPORTED;
    }

    ULONG padCtlRegIndex = pinData.PadCtlByteOffset / sizeof(ULONG);

    WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + padCtlRegIndex, PadCtl);

    LogInfo("Set Pin#%u: PAD CTL 0x%x", AbsolutePinNumber, PadCtl);

    return STATUS_SUCCESS;
} // IMX_GPIO::setPinPadCtl (...)

const IMX_PIN_INPUT_DATA* IMX_GPIO::findPinAltInputSetting (
    ULONG AbsolutePinNumber,
    IMX_GPIO_FUNCTION Function)
{
    ULONG altMode;
    ULONG idx = gpioAbsolutePinDataMap[AbsolutePinNumber].PadInputTableIndex;

    // no pin input table data
    if (idx == IMX_GPIO_INVALID_PIN) {
        return nullptr;
    }

    NT_ASSERT(idx < gpioPinInputSelectTableLength);
    NT_ASSERT(gpioPinInputSelectTable[idx].PadGpioAbsolutePin == AbsolutePinNumber);

    if (Function == IMX_GPIO_FUNCTION_DEFAULT) {
        altMode = gpioAbsolutePinDataMap[AbsolutePinNumber].PadMuxDefault;
    }
    else {
        altMode = Function;
    }

    // find the correct input select table element
    while ((idx < gpioPinInputSelectTableLength) &&
           (gpioPinInputSelectTable[idx].PadGpioAbsolutePin == AbsolutePinNumber)) {
        if (altMode == gpioPinInputSelectTable[idx].PadAltModeValue) {
            return &gpioPinInputSelectTable[idx];
        }

        ++idx;
    }

    return nullptr;
}

bool IMX_GPIO::isBankValid (BANK_ID BankId)
{
    if (BankId >= (bankCount * 2)) {
        LogError("Invalid BankID");
        return false;
    }

    return true;
}

bool IMX_GPIO::isBankAndPinValid (BANK_ID BankId, PIN_NUMBER BankPinNumber)
{
    if (BankId >= (bankCount * 2)) {
        LogError("Invalid BankID");
        return false;
    }

    if (BankPinNumber >= (IMX_GPIO_PINS_PER_BANK / 2)) {
        LogError("Invalid BankPinNumber");
        return false;
    }

    ULONG absolutePin = IMX_MAKE_PIN_0(
                            getPhysicalBankId(BankId),
                            getPhysicalPinNumber(BankId, BankPinNumber));
    if (absolutePin >= pinCount) {
        LogError("Invalid PinNumber");
        return false;
    }

    return true;
}

IMX_NONPAGED_SEGMENT_END; //===================================================
IMX_PAGED_SEGMENT_BEGIN; //====================================================

_Use_decl_annotations_
NTSTATUS IMX_GPIO::ConnectIoPins (
    PVOID ContextPtr,
    PGPIO_CONNECT_IO_PINS_PARAMETERS ConnectParametersPtr
    )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    if (!isBankValid(ConnectParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    switch (ConnectParametersPtr->ConnectMode) {
    case ConnectModeInput:
    case ConnectModeOutput:
        break;
    default:
        return STATUS_NOT_SUPPORTED;
    } // switch (ConnectParametersPtr->ConnectMode)

    NTSTATUS status;
    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(ConnectParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
    IMX_GPIO_PULL pullMode;
    IMX_VENDOR_DATA_ELEMENT padCtl;

    status = ParsePadCtlFromVendorData(ConnectParametersPtr->VendorData,
                                       ConnectParametersPtr->VendorDataLength,
                                       &padCtl);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = GpioPullModeToImxPullMode(ConnectParametersPtr->PullConfiguration, &pullMode);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // check pin numbers before making any changes
    for (USHORT i = 0; i < ConnectParametersPtr->PinCount; ++i) {
        if (!isBankAndPinValid(ConnectParametersPtr->BankId, ConnectParametersPtr->PinNumberTable[i])) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    volatile ULONG* const bankDirection = thisPtr->banksDirectionReg + bankId;
    ULONG inputPins = 0;
    ULONG outputPins = 0;

    for (USHORT i = 0; i < ConnectParametersPtr->PinCount; ++i) {
        const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(ConnectParametersPtr->BankId, ConnectParametersPtr->PinNumberTable[i]);
        const ULONG absolutePinNumber = IMX_MAKE_PIN_0(bankId, bankPinNumber);

        LogInfo("Connect Pin#%u", absolutePinNumber);

        NT_ASSERT(bankPinNumber < IMX_GPIO_PINS_PER_BANK);
        NT_ASSERT(absolutePinNumber < pinCount);

        // set the VendorData PadControl first if specified
        if (padCtl.Tag == IMX_TAG_PADCTL) {
            status = thisPtr->setPinPadCtl(absolutePinNumber, padCtl.PadCtl);
        }

        // always set to GPIO function for manual PIN I/O.
        if (NT_SUCCESS(status)) {
            status = thisPtr->setPinFunction(absolutePinNumber, pullMode, IMX_GPIO_FUNCTION_ALT5);
        }

        if (NT_SUCCESS(status)) {
            if (ConnectParametersPtr->ConnectMode == ConnectModeInput) {
                // Clear pin dir bit for input dir
                inputPins |= 1 << bankPinNumber;
            } else {
                // Set pin dir bit for output dir
                outputPins |= 1 << bankPinNumber;
            } // iff
        }
        else
        {
            LogError("Cannot set pin#%u to default function. Error %!STATUS!", absolutePinNumber, status);
            for (USHORT j = 0; j < i; ++j) {
                const PIN_NUMBER bankPinId = getPhysicalPinNumber(
                   ConnectParametersPtr->BankId,
                   ConnectParametersPtr->PinNumberTable[j]);

                // Revert previously modified pin settings to its default to
                // avoid partial resource initialization
                (void)thisPtr->resetPinFunction(IMX_MAKE_PIN_0(bankId, bankPinId));
            } // for (USHORT j = ...)

            return status;
        }
    } // for (USHORT i = ...)

    // update shadow copy of the direction register before physical register
    (void)InterlockedAnd(bankDirection, ~inputPins);
    (void)InterlockedOr(bankDirection, outputPins);
    WRITE_REGISTER_NOFENCE_ULONG(
        &bank->Direction,
        *bankDirection);

    // update in-use flags
    (void)InterlockedOr(thisPtr->openIoPins + bankId, outputPins | inputPins);

    return STATUS_SUCCESS;
} // IMX_GPIO::ConnectIoPins (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::DisconnectIoPins (
    PVOID ContextPtr,
    PGPIO_DISCONNECT_IO_PINS_PARAMETERS DisconnectParametersPtr
    )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    if (!isBankValid(DisconnectParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    const bool preserveConfiguration = DisconnectParametersPtr->DisconnectFlags.PreserveConfiguration;

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(DisconnectParametersPtr->BankId);
    IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];

    volatile ULONG* const bankDirection = thisPtr->banksDirectionReg + bankId;
    ULONG inputPins = 0;
    ULONG outputPins = 0;

    // check pin numbers before making any changes
    for (USHORT i = 0; i < DisconnectParametersPtr->PinCount; ++i) {
        if (!isBankAndPinValid(DisconnectParametersPtr->BankId, DisconnectParametersPtr->PinNumberTable[i])) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    // Revert pin settings to default
    for (USHORT i = 0; i < DisconnectParametersPtr->PinCount; ++i) {
        const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(DisconnectParametersPtr->BankId, DisconnectParametersPtr->PinNumberTable[i]);
        const ULONG absolutePinNumber = IMX_MAKE_PIN_0(bankId, bankPinNumber);
        const ULONG mask = 1 << bankPinNumber;

        LogInfo("Disconnect Pin#%u", absolutePinNumber);

        if (!(preserveConfiguration || (thisPtr->openInterruptPins[bankId] & mask))) {
            (void)thisPtr->resetPinFunction(absolutePinNumber);

            // reset pin direction
            if ((thisPtr->gpioDirectionDefaultValue[bankId] & mask) == 0) {
                inputPins |= mask;
            }
            else {
                outputPins |= mask;
            }
        }
    } // for (USHORT i = ...)

    // update shadow copy of the direction register before physical register
    (void)InterlockedAnd(bankDirection, ~inputPins);
    (void)InterlockedOr(bankDirection, outputPins);
    WRITE_REGISTER_NOFENCE_ULONG(
        &bank->Direction,
        *bankDirection);

    // update in-use flags
    (void)InterlockedAnd(thisPtr->openIoPins + bankId, ~(outputPins | inputPins));

    return STATUS_SUCCESS;
} // IMX_GPIO::DisconnectIoPins (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::QueryControllerBasicInformation (
    PVOID /*ContextPtr*/,
    PCLIENT_CONTROLLER_BASIC_INFORMATION ControllerInformationPtr
    )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    ControllerInformationPtr->Version = GPIO_CONTROLLER_BASIC_INFORMATION_VERSION;
    ControllerInformationPtr->Size = static_cast<USHORT>(sizeof(*ControllerInformationPtr));
    ControllerInformationPtr->TotalPins = pinCount;

    // iMX6 has 7 banks of 32 GPIO lines and splits each bank
    // across 2 interrupts. GpioClx/RHProxy doesn't allow multiple interrupts
    // per bank. So we split each physical bank into two logical banks by halving
    // the claimed pins per bank and fixing up pin numbers on function entry.
    ControllerInformationPtr->NumberOfPinsPerBank = IMX_GPIO_PINS_PER_BANK / 2;
    ControllerInformationPtr->Flags.MemoryMappedController = TRUE;
    ControllerInformationPtr->Flags.ActiveInterruptsAutoClearOnRead = FALSE;
    ControllerInformationPtr->Flags.FormatIoRequestsAsMasks = TRUE;
    ControllerInformationPtr->Flags.DeviceIdlePowerMgmtSupported = FALSE;
    ControllerInformationPtr->Flags.BankIdlePowerMgmtSupported = FALSE;
    ControllerInformationPtr->Flags.EmulateDebouncing = TRUE;
    ControllerInformationPtr->Flags.EmulateActiveBoth = TRUE;
    ControllerInformationPtr->Flags.IndependentIoHwSupported = TRUE;

    return STATUS_SUCCESS;
} // IMX_GPIO::QueryControllerBasicInformation (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::ConnectFunctionConfigPins (
   PVOID ContextPtr,
   PGPIO_CONNECT_FUNCTION_CONFIG_PINS_PARAMETERS ConnectParametersPtr
   )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    if (!isBankValid(ConnectParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    IMX_GPIO_FUNCTION function =
        static_cast<IMX_GPIO_FUNCTION>(ConnectParametersPtr->FunctionNumber);

    switch (ConnectParametersPtr->PullConfiguration) {
    case GPIO_PIN_PULL_CONFIGURATION_PULLUP:
    case GPIO_PIN_PULL_CONFIGURATION_PULLDOWN:
    case GPIO_PIN_PULL_CONFIGURATION_DEFAULT:
    case GPIO_PIN_PULL_CONFIGURATION_NONE:
        break;

    default:
        LogError("Invalid pull configuration");
        return STATUS_NOT_SUPPORTED;
    } // switch (ConnectParametersPtr->PullConfiguration)

    NTSTATUS status;
    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    const BANK_ID bankId = getPhysicalBankId(ConnectParametersPtr->BankId);
    IMX_GPIO_PULL pullMode;
    IMX_VENDOR_DATA_ELEMENT padCtl;

    status = ParsePadCtlFromVendorData(ConnectParametersPtr->VendorData,
                                       ConnectParametersPtr->VendorDataLength,
                                       &padCtl);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    LogInfo(
        "ConnectFunctionConfigPins Bank#%u Function: %u",
        ConnectParametersPtr->BankId,
        ConnectParametersPtr->FunctionNumber);

    status = GpioPullModeToImxPullMode(ConnectParametersPtr->PullConfiguration, &pullMode);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // check pin numbers before making any changes
    for (USHORT i = 0; i < ConnectParametersPtr->PinCount; ++i) {
        if (!isBankAndPinValid(ConnectParametersPtr->BankId, ConnectParametersPtr->PinNumberTable[i])) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    // set pins to requested drive mode
    for (USHORT i = 0; i < ConnectParametersPtr->PinCount; ++i) {
        const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(ConnectParametersPtr->BankId, ConnectParametersPtr->PinNumberTable[i]);
        const ULONG absolutePinNumber = IMX_MAKE_PIN_0(bankId, bankPinNumber);

        // set the VendorData PadControl first if specified
        if (padCtl.Tag == IMX_TAG_PADCTL) {
            status = thisPtr->setPinPadCtl(absolutePinNumber, padCtl.PadCtl);
        }

        if (NT_SUCCESS(status)) {
            status = thisPtr->setPinFunction(absolutePinNumber, pullMode, function);
        }

        if (!NT_SUCCESS(status)) {
            for (USHORT j = 0; j < i; ++j) {
                const PIN_NUMBER bankPinId = getPhysicalPinNumber(
                   ConnectParametersPtr->BankId,
                   ConnectParametersPtr->PinNumberTable[j]);

                // Revert previously modified pin settings to its default to
                // avoid partial resource initialization
                (void)thisPtr->resetPinFunction(IMX_MAKE_PIN_0(bankId, bankPinId));
            } // for (USHORT j = ...)

            LogError("Pin#%u alt function not set, error %!STATUS!", absolutePinNumber, status);
            return status;
        }
    } // for (USHORT i = ...)

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS IMX_GPIO::DisconnectFunctionConfigPins (
   PVOID ContextPtr,
   PGPIO_DISCONNECT_FUNCTION_CONFIG_PINS_PARAMETERS DisconnectParametersPtr
   )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    if (!isBankValid(DisconnectParametersPtr->BankId)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);

    LogInfo(
        "DisconnectFunctionConfigPins Bank#%u",
        DisconnectParametersPtr->BankId);

    NTSTATUS status;
    const BANK_ID bankId = getPhysicalBankId(DisconnectParametersPtr->BankId);

    // check pin numbers before making any changes
    for (USHORT i = 0; i < DisconnectParametersPtr->PinCount; ++i) {
        if (!isBankAndPinValid(DisconnectParametersPtr->BankId, DisconnectParametersPtr->PinNumberTable[i])) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    // set pins to requested drive mode
    for (USHORT i = 0; i < DisconnectParametersPtr->PinCount; ++i) {
        const PIN_NUMBER bankPinNumber = getPhysicalPinNumber(DisconnectParametersPtr->BankId, DisconnectParametersPtr->PinNumberTable[i]);
        const ULONG absolutePinNumber = IMX_MAKE_PIN_0(bankId, bankPinNumber);

        status = thisPtr->resetPinFunction(absolutePinNumber);
        if (!NT_SUCCESS(status)) {
            LogError("Pin function reset error %!STATUS!", status);
            return status;
        }
    } // for (USHORT i = ...)

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS IMX_GPIO::PrepareController (
    WDFDEVICE WdfDevice,
    PVOID ContextPtr,
    WDFCMRESLIST /*ResourcesRaw*/,
    WDFCMRESLIST ResourcesTranslated
    )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    NTSTATUS status = STATUS_SUCCESS;
    ULONG memResourceCount = 0;
    ULONG interruptResourceCount = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR iomuxcRes = nullptr;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR gpioRes = nullptr;
    IMX_IOMUXC_REGISTERS* iomuxcRegsPtr = nullptr;
    VOID* gpioBankAddr[IMX_GPIO_BANKCOUNT_MAX] = {0};
    PHYSICAL_ADDRESS physAddr = {};
    ULONG iomuxcRegsLength = 0;

    // Look for single memory resource
    const ULONG resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    for (ULONG i = 0; i < resourceCount; ++i) {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR res =
            WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch (res->Type) {
        case CmResourceTypeMemory:
            // GPIO Controller
            if (memResourceCount == 0) {
                if (res->u.Memory.Length < bankCount * bankStride) {
                    status = STATUS_DEVICE_CONFIGURATION_ERROR;
                    LogError("GPIO controller memory block is smaller than %u", bankCount * bankStride);
                    goto exit;
                } // if

                gpioRes = res;
            }
            // IOMUX Controller
            else if (memResourceCount == 1) {
                if (res->u.Memory.Length < sizeof(IMX_IOMUXC_REGISTERS)) {
                    status = STATUS_DEVICE_CONFIGURATION_ERROR;
                    LogError("IOMUX controller memory block is smaller than %u", sizeof(IMX_IOMUXC_REGISTERS));
                    goto exit;
                } // if

                iomuxcRes = res;
            }
            else {
                LogError("Unexpected memory resource.");
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
                goto exit;
            } // iff

            ++memResourceCount;
            break;

        case CmResourceTypeInterrupt:
            ++interruptResourceCount;
            break;
        } // switch (res->Type)
    } // for (ULONG i = ...)

    // Sanity check ACPI resources
    if ((memResourceCount != 2) ||
        (interruptResourceCount != bankCount * 2))
            // Each bank has 2 interrupt lines for its 32 pins -
            // 1 line for the first 16 pins and 1 for the rest.
            // So this driver splits the each physical bank into
            // two logical banks.
    {
        LogError("Incorrect number of memory or interrupt resources");
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto exit;
    } // if

    NT_ASSERT(gpioRes != nullptr);
    NT_ASSERT(iomuxcRes != nullptr);

    // Map each GPIO bank into memory individually
    for (SIZE_T bankId = 0; bankId < bankCount; ++bankId) {
        physAddr.QuadPart = gpioRes->u.Memory.Start.QuadPart + (bankId * bankStride);
        gpioBankAddr[bankId] = MmMapIoSpaceEx(
            physAddr,
            IMX_GPIO_BYTES_PER_BANK,
            PAGE_READWRITE | PAGE_NOCACHE);
        if (!gpioBankAddr[bankId]) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            LogError("Could not map GPIO controller memory block");
            goto exit;
        }
    }

    iomuxcRegsPtr = static_cast<IMX_IOMUXC_REGISTERS*>(MmMapIoSpaceEx(
        iomuxcRes->u.Memory.Start,
        iomuxcRes->u.Memory.Length,
        PAGE_READWRITE | PAGE_NOCACHE));

    if (!iomuxcRegsPtr) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError("Could not map IOMUX controller memory block");
        goto exit;
    } // if

    iomuxcRegsLength = iomuxcRes->u.Memory.Length;

    if (NT_SUCCESS(status)) {
        // Use GPIO_CLX_RegisterClient allocated device context
        auto thisPtr = new (ContextPtr) IMX_GPIO(
            iomuxcRegsPtr,
            iomuxcRegsLength,
            gpioBankAddr,
            inputSelectMap,
            inputSelectMapLength);

        if (thisPtr && (thisPtr->signature != _SIGNATURE::CONSTRUCTED)) {
            status = STATUS_INTERNAL_ERROR;
            LogError("GPIO class incorrectly initialized");
            goto exit;
        }
        else
        {
            // Read the initial state of the HW Mux and Ctl registers
            for (SIZE_T i = 0; i < sparsePinMapLength; ++i) {
                IMX_PIN_DATA pinData = sparsePinMap[i];
                const ULONG padCtlRegIndex = pinData.PadCtlByteOffset / sizeof(ULONG);
                const ULONG padMuxRegIndex = pinData.PadMuxByteOffset / sizeof(ULONG);

                pinData.PadCtlDefault = READ_REGISTER_NOFENCE_ULONG(iomuxcRegsPtr->Reg + padCtlRegIndex);
                pinData.PadMuxDefault = READ_REGISTER_NOFENCE_ULONG(iomuxcRegsPtr->Reg + padMuxRegIndex);
                pinData.PadInputTableIndex = IMX_GPIO_INVALID_PIN;
                thisPtr->gpioAbsolutePinDataMap[pinData.PadGpioAbsolutePin] = pinData;
            } // for (SIZE_T i = ...)

            // Traverse GPIO pin input select table to store defaults
            // and build pin to table indices
            for (UINT32 i = 0; i < inputSelectMapLength; ++i) {
                const IMX_PIN_INPUT_DATA& inputData = inputSelectMap[i];
                const ULONG pinIndex = inputData.PadGpioAbsolutePin;
                const ULONG inpSelRegIndex = inputData.PadSelInpByteOffset / sizeof(ULONG);

                // read and store initial state of input select register
                // and clear pin assignment
                thisPtr->gpioInputSelectOwningPin[inpSelRegIndex] = IMX_GPIO_INVALID_PIN;
                thisPtr->gpioInputSelectDefaultValue[inpSelRegIndex] =
                     READ_REGISTER_NOFENCE_ULONG(iomuxcRegsPtr->Reg + inpSelRegIndex);

                // Store index of first input select entry in master table for efficient lookup
                if (thisPtr->gpioAbsolutePinDataMap[pinIndex].PadInputTableIndex == IMX_GPIO_INVALID_PIN) {
                    thisPtr->gpioAbsolutePinDataMap[pinIndex].PadInputTableIndex = i;
                }
            }

            // read initial GPIO direction state and disable interrupts
            for (UINT32 bankId = 0; bankId < bankCount; ++bankId) {
                IMX_GPIO_BANK_REGISTERS* const bank = thisPtr->gpioBankAddr[bankId];
                thisPtr->gpioDirectionDefaultValue[bankId] = READ_REGISTER_NOFENCE_ULONG(&bank->Direction);

                WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptConfig1, 0);
                WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptConfig2, 0);
                WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptMask, 0);
                WRITE_REGISTER_NOFENCE_ULONG(&bank->InterruptStatus, 0xffffffff);
            }

            thisPtr->wdfDevice = WdfDevice;
        }
    }

exit:
    // Cleanup any claimed resources on failure
    if (!NT_SUCCESS(status))
    {
        if (iomuxcRegsPtr) {
            MmUnmapIoSpace(iomuxcRegsPtr, iomuxcRegsLength);
        }

        for (ULONG bankId = 0; bankId < IMX_GPIO_BANKCOUNT_MAX; ++bankId) {
            if (gpioBankAddr[bankId]) {
                MmUnmapIoSpace(gpioBankAddr[bankId], IMX_GPIO_BYTES_PER_BANK);   
            }
        }
    }

    return status;
} // IMX_GPIO::PrepareController (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::ReleaseController (
    WDFDEVICE /*WdfDevice*/,
    PVOID ContextPtr
    )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    auto thisPtr = static_cast<IMX_GPIO*>(ContextPtr);
    //
    // GpioClx is memory managing the device context
    // Call the device context destructor only
    //
    if (thisPtr->signature == _SIGNATURE::CONSTRUCTED) {
        thisPtr->~IMX_GPIO();
    } // if

    return STATUS_SUCCESS;
} // IMX_GPIO::ReleaseController (...)

_Use_decl_annotations_
NTSTATUS IMX_GPIO::EvtDriverDeviceAdd (
    WDFDRIVER WdfDriver,
    WDFDEVICE_INIT* DeviceInitPtr
    )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES wdfDeviceAttributes;
    WDFDEVICE wdfDevice;

    status = GPIO_CLX_ProcessAddDevicePreDeviceCreate(
        WdfDriver,
        DeviceInitPtr,
        &wdfDeviceAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    } // if

    status = WdfDeviceCreate(
        &DeviceInitPtr,
        &wdfDeviceAttributes,
        &wdfDevice);

    switch (status) {
    case STATUS_SUCCESS:
        break;

    case STATUS_INSUFFICIENT_RESOURCES:
        return status;

    default:
        NT_ASSERT(!"Incorrect usage of WdfDeviceCreate");
        return STATUS_INTERNAL_ERROR;
    } // switch (status)

    status = GPIO_CLX_ProcessAddDevicePostDeviceCreate(WdfDriver, wdfDevice);
    if (!NT_SUCCESS(status)) {
        return status;
    } // if

    return STATUS_SUCCESS;
} // IMX_GPIO::EvtDriverDeviceAdd (...)

_Use_decl_annotations_
VOID IMX_GPIO::EvtDriverUnload (WDFDRIVER WdfDriver)
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    LogEnter();

    PDRIVER_OBJECT DriverObject = WdfDriverWdmGetDriverObject(WdfDriver);

    GPIO_CLX_UnregisterClient(WdfDriver);

    WPP_CLEANUP(DriverObject);
} // IMX_GPIO::EvtDriverUnload (...)

// Sets the pin muxing function, input select register and pull mode for the
// passed pin.  This function will take ownership of the input select register
// for the new muxing function and release ownership of the prior input select
// register.
_Use_decl_annotations_
NTSTATUS IMX_GPIO::setPinFunction (
    ULONG AbsolutePinNumber,
    IMX_GPIO_PULL PullMode,
    IMX_GPIO_FUNCTION Function)
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(AbsolutePinNumber < ARRAYSIZE(gpioAbsolutePinDataMap));

    NTSTATUS status;
    IMX_IOMUXC_REGISTERS* const hw = iomuxcRegsPtr;
    const IMX_PIN_DATA & pinData = gpioAbsolutePinDataMap[AbsolutePinNumber];
    ULONG owningPin = IMX_GPIO_INVALID_PIN;

    if (AbsolutePinNumber != pinData.PadGpioAbsolutePin) {
        LogError("No entry for pin#%u in PinDataMap", AbsolutePinNumber);
        return STATUS_NOT_SUPPORTED;
    }

    // read current pad mux setting and input select config
    ULONG padMuxRegIndex = pinData.PadMuxByteOffset / sizeof(ULONG);
    IMX_GPIO_FUNCTION curPadMux =
        static_cast<IMX_GPIO_FUNCTION>(READ_REGISTER_NOFENCE_ULONG(hw->Reg + padMuxRegIndex) & IMX_GPIO_FUNCTION_MASK);

    const IMX_PIN_INPUT_DATA *curInpSel = findPinAltInputSetting(AbsolutePinNumber, curPadMux);
    ULONG curInpSelRegIndex = curInpSel == nullptr ? 0 :
         curInpSel->PadSelInpByteOffset / sizeof(ULONG);

    const IMX_PIN_INPUT_DATA *newInpSel = findPinAltInputSetting(AbsolutePinNumber, Function);
    ULONG newInpSelRegIndex = newInpSel == nullptr ? 0 :
         newInpSel->PadSelInpByteOffset / sizeof(ULONG);

    // check for contention on select input register
    if (newInpSel != nullptr) {
        NT_ASSERT(newInpSel->PadGpioAbsolutePin == AbsolutePinNumber);

        owningPin = InterlockedCompareExchange(
                reinterpret_cast<long *>(&gpioInputSelectOwningPin[newInpSelRegIndex]),
                static_cast<long>(AbsolutePinNumber),
                static_cast<long>(IMX_GPIO_INVALID_PIN));

        if ((owningPin != IMX_GPIO_INVALID_PIN) &&
            (owningPin != AbsolutePinNumber)) {
            LogError(
               "Pin#%u attempting to claim InputSelect %x already owned by pin#%u",
               AbsolutePinNumber,
               newInpSel->PadSelInpByteOffset,
               owningPin);

            return STATUS_DEVICE_BUSY;
        }
        LogInfo(
           "Pin#%u took ownership of InputSelect %x",
           AbsolutePinNumber,
           newInpSel->PadSelInpByteOffset);
    }

    // update pull up setting for pin
    status = setPullMode(AbsolutePinNumber, PullMode);
    if (!NT_SUCCESS(status)) {
        if (newInpSel != nullptr) {
            // on error reset pin ownership, though haven't updated
            // InputSelect yet.
            (void)InterlockedExchange(
               reinterpret_cast<long *>(&gpioInputSelectOwningPin[curInpSelRegIndex]),
               static_cast<long>(owningPin));
        }
        return status;
    }

    // reset the current select input register value
    if (curInpSel != nullptr) {
        NT_ASSERT(curInpSel->PadGpioAbsolutePin == AbsolutePinNumber);

        owningPin = gpioInputSelectOwningPin[curInpSelRegIndex];

        if ((owningPin == AbsolutePinNumber) && (curInpSel != newInpSel)) {
            // InputSelect is owned by current pin and muxing function is
            // changing. Reset and release it.
            WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + curInpSelRegIndex, gpioInputSelectDefaultValue[curInpSelRegIndex]);
            (void)InterlockedExchange(
               reinterpret_cast<long *>(&gpioInputSelectOwningPin[curInpSelRegIndex]),
               static_cast<long>(IMX_GPIO_INVALID_PIN));

            LogInfo(
               "Pin#%u released ownership of InputSelect %x",
               AbsolutePinNumber,
               curInpSel->PadSelInpByteOffset);
        }
    }

    // set select input register
    if (newInpSel != nullptr) {
        WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + newInpSelRegIndex, newInpSel->PadSelInpValue);
        LogInfo(
           "Pin#%u set InputSelect %x to %u",
           AbsolutePinNumber,
           newInpSel->PadSelInpByteOffset,
           newInpSel->PadSelInpValue);
    }

    // set pad mux register
    if (Function == IMX_GPIO_FUNCTION_DEFAULT) {
        ULONG padMuxDefault = pinData.PadMuxDefault;
        WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + padMuxRegIndex, padMuxDefault);
    }
    else {
        ULONG newMuxCtl = READ_REGISTER_NOFENCE_ULONG(hw->Reg + padMuxRegIndex);
        newMuxCtl &= ~IMX_GPIO_FUNCTION_MASK;
        newMuxCtl |= (Function & IMX_GPIO_FUNCTION_MASK);
        WRITE_REGISTER_NOFENCE_ULONG(hw->Reg + padMuxRegIndex, newMuxCtl);
    }

    LogInfo(
       "Set Pin#%u: %s function: %u",
       AbsolutePinNumber,
       PullModeToString(IMX_SOC_TYPE(cpuRev), PullMode),
       Function);

    return STATUS_SUCCESS;

}

_Use_decl_annotations_
NTSTATUS IMX_GPIO::readDeviceProperties (
    DEVICE_OBJECT* PdoPtr,
    _Out_ UINT32* SocTypePtr
    )
{
    IMX_ASSERT_LOW_IRQL();

    ACPI_EVAL_OUTPUT_BUFFER UNALIGNED* dsdBufferPtr = nullptr;
    UINT32 socType;
    NTSTATUS status;

    status = AcpiQueryDsd(PdoPtr, &dsdBufferPtr);
    if (!NT_SUCCESS(status)) {
        LogError("AcpiQueryDsd failed with error %!STATUS!", status);
        goto Cleanup;
    }

    const ACPI_METHOD_ARGUMENT UNALIGNED* devicePropertiesPkgPtr;
    status = AcpiParseDsdAsDeviceProperties(dsdBufferPtr, &devicePropertiesPkgPtr);
    if (!NT_SUCCESS(status)) {
        LogError("AcpiParseSdsAsDeviceProperties failed with error %!STATUS!", status);
        goto Cleanup;
    }

    status = AcpiDevicePropertiesQueryIntegerValue(
            devicePropertiesPkgPtr,
            "SocType",
            &socType);
    if (!NT_SUCCESS(status)) {
        LogError("AcpiDevicePropertiesQueryIntegerValue(SocType) failed with error %!STATUS!", status);
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(SocTypePtr)) {
        *SocTypePtr = socType;
    }

Cleanup:

    if (dsdBufferPtr != nullptr) {
        ExFreePoolWithTag(dsdBufferPtr, ACPI_TAG_EVAL_OUTPUT_BUFFER);
    }

    return status;
}

// Sets platform specific bank, pin, and pull information as static values in IMX_GPIO
_Use_decl_annotations_
NTSTATUS IMX_GPIO::configureCPUType (
    )
{
    PAGED_CODE();
    IMX_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    
    IMX_CPU cpuType;
    NTSTATUS status = STATUS_SUCCESS;

    status = ImxGetCpuRev(&cpuRev);
    if (!NT_SUCCESS(status)) {
        LogError("Failed to get CPU rev/type");
        return status;
    }

    cpuType = IMX_CPU_TYPE(cpuRev);

    switch (cpuType) {
    case IMX_CPU_MX6ULL:
        bankStride = IMX6ULL_GPIO_BANK_STRIDE;
        bankCount = IMX6ULL_GPIO_BANK_COUNT;
        pinCount = IMX6ULL_GPIO_PIN_COUNT;
        pullShift = IMX6ULL_GPIO_PULL_SHIFT;
        pullMask = IMX6ULL_GPIO_PULL_MASK;
        pullUp = IMX6_GPIO_PULL_UP;
        pullDown = IMX6_GPIO_PULL_DOWN;
        sparsePinMap = Imx6ULLGpioPinDataSparseMap;
        sparsePinMapLength = ARRAYSIZE(Imx6ULLGpioPinDataSparseMap);
        inputSelectMap = Imx6ULLGpioPinInputSelectTable;
        inputSelectMapLength = ARRAYSIZE(Imx6ULLGpioPinInputSelectTable);
        break;

    case IMX_CPU_MX6D:
    case IMX_CPU_MX6DP:
    case IMX_CPU_MX6Q:
    case IMX_CPU_MX6QP:
        bankStride = IMX6_GPIO_BANK_STRIDE;
        bankCount = IMX6_GPIO_BANK_COUNT;
        pinCount = IMX6_GPIO_PIN_COUNT;
        pullShift = IMX6_GPIO_PULL_SHIFT;
        pullMask = IMX6_GPIO_PULL_MASK;
        pullUp = IMX6_GPIO_PULL_UP;
        pullDown = IMX6_GPIO_PULL_DOWN;
        sparsePinMap = Imx6DQGpioPinDataSparseMap;
        sparsePinMapLength = ARRAYSIZE(Imx6DQGpioPinDataSparseMap);
        inputSelectMap = Imx6DQGpioPinInputSelectTable;
        inputSelectMapLength = ARRAYSIZE(Imx6DQGpioPinInputSelectTable);
        break;

    case IMX_CPU_MX6SX:
        bankStride = IMX6_GPIO_BANK_STRIDE;
        bankCount = IMX6_GPIO_BANK_COUNT;
        pinCount = IMX6_GPIO_PIN_COUNT;
        pullShift = IMX6_GPIO_PULL_SHIFT;
        pullMask = IMX6_GPIO_PULL_MASK;
        pullUp = IMX6_GPIO_PULL_UP;
        pullDown = IMX6_GPIO_PULL_DOWN;
        sparsePinMap = Imx6SXGpioPinDataSparseMap;
        sparsePinMapLength = ARRAYSIZE(Imx6SXGpioPinDataSparseMap);
        inputSelectMap = Imx6SXGpioPinInputSelectTable;
        inputSelectMapLength = ARRAYSIZE(Imx6SXGpioPinInputSelectTable);
        break;

    case IMX_CPU_MX6SOLO:
    case IMX_CPU_MX6DL:
        bankStride = IMX6_GPIO_BANK_STRIDE;
        bankCount = IMX6_GPIO_BANK_COUNT;
        pinCount = IMX6_GPIO_PIN_COUNT;
        pullShift = IMX6_GPIO_PULL_SHIFT;
        pullMask = IMX6_GPIO_PULL_MASK;
        pullUp = IMX6_GPIO_PULL_UP;
        pullDown = IMX6_GPIO_PULL_DOWN;
        sparsePinMap = Imx6SDLGpioPinDataSparseMap;
        sparsePinMapLength = ARRAYSIZE(Imx6SDLGpioPinDataSparseMap);
        inputSelectMap = Imx6SDLGpioPinInputSelectTable;
        inputSelectMapLength = ARRAYSIZE(Imx6SDLGpioPinInputSelectTable);
        break;

    case IMX_CPU_MX7D:
        bankStride = IMX7_GPIO_BANK_STRIDE;
        bankCount = IMX7_GPIO_BANK_COUNT;
        pinCount = IMX7_GPIO_PIN_COUNT;
        pullShift = IMX7_GPIO_PULL_SHIFT;
        pullMask = IMX7_GPIO_PULL_MASK;
        pullUp = IMX7_GPIO_PULL_UP;
        pullDown = IMX7_GPIO_PULL_DOWN;
        sparsePinMap = Imx7DGpioPinDataSparseMap;
        sparsePinMapLength = ARRAYSIZE(Imx7DGpioPinDataSparseMap);
        inputSelectMap = Imx7DGpioPinInputSelectTable;
        inputSelectMapLength = ARRAYSIZE(Imx7DGpioPinInputSelectTable);
        break;

    case IMX_CPU_MX8MQ:
        bankStride = IMX8M_GPIO_BANK_STRIDE;
        bankCount = IMX8M_GPIO_BANK_COUNT;
        pinCount = IMX8M_GPIO_PIN_COUNT;
        pullShift = IMX8M_GPIO_PULL_SHIFT;
        pullMask = IMX8M_GPIO_PULL_MASK;
        pullUp = IMX8M_GPIO_PULL_UP;
        pullDown = (UINT32)IMX_GPIO_PULL_INVALID;  // Force invalid value. iMX8M doesn't implement pull-down
        sparsePinMap = Imx8MGpioPinDataSparseMap;
        sparsePinMapLength = ARRAYSIZE(Imx8MGpioPinDataSparseMap);
        inputSelectMap = Imx8MGpioPinInputSelectTable;
        inputSelectMapLength = ARRAYSIZE(Imx8MGpioPinInputSelectTable);
        break;

    case IMX_CPU_MX8MM:
        bankStride = IMX8MM_GPIO_BANK_STRIDE;
        bankCount = IMX8MM_GPIO_BANK_COUNT;
        pinCount = IMX8MM_GPIO_PIN_COUNT;
        pullShift = IMX8MM_GPIO_PULL_SHIFT;
        pullMask = IMX8MM_GPIO_PULL_MASK;
        pullUp = IMX8MM_GPIO_PULL_UP;
        pullDown = IMX8MM_GPIO_PULL_DOWN;
        sparsePinMap = Imx8MMiniGpioPinDataSparseMap;
        sparsePinMapLength = ARRAYSIZE(Imx8MMiniGpioPinDataSparseMap);
        inputSelectMap = Imx8MMiniGpioPinInputSelectTable;
        inputSelectMapLength = ARRAYSIZE(Imx8MMiniGpioPinInputSelectTable);
        break;

    default:
        LogError("Unsupported CPU type: 0x%x", cpuType);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    NT_ASSERT(bankStride != 0);
    NT_ASSERT(pinCount != 0);
    NT_ASSERT(bankCount != 0);
    NT_ASSERT(pullShift != 0);
    NT_ASSERT(pullMask != 0);

    return status;
}

_Use_decl_annotations_
IMX_GPIO::IMX_GPIO (
    IMX_IOMUXC_REGISTERS* IomuxcRegsPtrVal,
    ULONG IomuxcRegsLength,
    VOID* GpioBankAddr[],
    const IMX_PIN_INPUT_DATA* GpioPinInputMapVal,
    ULONG GpioPinInputLength
    ) :
    signature(_SIGNATURE::UNINITIALIZED),
    gpioPinInputSelectTable(GpioPinInputMapVal),
    gpioPinInputSelectTableLength(GpioPinInputLength),
    gpioBankAddr{},
    iomuxcRegsPtr(IomuxcRegsPtrVal),
    iomuxcRegsLength(IomuxcRegsLength),
    banksDataReg{},
    banksDirectionReg{},
    banksInterruptConfig{},
    wdfDevice(WDF_NO_HANDLE),
    openIoPins{},
    openInterruptPins{},
    gpioAbsolutePinDataMap{},
    gpioInputSelectOwningPin{},
    gpioInputSelectDefaultValue{},
    gpioDirectionDefaultValue{}
{
    PAGED_CODE();

    NT_ASSERTMSG("IMX_GPIO_REGISTERS size does not match bank count", bankCount <= ARRAYSIZE(banksDataReg));

    // Save GPIO bank mappings into the class instance
    for (SIZE_T bankId = 0; bankId < bankCount; ++bankId) {
        gpioBankAddr[bankId] = static_cast<IMX_GPIO_BANK_REGISTERS*>(GpioBankAddr[bankId]);
    }

    // capture current GPIO line state
    for (SIZE_T bankId = 0; bankId < bankCount; ++bankId) {
        IMX_GPIO_BANK_REGISTERS* const bank = gpioBankAddr[bankId];
        banksDataReg[bankId] = READ_REGISTER_NOFENCE_ULONG(&bank->Data);
        banksDirectionReg[bankId] = READ_REGISTER_NOFENCE_ULONG(&bank->Direction);
    } // for (SIZE_T i = ...)

    for (SIZE_T i = 0; i < ARRAYSIZE(gpioInputSelectOwningPin); ++i) {
        gpioInputSelectOwningPin[i] = IMX_GPIO_INVALID_PIN;
    } // for (SIZE_T i = ...)

    signature = _SIGNATURE::CONSTRUCTED;
} // IMX_GPIO::IMX_GPIO (...)

_Use_decl_annotations_
IMX_GPIO::~IMX_GPIO ()
{
    PAGED_CODE();

    NT_ASSERT(signature == _SIGNATURE::CONSTRUCTED);
    NT_ASSERT(iomuxcRegsPtr);
    NT_ASSERT(iomuxcRegsLength);

    for (ULONG bankId = 0; bankId < IMX_GPIO_BANKCOUNT_MAX; ++bankId) {
        if (gpioBankAddr[bankId]) {
            MmUnmapIoSpace(gpioBankAddr[bankId], IMX_GPIO_BYTES_PER_BANK);   
        }
        gpioBankAddr[bankId] = nullptr;
    }

    MmUnmapIoSpace(iomuxcRegsPtr, iomuxcRegsLength);
    iomuxcRegsPtr = nullptr;
    iomuxcRegsLength = 0;

    signature = _SIGNATURE::DESTRUCTED;
} // IMX_GPIO::~IMX_GPIO ()

IMX_PAGED_SEGMENT_END; //======================================================
IMX_INIT_SEGMENT_BEGIN; //=====================================================

_Use_decl_annotations_
NTSTATUS DriverEntry (
    DRIVER_OBJECT* DriverObjectPtr,
    UNICODE_STRING* RegistryPathPtr
    )
{
    PAGED_CODE();

    NTSTATUS status;

    //
    // Initialize tracing
    //
    {
        WPP_INIT_TRACING(DriverObjectPtr, RegistryPathPtr);

        RECORDER_CONFIGURE_PARAMS recorderConfigureParams;
        RECORDER_CONFIGURE_PARAMS_INIT(&recorderConfigureParams);

        WppRecorderConfigure(&recorderConfigureParams);
#if DBG
        WPP_RECORDER_LEVEL_FILTER(TRACE_FUNC_INFO) = TRUE;
#endif // DBG

    } // Tracing

    WDFDRIVER wdfDriver;
    {
        WDF_OBJECT_ATTRIBUTES wdfObjectAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&wdfObjectAttributes);

        WDF_DRIVER_CONFIG wdfDriverConfig;
        WDF_DRIVER_CONFIG_INIT(
            &wdfDriverConfig,
            IMX_GPIO::EvtDriverDeviceAdd);
        wdfDriverConfig.DriverPoolTag = IMX_GPIO_ALLOC_TAG;
        wdfDriverConfig.EvtDriverUnload = IMX_GPIO::EvtDriverUnload;

        status = WdfDriverCreate(
            DriverObjectPtr,
            RegistryPathPtr,
            &wdfObjectAttributes,
            &wdfDriverConfig,
            &wdfDriver);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FUNC_INFO,
                "WdfDriverCreate failed with status %!STATUS!\n", status);

            WPP_CLEANUP(DriverObjectPtr);

            return status;
        } // if
    } // wdfDriver

    // Detect the CPU configuration to setup pin masks and strides.
    status = IMX_GPIO::configureCPUType();
    if (!NT_SUCCESS(status)) {
        WPP_CLEANUP(DriverObjectPtr);
        return status;
    }

    // Register with GpioClx
    GPIO_CLIENT_REGISTRATION_PACKET registrationPacket = {
        GPIO_CLIENT_VERSION,
        sizeof(GPIO_CLIENT_REGISTRATION_PACKET),
        0,          // Flags
        sizeof(IMX_GPIO),
        0,          // Reserved
        IMX_GPIO::PrepareController,
        IMX_GPIO::ReleaseController,
        IMX_GPIO::StartController,
        IMX_GPIO::StopController,
        IMX_GPIO::QueryControllerBasicInformation,
        nullptr,    // CLIENT_QuerySetControllerInformation
        IMX_GPIO::EnableInterrupt,
        IMX_GPIO::DisableInterrupt,
        IMX_GPIO::UnmaskInterrupt,
        IMX_GPIO::MaskInterrupts,
        IMX_GPIO::QueryActiveInterrupts,
        IMX_GPIO::ClearActiveInterrupts,
        IMX_GPIO::ConnectIoPins,
        IMX_GPIO::DisconnectIoPins,
        nullptr,    // CLIENT_ReadGpioPins
        nullptr,    // CLIENT_WriteGpioPins
        nullptr,    // CLIENT_SaveBankHardwareContext
        nullptr,    // CLIENT_RestoreBankHardwareContext
        nullptr,    // CLIENT_PreProcessControllerInterrupt
        nullptr,    // CLIENT_ControllerSpecificFunction
        IMX_GPIO::ReconfigureInterrupt,
        IMX_GPIO::QueryEnabledInterrupts,
        IMX_GPIO::ConnectFunctionConfigPins,
        IMX_GPIO::DisconnectFunctionConfigPins
    }; // registrationPacket

    registrationPacket.CLIENT_ReadGpioPinsUsingMask =
        IMX_GPIO::ReadGpioPinsUsingMask;

    registrationPacket.CLIENT_WriteGpioPinsUsingMask =
        IMX_GPIO::WriteGpioPinsUsingMask;

    status = GPIO_CLX_RegisterClient(
        wdfDriver,
        &registrationPacket,
        RegistryPathPtr);
    if (!NT_SUCCESS(status)) {
        WPP_CLEANUP(DriverObjectPtr);
        return status;
    } // if

    NT_ASSERT(status == STATUS_SUCCESS);
    return status;
} // DriverEntry (...)

IMX_INIT_SEGMENT_END; //======================================================

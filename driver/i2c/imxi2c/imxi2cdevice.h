/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imxi2cDevice.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

*/
#ifndef _IMXI2CDEVICE_H_
#define _IMXI2CDEVICE_H_

#define PERIPH_CLK_KHZ_VALUE_NAME L"Peripheral Clock kHz"
#define MODULE_CLK_KHZ_VALUE_NAME L"Module Clock kHz"
#define DELAY_BEFORE_START_us_VALUE_NAME L"Delay Before Start us"
#define USE_INTERRUPT_MODE_VALUE_NAME L"Use Interrupt Mode"
#define PARAMATER_NAME_LEN 80

typedef struct _IMX_I2C_CONFIG_DATA {
    ULONG PeripheralClock_kHz;
    ULONG ModuleClock_kHz;
    ULONG DelayBeforeStart_us;
} IMX_I2C_CONFIG_DATA;

// WDF event callbacks.

EVT_WDF_DEVICE_PREPARE_HARDWARE OnPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE OnReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY OnD0Entry;
EVT_WDF_DEVICE_D0_EXIT OnD0Exit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT OnSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP OnSelfManagedIoCleanup;

// enable I2C_IS_PEP_MANAGED macro when PEP will become fully functional on iMX platform

// #define I2C_IS_PEP_MANAGED 1

EVT_WDF_REQUEST_CANCEL OnCancel;

NTSTATUS GetI2cConfigValues(
    _In_ IMX_I2C_CONFIG_DATA* DriverI2cConfigPtr,
    _In_ WDFDEVICE Device);

// Power framework event callbacks.

__drv_functionClass(POWER_SETTING_CALLBACK)
_IRQL_requires_same_
NTSTATUS
OnMonitorPowerSettingCallback(
    _In_ LPCGUID SettingGuid,
    _In_reads_bytes_(ValueLength) PVOID Value,
    _In_ ULONG ValueLength,
    _Inout_opt_ PVOID Context);

// SPBCx event callbacks.

EVT_SPB_TARGET_CONNECT OnTargetConnect;
EVT_SPB_CONTROLLER_LOCK OnControllerLock;
EVT_SPB_CONTROLLER_UNLOCK OnControllerUnlock;
EVT_SPB_CONTROLLER_READ OnRead;
EVT_SPB_CONTROLLER_WRITE OnWrite;
EVT_SPB_CONTROLLER_SEQUENCE OnSequence;

EVT_WDF_IO_IN_CALLER_CONTEXT OnOtherInCallerContext;
EVT_SPB_CONTROLLER_OTHER OnOther;

// PBC function prototypes.

NTSTATUS
PbcTargetGetSettings(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_ PVOID ConnectionParameters,
    _Out_ PPBC_TARGET_SETTINGS SettingsPtr);

NTSTATUS
PbcRequestValidate(_In_ PPBC_REQUEST RequestPtr);

VOID
PbcRequestConfigureForNonSequence(
    _In_ WDFDEVICE SpbController,
    _In_ SPBTARGET SpbTarget,
    _In_ SPBREQUEST SpbRequest,
    _In_ size_t Length);

NTSTATUS
PbcRequestConfigureForIndex(
    _Inout_ PPBC_REQUEST RequestPtr,
    _In_ ULONG Index);

VOID
PbcRequestDoTransfer(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_ PPBC_REQUEST RequestPtr);

VOID PbcRequestComplete(_In_ PPBC_REQUEST RequestPtr);

EVT_WDF_TIMER OnDelayTimerExpired;

size_t PbcRequestGetInfoRemaining(_In_ PPBC_REQUEST RequestPtr);

NTSTATUS PbcRequestGetByte(
    _In_ PPBC_REQUEST RequestPtr,
   _In_  size_t Index,
   _Out_ UCHAR* BytePtr);

NTSTATUS PbcRequestSetByte(
    _In_ PPBC_REQUEST RequestPtr,
   _In_ size_t Index,
   _In_ UCHAR Byte);

#endif // of _IMXI2CDEVICE_H_
/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imxi2cinternal.h

Abstract:

    This module contains the common internal type and function
    definitions for the SPB controller driver.

Environment:

    kernel-mode only

Revision History:

*/

#ifndef _IMXI2CINTERNAL_H_
#define _IMXI2CINTERNAL_H_

#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4480)

#define IMX_I2C_POOL_TAG ((ULONG) 'C2IS')

// Common includes.

#include <initguid.h>
#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>
#include <ntstrsafe.h>

#include "SPBCx.h"
#include "trace.h"

// Hardware definitions.

#include "wdmguid.h"
#include "imxi2c.h"

// Resource and descriptor definitions.

#include "reshub.h"

// I2C Serial peripheral bus descriptor

#pragma pack(push, 1)

// I2C Serial Bus ACPI Descriptor
// See ACPI 5.0 spec table 6-192

typedef struct _PNP_I2C_SERIAL_BUS_DESCRIPTOR {

    PNP_SERIAL_BUS_DESCRIPTOR SerialBusDescriptor;
    ULONG ConnectionSpeed;
    USHORT SlaveAddress;

    // follwed by optional Vendor Data
    // followed by PNP_IO_DESCRIPTOR_RESOURCE_NAME

} PNP_I2C_SERIAL_BUS_DESCRIPTOR, *PPNP_I2C_SERIAL_BUS_DESCRIPTOR;

#pragma pack(pop)

// See section 6.4.3.8.2 of the ACPI 5.0 specification

#define I2C_SERIAL_BUS_TYPE 0x01 // 0 - reserved, 1 - i2c, 2 - SPI, 3 - serial
#define I2C_SERIAL_BUS_SPECIFIC_FLAG_10BIT_ADDRESS 0x0001

#define I2C_SLV_BIT 0x01 // 0 - master(=initiated by controller), 1 - slave(=by device)
#define I2C_MAX_ADDRESS 0x7F

#define IMX_I2C_MIN_CONNECTION_SPEED 100000 // min supported speed is 100 kHz on iMX6 Sabre
#define IMX_I2C_MAX_CONNECTION_SPEED 400000 // max supported speed is 400 kHz

// Settings.

// Power settings.

#define MONITOR_POWER_ON 1
#define MONITOR_POWER_OFF 0

#define IDLE_TIMEOUT_MONITOR_ON 2000
#define IDLE_TIMEOUT_MONITOR_OFF 50

// Target settings.

typedef enum ADDRESS_MODE {

    AddressMode7Bit,
    AddressMode10Bit
}
ADDRESS_MODE, *PADDRESS_MODE;

typedef enum I2C_CONTR_ID {

    I2C1=1,
    I2C2,
    I2C3,
    I2C4,
    I2CUNKNOWN
}
I2C_CONTR_ID;

// physical addresses of iMX6 I2C controllers

#define IMX6I2C1PA (LONGLONG)0x00000000021A0000
#define IMX6I2C2PA (LONGLONG)0x00000000021A4000
#define IMX6I2C3PA (LONGLONG)0x00000000021A8000

// physical addresses of iMX7 I2C controllers

#define IMX7I2C1PA (LONGLONG)0x0000000030A20000
#define IMX7I2C2PA (LONGLONG)0x0000000030A30000
#define IMX7I2C3PA (LONGLONG)0x0000000030A40000
#define IMX7I2C4PA (LONGLONG)0x0000000030A50000

typedef struct PBC_TARGET_SETTINGS {

    ADDRESS_MODE AddressMode;
    USHORT Address;
    ULONG ConnectionSpeed;
}
PBC_TARGET_SETTINGS, *PPBC_TARGET_SETTINGS;

// Transfer settings.

typedef enum BUS_CONDITION {

    BusConditionFree,
    BusConditionBusy,
    BusConditionDontCare
}
BUS_CONDITION, *PBUS_CONDITION;

typedef struct PBC_TRANSFER_SETTINGS {

    // May need Update this structure to include other
    // settings needed to configure the controller
    // for a specific transfer.

    BUS_CONDITION BusCondition;
    BOOLEAN IsStart;
    BOOLEAN IsEnd;
}
PBC_TRANSFER_SETTINGS, *PPBC_TRANSFER_SETTINGS;

// Context definitions.

// Request context.

typedef struct _PBC_REQUEST_ {

    // Variables that persist for the lifetime of
    // the request. Specifically these apply to an
    // entire sequence request (not just a single transfer).

    // Handle to the SPB request.

    SPBREQUEST SpbRequest;

    // SPB request type.

    SPB_REQUEST_TYPE Type;

    // Number of transfers in sequence and
    // index of the current one.

    ULONG TransferCount;
    ULONG TransferIndex;

    // Total bytes transferred.

    size_t TotalInformation;

    // Current status of the request.

    NTSTATUS Status;
    BOOLEAN bIoComplete;

    // Variables that are reused for each transfer within
    // a [sequence] request.

    // Pointer to the transfer buffer and length.

    size_t Length;
    PMDL pMdlChain;

    // Position of the current transfer within
    // the sequence and its associated controller
    // settings.

    SPB_REQUEST_SEQUENCE_POSITION SequencePosition;
    PBC_TRANSFER_SETTINGS Settings;

    // Direction of the current transfer.

    SPB_TRANSFER_DIRECTION Direction;

    // Time in microseconds to delay before starting transfer.

    ULONG DelayInMicroseconds;

    // Interrupt flag indicating data is ready to
    // be transferred.

    ULONG DataReadyFlag;

    // Bytes read/written in the current transfer.

    size_t Information;
} PBC_REQUEST, *PPBC_REQUEST;

// Target context.

typedef struct _PBC_TARGET {

    // Handle to the SPB target.

    SPBTARGET SpbTarget;

    // Target specific settings.

    PBC_TARGET_SETTINGS Settings;

    // Current request associated with the
    // target. This value should only be non-null
    // when this target is the controller's current
    // target.

    PPBC_REQUEST CurrentRequestPtr;
} PBC_TARGET, *PPBC_TARGET;


// Device context.

typedef struct _DEVICE_CONTEXT_ {

    I2C_CONTR_ID ControllerID;

    // Handle to the WDF device.

    WDFDEVICE WdfDevice;

    // iMX registers structure address

    IMXI2C_REGISTERS* RegistersPtr;
    ULONG RegistersCb;
    PHYSICAL_ADDRESS RegistersPhysicalAddress;

    // Target that the controller is currently
    // configured for. In most cases this value is only
    // set when there is a request being handled, however,
    // it will persist between lock and unlock requests.
    // There cannot be more than one current target.

    PPBC_TARGET CurrentTargetPtr;

    // Controller driver spinlock.

    WDFSPINLOCK Lock;

    // Delay timer used to stall between transfers in microseconds

    WDFTIMER DelayTimer;

    // The power setting callback handle

    PVOID MonitorPowerSettingHandlePtr;

    ULONG ModuleClock_kHz;
    ULONG PeripheralAccessClock_kHz;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

// Declate contexts for device, target, and request.

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT,  GetDeviceContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_TARGET,  GetTargetContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_REQUEST, GetRequestContext);

#pragma warning(pop)

#endif // of _IMXI2CINTERNAL_H_
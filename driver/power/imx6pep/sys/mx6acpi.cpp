// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6acpi.cpp
//
// Abstract:
//
//   IMX6 PEP ACPI Notification Routines
//

#include "precomp.h"

#include "trace.h"
#include "mx6acpi.tmh"

#include "mx6peputil.h"
#include "mx6pepioctl.h"
#include "mx6pephw.h"
#include "mx6pep.h"

MX6_NONPAGED_SEGMENT_BEGIN; //==============================================

namespace { // static

    //
    // This table maps CSU register slave indices to MX6_PEP::_DEVICE_ID's
    // The index in the table corresponds to the _DEVICE_ID
    //
    const USHORT CsuIdxMap[] = {
        CSU_INDEX_INVALID,                      // CPU0
        CSU_INDEX_INVALID,                      // CPU1
        CSU_INDEX_INVALID,                      // CPU2
        CSU_INDEX_INVALID,                      // CPU3
        CSU_SLAVE_INDEX(1, CSU_SLAVE_HIGH),     // GPT
        CSU_SLAVE_INDEX(1, CSU_SLAVE_HIGH),     // EPIT1
        CSU_SLAVE_INDEX(11, CSU_SLAVE_HIGH),    // I2C1
        CSU_SLAVE_INDEX(12, CSU_SLAVE_LOW),     // I2C2
        CSU_SLAVE_INDEX(12, CSU_SLAVE_HIGH),    // I2C3
        CSU_SLAVE_INDEX(18, CSU_SLAVE_HIGH),    // SPI1
        CSU_SLAVE_INDEX(19, CSU_SLAVE_LOW),     // SPI2
        CSU_SLAVE_INDEX(19, CSU_SLAVE_HIGH),    // SPI3
        CSU_SLAVE_INDEX(20, CSU_SLAVE_LOW),     // SPI4
        CSU_SLAVE_INDEX(20, CSU_SLAVE_HIGH),    // SPI5
        CSU_SLAVE_INDEX(21, CSU_SLAVE_LOW),     // UART1
        CSU_SLAVE_INDEX(34, CSU_SLAVE_HIGH),    // UART2
        CSU_SLAVE_INDEX(35, CSU_SLAVE_LOW),     // UART3
        CSU_SLAVE_INDEX(35, CSU_SLAVE_HIGH),    // UART4
        CSU_SLAVE_INDEX(36, CSU_SLAVE_LOW),     // UART5
        CSU_SLAVE_INDEX(9, CSU_SLAVE_HIGH),     // USDHC1
        CSU_SLAVE_INDEX(10, CSU_SLAVE_LOW),     // USDHC2
        CSU_SLAVE_INDEX(10, CSU_SLAVE_HIGH),    // USDHC3
        CSU_SLAVE_INDEX(11, CSU_SLAVE_LOW),     // USDHC4
        CSU_SLAVE_INDEX(13, CSU_SLAVE_HIGH),    // VPU
        CSU_SLAVE_INDEX(22, CSU_SLAVE_LOW),     // SSI1
        CSU_SLAVE_INDEX(22, CSU_SLAVE_HIGH),    // SSI2
        CSU_SLAVE_INDEX(23, CSU_SLAVE_LOW),     // SSI3
        CSU_SLAVE_INDEX(23, CSU_SLAVE_HIGH),    // ASRC
        CSU_SLAVE_INDEX(8, CSU_SLAVE_LOW),      // URS0
        CSU_SLAVE_INDEX(8, CSU_SLAVE_LOW),      // USB0
        CSU_SLAVE_INDEX(8, CSU_SLAVE_LOW),      // USB1
        CSU_SLAVE_INDEX(8, CSU_SLAVE_HIGH),     // ENET
        CSU_SLAVE_INDEX(32, CSU_SLAVE_HIGH),    // GPU
        CSU_SLAVE_INDEX(32, CSU_SLAVE_LOW),     // PCI0
        CSU_INDEX_INVALID,                      // GPIO
    };
} // namespace "static"

//
// Checks CSU registers if a device is reserved for secure world only.
// Sets IsDeviceReserved flag in the specific device's device context if
// the peripheral can only be accessed by secure world
//
_Use_decl_annotations_
NTSTATUS MX6_PEP::CheckForReservedDevices()
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NTSTATUS Status;
    PHYSICAL_ADDRESS csuPhysAddress = {};
    MX6_CSU_CSL_SLAVE_REG* CsuRegMap;
    MX6_CSU_CSL_SLAVE_REG CsuReg;
    _DEVICE_CONTEXT* contextPtr;

    Status = STATUS_UNSUCCESSFUL;
    const int deviceIdCount = static_cast<int>(_DEVICE_ID::_COUNT);
    static_assert(
        ARRAYSIZE(CsuIdxMap) == deviceIdCount,
        "Verifying CsuIdxMap matches up with _DEVICE_ID");

    //
    // Map CSU Address
    //
    csuPhysAddress.QuadPart = CSU_BASE_ADDRESS;
    CsuRegMap = (MX6_CSU_CSL_SLAVE_REG*)MmMapIoSpaceEx(
        csuPhysAddress,
        CSU_SIZE,
        PAGE_READONLY | PAGE_NOCACHE
        );
    if (CsuRegMap == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    for (int i = 0; i < deviceIdCount; ++i) {
        contextPtr = this->contextFromDeviceId(static_cast<_DEVICE_ID>(i));
        if (CsuIdxMap[i] == CSU_INDEX_INVALID) {
            contextPtr->isDeviceReserved = FALSE;
            continue;
        }

        CsuReg = CsuRegMap[CsuIdxMap[i]];
        if ((CsuReg.AsUshort & CSU_NON_SECURE_MASK) == 0) {
            contextPtr->isDeviceReserved = TRUE;
        }
    }

    MmUnmapIoSpace(CsuRegMap, CSU_SIZE);
    Status = STATUS_SUCCESS;

    return Status;
}

//
// Checks if device needs to be reserved
//
_Use_decl_annotations_
BOOLEAN MX6_PEP::AcpiPrepareDevice (PEP_ACPI_PREPARE_DEVICE* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    ArgsPtr->OutputFlags = PEP_ACPI_PREPARE_DEVICE_OUTPUT_FLAG_NONE;

    _DEVICE_ID pepDeviceId = this->pepDeviceIdFromPnpDeviceId(
                                    ArgsPtr->AcpiDeviceName
                                    );
    if (pepDeviceId == _DEVICE_ID::_INVALID) {
        ArgsPtr->DeviceAccepted = FALSE;
        return TRUE;
    }

    _DEVICE_CONTEXT* contextPtr = this->contextFromDeviceId(pepDeviceId);
    ArgsPtr->DeviceAccepted = contextPtr->isDeviceReserved;

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::AcpiAbandonDevice (PEP_ACPI_ABANDON_DEVICE* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    _DEVICE_ID pepDeviceId = this->pepDeviceIdFromPnpDeviceId(
                                    ArgsPtr->AcpiDeviceName
                                    );
    if (pepDeviceId == _DEVICE_ID::_INVALID) {
        ArgsPtr->DeviceAccepted = FALSE;
        return TRUE;
    }

    _DEVICE_CONTEXT* contextPtr = this->contextFromDeviceId(pepDeviceId);
    ArgsPtr->DeviceAccepted = contextPtr->isDeviceReserved;

    return TRUE;
}

//
// N.B. No need to keep track of the Kernel handle since all devices
// will implement the same _STA=0. This would need to change if more
// ACPI methods need to be overloaded.
//
_Use_decl_annotations_
BOOLEAN MX6_PEP::AcpiRegisterDevice (PEP_ACPI_REGISTER_DEVICE* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    _DEVICE_ID pepDeviceId = this->pepDeviceIdFromPnpDeviceId(
                                    ArgsPtr->AcpiDeviceName
                                    );
    if (pepDeviceId == _DEVICE_ID::_INVALID) {
        ArgsPtr->DeviceHandle = NULL;
        return TRUE;
    }

    _DEVICE_CONTEXT* contextPtr = this->contextFromDeviceId(pepDeviceId);
    if (contextPtr->isDeviceReserved == FALSE) {
        ArgsPtr->DeviceHandle = NULL;
        return TRUE;
    }

    // Use the device ID as the handle value
    ArgsPtr->DeviceHandle = pepHandleFromDeviceId(pepDeviceId);
    ArgsPtr->OutputFlags = PEP_ACPI_REGISTER_DEVICE_OUTPUT_FLAG_NONE;

    return TRUE;

}

//
// N.B. No need to check handle since all ACPI notify callbacks
// will implement the same _STA=0
//
_Use_decl_annotations_
BOOLEAN MX6_PEP::AcpiEnumerateDeviceNamespace (
    PEP_ACPI_ENUMERATE_DEVICE_NAMESPACE* ArgsPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    ArgsPtr->ObjectCount = 1;

    //
    // Check if output buffer size is sufficient or not
    //
    if (ArgsPtr->ObjectBufferSize < sizeof(PEP_ACPI_OBJECT_NAME_WITH_TYPE)) {
        ArgsPtr->Status = STATUS_BUFFER_TOO_SMALL;
        return TRUE;
    }

    ArgsPtr->Objects[0].Name.NameAsUlong = ACPI_OBJECT_NAME_STA;
    ArgsPtr->Objects[0].Type = PepAcpiObjectTypeMethod;
    ArgsPtr->Status = STATUS_SUCCESS;

    return TRUE;
}

//
// N.B. No need to check handle since all ACPI notify callbacks
// will implement the same _STA=0
//
_Use_decl_annotations_
BOOLEAN MX6_PEP::AcpiQueryObjectInformation (
    PEP_ACPI_QUERY_OBJECT_INFORMATION* ArgsPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    if (ArgsPtr->Name.NameAsUlong == ACPI_OBJECT_NAME_STA) {
        ArgsPtr->MethodObject.InputArgumentCount = 0;
        ArgsPtr->MethodObject.OutputArgumentCount = 1;
    }

    return TRUE;
}

//
// N.B. No need to check handle since all ACPI notify callbacks will
// implement the same _STA=0. Status not implemented is equivalent to _STA=0
//
_Use_decl_annotations_
BOOLEAN MX6_PEP::AcpiEvaluateControlMethod (
    PEP_ACPI_EVALUATE_CONTROL_METHOD* ArgsPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    ArgsPtr->MethodStatus = STATUS_NOT_IMPLEMENTED;

    return TRUE;
}

MX6_NONPAGED_SEGMENT_END; //================================================

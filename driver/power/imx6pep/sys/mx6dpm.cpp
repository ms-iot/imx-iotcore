// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6dpm.cpp
//
// Abstract:
//
//   IMX6 PEP Device Power Management Routines
//

#include "precomp.h"

#include "trace.h"
#include "mx6dpm.tmh"

#include "mx6powerdef.h"
#include "mx6peputil.h"
#include "mx6pepioctl.h"
#include "mx6pephw.h"
#include "mx6pep.h"

MX6_NONPAGED_SEGMENT_BEGIN; //==============================================

namespace { // static

    //
    // This table maps PNP Device IDs to MX6_PEP::_DEVICE_ID's.
    // The index in the table corresponds to the _DEVICE_ID
    //
    const UNICODE_STRING DeviceIdMap[] = {
        RTL_CONSTANT_STRING(L"\\_SB.CPU0"),      // CPU0
        RTL_CONSTANT_STRING(L"\\_SB.CPU1"),      // CPU1
        RTL_CONSTANT_STRING(L"\\_SB.CPU2"),      // CPU2
        RTL_CONSTANT_STRING(L"\\_SB.CPU3"),      // CPU3
        RTL_CONSTANT_STRING(L"VEN_NXPI&DEV_0101&SUBDEV_0000&REV_0000&UID_00000003"), // GPT
        RTL_CONSTANT_STRING(L"VEN_NXPI&DEV_0101&SUBDEV_0000&REV_0000&UID_00000004"), // EPIT1
        RTL_CONSTANT_STRING(L"\\_SB.I2C1"),      // I2C1
        RTL_CONSTANT_STRING(L"\\_SB.I2C2"),      // I2C2
        RTL_CONSTANT_STRING(L"\\_SB.I2C3"),      // I2C3
        RTL_CONSTANT_STRING(L"\\_SB.SPI1"),      // SPI1
        RTL_CONSTANT_STRING(L"\\_SB.SPI2"),      // SPI2
        RTL_CONSTANT_STRING(L"\\_SB.SPI3"),      // SPI3
        RTL_CONSTANT_STRING(L"\\_SB.SPI4"),      // SPI4
        RTL_CONSTANT_STRING(L"\\_SB.SPI5"),      // SPI5
        RTL_CONSTANT_STRING(L"\\_SB.UAR1"),      // UART1
        RTL_CONSTANT_STRING(L"\\_SB.UAR2"),      // UART2
        RTL_CONSTANT_STRING(L"\\_SB.UAR3"),      // UART3
        RTL_CONSTANT_STRING(L"\\_SB.UAR4"),      // UART4
        RTL_CONSTANT_STRING(L"\\_SB.UAR5"),      // UART5
        RTL_CONSTANT_STRING(L"\\_SB.SDH1"),      // USDHC1
        RTL_CONSTANT_STRING(L"\\_SB.SDH2"),      // USDHC2
        RTL_CONSTANT_STRING(L"\\_SB.SDH3"),      // USDHC3
        RTL_CONSTANT_STRING(L"\\_SB.SDH4"),      // USDHC4
        RTL_CONSTANT_STRING(L"\\_SB.VPU0"),      // VPU
        RTL_CONSTANT_STRING(L"\\_SB.SSI1"),      // SSI1
        RTL_CONSTANT_STRING(L"\\_SB.SSI2"),      // SSI2
        RTL_CONSTANT_STRING(L"\\_SB.SSI3"),      // SSI3
        RTL_CONSTANT_STRING(L"\\_SB.ASRC"),      // ASRC
        RTL_CONSTANT_STRING(L"\\_SB.URS0"),      // URS0
        RTL_CONSTANT_STRING(L"\\_SB.URS0.USB0"), // USB0
        RTL_CONSTANT_STRING(L"\\_SB.USB1"),      // USB1
        RTL_CONSTANT_STRING(L"\\_SB.ENET"),      // ENET
        RTL_CONSTANT_STRING(L"\\_SB.GPU0"),      // GPU
        RTL_CONSTANT_STRING(L"\\_SB.PCI0"),      // PCI0
        RTL_CONSTANT_STRING(L"\\_SB.GPIO"),      // GPIO
    };

    struct MX6_CCGR_INDEX {
        USHORT RegisterIndex;   // Register index (0-6)
        USHORT GateNumber;      // Gate number within register (0-15)
    };

    MX6_CCGR_INDEX Mx6CcgrIndexFromClkGate (MX6_CLK_GATE ClockGate)
    {
        static const MX6_CCGR_INDEX mx6CcgrIndexMap[] = {
            {0, 0},  // MX6_AIPS_TZ1_CLK_ENABLE
            {0, 1},  // MX6_AIPS_TZ2_CLK_ENABLE
            {0, 2},  // MX6_APBHDMA_HCLK_ENABLE
            {0, 3},  // MX6_ASRC_CLK_ENABLE
            {0, 4},  // MX6_CAAM_SECURE_MEM_CLK_ENABLE
            {0, 5},  // MX6_CAAM_WRAPPER_ACLK_ENABLE
            {0, 6},  // MX6_CAAM_WRAPPER_IPG_ENABLE
            {0, 7},  // MX6_CAN1_CLK_ENABLE
            {0, 8},  // MX6_CAN1_SERIAL_CLK_ENABLE
            {0, 9},  // MX6_CAN2_CLK_ENABLE
            {0, 10}, // MX6_CAN2_SERIAL_CLK_ENABLE
            {0, 11}, // MX6_ARM_DBG_CLK_ENABLE
            {0, 12}, // MX6_DCIC1_CLK_ENABLE
            {0, 13}, // MX6_DCIC2_CLK_ENABLE
            {0, 14}, // MX6_DTCP_CLK_ENABLE
            {1, 0},  // MX6_ECSPI1_CLK_ENABLE
            {1, 1},  // MX6_ECSPI2_CLK_ENABLE
            {1, 2},  // MX6_ECSPI3_CLK_ENABLE
            {1, 3},  // MX6_ECSPI4_CLK_ENABLE
            {1, 4},  // MX6_ECSPI5_CLK_ENABLE
            {1, 5},  // MX6_ENET_CLK_ENABLE
            {1, 6},  // MX6_EPIT1_CLK_ENABLE
            {1, 7},  // MX6_EPIT2_CLK_ENABLE
            {1, 8},  // MX6_ESAI_CLK_ENABLE
            {1, 10}, // MX6_GPT_CLK_ENABLE
            {1, 11}, // MX6_GPT_SERIAL_CLK_ENABLE
            {1, 12}, // MX6_GPU2D_CLK_ENABLE
            {1, 13}, // MX6_GPU3D_CLK_ENABLE
            {2, 0},  // MX6_HDMI_TX_ENABLE
            {2, 2},  // MX6_HDMI_TX_ISFRCLK_ENABLE
            {2, 3},  // MX6_I2C1_SERIAL_CLK_ENABLE
            {2, 4},  // MX6_I2C2_SERIAL_CLK_ENABLE
            {2, 5},  // MX6_I2C3_SERIAL_CLK_ENABLE
            {2, 6},  // MX6_IIM_CLK_ENABLE
            {2, 7},  // MX6_IOMUX_IPT_CLK_IO_ENABLE
            {2, 8},  // MX6_IPMUX1_CLK_ENABLE
            {2, 9},  // MX6_IPMUX2_CLK_ENABLE
            {2, 10}, // MX6_IPMUX3_CLK_ENABLE
            {2, 11}, // MX6_IPSYNC_IP2APB_TZASC1_IPG_MASTER_CLK_ENABLE
            {2, 12}, // MX6_IPSYNC_IP2APB_TZASC2_IPG_MASTER_CLK_ENABLE
            {2, 13}, // MX6_IPSYNC_VDOA_IPG_MASTER_CLK_ENABLE
            {3, 0},  // MX6_IPU1_IPU_CLK_ENABLE
            {3, 1},  // MX6_IPU1_IPU_DI0_CLK_ENABLE
            {3, 2},  // MX6_IPU1_IPU_DI1_CLK_ENABLE
            {3, 3},  // MX6_IPU2_IPU_CLK_ENABLE
            {3, 4},  // MX6_IPU2_IPU_DI0_CLK_ENABLE
            {3, 5},  // MX6_IPU2_IPU_DI1_CLK_ENABLE
            {3, 6},  // MX6_LDB_DI0_CLK_ENABLE
            {3, 7},  // MX6_LDB_DI1_CLK_ENABLE
            {3, 8},  // MX6_MIPI_CORE_CFG_CLK_ENABLE
            {3, 9},  // MX6_MLB_CLK_ENABLE
            {3, 10}, // MX6_MMDC_CORE_ACLK_FAST_CORE_P0_ENABLE
            {3, 12}, // MX6_MMDC_CORE_IPG_CLK_P0_ENABLE
            {3, 14}, // MX6_OCRAM_CLK_ENABLE
            {3, 15}, // MX6_OPENVGAXICLK_CLK_ROOT_ENABLE
            {4, 0},  // MX6_PCIE_ROOT_ENABLE
            {4, 4},  // MX6_PL301_MX6QFAST1_S133CLK_ENABLE
            {4, 6},  // MX6_PL301_MX6QPER1_BCHCLK_ENABLE
            {4, 7},  // MX6_PL301_MX6QPER2_MAINCLK_ENABLE
            {4, 8},  // MX6_PWM1_CLK_ENABLE
            {4, 9},  // MX6_PWM2_CLK_ENABLE
            {4, 10}, // MX6_PWM3_CLK_ENABLE
            {4, 11}, // MX6_PWM4_CLK_ENABLE
            {4, 12}, // MX6_RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE
            {4, 13}, // MX6_RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE
            {4, 14}, // MX6_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE
            {4, 15}, // MX6_RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE
            {5, 0},  // MX6_ROM_CLK_ENABLE
            {5, 2},  // MX6_SATA_CLK_ENABLE
            {5, 3},  // MX6_SDMA_CLK_ENABLE
            {5, 6},  // MX6_SPBA_CLK_ENABLE
            {5, 7},  // MX6_SPDIF_CLK_ENABLE
            {5, 9},  // MX6_SSI1_CLK_ENABLE
            {5, 10}, // MX6_SSI2_CLK_ENABLE
            {5, 11}, // MX6_SSI3_CLK_ENABLE
            {5, 12}, // MX6_UART_CLK_ENABLE
            {5, 13}, // MX6_UART_SERIAL_CLK_ENABLE
            {6, 0},  // MX6_USBOH3_CLK_ENABLE
            {6, 1},  // MX6_USDHC1_CLK_ENABLE
            {6, 2},  // MX6_USDHC2_CLK_ENABLE
            {6, 3},  // MX6_USDHC3_CLK_ENABLE
            {6, 4},  // MX6_USDHC4_CLK_ENABLE
            {6, 5},  // MX6_EIM_SLOW_CLK_ENABLE
            {6, 6},  // MX6_VDOAXICLK_CLK_ENABLE
            {6, 7},  // MX6_VPU_CLK_ENABLE
        };

        static_assert(
            MX6_CLK_GATE_MAX == ARRAYSIZE(mx6CcgrIndexMap),
            "Verifying size of mx6CcgrIndexMap");

        return mx6CcgrIndexMap[ClockGate];
    }

    //
    // Currently the URS framework uses this QC specific GUID for PEP. Update
    // the GUID once a standard mechanism is established.
    // {9942B45E-2C94-41f3-A15C-C1A591C70469}
    //
    DEFINE_GUID(GUID_QCPEP_POFXCONTROL_PSTATE_REQUEST_V2,
        0x9942b45e, 0x2c94, 0x41f3, 0xa1, 0x5c, 0xc1, 0xa5, 0x91, 0xc7, 0x4, 0x69);
} // namespace "static"

//
// Clocks and powers a device so it can be accessed by the driver
//
_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmPrepareDevice (PEP_PREPARE_DEVICE* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    _DEVICE_ID pepDeviceId = this->pepDeviceIdFromPnpDeviceId(ArgsPtr->DeviceId);
    if (pepDeviceId == _DEVICE_ID::_INVALID) {
        ArgsPtr->DeviceAccepted = FALSE;
        return TRUE;
    }

    MX6_LOG_TRACE(
        "Accepting PrepareDevice request. (DeviceId = %wZ)",
        ArgsPtr->DeviceId);

    //
    // Do clock tree initialization for devices
    //
    switch (pepDeviceId) {
    case _DEVICE_ID::GPU:
        this->configureGpuClockTree();
        break;
    }

    _DEVICE_CONTEXT* contextPtr = this->contextFromDeviceId(pepDeviceId);
    contextPtr->KernelHandle = nullptr;
    contextPtr->PowerState = PowerDeviceUnspecified;

    this->setDeviceD0(pepDeviceId);

    //
    // For devices that use component level power management, power on
    // all components
    //
    switch (pepDeviceId) {
    case _DEVICE_ID::GPIO:
        this->applyEnetWorkaround();
        break;

    case _DEVICE_ID::GPU:
        this->setGpuF0();
        break;

    case _DEVICE_ID::USDHC1:
    case _DEVICE_ID::USDHC2:
    case _DEVICE_ID::USDHC3:
    case _DEVICE_ID::USDHC4:
        this->setSdhcClockGate(pepDeviceId, true);
        break;
    }

    ArgsPtr->DeviceAccepted = TRUE;
    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmAbandonDevice (PEP_ABANDON_DEVICE* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    _DEVICE_ID pepDeviceId = this->pepDeviceIdFromPnpDeviceId(ArgsPtr->DeviceId);
    if (pepDeviceId == _DEVICE_ID::_INVALID) {
        ArgsPtr->DeviceAccepted = FALSE;
        return TRUE;
    }

    MX6_LOG_TRACE(
        "Accepting AbandonDevice request and powering off device. (DeviceId = %wZ)",
        ArgsPtr->DeviceId);

    this->setDeviceDx(pepDeviceId, PowerDeviceMaximum);

    ArgsPtr->DeviceAccepted = TRUE;
    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmRegisterDevice (PEP_REGISTER_DEVICE_V2* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    _DEVICE_ID pepDeviceId = this->pepDeviceIdFromPnpDeviceId(ArgsPtr->DeviceId);
    if (pepDeviceId == _DEVICE_ID::_INVALID) {
        ArgsPtr->DeviceAccepted = PepDeviceNotAccepted;
        return TRUE;
    }

    MX6_LOG_TRACE(
        "Accepting RegisterDevice request. (DeviceId = %wZ, pepDeviceId = %d)",
        ArgsPtr->DeviceId,
        int(pepDeviceId));

    static_assert(
        sizeof(ArgsPtr->DeviceHandle) >= sizeof(pepDeviceId),
        "Verifying _DEVICE_ID can be stored in PEPHANDLE");

    _DEVICE_CONTEXT* contextPtr = this->contextFromDeviceId(pepDeviceId);
    contextPtr->KernelHandle = ArgsPtr->KernelHandle;

    // Use the device ID as the handle value
    ArgsPtr->DeviceHandle = pepHandleFromDeviceId(pepDeviceId);
    ArgsPtr->DeviceAccepted = PepDeviceAccepted;

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmUnregisterDevice (PEP_UNREGISTER_DEVICE* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);
    MX6_LOG_TRACE(
        "Unregistering device. (DeviceId = %d)",
        int(deviceId));

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmDevicePowerState (PEP_DEVICE_POWER_STATE* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);

    if (ArgsPtr->PowerState == PowerDeviceD0) {
        // D0 - put device into working state
        if (ArgsPtr->Complete != FALSE) {
            // handle pre-notification only
            return TRUE;
        }

        // The power up path is always called at PASSIVE_LEVEL
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        this->setDeviceD0(deviceId);
    } else {
        // Not D0 - put device to sleep
        if (ArgsPtr->Complete == FALSE) {
            // handle post-notification only
            return TRUE;
        }

        // The power down path can be called at DISPATCH_LEVEL
        MX6_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
        this->setDeviceDx(deviceId, ArgsPtr->PowerState);
    }

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmRegisterDebugger (PEP_REGISTER_DEBUGGER* ArgsPtr)
{
    MX6_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);

    MX6_LOG_TRACE("Registering debugger. (deviceId = %d)", int(deviceId));

    PHYSICAL_ADDRESS physAddr = { 0 };
    switch (deviceId) {
    case _DEVICE_ID::UART1:
        physAddr.LowPart = 0x02020000;
        break;
    case _DEVICE_ID::UART2:
        physAddr.LowPart = 0x021E8000;
        break;
    case _DEVICE_ID::UART3:
        physAddr.LowPart = 0x021EC000;
        break;
    case _DEVICE_ID::UART4:
        physAddr.LowPart = 0x021F0000;
        break;
    case _DEVICE_ID::UART5:
        physAddr.LowPart = 0x021F4000;
        break;
    default:
        NT_ASSERT(!"UART is only supported debugger device");
        return FALSE;
    }

    this->setUartD0(contextFromDeviceId(deviceId));

    //
    // Map base address of UART block
    //
    this->debugger.uartRegistersPtr = static_cast<IMX_UART_REGISTERS*>(
            MmMapIoSpaceEx(
                physAddr,
                0x1000,
                PAGE_READWRITE | PAGE_NOCACHE));

    if (this->debugger.uartRegistersPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map UART registers");
        return FALSE;
    }

    //
    // Initialize DPC for ISR
    //
    IoInitializeDpcRequest(this->pdoPtr, uartDebuggerDpc);

    //
    // Allocate work item to request vetos
    //
    this->debugger.toggleVetoWorkItem = IoAllocateWorkItem(this->pdoPtr);
    if (this->debugger.toggleVetoWorkItem == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to allocate work item");
        return FALSE;
    }

    //
    // Initialize DPC and timer used to clear veto
    //
    KeInitializeDpc(&this->debugger.clearVetoDpc, vetoTimerDpcRoutine, this);
    KeInitializeTimer(&this->debugger.clearVetoTimer);

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        this->debugger.connectInterruptWorkItem = IoAllocateWorkItem(this->pdoPtr);
        if (this->debugger.connectInterruptWorkItem == nullptr) {
            MX6_LOG_LOW_MEMORY("Failed to allocate work item");
            return FALSE;
        }

        IoQueueWorkItem(
            this->debugger.connectInterruptWorkItem,
            connectInterruptWorkItemRoutine,
            DelayedWorkQueue,
            this);
    } else {
        NTSTATUS status = this->connectUartInterrupt();
        if (!NT_SUCCESS(status)) {
            return FALSE;
        }
    }

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmPowerControlRequest (PEP_POWER_CONTROL_REQUEST* ArgsPtr)
{
    if (IsEqualGUID(
            *ArgsPtr->PowerControlCode,
            GUID_QCPEP_POFXCONTROL_PSTATE_REQUEST_V2)) {
        //
        // This is for VBbus control related but is not documented yet. For
        // iMX6 do not do anything and just return true. We do set
        // BytesReturned to 0x14 which is the size of QC structure
        // QCPEP_PSTATE_REQUEST_RESULT_V2. There isn't a public header or
        // standard documentation on this so hard coding for now. Update
        // this implementation once a standard implementation is defined.
        //
        RtlZeroMemory(ArgsPtr->OutBuffer, 0x14);
        ArgsPtr->BytesReturned = 0x14;
        ArgsPtr->Status = STATUS_SUCCESS;

        return TRUE;
    } else {
        MX6_LOG_TRACE(
            "Unsupported DpmPowerControlRequest. (PowerControlCode = %!GUID!)",
            ArgsPtr->PowerControlCode);

        return FALSE;
    }
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmDeviceIdleContraints (
    PEP_DEVICE_PLATFORM_CONSTRAINTS* ArgsPtr
    )
{
    NT_ASSERT(ArgsPtr->PlatformStateCount == PLATFORM_IDLE_STATE_COUNT);

    const struct {
        DEVICE_POWER_STATE States[PLATFORM_IDLE_STATE_COUNT];
    } dependencies[] = {
        // WAIT, STOP_LIGHT, ARM_OFF
        {{ PowerDeviceUnspecified }},                                   // CPU0
        {{ PowerDeviceUnspecified }},                                   // CPU1
        {{ PowerDeviceUnspecified }},                                   // CPU2
        {{ PowerDeviceUnspecified }},                                   // CPU3
        {{ PowerDeviceUnspecified }},                                   // GPT (core system resource)
        {{ PowerDeviceUnspecified }},                                   // EPIT1 (core system resource)
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // I2C1 (PERCLK_CLK_ROOT -> IPG_CLK_ROOT -> AHB_CLK_ROOT -> PERIPH_CLK -> PRE_PERIPH_CLK -> PLL2_MAIN_CLK)
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // I2C2
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // I2C3
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SPI1 (ECSPI_CLK_ROOT -> PLL3_SW_CLK -> PLL3_MAIN_CLK)
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SPI2
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SPI3
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SPI4
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SPI5
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // UART1 UART cannot transmit or receive in STOP mode
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // UART2
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // UART3
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // UART4
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1}},               // UART5
        {{ PowerDeviceUnspecified }},                                   // USDHC1 (USDHC1_CLK_ROOT -> PLL2_PFD0 -> PLL2_MAIN_CLK)
        {{ PowerDeviceUnspecified }},                                   // USDHC2
        {{ PowerDeviceUnspecified }},                                   // USDHC3
        {{ PowerDeviceUnspecified }},                                   // USDHC4
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD3 }},              // VPU (VPU_AXI_CLK_ROOT -> AXI_CLK_ROOT -> ... -> PLL2_MAIN_CLK)
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SSI1 (SSI1_CLK_ROOT -> PLL3_PFD2)
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SSI2 (SSI2_CLK_ROOT -> PLL3_PFD2)
        {{ PowerDeviceD0, PowerDeviceD0, PowerDeviceD1 }},              // SSI3 (SSI3_CLK_ROOT -> PLL3_PFD2)
        {{ PowerDeviceUnspecified }},                                   // ASRC
        {{ PowerDeviceUnspecified }},                                   // URS0
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // USB0 (IPG_CLK_ROOT -> ... -> PLL2_MAIN_CLK)
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // USB1 (IPG_CLK_ROOT -> ... -> PLL2_MAIN_CLK)
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // ENET (IPG_CLK_ROOT -> ... -> PLL2_MAIN_CLK)
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // GPU (GPU3D_CORE_CLK_ROOT -> ... -> PLL2_PFD0)
        {{ PowerDeviceD0, PowerDeviceD1, PowerDeviceD1 }},              // PCI0
        {{ PowerDeviceUnspecified }},                                   // GPIO
    };

    static_assert(
        ARRAYSIZE(dependencies) == int(_DEVICE_ID::_COUNT),
        "Verifying size of dependencies array");

    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);

    NT_ASSERT(int(deviceId) < ARRAYSIZE(dependencies));
    if (dependencies[int(deviceId)].States[0] == PowerDeviceUnspecified) {
        return FALSE;
    }

    for (ULONG i = 0; i < ArgsPtr->PlatformStateCount; ++i) {
        ArgsPtr->MinimumDStates[i] = dependencies[int(deviceId)].States[i];
    }

    return TRUE;
}

//
// The PEP writes a ULONG array to this buffer that specifies the
// lowest-powered Fx state that the component can be in for each platform idle
// state. An element with a value of 0 indicates F0, a value of 1 indicates F1,
// and so on. If the platform supports M idle states, array elements 0 to M-1
// specify the Fx states corresponding to platform idle states 0 to M-1.
//
_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmComponentIdleConstraints (
    PEP_COMPONENT_PLATFORM_CONSTRAINTS* ArgsPtr
    )
{
    NT_ASSERT(ArgsPtr->PlatformStateCount == PLATFORM_IDLE_STATE_COUNT);

    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);
    switch (deviceId) {
    case _DEVICE_ID::GPU:
    {
        switch (ArgsPtr->Component) {
        case MX6_PWRCOMPONENT_DISPLAY_IPU:

            //
            // Platform can enter WAIT when IPU is in F0
            //
            ArgsPtr->MinimumFStates[PLATFORM_IDLE_STATE_WAIT] = 0;

            //
            // IPU must be in F1 or below before system can enter STOP
            // because IPU clocks get turned off in STOP mode.
            //
            ArgsPtr->MinimumFStates[PLATFORM_IDLE_STATE_STOP_LIGHT] = 1;
            ArgsPtr->MinimumFStates[PLATFORM_IDLE_STATE_ARM_OFF] = 1;

            return TRUE;

        default:
            return FALSE;
        }
    } // _DEVICE_ID::GPU

    case _DEVICE_ID::USDHC1:
    case _DEVICE_ID::USDHC2:
    case _DEVICE_ID::USDHC3:
    case _DEVICE_ID::USDHC4:
    {
        switch (ArgsPtr->Component) {
        case 0:

            //
            // Platform can enter WAIT when SDHC is in F0
            //
            ArgsPtr->MinimumFStates[PLATFORM_IDLE_STATE_WAIT] = 0;

            //
            // SDHC must be in F1 or below before system can enter STOP
            // because PLL2 is stopped in STOP mode.
            // USDHCN_CLK_ROOT -> PLL2_PFD0 -> PLL2_MAIN_CLK
            //
            ArgsPtr->MinimumFStates[PLATFORM_IDLE_STATE_STOP_LIGHT] = 1;
            ArgsPtr->MinimumFStates[PLATFORM_IDLE_STATE_ARM_OFF] = 1;

            return TRUE;

        default:
            NT_ASSERT(!"Unexpected component!");
            return FALSE;
        }
    } // _DEVICE_ID::SDHC

    default:
        return FALSE;
    } // switch (deviceId)
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmComponentIdleState (PEP_NOTIFY_COMPONENT_IDLE_STATE* ArgsPtr)
{
    if (ArgsPtr->IdleState == 0) {
        // F0 - put component into active state
        if (ArgsPtr->DriverNotified != FALSE) {
            // handle pre-notification only
            return FALSE;
        }

        // set component to f0
        return this->setComponentF0(ArgsPtr);
    } else {
        // Fx - put component in low power state
        if (ArgsPtr->DriverNotified == FALSE) {
            // handle post-notification only
            return FALSE;
        }

        // set component to fx state
        return this->setComponentFx(ArgsPtr);
    }
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmRegisterCrashdumpDevice (PEP_REGISTER_CRASHDUMP_DEVICE* ArgsPtr)
{
    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);

    switch (deviceId) {
    case _DEVICE_ID::USDHC1:
    case _DEVICE_ID::USDHC2:
    case _DEVICE_ID::USDHC3:
    case _DEVICE_ID::USDHC4:
        break;

    default:
        return FALSE;
    }

    ArgsPtr->PowerOnDumpDeviceCallback = MX6_PEP::PowerOnCrashdumpDevice;
    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::DpmWork (PEP_WORK* ArgsPtr)
{
    LIST_ENTRY* entryPtr = ExInterlockedRemoveHeadList(
            &this->workQueue.ListHead,
            &this->workQueue.ListLock);

    //
    // A work item should have been queued if PEP_DPM_WORK was sent
    //
    NT_ASSERT(entryPtr != nullptr);

    _WORK_ITEM* workItemPtr = CONTAINING_RECORD(
            entryPtr,
            _WORK_ITEM,
            ListEntry);

    workItemPtr->WorkRoutine(workItemPtr->ContextPtr, ArgsPtr);

    ExFreeToLookasideListEx(&this->workQueue.LookasideList, workItemPtr);

    return TRUE;
}

_Use_decl_annotations_
VOID
MX6_PEP::connectInterruptWorkItemRoutine (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    PVOID ContextPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = static_cast<MX6_PEP*>(ContextPtr);
    thisPtr->connectUartInterrupt();
    IoFreeWorkItem(thisPtr->debugger.connectInterruptWorkItem);
    thisPtr->debugger.connectInterruptWorkItem = nullptr;
}

//
// Convert PNP Device Instance ID to PEP ID. If the device ID is not found,
// returns _DEVICE_ID::_INVALID
//
MX6_PEP::_DEVICE_ID MX6_PEP::pepDeviceIdFromPnpDeviceId (
    const UNICODE_STRING* DeviceIdPtr
    )
{
    const int deviceIdCount = static_cast<int>(_DEVICE_ID::_COUNT);

    static_assert(
        ARRAYSIZE(DeviceIdMap) == deviceIdCount,
        "Verifying DeviceIdMap matches up with _DEVICE_ID");

    for (int i = 0; i < deviceIdCount; ++i) {
        if (RtlEqualUnicodeString(
                DeviceIdPtr,
                &DeviceIdMap[i],
                TRUE)) {            // CaseInSensitive

            return static_cast<_DEVICE_ID>(i);
        }
    }

    return _DEVICE_ID::_INVALID;
}

//
// Put a device in the D0 (working) state. Multiple devices can be powered
// up in parallel.
//
_Use_decl_annotations_
void MX6_PEP::setDeviceD0 (_DEVICE_ID DeviceId)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6_LOG_TRACE(
        "Setting device to D0. (DeviceId = %d)",
        int(DeviceId));

    _DEVICE_CONTEXT* contextPtr = this->contextFromDeviceId(DeviceId);
    switch (DeviceId) {
    case _DEVICE_ID::I2C1:
        this->setClockGate(MX6_I2C1_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::I2C2:
        this->setClockGate(MX6_I2C2_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::I2C3:
        this->setClockGate(MX6_I2C3_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::SPI1:
        this->setClockGate(MX6_ECSPI1_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::SPI2:
        this->setClockGate(MX6_ECSPI2_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::SPI3:
        this->setClockGate(MX6_ECSPI3_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::SPI4:
        this->setClockGate(MX6_ECSPI4_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::SPI5:
        this->setClockGate(MX6_ECSPI5_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::UART1:
    case _DEVICE_ID::UART2:
    case _DEVICE_ID::UART3:
    case _DEVICE_ID::UART4:
    case _DEVICE_ID::UART5:
        this->setUartD0(contextPtr);
        return;
    case _DEVICE_ID::VPU:
    {
        // Reference the power domain if we're coming from D3 or below.
        // Must not add a duplicate power domain reference if we're
        // already in D0-D2.
        if ((contextPtr->PowerState >= PowerDeviceD3) ||
            (contextPtr->PowerState == PowerDeviceUnspecified)) {

            this->referenceGpuVpuPowerDomain();
        }

        this->setClockGate(MX6_VPU_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    }
    case _DEVICE_ID::SSI1:
        this->setClockGate(MX6_SSI1_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::SSI2:
        this->setClockGate(MX6_SSI2_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::SSI3:
        this->setClockGate(MX6_SSI3_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::ASRC:
        this->setClockGate(MX6_ASRC_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    case _DEVICE_ID::ENET:
        this->setClockGate(MX6_ENET_CLK_ENABLE, MX6_CCM_CCGR_ON);
        break;
    default:
        MX6_LOG_TRACE(
            "Taking default behavior for device. (DeviceId = %d)",
            int(DeviceId));

        break;
    }

    contextPtr->PowerState = PowerDeviceD0;
}

//
// Move device to the specified low-power (Dx) state
//
_Use_decl_annotations_
void MX6_PEP::setDeviceDx (_DEVICE_ID DeviceId, DEVICE_POWER_STATE NewPowerState)
{
    _DEVICE_CONTEXT* contextPtr = this->contextFromDeviceId(DeviceId);
    DEVICE_POWER_STATE currentPowerState = contextPtr->PowerState;

    MX6_LOG_TRACE(
        "Setting device to low power state. (DeviceId = %d, currentPowerState = %d, NewPowerState = %d)",
        int(DeviceId),
        currentPowerState,
        NewPowerState);

    NT_ASSERT(currentPowerState == PowerDeviceD0);
    NT_ASSERT(NewPowerState > PowerDeviceD0);

    switch (DeviceId) {
    case _DEVICE_ID::I2C1:
        this->setClockGate(MX6_I2C1_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::I2C2:
        this->setClockGate(MX6_I2C2_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::I2C3:
        this->setClockGate(MX6_I2C3_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::SPI1:
        this->setClockGate(MX6_ECSPI1_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::SPI2:
        this->setClockGate(MX6_ECSPI2_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::SPI3:
        this->setClockGate(MX6_ECSPI3_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::SPI4:
        this->setClockGate(MX6_ECSPI4_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::SPI5:
        this->setClockGate(MX6_ECSPI5_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::UART1:
    case _DEVICE_ID::UART2:
    case _DEVICE_ID::UART3:
    case _DEVICE_ID::UART4:
    case _DEVICE_ID::UART5:
    {
        this->unreferenceUartClocks();
        break;
    }
    case _DEVICE_ID::VPU:
    {
        this->setClockGate(MX6_VPU_CLK_ENABLE, MX6_CCM_CCGR_OFF);

        // Unreference the GPU/VPU power domain if we're going to D3 or below
        if (NewPowerState >= PowerDeviceD3) {
            this->unreferenceGpuVpuPowerDomain();
        }

        break;
    }
    case _DEVICE_ID::SSI1:
        this->setClockGate(MX6_SSI1_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::SSI2:
        this->setClockGate(MX6_SSI2_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::SSI3:
        this->setClockGate(MX6_SSI3_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::ASRC:
        this->setClockGate(MX6_ASRC_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    case _DEVICE_ID::ENET:
        this->setClockGate(MX6_ENET_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        break;
    default:
        MX6_LOG_TRACE(
            "Taking default behavior for device. (DeviceId = %d)",
            int(DeviceId));

        break;
    }

    contextPtr->PowerState = NewPowerState;
}

_Use_decl_annotations_
void MX6_PEP::setClockGate (MX6_CLK_GATE ClockGate, MX6_CCM_CCGR State)
{
    volatile MX6_CCM_REGISTERS* registersPtr = this->ccmRegistersPtr;

    MX6_CCGR_INDEX index = Mx6CcgrIndexFromClkGate(ClockGate);
    const ULONG bitOffset = 2 * index.GateNumber;

    MX6_SPINLOCK_GUARD lock(&this->ccgrRegistersSpinLock);

    ULONG ccgrValue = this->ccgrRegistersShadow[index.RegisterIndex];

    auto currentState = static_cast<MX6_CCM_CCGR>(
            (ccgrValue >> bitOffset) & 0x3);

    if (currentState == State) {
        return;
    }

    ccgrValue = (ccgrValue & ~(0x3 << bitOffset)) | (State << bitOffset);

    MX6_LOG_TRACE(
        "Setting clock gate. (ClockGate = 0x%x, State = %d)",
        ClockGate,
        State);

    this->ccgrRegistersShadow[index.RegisterIndex] = ccgrValue;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->CCGR[index.RegisterIndex],
        ccgrValue);
}

MX6_CCM_CCGR MX6_PEP::getClockGate (MX6_CLK_GATE ClockGate)
{
    volatile MX6_CCM_REGISTERS* registersPtr = this->ccmRegistersPtr;

    const MX6_CCGR_INDEX index = Mx6CcgrIndexFromClkGate(ClockGate);

    const ULONG ccgrValue = READ_REGISTER_NOFENCE_ULONG(
            &registersPtr->CCGR[index.RegisterIndex]);

    const ULONG bitOffset = 2 * index.GateNumber;
    return static_cast<MX6_CCM_CCGR>((ccgrValue >> bitOffset) & 0x3);
}

_Use_decl_annotations_
void MX6_PEP::setGpuF0WorkRoutine (PVOID ContextPtr, PEP_WORK* PepWork)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = static_cast<MX6_PEP*>(ContextPtr);

    thisPtr->setGpuF0();

    PepWork->NeedWork = TRUE;
    PepWork->WorkInformation->WorkType = PepWorkCompleteIdleState;
    PepWork->WorkInformation->CompleteIdleState.DeviceHandle =
            thisPtr->contextFromDeviceId(_DEVICE_ID::GPU)->KernelHandle;

    PepWork->WorkInformation->CompleteIdleState.Component =
            MX6_PWRCOMPONENT_DISPLAY_3DENGINE;
}

_Use_decl_annotations_
void MX6_PEP::setGpuF0 ()
{
    _GPU_ENGINE_COMPONENT_CONTEXT* contextPtr = &this->gpuEngineComponentContext;

    // Reference power domain if we were in F2 or below. We must not add a
    // duplicate power domain reference.
    if (contextPtr->IdleState >= 2) {
        this->referenceGpuVpuPowerDomain();
    }

    // Ungate GPU clocks
    this->setClockGate(MX6_GPU3D_CLK_ENABLE, MX6_CCM_CCGR_ON);
    this->setClockGate(MX6_GPU2D_CLK_ENABLE, MX6_CCM_CCGR_ON);
    this->setClockGate(MX6_OPENVGAXICLK_CLK_ROOT_ENABLE, MX6_CCM_CCGR_ON);

    contextPtr->IdleState = 0;
}

_Use_decl_annotations_
void MX6_PEP::setGpuFx (ULONG NewIdleState)
{
    _GPU_ENGINE_COMPONENT_CONTEXT* contextPtr = &this->gpuEngineComponentContext;

    NT_ASSERT(NewIdleState > 0);

    // transitions to idle states always come from F0
    NT_ASSERT(contextPtr->IdleState == 0);

    // Gate GPU clocks
    this->setClockGate(MX6_GPU3D_CLK_ENABLE, MX6_CCM_CCGR_OFF);
    this->setClockGate(MX6_GPU2D_CLK_ENABLE, MX6_CCM_CCGR_OFF);
    this->setClockGate(MX6_OPENVGAXICLK_CLK_ROOT_ENABLE, MX6_CCM_CCGR_OFF);

    // Unreference power domain if we're going to F2 or below
    if (NewIdleState >= 2) {
        this->unreferenceGpuVpuPowerDomain();
    }

    contextPtr->IdleState = NewIdleState;
}

//
// The power-up path can wait for the power down path, but not vice versa.
// The power-up path waits on gpuVpuDomainStableEvent to serialize
// power-up of multiple devices, and to wait for any currently running
// power-down requests. The power-down path resets the gpuVpuDomainStableEvent
// before powering down, and only powers down if a power-up request is
// not currently in progress. This can cause a power-down action to be
// skipped if a power up request occurred just after the InterlockedDecrement().
// In that case, unreferenceGpuVpuPowerDomain() will see the state of the event
// as not signaled and will skip the call to turnOffGpuVpuPowerDomain().
// referenceGpuVpuPowerDomain() will then increment the refcount back to 1 and
// will call turnOnGpuVpuPowerDomain() even though the power domain was
// never turned off, which is harmless.
//
_Use_decl_annotations_
void MX6_PEP::referenceGpuVpuPowerDomain ()
{
    //
    // Wait for any pending transitions, and then acquire the domain stable lock.
    //
    NTSTATUS status = KeWaitForSingleObject(
            &this->gpuVpuDomainStableEvent,
            Executive,
            KernelMode,
            FALSE,
            nullptr);

    UNREFERENCED_PARAMETER(status);
    NT_ASSERT(status == STATUS_SUCCESS);

    if (InterlockedIncrement(&this->gpuVpuDomainRefCount) == 1) {
        this->turnOnGpuVpuPowerDomain();
    }

    // Release the domain stable lock
    KeSetEvent(&this->gpuVpuDomainStableEvent, IO_NO_INCREMENT, FALSE);
}

_Use_decl_annotations_
void MX6_PEP::unreferenceGpuVpuPowerDomain ()
{
    if (InterlockedDecrement(&this->gpuVpuDomainRefCount) == 0) {

        //
        // Try to acquire the domain stable lock. Only power down
        // the domain if the lock was successfully acquired.
        //
        if (KeResetEvent(&this->gpuVpuDomainStableEvent) != 0) {
            this->turnOffGpuVpuPowerDomain();
            KeSetEvent(&this->gpuVpuDomainStableEvent, IO_NO_INCREMENT, FALSE);
        }
    }
}

_Use_decl_annotations_
void MX6_PEP::turnOnGpuVpuPowerDomain ()
{
    volatile MX6_CCM_ANALOG_REGISTERS* analogRegistersTempPtr =
        this->analogRegistersPtr;

    volatile MX6_GPC_REGISTERS *gpcRegistersTempPtr = this->gpcRegistersPtr;
    volatile MX6_GPC_PGC_REGISTERS *gpuPgcRegistersPtr =
            &gpcRegistersTempPtr->PGC_GPU;

    MX6_LOG_TRACE("Turning on the GPU/VPU power domain.");

    // Turn on LDO_PU to 1.250V
    {
        MX6_PMU_REG_CORE_REG pmuCoreReg =
            {READ_REGISTER_NOFENCE_ULONG(&analogRegistersTempPtr->PMU_REG_CORE)};

        if (pmuCoreReg.REG1_TARG != 22) {
            pmuCoreReg.REG1_TARG = 22;

            WRITE_REGISTER_NOFENCE_ULONG(
                &analogRegistersTempPtr->PMU_REG_CORE,
                pmuCoreReg.AsUlong);

            LARGE_INTEGER interval;
            interval.QuadPart = -500 * 10LL;  // 500us (arbitrary)
            NTSTATUS status = KeDelayExecutionThread(KernelMode, FALSE, &interval);
            UNREFERENCED_PARAMETER(status);
            NT_ASSERT(status == STATUS_SUCCESS);
        }
    }

    // Configure GPC/PGC PUPSCR Register SW2ISO bits
    MX6_GPC_PGC_PUPSCR_REG pupscrReg =
            {READ_REGISTER_NOFENCE_ULONG(&gpuPgcRegistersPtr->PUPSCR)};

    if ((pupscrReg.SW != MX6_GPC_PGC_PUPSCR_SW_DEFAULT) ||
        (pupscrReg.SW2ISO != MX6_GPC_PGC_PUPSCR_SW2ISO_DEFAULT)) {

        pupscrReg.SW = MX6_GPC_PGC_PUPSCR_SW_DEFAULT;
        pupscrReg.SW2ISO = MX6_GPC_PGC_PUPSCR_SW2ISO_DEFAULT;

        WRITE_REGISTER_NOFENCE_ULONG(
            &gpuPgcRegistersPtr->PUPSCR,
            pupscrReg.AsUlong);
    }

    // Assert power up request
    {
        MX6_GPC_CNTR_REG gpcCntrReg =
            {READ_REGISTER_NOFENCE_ULONG(&gpcRegistersTempPtr->CNTR)};

        gpcCntrReg.gpu_vpu_pdn_req = 0;
        gpcCntrReg.gpu_vpu_pup_req = 1;

        WRITE_REGISTER_NOFENCE_ULONG(&gpcRegistersTempPtr->CNTR, gpcCntrReg.AsUlong);

        do {
            gpcCntrReg.AsUlong =
                READ_REGISTER_NOFENCE_ULONG(&gpcRegistersTempPtr->CNTR);

        } while (gpcCntrReg.gpu_vpu_pup_req != 0);
    }
}

void MX6_PEP::turnOffGpuVpuPowerDomain ()
{
    volatile MX6_GPC_REGISTERS *gpcRegistersTempPtr = this->gpcRegistersPtr;
    volatile MX6_GPC_PGC_REGISTERS *gpuPgcRegistersPtr =
            &gpcRegistersTempPtr->PGC_GPU;

    MX6_LOG_TRACE("Turning off the GPU/VPU power domain.");

    // Configure GPC/PGC PDNSCR Register ISO bits
    MX6_GPC_PGC_PDNSCR_REG pdnscrReg =
        {READ_REGISTER_NOFENCE_ULONG(&gpuPgcRegistersPtr->PDNSCR)};

    if ((pdnscrReg.ISO != MX6_GPC_PGC_PDNSCR_ISO_DEFAULT) ||
        (pdnscrReg.ISO2SW != MX6_GPC_PGC_PDNSCR_ISO2SW_DEFAULT)) {

        pdnscrReg.ISO = MX6_GPC_PGC_PDNSCR_ISO_DEFAULT;
        pdnscrReg.ISO2SW = MX6_GPC_PGC_PDNSCR_ISO2SW_DEFAULT;

        WRITE_REGISTER_NOFENCE_ULONG(
            &gpuPgcRegistersPtr->PDNSCR,
            pdnscrReg.AsUlong);
    }

    // Configure GPC/PGC CTRL[PCR] bit to allow power down of the blocks
    {
        MX6_GPC_PGC_CTRL_REG ctrlReg =
          {READ_REGISTER_NOFENCE_ULONG(&gpuPgcRegistersPtr->CTRL)};

        ctrlReg.PCR = 1;    // enable powering down of the blocks

        WRITE_REGISTER_NOFENCE_ULONG(&gpuPgcRegistersPtr->CTRL, ctrlReg.AsUlong);
    }

    // Assert power down request
    {
        MX6_GPC_CNTR_REG gpcCntrReg =
            {READ_REGISTER_NOFENCE_ULONG(&gpcRegistersTempPtr->CNTR)};

        gpcCntrReg.gpu_vpu_pdn_req = 1;
        gpcCntrReg.gpu_vpu_pup_req = 0;

        WRITE_REGISTER_NOFENCE_ULONG(&gpcRegistersTempPtr->CNTR, gpcCntrReg.AsUlong);

        do {
            gpcCntrReg.AsUlong =
                READ_REGISTER_NOFENCE_ULONG(&gpcRegistersTempPtr->CNTR);

        } while (gpcCntrReg.gpu_vpu_pdn_req != 0);
    }

    // Turn off LDO_PU
    {
        MX6_PMU_REG_CORE_REG pmuCoreReg = {0};
        pmuCoreReg.REG1_TARG = 0x1f;

        WRITE_REGISTER_NOFENCE_ULONG(
            &analogRegistersPtr->PMU_REG_CORE_CLR,
            pmuCoreReg.AsUlong);
    }
}

void MX6_PEP::configureGpuClockTree ()
{
    volatile MX6_CCM_REGISTERS *ccmRegistersTempPtr = this->ccmRegistersPtr;

    MX6_CCM_CBCMR_REG cbcmrReg =
            {READ_REGISTER_NOFENCE_ULONG(&ccmRegistersTempPtr->CBCMR)};

    // No need to write register if clocks are already configured
    if ((cbcmrReg.gpu2d_axi_clk_sel == MX6_CCM_GPU2D_AXI_CLK_SEL_AXI) &&
        (cbcmrReg.gpu3d_axi_clk_sel == MX6_CCM_GPU3D_AXI_CLK_SEL_AXI) &&
        (cbcmrReg.gpu2d_core_clk_sel == MX6_CCM_GPU2D_CORE_CLK_SEL_PLL2_PFD0) &&
        (cbcmrReg.gpu3d_core_clk_sel == MX6_CCM_GPU3D_CORE_CLK_SEL_MMDC_CH0_AXI) &&
        (cbcmrReg.gpu3d_shader_clk_sel == MX6_CCM_GPU3D_SHADER_CLK_SEL_MMDC_CH0_AXI) &&
        (cbcmrReg.gpu2d_core_clk_podf == 0) &&
        (cbcmrReg.gpu3d_core_podf == 0) &&
        (cbcmrReg.gpu3d_shader_podf == 0)) {

        return;
    }

    // Ensure GPU clocks are gated before configuring clock tree
    this->setClockGate(MX6_GPU3D_CLK_ENABLE, MX6_CCM_CCGR_OFF);
    this->setClockGate(MX6_GPU2D_CLK_ENABLE, MX6_CCM_CCGR_OFF);
    this->setClockGate(MX6_OPENVGAXICLK_CLK_ROOT_ENABLE, MX6_CCM_CCGR_OFF);

    cbcmrReg.gpu2d_axi_clk_sel = MX6_CCM_GPU2D_AXI_CLK_SEL_AXI;
    cbcmrReg.gpu3d_axi_clk_sel = MX6_CCM_GPU3D_AXI_CLK_SEL_AXI;

    cbcmrReg.gpu2d_core_clk_sel = MX6_CCM_GPU2D_CORE_CLK_SEL_PLL2_PFD0;
    cbcmrReg.gpu3d_core_clk_sel = MX6_CCM_GPU3D_CORE_CLK_SEL_MMDC_CH0_AXI;
    cbcmrReg.gpu3d_shader_clk_sel = MX6_CCM_GPU3D_SHADER_CLK_SEL_MMDC_CH0_AXI;

    cbcmrReg.gpu2d_core_clk_podf = 0;
    cbcmrReg.gpu3d_core_podf = 0;
    cbcmrReg.gpu3d_shader_podf = 0;

    WRITE_REGISTER_NOFENCE_ULONG(&ccmRegistersTempPtr->CBCMR, cbcmrReg.AsUlong);
}

_Use_decl_annotations_
void MX6_PEP::setUartD0 (_DEVICE_CONTEXT* ContextPtr)
{
    // Only reference UART clock if device is not already in D0
    if (ContextPtr->PowerState == PowerDeviceD0) {
        return;
    }

    this->referenceUartClocks();
    ContextPtr->PowerState = PowerDeviceD0;
}

//
// Gating the UART clocks saves about 3mW in the VDD_SOC domain.
// A single clock gate controls gating for all UART instances.
//
_Use_decl_annotations_
void MX6_PEP::referenceUartClocks ()
{
    MX6_SPINLOCK_GUARD lock(&this->uartClockSpinLock);
    if (++(this->uartClockRefCount) == 1) {
        this->setClockGate(MX6_UART_CLK_ENABLE, MX6_CCM_CCGR_ON);
        this->setClockGate(MX6_UART_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_ON);
    }
}

_Use_decl_annotations_
void MX6_PEP::unreferenceUartClocks ()
{
    MX6_SPINLOCK_GUARD lock(&this->uartClockSpinLock);
    if (--(this->uartClockRefCount) == 0) {
        this->setClockGate(MX6_UART_CLK_ENABLE, MX6_CCM_CCGR_OFF);
        this->setClockGate(MX6_UART_SERIAL_CLK_ENABLE, MX6_CCM_CCGR_OFF);
    }
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::connectUartInterrupt ()
{
    NT_ASSERT(this->debugger.uartInterruptResource.u.Interrupt.Vector != 0);
    NT_ASSERT(this->debugger.uartInterruptPtr == nullptr);

    IO_CONNECT_INTERRUPT_PARAMETERS params = {};
    params.Version = CONNECT_FULLY_SPECIFIED;
    params.FullySpecified.PhysicalDeviceObject = this->pdoPtr;
    params.FullySpecified.InterruptObject = &this->debugger.uartInterruptPtr;
    params.FullySpecified.ServiceRoutine = uartDebuggerIsr;
    params.FullySpecified.ServiceContext = this;
    params.FullySpecified.SpinLock = nullptr;
    params.FullySpecified.SynchronizeIrql =
        KIRQL(this->debugger.uartInterruptResource.u.Interrupt.Level);

    params.FullySpecified.FloatingSave = FALSE;
    params.FullySpecified.ShareVector = TRUE;
    params.FullySpecified.Vector =
        this->debugger.uartInterruptResource.u.Interrupt.Vector;

    params.FullySpecified.Irql = params.FullySpecified.SynchronizeIrql;
    params.FullySpecified.InterruptMode = LevelSensitive;
    params.FullySpecified.ProcessorEnableMask =
        this->debugger.uartInterruptResource.u.Interrupt.Affinity;

    NTSTATUS status = IoConnectInterruptEx(&params);
    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "Failed to connect UART debugger interrupt. (status = %!STATUS!)",
            status);

        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::setComponentF0 (PEP_NOTIFY_COMPONENT_IDLE_STATE* ArgsPtr)
{
    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);

    NT_ASSERT(ArgsPtr->IdleState == 0);

    switch (deviceId) {
    case _DEVICE_ID::GPU:
        switch (ArgsPtr->Component) {
        case MX6_PWRCOMPONENT_DISPLAY_3DENGINE:
            this->queueWorkItem(setGpuF0WorkRoutine, this);
            ArgsPtr->Completed = FALSE;
            return TRUE;

        case MX6_PWRCOMPONENT_DISPLAY_IPU:
        case MX6_PWRCOMPONENT_DISPLAY_MONITOR:
        default:
            return FALSE;
        }

    case _DEVICE_ID::USDHC1:
    case _DEVICE_ID::USDHC2:
    case _DEVICE_ID::USDHC3:
    case _DEVICE_ID::USDHC4:
    {
        this->setSdhcClockGate(deviceId, true);
        ArgsPtr->Completed = TRUE;
        return TRUE;
    } // USDHC

    default:
        return FALSE;
    } // switch (deviceId)
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::setComponentFx (PEP_NOTIFY_COMPONENT_IDLE_STATE* ArgsPtr)
{
    _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(ArgsPtr->DeviceHandle);

    NT_ASSERT(ArgsPtr->IdleState > 0);

    switch (deviceId) {
    case _DEVICE_ID::GPU:
        switch (ArgsPtr->Component) {
        case MX6_PWRCOMPONENT_DISPLAY_3DENGINE:
            this->setGpuFx(ArgsPtr->IdleState);
            ArgsPtr->Completed = TRUE;
            return TRUE;

        case MX6_PWRCOMPONENT_DISPLAY_IPU:
        case MX6_PWRCOMPONENT_DISPLAY_MONITOR:
        default:
            return FALSE;
        }

    case _DEVICE_ID::USDHC1:
    case _DEVICE_ID::USDHC2:
    case _DEVICE_ID::USDHC3:
    case _DEVICE_ID::USDHC4:
    {
        this->setSdhcClockGate(deviceId, false);
        ArgsPtr->Completed = TRUE;
        return TRUE;
    } // USDHC

    default:
        return FALSE;
    } // switch (deviceId)
}

void MX6_PEP::setSdhcClockGate (_DEVICE_ID DeviceId, bool On)
{
    MX6_CLK_GATE gate;
    switch (DeviceId) {
    case _DEVICE_ID::USDHC1:
        gate = MX6_USDHC1_CLK_ENABLE;
        break;
    case _DEVICE_ID::USDHC2:
        gate = MX6_USDHC2_CLK_ENABLE;
        break;
    case _DEVICE_ID::USDHC3:
        gate = MX6_USDHC3_CLK_ENABLE;
        break;
    case _DEVICE_ID::USDHC4:
        gate = MX6_USDHC4_CLK_ENABLE;
        break;
    default:
        NT_ASSERT(!"Invalid DeviceId");
        return;
    }

    this->setClockGate(
        gate,
        On ? MX6_CCM_CCGR_ON : MX6_CCM_CCGR_OFF);
}

//
// ERR006687 ENET: Only the ENET wake-up interrupt request can wake the system
// from Wait mode. When the system enters Wait mode, a normal RX Done or
// TX Done does not wake up the system because the GPC cannot see this
// interrupt. This impacts performance of the ENET block because its interrupts
// are serviced only when the chip exits Wait mode due to an interrupt from
// some other wake-up source.
// Workarounds: All of the interrupts can be selected by MUX and output to pad
// GPIO6. If GPIO6 is selected to output ENET interrupts and GPIO6 SION is
// set, the resulting GPIO interrupt will wake the system from Wait mode.
//
void MX6_PEP::applyEnetWorkaround ()
{
    IMX_CPU cpuType;
    UINT32 cpuRev;
    NTSTATUS status;

    // Detect chip type and apply appropriate workaround
    status = ImxGetCpuRev(&cpuRev);
    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR("Failed to get CPU rev/type.  Could not apply Enet Workaround.");
        return;
    }
    cpuType = IMX_CPU_TYPE(cpuRev);
    switch (cpuType) {
    case IMX_CPU_MX6D:
    case IMX_CPU_MX6DP:
    case IMX_CPU_MX6Q:
    case IMX_CPU_MX6QP:
        // Force input path of GPIO6 and select ALT1
        WRITE_REGISTER_NOFENCE_ULONG(
            (ULONG *)((char *)this->iomuxcRegistersPtr +
                MX6DQ_IOMUXC_SW_MUX_CTL_PAD_GPIO06),
            MX6_IOMUXC_SW_MUX_CTL_PAD_SION | 1);

        // select ENET IRQ on GPIO6
        WRITE_REGISTER_NOFENCE_ULONG(&this->iomuxcRegistersPtr->Gpr[15], 0x9);
        break;
    case IMX_CPU_MX6SOLO:
    case IMX_CPU_MX6DL:
        // Force input path of GPIO6 and select ALT1
        WRITE_REGISTER_NOFENCE_ULONG(
            (ULONG *)((char *)this->iomuxcRegistersPtr +
                MX6SDL_IOMUXC_SW_MUX_CTL_PAD_GPIO06),
            MX6_IOMUXC_SW_MUX_CTL_PAD_SION | 1);

        // select ENET IRQ on GPIO6
        WRITE_REGISTER_NOFENCE_ULONG(&this->iomuxcRegistersPtr->Gpr[15], 0x9);
        break;
    case IMX_CPU_MX6SX:
        // ERR006687 ENET workaround not needed for SoloX.
        break;
    default:
        MX6_LOG_ERROR("Unsupported CPU type: 0x%x.  Could not apply Enet Workaround", cpuType);
        break;
    }
}

_Use_decl_annotations_
void MX6_PEP::queueWorkItem (_PWORKITEM_ROUTINE WorkRoutine, PVOID ContextPtr)
{
    auto workItemPtr = static_cast<_WORK_ITEM*>(
            ExAllocateFromLookasideListEx(&this->workQueue.LookasideList));

    //
    // Failure of ExAllocateFromLookasideListEx() is catastrophic. Let the
    // system AV if the allocation failed.
    //

    workItemPtr->WorkRoutine = WorkRoutine;
    workItemPtr->ContextPtr = ContextPtr;

    ExInterlockedInsertTailList(
        &this->workQueue.ListHead,
        &workItemPtr->ListEntry,
        &this->workQueue.ListLock);

    //
    // The OS will deliver PEP_DPM_WORK when a worker thread is available
    //
    this->pepKernelInfo.RequestWorker(this->pepKernelInfo.Plugin);
}

MX6_NONPAGED_SEGMENT_END; //================================================

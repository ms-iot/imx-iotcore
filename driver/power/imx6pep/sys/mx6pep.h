// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6pep.h
//
// Abstract:
//
//   IMX6 PEP Declarations
//

#ifndef _MX6PEP_H_
#define _MX6PEP_H_

class MX6_PEP {
public: // NONPAGED

    //
    // Devices managed by the PEP
    //
    enum class _DEVICE_ID {
        CPU0,
        CPU1,
        CPU2,
        CPU3,
        GPT,
        EPIT1,
        I2C1,
        I2C2,
        I2C3,
        SPI1,
        SPI2,
        SPI3,
        SPI4,
        SPI5,
        UART1,
        UART2,
        UART3,
        UART4,
        UART5,
        USDHC1,
        USDHC2,
        USDHC3,
        USDHC4,
        VPU,
        SSI1,
        SSI2,
        SSI3,
        ASRC,
        URS0,
        USB0,
        USB1,
        ENET,
        GPU,
        PCI0,
        GPIO,
        _COUNT,
        _INVALID = _COUNT,
    };

    enum class _COMPONENT_ID {
        GPU,
        _COUNT,
    };

    enum CPU_IDLE_STATE : ULONG {
        CPU_IDLE_STATE_WFI,
        CPU_IDLE_STATE_WFI2,
        CPU_IDLE_STATE_POWER_GATED,
        CPU_IDLE_STATE_COUNT,
    };

    enum PLATFORM_IDLE_STATE : ULONG {
        PLATFORM_IDLE_STATE_WAIT,
        PLATFORM_IDLE_STATE_STOP_LIGHT,
        PLATFORM_IDLE_STATE_ARM_OFF,
        PLATFORM_IDLE_STATE_COUNT,
    };

    enum MX6_VETO_REASON : ULONG {
        MX6_VETO_REASON_DEBUGGER = 1,
        MX6_VETO_REASON_DISABLED,
        MX6_VETO_REASON_MAX,
        MX6_VETO_REASON_COUNT = MX6_VETO_REASON_MAX - 1,
    };

    struct UNMASKED_INTERRUPTS {
        ULONG Mask[4];
    };

    static PEPCALLBACKNOTIFYACPI AcceptAcpiNotification;
    static PEPCALLBACKNOTIFYDPM AcceptDeviceNotification;
    static PEPCALLBACKNOTIFYPPM AcceptProcessorNotification;

    static PEPCALLBACKPOWERONCRASHDUMPDEVICE PowerOnCrashdumpDevice;

    //
    // PEP ACPI Notification Callbacks
    //
    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN AcpiPrepareDevice(PEP_ACPI_PREPARE_DEVICE* ArgsPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN AcpiAbandonDevice (PEP_ACPI_ABANDON_DEVICE* ArgsPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN AcpiRegisterDevice (PEP_ACPI_REGISTER_DEVICE* ArgsPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN AcpiEnumerateDeviceNamespace (
        PEP_ACPI_ENUMERATE_DEVICE_NAMESPACE* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN AcpiQueryObjectInformation (
        PEP_ACPI_QUERY_OBJECT_INFORMATION* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN AcpiEvaluateControlMethod (
        PEP_ACPI_EVALUATE_CONTROL_METHOD* ArgsPtr
        );

    //
    // PEP Device Power Management Callbacks
    //

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN DpmPrepareDevice (PEP_PREPARE_DEVICE* ArgsPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN DpmAbandonDevice (PEP_ABANDON_DEVICE* ArgsPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN DpmRegisterDevice (PEP_REGISTER_DEVICE_V2* ArgsPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN DpmUnregisterDevice (PEP_UNREGISTER_DEVICE* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN DpmDevicePowerState (PEP_DEVICE_POWER_STATE* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN DpmRegisterDebugger (PEP_REGISTER_DEBUGGER* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN DpmPowerControlRequest (PEP_POWER_CONTROL_REQUEST* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN DpmDeviceIdleContraints (PEP_DEVICE_PLATFORM_CONSTRAINTS* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN DpmComponentIdleConstraints (PEP_COMPONENT_PLATFORM_CONSTRAINTS* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN DpmComponentIdleState (PEP_NOTIFY_COMPONENT_IDLE_STATE* ArgsPtr);

    _IRQL_requires_max_(HIGH_LEVEL)
    BOOLEAN DpmRegisterCrashdumpDevice (PEP_REGISTER_CRASHDUMP_DEVICE* ArgsPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN DpmWork (PEP_WORK* ArgsPtr);

    //
    // PEP Processor Power Management Callbacks
    //

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmQueryCapabilities (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_QUERY_CAPABILITIES* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmQueryIdleStatesV2 (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_QUERY_IDLE_STATES_V2* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmQueryPlatformStates (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_QUERY_PLATFORM_STATES* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmQueryCoordinatedStates (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_QUERY_COORDINATED_STATES* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmQueryCoordinatedDependency (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_QUERY_COORDINATED_DEPENDENCY* ArgsPtr
        );

    _IRQL_requires_max_(HIGH_LEVEL)
    BOOLEAN PpmIdleExecute (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr
        );

    _IRQL_requires_max_(HIGH_LEVEL)
    BOOLEAN PpmIdleComplete (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_IDLE_COMPLETE_V2* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmQueryVetoReasons (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_QUERY_VETO_REASONS* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmQueryVetoReason (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_QUERY_VETO_REASON* ArgsPtr
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    BOOLEAN PpmEnumerateBootVetoes (PEPHANDLE Handle);

    _IRQL_requires_max_(HIGH_LEVEL)
    BOOLEAN PpmTestIdleState (
        PEPHANDLE Handle,
        _Inout_ PEP_PPM_TEST_IDLE_STATE* ArgsPtr
        );

    //
    // Nonpaged WDM Dispatch Functions
    //

    _Dispatch_type_(IRP_MJ_POWER)
    _Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
    static DRIVER_DISPATCH DispatchUnhandledIrp;

    _Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
    static DRIVER_DISPATCH DispatchDeviceIoControl;

private: // NONPAGED

    enum : ULONG { _POOL_TAG = 'PPXM' };

    struct _DEVICE_EXTENSION {
        MX6_PEP* Mx6PepPtr;
    };

    // Per-device context information
    struct _DEVICE_CONTEXT {
        POHANDLE KernelHandle;
        DEVICE_POWER_STATE PowerState;
        BOOLEAN isDeviceReserved;
    };

    typedef
    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID _WORKITEM_ROUTINE (PVOID ContextPtr, PEP_WORK* PepWork);

    typedef _WORKITEM_ROUTINE *_PWORKITEM_ROUTINE;

    struct _WORK_ITEM {
        LIST_ENTRY ListEntry;
        _PWORKITEM_ROUTINE WorkRoutine;
        PVOID ContextPtr;
    };

    static IO_WORKITEM_ROUTINE connectInterruptWorkItemRoutine;

    static _DEVICE_ID pepDeviceIdFromPnpDeviceId (
        const UNICODE_STRING* DeviceIdPtr
        );

    __forceinline static _DEVICE_ID pepDeviceIdFromPepHandle (PEPHANDLE PepHandle)
    {
        int deviceId = reinterpret_cast<int>(PepHandle);
        NT_ASSERT(
            (deviceId >= 0) && (deviceId < int(_DEVICE_ID::_COUNT)));

        return static_cast<_DEVICE_ID>(deviceId);
    }

    __forceinline static PEPHANDLE pepHandleFromDeviceId (_DEVICE_ID DeviceId)
    {
        return reinterpret_cast<PEPHANDLE>(DeviceId);
    }

    __forceinline _DEVICE_CONTEXT* contextFromDeviceId (_DEVICE_ID DeviceId)
    {
        return &this->deviceData[int(DeviceId)];
    }

    _IRQL_requires_max_(PASSIVE_LEVEL)
    void setDeviceD0 (_DEVICE_ID DeviceId);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void setDeviceDx (_DEVICE_ID DeviceId, DEVICE_POWER_STATE NewPowerState);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void setClockGate (MX6_CLK_GATE ClockGate, MX6_CCM_CCGR State);

    MX6_CCM_CCGR getClockGate (MX6_CLK_GATE ClockGate);

    static _WORKITEM_ROUTINE setGpuF0WorkRoutine;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    void setGpuF0 ();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void setGpuFx (ULONG NewIdleState);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    void referenceGpuVpuPowerDomain ();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void unreferenceGpuVpuPowerDomain ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    void turnOnGpuVpuPowerDomain ();

    void turnOffGpuVpuPowerDomain ();

    void configureGpuClockTree ();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void setUartD0 (_DEVICE_CONTEXT* ContextPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void referenceUartClocks ();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void unreferenceUartClocks ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS connectUartInterrupt ();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN setComponentF0 (PEP_NOTIFY_COMPONENT_IDLE_STATE* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN setComponentFx (PEP_NOTIFY_COMPONENT_IDLE_STATE* ArgsPtr);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void setSdhcClockGate (_DEVICE_ID DeviceId, bool On);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void applyEnetWorkaround ();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    void queueWorkItem (_PWORKITEM_ROUTINE WorkRoutine, PVOID ContextPtr);

    //
    // PPM Functions
    //

    static _DEVICE_ID deviceIdFromDependencyIndex (ULONG DependencyIndex);

    static KSERVICE_ROUTINE uartDebuggerIsr;

    static IO_DPC_ROUTINE uartDebuggerDpc;

    static IO_WORKITEM_ROUTINE debugVetoWorkItemRoutine;

    static KDEFERRED_ROUTINE vetoTimerDpcRoutine;

    _IRQL_requires_max_(HIGH_LEVEL)
    void executePlatformIdleWait (_Inout_ PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr);

    _IRQL_requires_max_(HIGH_LEVEL)
    void executePlatformIdleStopLight (_Inout_ PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr);

    _IRQL_requires_max_(HIGH_LEVEL)
    void executePlatformIdleArmOff (_Inout_ PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr);

    _IRQL_requires_max_(HIGH_LEVEL)
    NTSTATUS updateGpcInterruptController ();

    void writeClpcrWaitStop (ULONG Clpcr);

    void enableDebuggerWake ();

    void maskGpcInterrupts ();

    void unmaskGpcInterrupts ();

    //
    // private members
    //

    volatile LONG activeProcessorCount;
    UNMASKED_INTERRUPTS unmaskedInterruptsCopy;

    volatile MX6_CCM_REGISTERS* ccmRegistersPtr;
    volatile MX6_CCM_ANALOG_REGISTERS* analogRegistersPtr;
    volatile MX6_GPC_REGISTERS* gpcRegistersPtr;
    volatile MX6_MMDC_REGISTERS* mmdcRegistersPtr;
    volatile MX6_IOMUXC_REGISTERS* iomuxcRegistersPtr;
    volatile void* armMpRegistersPtr;

    DEVICE_OBJECT* lowerDeviceObjectPtr;
    DEVICE_OBJECT* pdoPtr;
    DEVICE_OBJECT* fdoPtr;
    UNICODE_STRING deviceInterfaceName;
    PEP_KERNEL_INFORMATION pepKernelInfo;
    KSEMAPHORE ioRequestSemaphore;

    struct {
        IMX_UART_REGISTERS* uartRegistersPtr;
        PKINTERRUPT uartInterruptPtr;
        PIO_WORKITEM connectInterruptWorkItem;
        PIO_WORKITEM toggleVetoWorkItem;
        KDPC clearVetoDpc;
        KTIMER clearVetoTimer;
        CM_PARTIAL_RESOURCE_DESCRIPTOR uartInterruptResource;
        bool vetoActive;
        bool vetoTimerFired;
    } debugger;

    // Protects access to the clock gating control registers
    KSPIN_LOCK ccgrRegistersSpinLock;
    ULONG ccgrRegistersShadow[ARRAYSIZE(MX6_CCM_REGISTERS::CCGR)];

    // Stores number of references to UART_CLK_ENABLE and UART_SERIAL_CLK_ENABLE
    KSPIN_LOCK uartClockSpinLock;
    ULONG uartClockRefCount;

    // Stores refcount of the VDD_PU power domain
    KEVENT gpuVpuDomainStableEvent;
    volatile LONG gpuVpuDomainRefCount;

    // Stores per-device state information. Use contextFromDeviceId() to access.
    _DEVICE_CONTEXT deviceData[unsigned(_DEVICE_ID::_COUNT)];

    struct _GPU_ENGINE_COMPONENT_CONTEXT {
        ULONG IdleState;
    } gpuEngineComponentContext;

    struct {
        KSPIN_LOCK ListLock;
        LIST_ENTRY ListHead;
        LOOKASIDE_LIST_EX LookasideList;
    } workQueue;

public: // PAGED

    static DRIVER_ADD_DEVICE AddDevice;

    //
    // PAGED WDM Dispatch Routines
    //
    _Dispatch_type_(IRP_MJ_PNP)
    static DRIVER_DISPATCH DispatchPnp;

    _Dispatch_type_(IRP_MJ_CREATE)
    _Dispatch_type_(IRP_MJ_CLOSE)
    static DRIVER_DISPATCH DispatchFileCreateClose;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    MX6_PEP ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    ~MX6_PEP ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS InitializeResources ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS RegisterPlugin ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS CheckForReservedDevices();

private: // PAGED

    _IRQL_requires_max_(PASSIVE_LEVEL)
    static NTSTATUS validatePadDescriptor (MX6_PAD_DESCRIPTOR PadDesc);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS startDevice (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    void enableWaitMode ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Requires_lock_held_(this->ioRequestSemaphore)
    NTSTATUS ioctlDumpRegisters (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Requires_lock_held_(this->ioRequestSemaphore)
    NTSTATUS ioctlGetClockGateRegisters (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Requires_lock_held_(this->ioRequestSemaphore)
    NTSTATUS ioctlSetClockGate (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Requires_lock_held_(this->ioRequestSemaphore)
    NTSTATUS ioctlProfileMmdc (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Requires_lock_held_(this->ioRequestSemaphore)
    NTSTATUS ioctlWriteCcosr (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Requires_lock_held_(this->ioRequestSemaphore)
    NTSTATUS ioctlGetPadConfig (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Requires_lock_held_(this->ioRequestSemaphore)
    NTSTATUS ioctlSetPadConfig (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr);

};

// PAGED
extern "C" DRIVER_UNLOAD MX6PepDriverUnload;
extern "C" DRIVER_INITIALIZE DriverEntry;

#endif // _MX6PEP_H_

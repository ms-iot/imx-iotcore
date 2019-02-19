// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6pep.cpp
//
// Abstract:
//
//   IMX6 Platform Extension Plugin
//

#include "precomp.h"

#include "trace.h"
#include "mx6pep.tmh"

#include "mx6peputil.h"
#include "mx6pepioctl.h"
#include "mx6pephw.h"
#include "mx6pep.h"

MX6_NONPAGED_SEGMENT_BEGIN; //==============================================

namespace { // static

    MX6_PEP* pepGlobalContextPtr;

    _IRQL_requires_max_(DISPATCH_LEVEL)
    NTSTATUS MX6CompleteRequest (
        IRP* IrpPtr,
        NTSTATUS Status,
        ULONG_PTR Information = 0
        )
    {
        IrpPtr->IoStatus.Status = Status;
        IrpPtr->IoStatus.Information = Information,
        IoCompleteRequest(IrpPtr, IO_NO_INCREMENT);
        return Status;
    }

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    _IRQL_requires_max_(DISPATCH_LEVEL)
    NTSTATUS MX6SyncCompletion (
        _In_ DEVICE_OBJECT* /*DeviceObjectPtr*/,
        _In_ IRP* /*IrpPtr*/,
        _In_ void* ContextPtr
        )
    {
        auto eventPtr = static_cast<KEVENT*>(ContextPtr);
        KeSetEvent(eventPtr, IO_NO_INCREMENT, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    _IRQL_requires_max_(DISPATCH_LEVEL)
    NTSTATUS MX6ForwardAndWait (
        DEVICE_OBJECT* DeviceObjectPtr,
        IRP* IrpPtr
        )
    {
        KEVENT event;
        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(IrpPtr);
        IoSetCompletionRoutine(IrpPtr, MX6SyncCompletion, &event, TRUE, TRUE, TRUE);

        NTSTATUS status = IoCallDriver(DeviceObjectPtr, IrpPtr);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = IrpPtr->IoStatus.Status;
        }

        return status;
    }

    template <typename TInputBuffer>
    NTSTATUS MX6RetrieveInputBuffer (IRP* IrpPtr, TInputBuffer** InputBufferPPtr)
    {
        IO_STACK_LOCATION* ioStackPtr = IoGetCurrentIrpStackLocation(IrpPtr);
        NT_ASSERT(ioStackPtr->MajorFunction == IRP_MJ_DEVICE_CONTROL);
        NT_ASSERT((IrpPtr->Flags & IRP_BUFFERED_IO) != 0);

        // validate input buffer size
        if (ioStackPtr->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(TInputBuffer)) {

            return STATUS_INVALID_PARAMETER;
        }

        *InputBufferPPtr = static_cast<TInputBuffer*>(
                IrpPtr->AssociatedIrp.SystemBuffer);

        return STATUS_SUCCESS;
    }

    template <typename TOutputBuffer>
    NTSTATUS MX6RetrieveOutputBuffer (IRP* IrpPtr, TOutputBuffer** OutputBufferPPtr)
    {
        IO_STACK_LOCATION* ioStackPtr = IoGetCurrentIrpStackLocation(IrpPtr);
        NT_ASSERT(ioStackPtr->MajorFunction == IRP_MJ_DEVICE_CONTROL);
        NT_ASSERT((IrpPtr->Flags & IRP_BUFFERED_IO) != 0);

        // validate output buffer size
        if (ioStackPtr->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(TOutputBuffer)) {

            return STATUS_INVALID_PARAMETER;
        }

        *OutputBufferPPtr = static_cast<TOutputBuffer*>(
                IrpPtr->AssociatedIrp.SystemBuffer);

        return STATUS_SUCCESS;
    }

} // namespace "static"

void __cdecl operator delete ( void* ) {}

void __cdecl operator delete (void*, unsigned int) {}

_Use_decl_annotations_
BOOLEAN MX6_PEP::AcceptAcpiNotification (ULONG Notification, void* DataPtr)
{
    MX6_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    MX6_PEP* thisPtr = pepGlobalContextPtr;

    switch (Notification) {
    case PEP_NOTIFY_ACPI_PREPARE_DEVICE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->AcpiPrepareDevice(
            static_cast<PEP_ACPI_PREPARE_DEVICE*>(DataPtr));

    case PEP_NOTIFY_ACPI_ABANDON_DEVICE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->AcpiAbandonDevice(
            static_cast<PEP_ACPI_ABANDON_DEVICE*>(DataPtr));

    case PEP_NOTIFY_ACPI_REGISTER_DEVICE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->AcpiRegisterDevice(
            static_cast<PEP_ACPI_REGISTER_DEVICE*>(DataPtr));

    case PEP_NOTIFY_ACPI_ENUMERATE_DEVICE_NAMESPACE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->AcpiEnumerateDeviceNamespace(
            static_cast<PEP_ACPI_ENUMERATE_DEVICE_NAMESPACE*>(DataPtr));

    case PEP_NOTIFY_ACPI_QUERY_OBJECT_INFORMATION:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->AcpiQueryObjectInformation(
            static_cast<PEP_ACPI_QUERY_OBJECT_INFORMATION*>(DataPtr));

    case PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->AcpiEvaluateControlMethod(
            static_cast<PEP_ACPI_EVALUATE_CONTROL_METHOD*>(DataPtr));

    case PEP_NOTIFY_ACPI_UNREGISTER_DEVICE:
    case PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES:
    case PEP_NOTIFY_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES:
    case PEP_NOTIFY_ACPI_WORK:
    default:
        MX6_LOG_TRACE(
            "Unsupported ACPI notification. (Notification = 0x%x)",
            Notification);
        return FALSE;
    }
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::AcceptDeviceNotification (ULONG Notification, void* DataPtr)
{
    MX6_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    MX6_PEP* thisPtr = pepGlobalContextPtr;

    switch (Notification) {
    case PEP_DPM_PREPARE_DEVICE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->DpmPrepareDevice(
            static_cast<PEP_PREPARE_DEVICE*>(DataPtr));

    case PEP_DPM_ABANDON_DEVICE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->DpmAbandonDevice(
            static_cast<PEP_ABANDON_DEVICE*>(DataPtr));

    case PEP_DPM_REGISTER_DEVICE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->DpmRegisterDevice(
            static_cast<PEP_REGISTER_DEVICE_V2*>(DataPtr));

    case PEP_DPM_UNREGISTER_DEVICE:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->DpmUnregisterDevice(
            static_cast<PEP_UNREGISTER_DEVICE*>(DataPtr));

    case PEP_DPM_DEVICE_POWER_STATE:
        return thisPtr->DpmDevicePowerState(
            static_cast<PEP_DEVICE_POWER_STATE*>(DataPtr));

    case PEP_DPM_REGISTER_DEBUGGER:
        return thisPtr->DpmRegisterDebugger(
            static_cast<PEP_REGISTER_DEBUGGER*>(DataPtr));

    case PEP_DPM_POWER_CONTROL_REQUEST:
        return thisPtr->DpmPowerControlRequest(
            static_cast<PEP_POWER_CONTROL_REQUEST*>(DataPtr));

    case PEP_DPM_DEVICE_IDLE_CONSTRAINTS:
        return thisPtr->DpmDeviceIdleContraints(
            static_cast<PEP_DEVICE_PLATFORM_CONSTRAINTS*>(DataPtr));

    case PEP_DPM_NOTIFY_COMPONENT_IDLE_STATE:
        return thisPtr->DpmComponentIdleState(
            static_cast<PEP_NOTIFY_COMPONENT_IDLE_STATE*>(DataPtr));

    case PEP_DPM_COMPONENT_IDLE_CONSTRAINTS:
        return thisPtr->DpmComponentIdleConstraints(
            static_cast<PEP_COMPONENT_PLATFORM_CONSTRAINTS*>(DataPtr));

    case PEP_DPM_REGISTER_CRASHDUMP_DEVICE:
        return thisPtr->DpmRegisterCrashdumpDevice(
            static_cast<PEP_REGISTER_CRASHDUMP_DEVICE*>(DataPtr));

    case PEP_DPM_WORK:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->DpmWork(static_cast<PEP_WORK*>(DataPtr));

    case PEP_DPM_DEVICE_STARTED:
    case PEP_DPM_SYSTEM_LATENCY_UPDATE:
    case PEP_DPM_QUERY_COMPONENT_PERF_CAPABILTIES:
    case PEP_DPM_QUERY_COMPONENT_PERF_SET:
    case PEP_DPM_QUERY_COMPONENT_PERF_SET_NAME:
    case PEP_DPM_QUERY_COMPONENT_PERF_STATES:
    case PEP_DPM_REGISTER_COMPONENT_PERF_STATES:
    case PEP_DPM_REQUEST_COMPONENT_PERF_STATE:
    case PEP_DPM_QUERY_CURRENT_COMPONENT_PERF_STATE:
    case PEP_DPM_QUERY_DEBUGGER_TRANSITION_REQUIREMENTS:
    case PEP_DPM_QUERY_SOC_SUBSYSTEM_COUNT:
    case PEP_DPM_QUERY_SOC_SUBSYSTEM:
    case PEP_DPM_RESET_SOC_SUBSYSTEM_ACCOUNTING:
    case PEP_DPM_QUERY_SOC_SUBSYSTEM_BLOCKING_TIME:
    case PEP_DPM_QUERY_SOC_SUBSYSTEM_METADATA:
    case PEP_DPM_POWER_CONTROL_COMPLETE:
    default:
        MX6_LOG_TRACE(
            "Unsupported DPM notification. (Notification = 0x%x)",
            Notification);

        return FALSE;
    }
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::AcceptProcessorNotification (
    PEPHANDLE Handle,
    ULONG Notification,
    PVOID DataPtr
    )
{
    MX6_PEP* thisPtr = pepGlobalContextPtr;

    switch (Notification) {
    case PEP_NOTIFY_PPM_QUERY_CAPABILITIES:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmQueryCapabilities(
            Handle,
            static_cast<PEP_PPM_QUERY_CAPABILITIES*>(DataPtr));

    case PEP_NOTIFY_PPM_QUERY_IDLE_STATES_V2:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmQueryIdleStatesV2(
            Handle,
            static_cast<PEP_PPM_QUERY_IDLE_STATES_V2*>(DataPtr));

    case PEP_NOTIFY_PPM_QUERY_PLATFORM_STATES:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmQueryPlatformStates(
            Handle,
            static_cast<PEP_PPM_QUERY_PLATFORM_STATES*>(DataPtr));

    case PEP_NOTIFY_PPM_QUERY_COORDINATED_STATES:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmQueryCoordinatedStates(
            Handle,
            static_cast<PEP_PPM_QUERY_COORDINATED_STATES*>(DataPtr));

    case PEP_NOTIFY_PPM_QUERY_COORDINATED_DEPENDENCY:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmQueryCoordinatedDependency(
            Handle,
            static_cast<PEP_PPM_QUERY_COORDINATED_DEPENDENCY*>(DataPtr));

    case PEP_NOTIFY_PPM_IDLE_EXECUTE:
        return thisPtr->PpmIdleExecute(
            Handle,
            static_cast<PEP_PPM_IDLE_EXECUTE_V2*>(DataPtr));

    case PEP_NOTIFY_PPM_IDLE_COMPLETE:
        return thisPtr->PpmIdleComplete(
            Handle,
            static_cast<PEP_PPM_IDLE_COMPLETE_V2*>(DataPtr));

    case PEP_NOTIFY_PPM_QUERY_VETO_REASONS:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmQueryVetoReasons(
            Handle,
            static_cast<PEP_PPM_QUERY_VETO_REASONS*>(DataPtr));

    case PEP_NOTIFY_PPM_QUERY_VETO_REASON:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmQueryVetoReason(
            Handle,
            static_cast<PEP_PPM_QUERY_VETO_REASON*>(DataPtr));

    case PEP_NOTIFY_PPM_ENUMERATE_BOOT_VETOES:
        MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
        return thisPtr->PpmEnumerateBootVetoes(Handle);

    case PEP_NOTIFY_PPM_TEST_IDLE_STATE:
        return thisPtr->PpmTestIdleState(
            Handle,
            static_cast<PEP_PPM_TEST_IDLE_STATE*>(DataPtr));

    case PEP_NOTIFY_PPM_IS_PROCESSOR_HALTED:
    case PEP_NOTIFY_PPM_INITIATE_WAKE:
    case PEP_NOTIFY_PPM_QUERY_FEEDBACK_COUNTERS:
    case PEP_NOTIFY_PPM_FEEDBACK_READ:
    case PEP_NOTIFY_PPM_QUERY_PERF_CAPABILITIES:
    case PEP_NOTIFY_PPM_PERF_CONSTRAINTS:
    case PEP_NOTIFY_PPM_PERF_SET:
    case PEP_NOTIFY_PPM_PARK_SELECTION:
    case PEP_NOTIFY_PPM_CST_STATES:
    case PEP_NOTIFY_PPM_QUERY_PLATFORM_STATE:
    case PEP_NOTIFY_PPM_IDLE_PRE_EXECUTE:
    case PEP_NOTIFY_PPM_UPDATE_PLATFORM_STATE:
    case PEP_NOTIFY_PPM_QUERY_PLATFORM_STATE_RESIDENCIES:
    case PEP_NOTIFY_PPM_QUERY_COORDINATED_STATE_NAME:
    case PEP_NOTIFY_PPM_QUERY_PROCESSOR_STATE_NAME:
    case PEP_NOTIFY_PPM_PARK_SELECTION_V2:
    case PEP_NOTIFY_PPM_PARK_MASK:
    case PEP_NOTIFY_PPM_PERF_CHECK_COMPLETE:
    case PEP_NOTIFY_PPM_LPI_SUPPORTED:
    case PEP_NOTIFY_PPM_LPI_PROCESSOR_STATES:
    default:
        return FALSE;
    }
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PowerOnCrashdumpDevice (
    PEP_CRASHDUMP_INFORMATION* CrashdumpInformationPtr
    )
{
    //
    // Delegate to setComponentF0 to power on the SDHC component
    //
    PEP_NOTIFY_COMPONENT_IDLE_STATE args;
    args.DeviceHandle = CrashdumpInformationPtr->DeviceHandle;
    args.Component = 0;             // SDHC has one component
    args.IdleState = 0;             // F0
    args.DriverNotified = FALSE;    // Simulate pre-notification
    args.Completed = FALSE;
    return pepGlobalContextPtr->setComponentF0(&args);
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::DispatchDeviceIoControl (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr)
{
    MX6_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    const IO_STACK_LOCATION* ioStackPtr = IoGetCurrentIrpStackLocation(IrpPtr);
    NT_ASSERT(ioStackPtr->MajorFunction == IRP_MJ_DEVICE_CONTROL);

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        MX6_LOG_ERROR(
            "mx6pep may only be called at PASSIVE_LEVEL. (KeGetCurrentIrql() = %!irql!)",
            KeGetCurrentIrql());

        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_LEVEL);
    }

    auto DeviceExtensionPtr = static_cast<_DEVICE_EXTENSION*>(
                                        DeviceObjectPtr->DeviceExtension);
    auto thisPtr = DeviceExtensionPtr->Mx6PepPtr;

    // serialize all IRPs for simplicity
    NTSTATUS status = KeWaitForSingleObject(
            &thisPtr->ioRequestSemaphore,
            Executive,
            KernelMode,
            FALSE,
            nullptr);

    UNREFERENCED_PARAMETER(status);
    NT_ASSERT(status == STATUS_SUCCESS);

    auto releaseSemaphore = MX6_FINALLY::Do([&] {
        KeReleaseSemaphore(&thisPtr->ioRequestSemaphore, IO_NO_INCREMENT, 1, FALSE);
    });

    switch (ioStackPtr->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_MX6PEP_DUMP_REGISTERS:
        return thisPtr->ioctlDumpRegisters(DeviceObjectPtr, IrpPtr);

    case IOCTL_MX6PEP_GET_CLOCK_GATE_REGISTERS:
        return thisPtr->ioctlGetClockGateRegisters(DeviceObjectPtr, IrpPtr);

    case IOCTL_MX6PEP_SET_CLOCK_GATE:
        return thisPtr->ioctlSetClockGate(DeviceObjectPtr, IrpPtr);

    case IOCTL_MX6PEP_PROFILE_MMDC:
        return thisPtr->ioctlProfileMmdc(DeviceObjectPtr, IrpPtr);

    case IOCTL_MX6PEP_WRITE_CCOSR:
        return thisPtr->ioctlWriteCcosr(DeviceObjectPtr, IrpPtr);

    case IOCTL_MX6PEP_GET_PAD_CONFIG:
        return thisPtr->ioctlGetPadConfig(DeviceObjectPtr, IrpPtr);

    case IOCTL_MX6PEP_SET_PAD_CONFIG:
        return thisPtr->ioctlSetPadConfig(DeviceObjectPtr, IrpPtr);

    default:
        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_DEVICE_REQUEST);
    }
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::DispatchUnhandledIrp (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr)
{
    MX6_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    auto DeviceExtensionPtr = static_cast<_DEVICE_EXTENSION*>(
                                        DeviceObjectPtr->DeviceExtension);
    auto thisPtr = DeviceExtensionPtr->Mx6PepPtr;

    // Forward all IRPs
    IoSkipCurrentIrpStackLocation(IrpPtr);
    return IoCallDriver(thisPtr->lowerDeviceObjectPtr, IrpPtr);
}

MX6_NONPAGED_SEGMENT_END; //================================================
MX6_PAGED_SEGMENT_BEGIN; //=================================================

_Use_decl_annotations_
NTSTATUS MX6_PEP::RegisterPlugin ()
{
    PAGED_CODE();

    NT_ASSERT(pepGlobalContextPtr == nullptr);
    pepGlobalContextPtr = this;

    // Register with PEP framework
    PEP_INFORMATION pepInfo = {0};
    pepInfo.Version = PEP_INFORMATION_VERSION;
    pepInfo.Size = sizeof(pepInfo);
    pepInfo.AcceptAcpiNotification = MX6_PEP::AcceptAcpiNotification;
    pepInfo.AcceptDeviceNotification = MX6_PEP::AcceptDeviceNotification;
    pepInfo.AcceptProcessorNotification = MX6_PEP::AcceptProcessorNotification;

    this->pepKernelInfo = PEP_KERNEL_INFORMATION();
    this->pepKernelInfo.Version = PEP_INFORMATION_VERSION;
    this->pepKernelInfo.Size = sizeof(this->pepKernelInfo);
    NTSTATUS status = PoFxRegisterPlugin(&pepInfo, &this->pepKernelInfo);
    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "Failed to register with the PEP framework. (status = %!STATUS!)",
            status);

        NT_ASSERT(FALSE);
        pepGlobalContextPtr = nullptr;
        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::validatePadDescriptor (MX6_PAD_DESCRIPTOR PadDesc)
{
    PAGED_CODE();

    if ((PadDesc.CtrlRegOffset >= MX6_IOMUXC_LENGTH) ||
        (PadDesc.MuxRegOffset >= MX6_IOMUXC_LENGTH)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (((PadDesc.CtrlRegOffset % sizeof(ULONG)) != 0) ||
        ((PadDesc.MuxRegOffset % sizeof(ULONG)) != 0)) {

        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::ioctlDumpRegisters (DEVICE_OBJECT* /*DeviceObjectPtr*/, IRP* IrpPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    MX6PEP_DUMP_REGISTERS_OUTPUT* outputBufferPtr;
    NTSTATUS status = MX6RetrieveOutputBuffer(IrpPtr, &outputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    // CCM
    outputBufferPtr->Ccm.CCR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CCR);

    outputBufferPtr->Ccm.CCDR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CCDR);

    outputBufferPtr->Ccm.CSR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CSR);

    outputBufferPtr->Ccm.CCSR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CCSR);

    outputBufferPtr->Ccm.CACRR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CACRR);

    outputBufferPtr->Ccm.CBCDR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CBCDR);

    outputBufferPtr->Ccm.CBCMR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CBCMR);

    outputBufferPtr->Ccm.CSCMR1 =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CSCMR1);

    outputBufferPtr->Ccm.CSCMR2 =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CSCMR2);

    outputBufferPtr->Ccm.CSCDR1 =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CSCDR1);

    outputBufferPtr->Ccm.CS1CDR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CS1CDR);

    outputBufferPtr->Ccm.CS2CDR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CS2CDR);

    outputBufferPtr->Ccm.CDCDR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CDCDR);

    outputBufferPtr->Ccm.CHSCCDR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CHSCCDR);

    outputBufferPtr->Ccm.CSCDR2 =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CSCDR2);

    outputBufferPtr->Ccm.CSCDR3 =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CSCDR3);

    outputBufferPtr->Ccm.CDHIPR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CDHIPR);

    outputBufferPtr->Ccm.CLPCR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CLPCR);

    outputBufferPtr->Ccm.CISR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CISR);

    outputBufferPtr->Ccm.CIMR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CIMR);

    outputBufferPtr->Ccm.CCOSR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CCOSR);

    outputBufferPtr->Ccm.CGPR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CGPR);

    for (ULONG i = 0; i < ARRAYSIZE(outputBufferPtr->Ccm.CCGR); ++i) {
        outputBufferPtr->Ccm.CCGR[i] =
            READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CCGR[i]);
    }

    outputBufferPtr->Ccm.CMEOR =
        READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CMEOR);


    // Analog
    outputBufferPtr->Analog.PLL_ARM =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_ARM);

    outputBufferPtr->Analog.PLL_USB1 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_USB1);

    outputBufferPtr->Analog.PLL_USB2 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_USB2);

    outputBufferPtr->Analog.PLL_SYS =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_SYS);

    outputBufferPtr->Analog.PLL_SYS_SS =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_SYS_SS);

    outputBufferPtr->Analog.PLL_SYS_NUM =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_SYS_NUM);

    outputBufferPtr->Analog.PLL_SYS_DENOM =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_SYS_DENOM);

    outputBufferPtr->Analog.PLL_AUDIO =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_AUDIO);

    outputBufferPtr->Analog.PLL_AUDIO_NUM =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_AUDIO_NUM);

    outputBufferPtr->Analog.PLL_AUDIO_DENOM =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_AUDIO_DENOM);

    outputBufferPtr->Analog.PLL_VIDEO =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_VIDEO);

    outputBufferPtr->Analog.PLL_VIDEO_NUM =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_VIDEO_NUM);

    outputBufferPtr->Analog.PLL_VIDEO_DENOM =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_VIDEO_DENOM);

    outputBufferPtr->Analog.PLL_MLB =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_MLB);

    outputBufferPtr->Analog.PLL_ENET =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PLL_ENET);

    outputBufferPtr->Analog.PFD_480 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PFD_480);

    outputBufferPtr->Analog.PFD_528 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PFD_528);

    outputBufferPtr->Analog.PMU_REG_1P1 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PMU_REG_1P1);

    outputBufferPtr->Analog.PMU_REG_3P0 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PMU_REG_3P0);

    outputBufferPtr->Analog.PMU_REG_2P5 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PMU_REG_2P5);

    outputBufferPtr->Analog.PMU_REG_CORE =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->PMU_REG_CORE);

    outputBufferPtr->Analog.MISC0 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->MISC0);

    outputBufferPtr->Analog.MISC1 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->MISC1);

    outputBufferPtr->Analog.MISC2 =
        READ_REGISTER_NOFENCE_ULONG(&this->analogRegistersPtr->MISC2);

    //
    // GPC Register Dump
    //

    outputBufferPtr->Gpc.CNTR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->CNTR);

    outputBufferPtr->Gpc.PGR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGR);

    for (ULONG i = 0; i < ARRAYSIZE(outputBufferPtr->Gpc.IMR); ++i) {
        outputBufferPtr->Gpc.IMR[i] =
            READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->IMR[i]);
    }

    for (ULONG i = 0; i < ARRAYSIZE(outputBufferPtr->Gpc.ISR); ++i) {
        outputBufferPtr->Gpc.ISR[i] =
            READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->ISR[i]);
    }

    //
    // PGC_GPU
    //

    outputBufferPtr->Gpc.PGC_GPU.CTRL =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_GPU.CTRL);

    outputBufferPtr->Gpc.PGC_GPU.PUPSCR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_GPU.PUPSCR);

    outputBufferPtr->Gpc.PGC_GPU.PDNSCR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_GPU.PDNSCR);

    outputBufferPtr->Gpc.PGC_GPU.SR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_GPU.SR);

    //
    // PGC_GPU
    //

    outputBufferPtr->Gpc.PGC_CPU.CTRL =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_CPU.CTRL);

    outputBufferPtr->Gpc.PGC_CPU.PUPSCR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_CPU.PUPSCR);

    outputBufferPtr->Gpc.PGC_CPU.PDNSCR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_CPU.PDNSCR);

    outputBufferPtr->Gpc.PGC_CPU.SR =
        READ_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_CPU.SR);


    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS, sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::ioctlGetClockGateRegisters (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    IRP* IrpPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;

    MX6PEP_GET_CLOCK_GATE_REGISTERS_OUTPUT* outputBufferPtr;
    status = MX6RetrieveOutputBuffer(IrpPtr, &outputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    for (ULONG i = 0; i < ARRAYSIZE(outputBufferPtr->CcgrRegisters); ++i) {
        outputBufferPtr->CcgrRegisters[i] =
            READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CCGR[i]);
    }

    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS, sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::ioctlSetClockGate (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    IRP* IrpPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;

    MX6PEP_SET_CLOCK_GATE_INPUT* inputBufferPtr;
    status = MX6RetrieveInputBuffer(IrpPtr, &inputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    const MX6_CLK_GATE clockGate = inputBufferPtr->ClockGate;
    if ((clockGate < 0) || (clockGate >= MX6_CLK_GATE_MAX)) {
        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_PARAMETER);
    }

    switch (inputBufferPtr->State) {
    case MX6_CCM_CCGR_OFF:
    case MX6_CCM_CCGR_ON_RUN:
    case MX6_CCM_CCGR_ON:
        break;
    default:
        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_PARAMETER);
    }

    this->setClockGate(clockGate, inputBufferPtr->State);

    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS);
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::ioctlProfileMmdc (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    IRP* IrpPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;

    MX6PEP_PROFILE_MMDC_INPUT* inputBufferPtr;
    status = MX6RetrieveInputBuffer(IrpPtr, &inputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    MX6PEP_PROFILE_MMDC_OUTPUT* outputBufferPtr;
    status = MX6RetrieveOutputBuffer(IrpPtr, &outputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    if (inputBufferPtr->DurationMillis > MX6_MMDC_PROFILE_DURATION_MAX) {
        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_PARAMETER);
    }

    volatile MX6_MMDC_REGISTERS* registersPtr = this->mmdcRegistersPtr;

    // reset
    {
        MX6_MMDC_MADPCR0_REG madpcr0 = {0};
        madpcr0.DBG_RST = 1;
        madpcr0.CYC_OVF = 1;
        WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->MADPCR0, madpcr0.AsUlong);
    }

    // configure AXI ID
    {
        MX6_MMDC_MADPCR1_REG madpcr1 = {0};
        madpcr1.PRF_AXI_ID = inputBufferPtr->AxiId;
        madpcr1.PRF_AXI_ID_MASK = inputBufferPtr->AxiIdMask;
        WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->MADPCR1, madpcr1.AsUlong);
    }

    ULONGLONG startTime = KeQueryInterruptTime();

    // start profiling
    {
        MX6_MMDC_MADPCR0_REG madpcr0 = {0};
        madpcr0.DBG_EN = 1;
        WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->MADPCR0, madpcr0.AsUlong);
    }

    // Delay for specified duration
    LARGE_INTEGER interval;
    interval.QuadPart = -10000LL * inputBufferPtr->DurationMillis;
    status = KeDelayExecutionThread(KernelMode, FALSE, &interval);
    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "KeDelayExecutionThread failed! (status = %!STATUS!)",
            status);

        NT_ASSERT(FALSE);
        return MX6CompleteRequest(IrpPtr, status);
    }

    // Freeze profile
    {
        MX6_MMDC_MADPCR0_REG madpcr0 = {0};
        madpcr0.DBG_EN = 1;
        madpcr0.PRF_FRZ = 1;
        WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->MADPCR0, madpcr0.AsUlong);
    }

    ULONGLONG stopTime = KeQueryInterruptTime();

    // Check for overflow
    const MX6_MMDC_MADPCR0_REG madpcr0Reg =
        {READ_REGISTER_NOFENCE_ULONG(&registersPtr->MADPCR0)};

    if (madpcr0Reg.CYC_OVF != 0) {
        MX6_LOG_ERROR(
            "MMDC profiling overflow occurred. (madpcr0Reg = 0x%x)",
            madpcr0Reg.AsUlong);

        return MX6CompleteRequest(IrpPtr, STATUS_INTEGER_OVERFLOW);
    }

    // Return data to user
    *outputBufferPtr = MX6PEP_PROFILE_MMDC_OUTPUT();
    outputBufferPtr->ActualDurationMillis =
        static_cast<ULONG>(stopTime - startTime) / 10000;

    outputBufferPtr->TotalProfilingCount =
        READ_REGISTER_NOFENCE_ULONG(&registersPtr->MADPSR0);

    outputBufferPtr->BusyCycleCount =
        READ_REGISTER_NOFENCE_ULONG(&registersPtr->MADPSR1);

    outputBufferPtr->ReadAccessCount =
        READ_REGISTER_NOFENCE_ULONG(&registersPtr->MADPSR2);

    outputBufferPtr->WriteAccessCount =
        READ_REGISTER_NOFENCE_ULONG(&registersPtr->MADPSR3);

    outputBufferPtr->BytesRead =
        READ_REGISTER_NOFENCE_ULONG(&registersPtr->MADPSR4);

    outputBufferPtr->BytesWritten =
        READ_REGISTER_NOFENCE_ULONG(&registersPtr->MADPSR5);

    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS, sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::ioctlWriteCcosr (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    IRP* IrpPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    const ULONG* inputBufferPtr;
    NTSTATUS status = MX6RetrieveInputBuffer(IrpPtr, &inputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    WRITE_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CCOSR, *inputBufferPtr);
    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS);
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::ioctlGetPadConfig (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    IRP* IrpPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;

    const MX6PEP_GET_PAD_CONFIG_INPUT* inputBufferPtr;
    status = MX6RetrieveInputBuffer(IrpPtr, &inputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_PARAMETER);
    }

    const MX6_PAD_DESCRIPTOR padDesc = inputBufferPtr->PadDescriptor;
    status = this->validatePadDescriptor(padDesc);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    MX6PEP_GET_PAD_CONFIG_OUTPUT* outputBufferPtr;
    status = MX6RetrieveOutputBuffer(IrpPtr, &outputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_PARAMETER);
    }

    outputBufferPtr->PadMuxRegister = READ_REGISTER_NOFENCE_ULONG(
            MX6RegAddress(this->iomuxcRegistersPtr, padDesc.MuxRegOffset));

    outputBufferPtr->PadControlRegister = READ_REGISTER_NOFENCE_ULONG(
            MX6RegAddress(this->iomuxcRegistersPtr, padDesc.CtrlRegOffset));

    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS, sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::ioctlSetPadConfig (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    IRP* IrpPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;

    const MX6PEP_SET_PAD_CONFIG_INPUT* inputBufferPtr;
    status = MX6RetrieveInputBuffer(IrpPtr, &inputBufferPtr);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, STATUS_INVALID_PARAMETER);
    }

    const MX6_PAD_DESCRIPTOR padDesc = inputBufferPtr->PadDescriptor;
    status = this->validatePadDescriptor(padDesc);
    if (!NT_SUCCESS(status)) {
        return MX6CompleteRequest(IrpPtr, status);
    }

    if (inputBufferPtr->PadMuxRegister != MX6_PAD_REGISTER_INVALID) {
        WRITE_REGISTER_NOFENCE_ULONG(
           MX6RegAddress(this->iomuxcRegistersPtr, padDesc.MuxRegOffset),
           inputBufferPtr->PadMuxRegister & 0x7);
    }

    if (inputBufferPtr->PadControlRegister != MX6_PAD_REGISTER_INVALID) {
        WRITE_REGISTER_NOFENCE_ULONG(
           MX6RegAddress(this->iomuxcRegistersPtr, padDesc.CtrlRegOffset),
           inputBufferPtr->PadControlRegister & 0x0001f8f9);
    }

    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS);
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::InitializeResources ()
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;
    PHYSICAL_ADDRESS ccmPhysAddress = {};
    PHYSICAL_ADDRESS analogPhysAddress = {};
    PHYSICAL_ADDRESS gpcPhysAddress = {};
    PHYSICAL_ADDRESS mmdcPhysAddress = {};
    PHYSICAL_ADDRESS iomuxcPhysAddress = {};
    PHYSICAL_ADDRESS armMpPhysAddress = {};

    //
    // Map CCM Registers
    //
    ccmPhysAddress.QuadPart = MX6_CCM_BASE;
    this->ccmRegistersPtr = static_cast<MX6_CCM_REGISTERS*>(MmMapIoSpaceEx(
            ccmPhysAddress,
            sizeof(*this->ccmRegistersPtr),
            PAGE_READWRITE | PAGE_NOCACHE));

    if (this->ccmRegistersPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map memory for CCM registers.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Map Analog Registers
    //
    analogPhysAddress.QuadPart = MX6_ANALOG_BASE;
    this->analogRegistersPtr = static_cast<MX6_CCM_ANALOG_REGISTERS*>(
            MmMapIoSpaceEx(
                analogPhysAddress,
                sizeof(*this->analogRegistersPtr),
                PAGE_READWRITE | PAGE_NOCACHE));

    if (this->analogRegistersPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map memory for ANALOG registers.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Map GPC Registers
    //
    gpcPhysAddress.QuadPart = MX6_GPC_BASE;
    this->gpcRegistersPtr = static_cast<MX6_GPC_REGISTERS*>(
            MmMapIoSpaceEx(
                gpcPhysAddress,
                sizeof(*this->gpcRegistersPtr),
                PAGE_READWRITE | PAGE_NOCACHE));

    if (this->gpcRegistersPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map memory for GPC registers.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Map MMDC Registers
    //
    mmdcPhysAddress.QuadPart = MX6_MMDC_BASE;
    this->mmdcRegistersPtr = static_cast<MX6_MMDC_REGISTERS*>(
            MmMapIoSpaceEx(
                mmdcPhysAddress,
                sizeof(*this->mmdcRegistersPtr),
                PAGE_READWRITE | PAGE_NOCACHE));

    if (this->mmdcRegistersPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map memory for MMDC registers.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Map IOMUXC registers
    // The IOMUXC registers are not passed as a resource because that would
    // cause a resource conflict with the GPIO driver. Currently, the
    // IOMUXC block is used for diagnostic purposes only.
    //
    iomuxcPhysAddress.QuadPart = MX6_IOMUXC_BASE;
    this->iomuxcRegistersPtr = static_cast<MX6_IOMUXC_REGISTERS*>(MmMapIoSpaceEx(
                iomuxcPhysAddress,
                MX6_IOMUXC_LENGTH,
                PAGE_READWRITE | PAGE_NOCACHE));

    if (this->iomuxcRegistersPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map memory for IOMUXC registers.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Map ARM MP Registers
    //
    armMpPhysAddress.QuadPart = MX6_ARMMP_BASE;
    this->armMpRegistersPtr = MmMapIoSpaceEx(
            armMpPhysAddress,
            MX6_ARMMP_LENGTH,
            PAGE_READWRITE | PAGE_NOCACHE);

    if (this->armMpRegistersPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map memory for ARM MP registers.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the CCGR shadow registers
    for (ULONG i = 0; i < ARRAYSIZE(this->ccgrRegistersShadow); ++i) {
        this->ccgrRegistersShadow[i] = READ_REGISTER_NOFENCE_ULONG(
                &this->ccmRegistersPtr->CCGR[i]);
    }

    this->enableWaitMode();

    status = ExInitializeLookasideListEx(
        &this->workQueue.LookasideList,
        nullptr,                // Allocate
        nullptr,                // Free
        NonPagedPoolNx,
        0,                      // Flags
        sizeof(_WORK_ITEM),
        _POOL_TAG,
        0);                     // Depth

    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "Failed to initialize work item lookaside list. (status = %!STATUS!)",
            status);

        return status;
    }

    //
    // Check CSU for peripherals reserved in secure zone
    //
    status = this->CheckForReservedDevices();
    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "Failed checking reserved devices. (status = %!STATUS!)",
            status);

        return status;
    }

    //
    // Register with PEP framework
    //
    return this->RegisterPlugin();
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::startDevice (DEVICE_OBJECT* /*DeviceObjectPtr*/, IRP* IrpPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;

    const IO_STACK_LOCATION* ioStackPtr = IoGetCurrentIrpStackLocation(IrpPtr);

    // parse resources
    const CM_RESOURCE_LIST* resListPtr =
            ioStackPtr->Parameters.StartDevice.AllocatedResourcesTranslated;

    NT_ASSERT(resListPtr->Count > 0);
    const CM_PARTIAL_RESOURCE_LIST* partialResListPtr =
            &resListPtr->List[0].PartialResourceList;

    ULONG interruptResourceCount = 0;

    for (ULONG i = 0; i < partialResListPtr->Count; ++i) {
        const CM_PARTIAL_RESOURCE_DESCRIPTOR* descriptorPtr =
                &partialResListPtr->PartialDescriptors[i];

        switch (descriptorPtr->Type) {
        case CmResourceTypeInterrupt:
            switch (interruptResourceCount) {
            case 3:
                //
                // The third interrupt resource is the UART debugger
                // interrupt resource, used to wake up the system from
                // STOP mode when a debug break is issued.
                //
                this->debugger.uartInterruptResource = *descriptorPtr;
                break;
            } // switch (interruptResourceCount)

            ++interruptResourceCount;
            break;
        } // switch (descriptorPtr->Type)
    } // for (... partial resource list ...)

    status = IoSetDeviceInterfaceState(
            &this->deviceInterfaceName,
            TRUE);

    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "Failed to enable device interface. (status = %!STATUS!, this->deviceInterfaceName = %wZ)",
            status,
            &this->deviceInterfaceName);

        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void MX6_PEP::enableWaitMode ()
{
    PAGED_CODE();

    //
    // Enable SCU standby
    //
    {
        auto scuRegistersPtr = reinterpret_cast<volatile MX6_SCU_REGISTERS*>(
            static_cast<volatile char*>(this->armMpRegistersPtr) +
            MX6_SCU_OFFSET);

        MX6_SCU_CONTROL_REG controlReg =
            { READ_REGISTER_NOFENCE_ULONG(&scuRegistersPtr->Control) };

        NT_ASSERT(controlReg.Enable != 0);
        controlReg.StandbyEnable = 1;

        WRITE_REGISTER_NOFENCE_ULONG(
            &scuRegistersPtr->Control,
            controlReg.AsUlong);
    }

    //
    // Set required bits in CGPR
    //
    {
        MX6_CCM_CGPR_REG cgpr =
            { READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CGPR) };

        cgpr.must_be_one = 1;
        cgpr.INT_MEM_CLK_LPM = 1;

        WRITE_REGISTER_NOFENCE_ULONG(
            &this->ccmRegistersPtr->CGPR,
            cgpr.AsUlong);
    }

    //
    // Initialize masked interrupt sources
    //
    for (ULONG i = 0; i < ARRAYSIZE(this->gpcRegistersPtr->IMR); ++i) {
        WRITE_REGISTER_NOFENCE_ULONG(
            &this->gpcRegistersPtr->IMR[i],
            ~this->unmaskedInterruptsCopy.Mask[i]);
    }

    //
    // Configure IOMUXC GINT interrupt to be always asserted
    //
    {
        MX6_IOMUXC_GPR1_REG gpr1;
        gpr1.AsUlong = READ_REGISTER_NOFENCE_ULONG(&this->iomuxcRegistersPtr->Gpr[1]);
        gpr1.GINT = 1;
        WRITE_REGISTER_NOFENCE_ULONG(
            &this->iomuxcRegistersPtr->Gpr[1],
            gpr1.AsUlong);
    }

    //
    // Configure CLPCR register
    //
    {
        MX6_CCM_CLPCR_REG clpcr =
            { READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CLPCR) };

        clpcr.LPM = MX6_CCM_CLPCR_LPM_RUN;
        clpcr.ARM_clk_dis_on_lpm = 1;
        clpcr.SBYOS = 0;
        clpcr.VSTBY = 0;
        clpcr.bypass_mmdc_ch0_lpm_hs = 0;
        clpcr.bypass_mmdc_ch1_lpm_hs = 1;
        clpcr.mask_core0_wfi = 0;
        clpcr.mask_core1_wfi = 0;
        clpcr.mask_core2_wfi = 0;
        clpcr.mask_core3_wfi = 0;
        clpcr.mask_scu_idle = 0;
        clpcr.mask_l2cc_idle = 0;

        WRITE_REGISTER_NOFENCE_ULONG(
            &this->ccmRegistersPtr->CLPCR,
            clpcr.AsUlong);
    }
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::DispatchPnp (DEVICE_OBJECT* DeviceObjectPtr, IRP* IrpPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;

    IO_STACK_LOCATION* ioStackPtr = IoGetCurrentIrpStackLocation(IrpPtr);
    NT_ASSERT(ioStackPtr->MajorFunction == IRP_MJ_PNP);

    auto DeviceExtensionPtr = static_cast<_DEVICE_EXTENSION*>(
                                        DeviceObjectPtr->DeviceExtension);
    auto thisPtr = DeviceExtensionPtr->Mx6PepPtr;

    switch (ioStackPtr->MinorFunction) {
    case IRP_MN_START_DEVICE:
    {
        status = MX6ForwardAndWait(
                thisPtr->lowerDeviceObjectPtr,
                IrpPtr);

        if (!NT_SUCCESS(status)) {
            return MX6CompleteRequest(IrpPtr, status);
        }

        status = thisPtr->startDevice(DeviceObjectPtr, IrpPtr);
        return MX6CompleteRequest(IrpPtr, status);
    }
    case IRP_MN_QUERY_CAPABILITIES:
    {
        status = MX6ForwardAndWait(
                thisPtr->lowerDeviceObjectPtr,
                IrpPtr);

        if (!NT_SUCCESS(status)) {
            return MX6CompleteRequest(IrpPtr, status);
        }

        ioStackPtr = IoGetCurrentIrpStackLocation(IrpPtr);
        DEVICE_CAPABILITIES* deviceCapsPtr =
                ioStackPtr->Parameters.DeviceCapabilities.Capabilities;

        deviceCapsPtr->EjectSupported = FALSE;
        deviceCapsPtr->Removable = FALSE;
        deviceCapsPtr->SilentInstall = TRUE;

        MX6_LOG_INFORMATION("PNP Capabilities queried");
        return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS);
    }
    case IRP_MN_QUERY_REMOVE_DEVICE:
        // The device cannot be removed
        return MX6CompleteRequest(IrpPtr, STATUS_UNSUCCESSFUL);

    case IRP_MN_REMOVE_DEVICE:
    {
        //
        // Although the PEP device is not removable, this IRP can be sent
        // if this device (or another device in the stack) failed to start.
        //

        DEVICE_OBJECT* lowerDeviceObjectPtr = thisPtr->lowerDeviceObjectPtr;

        // pass to next lower level driver
        IrpPtr->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(IrpPtr);
        status = IoCallDriver(lowerDeviceObjectPtr, IrpPtr);

        IoDetachDevice(lowerDeviceObjectPtr);
        IoDeleteDevice(DeviceObjectPtr);

        return status;
    }

    default:
        // Forward all other PNP IRPs to next lower device
        IoSkipCurrentIrpStackLocation(IrpPtr);
        return IoCallDriver(thisPtr->lowerDeviceObjectPtr, IrpPtr);

    } // switch
}

//
// Do-nothing dispatch handler for IRP_MJ_CREATE and IRP_MJ_CLOSE
//
_Use_decl_annotations_
NTSTATUS MX6_PEP::DispatchFileCreateClose (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    IRP* IrpPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();
    return MX6CompleteRequest(IrpPtr, STATUS_SUCCESS);
}

_Use_decl_annotations_
MX6_PEP::MX6_PEP () :
    activeProcessorCount(),
    ccmRegistersPtr(),
    analogRegistersPtr(),
    gpcRegistersPtr(),
    mmdcRegistersPtr(),
    iomuxcRegistersPtr(),
    armMpRegistersPtr(),
    lowerDeviceObjectPtr(),
    pdoPtr(),
    fdoPtr(),
    deviceInterfaceName(),
    pepKernelInfo(),
    debugger(),
    workQueue(),
    uartClockRefCount(0),
    gpuVpuDomainRefCount(0)
{
    PAGED_CODE();

    for (ULONG i = 0; i < ARRAYSIZE(this->unmaskedInterruptsCopy.Mask); ++i) {
        this->unmaskedInterruptsCopy.Mask[i] = 0xffffffff;
    }

    KeInitializeSemaphore(&this->ioRequestSemaphore, 1, 1);
    KeInitializeSpinLock(&this->ccgrRegistersSpinLock);
    KeInitializeSpinLock(&this->uartClockSpinLock);
    KeInitializeEvent(&this->gpuVpuDomainStableEvent, SynchronizationEvent, TRUE);
    RtlZeroMemory(this->deviceData, sizeof(this->deviceData));
    KeInitializeSpinLock(&this->workQueue.ListLock);
    InitializeListHead(&this->workQueue.ListHead);
}

_Use_decl_annotations_
MX6_PEP::~MX6_PEP ()
{
    PAGED_CODE();
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    // This destructed object should not be accessible
    NT_ASSERT(pepGlobalContextPtr == nullptr);

    // Disable device interface
    if (this->deviceInterfaceName.Buffer != nullptr) {
        NTSTATUS status = IoSetDeviceInterfaceState(
                &this->deviceInterfaceName,
                FALSE);

        UNREFERENCED_PARAMETER(status);
        NT_ASSERT(
            NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

        RtlFreeUnicodeString(&this->deviceInterfaceName);
        this->deviceInterfaceName = UNICODE_STRING();
    }

    if (this->ccmRegistersPtr != nullptr) {
        MmUnmapIoSpace(
            const_cast<MX6_CCM_REGISTERS*>(this->ccmRegistersPtr),
            sizeof(*this->ccmRegistersPtr));

        this->ccmRegistersPtr = nullptr;
    }

    if (this->analogRegistersPtr != nullptr) {
        MmUnmapIoSpace(
            const_cast<MX6_CCM_ANALOG_REGISTERS*>(this->analogRegistersPtr),
            sizeof(*this->analogRegistersPtr));

        this->analogRegistersPtr = nullptr;
    }

    if (this->gpcRegistersPtr != nullptr) {
         MmUnmapIoSpace(
            const_cast<MX6_GPC_REGISTERS*>(this->gpcRegistersPtr),
            sizeof(*this->gpcRegistersPtr));

        this->gpcRegistersPtr = nullptr;
    }

    if (this->mmdcRegistersPtr != nullptr) {
         MmUnmapIoSpace(
            const_cast<MX6_MMDC_REGISTERS*>(this->mmdcRegistersPtr),
            sizeof(*this->mmdcRegistersPtr));

        this->mmdcRegistersPtr = nullptr;
    }

    if (this->iomuxcRegistersPtr != nullptr) {
         MmUnmapIoSpace(
            const_cast<MX6_IOMUXC_REGISTERS*>(this->iomuxcRegistersPtr),
            MX6_IOMUXC_LENGTH);

        this->iomuxcRegistersPtr = nullptr;
    }

    if (this->armMpRegistersPtr != nullptr) {
        MmUnmapIoSpace(
            const_cast<void*>(this->armMpRegistersPtr),
            MX6_ARMMP_LENGTH);

        this->armMpRegistersPtr = nullptr;
    }
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::AddDevice (
    DRIVER_OBJECT* DriverObjectPtr,
    DEVICE_OBJECT* PhysicalDeviceObjectPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();
    DEVICE_OBJECT* fdoPtr;
    NTSTATUS status;

    MX6_PEP* thisPtr = pepGlobalContextPtr;

    status = IoCreateDevice(
            DriverObjectPtr,
            sizeof(_DEVICE_EXTENSION),
            nullptr,                // DeviceName
            FILE_DEVICE_UNKNOWN,
            FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME,
            FALSE,                  // Exclusive
            &fdoPtr);

    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "IoCreateDevice() failed. (status = %!STATUS!)",
            status);
        return status;
    }

    // Set device extension to MX6_PEP class
    auto DeviceExtensionPtr = static_cast<_DEVICE_EXTENSION*>(
                                    fdoPtr->DeviceExtension);
    DeviceExtensionPtr->Mx6PepPtr = thisPtr;

    // Set Device Objects
    NT_ASSERT(thisPtr->fdoPtr == nullptr);
    thisPtr->fdoPtr = fdoPtr;
    thisPtr->pdoPtr = PhysicalDeviceObjectPtr;

    // Set device object flags
    thisPtr->fdoPtr->Flags &= ~DO_POWER_PAGABLE;
    thisPtr->fdoPtr->Flags |= DO_BUFFERED_IO;
    auto deleteDevice = MX6_FINALLY::DoUnless([&] {
        PAGED_CODE();
        IoDeleteDevice(thisPtr->fdoPtr);
    });

    // Register device interface
    status = IoRegisterDeviceInterface(
            PhysicalDeviceObjectPtr,
            &GUID_DEVINTERFACE_MX6PEP,
            nullptr,    // ReferenceString
            &thisPtr->deviceInterfaceName);

    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "IoRegisterDeviceInterface() failed. (status = %!STATUS!, PhysicalDeviceObjectPtr = 0x%p)",
            status,
            PhysicalDeviceObjectPtr);

        return status;
    }

    thisPtr->lowerDeviceObjectPtr = IoAttachDeviceToDeviceStack(
            thisPtr->fdoPtr,
            PhysicalDeviceObjectPtr);

    deleteDevice.DoNot();
    thisPtr->fdoPtr->Flags &= ~DO_DEVICE_INITIALIZING;

    MX6_LOG_INFORMATION(
        "PEP device added. (PhysicalDeviceObjectPtr = 0x%p, fdoPtr = 0x%p)",
        PhysicalDeviceObjectPtr,
        thisPtr->fdoPtr);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID MX6PepDriverUnload (DRIVER_OBJECT* DriverObjectPtr)
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    //
    // If we haven't cleaned up prior to driver unload, clean up now
    //
    if (pepGlobalContextPtr != nullptr) {
        MX6_PEP* thisPtr = pepGlobalContextPtr;
        pepGlobalContextPtr = nullptr;
        thisPtr->~MX6_PEP();
        ExFreePoolWithTag(thisPtr, MX6PEP_POOL_TAG);
    }

    WPP_CLEANUP(DriverObjectPtr);
}

_Use_decl_annotations_
NTSTATUS InitializePepDevice ()
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);
    PAGED_CODE();

    NTSTATUS status;
    UINT32 cpuRev;

    //
    // The PEP could be loaded on SOC families other than IMX6
    // because it is included in UpdateOS, and UpdateOS is shared
    // between IMX6 and 7. Only proceed with initialization
    // if we're on IMX6.
    //
    status = ImxGetCpuRev(&cpuRev);
    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR("Failed to get CPU rev/type.");
        return status;
    }

    if (IMX_SOC_TYPE(cpuRev) != IMX_SOC_MX6) {
        MX6_LOG_ERROR(
            "Skipping initialization of PEP on non-IMX6 chip. "
            "(cpuRev = 0x%x)",
            cpuRev);

        return STATUS_NOT_SUPPORTED;
    }

    auto deviceContextPtr = static_cast<MX6_PEP*>(
        ExAllocatePoolWithTag(
            NonPagedPoolNx,
            sizeof(MX6_PEP),
            MX6PEP_POOL_TAG
            )
    );

    if (deviceContextPtr == nullptr) {
        MX6_LOG_LOW_MEMORY("Failed to map memory for MX6 class.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceContextPtr, sizeof(*deviceContextPtr));
    deviceContextPtr->MX6_PEP::MX6_PEP();

    status = deviceContextPtr->MX6_PEP::InitializeResources();
    if (!NT_SUCCESS(status)) {
        MX6_LOG_ERROR(
            "InitializeResources() failed. (status = %!STATUS!)",
            status);
        deviceContextPtr->MX6_PEP::~MX6_PEP();
        ExFreePoolWithTag(deviceContextPtr, MX6PEP_POOL_TAG);
        return status;
    }

    return STATUS_SUCCESS;
}

MX6_PAGED_SEGMENT_END; //===================================================
MX6_INIT_SEGMENT_BEGIN; //==================================================

_Use_decl_annotations_
NTSTATUS DriverEntry (
    DRIVER_OBJECT* DriverObjectPtr,
    UNICODE_STRING* RegistryPathPtr
    )
{
    PAGED_CODE();

    // Initialize logging
    {
        WPP_INIT_TRACING(DriverObjectPtr, RegistryPathPtr);
        RECORDER_CONFIGURE_PARAMS recorderConfigureParams;
        RECORDER_CONFIGURE_PARAMS_INIT(&recorderConfigureParams);
        WppRecorderConfigure(&recorderConfigureParams);
#if DBG
        WPP_RECORDER_LEVEL_FILTER(MX6_TRACING_DEFAULT) = TRUE;
#endif // DBG
    }

    DriverObjectPtr->DriverExtension->AddDevice = MX6_PEP::AddDevice;
    DriverObjectPtr->DriverUnload = MX6PepDriverUnload;

    DriverObjectPtr->MajorFunction[IRP_MJ_PNP] = MX6_PEP::DispatchPnp;
    DriverObjectPtr->MajorFunction[IRP_MJ_POWER] = MX6_PEP::DispatchUnhandledIrp;
    DriverObjectPtr->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = MX6_PEP::DispatchUnhandledIrp;
    DriverObjectPtr->MajorFunction[IRP_MJ_CREATE] = MX6_PEP::DispatchFileCreateClose;
    DriverObjectPtr->MajorFunction[IRP_MJ_CLOSE] = MX6_PEP::DispatchFileCreateClose;
    DriverObjectPtr->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MX6_PEP::DispatchDeviceIoControl;

    // Initialize and Register Pep
    NTSTATUS status = InitializePepDevice();
    if (!NT_SUCCESS(status)) {
        WPP_CLEANUP(DriverObjectPtr);
        return status;
    }

    return STATUS_SUCCESS;
}

MX6_INIT_SEGMENT_END; //====================================================

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6ppm.cpp
//
// Abstract:
//
//   IMX6 PEP Processor Power Management Routines
//

#include "precomp.h"

#include "trace.h"
#include "mx6ppm.tmh"

#include "mx6peputil.h"
#include "mx6pepioctl.h"
#include "mx6pephw.h"
#include "mx6pep.h"

MX6_NONPAGED_SEGMENT_BEGIN; //==============================================

extern "C" BOOLEAN enumerateUnmaskedInterruptsCallback (
    VOID* CallbackContextPtr,
    PEP_UNMASKED_INTERRUPT_INFORMATION* InterruptInformationPtr
    )
{
    const ULONG gsiv = InterruptInformationPtr->Gsiv;
    if (gsiv < 32) {            // lowest IRQ number
        return TRUE;
    } else if (gsiv > 159) {    // highest IRQ number
        return FALSE;
    }

    auto unmaskedInterruptsPtr =
        static_cast<MX6_PEP::UNMASKED_INTERRUPTS*>(CallbackContextPtr);

    unmaskedInterruptsPtr->Mask[(gsiv - 32) / 32] |= (1 << (gsiv % 32));
    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmQueryCapabilities (
    PEPHANDLE Handle,
    PEP_PPM_QUERY_CAPABILITIES* ArgsPtr
    )
{
    // DBG
    {
        _DEVICE_ID deviceId = pepDeviceIdFromPepHandle(Handle);
        UNREFERENCED_PARAMETER(deviceId);
        NT_ASSERT(
            (deviceId == _DEVICE_ID::CPU0) ||
            (deviceId == _DEVICE_ID::CPU1) ||
            (deviceId == _DEVICE_ID::CPU2) ||
            (deviceId == _DEVICE_ID::CPU3));
    }

    ArgsPtr->FeedbackCounterCount = 0;
    ArgsPtr->IdleStateCount = CPU_IDLE_STATE_COUNT;
    ArgsPtr->PerformanceStatesSupported = FALSE;
    ArgsPtr->ParkingSupported = FALSE;

    ++this->activeProcessorCount;

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmQueryIdleStatesV2 (
    PEPHANDLE /*Handle*/,
    PEP_PPM_QUERY_IDLE_STATES_V2* ArgsPtr
    )
{
    NT_ASSERT(ArgsPtr->Count == CPU_IDLE_STATE_COUNT);

    //
    // WFI State
    //
    {
        PEP_PROCESSOR_IDLE_STATE_V2* statePtr =
                &ArgsPtr->IdleStates[CPU_IDLE_STATE_WFI];

        statePtr->Interruptible = TRUE;
        statePtr->CacheCoherent = TRUE;
        statePtr->ThreadContextRetained = TRUE;
        statePtr->CStateType = 0;
        statePtr->WakesSpuriously = TRUE;
        statePtr->PlatformOnly = FALSE;
        statePtr->Autonomous = FALSE;
        statePtr->Latency = 0;
        statePtr->BreakEvenDuration = 0;
    }

    //
    // Second processor idle state also representing WFI, to be used as
    // a dependency of coordinated idle states
    //
    {
        PEP_PROCESSOR_IDLE_STATE_V2* statePtr =
                &ArgsPtr->IdleStates[CPU_IDLE_STATE_WFI2];

        statePtr->Interruptible = TRUE;
        statePtr->CacheCoherent = TRUE;
        statePtr->ThreadContextRetained = TRUE;
        statePtr->CStateType = 0;
        statePtr->WakesSpuriously = TRUE;
        statePtr->PlatformOnly = FALSE;
        statePtr->Autonomous = FALSE;
        statePtr->Latency = 0;
        statePtr->BreakEvenDuration = 0;
    }

    //
    // Processor idle state representing power gated state in which all context
    // is lost.
    //
    {
        PEP_PROCESSOR_IDLE_STATE_V2* statePtr =
                &ArgsPtr->IdleStates[CPU_IDLE_STATE_POWER_GATED];

        statePtr->Interruptible = TRUE;
        statePtr->CacheCoherent = FALSE;
        statePtr->ThreadContextRetained = FALSE;
        statePtr->CStateType = 0;

        //
        // Pending interrupt when WFI is executed could cause execution to
        // continue instead of entering STOP mode
        //
        statePtr->WakesSpuriously = TRUE;
        statePtr->PlatformOnly = TRUE;
        statePtr->Autonomous = FALSE;
        statePtr->Latency = 0;
        statePtr->BreakEvenDuration = 0;
    }

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmQueryPlatformStates (
    PEPHANDLE /*Handle*/,
    PEP_PPM_QUERY_PLATFORM_STATES* ArgsPtr
    )
{
    ArgsPtr->PlatformStateCount = PLATFORM_IDLE_STATE_COUNT;
    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmQueryCoordinatedStates (
    PEPHANDLE /*Handle*/,
    PEP_PPM_QUERY_COORDINATED_STATES* ArgsPtr
    )
{
    NT_ASSERT(ArgsPtr->Count == PLATFORM_IDLE_STATE_COUNT);

    //
    // WAIT mode
    //
    {
        PEP_COORDINATED_IDLE_STATE* statePtr =
                &ArgsPtr->States[PLATFORM_IDLE_STATE_WAIT];

        statePtr->Latency = 0;
        statePtr->BreakEvenDuration = 0;
        statePtr->DependencyCount = this->activeProcessorCount;
        statePtr->MaximumDependencySize = 1;
    }

    //
    // STOP (light) state
    //
    {
        PEP_COORDINATED_IDLE_STATE* statePtr =
                &ArgsPtr->States[PLATFORM_IDLE_STATE_STOP_LIGHT];

        statePtr->Latency = 500;                // 50us
        statePtr->BreakEvenDuration = 0;
        statePtr->DependencyCount = this->activeProcessorCount;
        statePtr->MaximumDependencySize = 1;
    }

    //
    // STOP (deep) state - all clocks on SOC turned off, ARM domain power
    // removed, standby voltage applied, DDR in self refresh
    //
    {
        PEP_COORDINATED_IDLE_STATE* statePtr =
                &ArgsPtr->States[PLATFORM_IDLE_STATE_ARM_OFF];

        statePtr->Latency = 10000;              // 1ms
        statePtr->BreakEvenDuration = 10000;
        statePtr->DependencyCount = this->activeProcessorCount;
        statePtr->MaximumDependencySize = 1;
    }

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmQueryCoordinatedDependency (
    PEPHANDLE /*Handle*/,
    PEP_PPM_QUERY_COORDINATED_DEPENDENCY* ArgsPtr
    )
{
    switch (ArgsPtr->StateIndex) {
    case PLATFORM_IDLE_STATE_WAIT:
    case PLATFORM_IDLE_STATE_STOP_LIGHT:
    {
        switch (ArgsPtr->DependencyIndex) {
        case 0:     // CPU0
        case 1:     // CPU1
        case 2:     // CPU2
        case 3:     // CPU3
        {
            //
            // Should match MaximumDependencySize above
            //
            NT_ASSERT(ArgsPtr->DependencySize == 1);

            const _DEVICE_ID deviceId =
                    deviceIdFromDependencyIndex(ArgsPtr->DependencyIndex);

            const _DEVICE_CONTEXT* deviceContextPtr =
                    this->contextFromDeviceId(deviceId);

            ArgsPtr->DependencySizeUsed = 1;
            ArgsPtr->TargetProcessor = deviceContextPtr->KernelHandle;
            ArgsPtr->Options[0].ExpectedStateIndex = CPU_IDLE_STATE_WFI2;

            //
            // must be TRUE since WakesSpuriously flag is set
            //
            ArgsPtr->Options[0].LooseDependency = TRUE;

            //
            // OS may simultaneously enter the state referred to by this dependency
            //
            ArgsPtr->Options[0].InitiatingState = TRUE;

            //
            // Dependent state is a valid state for target processor to be in
            //
            ArgsPtr->Options[0].DependentState = TRUE;

            break;
        }

        default:
            NT_ASSERT(!"Invalid dependency index");
            return FALSE;
        }
        break;
    }  // PLATFORM_IDLE_STATE_WAIT, PLATFORM_IDLE_STATE_STOP_LIGHT

    case PLATFORM_IDLE_STATE_ARM_OFF:
    {
        switch (ArgsPtr->DependencyIndex) {
        case 0:     // CPU0
        case 1:     // CPU1
        case 2:     // CPU2
        case 3:     // CPU3
        {
            //
            // Should match MaximumDependencySize above
            //
            NT_ASSERT(ArgsPtr->DependencySize == 1);

            const _DEVICE_ID deviceId =
                    deviceIdFromDependencyIndex(ArgsPtr->DependencyIndex);

            const _DEVICE_CONTEXT* deviceContextPtr =
                    this->contextFromDeviceId(deviceId);

            ArgsPtr->DependencySizeUsed = 1;
            ArgsPtr->TargetProcessor = deviceContextPtr->KernelHandle;
            ArgsPtr->Options[0].ExpectedStateIndex = CPU_IDLE_STATE_POWER_GATED;

            //
            // OS must guarantee the processor remains in the expected idle
            // state for the duration of the platform idle transition.
            // Must be TRUE if WakesSpuriously is set.
            //
            ArgsPtr->Options[0].LooseDependency = TRUE;

            //
            // OS may simultaneously enter the state referred to by this dependency
            //
            ArgsPtr->Options[0].InitiatingState = TRUE;

            //
            // Dependent state is a valid state for target processor to be in
            //
            ArgsPtr->Options[0].DependentState = TRUE;

            break;
        }

        default:
            NT_ASSERT(!"Invalid dependency index");
            return FALSE;
        }
        break;
    }  // PLATFORM_IDLE_STATE_ARM_OFF

    default:
        NT_ASSERT(!"Invalid coordinated idle state index");
        return FALSE;
    } // switch (ArgsPtr->StateIndex)

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmIdleExecute (
    PEPHANDLE /*Handle*/,
    PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr
    )
{
    NT_ASSERT(ArgsPtr->Status == STATUS_SUCCESS);

    switch (ArgsPtr->PlatformState) {
    case PEP_PLATFORM_IDLE_STATE_NONE:
    {
        //
        // This is a processor-only transition
        //
        switch (ArgsPtr->ProcessorState) {
        case CPU_IDLE_STATE_WFI:
        case CPU_IDLE_STATE_WFI2:
        case CPU_IDLE_STATE_POWER_GATED:
            _DataSynchronizationBarrier();
            __wfi();
            return TRUE;

        default:
            NT_ASSERT(!"Invalid processor idle state");
            return FALSE;
        } // switch (ArgsPtr->ProcessorState)
    }

    case PLATFORM_IDLE_STATE_WAIT:
    {
        this->executePlatformIdleWait(ArgsPtr);
        return TRUE;
    }

    case PLATFORM_IDLE_STATE_STOP_LIGHT:
    {
        this->executePlatformIdleStopLight(ArgsPtr);
        return TRUE;
    }

    case PLATFORM_IDLE_STATE_ARM_OFF:
    {
        this->executePlatformIdleArmOff(ArgsPtr);
        return TRUE;
    }

    default:
        NT_ASSERT(!"Invalid platform idle state");
        return FALSE;
    } // ArgsPtr->PlatformState
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmIdleComplete (
    PEPHANDLE /*Handle*/,
    PEP_PPM_IDLE_COMPLETE_V2* ArgsPtr
    )
{
    switch (ArgsPtr->PlatformState) {
    case PLATFORM_IDLE_STATE_ARM_OFF:

        //
        // Notify system that GPT is back online
        //
        this->pepKernelInfo.TransitionCriticalResource(
            this->contextFromDeviceId(_DEVICE_ID::GPT)->KernelHandle,
            0,
            TRUE);

        break;

    case PLATFORM_IDLE_STATE_STOP_LIGHT:

        //
        // Notify system that GPT is back online
        //
        this->pepKernelInfo.TransitionCriticalResource(
            this->contextFromDeviceId(_DEVICE_ID::GPT)->KernelHandle,
            0,
            TRUE);

        __fallthrough;

    case PLATFORM_IDLE_STATE_WAIT:

        MX6_CCM_CLPCR_REG clpcr =
            { READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CLPCR) };

        clpcr.LPM = MX6_CCM_CLPCR_LPM_RUN;
        WRITE_REGISTER_NOFENCE_ULONG(
            &this->ccmRegistersPtr->CLPCR,
            clpcr.AsUlong);

        break;
    }

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmQueryVetoReasons (
    PEPHANDLE /*Handle*/,
    PEP_PPM_QUERY_VETO_REASONS* ArgsPtr
    )
{
    ArgsPtr->VetoReasonCount = MX6_VETO_REASON_COUNT;
    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmQueryVetoReason (
    PEPHANDLE /*Handle*/,
    PEP_PPM_QUERY_VETO_REASON* ArgsPtr
    )
{
    static const PCWSTR vetoNames[] = {
        L"Debug break",                                 // MX6_VETO_REASON_DEBUGGER
        L"This state is intentionally disabled",        // MX6_VETO_REASON_DISABLED
    };

    static_assert(
        ARRAYSIZE(vetoNames) == MX6_VETO_REASON_COUNT,
        "Verifying size of vetoNames array");

    NT_ASSERT(
        (ArgsPtr->VetoReason < MX6_VETO_REASON_MAX) &&
        ArgsPtr->VetoReason <= ARRAYSIZE(vetoNames));

    //
    // First call is for the length of the string, second call is for
    // the actual string.
    //
    if (ArgsPtr->Name == nullptr) {
        ArgsPtr->NameSize =
            static_cast<USHORT>(wcslen(vetoNames[ArgsPtr->VetoReason - 1]) + 1);

    } else {
        NTSTATUS status = RtlStringCchCopyW(
                ArgsPtr->Name,
                ArgsPtr->NameSize,
                vetoNames[ArgsPtr->VetoReason - 1]);

        UNREFERENCED_PARAMETER(status);
        NT_ASSERT(NT_SUCCESS(status));
    }

    return TRUE;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN MX6_PEP::PpmEnumerateBootVetoes (PEPHANDLE /*Handle*/)
{
    NTSTATUS status;
    POHANDLE cpu0Handle =
            this->contextFromDeviceId(_DEVICE_ID::CPU0)->KernelHandle;

    //
    // Disable stop light mode due to always-on counter dependency
    //
    status = this->pepKernelInfo.PlatformIdleVeto(
            cpu0Handle,
            PLATFORM_IDLE_STATE_STOP_LIGHT,
            MX6_VETO_REASON_DISABLED,
            TRUE);

    UNREFERENCED_PARAMETER(status);
    NT_ASSERT(NT_SUCCESS(status));

    //
    // ARM off platform idle state disabled due to lack of PSCI support
    //
    status = this->pepKernelInfo.PlatformIdleVeto(
            cpu0Handle,
            PLATFORM_IDLE_STATE_ARM_OFF,
            MX6_VETO_REASON_DISABLED,
            TRUE);

    UNREFERENCED_PARAMETER(status);
    NT_ASSERT(NT_SUCCESS(status));

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::PpmTestIdleState (
    PEPHANDLE /*Handle*/,
    PEP_PPM_TEST_IDLE_STATE* /*ArgsPtr*/
    )
{
    return TRUE;
}

MX6_PEP::_DEVICE_ID MX6_PEP::deviceIdFromDependencyIndex (ULONG DependencyIndex)
{
    switch (DependencyIndex) {
    case 0: return _DEVICE_ID::CPU0;
    case 1: return _DEVICE_ID::CPU1;
    case 2: return _DEVICE_ID::CPU2;
    case 3: return _DEVICE_ID::CPU3;
    default:
        NT_ASSERT(FALSE);
        return _DEVICE_ID::_INVALID;
    }
}

_Use_decl_annotations_
BOOLEAN MX6_PEP::uartDebuggerIsr (
    PKINTERRUPT /*InterruptPtr*/,
    PVOID ContextPtr
    )
{
    BOOLEAN claimed = FALSE;

    auto thisPtr = static_cast<MX6_PEP*>(ContextPtr);

    ULONG usr1 = READ_REGISTER_NOFENCE_ULONG(&thisPtr->debugger.uartRegistersPtr->Usr1);

    if ((usr1 & IMX_UART_USR1_AWAKE) != 0) {

        //
        // Disable AWAKE interrupt
        //
        WRITE_REGISTER_NOFENCE_ULONG(
            &thisPtr->debugger.uartRegistersPtr->Ucr3,
            IMX_UART_UCR3_RXDMUXSEL);

        //
        // If veto is not currently active, queue a DPC/work item to request a veto
        //
        if (thisPtr->debugger.vetoActive == false) {
            thisPtr->debugger.vetoActive = true;
            IoRequestDpc(thisPtr->pdoPtr, nullptr, thisPtr);
        }

        WRITE_REGISTER_NOFENCE_ULONG(
            &thisPtr->debugger.uartRegistersPtr->Usr1,
            IMX_UART_USR1_AWAKE);

        claimed = TRUE;
    }

    return claimed;
}

_Use_decl_annotations_
VOID
MX6_PEP::uartDebuggerDpc (
    PKDPC /*Dpc*/,
    struct _DEVICE_OBJECT * /*DeviceObject*/,
    struct _IRP * /*Irp*/,
    PVOID ContextPtr
    )
{
    auto thisPtr = static_cast<MX6_PEP*>(ContextPtr);

    //
    // Queue a work item to increment the "debug break" veto
    //
    thisPtr->debugger.vetoTimerFired = false;
    IoQueueWorkItem(
        thisPtr->debugger.toggleVetoWorkItem,
        debugVetoWorkItemRoutine,
        DelayedWorkQueue,
        thisPtr);
}

//
// This work item routine is called to both increment and decrement the
// "Debug break" veto. If the veto expiration timer has not yet fired,
// increment the veto. If the work item has been invoked due to the veto
// expiration timer, decrement the veto.
//
_Use_decl_annotations_
VOID
MX6_PEP::debugVetoWorkItemRoutine (
    DEVICE_OBJECT* /*DeviceObjectPtr*/,
    PVOID ContextPtr
    )
{
    MX6_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = static_cast<MX6_PEP*>(ContextPtr);

    //
    // Veto all platform STOP modes for 10 seconds
    //
    POHANDLE cpu0Handle =
            thisPtr->contextFromDeviceId(_DEVICE_ID::CPU0)->KernelHandle;

    for (ULONG platformState = PLATFORM_IDLE_STATE_STOP_LIGHT;
         platformState < PLATFORM_IDLE_STATE_COUNT;
         ++platformState) {

        NTSTATUS status = thisPtr->pepKernelInfo.PlatformIdleVeto(
                cpu0Handle,
                platformState,
                MX6_VETO_REASON_DEBUGGER,
                !thisPtr->debugger.vetoTimerFired);

        UNREFERENCED_PARAMETER(status);
        NT_ASSERT(NT_SUCCESS(status));
    }

    //
    // Set a timer to clear the veto after 10 seconds
    //
    if (thisPtr->debugger.vetoTimerFired == false) {
        LARGE_INTEGER dueTime;
        dueTime.QuadPart = -10LL * 10000000;     // 10 seconds
        KeSetTimer(
            &thisPtr->debugger.clearVetoTimer,
            dueTime,
            &thisPtr->debugger.clearVetoDpc);

    } else {
        thisPtr->debugger.vetoActive = false;
    }
}

_Use_decl_annotations_
VOID
MX6_PEP::vetoTimerDpcRoutine (
    struct _KDPC * /*Dpc*/,
    PVOID DeferredContext,
    PVOID /*SystemArgument1*/,
    PVOID /*SystemArgument2*/
    )
{
    auto thisPtr = static_cast<MX6_PEP*>(DeferredContext);

    //
    // Request a work item to decrement the veto condition
    //
    thisPtr->debugger.vetoTimerFired = true;
    IoQueueWorkItem(
        thisPtr->debugger.toggleVetoWorkItem,
        debugVetoWorkItemRoutine,
        DelayedWorkQueue,
        thisPtr);
}

_Use_decl_annotations_
void MX6_PEP::executePlatformIdleWait (PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr)
{
    //
    // This is a coordinated idle transition to WAIT mode
    //
    UNREFERENCED_PARAMETER(ArgsPtr);
    NT_ASSERT(ArgsPtr->PlatformState == PLATFORM_IDLE_STATE_WAIT);

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
    // Set LPM=WAIT, VSTBY=0, SBYOS=0
    //
    MX6_CCM_CLPCR_REG clpcr =
        { READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CLPCR) };

    clpcr.LPM = MX6_CCM_CLPCR_LPM_WAIT;
    clpcr.ARM_clk_dis_on_lpm = 1;
    clpcr.SBYOS = 0;
    clpcr.VSTBY = 0;

    this->writeClpcrWaitStop(clpcr.AsUlong);

    this->updateGpcInterruptController();

    //
    // Execute WFI, triggering entry to WAIT mode
    //
    _DataSynchronizationBarrier();
    __wfi();
}

_Use_decl_annotations_
void MX6_PEP::executePlatformIdleStopLight (PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr)
{
    //
    // This is a coordinated idle transition to STOP mode
    //
    UNREFERENCED_PARAMETER(ArgsPtr);
    NT_ASSERT(ArgsPtr->PlatformState == PLATFORM_IDLE_STATE_STOP_LIGHT);

    //
    // Enable the debugger UART to generate an AWAKE interrupt
    //
    this->enableDebuggerWake();

    //
    // Set LPM=STOP, VSTBY=0, SBYOS=0
    //
    MX6_CCM_CLPCR_REG clpcr =
        { READ_REGISTER_NOFENCE_ULONG(&this->ccmRegistersPtr->CLPCR) };

    clpcr.LPM = MX6_CCM_CLPCR_LPM_STOP;
    clpcr.ARM_clk_dis_on_lpm = 1;
    clpcr.SBYOS = 0;
    clpcr.VSTBY = 0;

    this->writeClpcrWaitStop(clpcr.AsUlong);

    //
    // Set GPC_PGC_CPU_PDN to 0, so that power does not get removed from CPU
    //
    WRITE_REGISTER_NOFENCE_ULONG(&this->gpcRegistersPtr->PGC_CPU.CTRL, 0);

    //
    // Configure PMU_MISC0 for light sleep mode
    //
    WRITE_REGISTER_NOFENCE_ULONG(
        &this->analogRegistersPtr->MISC0_SET,
        MX6_PMU_MISC0_STOP_MODE_CONFIG);

    this->updateGpcInterruptController();

    //
    // Notify system that GPT is going offline
    //
    this->pepKernelInfo.TransitionCriticalResource(
        this->contextFromDeviceId(_DEVICE_ID::GPT)->KernelHandle,
        0,
        FALSE);

    //
    // Execute WFI, triggering entry to STOP mode
    //
    _DataSynchronizationBarrier();
    __wfi();
}

_Use_decl_annotations_
void MX6_PEP::executePlatformIdleArmOff (PEP_PPM_IDLE_EXECUTE_V2* ArgsPtr)
{
    //
    // This is a coordinated idle transition to STOP mode
    //
    UNREFERENCED_PARAMETER(ArgsPtr);
    NT_ASSERT(ArgsPtr->PlatformState == PLATFORM_IDLE_STATE_ARM_OFF);

    //
    // Enable the debugger UART to generate an AWAKE interrupt
    //
    this->enableDebuggerWake();

    this->updateGpcInterruptController();

    //
    // Notify system that GPT is going offline
    //
    this->pepKernelInfo.TransitionCriticalResource(
        this->contextFromDeviceId(_DEVICE_ID::GPT)->KernelHandle,
        0,
        FALSE);

    PSCI_CPU_SUSPEND_POWER_STATE state = {};
    state.StateId = 1;
    state.StateType = PSCI_CPU_SUSPEND_POWER_STATE_TYPE_POWER_DOWN;

    this->pepKernelInfo.ProcessorHalt(
            PROCESSOR_HALT_VIA_PSCI_CPU_SUSPEND | PROCESSOR_HALT_CACHE_FLUSH_OVERRIDE,
            &state,
            nullptr);
}

_Use_decl_annotations_
NTSTATUS MX6_PEP::updateGpcInterruptController ()
{
    UNMASKED_INTERRUPTS unmaskedInterrupts = { 0 };
    PEP_UNMASKED_INTERRUPT_INFORMATION info;
    info.Version = PEP_UNMASKED_INTERRUPT_INFORMATION_V1;
    info.Size = sizeof(info);
    NTSTATUS status = this->pepKernelInfo.EnumerateUnmaskedInterrupts(
            nullptr,
            PEP_ENUMERATE_UNMASKED_INTERRUPT_FLAGS_NONE,
            enumerateUnmaskedInterruptsCallback,
            &unmaskedInterrupts,
            &info);

    if (!NT_SUCCESS(status)) {

        //
        // This will result in a bugcheck
        //
        NT_ASSERT(FALSE);
        return status;
    }

    //
    // Program changed values into IMR registers
    //
    for (ULONG i = 0; i < ARRAYSIZE(unmaskedInterrupts.Mask); ++i) {
        if (unmaskedInterrupts.Mask[i] != this->unmaskedInterruptsCopy.Mask[i]) {
            this->unmaskedInterruptsCopy.Mask[i] = unmaskedInterrupts.Mask[i];
            WRITE_REGISTER_NOFENCE_ULONG(
                &this->gpcRegistersPtr->IMR[i],
                ~unmaskedInterrupts.Mask[i]);
        }
    }

    return STATUS_SUCCESS;
}

//
// Use the sequence in Errata ERR007265 to set LPM to WAIT or STOP mode.
//
void MX6_PEP::writeClpcrWaitStop (ULONG Clpcr)
{
    //
    // Precondition: GINT should be asserted
    //
    NT_ASSERT(
        (READ_REGISTER_NOFENCE_ULONG(&this->iomuxcRegistersPtr->Gpr[1]) &
         (1 << 12)) != 0);

    const ULONG iomuxcIrqNum = 32;

    //
    // Unmask IOMUX interrupt in GPC IMR
    //
    const ULONG imrIndex = (iomuxcIrqNum - 32) / 32;
    const ULONG imr = ~this->unmaskedInterruptsCopy.Mask[imrIndex];
    WRITE_REGISTER_NOFENCE_ULONG(
        &this->gpcRegistersPtr->IMR[imrIndex],
        imr & ~(1 << (iomuxcIrqNum % 32)));

    //
    // Update CLPCR
    //
    WRITE_REGISTER_NOFENCE_ULONG(
        &this->ccmRegistersPtr->CLPCR,
        Clpcr);

    //
    // Mask IOMUX interrupt in GPC IMR
    //
    WRITE_REGISTER_NOFENCE_ULONG(
        &this->gpcRegistersPtr->IMR[imrIndex],
        imr | (1 << (iomuxcIrqNum % 32)));
}

//
// Enable the debugger UART to generate an AWAKE interrupt
//
void MX6_PEP::enableDebuggerWake ()
{
    if (this->debugger.uartRegistersPtr == nullptr)
        return;

    WRITE_REGISTER_NOFENCE_ULONG(
        &this->debugger.uartRegistersPtr->Usr1,
        IMX_UART_USR1_AWAKE);

    WRITE_REGISTER_NOFENCE_ULONG(
        &this->debugger.uartRegistersPtr->Ucr3,
        IMX_UART_UCR3_RXDMUXSEL |
        IMX_UART_UCR3_AWAKEN);
}

void MX6_PEP::maskGpcInterrupts ()
{
    for (ULONG i = 0; i < ARRAYSIZE(this->gpcRegistersPtr->IMR); ++i) {
        WRITE_REGISTER_NOFENCE_ULONG(
            &this->gpcRegistersPtr->IMR[i],
            0xffffffff);
    }
}

void MX6_PEP::unmaskGpcInterrupts ()
{
    for (ULONG i = 0; i < ARRAYSIZE(this->gpcRegistersPtr->IMR); ++i) {
        WRITE_REGISTER_NOFENCE_ULONG(
            &this->gpcRegistersPtr->IMR[i],
            ~this->unmaskedInterruptsCopy.Mask[i]);
    }
}

MX6_NONPAGED_SEGMENT_END; //================================================


/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    HalExtension.c

Abstract:

    This file implements a HAL Extension Module for the i.MX SDMA
    DMA controller.

*/

//
// ------------------------------------------------------------------- Includes
//

#include <nthalext.h>

#pragma warning(disable:4201)   // nameless struct/union

#include <ImxCpuRev.h>
#include "sdma_imx6q_script_code.h"
#include "sdma_imx7d_script_code.h"
#include "HalExtiMXDmaCfg.h"
#include "ImxSdmaHw.h"
#include "HalExtiMXDma.h"

//
// Platform specific
//
#include "IMX6QdDmaHw.h"
#include "IMX6SxDmaHw.h"
#include "IMX6UlldmaHw.h"
#include "IMX8MDmaHw.h"

//
// ------------------------------------------------- Data Structure Definitions
//

#if DBG
    typedef struct _SDMP_REG_DUMP {
        const char* NameSz;
        ULONG Offset;
        ULONG Value;
    } SDMP_REG_DUMP;
#endif // DBG

//
// -------------------------------------------------------------------- Globals
//


DMA_FUNCTION_TABLE SdmaFunctionTable =
{
    SdmaInitializeController,
    SdmaValidateRequestLineBinding,
    SdmaQueryMaxFragments,
    SdmaProgramChannel,
    SdmaConfigureChannel,
    SdmaFlushChannel,
    SdmaHandleInterrupt,
    SdmaReadDmaCounter,
    SdmaReportCommonBuffer,
    SdmaCancelTransfer
};

#if DBG
    SDMP_REG_DUMP SdmaRegsDump[] = {
        { "MC0PTR", FIELD_OFFSET(SDMA_REGS, MC0PTR), 0 },
        { "INTR", FIELD_OFFSET(SDMA_REGS, INTR), 0 },
        { "STOP_STAT", FIELD_OFFSET(SDMA_REGS, STOP_STAT), 0 },
        { "HSTART", FIELD_OFFSET(SDMA_REGS, HSTART), 0 },
        { "EVTOVR", FIELD_OFFSET(SDMA_REGS, EVTOVR), 0 },
        { "DSPOVR", FIELD_OFFSET(SDMA_REGS, DSPOVR), 0 },
        { "HOSTOVR", FIELD_OFFSET(SDMA_REGS, HOSTOVR), 0 },
        { "EVTERR", FIELD_OFFSET(SDMA_REGS, EVTERR), 0 },
        { "INTRMASK", FIELD_OFFSET(SDMA_REGS, INTRMASK), 0 },
        { "EVTERRDBG", FIELD_OFFSET(SDMA_REGS, EVTERRDBG), 0 },
        { "CONFIG", FIELD_OFFSET(SDMA_REGS, CONFIG), 0 },
        { "CHN0ADDR", FIELD_OFFSET(SDMA_REGS, CHN0ADDR), 0 },
        { "EVT_MIRROR", FIELD_OFFSET(SDMA_REGS, EVT_MIRROR), 0 },
        { "EVT_MIRROR2", FIELD_OFFSET(SDMA_REGS, EVT_MIRROR2), 0 }
    };
#endif


//
// ------------------------------------------------------------------ Functions
//


NTSTATUS
AddResourceGroup (
    _In_ ULONG Handle,
    _In_ PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    )

/*++

Routine Description:

    This is the entry point for the DMA HAL extension.
    It registers the SDMA controller and channels with the HAL.

Arguments:

    Handle - Supplies the HAL Extension handle which must be passed to other
        HAL Extension APIs.

    ResourceGroup - Supplies a pointer to the Resource Group which the
        HAL Extension has been installed on.

Return Value:

    NTSTATUS code.

--*/

{

    const CSRT_RESOURCE_DESCRIPTOR_SDMA_CONTROLLER* CsrtDmaDescPtr;
    DMA_CHANNEL_INITIALIZATION_BLOCK DmaChannelInitBlock;
    DMA_INITIALIZATION_BLOCK DmaInitBlock;
    ULONG ChIndex;
    UINT32 CpuRev;
    IMX_CPU CpuType;
    PHYSICAL_ADDRESS PhysicalAddress;
    CSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptorHeader;
    SDMA_CONTROLLER SdmaController;
    ULONG ChannelId;
    NTSTATUS Status;

    Status = STATUS_UNSUCCESSFUL;
    ChannelId = 1000;   // start channel Uids at 1000.
    CsrtDmaDescPtr = NULL;

    //
    // Get the CPU type
    //
    Status = ImxGetCpuRev(&CpuRev);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    CpuType = IMX_CPU_TYPE(CpuRev);

    // iterate through SDMA controllers
    for (;;)
    {
        //
        // Get the SDMA controller resource descriptor
        //

#pragma prefast(suppress: 25024, "CSRT_RESOURCE_DESCRIPTOR_SDMA_CONTROLLER starts with CSRT_RESOURCE_DESCRIPTOR_HEADER")
        CsrtDmaDescPtr = (const CSRT_RESOURCE_DESCRIPTOR_SDMA_CONTROLLER*)
            GetNextResourceDescriptor(Handle,
                                      ResourceGroup,
                                      (PCSRT_RESOURCE_DESCRIPTOR_HEADER)CsrtDmaDescPtr,
                                      CSRT_RD_TYPE_DMA,
                                      CSRT_RD_SUBTYPE_DMA_CONTROLLER,
                                      CSRT_RD_UID_ANY);

        if (CsrtDmaDescPtr == NULL) {
            break;
        }

        //
        // Initialize the controller internal data
        //

        RtlZeroMemory(&SdmaController, sizeof(SdmaController));
        SdmaController.SdmaRegsAddress.QuadPart = CsrtDmaDescPtr->RegistersBaseAddress;
        SdmaController.IoMuxGPR0RegAddress.QuadPart = CsrtDmaDescPtr->IoMuxGPR0Address;
        SdmaController.CoreClockRatio = CsrtDmaDescPtr->SdmaCoreClockRatio;
        SdmaController.ControllerStatus = STATUS_DEVICE_NOT_READY;
        SdmaController.SdmaInstance = CsrtDmaDescPtr->Header.Uid;

        //
        // Get the DMA request mapping for the our SOC flavor and SDMA instance
        //

        Status = SdmaInitDmaReqMapping(&SdmaController, CpuType, CsrtDmaDescPtr->Header.Uid);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // Register this physical address space with the HAL.
        //

        PhysicalAddress.QuadPart = (LONGLONG)CsrtDmaDescPtr->RegistersBaseAddress;
        Status = HalRegisterPermanentAddressUsage(PhysicalAddress,
                                                  sizeof(SDMA_REGS));

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        PhysicalAddress.QuadPart = (LONGLONG)CsrtDmaDescPtr->IoMuxGPR0Address;
        Status = HalRegisterPermanentAddressUsage(PhysicalAddress,
                                                  sizeof(ULONG));

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        RtlZeroMemory(&DmaInitBlock, sizeof(DmaInitBlock));
        INITIALIZE_DMA_HEADER(&DmaInitBlock);
        // DmaInitBlock.ControllerId is set on return from RegisterResourceDescriptor
        DmaInitBlock.ChannelCount = SDMA_NUM_CHANNELS;
        DmaInitBlock.SupportedPortWidths = DMA_UNLIMITED_PORT_WIDTH |
                                           DMA_8_BIT_PORT_WIDTH |
                                           DMA_16_BIT_PORT_WIDTH |
                                           DMA_32_BIT_PORT_WIDTH;
        DmaInitBlock.MinimumTransferUnit = 1;
        DmaInitBlock.MinimumRequestLine = SdmaController.SdmaInstance << SDMA_INSTANCE_ID_SHIFT;
        DmaInitBlock.MaximumRequestLine = (SdmaController.SdmaInstance << SDMA_INSTANCE_ID_SHIFT) + SdmaController.SdmaReqMaxId;
        DmaInitBlock.CacheCoherent = FALSE;
        DmaInitBlock.GeneratesInterrupt = TRUE;
        DmaInitBlock.InternalData = (PVOID)&SdmaController;
        DmaInitBlock.InternalDataSize = sizeof(SdmaController);
        DmaInitBlock.DmaAddressWidth = 32;
        DmaInitBlock.Gsi = CsrtDmaDescPtr->Interrupt;
        DmaInitBlock.InterruptPolarity = InterruptActiveHigh;
        DmaInitBlock.InterruptMode = LevelSensitive;
        DmaInitBlock.Operations = &SdmaFunctionTable;

        //
        // Register controller.
        //

        RtlZeroMemory(&ResourceDescriptorHeader, sizeof(ResourceDescriptorHeader));
        ResourceDescriptorHeader.Type = CSRT_RD_TYPE_DMA;
        ResourceDescriptorHeader.Subtype = CSRT_RD_SUBTYPE_DMA_CONTROLLER;
        ResourceDescriptorHeader.Uid = SdmaController.SdmaInstance;
        ResourceDescriptorHeader.Length = sizeof(CSRT_RESOURCE_DESCRIPTOR_HEADER);

        Status = RegisterResourceDescriptor(Handle,
                                            ResourceGroup,
                                            &ResourceDescriptorHeader,
                                            &DmaInitBlock);

        if (Status != STATUS_SUCCESS) {
            return Status;
        }

        //
        //  Register all DMA channels
        //

        INITIALIZE_DMA_CHANNEL_HEADER(&DmaChannelInitBlock);
        ResourceDescriptorHeader.Subtype = CSRT_RD_SUBTYPE_DMA_CHANNEL;

        for (ChIndex = 0; ChIndex < SDMA_NUM_CHANNELS; ++ChIndex) {
            DmaChannelInitBlock.CommonBufferLength = sizeof(SDMA_CHANNEL);
            if (ChIndex == 0) {
                DmaChannelInitBlock.CommonBufferLength = sizeof(SDMA_CHANNEL0);
            }
            DmaChannelInitBlock.ControllerId = DmaInitBlock.ControllerId;
            DmaChannelInitBlock.GeneratesInterrupt = FALSE;
            DmaChannelInitBlock.ChannelNumber = ChIndex;

            ResourceDescriptorHeader.Uid = ChannelId++;
            Status = RegisterResourceDescriptor(Handle,
                                                ResourceGroup,
                                                &ResourceDescriptorHeader,
                                                &DmaChannelInitBlock);

            if (Status != STATUS_SUCCESS) {
                return Status;
            }
        }
    } // for (;;)

    return Status;
}


_Use_decl_annotations_
VOID
SdmaInitializeController (
    PVOID ControllerContextPtr
    )

/*++

Routine Description:

    This routine is called by the framework for DMA controllers to initialize.

Arguments:

    ControllerContextPtr - Supplies a pointer to the controller's internal data.

Return Value:

    None.

--*/

{

    SDMA_CONTROLLER* SdmaControllerPtr;

    SdmaControllerPtr = (SDMA_CONTROLLER*)ControllerContextPtr;

    if (SdmaControllerPtr->SdmaRegsPtr == NULL) {
        SdmaControllerPtr->SdmaRegsPtr = (volatile SDMA_REGS*)
            HalMapIoSpace(SdmaControllerPtr->SdmaRegsAddress,
                          ROUND_TO_PAGES(sizeof(SDMA_REGS)),
                          MmNonCached);
    }

    if (SdmaControllerPtr->IoMuxGPR0RegPtr == NULL) {
        SdmaControllerPtr->IoMuxGPR0RegPtr = (volatile ULONG*)
            HalMapIoSpace(SdmaControllerPtr->IoMuxGPR0RegAddress,
                          sizeof(ULONG),
                          MmNonCached);
    }

    NT_ASSERT(SdmaControllerPtr->SdmaRegsPtr != NULL);
    NT_ASSERT(SdmaControllerPtr->IoMuxGPR0RegPtr != NULL);

    SDMA_DUMP_REGS(SdmaControllerPtr);

    //
    // We cannot initialize the controller until we get channel0 resources.
    // Thus, SDMA controller initialization is deferred until
    // SdmaReportCommonBuffer() is called for channel0.
    //

    return;
}


_Use_decl_annotations_
BOOLEAN
SdmaValidateRequestLineBinding (
    PVOID ControllerContextPtr,
    PDMA_REQUEST_LINE_BINDING_DESCRIPTION BindingDescriptionPtr
    )

/*++

Routine Description:

    This routine queries a DMA controller extension to test the validity of a
    request line binding.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    BindingDescriptionPtr - Supplies a pointer to the request information.

Return Value:

    TRUE if the request line binding is valid and supported by the controller.

    FALSE if the binding is invalid.

Environment:

    PASSIVE_LEVEL.

--*/

{

    ULONG ChannelNumber;
    ULONG SdmaRequestLine;
    ULONG SdmaInstance;
    const SDMA_CHANNEL_CONFIG* SdmaChannelConfigPtr;
    const SDMA_CONTROLLER* SdmaControllerPtr;


    SdmaControllerPtr = (const SDMA_CONTROLLER*)ControllerContextPtr;
    ChannelNumber = BindingDescriptionPtr->ChannelNumber;
    SdmaRequestLine = BindingDescriptionPtr->RequestLine & SDMA_REQ_LINE_ID_MASK;
    SdmaInstance = BindingDescriptionPtr->RequestLine >> SDMA_INSTANCE_ID_SHIFT;

    if (!NT_SUCCESS(SdmaControllerPtr->ControllerStatus)) {
        return FALSE;
    }

    //
    // Channel 0 is only used for channel setup support.
    // In the future we may consider using it also as a normal
    // channel, if we need to.
    //

    if (ChannelNumber == 0) {
        return FALSE;
    }

    NT_ASSERT(SdmaControllerPtr->SdmaReqToChannelConfigPtr != NULL);
    NT_ASSERT(SdmaControllerPtr->SdmaReqMaxId != 0);

    if (SdmaInstance != SdmaControllerPtr->SdmaInstance) {
        return FALSE;
    }

    if (SdmaRequestLine > SdmaControllerPtr->SdmaReqMaxId) {
        return FALSE;
    }

    SdmaChannelConfigPtr =
        &SdmaControllerPtr->SdmaReqToChannelConfigPtr[SdmaRequestLine];

    if (SdmaChannelConfigPtr->SdmaScriptAddr == SDMA_UNSUPPORTED_REQUEST_ID) {
        return FALSE;
    }

    if (SdmaChannelConfigPtr->DmaRequestId != SdmaRequestLine) {
        NT_ASSERT(SdmaChannelConfigPtr->DmaRequestId == SdmaRequestLine);
        return FALSE;
    }

    if (SdmaChannelConfigPtr->SdmaInstance != SdmaInstance) {
        NT_ASSERT(SdmaChannelConfigPtr->SdmaInstance == SdmaInstance);
        return FALSE;
    }

    if (SdmaControllerPtr->ChannelsPtr[ChannelNumber] == NULL) {
        NT_ASSERT(SdmaControllerPtr->ChannelsPtr[ChannelNumber] != NULL);
        return FALSE;
    }

    if (SDMA_DEVICE_FLAG_ON(SdmaChannelConfigPtr->DeviceFlags,
                            SDMA_DEVICE_FLAG_P2P) &&
        (SdmaChannelConfigPtr->Peripheral2Address == 0)) {
        return FALSE;
    }

    return TRUE;
}


_Use_decl_annotations_
ULONG
SdmaQueryMaxFragments (
    PVOID ControllerContextPtr,
    ULONG ChannelNumber,
    ULONG MaxFragmentsRequested
    )

/*++

Routine Description:

    This routine queries the DMA extension to determine the number of
    scatter gather fragments that the next transfer can support.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - The target channel index.

    MaxFragmentsRequested - Supplies a hint to the maximum fragments useful to
        this transfer.

Return Value:

    Number of fragments the next transfer on this channel can support.

--*/

{

    UNREFERENCED_PARAMETER(ControllerContextPtr);
    UNREFERENCED_PARAMETER(MaxFragmentsRequested);


    if (ChannelNumber == 0) {
        return 0;
    }

    return SDMA_SG_LIST_MAX_SIZE;
}


_Use_decl_annotations_
VOID
SdmaProgramChannel (
    PVOID ControllerContextPtr,
    ULONG ChannelNumber,
    ULONG RequestLine,
    PDMA_SCATTER_GATHER_LIST MemoryAddressesPtr,
    PHYSICAL_ADDRESS DeviceAddress,
    BOOLEAN WriteToDevice,
    BOOLEAN LoopTransfer
    )

/*++

Routine Description:

    This routine programs a DMA controller channel for a specific transfer.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the number of the channel to program.

    RequestLine - Supplies the request line number to program.  This request
        line number is system-unique (as provided to the HAL during
        registration) and must be translated by the extension.

    MemoryAddress - Supplies the address to be programmed into the memory
        side of the channel configuration.

    DeviceAddress - Supplies the address to be programmed into the device
        side of the channel configuration.

    WriteToDevice - Supplies the direction of the transfer.

    LoopTransfer - Supplies whether AutoInitialize has been set in the
        adapter making this request.

Return Value:

    None.

--*/

{

    SDMA_CHANNEL* SdmaChannelPtr;
    SDMA_CHANNEL0* SdmaChannel0Ptr;
    SDMA_CONTROLLER* SdmaControllerPtr;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(WriteToDevice);

    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaControllerPtr = (SDMA_CONTROLLER*)ControllerContextPtr;
#pragma prefast(suppress: 25024, "Channel 0 context is SDMA_CHANNEL0*")
    SdmaChannel0Ptr = (SDMA_CHANNEL0*)SdmaControllerPtr->ChannelsPtr[0];
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];

    NT_ASSERT(SdmaChannel0Ptr != NULL);
    NT_ASSERT(SdmaChannelPtr != NULL);
    NT_ASSERT(SdmaChannelPtr->ChannelConfigPtr == NULL);

    //
    // Associate the DMA events that trigger the transfer
    // with the given channel.
    //

    Status = SdmaHwBindDmaEvents(SdmaControllerPtr, ChannelNumber, RequestLine);

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(NT_SUCCESS(Status));
        goto Done;
    }

    SdmaChannelPtr->IsAutoInitialize = LoopTransfer;
    SdmaChannelPtr->DeviceAddress = DeviceAddress;
    SdmaChannelPtr->AutoInitNextBufferIndex = 0;
    SdmaChannelPtr->AutoInitBytesTransferred = 0;
    SdmaChannelPtr->TransferLength = 0;
    SdmaChannelPtr->ActiveBufferCount = 0;

    NT_ASSERT(MemoryAddressesPtr->NumberOfElements <= SDMA_SG_LIST_MAX_SIZE);
    NT_ASSERT(SdmaChannelPtr->ChannelConfigPtr != NULL);

    //
    // Build the scatter gather list for the caller buffer
    //

    Status = SdmaHwConfigureSgList(SdmaChannelPtr,
                                   MemoryAddressesPtr,
                                   DeviceAddress,
                                   LoopTransfer);

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(Status == STATUS_INSUFFICIENT_RESOURCES);

        //
        // Not enough buffer descriptors to accommodate the desired
        // notification threshold, thus disable it.
        //

        SdmaChannelPtr->NotificationThreshold = 0;

        Status = SdmaHwConfigureSgList(SdmaChannelPtr,
                                       MemoryAddressesPtr,
                                       DeviceAddress,
                                       LoopTransfer);

        NT_ASSERT(NT_SUCCESS(Status));
    }

    //
    // Reset the current buffer descriptor address
    //

    SdmaChannel0Ptr->SdmaCCBs[ChannelNumber].CurrentBdAddress =
        SdmaChannel0Ptr->SdmaCCBs[ChannelNumber].BasedBdAddress;

    //
    // Setup the channel: set and load the context etc.
    //

    Status = SdmaHwSetupChannel(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(NT_SUCCESS(Status));
        goto Done;
    }

    //
    // Start the transfer
    //

    Status = SdmaHwStartChannel(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(NT_SUCCESS(Status));
        goto Done;
    }

Done:

    if (!NT_SUCCESS(Status)) {
        SdmaChannelPtr->ChannelConfigPtr = NULL;
    }

    return;
}


_Use_decl_annotations_
BOOLEAN
SdmaCancelTransfer (
    PVOID ControllerContextPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine must disable the selected channel.  The channel must not be
    capable of interrupting for this transfer after being cleared in this way.

    Note:
        The ISR's access to the extension is serialized with this callback.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel number.

Return Value:

    FALSE if the channel is already idle or if the channel is already asserting
    an interrupt.  TRUE is the channel is active and no interrupt is asserted.

--*/

{

    ULONG InterruptStatus;
    SDMA_CHANNEL_STATE OldChannelState;
    SDMA_CHANNEL* SdmaChannelPtr;
    SDMA_CONTROLLER* SdmaControllerPtr;
    volatile SDMA_REGS* SdmaRegsPtr;
    NTSTATUS Status;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaControllerPtr = (SDMA_CONTROLLER*)ControllerContextPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;
    OldChannelState = SdmaChannelPtr->State;

    SdmaChannelPtr->State = CHANNEL_ABORTING;

    //
    // Stop the transfer but do not clear the buffer resources, so
    // the they are kept valid until transfer is stopped.
    //

    Status = SdmaHwStopTransfer(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(NT_SUCCESS(Status));
    }

    //
    // Since the channel should not be able to generate an interrupt after
    // this callback returns, check if there is already an interrupt pending.
    // If there is an interrupt pending, add it to the stored interrupts in
    // the controller context and it will be handled by the next ISR call.
    // We can do it here due to the fact that ISR's access to the extension
    // is serialized with this routine.
    //

    InterruptStatus =
        SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->INTR) & (1 << ChannelNumber);
    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->INTR, InterruptStatus);

    SdmaControllerPtr->PendingInterrupts |= InterruptStatus;

    if ((SdmaControllerPtr->PendingInterrupts & (1 << ChannelNumber)) != 0) {
        return FALSE;
    }

    //
    // ISR will not be called for the transfer, clear transfer resources
    //

    SdmaHwStopChannel(SdmaControllerPtr, ChannelNumber);

    return OldChannelState == CHANNEL_RUNNING;
}


_Use_decl_annotations_
NTSTATUS
SdmaConfigureChannel (
    PVOID ControllerContextPtr,
    ULONG ChannelNumber,
    ULONG FunctionNumber,
    PVOID ContextPtr
    )

/*++

Routine Description:

    This routine configures the channel for a DMA extension specific operation.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel to configure.

    FunctionNumber - Supplies the ID of the operation to perform.

    Context - Supplies parameters for this operation.

Return Value:

    NTSTATUS code.

--*/

{

    BOOLEAN BoolValue;
    SDMA_CHANNEL* SdmaChannelPtr;
    SDMA_CONTROLLER* SdmaControllerPtr;
    ULONG UlongValue;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaControllerPtr = (SDMA_CONTROLLER*)ControllerContextPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    BoolValue = FALSE;

    switch (FunctionNumber) {
    case SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL:
        if (ContextPtr == NULL) {
            return STATUS_INVALID_PARAMETER;
        }

        if (SdmaChannelPtr->State == CHANNEL_RUNNING) {
            return STATUS_INVALID_DEVICE_STATE;
        }

        UlongValue = *((const ULONG*)ContextPtr);

        if (UlongValue >= SDMA_MAX_WATERMARK_LEVEL) {
            return STATUS_INVALID_PARAMETER;
        }

        SdmaChannelPtr->WatermarkLevel = UlongValue;
        return STATUS_SUCCESS;

    case SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE:
        BoolValue = TRUE;
        __fallthrough;

    case SDMA_CFG_FUN_RELEASE_REQUEST_LINE:
        if (ContextPtr == NULL) {
            return STATUS_INVALID_PARAMETER;
        }

        if (SdmaChannelPtr->State == CHANNEL_RUNNING) {
            return STATUS_INVALID_DEVICE_STATE;
        }

        UlongValue = *((const ULONG*)ContextPtr);

        if (UlongValue > SdmaControllerPtr->SdmaReqMaxId) {
            return STATUS_INVALID_PARAMETER;
        }

        return SdmaHwSetRequestLineOwnership(SdmaControllerPtr,
                                             ChannelNumber,
                                             UlongValue,
                                             BoolValue);

    case SDMA_CFG_FUN_SET_CHANNEL_NOTIFICATION_THRESHOLD:
        if (ContextPtr == NULL) {
            return STATUS_INVALID_PARAMETER;
        }

        if (SdmaChannelPtr->State == CHANNEL_RUNNING) {
            return STATUS_INVALID_DEVICE_STATE;
        }

        UlongValue = *((const ULONG*)ContextPtr);

        if (UlongValue >= SDMA_BD_MAX_COUNT) {
            return STATUS_INVALID_PARAMETER;
        }

        SdmaChannelPtr->NotificationThreshold = UlongValue;
        return STATUS_SUCCESS;

    default:
        NT_ASSERT(FALSE);
        break;
    }

    return STATUS_NOT_IMPLEMENTED;
}


_Use_decl_annotations_
VOID
SdmaFlushChannel (
    PVOID ControllerContextPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine resets the state of a channel and returns the
    channel to a state ready for the next ProgramChannel call.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel to flush.

Return Value:

    None.

--*/

{

    const SDMA_CHANNEL* SdmaChannelPtr;
    SDMA_CONTROLLER* SdmaControllerPtr;
    NTSTATUS Status;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaControllerPtr = (SDMA_CONTROLLER*)ControllerContextPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];

    if (SdmaChannelPtr->State != CHANNEL_ERROR) {
        NT_ASSERT(!SdmaHwIsChannelRunning(SdmaControllerPtr, ChannelNumber));
    }

    Status = SdmaHwStopChannel(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(NT_SUCCESS(Status));
    }

    return;
}


BOOLEAN
SdmaHandleInterrupt (
    PVOID ControllerContextPtr,
    PULONG ChannelNumber,
    PDMA_INTERRUPT_TYPE InterruptType
    )

/*++

Routine Description:

    This routine probes a controller for interrupts, clears any interrupts
    found, fills in channel and interrupt type information.  This routine
    will be called repeatedly until FALSE is returned.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies a placeholder for the extension to fill in which
        channel is interrupting.

    InterruptType - Supplies a placeholder for the extension to fill in the
        interrupt type.

Return Value:

    TRUE if an interrupt was found on this controller.

    FALSE otherwise.

--*/

{

    ULONG ChannelMask;
    ULONG ChannelIndex;
    ULONG Errors;
    ULONG InterruptStatus;
    SDMA_CHANNEL* SdmaChannelPtr;
    SDMA_CONTROLLER* SdmaControllerPtr;
    volatile SDMA_REGS* SdmaRegsPtr;


    SdmaControllerPtr = (SDMA_CONTROLLER*)ControllerContextPtr;
    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;

    //
    // Read interrupt status and acknowledge.
    // Check if this is the first call into this routine during this
    // interrupt assertion.  If not, then the controller status has
    // already been read and various channels are being singly reported.
    //

    InterruptStatus = SdmaControllerPtr->PendingInterrupts;

    if (InterruptStatus == 0) {
        InterruptStatus = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->INTR);
        SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->INTR, InterruptStatus);
    }

    //
    // Not interested in channel 0 interrupt.
    //

    InterruptStatus &= ~1;
    if (InterruptStatus == 0) {
        return FALSE;
    }

    //
    // Read and clear errors.
    // We only populate buffer descriptors errors.
    //

    Errors = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->EVTERR);

    //
    // Start with channel 1
    //

    ChannelMask = (1 << 1);

    for (ChannelIndex = 1;
         (InterruptStatus != 0) && (ChannelIndex < SDMA_NUM_CHANNELS);
         ++ChannelIndex, ChannelMask <<= 1) {

        if ((InterruptStatus & ChannelMask) == 0) {
            continue;
        }
        InterruptStatus &= ~ChannelMask;

        SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelIndex];

        switch (SdmaChannelPtr->State) {
        case CHANNEL_IDLE:
            continue;

        case CHANNEL_RUNNING:
            *InterruptType = InterruptTypeCompletion;

            //
            // Process the completed buffers descriptors for Auto-Initialize
            // transfers.
            //

            if (SdmaChannelPtr->IsAutoInitialize) {
                SdmaHwUpdateChannelForAutoInitialize(SdmaControllerPtr,
                                                     ChannelIndex);

                //
                // SdmaHwUpdateChannelForAutoInitialize may update the channel
                // state.
                //

                if (SdmaChannelPtr->State == CHANNEL_ERROR) {
                    *InterruptType = InterruptTypeError;
                    SdmaChannelPtr->State = CHANNEL_RUNNING;
                }
            }
            break;

        case CHANNEL_ABORTING:
            *InterruptType = InterruptTypeCancelled;
            break;

        case CHANNEL_ERROR:
            *InterruptType = InterruptTypeError;
            SdmaChannelPtr->State = CHANNEL_RUNNING;
            break;

        default:
            NT_ASSERT(FALSE);
            *InterruptType = InterruptTypeError;
            break;
        }

        *ChannelNumber = ChannelIndex;
        SdmaControllerPtr->PendingInterrupts = InterruptStatus;
        return TRUE;
    }

    SdmaControllerPtr->PendingInterrupts = 0;
    return FALSE;
}


_Use_decl_annotations_
ULONG
SdmaReadDmaCounter (
    PVOID ControllerContextPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine determines how many bytes remain to be transferred on the
    given channel.  If the current transfer is set to loop, this routine
    will return the number of bytes remaining in the current iteration.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel number.

Return Value:

    Returns the number of bytes remaining to be transferred on the given
    channel.

--*/

{

    ULONG ActualBufferDescLenth;
    ULONG BufferIndex;
    ULONG BytesTrasnferred;
    ULONG DmaWordSize;
    const volatile SDMA_BD* SdmaBufferDescPtr;
    const SDMA_BD_EXT* SdmaBufferDescExPtr;
    SDMA_CHANNEL* SdmaChannelPtr;
    const SDMA_CONTROLLER* SdmaControllerPtr;
    ULONG WordsTransferred;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaControllerPtr = (const SDMA_CONTROLLER*)ControllerContextPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    DmaWordSize = SdmaChannelPtr->DmaWordSize;

    WordsTransferred = 0;
    for (BufferIndex = 0; BufferIndex < SdmaChannelPtr->ActiveBufferCount;
         ++BufferIndex) {

        SdmaBufferDescPtr = &SdmaChannelPtr->SdmaBD[BufferIndex];
        SdmaBufferDescExPtr = &SdmaChannelPtr->SdmaBdExt[BufferIndex];

        NT_ASSERT(SdmaBufferDescExPtr->ByteLength >=
                  SdmaBufferDescExPtr->BytesTransferred);

        ActualBufferDescLenth = SdmaBufferDescExPtr->ByteLength -
            SdmaBufferDescExPtr->BytesTransferred;

        if (SdmaBufferDescPtr->Attributes.R != 0) {
            SdmaChannelPtr->State = CHANNEL_ERROR;
        }

        if (SdmaBufferDescPtr->Attributes.D != 0) {

            //
            // SDMA owns the buffer descriptor, check if it is currently
            // in progress, and get the current count.
            //

            if ((SdmaBufferDescPtr->Attributes.Count * DmaWordSize) <
                ActualBufferDescLenth) {

                WordsTransferred += (ULONG)SdmaBufferDescPtr->Attributes.Count;
            }
            break;
        }

        WordsTransferred += (ULONG)SdmaBufferDescPtr->Attributes.Count;
    }

    BytesTrasnferred = SdmaChannelPtr->AutoInitBytesTransferred +
        WordsTransferred * SdmaChannelPtr->DmaWordSize;

    NT_ASSERT(SdmaChannelPtr->TransferLength >= BytesTrasnferred);

    return SdmaChannelPtr->TransferLength - BytesTrasnferred;
}


_Use_decl_annotations_
VOID
SdmaReportCommonBuffer (
    PVOID ControllerContextPtr,
    ULONG ChannelNumber,
    PVOID VirtualAddressPtr,
    PHYSICAL_ADDRESS LogicalAddress
    )

/*++

Routine Description:

    This routine allows the HAL to report to the extension information about
    a common buffer allocated on behalf of a channel.  This allocation is the
    result of a requirement listed in the channel initialization block.
    The buffer is non-cached.

Arguments:

    ControllerContextPtr - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel number.

    VirtualAddressPtr - Supplies the virtual address of the common buffer.

    LogicalAddress - Supplies the DMA controller visible address of the common
        buffer.

Return Value:

    None.

--*/

{

    SDMA_CHANNEL* SdmaChannelPtr;
    SDMA_CHANNEL0* SdmChannel0Ptr;
    SDMA_CONTROLLER* SdmaControllerPtr;


    SdmaControllerPtr = (SDMA_CONTROLLER*)ControllerContextPtr;

    NT_ASSERT(SdmaControllerPtr->ChannelsPtr[ChannelNumber] == NULL);

    if (ChannelNumber == 0) {
        RtlZeroMemory(VirtualAddressPtr, sizeof(SDMA_CHANNEL0));
    } else {
        RtlZeroMemory(VirtualAddressPtr, sizeof(SDMA_CHANNEL));
    }

    //
    // Save the channel logical base address
    //

    SdmaChannelPtr = (SDMA_CHANNEL*)VirtualAddressPtr;
    SdmaChannelPtr->This.QuadPart = LogicalAddress.QuadPart;

    SdmaControllerPtr->ChannelsPtr[ChannelNumber] = SdmaChannelPtr;
#pragma prefast(suppress: 25024, "Channel 0 context is SDMA_CHANNEL0*")
    SdmChannel0Ptr = (SDMA_CHANNEL0*)SdmaControllerPtr->ChannelsPtr[0];

    NT_ASSERT(SdmChannel0Ptr != NULL);

    //
    // Initialize the CCB to point to the channel BD.
    //

    SdmChannel0Ptr->SdmaCCBs[ChannelNumber].CurrentBdAddress =
        SDMA_CHN_LOGICAL_ADDR(SdmaChannelPtr, SdmaBD[0]);
    SdmChannel0Ptr->SdmaCCBs[ChannelNumber].BasedBdAddress =
        SDMA_CHN_LOGICAL_ADDR(SdmaChannelPtr, SdmaBD[0]);

    if (ChannelNumber == 0) {
        SdmaHwInitialize(SdmaControllerPtr);
    }

    return;
}

//
// ---------------------------------------------------------- Private Functions
//

_Use_decl_annotations_
NTSTATUS
SdmaInitDmaReqMapping (
    SDMA_CONTROLLER* SdmaControllerPtr,
    IMX_CPU CpuType,
    ULONG Instance
    )

/*++

Routine Description:

    SdmaInitDmaReqMapping fills SOC specific information of the
    SDMA_CONTROLLER structure based on the passed CpuType value.
    Also checks that the SDMA controller instance number conforms
    with the SOC type.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    CpuType - The CPU type (flavor) of the IMX SOC.
 
    Instance - The SDMA controller instance
Return Value:

    STATUS_SUCCESS, or STATUS_NOT_SUPPORTED

--*/

{

    ULONG Index;

    // check range of Instance
    switch (CpuType) {
    case IMX_CPU_MX6ULL:
    case IMX_CPU_MX6Q:
    case IMX_CPU_MX6QP:
    case IMX_CPU_MX6D:
    case IMX_CPU_MX6DP:
    case IMX_CPU_MX6SX:
        if (Instance != 0) {
            return STATUS_NOT_SUPPORTED;
        }
        break;

    case IMX_CPU_MX8MQ:
        if (Instance > 1) {
            return STATUS_NOT_SUPPORTED;
        }
        break;

    default:
        return STATUS_NOT_SUPPORTED;
    }

    switch (CpuType) {
    case IMX_CPU_MX6Q:
    case IMX_CPU_MX6QP:
    case IMX_CPU_MX6D:
    case IMX_CPU_MX6DP:
        SdmaControllerPtr->SdmaReqToChannelConfigPtr = Imx6QdDmaReqToChannelConfig;
        SdmaControllerPtr->SdmaReqMaxId = Imx6QdDmaReqMax;
        SdmaControllerPtr->SdmaCodeBlock = imx6_sdma_code;
        SdmaControllerPtr->SdmaCodeSize = IMX6_RAM_CODE_SIZE * sizeof(short);
        SdmaControllerPtr->SdmaAp2ApScript = imx6_ap_2_ap_ADDR;
        SdmaControllerPtr->SdmaPer2PerScript = imx6_p_2_p_ADDR;
        break;

    case IMX_CPU_MX6ULL:
        SdmaControllerPtr->SdmaReqToChannelConfigPtr = Imx6UllDmaReqToChannelConfig;
        SdmaControllerPtr->SdmaReqMaxId = Imx6UllDmaReqMax;
        SdmaControllerPtr->SdmaCodeBlock = imx6_sdma_code;
        SdmaControllerPtr->SdmaCodeSize = IMX6_RAM_CODE_SIZE * sizeof(short);
        SdmaControllerPtr->SdmaAp2ApScript = imx6_ap_2_ap_ADDR;
        SdmaControllerPtr->SdmaPer2PerScript = imx6_p_2_p_ADDR;
        break;

    case IMX_CPU_MX6SX:
        SdmaControllerPtr->SdmaReqToChannelConfigPtr = Imx6SxDmaReqToChannelConfig;
        SdmaControllerPtr->SdmaReqMaxId = Imx6SxDmaReqMax;
        SdmaControllerPtr->SdmaCodeBlock = imx6_sdma_code;
        SdmaControllerPtr->SdmaCodeSize = IMX6_RAM_CODE_SIZE * sizeof(short);
        SdmaControllerPtr->SdmaAp2ApScript = imx6_ap_2_ap_ADDR;
        SdmaControllerPtr->SdmaPer2PerScript = imx6_p_2_p_ADDR;
        break;

    // iMX8 uses the same SDMA controller as the iMX7, so use that SDMA script code.
    case IMX_CPU_MX8MQ:
        SdmaControllerPtr->SdmaReqToChannelConfigPtr = Imx8mDmaReqToChannelConfig;
        SdmaControllerPtr->SdmaReqMaxId = Imx8mDmaReqMax;
        SdmaControllerPtr->SdmaCodeBlock = imx7_sdma_code;
        SdmaControllerPtr->SdmaCodeSize = IMX7_RAM_CODE_SIZE * sizeof(short);
        SdmaControllerPtr->SdmaAp2ApScript = imx7_ap_2_ap_ADDR;
        SdmaControllerPtr->SdmaPer2PerScript = imx7_per_2_per_ADDR;
        break;

    default:
        return STATUS_NOT_SUPPORTED;
    }

    // Reset OwnerChannels for passed SDMA instance request lines
    for (Index = 0; Index <= SdmaControllerPtr->SdmaReqMaxId; ++Index) {
        if (SdmaControllerPtr->SdmaReqToChannelConfigPtr[Index].SdmaInstance == Instance) {
            NT_ASSERT (Index == SdmaControllerPtr->SdmaReqToChannelConfigPtr[Index].DmaRequestId);
            SdmaControllerPtr->SdmaReqToChannelConfigPtr[Index].OwnerChannel = 0;
        }
    }

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SdmaAcquireChannel0 (
    SDMA_CONTROLLER* SdmaControllerPtr
    )

/*++

Routine Description:

    SdmaAcquireChannel0 is called acquire channel 0 for exclusive use.
    When channel 0 is acquired, its priority is set to the highest value.

Arguments:

    SdmaControllerPtr - The controller's internal data.

Return Value:

    None.

--*/

{

    NT_ASSERT(SdmaControllerPtr->ChannelsPtr[0]->State == CHANNEL_IDLE);

    SdmaHwSetChannelPriority(SdmaControllerPtr, 0, CHN_PRI_HIGHEST);

    return;
}


_Use_decl_annotations_
VOID
SdmaReleaseChannel0 (
    SDMA_CONTROLLER* SdmaControllerPtr
    )

/*++

Routine Description:

    SdmaReleaseChannel0 is called release channel 0, after it was acquired.
    using SdmaAcquireChannel0.
    Before channel is release, its priority is set to 0 (disabled).

Arguments:

    SdmaControllerPtr - The controller's internal data.

Return Value:

    None.

--*/

{

    SdmaHwSetChannelPriority(SdmaControllerPtr, 0, CHN_PRI_DISABLED);

    SdmaControllerPtr->ChannelsPtr[0]->State = CHANNEL_IDLE;

    return;
}


//
// ------------------------------------------------------- HW support Functions
//

_Use_decl_annotations_
VOID
SdmaHwInitialize (
    SDMA_CONTROLLER* SdmaControllerPtr
    )

/*++

Routine Description:

    SdmaHwInitialize is called to initialize the controller.
    It is called after we get channel0 resources, since it needs access
    to a shared buffer, where the logical address is available.
    This resets the channels runtime registers and loads the scripts code into
    SDMA RAM.

Arguments:

    SdmaControllerPtr - The controller's internal data.

Return Value:

    None: The SDMA_CONTROLLER status is reflected in
        SDMA_CONTROLLER::ControllerStatus

--*/

{

    ULONG Index;
    SDMA_CHANNEL0* SdmChannel0Ptr;
    SDMA_CONFIG_REG ConfigReg;
    SDMA_CHN0ADDR_REG Channel0AddrReg;
    volatile SDMA_REGS* SdmaRegsPtr;
    NTSTATUS Status;


    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;
#pragma prefast(suppress: 25024, "Channel 0 context is SDMA_CHANNEL0*")
    SdmChannel0Ptr = (SDMA_CHANNEL0*)SdmaControllerPtr->ChannelsPtr[0];

    NT_ASSERT(SdmaRegsPtr != NULL);
    NT_ASSERT(SdmChannel0Ptr != NULL);

    SdmaControllerPtr->ControllerStatus = STATUS_DEVICE_NOT_READY;

    //
    // First, bring the controller into known state
    // where all the channels are disabled.
    //

    //
    // Clear request-channel map
    //

    for (Index = 0; Index < SDMA_NUM_EVENTS; ++Index) {
        SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->CHNENBL[Index], 0);
    }

    //
    // Disable all channels
    //

    for (Index = 0; Index < SDMA_NUM_CHANNELS; ++Index) {
        SdmaHwSetChannelPriority(SdmaControllerPtr,
                                 Index, // ChannelNumber
                                 CHN_PRI_DISABLED);
    }

    //
    // Do initial controller initialization:
    // - Set the channel0 CCB address
    // - Set the controller configuration:
    //   - Static context switching required for the initial setup.
    //        After initialization completes, we change to dynamic context
    //        switching for improved power consumption, and better performance.
    //   - DMA/SDMA core clock ratio
    //   - Set context size to 32 DWORDS.
    //

    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->MC0PTR,
        SDMA_CHN0_LOGICAL_ADDR(SdmChannel0Ptr, SdmaCCBs[0]));

    ConfigReg.AsUlong = 0;
    ConfigReg.CSM = CSM_STATIC;

    if (SdmaControllerPtr->CoreClockRatio ==
        IMX_SDMA_CORE_CLOCK_TWICE_CORE_FREQ) {
        ConfigReg.ACR = 0;
    } else {
        NT_ASSERT(SdmaControllerPtr->CoreClockRatio ==
                  IMX_SDMA_CORE_CLOCK_EQUALS_CORE_FREQ);
        ConfigReg.ACR = 1;
    }

    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->CONFIG, ConfigReg.AsUlong);

    Channel0AddrReg.AsUlong = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->CHN0ADDR);
    Channel0AddrReg.SMSZ = 1;
    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->CHN0ADDR,
                                 Channel0AddrReg.AsUlong);

    //
    // Remove channel 0 host override so we can use it.
    //

    SdmaHwSetChannelOverride(SdmaControllerPtr,
                             0,     // Channel0
                             TRUE); // Event override

    //
    // Copy the scripts code to a shared area in channel0 extension.
    //

    SdmChannel0Ptr->ScriptsCodeSize = SdmaControllerPtr->SdmaCodeSize;
    RtlCopyMemory(SdmChannel0Ptr->ScriptsCode,
                  SdmaControllerPtr->SdmaCodeBlock,
                  SdmChannel0Ptr->ScriptsCodeSize);

    //
    // Load the script code to SDMA RAM in PM mode,
    // using channel 0.
    //

    Status = SdmaHwLoadRamPM(SdmaControllerPtr,
                             SDMA_CHN0_LOGICAL_ADDR(SdmChannel0Ptr, // Source
                                                    ScriptsCode),
                             SDMA_PM_CODE_START,                    // Dest
                             SdmChannel0Ptr->ScriptsCodeSize);      // Size

    SdmaControllerPtr->ControllerStatus = Status;

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(NT_SUCCESS(Status));
    }

    //
    // Force re-evaluate scheduling for all channels.
    //

    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->EVTPEND, 0xFFFFFFFF);

    //
    // After initialization is completed, change to dynamic context switching,
    // for improved power consumption, and better performance.
    //

    ConfigReg.AsUlong = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->CONFIG);
    ConfigReg.CSM = CSM_DYNAMIC;

    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->CONFIG, ConfigReg.AsUlong);

    return;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwStartChannel (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwStartChannel starts the given channel by setting the associated bit
    in HSTART register.
    The routine does some sanity checks to validate channel can run, by checking
    the channel priority.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - ChannelNumber index.

Return Value:

    STATUS_SUCCESS, or STATUS_INVALID_DEVICE_STATE if the channel priority
    does not let it run.

--*/

{

    ULONG ChannelRunMask;
    SDMA_CHANNEL_STATE OldChannelState;
    ULONG RegValue;
    SDMA_CHANNEL* SdmaChannelPtr;
    volatile SDMA_REGS* SdmaRegsPtr;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    OldChannelState = SdmaChannelPtr->State;

    if (OldChannelState == CHANNEL_RUNNING) {
        NT_ASSERT(OldChannelState != CHANNEL_RUNNING);
    }
    SdmaChannelPtr->State = CHANNEL_RUNNING;

    //
    // Make sure channel priority allows the channel to run.
    //

    RegValue = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->CHNPRI[ChannelNumber]);

    if (RegValue == CHN_PRI_DISABLED) {
        SdmaChannelPtr->State = CHANNEL_IDLE;
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Start the channel if it is not already running.
    //

    RegValue = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->HSTART);
    ChannelRunMask = (1 << ChannelNumber);

    if ((RegValue & ChannelRunMask) == 0) {
        SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->HSTART, ChannelRunMask);
    }

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwResumeChannel (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwResumeChannel resumes the channel by reloading it's context to start
    from the current buffer descriptor, and running it by setting
    the associated bit in HSTART register.

    SdmaHwResumeChannel assumes the channel is stopped.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - ChannelNumber index.

Return Value:

    Channel setup status.

--*/

{

    SDMA_CHANNEL* SdmaChannelPtr;
    volatile SDMA_REGS* SdmaRegsPtr;
    NTSTATUS Status;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];

    NT_ASSERT(!SdmaHwIsChannelRunning(SdmaControllerPtr, ChannelNumber));

    //
    // Setup the channel: set and load the context etc.
    //

    Status = SdmaHwSetupChannel(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        NT_ASSERT(NT_SUCCESS(Status));
        SdmaChannelPtr->State = CHANNEL_ERROR;
        return Status;
    }

    SdmaChannelPtr->State = CHANNEL_RUNNING;

    //
    // Start the channel
    //

    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->HSTART, (1 << ChannelNumber));

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
BOOLEAN
SdmaHwIsChannelRunning (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwIsChannelRunning returns the state of the channel.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - ChannelNumber index.

Return Value:

    TRUE if channel is running.

--*/

{

    ULONG RegValue;
    volatile SDMA_REGS* SdmaRegsPtr;


    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;

    RegValue = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->STOP_STAT);

    return  (RegValue & (1 << ChannelNumber)) != 0;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwWaitChannelDone (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwWaitChannel waits until the channel is done running

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - ChannelNumber index.

Return Value:

    STATUS_SUCCESS, or STATUS_TIMEOUT if channel was not done within
    the pre-defined timeout.

--*/

{

    ULONG Retry;


    //
    // Since we cannot use any time/stall services, poll the channel status.
    //

    for (Retry = 0; Retry < IMX_MAX_CHANNEL_DONE_WAIT_RETRY; ++Retry) {
        if (!SdmaHwIsChannelRunning(SdmaControllerPtr, ChannelNumber)) {
            return STATUS_SUCCESS;
        }
    }

    NT_ASSERT(FALSE);

    return STATUS_TIMEOUT;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwStopTransfer (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwStopTransfer stops a the given channel by setting the associated bit
    in STOP_STAT register.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - Channel index.

Return Value:

    STATUS_SUCCESS, or STATUS_INVALID_DEVICE_STATE if the transfer was not stopped.

--*/

{

    ULONG ChannelMask;
    ULONG RegValue;
    volatile SDMA_REGS* SdmaRegsPtr;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;
    ChannelMask = (1 << ChannelNumber);

    //
    // Disable the channel by clearing HE[i], HSTART[i] bits.
    //

    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->STOP_STAT, ChannelMask);

    RegValue = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->STOP_STAT);
    if ((RegValue & ChannelMask) != 0) {
        NT_ASSERT((RegValue & ChannelMask) == 0);
        return STATUS_INVALID_DEVICE_STATE;
    }

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwStopChannel (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwStopChannel stops a the given channel transfer, disables the
    channel and clears the channel resources.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - Channel index.

Return Value:

    STATUS_SUCCESS, or STATUS_INVALID_DEVICE_STATE if the channel was not stopped.

--*/

{

    SDMA_CHANNEL* SdmaChannelPtr;
    NTSTATUS Status;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];

    Status = SdmaHwStopTransfer(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    SdmaHwSetChannelPriority(SdmaControllerPtr, ChannelNumber, CHN_PRI_DISABLED);

    SdmaChannelPtr->State = CHANNEL_IDLE;

    //
    // Clear the DMA events associated with the current request ID.
    //

    SdmaHwUnbindDmaEvents(SdmaControllerPtr, ChannelNumber);

    //
    // Clear buffer descriptors
    //

    RtlZeroMemory((PVOID)SdmaChannelPtr->SdmaBD, sizeof(SdmaChannelPtr->SdmaBD));
    RtlZeroMemory(SdmaChannelPtr->SdmaBdExt, sizeof(SdmaChannelPtr->SdmaBdExt));
    SdmaChannelPtr->ActiveBufferCount = 0;
    SdmaChannelPtr->NotificationThreshold = 0;

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SdmaHwSetChannelOverride (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber,
    BOOLEAN IsEventOverride
    )

/*++

Routine Description:

    SdmaHwSetChannelOverride updates the host (HO) and event (PO) overrides.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - ChannelNumber index

    IsEventOverride - if to override DMA event (request) EO[i]

Return Value:

--*/

{

    ULONG ChannelMask;
    ULONG RegValue;
    volatile SDMA_REGS* SdmaRegsPtr;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;

    ChannelMask = (1 << ChannelNumber);

    RegValue = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->EVTOVR);
    if (IsEventOverride) {
        RegValue |= ChannelMask;
    } else {
        RegValue &= ~ChannelMask;
    }
    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->EVTOVR, RegValue);

    return;
}


_Use_decl_annotations_
VOID
SdmaHwSetChannelPriority (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber,
    SDMA_CHANNEL_PRIORITY Priority
    )

/*++

Routine Description:

    SdmaHwSetChannelPriority sets the channel priority.

Arguments:

    SdmaControllerPtr - The controller's internal data.

Return Value:

--*/

{

    volatile SDMA_REGS* SdmaRegsPtr;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;

    SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->CHNPRI[ChannelNumber],
                                 (ULONG)Priority);

    return;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwBindDmaEvents (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber,
    ULONG RequestLine
    )

/*++

Routine Description:

    SdmaHwBindDmaEvents configures the DMA event registers for the given channel.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - The target channel index.

    RequestLine - Supplies the request line number to program.  This request
        line number is system-unique (as provided to the HAL during
        registration) and must be translated by the extension.

Return Value:

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER - RequestLine is invalid
    STATUS_NOT_SUPPORTED - RequestLine is not supported
    STATUS_DEVICE_CONFIGURATION_ERROR - Bad configuration for RequestLine.
        This is usually due to malformed/typo configuration table.

--*/

{

    ULONG CHNENBLnReg;
    ULONG EventId;
    ULONG Index;
    SDMA_CHANNEL* SdmaChannelPtr;
    ULONG SdmaRequestLine;
    ULONG SdmaInstance;
    const SDMA_CHANNEL_CONFIG* SdmaChannelConfigPtr;
    const SDMA_EVENT_CONFIG* SdmaEventConfigPtr;
    volatile SDMA_REGS* SdmaRegsPtr;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaRequestLine = RequestLine & SDMA_REQ_LINE_ID_MASK;
    SdmaInstance = RequestLine >> SDMA_INSTANCE_ID_SHIFT;

    if (SdmaInstance != SdmaControllerPtr->SdmaInstance) {
        return STATUS_INVALID_PARAMETER;
    }

    if (SdmaRequestLine > SdmaControllerPtr->SdmaReqMaxId) {
        NT_ASSERT(SdmaRequestLine <= SdmaControllerPtr->SdmaReqMaxId);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get the channel configuration parameters, based on the request line ID.
    //

    SdmaChannelConfigPtr =
        &SdmaControllerPtr->SdmaReqToChannelConfigPtr[SdmaRequestLine];

    if (SdmaChannelConfigPtr->DmaRequestId != SdmaRequestLine) {
        NT_ASSERT(SdmaChannelConfigPtr->DmaRequestId == SdmaRequestLine);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (SdmaChannelConfigPtr->SdmaInstance != SdmaInstance) {
        NT_ASSERT(SdmaInstance == SdmaChannelConfigPtr->SdmaInstance);
        return STATUS_INVALID_PARAMETER;
    }

    if (SdmaChannelConfigPtr->SdmaScriptAddr == SDMA_UNSUPPORTED_REQUEST_ID) {
        NT_ASSERT(SdmaChannelConfigPtr->SdmaScriptAddr !=
                  SDMA_UNSUPPORTED_REQUEST_ID);
        return STATUS_NOT_SUPPORTED;
    }

    NT_ASSERT(SdmaChannelConfigPtr->OwnerChannel == ChannelNumber);

    //
    // Bind the channel to DMA event(s)
    //

    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    SdmaChannelPtr->DmaWordSize =
        SdmaHwGetDataWidth(SdmaChannelConfigPtr->TransferWidth);

    if (SdmaChannelConfigPtr->TriggerDmaEventCount != 0) {
        SdmaEventConfigPtr = &SdmaChannelConfigPtr->TriggerDmaEvents[0];

        for (Index = 0;
             Index < SdmaChannelConfigPtr->TriggerDmaEventCount;
             ++Index, ++SdmaEventConfigPtr) {

            EventId = SdmaEventConfigPtr->SdmaEventId;
            NT_ASSERT(EventId < SDMA_NUM_EVENTS);

            _Analysis_assume_(EventId < SDMA_NUM_EVENTS);
            CHNENBLnReg =
                SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->CHNENBL[EventId]);

            _Analysis_assume_(ChannelNumber < SDMA_NUM_CHANNELS);
            CHNENBLnReg |= (1 << ChannelNumber);

            SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->CHNENBL[EventId],
                                         CHNENBLnReg);
        }

        //
        // Configure the channel so the DMA event (EP[i])
        // makes it runnable: EO[i] -> 0.
        //
        // A channel is runnable if
        // (HE[i] or HO[i]) and (DO[i]) and (EP[i] or EO[i])
        //
        // In our case DO[i] is always 1.
        //

        SdmaHwSetChannelOverride(SdmaControllerPtr,
                                 ChannelNumber,
                                 FALSE); // Event override

    } else {

        //
        // No DMA event is associated with the request (memory to memory).
        //
        // Configure the channel so it is not dependent on
        // DMA event (EP[i]): EO[i] -> 1.
        //
        // A channel is runnable if
        // (HE[i] or HO[i]) and (DO[i]) and (EP[i] or EO[i])
        //
        // In our case DO[i] is always 1.
        //

        SdmaHwSetChannelOverride(SdmaControllerPtr,
                                 ChannelNumber,
                                 TRUE); // Event override
    }

    SdmaChannelPtr->ChannelConfigPtr = (SDMA_CHANNEL_CONFIG*)SdmaChannelConfigPtr;

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SdmaHwUnbindDmaEvents (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine programs a DMA controller channel for a specific transfer.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - The target channel index.

Return Value:

    None.

--*/

{

    ULONG CHNENBLnReg;
    ULONG EventId;
    ULONG Index;
    SDMA_CHANNEL* SdmaChannelPtr;
    const SDMA_CHANNEL_CONFIG* SdmaChannelConfigPtr;
    const SDMA_EVENT_CONFIG* SdmaEventConfigPtr;
    volatile SDMA_REGS* SdmaRegsPtr;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    SdmaRegsPtr = SdmaControllerPtr->SdmaRegsPtr;
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    SdmaChannelConfigPtr = SdmaChannelPtr->ChannelConfigPtr;
    SdmaEventConfigPtr = &SdmaChannelConfigPtr->TriggerDmaEvents[0];

    //
    // DMA request line may not be bound
    //

    if (SdmaChannelConfigPtr == NULL) {
        return;
    }

    //
    // Unbind the channel to DMA event(s), if any...
    //

    for (Index = 0; Index < SdmaChannelConfigPtr->TriggerDmaEventCount;
         ++Index, ++SdmaEventConfigPtr) {

        EventId = SdmaEventConfigPtr->SdmaEventId;
        NT_ASSERT(EventId < SDMA_NUM_EVENTS);

        _Analysis_assume_(EventId < SDMA_NUM_EVENTS);
        CHNENBLnReg =
            SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr->CHNENBL[EventId]);

        _Analysis_assume_(ChannelNumber < SDMA_NUM_CHANNELS);
        CHNENBLnReg &= ~(1 << ChannelNumber);

        SDMA_WRITE_REGISTER_ULONG(&SdmaRegsPtr->CHNENBL[EventId],
                                     CHNENBLnReg);
    }

    SdmaHwSetChannelOverride(SdmaControllerPtr,
                             ChannelNumber,
                             FALSE); // Event override

    SdmaChannelPtr->ChannelConfigPtr = NULL;
    SdmaChannelPtr->WatermarkLevel = 0;

    return;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwConfigureSgList (
    SDMA_CHANNEL* SdmaChannelPtr,
    const DMA_SCATTER_GATHER_LIST* MemoryAddressesPtr,
    PHYSICAL_ADDRESS DeviceAddress,
    BOOLEAN IsLoopTransfer
    )

/*++

Routine Description:

    This routine builds the SDMA buffer descriptor list based on the caller's
    buffer scatter-gather list and the notification threshold.

Arguments:

    SdmChannelPtr - The SDMA channel descriptor.

    MemoryAddress - Supplies the address to be programmed into the memory
    side of the channel configuration.

    DeviceAddress - Supplies the address to be programmed into the device
    side of the channel configuration.

    IsLoopTransfer - Supplies whether AutoInitialize has been set in the
    adapter making this request.

Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES if there are not enough
    SDMA buffers descriptors to support the desired notification threshold.
    In the later case, SdmaHwConfigureSgList will be called with notification
    threshold disabled.

--*/

{

    PHYSICAL_ADDRESS BufferAddress;
    ULONG BufferIndex;
    ULONG BufferLength;
    ULONG DeviceConfigFlags;
    ULONG MinThreshold;
    BOOLEAN IsLastBuffer;
    ULONG SgElementOffset;
    ULONG SgElementIndex;
    const SCATTER_GATHER_ELEMENT* SgElementPtr;
    ULONG Threshold;
    ULONG TransferLength;
    ULONG TransferOffset;

    __analysis_assume(SdmaChannelPtr->ChannelConfigPtr != NULL);
    DeviceConfigFlags = SdmaChannelPtr->ChannelConfigPtr->DeviceFlags;

    TransferLength = SdmaGetTransferLength(MemoryAddressesPtr);
    TransferOffset = 0;

    SgElementPtr = &MemoryAddressesPtr->Elements[0];
    SgElementOffset = 0;
    SgElementIndex = 0;

    Threshold = 0xFFFFFFFFUL;
    if (SdmaChannelPtr->NotificationThreshold != 0) {
        MinThreshold = TransferLength / (SDMA_SG_LIST_MAX_SIZE * 80 / 100);
        Threshold = max(MinThreshold, SdmaChannelPtr->NotificationThreshold);
    }

    BufferLength = min(Threshold, SgElementPtr->Length);
    BufferAddress = SgElementPtr->Address;
    BufferIndex = 0;

    for (;;) {

        NT_ASSERT((TransferOffset + BufferLength) <= TransferLength);

        if (BufferIndex == SDMA_SG_LIST_MAX_SIZE) {
            NT_ASSERT(BufferIndex < SDMA_SG_LIST_MAX_SIZE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IsLastBuffer = ((TransferOffset + BufferLength) == TransferLength);

        SdmaHwInitBufferDescriptor(SdmaChannelPtr,
                                   BufferIndex,
                                   BufferLength,
                                   BufferAddress,
                                   DeviceAddress,
                                   IsLastBuffer,
                                   IsLoopTransfer);

        if (SDMA_DEVICE_FLAG_ON(DeviceConfigFlags,
                                SDMA_DEVICE_FLAG_EXT_ADDRESS) &&
            !SDMA_DEVICE_FLAG_ON(DeviceConfigFlags,
                                 SDMA_DEVICE_FLAG_FIXED_ADDRESS)) {

            DeviceAddress.QuadPart += BufferLength;
        }

        ++BufferIndex;
        TransferOffset += BufferLength;
        SgElementOffset += BufferLength;
        NT_ASSERT(SgElementOffset <= SgElementPtr->Length);

        if (SgElementOffset == SgElementPtr->Length) {
            ++SgElementIndex;
            if (SgElementIndex == MemoryAddressesPtr->NumberOfElements) {
                NT_ASSERT(TransferOffset == TransferLength);
                break;
            }

            SgElementPtr = &MemoryAddressesPtr->Elements[SgElementIndex];
            BufferAddress = SgElementPtr->Address;
            SgElementOffset = 0;
            BufferLength = min(Threshold, SgElementPtr->Length);
        } else {
            BufferAddress.QuadPart += BufferLength;
            BufferLength = min(BufferLength,
                               (SgElementPtr->Length - SgElementOffset));
        }
    }

    SdmaChannelPtr->TransferLength = TransferLength;
    SdmaChannelPtr->ActiveBufferCount = BufferIndex;

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
ULONG
SdmaGetTransferLength (
    const DMA_SCATTER_GATHER_LIST* MemoryAddressesPtr
    )

/*++

Routine Description:

    This helper routine calculate the transfer length from the scatter gather
    list.

Arguments:

    MemoryAddressesPtr - The scatter gather list that describes the caller
        buffer.

Return Value:

    The transfer length bytes.

--*/

{

    ULONG Elemenet;
    ULONG TransferLength;
    const SCATTER_GATHER_ELEMENT* SgElementPtr;

    SgElementPtr = &MemoryAddressesPtr->Elements[0];
    TransferLength = 0;
    for (Elemenet = 0; Elemenet < MemoryAddressesPtr->NumberOfElements; ++Elemenet) {
        TransferLength += SgElementPtr->Length;
        SgElementPtr++;
    }

    return TransferLength;
}


_Use_decl_annotations_
VOID
SdmaHwInitBufferDescriptor (
    SDMA_CHANNEL* SdmaChannelPtr,
    ULONG BufferIndex,
    ULONG Length,
    PHYSICAL_ADDRESS Address1,
    PHYSICAL_ADDRESS Address2,
    BOOLEAN IsLast,
    BOOLEAN IsAutoInitialize
    )

/*++

Routine Description:

    This routine initializes a specific buffer descriptor.

Arguments:

    SdmaChannelPtr - The channel descriptor

    BufferIndex - Buffer descriptor index

    Length - Buffer length.

    Address1 - Address in AP memory.
               For memory-to-memory transfers, this parameter specifies
               the physical memory source address for the transfer. For
               memory-to-peripheral transfers, this parameter specifies the
               physical memory source address for the transfer. For
               peripheral-to-memory transfers, this parameter specifies the
               physical memory destination address for the transfer.

    Address2 - Additional address in AP memory to be used
               Used only for memory-to-memory transfers to specify
               the physical memory destination address for the transfer.
               Ignored for memory-to-peripheral and peripheral-to-memory
               transfers.

    IsLast - If this is the last descriptor of the chain.

    IsAutoInitialize - If this buffer is used as a circular buffer.
        IsAutoInitialize is ignored if IsLast is FALSE.

Return Value:

    None.

--*/

{

    const SDMA_CHANNEL_CONFIG* SdmaChannelConfigPtr;
    volatile SDMA_BD_ATTRIBUTES* SdmaBufferDescAttrPtr;
    volatile SDMA_BD* SdmaBufferDescPtr;
    SDMA_BD_EXT* SdmaBufferDescExtPtr;


    NT_ASSERT(BufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBD));

    SdmaChannelConfigPtr = SdmaChannelPtr->ChannelConfigPtr;

    _Analysis_assume_(BufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBD));
    SdmaBufferDescPtr = &SdmaChannelPtr->SdmaBD[BufferIndex];
    SdmaBufferDescAttrPtr = &SdmaBufferDescPtr->Attributes;

    _Analysis_assume_(BufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBdExt));
    SdmaBufferDescExtPtr = &SdmaChannelPtr->SdmaBdExt[BufferIndex];
    SdmaBufferDescExtPtr->Address = Address1.LowPart;
    SdmaBufferDescExtPtr->ByteLength = Length;
    SdmaBufferDescExtPtr->BytesTransferred = 0;

    SdmaBufferDescAttrPtr->AsUlong = 0;
    SdmaBufferDescAttrPtr->Count = Length / SdmaChannelPtr->DmaWordSize;
    SdmaBufferDescAttrPtr->Command = (ULONG)SdmaChannelConfigPtr->TransferWidth;

    if (IsLast) {

        //
        // Last descriptor:
        //  - Generate an interrupt
        //  - Either set as last or cyclic mode
        //    In cyclic mode, we do not set the 'C' flag since we
        //    want to make sure the BDs have been re-initialized.
        //

        SdmaBufferDescAttrPtr->I = 1;

        if (IsAutoInitialize) {
            SdmaBufferDescAttrPtr->C = 1;
            SdmaBufferDescAttrPtr->W = 1;
        } else {
            SdmaBufferDescAttrPtr->L = 1;
        }

    } else {

        //
        // For Auto-Initialize transfers we set each BD to
        // generate an interrupt.
        //

        if (IsAutoInitialize) {
            SdmaBufferDescAttrPtr->I = 1;
        }

        //
        // Not the last descriptor:
        //  - Continue to next BD when done.
        //

        SdmaBufferDescAttrPtr->C = 1;
    }

    SdmaBufferDescPtr->Address = Address1.LowPart;
    if (SDMA_DEVICE_FLAG_ON(SdmaChannelConfigPtr->DeviceFlags,
                            SDMA_DEVICE_FLAG_EXT_ADDRESS)) {

        SdmaBufferDescPtr->ExtendedAddress = Address2.LowPart;
    }

    //
    // Buffer descriptor is now ready for SDMA
    //

    SdmaBufferDescAttrPtr->D = 1;

    return;
}


_Use_decl_annotations_
BOOLEAN
SdmaHwUpdateBufferDescriptorForAutoInitialize (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber,
    ULONG BufferIndex
    )

/*++

Routine Description:

    This routine initializes a partially filled buffer descriptor for
    auto-initialize transfers.
    It is used during auto-initialize transfers, when the data flow has paused,
    causing the SDMA controller to skip to next buffer descriptor.
    An example is a UART receive script where the aging timer has triggered a
    DMA event that caused the controller to read the FIFO and close the current BD.
    This routine makes sure that next time the buffer descriptor is used, it
    will start from the last location.

Arguments:

    SdmaChannelPtr - The channel descriptor

    ChannelNumber - The target channel index.

    BufferIndex - The target buffer index.

Return Value:

    TRUE descriptor is done, otherwise FALSE.

--*/

{

    ULONG CurrentCount;
    ULONG DmaWordSize;
    BOOLEAN IsDescriptorDone;
    ULONG NextBufferIndex;
    volatile SDMA_BD* NextSdmaBufferDescPtr;
    volatile SDMA_BD_ATTRIBUTES* NextSdmaBufferDescAttrPtr;
    SDMA_BD_EXT* NextSdmaBufferDescExtPtr;
    SDMA_CHANNEL0* SdmaChannel0Ptr;
    volatile SDMA_BD_ATTRIBUTES* SdmaBufferDescAttrPtr;
    volatile SDMA_BD* SdmaBufferDescPtr;
    SDMA_BD_EXT* SdmaBufferDescExtPtr;
    SDMA_CHANNEL* SdmaChannelPtr;


    NT_ASSERT(BufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBD));

#pragma prefast(suppress: 25024, "Channel 0 context is SDMA_CHANNEL0*")
    SdmaChannel0Ptr = (SDMA_CHANNEL0*)SdmaControllerPtr->ChannelsPtr[0];
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];

    NT_ASSERT(SdmaChannelPtr->IsAutoInitialize);

    DmaWordSize = SdmaChannelPtr->DmaWordSize;

    _Analysis_assume_(BufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBD));
    SdmaBufferDescPtr = &SdmaChannelPtr->SdmaBD[BufferIndex];
    SdmaBufferDescAttrPtr = &SdmaBufferDescPtr->Attributes;

    _Analysis_assume_(BufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBdExt));
    SdmaBufferDescExtPtr = &SdmaChannelPtr->SdmaBdExt[BufferIndex];

    //
    // Get and update the BD progress
    //

    CurrentCount = (ULONG)SdmaBufferDescAttrPtr->Count;
    SdmaBufferDescExtPtr->BytesTransferred += (CurrentCount * DmaWordSize);
    SdmaChannelPtr->AutoInitBytesTransferred += (CurrentCount * DmaWordSize);

    NT_ASSERT(SdmaBufferDescExtPtr->BytesTransferred <=
              SdmaBufferDescExtPtr->ByteLength);

    //
    // If BD is not done, update it so transfer can resume from it.
    //

    if (SdmaBufferDescExtPtr->BytesTransferred <
        SdmaBufferDescExtPtr->ByteLength) {

        if (SdmaHwIsChannelRunning(SdmaControllerPtr, ChannelNumber)) {
            SdmaHwStopTransfer(SdmaControllerPtr, ChannelNumber);
            NT_ASSERT(!SdmaHwIsChannelRunning(SdmaControllerPtr, ChannelNumber));
        }

        SdmaBufferDescAttrPtr->Count =
            (SdmaBufferDescExtPtr->ByteLength -
             SdmaBufferDescExtPtr->BytesTransferred) / DmaWordSize;

        SdmaBufferDescPtr->Address +=  CurrentCount;

        //
        // Set the next BD to this one
        //

        SdmaChannel0Ptr->SdmaCCBs[ChannelNumber].CurrentBdAddress =
            SDMA_CHN_LOGICAL_ADDR(SdmaChannelPtr, SdmaBD[BufferIndex]);

        //
        // Re-initialize the following BDs
        //

        NextBufferIndex = BufferIndex + 1;
        while (NextBufferIndex < SdmaChannelPtr->ActiveBufferCount) {

            _Analysis_assume_(
                NextBufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBD));
            NextSdmaBufferDescPtr = &SdmaChannelPtr->SdmaBD[NextBufferIndex];
            NextSdmaBufferDescAttrPtr = &NextSdmaBufferDescPtr->Attributes;

            _Analysis_assume_(
                NextBufferIndex < ARRAYSIZE(SdmaChannelPtr->SdmaBdExt));
            NextSdmaBufferDescExtPtr = &SdmaChannelPtr->SdmaBdExt[NextBufferIndex];

            NextSdmaBufferDescPtr->Address = NextSdmaBufferDescExtPtr->Address;
            NextSdmaBufferDescAttrPtr->Count =
                (int)(NextSdmaBufferDescExtPtr->ByteLength / DmaWordSize);

            NextSdmaBufferDescAttrPtr->R = 0;
            NextSdmaBufferDescAttrPtr->D = 1;

            ++NextBufferIndex;
        }

        IsDescriptorDone = FALSE;

    } else {
        SdmaBufferDescAttrPtr->Count =
            (int)(SdmaBufferDescExtPtr->ByteLength / DmaWordSize);

        SdmaBufferDescPtr->Address = SdmaBufferDescExtPtr->Address;
        SdmaBufferDescExtPtr->BytesTransferred = 0;

        IsDescriptorDone = TRUE;
    }

    SdmaBufferDescAttrPtr->R = 0;
    SdmaBufferDescAttrPtr->D = 1;

    return IsDescriptorDone;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwSetupChannel (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine prepares the channel for running:
    - Initializes the channel context in Arm memory.
    - Loads channel context to SDMA memory, using channel 0.

Arguments:

    SdmaControllerPtr - The controller private data.

    ChannelNumber - The channel number.

Return Value:

    NTSTATUS

--*/

{

    NTSTATUS Status;


    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);

    //
    // Prepare channel context for target script
    //

    Status = SdmaHwSetupChannelContext(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Load prepared channel context to SDMA RAM (DM mode)
    //

    Status = SdmaHwLoadChannelContext(SdmaControllerPtr, ChannelNumber);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    SdmaHwSetChannelPriority(SdmaControllerPtr, ChannelNumber, CHN_PRI_NORMAL);

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwLoadRamPM (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG SourceAddress,
    ULONG DestinationAddress,
    ULONG ByteCount
    )

/*++

Routine Description:

    SdmaHwLoadRamPM is called to load the SDMA RAM with code (PM mode).
    The routine uses channel0 to load the RAM, and assumes HI(0) is set.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    SourceAddress - Source buffer logical address of buffer in ARM
        address space.

    DestinationAddress - Address in SDMA PM address space

    ByteCount - Number of bytes to write.

Return Value:

    STATUS_SUCCESS or STATUS_DEVICE_NOT_READY in case transfer failed.

--*/

{

    volatile SDMA_BD_ATTRIBUTES* Channel0BdAttrPtr;
    volatile SDMA_BD* Channel0BdPtr;
    SDMA_CHANNEL0* Channel0Ptr;
    NTSTATUS Status;


#pragma prefast(suppress: 25024, "Channel 0 context is SDMA_CHANNEL0*")
    Channel0Ptr = (SDMA_CHANNEL0*)SdmaControllerPtr->ChannelsPtr[0];
    Channel0BdPtr = &Channel0Ptr->SdmaChannel.SdmaBD[0];
    Channel0BdAttrPtr = &Channel0BdPtr->Attributes;

    SdmaAcquireChannel0(SdmaControllerPtr);

    //
    // Configure channel 0 BD
    //

    Channel0BdAttrPtr->AsUlong = 0;
    Channel0BdAttrPtr->Command = CMD_C0_SET_PM;
    Channel0BdAttrPtr->Count = ByteCount / sizeof(USHORT);
    Channel0BdAttrPtr->W = 1; // Wrap. Move to this buffer after it is done.
    Channel0BdAttrPtr->D = 1; // Buffer is ready for SDMA

    Channel0BdPtr->Address = SourceAddress;
    Channel0BdPtr->ExtendedAddress = DestinationAddress;

    //
    // Start the transfer
    //

    Status = SdmaHwStartChannel(SdmaControllerPtr, 0);

    if (!NT_SUCCESS(Status)) {
        goto Done;
    }

    //
    // Wait for transfer to complete.
    //

    Status = SdmaHwWaitChannelDone(SdmaControllerPtr, 0);

    if (!NT_SUCCESS(Status)) {
        goto Done;
    }


    //
    // Make sure buffer was processed, and did not encounter an error.
    //

    if ((Channel0BdAttrPtr->D != 0) || (Channel0BdAttrPtr->R != 0)) {
        Status = STATUS_DEVICE_NOT_READY;
        goto Done;
    }

    Status = STATUS_SUCCESS;

Done:

    SDMA_DUMP_REGS(SdmaControllerPtr);

    SdmaReleaseChannel0(SdmaControllerPtr);

    return Status;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwLoadChannelContext (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwLoadChannelContext is called to load the given channel context (DM mode).
    The routine uses channel0 to load the RAM, and assumes HI(0) is set.

Arguments:

    SdmaControllerPtr - The controller private data.

    ChannelNumber - The channel number.

    IsHostToSdma - If load SDMA (TRUE) or read from SDMA (FALSE)

Return Value:

    STATUS_SUCCESS or STATUS_DEVICE_NOT_READY in case transfer failed.

--*/

{

    SDMA_CHANNEL0* Channel0Ptr;
    volatile SDMA_BD_ATTRIBUTES* Channel0BdAttrPtr;
    volatile SDMA_BD* Channel0BdPtr;
    NTSTATUS Status;


#pragma prefast(suppress: 25024, "Channel 0 context is SDMA_CHANNEL0*")
    Channel0Ptr = (SDMA_CHANNEL0*)SdmaControllerPtr->ChannelsPtr[0];
    Channel0BdPtr = &Channel0Ptr->SdmaChannel.SdmaBD[0];
    Channel0BdAttrPtr = &Channel0BdPtr->Attributes;

    SdmaAcquireChannel0(SdmaControllerPtr);

    //
    // Configure channel 0 buffer descriptor
    //

    Channel0BdAttrPtr->AsUlong = 0;
    Channel0BdAttrPtr->Command = CH0_CMD_SETCTX(ChannelNumber);
    Channel0BdPtr->Address = SDMA_CHN0_LOGICAL_ADDR(Channel0Ptr,
                                                    ChannelContext);
    Channel0BdAttrPtr->Count = sizeof(SDMA_CHANNEL_CONTEXT) / sizeof(ULONG);
    Channel0BdAttrPtr->W = 1; // Wrap. Move to this buffer after it is done.
    Channel0BdAttrPtr->D = 1; // Buffer is ready for SDMA

    //
    // Start the transfer
    //

    Status = SdmaHwStartChannel(SdmaControllerPtr, 0);

    if (!NT_SUCCESS(Status)) {
        goto Done;
    }

    //
    // Wait for transfer to complete.
    //

    Status = SdmaHwWaitChannelDone(SdmaControllerPtr, 0);

    if (!NT_SUCCESS(Status)) {
        goto Done;
    }

    //
    // Make sure buffer was processed, and did not encounter an error.
    //

    if ((Channel0BdAttrPtr->D != 0) || (Channel0BdAttrPtr->R != 0)) {
        Status = STATUS_DEVICE_NOT_READY;
        goto Done;
    }

    Status = STATUS_SUCCESS;

Done:

    SdmaReleaseChannel0(SdmaControllerPtr);

    return Status;
}


_Use_decl_annotations_
NTSTATUS
SdmaHwSetupChannelContext (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwSetupChannelContext is called to initialize channel context image
    and set it up with the parameters required by the target script.

Arguments:

    SdmaControllerPtr - The controller private data.

    ChannelNumber - The channel number.

Return Value:

    STATUS_SUCCESS or STATUS_DEVICE_CONFIGURATION_ERROR in case the
    channel configuration is invalid.

--*/

{

    const SDMA_CHANNEL_CONFIG* SdmaChannelConfigPtr;
    const SDMA_CHANNEL* SdmaChannelPtr;
    SDMA_CHANNEL0* SdmaChannel0Ptr;
    SDMA_CHANNEL_CONTEXT* SdmaChannelContextPtr;
    LARGE_INTEGER DmaEventMask;
    ULONG WaterMarkLevel;


#pragma prefast(suppress: 25024, "Channel 0 context is SDMA_CHANNEL0*")
    SdmaChannel0Ptr = (SDMA_CHANNEL0*)SdmaControllerPtr->ChannelsPtr[0];
    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    SdmaChannelContextPtr = &SdmaChannel0Ptr->ChannelContext;
    SdmaChannelConfigPtr = SdmaChannelPtr->ChannelConfigPtr;

    RtlZeroMemory(SdmaChannelContextPtr, sizeof(SDMA_CHANNEL_CONTEXT));

    WaterMarkLevel = SdmaChannelPtr->WatermarkLevel *
        SdmaChannelConfigPtr->WatermarkLevelScale / 100;
    WaterMarkLevel = max(WaterMarkLevel, 1);

    SdmaHwGetDmaEventMask(SdmaChannelConfigPtr, &DmaEventMask);

    if (SdmaChannelConfigPtr->SdmaScriptAddr == SdmaControllerPtr->SdmaAp2ApScript) {
        // Memory to Memory
    }
    else if (SdmaChannelConfigPtr->SdmaScriptAddr == SdmaControllerPtr->SdmaPer2PerScript) {
        // peripheral to peripheral transfer
        if (SdmaChannelConfigPtr->Peripheral2Address == 0) {
            NT_ASSERT(SdmaChannelConfigPtr->Peripheral2Address != 0);
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        SdmaChannelContextPtr->GR[0] = DmaEventMask.HighPart;
        SdmaChannelContextPtr->GR[1] = DmaEventMask.LowPart;
        SdmaChannelContextPtr->GR[2] = SdmaChannelPtr->DeviceAddress.LowPart;
        SdmaChannelContextPtr->GR[6] = SdmaChannelConfigPtr->Peripheral2Address;
        SdmaChannelContextPtr->GR[7] = WaterMarkLevel;
    }
    else {
        // other
        SdmaChannelContextPtr->GR[0] = DmaEventMask.HighPart;
        SdmaChannelContextPtr->GR[1] = DmaEventMask.LowPart;
        SdmaChannelContextPtr->GR[6] = SdmaChannelPtr->DeviceAddress.LowPart;
        SdmaChannelContextPtr->GR[7] = WaterMarkLevel;
    }

    SdmaChannelContextPtr->StateRegs.PC = SdmaChannelConfigPtr->SdmaScriptAddr;

    return STATUS_SUCCESS;;
}


_Use_decl_annotations_
VOID
SdmaHwGetDmaEventMask (
    const SDMA_CHANNEL_CONFIG* ChannelConfigPtr,
    LARGE_INTEGER* DmaEventMaskPtr
    )

/*++

Routine Description:

    SdmaHwGetDmaEventMask is called to get the 48bit SDMA event mask based
    on the SDMA HW event that is configured for the given channel.

Arguments:

    ChannelConfigPtr - The channel configuration descriptor.

    DmaEventMaskPtr - Address of a 64bit var to receive the 48bit SDMA event
        mask.

Return Value:

    None.

--*/

{

    UINT64 DmaEventMask;
    ULONG EventCount;
    const SDMA_EVENT_CONFIG* SdmaEventConfigPtr;


    DmaEventMask = 0;
    EventCount = ChannelConfigPtr->TriggerDmaEventCount;
    SdmaEventConfigPtr = &ChannelConfigPtr->TriggerDmaEvents[0];

    while (EventCount != 0) {

        _Analysis_assume_(SdmaEventConfigPtr->SdmaEventId < SDMA_NUM_EVENTS);
        DmaEventMask |= (1ui64 << SdmaEventConfigPtr->SdmaEventId);

        --EventCount;
        ++SdmaEventConfigPtr;
    }
    DmaEventMaskPtr->QuadPart = (LONGLONG)DmaEventMask;

    return;
}


_Use_decl_annotations_
VOID
SdmaHwUpdateChannelForAutoInitialize (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber
    )

/*++

Routine Description:

    SdmaHwUpdateChannelForAutoInitialize is called to re-initialize channel descriptors
    for Auto-Initialize configured channels.

Arguments:

    SdmaControllerPtr - The controller private data.

    ChannelNumber - The target channel index.

Return Value:

    None.

--*/

{

    ULONG ActiveBufferCount;
    ULONG BufferIndex;
    ULONG DmaWordSize;
    BOOLEAN IsPartialDescriptorDone;
    ULONG SdmaBufferDescLength;
    volatile SDMA_BD* SdmaBufferDescPtr;
    volatile SDMA_BD_ATTRIBUTES* SdmaBufferDescAttrPtr;
    const SDMA_BD_EXT* SdmaBufferDescExtPtr;
    SDMA_CHANNEL* SdmaChannelPtr;


    SdmaChannelPtr = SdmaControllerPtr->ChannelsPtr[ChannelNumber];
    ActiveBufferCount = SdmaChannelPtr->ActiveBufferCount;
    DmaWordSize = SdmaChannelPtr->DmaWordSize;

    BufferIndex = SdmaChannelPtr->AutoInitNextBufferIndex;

    for (;;) {
        SdmaBufferDescPtr = &SdmaChannelPtr->SdmaBD[BufferIndex];
        SdmaBufferDescAttrPtr = &SdmaBufferDescPtr->Attributes;
        SdmaBufferDescExtPtr = &SdmaChannelPtr->SdmaBdExt[BufferIndex];
        SdmaBufferDescLength = SdmaBufferDescExtPtr->ByteLength;
        IsPartialDescriptorDone = FALSE;

        if (SdmaBufferDescAttrPtr->R != 0) {
            SdmaChannelPtr->State = CHANNEL_ERROR;
        }

        if (SdmaBufferDescAttrPtr->D != 0) {
            break;
        }

        if (((ULONG)SdmaBufferDescAttrPtr->Count * DmaWordSize) <
            SdmaBufferDescLength) {

            IsPartialDescriptorDone =
                SdmaHwUpdateBufferDescriptorForAutoInitialize(
                    SdmaControllerPtr, ChannelNumber, BufferIndex);

            if (!IsPartialDescriptorDone) {
                SdmaHwResumeChannel(SdmaControllerPtr, ChannelNumber);
                break;
            }
        }

        SdmaBufferDescAttrPtr->R = 0;
        SdmaBufferDescAttrPtr->D = 1;

        BufferIndex = (BufferIndex + 1) % ActiveBufferCount;

        //
        // Update the number of bytes transfered for this iteration
        //

        if (BufferIndex == 0) {
            SdmaChannelPtr->AutoInitBytesTransferred = 0;
        } else {

            //
            // If a partial descriptor was just completed, its
            // bytes have already been accounted for!
            //

            if (!IsPartialDescriptorDone) {
                SdmaChannelPtr->AutoInitBytesTransferred += SdmaBufferDescLength;
            }
        }
    }

    SdmaChannelPtr->AutoInitNextBufferIndex = BufferIndex;

    return;
}

_Use_decl_annotations_
NTSTATUS
SdmaHwSetRequestLineOwnership (
    SDMA_CONTROLLER* SdmaControllerPtr,
    ULONG ChannelNumber,
    ULONG RequestLine,
    BOOLEAN IsAcquire
    )

/*++

Routine Description:

    SdmaHwSetRequestLineOwnership is called to acquire or release ownership of
    a shared DMA request.

Arguments:

    SdmaControllerPtr - The controller's internal data.

    ChannelNumber - The channel for which the DMA request lines is either acquired,
        or released.

    RequestLine - The DMA request line for which ownership update is required.

    IsAcquire - If to acquire (TRUE) or release (FALSE).

Return Value:

    STATUS_SUCCESS - Request line ownership update succeeded.
    STATUS_SHARING_VIOLATION - Request line already acquired.
    STATUS_INVALID_PARAMETER - Trying to release a request line that is not owned.
--*/

{

    ULONG DmaEventCount;
    ULONG IoMuxGPR0Reg;
    ULONG SdmaRequestLine;
    ULONG SdmaInstance;
    volatile ULONG* IoMuxGPR0RegPtr;
    SDMA_CHANNEL_CONFIG* SdmaChannelConfigPtr;
    const SDMA_EVENT_CONFIG* SdmaEventConfigPtr;


    SdmaRequestLine = RequestLine & SDMA_REQ_LINE_ID_MASK;
    SdmaInstance = RequestLine >> SDMA_INSTANCE_ID_SHIFT;

    NT_ASSERT(ChannelNumber < SDMA_NUM_CHANNELS);
    NT_ASSERT(SdmaRequestLine <= SdmaControllerPtr->SdmaReqMaxId);
    NT_ASSERT(SdmaInstance == SdmaControllerPtr->SdmaInstance);

    SdmaChannelConfigPtr = (SDMA_CHANNEL_CONFIG*)
        &SdmaControllerPtr->SdmaReqToChannelConfigPtr[SdmaRequestLine];

    if (SdmaInstance != SdmaChannelConfigPtr->SdmaInstance) {
        return STATUS_INVALID_PARAMETER;
    }

    if (IsAcquire) {

        //
        // Acquire channel ownership
        //

        if ((SdmaChannelConfigPtr->OwnerChannel != 0)) {
            if (SdmaChannelConfigPtr->OwnerChannel == ChannelNumber) {
                return STATUS_SUCCESS;
            } else {
                return STATUS_SHARING_VIOLATION;
            }
        }

        SdmaChannelConfigPtr->OwnerChannel = ChannelNumber;

        //
        // Proceed to configure shared DMA event...
        //

    } else {

        //
        // Release channel ownership
        //

        if (SdmaChannelConfigPtr->OwnerChannel != ChannelNumber) {
            NT_ASSERT(SdmaChannelConfigPtr->OwnerChannel == ChannelNumber);
            return STATUS_INVALID_PARAMETER;
        }

        SdmaChannelConfigPtr->OwnerChannel = 0;
        return STATUS_SUCCESS;
    }

    IoMuxGPR0RegPtr = SdmaControllerPtr->IoMuxGPR0RegPtr;
    DmaEventCount = SdmaChannelConfigPtr->TriggerDmaEventCount;
    SdmaEventConfigPtr = &SdmaChannelConfigPtr->TriggerDmaEvents[0];

    while (DmaEventCount != 0) {

        //
        // Apply MUX selection for shared DMA events
        //

        if (SdmaEventConfigPtr->DmaEventSelect != DMA_EVENT_EXCLUSIVE) {
            IoMuxGPR0Reg = SDMA_READ_REGISTER_ULONG(IoMuxGPR0RegPtr);

            if (SdmaEventConfigPtr->SelectConfig == 1) {
                IoMuxGPR0Reg |= SdmaEventConfigPtr->DmaEventSelect;
            } else {
                NT_ASSERT(SdmaEventConfigPtr->SelectConfig == 0);
                IoMuxGPR0Reg &= ~SdmaEventConfigPtr->DmaEventSelect;
            }

            SDMA_WRITE_REGISTER_ULONG(IoMuxGPR0RegPtr, IoMuxGPR0Reg);
        }

        --DmaEventCount;
        ++SdmaEventConfigPtr;
    }

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
ULONG
SdmaHwGetDataWidth (
    SDMA_TRANSFER_WIDTH SdmaTransferWidth
    )

/*++

Routine Description:

    SdmaHwGetDataWidth return the data width in bytes given the SDMA transfer
    width codes.

Arguments:

    SdmaTransferWidth - SDMA transfer width code.

Return Value:

    Data width in bytes.

--*/

{

    switch (SdmaTransferWidth) {
    case DMA_WIDTH_8BIT:
        return sizeof(UCHAR);

    case DMA_WIDTH_16BIT:
        return sizeof(USHORT);

    case DMA_WIDTH_24BIT:
        __fallthrough;

    case DMA_WIDTH_32BIT:
        return sizeof(ULONG);

    default:
        NT_ASSERT(FALSE);
    }

    return (ULONG)-1;
}


#if DBG
_Use_decl_annotations_
VOID
SdmaHwDumpRegs (
    const SDMA_CONTROLLER* SdmaControllerPtr
    )

/*++

Routine Description:

    SdmaHwDumpRegs dumps the SDMA controller registers to SdmaRegsDump.

Arguments:

    SdmaControllerPtr - The controller's internal data.

Return Value:

--*/

{

    ULONG Index;
    SDMP_REG_DUMP* RegDumpPtr;
    ULONG RegOffset;
    volatile ULONG* SdmaRegsPtr;


    SdmaRegsPtr = (ULONG*)SdmaControllerPtr->SdmaRegsPtr;

    for (Index = 0; Index < ARRAYSIZE(SdmaRegsDump); ++Index) {
            RegDumpPtr = &SdmaRegsDump[Index];
            RegOffset = RegDumpPtr->Offset / sizeof(ULONG);

        RegDumpPtr->Value = SDMA_READ_REGISTER_ULONG(&SdmaRegsPtr[RegOffset]);
    }

    return;
}
#endif // DBG
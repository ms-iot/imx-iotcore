// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "precomp.h"

#include "MX6DodLogging.h"
#include "MX6DodDevice.tmh"

#include "Ipu.h"
#include "Imx6Hdmi.h"
#include "MX6DodCommon.h"
#include "MX6DodDevice.h"

MX6DOD_NONPAGED_SEGMENT_BEGIN; //==============================================

void BltBits (
    const void *SourceBitsPtr,
    ULONG SourcePitch,
    void *DestBitsPtr,
    ULONG DestPitch,
    _In_reads_(RectCount) const RECT* RectsPtr,
    ULONG RectCount
    )
{
    for (UINT i = 0; i < RectCount; ++i) {
        const RECT* rectPtr = &RectsPtr[i];

        NT_ASSERT(rectPtr->right >= rectPtr->left);
        NT_ASSERT(rectPtr->bottom >= rectPtr->top);

        const UINT numPixels = rectPtr->right - rectPtr->left;
        const UINT numRows = rectPtr->bottom - rectPtr->top;
        const UINT bytesToCopy = numPixels * 4;
        BYTE* dstStartPtr = static_cast<BYTE*>(DestBitsPtr) +
                          rectPtr->top * DestPitch +
                          rectPtr->left * 4;

        const BYTE* srcStartPtr = static_cast<const BYTE*>(SourceBitsPtr) +
                                rectPtr->top * SourcePitch +
                                rectPtr->left * 4;

        for (UINT row = 0; row < numRows; ++row) {
            RtlCopyMemory(dstStartPtr, srcStartPtr, bytesToCopy);
            dstStartPtr += DestPitch;
            srcStartPtr += SourcePitch;
        }
    }
}

_Use_decl_annotations_
VOID MX6DOD_DEVICE::DdiResetDevice (VOID* const /*MiniportDeviceContextPtr*/)
{
    MX6DOD_LOG_INFORMATION("DdiResetDevice was called, nothing to do");
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiSystemDisplayEnable (
    VOID* const ContextPtr,
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    DXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS* /*FlagsPtr*/,
    UINT* WidthPtr,
    UINT* HeightPtr,
    D3DDDIFORMAT* ColorFormatPtr
    )
{
    auto thisPtr = static_cast<MX6DOD_DEVICE*>(ContextPtr);

    UNREFERENCED_PARAMETER(TargetId);
    NT_ASSERT(TargetId == 0);

    *WidthPtr = thisPtr->dxgkDisplayInfo.Width;
    *HeightPtr = thisPtr->dxgkDisplayInfo.Height;
    *ColorFormatPtr = thisPtr->dxgkDisplayInfo.ColorFormat;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID MX6DOD_DEVICE::DdiSystemDisplayWrite (
    VOID* const ContextPtr,
    VOID* SourcePtr,
    UINT SourceWidth,
    UINT SourceHeight,
    UINT SourceStride,
    UINT PositionX,
    UINT PositionY
    )
{
    auto thisPtr = static_cast<MX6DOD_DEVICE*>(ContextPtr);

    const UINT destPitch = thisPtr->dxgkDisplayInfo.Pitch;
    const UINT bytesPerLine = SourceWidth * 4;
    BYTE* dstStartPtr = static_cast<BYTE*>(thisPtr->biosFrameBufferPtr) +
                      PositionY * destPitch +
                      PositionX * 4;

    const BYTE* srcStartPtr = static_cast<const BYTE*>(SourcePtr);

    for (UINT row = 0; row < SourceHeight; ++row) {
        RtlCopyMemory(dstStartPtr, srcStartPtr, bytesPerLine);
        dstStartPtr += destPitch;
        srcStartPtr += SourceStride;
    }
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiSetPowerComponentFState (
    IN_CONST_HANDLE DriverContextPtr,
    UINT ComponentIndex,
    UINT FState
    )
{
    auto thisPtr = static_cast<MX6DOD_DEVICE*>(DriverContextPtr);

    MX6DOD_LOG_TRACE(
        "Set component FState (ComponentIndex = %u, FState = %u)",
        ComponentIndex,
        FState);

    switch (ComponentIndex) {
    case MX6_PWRCOMPONENT_DISPLAY_IPU:
        if (FState == 0) {
            thisPtr->IpuOn();
            thisPtr->HdmiPhyOn();
        } else {
            thisPtr->HdmiPhyOff();
            thisPtr->IpuOff();
        }
        break;
    // default:
        // NT_ASSERT(!"Invalid component index");
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiPowerRuntimeControlRequest (
    IN_CONST_HANDLE /*DriverContextPtr*/,
    LPCGUID PowerControlCode,
    PVOID /*InBufferPtr*/,
    SIZE_T /*InBufferSize*/,
    PVOID /*OutBufferPtr*/,
    SIZE_T /*OutBufferSize*/,
    PSIZE_T /*BytesReturned*/
    )
{
    MX6DOD_LOG_TRACE(
        "Received power runtime control request. (PowerControlCode = %!GUID!)",
        PowerControlCode);

    return STATUS_NOT_SUPPORTED;
}

void MX6DOD_DEVICE::HdmiPhyOn ()
{
    MX6DOD_LOG_TRACE("Turning on HDMI PHY");

    UCHAR val = this->readHdmiRegister(HDMI_PHY_CONF0);

    val |= HDMI_PHY_CONF0_PDZ;
    this->writeHdmiRegister(HDMI_PHY_CONF0, val);
    KeStallExecutionProcessor(3000);

    val |= HDMI_PHY_CONF0_ENTMDS;
    this->writeHdmiRegister(HDMI_PHY_CONF0, val);
    KeStallExecutionProcessor(3000);

    val |= HDMI_PHY_CONF0_TXPWRON;
    this->writeHdmiRegister(HDMI_PHY_CONF0, val);

    // Take PHY out of reset
    this->writeHdmiRegister(HDMI_MC_PHYRSTZ, 0);
}

void MX6DOD_DEVICE::HdmiPhyOff ()
{
    MX6DOD_LOG_TRACE("Turning off HDMI PHY");

    // Put PHY in reset
    this->writeHdmiRegister(HDMI_MC_PHYRSTZ, HDMI_MC_PHYRSTZ_PHYRSTZ);

    // Put PHY in powerdown
    UCHAR val = this->readHdmiRegister(HDMI_PHY_CONF0);
    val &= ~(HDMI_PHY_CONF0_PDZ | HDMI_PHY_CONF0_ENTMDS |
             HDMI_PHY_CONF0_TXPWRON);

    this->writeHdmiRegister(HDMI_PHY_CONF0, val);
}

void MX6DOD_DEVICE::IpuOn ()
{
    MX6DOD_LOG_TRACE("Turning on IPU");
    ULONG dispGen = this->readIpuRegister(IPU_IPU_DISP_GEN_OFFSET);
    dispGen |= DI0_COUNTER_RELEASE;
    this->writeIpuRegister(IPU_IPU_DISP_GEN_OFFSET, dispGen);

    this->writeIpuRegister(IPU_IPU_CONF_OFFSET, this->ipu1Conf);
}

void MX6DOD_DEVICE::IpuOff ()
{
    MX6DOD_LOG_TRACE("Turning off IPU");
    this->writeIpuRegister(IPU_IPU_CONF_OFFSET, 0);

    ULONG dispGen = this->readIpuRegister(IPU_IPU_DISP_GEN_OFFSET);
    dispGen &= ~DI0_COUNTER_RELEASE;
    this->writeIpuRegister(IPU_IPU_DISP_GEN_OFFSET, dispGen);
}

MX6DOD_NONPAGED_SEGMENT_END; //================================================
MX6DOD_PAGED_SEGMENT_BEGIN; //=================================================

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiAddDevice (
    DEVICE_OBJECT* const PhysicalDeviceObjectPtr,
    VOID** MiniportDeviceContextPPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    // freed below in MX6DodDdiRemoveDevice
    void* contextPtr = ExAllocatePoolWithTag(
            NonPagedPoolNx,
            sizeof(MX6DOD_DEVICE),
            MX6DOD_ALLOC_TAG::DEVICE);

    if (contextPtr == nullptr) {
        MX6DOD_LOG_LOW_MEMORY(
            "Failed to allocate MX6DOD_DEVICE from nonpaged pool.");

        return STATUS_NO_MEMORY;
    }

    auto thisPtr = new (contextPtr) MX6DOD_DEVICE(PhysicalDeviceObjectPtr);

    MX6DOD_LOG_TRACE(
        "Successfully allocated device context. "
        "(thisPtr = %p, PhysicalDeviceObjectPtr = %p)",
        thisPtr,
        PhysicalDeviceObjectPtr);

    *MiniportDeviceContextPPtr = thisPtr;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiRemoveDevice (VOID* const MiniportDeviceContextPtr)
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);
    thisPtr->~MX6DOD_DEVICE();

    ExFreePoolWithTag(thisPtr, MX6DOD_ALLOC_TAG::DEVICE);

    MX6DOD_LOG_TRACE(L"Successfully deallocated device context.");
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiStartDevice (
    VOID* const MiniportDeviceContextPtr,
    DXGK_START_INFO* DxgkStartInfoPtr,
    DXGKRNL_INTERFACE* DxgkInterfacePtr,
    ULONG* NumberOfVideoPresentSourcesPtr,
    ULONG* NumberOfChildrenPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);
    thisPtr->dxgkStartInfo = *DxgkStartInfoPtr;
    thisPtr->dxgkInterface = *DxgkInterfacePtr;

    NTSTATUS status = thisPtr->dxgkInterface.DxgkCbGetDeviceInformation(
            thisPtr->dxgkInterface.DeviceHandle,
            &thisPtr->dxgkDeviceInfo);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DxgkCbGetDeviceInformation() failed. "
            "(status = %!STATUS!, dxgkInterface.DeviceHandle = %p)",
            status,
            thisPtr->dxgkInterface.DeviceHandle);

        return status;
    }

    // Find and validate hardware resources
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* ipuMemoryResourcePtr = nullptr;
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* hdmiMemoryResourcePtr = nullptr;
    {
        const CM_RESOURCE_LIST* resourceListPtr =
            thisPtr->dxgkDeviceInfo.TranslatedResourceList;

        NT_ASSERT(resourceListPtr->Count == 1);
        const CM_PARTIAL_RESOURCE_LIST* partialResourceListPtr =
            &resourceListPtr->List[0].PartialResourceList;

        ULONG memResourceCount = 0;
        for (ULONG i = 0; i < partialResourceListPtr->Count; ++i) {
            const CM_PARTIAL_RESOURCE_DESCRIPTOR* resourcePtr =
                &partialResourceListPtr->PartialDescriptors[i];

            switch (resourcePtr->Type) {
            case CmResourceTypeMemory:
                switch (memResourceCount) {
                case 2:
                    // third memory resource is the IPU block
                    ipuMemoryResourcePtr = resourcePtr;
                    break;
                case 3:
                    // fourth memory resource is the HDMI block
                    hdmiMemoryResourcePtr = resourcePtr;
                    break;
                }
                ++memResourceCount;
                break;
            }
        }

        if (ipuMemoryResourcePtr == nullptr) {
            MX6DOD_LOG_ERROR(
                "Did not find required memory resource. "
                "(TranslatedResourceList = 0x%p)",
                thisPtr->dxgkDeviceInfo.TranslatedResourceList);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        if (ipuMemoryResourcePtr->u.Memory.Length < IPU_REGISTERS_LENGTH) {
            MX6DOD_LOG_ERROR(
                "Memory region is too small to fit IPU block. "
                "(ipuMemoryResourcePtr = 0x%p)",
                ipuMemoryResourcePtr);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        if (hdmiMemoryResourcePtr == nullptr) {
            MX6DOD_LOG_ERROR(
                "Did not find HDMI memory resource. "
                "(TranslatedResourceList = 0x%p)",
                thisPtr->dxgkDeviceInfo.TranslatedResourceList);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        if (hdmiMemoryResourcePtr->u.Memory.Length < HDMI_REGISTERS_LENGTH) {
            MX6DOD_LOG_ERROR(
                "Memory region is too small to fit HDMI block. "
                "(hdmiMemoryResourcePtr = 0x%p)",
                hdmiMemoryResourcePtr);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }

    PVOID ipuRegistersPtr;
    status = thisPtr->dxgkInterface.DxgkCbMapMemory(
            thisPtr->dxgkInterface.DeviceHandle,
            ipuMemoryResourcePtr->u.Memory.Start,
            0x1000, // We only need the first page for now
            FALSE,
            FALSE,
            MmNonCached,
            reinterpret_cast<PVOID*>(&ipuRegistersPtr));

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_LOW_MEMORY(
            "Failed to map IPU registers into system address space. "
            "(status = %!STATUS!, "
            "ipuMemoryResourcePtr->u.Memory.Start = 0x%I64x, length=%d)",
            status,
            ipuMemoryResourcePtr->u.Memory.Start.QuadPart,
            IPU_REGISTERS_LENGTH);

        return status;
    }
    auto unmapIpuRegisters = MX6DOD_FINALLY::DoUnless([&] {
        PAGED_CODE();
        NTSTATUS unmapStatus = thisPtr->dxgkInterface.DxgkCbUnmapMemory(
                thisPtr->dxgkInterface.DeviceHandle,
                ipuRegistersPtr);

        UNREFERENCED_PARAMETER(unmapStatus);
        NT_ASSERT(NT_SUCCESS(unmapStatus));
    });

    PVOID hdmiRegistersPtr;
    status = thisPtr->dxgkInterface.DxgkCbMapMemory(
            thisPtr->dxgkInterface.DeviceHandle,
            hdmiMemoryResourcePtr->u.Memory.Start,
            HDMI_REGISTERS_LENGTH,
            FALSE,
            FALSE,
            MmNonCached,
            reinterpret_cast<PVOID*>(&hdmiRegistersPtr));

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_LOW_MEMORY(
            "Failed to map HDMI registers into system address space. "
            "(status = %!STATUS!, "
            "hdmiRegistersPtr->u.Memory.Start = 0x%I64x, length=%d)",
            status,
            hdmiMemoryResourcePtr->u.Memory.Start.QuadPart,
            HDMI_REGISTERS_LENGTH);

        return status;
    }
    auto unmapHdmiRegisters = MX6DOD_FINALLY::DoUnless([&] {
        PAGED_CODE();
        NTSTATUS unmapStatus = thisPtr->dxgkInterface.DxgkCbUnmapMemory(
                thisPtr->dxgkInterface.DeviceHandle,
                hdmiRegistersPtr);

        UNREFERENCED_PARAMETER(unmapStatus);
        NT_ASSERT(NT_SUCCESS(unmapStatus));
    });

    status = thisPtr->dxgkInterface.DxgkCbAcquirePostDisplayOwnership(
            thisPtr->dxgkInterface.DeviceHandle,
            &thisPtr->dxgkDisplayInfo);

    if (!NT_SUCCESS(status) || (thisPtr->dxgkDisplayInfo.Width == 0)) {
        MX6DOD_LOG_ERROR(
            "DxgkCbAcquirePostDisplayOwnership() failed. (status = %!STATUS!, "
            "thisPtr->dxgkDisplayInfo.Width = %d, "
            "thisPtr->dxgkInterface.DeviceHandle = %p)",
            status,
            thisPtr->dxgkDisplayInfo.Width,
            thisPtr->dxgkInterface.DeviceHandle);

        return STATUS_UNSUCCESSFUL;
    }

    // set up the video signal info which we'll report in RecommendMonitorModes
    // and EnumCofuncModality
    thisPtr->dxgkVideoSignalInfo.VideoStandard = D3DKMDT_VSS_OTHER;
    thisPtr->dxgkVideoSignalInfo.TotalSize.cx = thisPtr->dxgkDisplayInfo.Width;
    thisPtr->dxgkVideoSignalInfo.TotalSize.cy = thisPtr->dxgkDisplayInfo.Height;
    thisPtr->dxgkVideoSignalInfo.ActiveSize =
            thisPtr->dxgkVideoSignalInfo.TotalSize;

    thisPtr->dxgkVideoSignalInfo.VSyncFreq.Numerator =
            D3DKMDT_FREQUENCY_NOTSPECIFIED;

    thisPtr->dxgkVideoSignalInfo.VSyncFreq.Denominator =
            D3DKMDT_FREQUENCY_NOTSPECIFIED;

    thisPtr->dxgkVideoSignalInfo.HSyncFreq.Numerator =
            D3DKMDT_FREQUENCY_NOTSPECIFIED;

    thisPtr->dxgkVideoSignalInfo.HSyncFreq.Denominator =
            D3DKMDT_FREQUENCY_NOTSPECIFIED;

    thisPtr->dxgkVideoSignalInfo.PixelRate = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    thisPtr->dxgkVideoSignalInfo.ScanLineOrdering = D3DDDI_VSSLO_PROGRESSIVE;

    MX6DOD_LOG_TRACE(
        "Successfully acquired post display ownership. "
        "(Width = %d, Height = %d, Pitch = %d, ColorFormat = %d, "
        "TargetId = %d, AcpiId = %d)",
        thisPtr->dxgkDisplayInfo.Width,
        thisPtr->dxgkDisplayInfo.Height,
        thisPtr->dxgkDisplayInfo.Pitch,
        thisPtr->dxgkDisplayInfo.ColorFormat,
        thisPtr->dxgkDisplayInfo.TargetId,
        thisPtr->dxgkDisplayInfo.AcpiId);

    // map frame buffer from physical address
    ULONG frameBufferLength = thisPtr->dxgkDisplayInfo.Pitch *
        thisPtr->dxgkDisplayInfo.Height;

    void* biosFrameBufferPtr = MmMapIoSpaceEx(
            thisPtr->dxgkDisplayInfo.PhysicAddress,
            frameBufferLength,
            PAGE_READWRITE | PAGE_NOCACHE);

    if (biosFrameBufferPtr == nullptr) {
        MX6DOD_LOG_LOW_MEMORY(
            "Failed to map frame buffer into system address space. "
            "(thisPtr->dxgkDisplayInfo.PhysicAddress = 0x%I64x, "
            "frameBufferLength = %d)",
            thisPtr->dxgkDisplayInfo.PhysicAddress.QuadPart,
            frameBufferLength);

        return STATUS_NO_MEMORY;
    }

    auto unmapBiosFrameBuffer = MX6DOD_FINALLY::DoUnless([&] {
        PAGED_CODE();
        MmUnmapIoSpace(biosFrameBufferPtr, frameBufferLength);
    });

    unmapIpuRegisters.DoNot();
    thisPtr->ipuRegistersPtr = ipuRegistersPtr;

    unmapHdmiRegisters.DoNot();
    thisPtr->hdmiRegistersPtr = hdmiRegistersPtr;

    thisPtr->frameBufferLength = frameBufferLength;

    unmapBiosFrameBuffer.DoNot();
    thisPtr->biosFrameBufferPtr = biosFrameBufferPtr;

    thisPtr->ipu1Conf = thisPtr->readIpuRegister(IPU_IPU_CONF_OFFSET);

    *NumberOfVideoPresentSourcesPtr = 1;
    *NumberOfChildrenPtr = CHILD_COUNT;     // represents the HDMI connector

    MX6DOD_LOG_TRACE("Successfully started device.");
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiStopDevice (VOID* const MiniportDeviceContextPtr)
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);

    // Unmap BIOS frame buffer
    NT_ASSERT(thisPtr->biosFrameBufferPtr);
    MmUnmapIoSpace(
        thisPtr->biosFrameBufferPtr,
        thisPtr->frameBufferLength);

    thisPtr->biosFrameBufferPtr = nullptr;
    thisPtr->frameBufferLength = 0;

    // Unmap IPU register block
    NT_ASSERT(thisPtr->ipuRegistersPtr);
    NTSTATUS unmapStatus = thisPtr->dxgkInterface.DxgkCbUnmapMemory(
            thisPtr->dxgkInterface.DeviceHandle,
            thisPtr->ipuRegistersPtr);

    UNREFERENCED_PARAMETER(unmapStatus);
    NT_ASSERT(NT_SUCCESS(unmapStatus));
    thisPtr->ipuRegistersPtr = nullptr;

    // Unmap HDMI register block
    NT_ASSERT(thisPtr->hdmiRegistersPtr);
    unmapStatus = thisPtr->dxgkInterface.DxgkCbUnmapMemory(
            thisPtr->dxgkInterface.DeviceHandle,
            thisPtr->hdmiRegistersPtr);

    UNREFERENCED_PARAMETER(unmapStatus);
    NT_ASSERT(NT_SUCCESS(unmapStatus));
    thisPtr->hdmiRegistersPtr = nullptr;

    MX6DOD_LOG_TRACE("Successfully stopped device.");
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiDispatchIoRequest (
    VOID* const /*MiniportDeviceContextPtr*/,
    ULONG /*VidPnSourceId*/,
    VIDEO_REQUEST_PACKET* VideoRequestPacketPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6DOD_LOG_WARNING(
        "Custom IO requests are not supported. "
        "(VideoRequestPacketPtr->IoControlCode = 0x%lx)",
        VideoRequestPacketPtr->IoControlCode);

    return STATUS_NOT_IMPLEMENTED;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiQueryChildRelations (
    VOID* const /*MiniportDeviceContextPtr*/,
    DXGK_CHILD_DESCRIPTOR* ChildRelationsPtr,
    ULONG ChildRelationsSizeInBytes
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    // Enumerate the child devices of the adapter. There is a single child
    // to enumerate which is the HDMI connector. The number of children
    // is specified above in NumberOfChildrenPtr

    const ULONG childRelationsCount =
        ChildRelationsSizeInBytes / sizeof(*ChildRelationsPtr);

    // The caller allocates and zeros one more entry that we specifed in
    // the NumberOfChildrenPtr output parameter of StartDevice
    NT_ASSERT(childRelationsCount == (CHILD_COUNT + 1));

    // The following code assumes a single child descriptor
    NT_ASSERT(CHILD_COUNT == 1);

    ChildRelationsPtr[0].ChildDeviceType = TypeVideoOutput;
    ChildRelationsPtr[0].ChildCapabilities.HpdAwareness =
            HpdAwarenessInterruptible;

    ChildRelationsPtr[0].ChildCapabilities.Type.VideoOutput.InterfaceTechnology =
            D3DKMDT_VOT_HDMI;

    ChildRelationsPtr[0].ChildCapabilities.Type.VideoOutput.MonitorOrientationAwareness =
            D3DKMDT_MOA_NONE;

    ChildRelationsPtr[0].ChildCapabilities.Type.VideoOutput.SupportsSdtvModes =
            FALSE;

    // child device is not an ACPI device
    NT_ASSERT(ChildRelationsPtr[0].AcpiUid == 0);

    ChildRelationsPtr[0].ChildUid = 0;

    MX6DOD_LOG_TRACE("Child relations reported successfully.");
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiQueryChildStatus (
    VOID* const MiniportDeviceContextPtr,
    DXGK_CHILD_STATUS* ChildStatusPtr,
    BOOLEAN NonDestructiveOnly
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(ChildStatusPtr->ChildUid == 0);

    switch (ChildStatusPtr->Type) {
    case StatusConnection:
        //
        // Hotplug detection not currently supported - always return TRUE
        //
        ChildStatusPtr->HotPlug.Connected = TRUE;
        break;
    case StatusRotation:
        MX6DOD_LOG_ERROR(
            "Received StatusRotation query even though D3DKMDT_MOA_NONE was "
            "reported. (MiniportDeviceContextPtr = %p)",
            MiniportDeviceContextPtr);

        return STATUS_INVALID_PARAMETER;
    case StatusMiracastConnection:
        MX6DOD_LOG_ERROR(
            "Miracast is not supported. (MiniportDeviceContextPtr = %p)",
            MiniportDeviceContextPtr);

        return STATUS_NOT_SUPPORTED;
    case StatusUninitialized:
    default:
        MX6DOD_LOG_ERROR(
            "Received invalid Type value in DdiQueryChildStatus. "
            "(ChildStatusPtr->Type = %d, MiniportDeviceContextPtr = %p)",
            ChildStatusPtr->Type,
            MiniportDeviceContextPtr);

        return STATUS_INVALID_PARAMETER;
    }

    MX6DOD_LOG_TRACE(
        "Child status reported successfully. (NonDestructiveOnly = %d)",
        NonDestructiveOnly);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiQueryDeviceDescriptor (
    VOID* const /*MiniportDeviceContextPtr*/,
    ULONG ChildUid,
    DXGK_DEVICE_DESCRIPTOR* /*DeviceDescriptorPtr*/
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    UNREFERENCED_PARAMETER(ChildUid);
    NT_ASSERT(ChildUid == 0);

    MX6DOD_LOG_ERROR("Device descriptor query is not implemented.");

    return STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiSetPowerState (
    VOID* const /*MiniportDeviceContextPtr*/,
    ULONG DeviceUid,
    DEVICE_POWER_STATE DevicePowerState,
    POWER_ACTION ActionType
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    //
    // Power management is handled at the component level
    //

    MX6DOD_LOG_TRACE(
        L"Successfully set power state. "
        "(DeviceUid = %d, DevicePowerState = %d, ActionType = %d)",
        DeviceUid,
        DevicePowerState,
        ActionType);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiQueryAdapterInfo (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_QUERYADAPTERINFO* QueryAdapterInfoPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    switch (QueryAdapterInfoPtr->Type) {
    case DXGKQAITYPE_DRIVERCAPS: // DXGK_DRIVERCAPS
        DXGK_DRIVERCAPS* driverCapsPtr;
        if (QueryAdapterInfoPtr->OutputDataSize < sizeof(*driverCapsPtr)) {
            MX6DOD_LOG_ERROR(
                "Output buffer is not large enough to hold DXGK_DRIVERCAPS. "
                "(QueryAdapterInfoPtr->OutputDataSize = %d, "
                "sizeof(DXGK_DRIVERCAPS) = %d, MiniportDeviceContextPtr = %p)",
                QueryAdapterInfoPtr->OutputDataSize,
                sizeof(DXGK_DRIVERCAPS),
                MiniportDeviceContextPtr);

            return STATUS_BUFFER_TOO_SMALL;
        }
        driverCapsPtr = reinterpret_cast<DXGK_DRIVERCAPS*>(
                QueryAdapterInfoPtr->pOutputData);

        *driverCapsPtr = DXGK_DRIVERCAPS();

        // Explicitly spell out each capability for reference
        driverCapsPtr->HighestAcceptableAddress = PHYSICAL_ADDRESS{ULONG(-1)};
        driverCapsPtr->MaxAllocationListSlotId = 0;
        driverCapsPtr->ApertureSegmentCommitLimit = 0;
        driverCapsPtr->MaxPointerWidth = 0;
        driverCapsPtr->MaxPointerHeight = 0;

        // Pointer capabilities
        driverCapsPtr->PointerCaps.Monochrome = FALSE;
        driverCapsPtr->PointerCaps.Color = FALSE;
        driverCapsPtr->PointerCaps.MaskedColor = FALSE;

        driverCapsPtr->InterruptMessageNumber = 0;
        driverCapsPtr->NumberOfSwizzlingRanges = 0;
        driverCapsPtr->MaxOverlays = 0;
        driverCapsPtr->GammaRampCaps.Gamma_Rgb256x3x16 = FALSE;

        // Presentation capabilities
        driverCapsPtr->PresentationCaps.NoScreenToScreenBlt = FALSE;
        driverCapsPtr->PresentationCaps.NoOverlapScreenBlt = FALSE;
        driverCapsPtr->PresentationCaps.SupportKernelModeCommandBuffer = FALSE;
        driverCapsPtr->PresentationCaps.NoSameBitmapAlphaBlend = FALSE;
        driverCapsPtr->PresentationCaps.NoSameBitmapStretchBlt = FALSE;
        driverCapsPtr->PresentationCaps.NoSameBitmapTransparentBlt = FALSE;
        driverCapsPtr->PresentationCaps.NoSameBitmapOverlappedAlphaBlend = FALSE;
        driverCapsPtr->PresentationCaps.NoSameBitmapOverlappedStretchBlt = FALSE;
        driverCapsPtr->PresentationCaps.DriverSupportsCddDwmInterop = FALSE;
        driverCapsPtr->PresentationCaps.AlignmentShift = 0;
        driverCapsPtr->PresentationCaps.MaxTextureWidthShift = 0;
        driverCapsPtr->PresentationCaps.MaxTextureHeightShift = 0;
        driverCapsPtr->PresentationCaps.SupportAllBltRops = FALSE;
        driverCapsPtr->PresentationCaps.SupportMirrorStretchBlt = FALSE;
        driverCapsPtr->PresentationCaps.SupportMonoStretchBltModes = FALSE;
        driverCapsPtr->PresentationCaps.StagingRectStartPitchAligned = FALSE;
        driverCapsPtr->PresentationCaps.NoSameBitmapBitBlt = FALSE;
        driverCapsPtr->PresentationCaps.NoSameBitmapOverlappedBitBlt = FALSE;
        driverCapsPtr->PresentationCaps.NoTempSurfaceForClearTypeBlend = FALSE;
        driverCapsPtr->PresentationCaps.SupportSoftwareDeviceBitmaps = FALSE;
        driverCapsPtr->PresentationCaps.NoCacheCoherentApertureMemory = FALSE;
        driverCapsPtr->PresentationCaps.SupportLinearHeap = FALSE;

        driverCapsPtr->MaxQueuedFlipOnVSync = 0;

        // Flipping capabilities
        driverCapsPtr->FlipCaps.FlipOnVSyncWithNoWait = FALSE;
        driverCapsPtr->FlipCaps.FlipOnVSyncMmIo = FALSE;
        driverCapsPtr->FlipCaps.FlipInterval = FALSE;
        driverCapsPtr->FlipCaps.FlipImmediateMmIo = FALSE;
        driverCapsPtr->FlipCaps.FlipIndependent = FALSE;

        // Scheduling capabilities
        driverCapsPtr->SchedulingCaps.MultiEngineAware = FALSE;
        driverCapsPtr->SchedulingCaps.VSyncPowerSaveAware = FALSE;
        driverCapsPtr->SchedulingCaps.PreemptionAware = FALSE;
        driverCapsPtr->SchedulingCaps.NoDmaPatching = FALSE;
        driverCapsPtr->SchedulingCaps.CancelCommandAware = FALSE;
        driverCapsPtr->SchedulingCaps.No64BitAtomics = FALSE;

        // Memory management capabilities
        driverCapsPtr->MemoryManagementCaps.OutOfOrderLock = FALSE;
        driverCapsPtr->MemoryManagementCaps.DedicatedPagingEngine = FALSE;
        driverCapsPtr->MemoryManagementCaps.PagingEngineCanSwizzle = FALSE;
        driverCapsPtr->MemoryManagementCaps.SectionBackedPrimary = FALSE;
        driverCapsPtr->MemoryManagementCaps.CrossAdapterResource = FALSE;
        driverCapsPtr->MemoryManagementCaps.VirtualAddressingSupported = FALSE;
        driverCapsPtr->MemoryManagementCaps.GpuMmuSupported = FALSE;
        driverCapsPtr->MemoryManagementCaps.IoMmuSupported = FALSE;
        driverCapsPtr->MemoryManagementCaps.ReplicateGdiContent = FALSE;

        driverCapsPtr->GpuEngineTopology.NbAsymetricProcessingNodes = 0;

        // If we use 1.3 or higher, we must support FlipCaps.FlipIndependent
        driverCapsPtr->WDDMVersion = DXGKDDI_WDDMv1_2;

        // GPU preemption capabilities
        driverCapsPtr->PreemptionCaps.GraphicsPreemptionGranularity =
                D3DKMDT_GRAPHICS_PREEMPTION_NONE;

        driverCapsPtr->PreemptionCaps.ComputePreemptionGranularity =
                D3DKMDT_COMPUTE_PREEMPTION_NONE;

        // must support DxgkDdiStopDeviceAndReleasePostDisplayOwnership
        driverCapsPtr->SupportNonVGA = TRUE;

        // must support updating path rotation in
        // DxgkDdiUpdateActiveVidPnPresentPath
        driverCapsPtr->SupportSmoothRotation = TRUE;

        driverCapsPtr->SupportPerEngineTDR = FALSE;
        driverCapsPtr->SupportDirectFlip = FALSE;
        driverCapsPtr->SupportMultiPlaneOverlay = FALSE;
        driverCapsPtr->SupportRuntimePowerManagement = TRUE;
        driverCapsPtr->SupportSurpriseRemovalInHibernation = FALSE;
        driverCapsPtr->HybridDiscrete = FALSE;
        driverCapsPtr->MaxOverlayPlanes = 0;

        return STATUS_SUCCESS;

    case DXGKQAITYPE_NUMPOWERCOMPONENTS:
    {
        NT_ASSERT(QueryAdapterInfoPtr->OutputDataSize >= sizeof(UINT));
        UINT* numComponentsPtr = reinterpret_cast<UINT*>(
                QueryAdapterInfoPtr->pOutputData);

        *numComponentsPtr = MX6_PWRCOMPONENT_DISPLAY_COUNT;
        return STATUS_SUCCESS;
    }

    case DXGKQAITYPE_POWERCOMPONENTINFO:
    {
        NT_ASSERT(QueryAdapterInfoPtr->InputDataSize >= sizeof(UINT));
        UINT componentIndex = *static_cast<UINT*>(QueryAdapterInfoPtr->pInputData);

        NT_ASSERT(
            QueryAdapterInfoPtr->OutputDataSize >=
            sizeof(DXGK_POWER_RUNTIME_COMPONENT));

        DXGK_POWER_RUNTIME_COMPONENT* componentInfoPtr =
            static_cast<DXGK_POWER_RUNTIME_COMPONENT*>(
                QueryAdapterInfoPtr->pOutputData);

        switch (componentIndex) {
        case MX6_PWRCOMPONENT_DISPLAY_3DENGINE:
        {
            componentInfoPtr->StateCount = 2;
            NT_ASSERT(componentInfoPtr->StateCount <= DXGK_MAX_F_STATES);

            // F0 - fully functional
            {
                DXGK_POWER_RUNTIME_STATE* statePtr = &componentInfoPtr->States[0];
                statePtr->TransitionLatency = 0;
                statePtr->ResidencyRequirement = 0;
                statePtr->NominalPower = 100000;      // 100mW
            }

            // F1 - clock gated
            {
                DXGK_POWER_RUNTIME_STATE* statePtr = &componentInfoPtr->States[1];
                statePtr->TransitionLatency = 0;
                statePtr->ResidencyRequirement = 0;
                statePtr->NominalPower = 20000;      // 20mW
            }

            //
            // This is reported as component type OTHER rather than ENGINE
            // because dxgkrnl does not expect display-only drivers to have
            // engine components
            //
            componentInfoPtr->ComponentMapping.ComponentType =
                    DXGK_POWER_COMPONENT_OTHER;

            componentInfoPtr->ComponentMapping.EngineDesc.NodeIndex = 0;
            componentInfoPtr->Flags.Value = 0;

            componentInfoPtr->ComponentGuid =
                    MX6_PWRCOMPONENTGUID_DISPLAY_3DENGINE;

            RtlStringCchCopyA(
                reinterpret_cast<char*>(componentInfoPtr->ComponentName),
                ARRAYSIZE(componentInfoPtr->ComponentName),
                "GPU3D");

            // Dependencies
            componentInfoPtr->ProviderCount = 0;
            break;
        }

        case MX6_PWRCOMPONENT_DISPLAY_IPU:
        {
            componentInfoPtr->StateCount = 2;   // F0, F1
            NT_ASSERT(componentInfoPtr->StateCount <= DXGK_MAX_F_STATES);

            // F0 - fully functional
            {
                DXGK_POWER_RUNTIME_STATE* statePtr = &componentInfoPtr->States[0];
                statePtr->TransitionLatency = 0;
                statePtr->ResidencyRequirement = 0;
                statePtr->NominalPower = 1000;      // 1mW
            }

            // F1 - clock gated
            {
                DXGK_POWER_RUNTIME_STATE* statePtr = &componentInfoPtr->States[1];
                statePtr->TransitionLatency = 0;
                statePtr->ResidencyRequirement = 0;
                statePtr->NominalPower = 500;      // 0.5mW
            }

            componentInfoPtr->ComponentMapping.ComponentType =
                    DXGK_POWER_COMPONENT_MONITOR_REFRESH;

            componentInfoPtr->ComponentMapping.MonitorRefreshDesc.VidPnSourceID = 0;
            componentInfoPtr->Flags.Value = 0;

            componentInfoPtr->ComponentGuid = MX6_PWRCOMPONENTGUID_DISPLAY_IPU;

            RtlStringCchCopyA(
                reinterpret_cast<char*>(componentInfoPtr->ComponentName),
                ARRAYSIZE(componentInfoPtr->ComponentName),
                "IPU");

            // Dependencies
            componentInfoPtr->ProviderCount = 0;
            break;
        }

        case MX6_PWRCOMPONENT_DISPLAY_MONITOR:
        {
            componentInfoPtr->StateCount = 2;   // F0, F1
            NT_ASSERT(componentInfoPtr->StateCount <= DXGK_MAX_F_STATES);

            // F0 - on
            {
                DXGK_POWER_RUNTIME_STATE* statePtr = &componentInfoPtr->States[0];
                statePtr->TransitionLatency = 0;
                statePtr->ResidencyRequirement = 0;
                statePtr->NominalPower = 1000;      // 1mW
            }

            // F1 - off
            {
                DXGK_POWER_RUNTIME_STATE* statePtr = &componentInfoPtr->States[1];
                statePtr->TransitionLatency = 10000 * 100;        // 100ms
                statePtr->ResidencyRequirement = 10000 * 1000;    // 1000ms
                statePtr->NominalPower = 0;                       // 0mw
            }

            componentInfoPtr->ComponentMapping.ComponentType =
                    DXGK_POWER_COMPONENT_MONITOR;

            componentInfoPtr->ComponentMapping.MonitorDesc.VidPnTargetID = 0;
            componentInfoPtr->Flags.Value = 0;

            componentInfoPtr->ComponentGuid =
                    MX6_PWRCOMPONENTGUID_DISPLAY_MONITOR;

            RtlStringCchCopyA(
                reinterpret_cast<char*>(componentInfoPtr->ComponentName),
                ARRAYSIZE(componentInfoPtr->ComponentName),
                "Monitor");

            //
            // The monitor refresh component (the IPU) must be active before
            // the monitor can be made active
            //
            componentInfoPtr->ProviderCount = 1;
            componentInfoPtr->Providers[0] = MX6_PWRCOMPONENT_DISPLAY_IPU;
            break;
        }

        default:
            NT_ASSERT(!"Invalid component index");
        } // switch (componentIndex)

        return STATUS_SUCCESS;
    }
    case DXGKQAITYPE_UMDRIVERPRIVATE:
    case DXGKQAITYPE_QUERYSEGMENT:
    case DXGKQAITYPE_QUERYSEGMENT3:
    case DXGKQAITYPE_HISTORYBUFFERPRECISION:
    case DXGKQAITYPE_GPUMMUCAPS:
    default:
        MX6DOD_LOG_WARNING(
            "Recevied QueryAdapterInfo query for unsupported type. "
            "(QueryAdapterInfoPtr->Type = %d, MiniportDeviceContextPtr = %p)",
            QueryAdapterInfoPtr->Type,
            MiniportDeviceContextPtr);

        return STATUS_NOT_SUPPORTED;
    }
}

//
// Even though this driver does not support hardware cursors,
// and reports such in QueryAdapterInfo. This function can still be called to
// set the pointer to not visible
//
_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiSetPointerPosition (
    VOID* const /*MiniportDeviceContextPtr*/,
    const DXGKARG_SETPOINTERPOSITION* SetPointerPositionPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(SetPointerPositionPtr->VidPnSourceId == 0);
    if (!SetPointerPositionPtr->Flags.Visible) {
        MX6DOD_LOG_TRACE("Received request to set pointer visibility to OFF.");
        return STATUS_SUCCESS;
    } else {
        MX6DOD_LOG_ERROR(
            "SetPointerPosition should never be called to set the pointer to "
            "visible since MX6DOD doesn't support hardware cursors.");

        return STATUS_UNSUCCESSFUL;
    }
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiSetPointerShape (
    VOID* const /*MiniportDeviceContextPtr*/,
    const DXGKARG_SETPOINTERSHAPE* SetPointerShapePtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    UNREFERENCED_PARAMETER(SetPointerShapePtr);
    NT_ASSERT(SetPointerShapePtr->VidPnSourceId == 0);

    MX6DOD_LOG_ERROR(
        "SetPointerShape should never be called since MX6DOD doesn't support "
        "hardware cursors.");

    return STATUS_NOT_IMPLEMENTED;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiIsSupportedVidPn (
    VOID* const MiniportDeviceContextPtr,
    DXGKARG_ISSUPPORTEDVIDPN* IsSupportedVidPnPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IsSupportedVidPnPtr->IsVidPnSupported = FALSE;

    if (IsSupportedVidPnPtr->hDesiredVidPn == 0) {
        IsSupportedVidPnPtr->IsVidPnSupported = TRUE;
        MX6DOD_LOG_TRACE("Returning IsVidPnSupported=TRUE for hDesiredVidPn=0.");
        return STATUS_SUCCESS;
    }

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);

    // Verify that there is exactly one path and source and target IDs are both 0
    const DXGK_VIDPN_INTERFACE* vidPnInterfacePtr;
    NTSTATUS status = thisPtr->dxgkInterface.DxgkCbQueryVidPnInterface(
            IsSupportedVidPnPtr->hDesiredVidPn,
            DXGK_VIDPN_INTERFACE_VERSION_V1,
            &vidPnInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DxgkCbQueryVidPnInterface() failed. "
            "(status = %!STATUS!, IsSupportedVidPnPtr->hDesiredVidPn = %p, "
            "thisPtr = %p)",
            status,
            IsSupportedVidPnPtr->hDesiredVidPn,
            thisPtr);

        return status;
    }

    D3DKMDT_HVIDPNTOPOLOGY vidPnTopologyHandle;
    const DXGK_VIDPNTOPOLOGY_INTERFACE* topologyInterfacePtr;
    status = vidPnInterfacePtr->pfnGetTopology(
        IsSupportedVidPnPtr->hDesiredVidPn,
        &vidPnTopologyHandle,
        &topologyInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "pfnGetTopology() failed. (status = %!STATUS!, "
            "IsSupportedVidPnPtr->hDesiredVidPn = %p, vidPnInterfacePtr = %p)",
            status,
            IsSupportedVidPnPtr->hDesiredVidPn,
            vidPnInterfacePtr);

        return status;
    }

    // verify this topology contains exactly one path
    SIZE_T numPaths;
    status = topologyInterfacePtr->pfnGetNumPaths(
            vidPnTopologyHandle,
            &numPaths);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "Failed to get number of paths in topology. (status = %!STATUS!, "
            "vidPnTopologyHandle = %p)",
            status,
            vidPnTopologyHandle);

        return status;
    }

    if (numPaths != 1) {
        MX6DOD_LOG_ERROR(
            "Returning 'not supported' for a topology that does not contain "
            "exactly 1 path. (numPaths = %d, vidPnTopologyHandle = %p)",
            numPaths,
            vidPnTopologyHandle);

        NT_ASSERT(!IsSupportedVidPnPtr->IsVidPnSupported);
        return STATUS_SUCCESS;
    }

    // get the only path in the topology
    const D3DKMDT_VIDPN_PRESENT_PATH* presentPathPtr;
    status = topologyInterfacePtr->pfnAcquireFirstPathInfo(
            vidPnTopologyHandle,
            &presentPathPtr);

    if (status != STATUS_SUCCESS) {
        MX6DOD_LOG_ERROR(
            "Failed to get first path info from vidpn topology. "
            "(status = %!STATUS!, IsSupportedVidPnPtr->hDesiredVidPn = %p, "
            "vidPnTopologyHandle = %p)",
            status,
            IsSupportedVidPnPtr->hDesiredVidPn,
            vidPnTopologyHandle);

        return status;
    }
    NT_ASSERT(presentPathPtr);
    auto releasePathInfo = MX6DOD_FINALLY::Do([&, presentPathPtr] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = topologyInterfacePtr->pfnReleasePathInfo(
                vidPnTopologyHandle,
                presentPathPtr);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    // We support exactly one source and target
    if (!((presentPathPtr->VidPnSourceId == 0) &&
          (presentPathPtr->VidPnTargetId == 0)))
    {
        MX6DOD_LOG_ERROR(
            "Returning 'not supported' for out of range source or target id. "
            "(presentPathPtr->VidPnSourceId = %d, "
            "presentPathPtr->VidPnTargetId = %d)",
            presentPathPtr->VidPnSourceId,
            presentPathPtr->VidPnTargetId);

        NT_ASSERT(!IsSupportedVidPnPtr->IsVidPnSupported);
        return STATUS_SUCCESS;
    }

    MX6DOD_LOG_TRACE(
        "Returning 'supported' for vidpn. "
        "(IsSupportedVidPnPtr->hDesiredVidPn = %p)",
        IsSupportedVidPnPtr->hDesiredVidPn);

    IsSupportedVidPnPtr->IsVidPnSupported = TRUE;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiRecommendFunctionalVidPn (
    VOID* const /*MiniportDeviceContextPtr*/,
    const DXGKARG_RECOMMENDFUNCTIONALVIDPN* const /*RecommendFunctionalVidPnPtr*/
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6DOD_LOG_ERROR("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiEnumVidPnCofuncModality (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_ENUMVIDPNCOFUNCMODALITY* const EnumCofuncModalityPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);

    // get the vidPn interface
    const DXGK_VIDPN_INTERFACE* vidPnInterfacePtr;
    NTSTATUS status = thisPtr->dxgkInterface.DxgkCbQueryVidPnInterface(
            EnumCofuncModalityPtr->hConstrainingVidPn,
            DXGK_VIDPN_INTERFACE_VERSION_V1,
            &vidPnInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DxgkCbQueryVidPnInterface() failed. "
            "(status = %!STATUS!, "
            "EnumCofuncModalityPtr->hConstrainingVidPn, = %p, thisPtr = %p)",
            status,
            EnumCofuncModalityPtr->hConstrainingVidPn,
            thisPtr);

        return status;
    }

    // get the topology
    D3DKMDT_HVIDPNTOPOLOGY vidPnTopologyHandle;
    const DXGK_VIDPNTOPOLOGY_INTERFACE* topologyInterfacePtr;
    status = vidPnInterfacePtr->pfnGetTopology(
        EnumCofuncModalityPtr->hConstrainingVidPn,
        &vidPnTopologyHandle,
        &topologyInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "pfnGetTopology() failed. (status = %!STATUS!, "
            "EnumCofuncModalityPtr->hConstrainingVidPn, = %p, "
            "vidPnInterfacePtr = %p)",
            status,
            EnumCofuncModalityPtr->hConstrainingVidPn,
            vidPnInterfacePtr);

        return status;
    }

    // iterate through each path in the topology
    const D3DKMDT_VIDPN_PRESENT_PATH* presentPathPtr;
    status = topologyInterfacePtr->pfnAcquireFirstPathInfo(
            vidPnTopologyHandle,
            &presentPathPtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "Failed to get first path info from vidpn topology. "
            "(status = %!STATUS!, "
            "EnumCofuncModalityPtr->hConstrainingVidPn, = %p, "
            "vidPnTopologyHandle = %p)",
            status,
            EnumCofuncModalityPtr->hConstrainingVidPn,
            vidPnTopologyHandle);

        return status;
    }

    while (status == STATUS_SUCCESS) {
        NT_ASSERT(presentPathPtr);
        auto releasePathInfo = MX6DOD_FINALLY::Do([&, presentPathPtr] () {
            PAGED_CODE();
            NTSTATUS releaseStatus = topologyInterfacePtr->pfnReleasePathInfo(
                    vidPnTopologyHandle,
                    presentPathPtr);

            UNREFERENCED_PARAMETER(releaseStatus);
            NT_ASSERT(NT_SUCCESS(releaseStatus));
        });

        // If this source mode set isn't the pivot point, inspect the source
        // mode set and potentially add cofunctional modes
        if (!((EnumCofuncModalityPtr->EnumPivotType ==
               D3DKMDT_EPT_VIDPNSOURCE) &&
              (EnumCofuncModalityPtr->EnumPivot.VidPnSourceId ==
               presentPathPtr->VidPnSourceId)))
        {
            status = SourceHasPinnedMode(
                    EnumCofuncModalityPtr->hConstrainingVidPn,
                    vidPnInterfacePtr,
                    presentPathPtr->VidPnSourceId);

            switch (status) {
            case STATUS_SUCCESS: break;
            case STATUS_NOT_FOUND:
            {
                // This source does not have a pinned mode.
                // Create a new source mode set
                MX6DOD_LOG_TRACE(
                    "This source does not have a pinned mode. "
                    "Creating a new source mode set. "
                    "(presentPathPtr->VidPnSourceId = %d)",
                    presentPathPtr->VidPnSourceId);

                status = thisPtr->CreateAndAssignSourceModeSet(
                        EnumCofuncModalityPtr->hConstrainingVidPn,
                        vidPnInterfacePtr,
                        presentPathPtr->VidPnSourceId,
                        presentPathPtr->VidPnTargetId);

                if (!NT_SUCCESS(status)) {
                    return status;
                }

                break;
            }
            default:
                return status;
            }
        } // source mode set

        // If this target mode set isn't the pivot point, inspect the target
        // mode set and potentially add cofunctional modes
        if (!((EnumCofuncModalityPtr->EnumPivotType == D3DKMDT_EPT_VIDPNTARGET) &&
              (EnumCofuncModalityPtr->EnumPivot.VidPnTargetId ==
               presentPathPtr->VidPnTargetId)))
        {
            status = TargetHasPinnedMode(
                    EnumCofuncModalityPtr->hConstrainingVidPn,
                    vidPnInterfacePtr,
                    presentPathPtr->VidPnTargetId);

            switch (status) {
            case STATUS_SUCCESS: break;
            case STATUS_NOT_FOUND:
            {
                // This target does not have a pinned mode.
                // Create a new target mode set
                MX6DOD_LOG_TRACE(
                    "This target does not have a pinned mode. Creating a new "
                    "target mode set. (presentPathPtr->VidPnTargetId = %d)",
                    presentPathPtr->VidPnTargetId);

                status = thisPtr->CreateAndAssignTargetModeSet(
                        EnumCofuncModalityPtr->hConstrainingVidPn,
                        vidPnInterfacePtr,
                        presentPathPtr->VidPnSourceId,
                        presentPathPtr->VidPnTargetId);

                if (!NT_SUCCESS(status)) {
                    return status;
                }

                break;
            }
            default:
                return status;
            }
        } // target mode set

        D3DKMDT_VIDPN_PRESENT_PATH modifiedPresentPath = *presentPathPtr;
        bool presentPathModified = false;

        // SCALING: If this path's scaling isn't the pivot point, do work on
        // the scaling support
        if (!((EnumCofuncModalityPtr->EnumPivotType == D3DKMDT_EPT_SCALING) &&
              (EnumCofuncModalityPtr->EnumPivot.VidPnSourceId ==
               presentPathPtr->VidPnSourceId) &&
              (EnumCofuncModalityPtr->EnumPivot.VidPnTargetId ==
               presentPathPtr->VidPnTargetId)))
        {
            // If the scaling is unpinned, then modify the scaling support field
            if (presentPathPtr->ContentTransformation.Scaling ==
                D3DKMDT_VPPS_UNPINNED)
            {
                // Identity and centered scaling are supported, but not
                // any stretch modes
                MX6DOD_LOG_TRACE("Setting scaling support to Identity.");

                modifiedPresentPath.ContentTransformation.ScalingSupport =
                    D3DKMDT_VIDPN_PRESENT_PATH_SCALING_SUPPORT();

                // We do not support scaling
                modifiedPresentPath.ContentTransformation.ScalingSupport.Identity = TRUE;

                presentPathModified = true;
            }
        } // scaling

        // ROTATION: If this path's rotation isn't the pivot point, do work on
        // the rotation support
        if (!((EnumCofuncModalityPtr->EnumPivotType != D3DKMDT_EPT_ROTATION) &&
              (EnumCofuncModalityPtr->EnumPivot.VidPnSourceId ==
               presentPathPtr->VidPnSourceId) &&
              (EnumCofuncModalityPtr->EnumPivot.VidPnTargetId ==
               presentPathPtr->VidPnTargetId)))
        {
            // If the rotation is unpinned, then modify the rotation support field
            if (presentPathPtr->ContentTransformation.Rotation == D3DKMDT_VPPR_UNPINNED)
            {
                MX6DOD_LOG_TRACE("Setting rotation support to Identity.");

                modifiedPresentPath.ContentTransformation.RotationSupport =
                    D3DKMDT_VIDPN_PRESENT_PATH_ROTATION_SUPPORT();

                // We do not support rotation
                modifiedPresentPath.ContentTransformation.RotationSupport.Identity = TRUE;
                modifiedPresentPath.ContentTransformation.RotationSupport.Offset0 = TRUE;

                presentPathModified = true;
            }
        } // rotation

        // Update the content transformation
        if (presentPathModified) {
            // The correct path will be found by this function and the
            // appropriate fields updated
            status = topologyInterfacePtr->pfnUpdatePathSupportInfo(
                    vidPnTopologyHandle,
                    &modifiedPresentPath);

            if (!NT_SUCCESS(status)) {
                MX6DOD_LOG_ERROR(
                    "DXGK_VIDPNTOPOLOGY_INTERFACE::pfnUpdatePathSupportInfo() "
                    "failed. (status = %!STATUS!)",
                    status);

                return status;
            }
        }

        // get next path
        const D3DKMDT_VIDPN_PRESENT_PATH* nextPresentPathPtr;
        status = topologyInterfacePtr->pfnAcquireNextPathInfo(
            vidPnTopologyHandle,
            presentPathPtr,
            &nextPresentPathPtr);

        if (!NT_SUCCESS(status)) {
            MX6DOD_LOG_ERROR(
                "pfnAcquireNextPathInfo() failed. "
                "(status = %!STATUS!, "
                "EnumCofuncModalityPtr->hConstrainingVidPn, = %p, "
                "vidPnTopologyHandle = %p)",
                status,
                EnumCofuncModalityPtr->hConstrainingVidPn,
                vidPnTopologyHandle);

            return status;
        }

        presentPathPtr = nextPresentPathPtr;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiSetVidPnSourceVisibility (
    VOID* const /*MiniportDeviceContextPtr*/,
    const DXGKARG_SETVIDPNSOURCEVISIBILITY* SetVidPnSourceVisibilityPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6DOD_LOG_TRACE(
        "Received request to set visibility. (VidPnSourceId = %d, Visible = %d)",
        SetVidPnSourceVisibilityPtr->VidPnSourceId,
        SetVidPnSourceVisibilityPtr->Visible);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiCommitVidPn (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_COMMITVIDPN* const CommitVidPnPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6DOD_LOG_TRACE(
        "DdiCommitVidPn() was called. "
        "(hFunctionalVidPn = %p, AffectedVidPnSourceId = %d, "
        "MonitorConnectivityChecks = %d, hPrimaryAllocation = %p, "
        "Flags.PathPowerTransition = %d, Flags.PathPoweredOff = %d)",
        CommitVidPnPtr->hFunctionalVidPn,
        CommitVidPnPtr->AffectedVidPnSourceId,
        CommitVidPnPtr->MonitorConnectivityChecks,
        CommitVidPnPtr->hPrimaryAllocation,
        CommitVidPnPtr->Flags.PathPowerTransition,
        CommitVidPnPtr->Flags.PathPoweredOff);

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);

    // get the vidPn interface
    const DXGK_VIDPN_INTERFACE* vidPnInterfacePtr;
    NTSTATUS status = thisPtr->dxgkInterface.DxgkCbQueryVidPnInterface(
            CommitVidPnPtr->hFunctionalVidPn,
            DXGK_VIDPN_INTERFACE_VERSION_V1,
            &vidPnInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DxgkCbQueryVidPnInterface() failed. (status = %!STATUS!, "
            "CommitVidPnPtr->hFunctionalVidPn, = %p, thisPtr = %p)",
            status,
            CommitVidPnPtr->hFunctionalVidPn,
            thisPtr);

        return status;
    }

    // get the topology
    D3DKMDT_HVIDPNTOPOLOGY vidPnTopologyHandle;
    const DXGK_VIDPNTOPOLOGY_INTERFACE* topologyInterfacePtr;
    status = vidPnInterfacePtr->pfnGetTopology(
        CommitVidPnPtr->hFunctionalVidPn,
        &vidPnTopologyHandle,
        &topologyInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "pfnGetTopology() failed. (status = %!STATUS!, "
            "CommitVidPnPtr->hFunctionalVidPn, = %p, vidPnInterfacePtr = %p)",
            status,
            CommitVidPnPtr->hFunctionalVidPn,
            vidPnInterfacePtr);

        return status;
    }

    // Find out the number of paths now, if it's 0 don't bother with source
    // mode set and pinned mode, just clear current and then quit
    SIZE_T numPaths;
    status = topologyInterfacePtr->pfnGetNumPaths(
            vidPnTopologyHandle,
            &numPaths);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPNTOPOLOGY_INTERFACE::pfnGetNumPaths() failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }

    if (numPaths == 0) {
        MX6DOD_LOG_INFORMATION(L"There are no paths in this topology.");
        return STATUS_SUCCESS;
    }

    // Get the source mode set for this SourceId
    D3DKMDT_HVIDPNSOURCEMODESET sourceModeSetHandle;
    const DXGK_VIDPNSOURCEMODESET_INTERFACE* smsInterfacePtr;
    status = vidPnInterfacePtr->pfnAcquireSourceModeSet(
            CommitVidPnPtr->hFunctionalVidPn,
            CommitVidPnPtr->AffectedVidPnSourceId,
            &sourceModeSetHandle,
            &smsInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPN_INTERFACE::pfnAcquireSourceModeSet() failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }

    NT_ASSERT(sourceModeSetHandle);
    auto releaseSms = MX6DOD_FINALLY::Do([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = vidPnInterfacePtr->pfnReleaseSourceModeSet(
                CommitVidPnPtr->hFunctionalVidPn,
                sourceModeSetHandle);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    // Get the source mode to pin
    const D3DKMDT_VIDPN_SOURCE_MODE* pinnedSourceModeInfoPtr;
    status = smsInterfacePtr->pfnAcquirePinnedModeInfo(
            sourceModeSetHandle,
            &pinnedSourceModeInfoPtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPNSOURCEMODESET_INTERFACE::pfnAcquirePinnedModeInfo() "
            "failed. (status = %!STATUS!, sourceModeSetHandle = %p)",
            status,
            sourceModeSetHandle);

        return status;
    }

    if (status != STATUS_SUCCESS) {
        NT_ASSERT(pinnedSourceModeInfoPtr == nullptr);

        // There is no mode to pin on this source
        return STATUS_SUCCESS;
    }

    auto releaseModeInfo = MX6DOD_FINALLY::Do([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = smsInterfacePtr->pfnReleaseModeInfo(
                sourceModeSetHandle,
                pinnedSourceModeInfoPtr);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    status = thisPtr->IsVidPnSourceModeFieldsValid(pinnedSourceModeInfoPtr);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if ((pinnedSourceModeInfoPtr->Format.Graphics.PrimSurfSize.cx !=
         thisPtr->dxgkVideoSignalInfo.TotalSize.cx) ||
        (pinnedSourceModeInfoPtr->Format.Graphics.PrimSurfSize.cy !=
         thisPtr->dxgkVideoSignalInfo.TotalSize.cy))
    {
        MX6DOD_LOG_ERROR(
            "VidPn source has different size than monitor. "
            "(pinnedSourceModeInfoPtr->Format.Graphics.PrimSurfSize = %d,%d, "
            "thisPtr->dxgkVideoSignalInfo.TotalSize = %d, %d)",
            pinnedSourceModeInfoPtr->Format.Graphics.PrimSurfSize.cx,
            pinnedSourceModeInfoPtr->Format.Graphics.PrimSurfSize.cy,
            thisPtr->dxgkVideoSignalInfo.TotalSize.cx,
            thisPtr->dxgkVideoSignalInfo.TotalSize.cy);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }

    // Get the number of paths from this source so we can loop through all paths
    SIZE_T numPathsFromSource = 0;
    status = topologyInterfacePtr->pfnGetNumPathsFromSource(
            vidPnTopologyHandle,
            CommitVidPnPtr->AffectedVidPnSourceId,
            &numPathsFromSource);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPNTOPOLOGY_INTERFACE::pfnGetNumPathsFromSource() failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }

    // Loop through all paths to set this mode
    for (SIZE_T pathIndex = 0; pathIndex < numPathsFromSource; ++pathIndex) {
        // Get the target id for this path
        D3DDDI_VIDEO_PRESENT_TARGET_ID targetId;
        status = topologyInterfacePtr->pfnEnumPathTargetsFromSource(
                vidPnTopologyHandle,
                CommitVidPnPtr->AffectedVidPnSourceId,
                pathIndex,
                &targetId);

        if (!NT_SUCCESS(status)) {
            MX6DOD_LOG_ERROR(
                "DXGK_VIDPNTOPOLOGY_INTERFACE::pfnEnumPathTargetsFromSource() "
                "failed. (status = %!STATUS!)",
                status);

            return status;
        }

        // Get the actual path info
        const D3DKMDT_VIDPN_PRESENT_PATH* presentPathPtr;
        status = topologyInterfacePtr->pfnAcquirePathInfo(
                vidPnTopologyHandle,
                CommitVidPnPtr->AffectedVidPnSourceId,
                targetId,
                &presentPathPtr);

        if (!NT_SUCCESS(status)) {
            MX6DOD_LOG_ERROR(
                "DXGK_VIDPNTOPOLOGY_INTERFACE::pfnAcquirePathInfo() failed. "
                "(status = %!STATUS!, AffectedVidPnSourceId = %d, targetId = %d)",
                status,
                CommitVidPnPtr->AffectedVidPnSourceId,
                targetId);

            return status;
        }
        auto releasePathInfo = MX6DOD_FINALLY::Do([&] {
            PAGED_CODE();
            NTSTATUS releaseStatus = topologyInterfacePtr->pfnReleasePathInfo(
                    vidPnTopologyHandle,
                    presentPathPtr);

            UNREFERENCED_PARAMETER(releaseStatus);
            NT_ASSERT(NT_SUCCESS(releaseStatus));
        });

        status = thisPtr->IsVidPnPathFieldsValid(presentPathPtr);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    thisPtr->dxgkCurrentSourceMode = *pinnedSourceModeInfoPtr;

    MX6DOD_LOG_TRACE(
        "Successfully committed VidPn. (hFunctionalVidPn = %p, "
        "PrimSurfSize = %d, %d, VisibleRegionSize = %d,%d, Stride = %d, "
        "PixelFormat = %d, ColorBasis = %d, PixelValueAccessMode = %d)",
        CommitVidPnPtr->hFunctionalVidPn,
        pinnedSourceModeInfoPtr->Format.Graphics.PrimSurfSize.cx,
        pinnedSourceModeInfoPtr->Format.Graphics.PrimSurfSize.cy,
        pinnedSourceModeInfoPtr->Format.Graphics.VisibleRegionSize.cx,
        pinnedSourceModeInfoPtr->Format.Graphics.VisibleRegionSize.cy,
        pinnedSourceModeInfoPtr->Format.Graphics.Stride,
        pinnedSourceModeInfoPtr->Format.Graphics.PixelFormat,
        pinnedSourceModeInfoPtr->Format.Graphics.ColorBasis,
        pinnedSourceModeInfoPtr->Format.Graphics.PixelValueAccessMode);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiUpdateActiveVidPnPresentPath (
    VOID* const /*MiniportDeviceContextPtr*/,
    const DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH* const /*ArgsPtr*/
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6DOD_LOG_ERROR("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiRecommendMonitorModes (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_RECOMMENDMONITORMODES* const RecommendMonitorModesPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);
    NT_ASSERT(RecommendMonitorModesPtr->VideoPresentTargetId == 0);

    const auto& tbl = *RecommendMonitorModesPtr->pMonitorSourceModeSetInterface;

    D3DKMDT_MONITOR_SOURCE_MODE* monitorModePtr;
    NTSTATUS status = tbl.pfnCreateNewModeInfo(
            RecommendMonitorModesPtr->hMonitorSourceModeSet,
            &monitorModePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "pfnCreateNewModeInfo() failed. (status = %!STATUS!)",
            status);

        return status;
    }
    auto releaseMonitorMode = MX6DOD_FINALLY::DoUnless([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = tbl.pfnReleaseModeInfo(
                RecommendMonitorModesPtr->hMonitorSourceModeSet,
                monitorModePtr);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    *monitorModePtr = D3DKMDT_MONITOR_SOURCE_MODE();
    monitorModePtr->VideoSignalInfo = thisPtr->dxgkVideoSignalInfo;

    // We set the preference to PREFERRED since this is the only supported mode
    monitorModePtr->Origin = D3DKMDT_MCO_DRIVER;
    monitorModePtr->Preference = D3DKMDT_MP_PREFERRED;
    monitorModePtr->ColorBasis = D3DKMDT_CB_SRGB;
    monitorModePtr->ColorCoeffDynamicRanges.FirstChannel = 8;
    monitorModePtr->ColorCoeffDynamicRanges.SecondChannel = 8;
    monitorModePtr->ColorCoeffDynamicRanges.ThirdChannel = 8;
    monitorModePtr->ColorCoeffDynamicRanges.FourthChannel = 8;

    status = tbl.pfnAddMode(
            RecommendMonitorModesPtr->hMonitorSourceModeSet,
            monitorModePtr);

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET) {
            status = STATUS_SUCCESS;
        } else {
            MX6DOD_LOG_ERROR(
                "pfnAddMode failed. (status = %!STATUS!, "
                "RecommendMonitorModesPtr->hMonitorSourceModeSet = %p, "
                "monitorModePtr = %p)",
                status,
                RecommendMonitorModesPtr->hMonitorSourceModeSet,
                monitorModePtr);
        }
        return status;
    }

    releaseMonitorMode.DoNot();

    MX6DOD_LOG_TRACE(
        "Added single monitor mode. (...TotalSize = %d,%d)",
        monitorModePtr->VideoSignalInfo.TotalSize.cx,
        monitorModePtr->VideoSignalInfo.TotalSize.cy);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiQueryVidPnHWCapability (
    VOID* const /*MiniportDeviceContextPtr*/,
    DXGKARG_QUERYVIDPNHWCAPABILITY* VidPnHWCapsPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6DOD_LOG_TRACE(
        "DdiQueryVidPnHWCapability() was called. (hFunctionalVidPn = %p, "
        "SourceId = %d, TargetId = %d)",
        VidPnHWCapsPtr->hFunctionalVidPn,
        VidPnHWCapsPtr->SourceId,
        VidPnHWCapsPtr->TargetId);

    NT_ASSERT(VidPnHWCapsPtr->SourceId == 0);
    NT_ASSERT(VidPnHWCapsPtr->TargetId == 0);

    // we do not have hardware capability for any transformations
    VidPnHWCapsPtr->VidPnHWCaps.DriverRotation = FALSE;
    VidPnHWCapsPtr->VidPnHWCaps.DriverScaling = FALSE;
    VidPnHWCapsPtr->VidPnHWCaps.DriverCloning = FALSE;
    VidPnHWCapsPtr->VidPnHWCaps.DriverColorConvert = TRUE;
    VidPnHWCapsPtr->VidPnHWCaps.DriverLinkedAdapaterOutput = FALSE;
    VidPnHWCapsPtr->VidPnHWCaps.DriverRemoteDisplay = FALSE;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiPresentDisplayOnly (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_PRESENT_DISPLAYONLY* PresentDisplayOnlyPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    auto thisPtr = reinterpret_cast<MX6DOD_DEVICE*>(MiniportDeviceContextPtr);

    MX6DOD_LOG_TRACE(
        "Received present call. (VidPnSourceId = %d, BytesPerPixel = %d, "
        "Pitch = %d, Flags.Rotate = %d, NumMoves = %d, NumDirtyRects = %d)",
        PresentDisplayOnlyPtr->VidPnSourceId,
        PresentDisplayOnlyPtr->BytesPerPixel,
        PresentDisplayOnlyPtr->Pitch,
        PresentDisplayOnlyPtr->Flags.Rotate,
        PresentDisplayOnlyPtr->NumMoves,
        PresentDisplayOnlyPtr->NumDirtyRects);

    NT_ASSERT(PresentDisplayOnlyPtr->BytesPerPixel == 4);
    NT_ASSERT(!PresentDisplayOnlyPtr->Flags.Rotate);

    //
    // Copy source pixels to frame buffer
    //
    __try {

        BltBits(
            PresentDisplayOnlyPtr->pSource,
            PresentDisplayOnlyPtr->Pitch,
            thisPtr->biosFrameBufferPtr,
            thisPtr->dxgkDisplayInfo.Pitch,
            PresentDisplayOnlyPtr->pDirtyRect,
            PresentDisplayOnlyPtr->NumDirtyRects);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        MX6DOD_LOG_ERROR("An exception occurred while accessing the user buffer.");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::DdiStopDeviceAndReleasePostDisplayOwnership (
    VOID* const /*MiniportDeviceContextPtr*/,
    D3DDDI_VIDEO_PRESENT_TARGET_ID /*TargetId*/,
    DXGK_DISPLAY_INFORMATION* /*DisplayInfoPtr*/
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    MX6DOD_LOG_ERROR("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

//
// Returns STATUS_SUCCESS if the source has a pinned mode, or STATUS_NOT_FOUND
// if the source does not have a pinned mode.
//
_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::SourceHasPinnedMode (
    D3DKMDT_HVIDPN VidPnHandle,
    const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
    D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID SourceId
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    // Get the source mode set for this SourceId
    D3DKMDT_HVIDPNSOURCEMODESET sourceModeSetHandle;
    const DXGK_VIDPNSOURCEMODESET_INTERFACE* smsInterfacePtr;
    NTSTATUS status = VidPnInterfacePtr->pfnAcquireSourceModeSet(
            VidPnHandle,
            SourceId,
            &sourceModeSetHandle,
            &smsInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPN_INTERFACE::pfnAcquireSourceModeSet() failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }
    NT_ASSERT(sourceModeSetHandle);
    auto releaseSms = MX6DOD_FINALLY::Do([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = VidPnInterfacePtr->pfnReleaseSourceModeSet(
                VidPnHandle,
                sourceModeSetHandle);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    // Get the pinned mode info for this source info set
    const D3DKMDT_VIDPN_SOURCE_MODE* pinnedSourceModeInfoPtr;
    status = smsInterfacePtr->pfnAcquirePinnedModeInfo(
            sourceModeSetHandle,
            &pinnedSourceModeInfoPtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPNSOURCEMODESET_INTERFACE::pfnAcquirePinnedModeInfo() "
            "failed. (status = %!STATUS!, sourceModeSetHandle = %p)",
            status,
            sourceModeSetHandle);

        return status;
    }

    if (status != STATUS_SUCCESS) {
        NT_ASSERT(pinnedSourceModeInfoPtr == nullptr);
        return STATUS_NOT_FOUND;
    }

    auto releaseModeInfo = MX6DOD_FINALLY::Do([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = smsInterfacePtr->pfnReleaseModeInfo(
                sourceModeSetHandle,
                pinnedSourceModeInfoPtr);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    return STATUS_SUCCESS;
}

//
// Creates a new source mode set, adds a mode that is compatible with the
// connected monitor
//
_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::CreateAndAssignSourceModeSet (
    D3DKMDT_HVIDPN VidPnHandle,
    const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
    D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID SourceId,
    D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID /*TargetId*/
    ) const
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    // Create a new source mode set which will be added to the constraining
    // VidPn with all the possible modes
    D3DKMDT_HVIDPNSOURCEMODESET sourceModeSetHandle;
    const DXGK_VIDPNSOURCEMODESET_INTERFACE* smsInterfacePtr;
    NTSTATUS status = VidPnInterfacePtr->pfnCreateNewSourceModeSet(
            VidPnHandle,
            SourceId,
            &sourceModeSetHandle,
            &smsInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPN_INTERFACE::pfnCreateNewSourceModeSet failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }
    NT_ASSERT(sourceModeSetHandle);
    auto releaseSms = MX6DOD_FINALLY::DoUnless([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = VidPnInterfacePtr->pfnReleaseSourceModeSet(
                VidPnHandle,
                sourceModeSetHandle);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    // Create new mode info and add it to the source mode set
    {
        D3DKMDT_VIDPN_SOURCE_MODE* sourceModeInfoPtr;
        status = smsInterfacePtr->pfnCreateNewModeInfo(
                sourceModeSetHandle,
                &sourceModeInfoPtr);

        if (!NT_SUCCESS(status)) {
            MX6DOD_LOG_ERROR(
                "DXGK_VIDPNSOURCEMODESET_INTERFACE::pfnCreateNewModeInfo() "
                "failed. (status = %!STATUS!)",
                status);

            return status;
        }
        NT_ASSERT(sourceModeInfoPtr);
        auto releaseModeInfo = MX6DOD_FINALLY::DoUnless([&] () {
            PAGED_CODE();
            NTSTATUS releaseStatus = smsInterfacePtr->pfnReleaseModeInfo(
                    sourceModeSetHandle,
                    sourceModeInfoPtr);

            UNREFERENCED_PARAMETER(releaseStatus);
            NT_ASSERT(NT_SUCCESS(releaseStatus));
        });

        // Populate mode info with values from current mode and hard-coded values
        // Always report 32 bpp format, this will be color converted during the
        // present if the mode is < 32bpp
        *sourceModeInfoPtr = D3DKMDT_VIDPN_SOURCE_MODE();
        sourceModeInfoPtr->Type = D3DKMDT_RMT_GRAPHICS;
        sourceModeInfoPtr->Format.Graphics.PrimSurfSize.cx =
                this->dxgkDisplayInfo.Width;

        sourceModeInfoPtr->Format.Graphics.PrimSurfSize.cy =
                this->dxgkDisplayInfo.Height;

        sourceModeInfoPtr->Format.Graphics.VisibleRegionSize =
                sourceModeInfoPtr->Format.Graphics.PrimSurfSize;

        sourceModeInfoPtr->Format.Graphics.Stride = this->dxgkDisplayInfo.Pitch;
        sourceModeInfoPtr->Format.Graphics.PixelFormat = D3DDDIFMT_A8R8G8B8;
        sourceModeInfoPtr->Format.Graphics.ColorBasis = D3DKMDT_CB_SCRGB;
        sourceModeInfoPtr->Format.Graphics.PixelValueAccessMode =
                D3DKMDT_PVAM_DIRECT;

        MX6DOD_LOG_TRACE(
            "Adding source mode. (PrimSurfSize = %d,%d, Stride = %d, "
            "PixelFormat = %d, ColorBasis = %d, PixelValueAccessMode = %d)",
            sourceModeInfoPtr->Format.Graphics.PrimSurfSize.cx,
            sourceModeInfoPtr->Format.Graphics.PrimSurfSize.cy,
            sourceModeInfoPtr->Format.Graphics.Stride,
            sourceModeInfoPtr->Format.Graphics.PixelFormat,
            sourceModeInfoPtr->Format.Graphics.ColorBasis,
            sourceModeInfoPtr->Format.Graphics.PixelValueAccessMode);

        // Add mode to the source mode set
        status = smsInterfacePtr->pfnAddMode(
                sourceModeSetHandle,
                sourceModeInfoPtr);

        if (!NT_SUCCESS(status)) {
            MX6DOD_LOG_ERROR(
                "DXGK_VIDPNSOURCEMODESET_INTERFACE::pfnAddMode() failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }
        releaseModeInfo.DoNot();
    } // source mode info

    // Assign source mode set to source
    status = VidPnInterfacePtr->pfnAssignSourceModeSet(
            VidPnHandle,
            SourceId,
            sourceModeSetHandle);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPN_INTERFACE::pfnAssignSourceModeSet() failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }
    releaseSms.DoNot();

    return STATUS_SUCCESS;
}

//
// Returns STATUS_SUCCESS if the source has a pinned mode, or STATUS_NOT_FOUND
// if the target does not have a pinned mode.
//
_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::TargetHasPinnedMode (
    D3DKMDT_HVIDPN VidPnHandle,
    const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
    D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID TargetId
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    // Get the source mode set for this SourceId
    D3DKMDT_HVIDPNTARGETMODESET targetModeSetHandle;
    const DXGK_VIDPNTARGETMODESET_INTERFACE* tmsInterfacePtr;
    NTSTATUS status = VidPnInterfacePtr->pfnAcquireTargetModeSet(
            VidPnHandle,
            TargetId,
            &targetModeSetHandle,
            &tmsInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPN_INTERFACE::pfnAcquireTargetModeSet() failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }
    NT_ASSERT(targetModeSetHandle);
    auto releaseTargetModeSet = MX6DOD_FINALLY::Do([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = VidPnInterfacePtr->pfnReleaseTargetModeSet(
                VidPnHandle,
                targetModeSetHandle);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    // Get the pinned mode info for this source info set
    const D3DKMDT_VIDPN_TARGET_MODE* pinnedTargetModeInfoPtr;
    status = tmsInterfacePtr->pfnAcquirePinnedModeInfo(
            targetModeSetHandle,
            &pinnedTargetModeInfoPtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPNTARGETMODESET_INTERFACE::pfnAcquirePinnedModeInfo() "
            "failed. (status = %!STATUS!, targetModeSetHandle = %p)",
            status,
            targetModeSetHandle);

        return status;
    }

    if (status != STATUS_SUCCESS) {
        NT_ASSERT(!pinnedTargetModeInfoPtr);
        return STATUS_NOT_FOUND;
    }

    auto releaseModeInfo = MX6DOD_FINALLY::Do([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = tmsInterfacePtr->pfnReleaseModeInfo(
                targetModeSetHandle,
                pinnedTargetModeInfoPtr);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    return STATUS_SUCCESS;
}

//
// Creates a new target mode set, adds a mode that is compatible with the
// connected monitor.
//
_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::CreateAndAssignTargetModeSet (
    D3DKMDT_HVIDPN VidPnHandle,
    const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
    D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID /*SourceId*/,
    D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID TargetId
    ) const
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    D3DKMDT_HVIDPNTARGETMODESET targetModeSetHandle;
    const DXGK_VIDPNTARGETMODESET_INTERFACE* tmsInterfacePtr;
    NTSTATUS status = VidPnInterfacePtr->pfnCreateNewTargetModeSet(
            VidPnHandle,
            TargetId,
            &targetModeSetHandle,
            &tmsInterfacePtr);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPN_INTERFACE::pfnCreateNewTargetModeSet failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }
    NT_ASSERT(targetModeSetHandle);
    auto releaseSms = MX6DOD_FINALLY::DoUnless([&] () {
        PAGED_CODE();
        NTSTATUS releaseStatus = VidPnInterfacePtr->pfnReleaseTargetModeSet(
                VidPnHandle,
                targetModeSetHandle);

        UNREFERENCED_PARAMETER(releaseStatus);
        NT_ASSERT(NT_SUCCESS(releaseStatus));
    });

    // Create new mode info and add it to the target mode set
    {
        D3DKMDT_VIDPN_TARGET_MODE* targetModeInfoPtr;
        status = tmsInterfacePtr->pfnCreateNewModeInfo(
                targetModeSetHandle,
                &targetModeInfoPtr);

        if (!NT_SUCCESS(status)) {
            MX6DOD_LOG_ERROR(
                "DXGK_VIDPNTARGETMODESET_INTERFACE::pfnCreateNewModeInfo() "
                "failed. (status = %!STATUS!)",
                status);

            return status;
        }
        NT_ASSERT(targetModeInfoPtr);
        auto releaseModeInfo = MX6DOD_FINALLY::DoUnless([&] () {
            PAGED_CODE();
            NTSTATUS releaseStatus = tmsInterfacePtr->pfnReleaseModeInfo(
                    targetModeSetHandle,
                    targetModeInfoPtr);

            UNREFERENCED_PARAMETER(releaseStatus);
            NT_ASSERT(NT_SUCCESS(releaseStatus));
        });

        // Report the same video signal info that we reported in
        // RecommendMonitorModes
        targetModeInfoPtr->Id = TargetId;
        targetModeInfoPtr->VideoSignalInfo = this->dxgkVideoSignalInfo;
        targetModeInfoPtr->Preference = D3DKMDT_MP_PREFERRED;

        // Add mode to the target mode set
        status = tmsInterfacePtr->pfnAddMode(
                targetModeSetHandle,
                targetModeInfoPtr);

        if (!NT_SUCCESS(status)) {
            MX6DOD_LOG_ERROR(
                "DXGK_VIDPNTARGETMODESET_INTERFACE::pfnAddMode() failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }
        releaseModeInfo.DoNot();
    } // target mode info

    // Assign target mode set to target
    status = VidPnInterfacePtr->pfnAssignTargetModeSet(
            VidPnHandle,
            TargetId,
            targetModeSetHandle);

    if (!NT_SUCCESS(status)) {
        MX6DOD_LOG_ERROR(
            "DXGK_VIDPN_INTERFACE::pfnAssignTargetModeSet() failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }
    releaseSms.DoNot();

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::IsVidPnSourceModeFieldsValid (
    const D3DKMDT_VIDPN_SOURCE_MODE* SourceModePtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    if (SourceModePtr->Type != D3DKMDT_RMT_GRAPHICS) {
        MX6DOD_LOG_ERROR(
            "Pinned source mode is not a graphics mode. "
            "(SourceModePtr->Type = %d)",
            SourceModePtr->Type);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }

    if ((SourceModePtr->Format.Graphics.ColorBasis != D3DKMDT_CB_SCRGB) &&
        (SourceModePtr->Format.Graphics.ColorBasis != D3DKMDT_CB_UNINITIALIZED))
    {
        MX6DOD_LOG_ERROR(
            "Pinned source mode has a non-linear RGB color basis. "
            "(ColorBasis = %d)",
            SourceModePtr->Format.Graphics.ColorBasis);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }

    if (SourceModePtr->Format.Graphics.PixelValueAccessMode !=
        D3DKMDT_PVAM_DIRECT) {

        MX6DOD_LOG_ERROR(
            "Pinned source mode has a palettized access mode. "
            "(PixelValueAccessMode = %d)",
            SourceModePtr->Format.Graphics.PixelValueAccessMode);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }

    if (SourceModePtr->Format.Graphics.PixelFormat != D3DDDIFMT_A8R8G8B8) {
        MX6DOD_LOG_ERROR(
            "Pinned source mode has invalid pixel format. "
            "(SourceModePtr->Format.Graphics.PixelFormat = %d, "
            "D3DDDIFMT_A8R8G8B8 = %d)",
            SourceModePtr->Format.Graphics.PixelFormat,
            D3DDDIFMT_A8R8G8B8);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MX6DOD_DEVICE::IsVidPnPathFieldsValid (
    const D3DKMDT_VIDPN_PRESENT_PATH* PathPtr
    )
{
    PAGED_CODE();
    MX6DOD_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    if (PathPtr->VidPnSourceId != 0) {
        MX6DOD_LOG_ERROR(
            "Path contains invalid source id. (VidPnSourceId = %d)",
            PathPtr->VidPnSourceId);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE;
    }

    if (PathPtr->VidPnTargetId != 0) {
        MX6DOD_LOG_ERROR(
            "Path contains invalid target id. (VidPnTargetId = %d)",
            PathPtr->VidPnTargetId);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET;
    }

    if (PathPtr->GammaRamp.Type != D3DDDI_GAMMARAMP_DEFAULT)
    {
        MX6DOD_LOG_ERROR(
            "Path contains a gamma ramp. (GammaRamp.Type = %d)",
            PathPtr->GammaRamp.Type);

        return STATUS_GRAPHICS_GAMMA_RAMP_NOT_SUPPORTED;
    }

    if ((PathPtr->ContentTransformation.Scaling != D3DKMDT_VPPS_IDENTITY) &&
        (PathPtr->ContentTransformation.Scaling != D3DKMDT_VPPS_NOTSPECIFIED) &&
        (PathPtr->ContentTransformation.Scaling != D3DKMDT_VPPS_UNINITIALIZED))
    {
        MX6DOD_LOG_ERROR(
            "Path contains a non-identity scaling. "
            "(ContentTransformation.Scaling = %d)",
            PathPtr->ContentTransformation.Scaling);

        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }

    if ((PathPtr->ContentTransformation.Rotation != D3DKMDT_VPPR_IDENTITY) &&
        (PathPtr->ContentTransformation.Rotation != D3DKMDT_VPPR_NOTSPECIFIED) &&
        (PathPtr->ContentTransformation.Rotation != D3DKMDT_VPPR_UNINITIALIZED))
    {
        MX6DOD_LOG_ERROR(
            "Path contains a not-identity rotation. "
            "(ContentTransformation.Rotation = %d)",
            PathPtr->ContentTransformation.Rotation);

        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }

    if ((PathPtr->VidPnTargetColorBasis != D3DKMDT_CB_SCRGB) &&
        (PathPtr->VidPnTargetColorBasis != D3DKMDT_CB_UNINITIALIZED))
    {
        MX6DOD_LOG_ERROR(
            "Path has a non-linear RGB color basis. (VidPnTargetColorBasis = %d)",
            PathPtr->VidPnTargetColorBasis);

        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }

    return STATUS_SUCCESS;
}

MX6DOD_PAGED_SEGMENT_END; //===================================================

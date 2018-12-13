// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//  MX6DodDevice.h
//
// Abstract:
//
//    This is MX6DOD device implementation
//
// Environment:
//
//    Kernel mode only.
//

#ifndef _MX6DODDEVICE_HPP_
#define  _MX6DODDEVICE_HPP_ 1

class MX6DOD_DEVICE {
public: // NONPAGED

    static DXGKDDI_RESET_DEVICE DdiResetDevice;
    static DXGKDDI_SYSTEM_DISPLAY_ENABLE DdiSystemDisplayEnable;
    static DXGKDDI_SYSTEM_DISPLAY_WRITE DdiSystemDisplayWrite;
    static DXGKDDISETPOWERCOMPONENTFSTATE DdiSetPowerComponentFState;
    static DXGKDDIPOWERRUNTIMECONTROLREQUEST DdiPowerRuntimeControlRequest;

    static DXGKDDI_INTERRUPT_ROUTINE DdiInterruptRoutine;
    static DXGKDDI_DPC_ROUTINE DdiDpcRoutine;

    __forceinline MX6DOD_DEVICE (const DEVICE_OBJECT* PhysicalDeviceObjectPtr) :
        physicalDeviceObjectPtr(PhysicalDeviceObjectPtr),
        dxgkInterface(),
        dxgkStartInfo(),
        dxgkDeviceInfo(),
        dxgkDisplayInfo(),
        dxgkVideoSignalInfo(),
        dxgkCurrentSourceMode(),
        ipuRegistersPtr(),
        frameBufferLength(0),
        biosFrameBufferPtr(),
        ipu1Conf(0)
    {}

private: // NONPAGED

    enum : ULONG { CHILD_COUNT = 1 };

    enum POWER_COMPONENT {
        POWER_COMPONENT_GPU3D,
        POWER_COMPONENT_IPU,
        POWER_COMPONENT_MONITOR,
        POWER_COMPONENT_COUNT,
    };

    MX6DOD_DEVICE (const MX6DOD_DEVICE&) = delete;
    MX6DOD_DEVICE& operator= (const MX6DOD_DEVICE&) = delete;

    void IpuOn ();
    void IpuOff ();

    void HdmiPhyOn ();
    void HdmiPhyOff ();

    __forceinline void writeIpuRegister (ULONG Offset, ULONG Value) const
    {
        WRITE_REGISTER_NOFENCE_ULONG(
            reinterpret_cast<ULONG*>(
                reinterpret_cast<char*>(this->ipuRegistersPtr) +
                Offset),
            Value);
    }

    __forceinline ULONG readIpuRegister (ULONG Offset) const
    {
        return READ_REGISTER_NOFENCE_ULONG(reinterpret_cast<ULONG*>(
            reinterpret_cast<char*>(this->ipuRegistersPtr) + Offset));
    }

    __forceinline void writeHdmiRegister (ULONG Offset, UCHAR Value) const
    {
        WRITE_REGISTER_NOFENCE_UCHAR(
            reinterpret_cast<UCHAR*>(
                reinterpret_cast<char*>(this->hdmiRegistersPtr) +
                Offset),
            Value);
    }

    __forceinline UCHAR readHdmiRegister (ULONG Offset) const
    {
        return READ_REGISTER_NOFENCE_UCHAR(reinterpret_cast<UCHAR*>(
            reinterpret_cast<char*>(this->hdmiRegistersPtr) + Offset));
    }

    const DEVICE_OBJECT* const physicalDeviceObjectPtr;
    DXGKRNL_INTERFACE dxgkInterface;
    DXGK_START_INFO dxgkStartInfo;
    DXGK_DEVICE_INFO dxgkDeviceInfo;
    DXGK_DISPLAY_INFORMATION dxgkDisplayInfo;
    D3DKMDT_VIDEO_SIGNAL_INFO dxgkVideoSignalInfo;
    D3DKMDT_VIDPN_SOURCE_MODE dxgkCurrentSourceMode;

    PVOID ipuRegistersPtr;
    PVOID hdmiRegistersPtr;
    SIZE_T frameBufferLength;
    VOID* biosFrameBufferPtr;       // must be freed with MmUnmapIoSpace

    ULONG ipu1Conf;

public: // PAGED

    static DXGKDDI_ADD_DEVICE DdiAddDevice;
    static DXGKDDI_REMOVE_DEVICE DdiRemoveDevice;
    static DXGKDDI_START_DEVICE DdiStartDevice;
    static DXGKDDI_STOP_DEVICE DdiStopDevice;
    static DXGKDDI_DISPATCH_IO_REQUEST DdiDispatchIoRequest;

    static DXGKDDI_QUERY_CHILD_RELATIONS DdiQueryChildRelations;
    static DXGKDDI_QUERY_CHILD_STATUS DdiQueryChildStatus;
    static DXGKDDI_QUERY_DEVICE_DESCRIPTOR DdiQueryDeviceDescriptor;
    static DXGKDDI_SET_POWER_STATE DdiSetPowerState;

    static DXGKDDI_QUERYADAPTERINFO DdiQueryAdapterInfo;
    static DXGKDDI_SETPOINTERPOSITION DdiSetPointerPosition;
    static DXGKDDI_SETPOINTERSHAPE DdiSetPointerShape;

    static DXGKDDI_ISSUPPORTEDVIDPN DdiIsSupportedVidPn;
    static DXGKDDI_RECOMMENDFUNCTIONALVIDPN DdiRecommendFunctionalVidPn;
    static DXGKDDI_ENUMVIDPNCOFUNCMODALITY DdiEnumVidPnCofuncModality;
    static DXGKDDI_SETVIDPNSOURCEVISIBILITY DdiSetVidPnSourceVisibility;
    static DXGKDDI_COMMITVIDPN DdiCommitVidPn;
    static DXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH DdiUpdateActiveVidPnPresentPath;

    static DXGKDDI_RECOMMENDMONITORMODES DdiRecommendMonitorModes;
    static DXGKDDI_QUERYVIDPNHWCAPABILITY DdiQueryVidPnHWCapability;
    static DXGKDDI_PRESENTDISPLAYONLY DdiPresentDisplayOnly;

    static DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP
        DdiStopDeviceAndReleasePostDisplayOwnership;

private: // PAGED

    _IRQL_requires_(PASSIVE_LEVEL)
    static NTSTATUS SourceHasPinnedMode (
        D3DKMDT_HVIDPN VidPnHandle,
        const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
        D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID SourceId
        );

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS CreateAndAssignSourceModeSet (
        D3DKMDT_HVIDPN VidPnHandle,
        const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
        D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID SourceId,
        D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID TargetId
        ) const;

    _IRQL_requires_(PASSIVE_LEVEL)
    static NTSTATUS TargetHasPinnedMode (
        D3DKMDT_HVIDPN VidPnHandle,
        const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
        D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID TargetId
        );

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS CreateAndAssignTargetModeSet (
        D3DKMDT_HVIDPN VidPnHandle,
        const DXGK_VIDPN_INTERFACE* VidPnInterfacePtr,
        D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID SourceId,
        D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID TargetId
        ) const;

    _IRQL_requires_(PASSIVE_LEVEL)
    static NTSTATUS IsVidPnSourceModeFieldsValid (
        const D3DKMDT_VIDPN_SOURCE_MODE* SourceModePtr
        );

    _IRQL_requires_(PASSIVE_LEVEL)
    static NTSTATUS IsVidPnPathFieldsValid (
        const D3DKMDT_VIDPN_PRESENT_PATH* PathPtr
        );
};

#endif // _MX6DODDEVICE_HPP_

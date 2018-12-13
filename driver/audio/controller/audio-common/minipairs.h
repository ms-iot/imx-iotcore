/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Local audio endpoint filter definitions.

*/

#pragma once

#include "speakerhptopo.h"
#include "speakerhptoptable.h"
#include "speakerhpwavtable.h"
#include "micinhptopotable.h"

NTSTATUS
CreateMiniportWaveRTIMXWAV
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_TYPE,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

NTSTATUS
CreateMiniportTopologyIMXWAV
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_TYPE,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

//
// Render miniports.
//
/*********************************************************************
* Topology/Wave bridge connection for speaker (external:headphone)   *
*                                                                    *
*              +------+                +------+                      *
*              | Wave |                | Topo |                      *
*              |      |                |      |                      *
* System   --->|0    1|--------------->|0    1|---> Line Out         *
*              |      |                |      |                      *
*              |      |                |      |                      *
*              +------+                +------+                      *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE SpeakerHpTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER_SOURCE,   // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR SpeakerHpMiniports =
{
    eSpeakerHpDevice,
    L"TopologySpeakerHeadphone",            // make sure this name matches with KSNAME_TopologySpeakerHeadphone in the inf's [Strings] section 
    CreateMiniportTopologyIMXWAV,
    &SpeakerHpTopoMiniportFilterDescriptor,
    L"WaveSpeakerHeadphone",                // make sure this name matches with KSNAME_WaveSpeakerHeadphone in the inf's [Strings] section
    CreateMiniportWaveRTIMXWAV,
    &SpeakerHpWaveMiniportFilterDescriptor,
    SPEAKERHP_DEVICE_MAX_CHANNELS,
    SpeakerHpPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerHpPinDeviceFormatsAndModes),
    SpeakerHpTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerHpTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS
};

//=============================================================================
//
// Render miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_RenderEndpoints[] = 
{
    &SpeakerHpMiniports,
};

#define g_cRenderEndpoints  (SIZEOF_ARRAY(g_RenderEndpoints))

static
PHYSICALCONNECTIONTABLE MicInTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};


static
ENDPOINT_MINIPAIR MicInMiniports =
{
    eMicInDevice,
    L"TopologyMicIn",                                   // make sure this name matches with KSNAME_TopologyMicIn in the inf's [Strings] section 
    CreateMiniportTopologyIMXWAV,
    &MicInTopoMiniportFilterDescriptor,
    L"WaveMicIn",                                       // make sure this name matches with KSNAME_WaveMicIn in the inf's [Strings] section
    CreateMiniportWaveRTIMXWAV,
    &MicInWaveMiniportFilterDescriptor,
    1,                                             // DeviceMaxChannels
    MicInPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicInPinDeviceFormatsAndModes),
    MicInTopologyPhysicalConnections,
    SIZEOF_ARRAY(MicInTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS
};

//=============================================================================
//
// Capture miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_CaptureEndpoints[] = 
{
    &MicInMiniports,
};

#define g_cCaptureEndpoints (SIZEOF_ARRAY(g_CaptureEndpoints))

//=============================================================================
//
// Total miniports = # endpoints * 2 (topology + wave).
//
#define g_MaxMiniports  ((g_cRenderEndpoints + g_cCaptureEndpoints) * 2)



/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Declaration of topology miniport for the mic in jack (external: microphone).

*/

#pragma once

#include "simple.h"

#define MICIN_DEVICE_MAX_CHANNELS              1       // Max Channels.
#define MICIN_MIN_BITS_PER_SAMPLE              24      // Min Bits Per Sample
#define MICIN_MAX_BITS_PER_SAMPLE              24      // Max Bits Per Sample
#define MICIN_MIN_SAMPLE_RATE                  44100   // Min Sample Rate
#define MICIN_MAX_SAMPLE_RATE                  44100   // Max Sample Rate



NTSTATUS 
PropertyHandler_MicInTopoFilter
(
    _In_ PPCPROPERTY_REQUEST PropertyRequest
);

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE MicInPinSupportedDeviceFormats[] =
{
    { // 2
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        {
            {
                WAVE_FORMAT_EXTENSIBLE,
                1,
                MICIN_MAX_SAMPLE_RATE,
                4*MICIN_MAX_SAMPLE_RATE,
                4,
                32,  // specify a 32 bit container for 24 bit audio to make fifo management easier.
                sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
            },
            MICIN_MAX_BITS_PER_SAMPLE, // 24 bit resolution in a 32 bit container.
            KSAUDIO_SPEAKER_MONO,
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
        }
    },
};

//
// Supported modes (only on streaming pins).
//
static
MODE_AND_DEFAULT_FORMAT MicInPinSupportedDeviceModes[] =
{
    {
        STATIC_AUDIO_SIGNALPROCESSINGMODE_RAW,
        &MicInPinSupportedDeviceFormats[0].DataFormat // 44.1kHz
    },
};

//
// The entries here must follow the same order as the filter's pin
// descriptor array.
//
static 
PIN_DEVICE_FORMATS_AND_MODES MicInPinDeviceFormatsAndModes[] = 
{
    {
        BridgePin,
        NULL,
        0,
        NULL,
        0
    },
    {
        SystemCapturePin,
        MicInPinSupportedDeviceFormats,
        SIZEOF_ARRAY(MicInPinSupportedDeviceFormats),
        MicInPinSupportedDeviceModes,
        SIZEOF_ARRAY(MicInPinSupportedDeviceModes)
    },
};



//=============================================================================
static
KSDATARANGE MicInPinDataRangesBridge[] =
{
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }
};

static
PKSDATARANGE MicInPinDataRangePointersBridge[] =
{
    &MicInPinDataRangesBridge[0]
};

//=============================================================================
static
KSDATARANGE_AUDIO MicInPinDataRangesStream[] =
{
    { // 0
        {
            sizeof(KSDATARANGE_AUDIO),
            KSDATARANGE_ATTRIBUTES,         // An attributes list follows this data range
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        MICIN_DEVICE_MAX_CHANNELS,           
        MICIN_MIN_BITS_PER_SAMPLE,    
        MICIN_MAX_BITS_PER_SAMPLE,    
        MICIN_MIN_SAMPLE_RATE,            
        MICIN_MAX_SAMPLE_RATE             
    },
};

static
PKSDATARANGE MicInPinDataRangePointersStream[] =
{
    PKSDATARANGE(&MicInPinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangeAttributeList)
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicInWaveMiniportPins[] =
{
    // Wave In Bridge Pin (Capture - From Topology) KSPIN_WAVE_BRIDGE
    {
        0,
        0,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(MicInPinDataRangePointersBridge),
            MicInPinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },

    // Wave In Streaming Pin (Capture) KSPIN_WAVEIN_HOST
    {
        MAX_INPUT_SYSTEM_STREAMS,
        MAX_INPUT_SYSTEM_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(MicInPinDataRangePointersStream),
            MicInPinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            &KSAUDFNAME_RECORDING_CONTROL,
            0
        }
    }
};

//=============================================================================
static
PCNODE_DESCRIPTOR MicInWaveMiniportNodes[] =
{
    // KSNODE_WAVE_ADC
    {
        0,                          // Flags
        NULL,                       // AutomationTable
        &KSNODETYPE_ADC,            // Type
        NULL                        // Name
    }
};

static
PCCONNECTION_DESCRIPTOR MicInWaveMiniportConnections[] =
{
    { PCFILTER_NODE,        KSPIN_WAVE_BRIDGE,      KSNODE_WAVE_ADC,     1 },    
    { KSNODE_WAVE_ADC,      0,                      PCFILTER_NODE,       KSPIN_WAVEIN_HOST },
};

//=============================================================================
static
PCPROPERTY_ITEM PropertiesMicInWaveFilter[] =
{
    {
        &KSPROPSETID_General,
        KSPROPERTY_GENERAL_COMPONENTID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    },
    {
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_PROPOSEDATAFORMAT,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_WaveFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicInWaveFilter, PropertiesMicInWaveFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicInWaveMiniportFilterDescriptor =
{
    0,                                              // Version
    &AutomationMicInWaveFilter,                     // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                       // PinSize
    SIZEOF_ARRAY(MicInWaveMiniportPins),            // PinCount
    MicInWaveMiniportPins,                          // Pins
    sizeof(PCNODE_DESCRIPTOR),                      // NodeSize
    SIZEOF_ARRAY(MicInWaveMiniportNodes),           // NodeCount
    MicInWaveMiniportNodes,                         // Nodes
    SIZEOF_ARRAY(MicInWaveMiniportConnections),     // ConnectionCount
    MicInWaveMiniportConnections,                   // Connections
    0,                                              // CategoryCount
    NULL                                            // Categories  - use defaults (audio, render, capture)
};


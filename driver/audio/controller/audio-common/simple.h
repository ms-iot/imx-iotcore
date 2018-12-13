/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Node and Pin numbers and other common definitions for configuration.

*/

#pragma once

//----------------------------------------------------
// New defines for the render endpoints.
//----------------------------------------------------

// Default pin instances.
#define MAX_INPUT_SYSTEM_STREAMS        1

// Wave Topology nodes
enum
{
    KSNODE_WAVE_AUDIO_ENGINE = 0
};

// Wave pins
enum
{
    KSPIN_WAVE_RENDER_SINK_SYSTEM = 0,
    KSPIN_WAVE_RENDER_SOURCE,
};

// Topology pins.
enum
{
    KSPIN_TOPO_WAVEOUT_SOURCE = 0,
    KSPIN_TOPO_LINEOUT_DEST,
};

//----------------------------------------------------
// New defines for the mic in
//----------------------------------------------------

// Wave pins
enum 
{
    KSPIN_WAVE_BRIDGE = 0,
    KSPIN_WAVEIN_HOST
};

// Wave Topology nodes.
enum 
{
    KSNODE_WAVE_ADC = 0
};

// topology pins.
enum
{
    KSPIN_TOPO_MIC_ELEMENTS,
    KSPIN_TOPO_BRIDGE
};

// topology nodes.
enum
{
    KSNODE_TOPO_VOLUME,
    KSNODE_TOPO_MUTE
};

// data format attribute range definitions.
static
KSATTRIBUTE PinDataRangeSignalProcessingModeAttribute =
{
    sizeof(KSATTRIBUTE),
    0,
    STATICGUIDOF(KSATTRIBUTEID_AUDIOSIGNALPROCESSING_MODE),
};

static
PKSATTRIBUTE PinDataRangeAttributes[] =
{
    &PinDataRangeSignalProcessingModeAttribute,
};

static
KSATTRIBUTE_LIST PinDataRangeAttributeList =
{
    ARRAYSIZE(PinDataRangeAttributes),
    PinDataRangeAttributes,
};



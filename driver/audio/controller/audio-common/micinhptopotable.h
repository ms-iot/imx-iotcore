/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Declaration of topology tables for the speaker (external: headphone).

*/

#pragma once

#include "micinwavtable.h"
#include "hptoptable.h"

static
KSJACK_DESCRIPTION JackDescMics =
{
    KSAUDIO_SPEAKER_STEREO,
    0xB3C98C,               // Color spec for green
    eConnType3Point5mm,
    eGeoLocRear,
    eGenLocPrimaryBox,
    ePortConnJack,
    TRUE
};

//=============================================================================
static
KSDATARANGE MicInTopoPinDataRangesBridge[] =
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

//=============================================================================
static
PKSDATARANGE MicInTopoPinDataRangePointersBridge[] =
{
  &MicInTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicInTopoMiniportPins[] =
{
  // KSPIN_TOPO_MIC_ELEMENTS
  {
    0,
    0,
    0,                                                  // InstanceCount
    NULL,                                               // AutomationTable
    {                                                   // KsPinDescriptor
      0,                                                // InterfacesCount
      NULL,                                             // Interfaces
      0,                                                // MediumsCount
      NULL,                                             // Mediums
      SIZEOF_ARRAY(MicInTopoPinDataRangePointersBridge),// DataRangesCount
      MicInTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_MICROPHONE,                           // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  },
  // KSPIN_TOPO_BRIDGE
  {
    0,
    0,
    0,                                                  // InstanceCount
    NULL,                                               // AutomationTable
    {                                                   // KsPinDescriptor
      0,                                                // InterfacesCount
      NULL,                                             // Interfaces
      0,                                                // MediumsCount
      NULL,                                             // Mediums
      SIZEOF_ARRAY(MicInTopoPinDataRangePointersBridge),// DataRangesCount
      MicInTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSCATEGORY_AUDIO,                                // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

//=============================================================================
// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION MicInJackDescriptions[] =
{
    &JackDescMics,
    NULL,
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR MicInTopoMiniportConnections[] =
{
  //  FromNode,                     FromPin,                        ToNode,                      ToPin
  {   PCFILTER_NODE,                KSPIN_TOPO_MIC_ELEMENTS,            PCFILTER_NODE,               KSPIN_TOPO_BRIDGE, }
};


//=============================================================================
static
PCPROPERTY_ITEM PropertiesMicInTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicInTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicInTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicInTopoFilter, PropertiesMicInTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicInTopoMiniportFilterDescriptor =
{
    0,                                            // Version
    &AutomationMicInTopoFilter,                   // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                     // PinSize
    SIZEOF_ARRAY(MicInTopoMiniportPins),          // PinCount
    MicInTopoMiniportPins,                        // Pins
    sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
    0,                                            // NodeCount
    NULL,                                         // Nodes
    SIZEOF_ARRAY(MicInTopoMiniportConnections),   // ConnectionCount
    MicInTopoMiniportConnections,                 // Connections
    0,                                            // CategoryCount
    NULL                                          // Categories
};

    


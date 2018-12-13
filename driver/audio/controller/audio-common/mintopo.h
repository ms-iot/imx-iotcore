/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Declaration of topology miniport.

*/

#pragma once

#include "basetopo.h"

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CMiniportTopology 
//   

class CMiniportTopology : 
    public CMiniportTopologyIMXWAV,
    public IMiniportTopology,
    public CUnknown
{
  private:
    eDeviceType             m_DeviceType;
    union {
        PVOID               m_DeviceContext;
    };

public:
    DECLARE_STD_UNKNOWN();
    CMiniportTopology
    (
        _In_opt_    PUNKNOWN                UnknownOuter,
        _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
        _In_        USHORT                  DeviceMaxChannels,
        _In_        eDeviceType             DeviceType, 
        _In_opt_    PVOID                   DeviceContext
    )
    : CUnknown(UnknownOuter),
      CMiniportTopologyIMXWAV(FilterDesc, DeviceMaxChannels),
      m_DeviceType(DeviceType),
      m_DeviceContext(DeviceContext)
    {
    }

    ~CMiniportTopology();

    IMP_IMiniportTopology;

    NTSTATUS PropertyHandlerJackDescription
    (
        _In_        PPCPROPERTY_REQUEST                      PropertyRequest,
        _In_        ULONG                                    NumJackDescriptions,
        _In_reads_(NumJackDescriptions) PKSJACK_DESCRIPTION *JackDescriptions
    );

    NTSTATUS PropertyHandlerJackDescription2
    ( 
        _In_        PPCPROPERTY_REQUEST                      PropertyRequest,
        _In_        ULONG                                    NumJackDescriptions,
        _In_reads_(NumJackDescriptions) PKSJACK_DESCRIPTION *  JackDescriptions,
        _In_        DWORD                                    JackCapabilities
    );
    
};

typedef CMiniportTopology *PCMiniportTopology;



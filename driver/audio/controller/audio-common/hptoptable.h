/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Common declaration of topology table for the headphone.

*/

#pragma once

//=============================================================================
static
KSJACK_DESCRIPTION HpJackDesc =
{
    KSAUDIO_SPEAKER_STEREO,
    JACKDESC_RGB(179, 201, 140),
    eConnType3Point5mm,
    eGeoLocTop,
    eGenLocPrimaryBox,
    ePortConnJack,
    TRUE
};


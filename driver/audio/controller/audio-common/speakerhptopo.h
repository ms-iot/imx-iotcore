/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Declaration of topology miniport for the speaker (external: headphone).

*/

#pragma once

NTSTATUS 
PropertyHandler_SpeakerHpTopoFilter
(
    _In_ PPCPROPERTY_REQUEST PropertyRequest
);


// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "precomp.hpp"
#pragma hdrstop

#include "trace.hpp"
#include "trace.tmh"

BOOLEAN _BreakOnError = FALSE;
BOOLEAN _BreakOnDdiEnter = FALSE;
BOOLEAN _BreakOnDdiExit = FALSE;

#if DBG
ULONG _SdhcCrashdumpDebugLevel = DPFLTR_TRACE_LEVEL;
#else 
ULONG _SdhcCrashdumpDebugLevel = DPFLTR_ERROR_LEVEL;
#endif // DBG

VOID
_SdhcDebugPrint(
    ULONG DebugLevel,
    ULONG CurrentDebugLevel,
    PSTR DebugMessage,
    ...
    )
{
    va_list ap;
    va_start(ap, DebugMessage);
    if (DebugLevel <= CurrentDebugLevel) {
        vDbgPrintExWithPrefix(USDHC_DBG_PRINT_PREFIX ": ",
                              USDHC_COMPONENT_ID,
                              DebugLevel,
                              DebugMessage,
                              ap);
    }
    va_end(ap);
}


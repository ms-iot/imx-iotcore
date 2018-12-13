// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPItrace.cpp
//
// Abstract:
//
//    This module contains implementation of various trace/debugging support
//    routines for the IMX ECSPI controller driver.
//
// Environment:
//
//    kernel-mode only
//
#include "precomp.h"
#pragma hdrstop

#define _ECSPI_TRACE_CPP_

// Logging
#include "ECSPItrace.h"
#include "ECSPItrace.tmh"

// Module specific header files


//
// Routine Description:
//
//  ECSPIIsDebuggerPresent is called to check if the debugger is present
//  and enabled.
//
// Arguments:
//
// Return Value:
//  
//  TRUE debugger is present, otherwise FALSE.
//
BOOLEAN
ECSPIIsDebuggerPresent ()
{
    static LONG isDebuggerStateUpToDate = FALSE;

    if (InterlockedExchange(&isDebuggerStateUpToDate, TRUE) == 0) {

        KdRefreshDebuggerNotPresent();
    }

    return (KD_DEBUGGER_ENABLED && !KD_DEBUGGER_NOT_PRESENT);
}


//
// Routine Description:
//
//  ECSPIBreakPoint is called by ECSPI_ASSERT to break if debugger
//  is present and enabled.
//
// Arguments:
//
// Return Value:
//
//  Always TRUE to continue...
//
BOOLEAN
ECSPIBreakPoint ()
{
    if (ECSPIIsDebuggerPresent()) {

        DbgBreakPoint();
    }

    return TRUE;
}


#undef _ECSPI_TRACE_CPP_

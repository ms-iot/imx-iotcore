// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name: 
//
//    ECSPItrace.h
//
// Abstract:
//
//    This module contains the tracing definitions for the 
//    IMX ECSPI controller driver.
//
// Environment:
//
//    kernel-mode only
//

#ifndef _ECSPI_TRACE_H_
#define _ECSPI_TRACE_H_

WDF_EXTERN_C_START


//
// Defining control guids, including this is required to happen before
// including the tmh file (if the WppRecorder API is used)
//
#include <WppRecorder.h>

//
// Debug support
//
extern BOOLEAN ECSPIIsDebuggerPresent ();
extern BOOLEAN ECSPIBreakPoint ();


//
// Trace log buffer size
//
#if DBG
    #define ECSPI_RECORDER_TOTAL_BUFFER_SIZE       (PAGE_SIZE * 4)
    #define ECSPI_RECORDER_ERROR_PARTITION_SIZE    (ECSPI_RECORDER_TOTAL_BUFFER_SIZE / 4)
#else
    #define ECSPI_RECORDER_TOTAL_BUFFER_SIZE       (PAGE_SIZE)
    #define ECSPI_RECORDER_ERROR_PARTITION_SIZE    (ECSPI_RECORDER_TOTAL_BUFFER_SIZE / 4)
#endif 


//
// The trace log identifier for each ECSPI controller (device)
//
#define ECSPI_RECORDER_LOG_ID   "ECSPI%08X"


//
// Tracing GUID - 1146023C-C8DE-4977-863B-5E3ED060C94D
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(IMXECSPI, (1146023C,C8DE,4977,863B,5E3ED060C94D), \
        WPP_DEFINE_BIT(ECSPI_TRACING_DEFAULT) \
        WPP_DEFINE_BIT(ECSPI_TRACING_VERBOSE) \
        WPP_DEFINE_BIT(ECSPI_TRACING_DEBUG) \
    )

// begin_wpp config
//
// FUNC ECSPI_LOG_ASSERTION{LEVEL=TRACE_LEVEL_ERROR, FLAGS=ECSPI_TRACING_DEBUG}(IFRLOG, MSG, ...);
// USEPREFIX (ECSPI_LOG_ASSERTION, "%!STDPREFIX! [%s @ %u] ASSERTION :", __FILE__, __LINE__);
//
// FUNC ECSPI_LOG_ERROR{LEVEL=TRACE_LEVEL_ERROR, FLAGS=ECSPI_TRACING_DEFAULT}(IFRLOG, MSG, ...);
// USEPREFIX (ECSPI_LOG_ERROR, "%!STDPREFIX! [%s @ %u] ERROR :", __FILE__, __LINE__);
//
// FUNC ECSPI_LOG_WARNING{LEVEL=TRACE_LEVEL_WARNING, FLAGS=ECSPI_TRACING_DEFAULT}(IFRLOG, MSG, ...);
// USEPREFIX (ECSPI_LOG_WARNING, "%!STDPREFIX! [%s @ %u] WARNING :", __FILE__, __LINE__);
//
// FUNC ECSPI_LOG_INFORMATION{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=ECSPI_TRACING_DEFAULT}(IFRLOG, MSG, ...);
// USEPREFIX (ECSPI_LOG_INFORMATION, "%!STDPREFIX! [%s @ %u] INFO :", __FILE__, __LINE__);
//
// FUNC ECSPI_LOG_TRACE{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=ECSPI_TRACING_VERBOSE}(IFRLOG, MSG, ...);
// USEPREFIX (ECSPI_LOG_TRACE, "%!STDPREFIX! [%s @ %u] TRACE :", __FILE__, __LINE__);
//
// FUNC ECSPI_ASSERT{LEVEL=TRACE_LEVEL_ERROR, FLAGS=ECSPI_TRACING_DEBUG}(IFRLOG, ECSPI_ASSERT_EXP);
// USEPREFIX (ECSPI_ASSERT, "%!STDPREFIX! [%s @ %u] ASSERTION :%s", __FILE__, __LINE__, #ECSPI_ASSERT_EXP);
//
// end_wpp

//
// ECSPI_ASSERT customization
//
#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_ECSPI_ASSERT_EXP_FILTER(LEVEL, FLAGS, IFRLOG, ECSPI_ASSERT_EXP) \
    (!(ECSPI_ASSERT_EXP))

#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_ECSPI_ASSERT_EXP_ARGS(LEVEL, FLAGS, IFRLOG, ECSPI_ASSERT_EXP) \
    IFRLOG, LEVEL, WPP_BIT_ ## FLAGS  

#define WPP_LEVEL_FLAGS_IFRLOG_ECSPI_ASSERT_EXP_POST(LEVEL, FLAGS, IFRLOG, ECSPIP_ASSERT_EXP) \
    ,((!(ECSPIP_ASSERT_EXP)) ? ECSPIBreakPoint() : 1)


//
// Custom types
//

// begin_wpp config
// CUSTOM_TYPE(REQUESTTYPE, ItemEnum(ECSPI_REQUEST_TYPE));
// end_wpp


WDF_EXTERN_C_END

#endif // !_ECSPI_TRACE_H_
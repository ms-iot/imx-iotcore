// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _TRACE_H_
#define _TRACE_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Defining control guids, including this is required to happen before
// including the tmh file (if the WppRecorder API is used)
//
#include <WppRecorder.h>

//
// Tracing GUID - {E2BDF62D-48DA-4195-B31C-F47D1AB8015C}
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(IMXPWM, (E2BDF62D,48DA,4195,B31C,F47D1AB8015C), \
        WPP_DEFINE_BIT(IMXPWM_TRACING_DEFAULT) \
    )

// begin_wpp config
//
// FUNC IMXPWM_LOG_ERROR{LEVEL=TRACE_LEVEL_ERROR, FLAGS=IMXPWM_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (IMXPWM_LOG_ERROR, "%!STDPREFIX! [%s @ %u] ERROR :", __FILE__, __LINE__);
//
// FUNC IMXPWM_LOG_LOW_MEMORY{LEVEL=TRACE_LEVEL_ERROR, FLAGS=IMXPWM_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (IMXPWM_LOG_LOW_MEMORY, "%!STDPREFIX! [%s @ %u] LOW MEMORY :", __FILE__, __LINE__);
//
// FUNC IMXPWM_LOG_WARNING{LEVEL=TRACE_LEVEL_WARNING, FLAGS=IMXPWM_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (IMXPWM_LOG_WARNING, "%!STDPREFIX! [%s @ %u] WARNING :", __FILE__, __LINE__);
//
// FUNC IMXPWM_LOG_INFORMATION{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=IMXPWM_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (IMXPWM_LOG_INFORMATION, "%!STDPREFIX! [%s @ %u] INFO :", __FILE__, __LINE__);
//
// FUNC IMXPWM_LOG_TRACE{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=IMXPWM_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (IMXPWM_LOG_TRACE, "%!STDPREFIX! [%s @ %u] TRACE :", __FILE__, __LINE__);
//
// end_wpp

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // _TRACE_H_
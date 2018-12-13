// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//     trace.h
//
// Abstract:
//
//     Header file for WPP tracing related function definitions and macros.
//

#include <WppRecorder.h>

//
// Define the tracing flags.
//
// iMX GPIO Controller tracing GUID - 5311c0b2-1af6-49e0-9d3a-12744a7fbaf1
//

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        imxgpioTraceGuid, (5311c0b2,1af6,49e0,9d3a,12744a7fbaf1), \
        WPP_DEFINE_BIT(TRACE_FUNC_INFO)      /* bit  0 = 0x00000001 */ \
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                             \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                            \
    (WPP_LEVEL_ENABLED(flag) &&                                        \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)


//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC LogEnter{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=TRACE_FUNC_INFO}(...);
// USEPREFIX (LogEnter, "%!STDPREFIX!");
// USESUFFIX (LogEnter, "-> %!FUNC!");
// FUNC LogInfo{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=TRACE_FUNC_INFO}(MSG, ...);
// FUNC LogError{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_FUNC_INFO}(MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

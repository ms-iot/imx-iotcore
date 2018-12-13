/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

Trace.h

Abstract:

Header file for the debug tracing related function defintions and macros.

Environment:

Kernel mode

*/

#pragma once

#ifdef WPP_TRACING

#include <WppRecorder.h>

//
// Define the tracing flags
//
// Tracing GUID - 1f04d221, 7a63, 4c1d, 9a395620afbe6467
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(TreeMiniPortGuid, (1F04D221,7A63,4C1D,9A39,5620AFBE6467), \
                            WPP_DEFINE_BIT(DRVR_LVL_ERR)                              \
                            WPP_DEFINE_BIT(DRVR_LVL_WARN)                             \
                            WPP_DEFINE_BIT(DRVR_LVL_INFO)                             \
                            WPP_DEFINE_BIT(DRVR_LVL_DBG)                              \
                            WPP_DEFINE_BIT(DRVR_LVL_FUNC)                             \
                            )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
                               WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
                               (WPP_LEVEL_ENABLED(flags) && \
                                WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
                               WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
                                (WPP_LEVEL_ENABLED(flags) && \
                                 WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)


//           
// WPP orders static parameters before dynamic parameters. To support the Trace function
// defined below which sets FLAGS=MYDRIVER_ALL_INFO, a custom macro must be defined to
// reorder the arguments to what the .tpl configuration file expects.
//
#define WPP_RECORDER_FLAGS_LEVEL_ARGS(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags)
#define WPP_RECORDER_FLAGS_LEVEL_FILTER(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
//
// FUNC TraceInformation{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=DRVR_LVL_INFO}(MSG,...);
// USEPREFIX (TraceInformation, "%!STDPREFIX! [%s @ %u] INFORMATION :", __FILE__, __LINE__);
//
// FUNC TraceError{LEVEL=TRACE_LEVEL_ERROR,FLAGS=DRVR_LVL_ERR}(MSG,...);
// USEPREFIX (TraceError, "%!STDPREFIX! [%s @ %u] ERROR :", __FILE__, __LINE__);
//
// FUNC TraceWarning{LEVEL=TRACE_LEVEL_WARNING,FLAGS=DRVR_LVL_WARN}(MSG,...);
// USEPREFIX (TraceWarning, "%!STDPREFIX! [%s @ %u] WARNING :", __FILE__, __LINE__);
//
// FUNC TraceDebug{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=DRVR_LVL_DBG}(MSG,...);
// USEPREFIX (TraceDebug, "%!STDPREFIX! [%s @ %u] DEBUG :", __FILE__, __LINE__);
//
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

#else   // WPP_TRACING

extern ULONG DefaultDebugLevel;
extern ULONG DefaultDebugFlags;

#define DEBUG_FLAG_ALWAYS       0x00000001
#define DEBUG_FLAG_OPTEE_LIB    0x00000002

#define DRVR_LVL_ERR         0x00000001
#define DRVR_LVL_WARN        0x00000002
#define DRVR_LVL_DBG         0x00000004
#define DRVR_LVL_INFO        0x00000008
#define DRVR_LVL_FUNC        0x00000010

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
    (DefaultDebugLevel >= (lvl) &&          \
     DefaultDebugFlags & (flags) )


#define TraceMessage(_level_,_flag_, ...)          \
    if (DefaultDebugLevel >= (_level_) &&          \
        DefaultDebugFlags & (_flag_)) {            \
        OutputMessage("OPTEE: %a:", __FUNCTION__); \
        OutputMessage(__VA_ARGS__);                \
        }

#define TraceError(...) TraceMessage(DRVR_LVL_ERR, DEBUG_FLAG_ALWAYS, __VA_ARGS__)
#define TraceWarning(...) TraceMessage(DRVR_LVL_WARN, DEBUG_FLAG_ALWAYS, __VA_ARGS__)
#define TraceInformation(...) TraceMessage(DRVR_LVL_INFO, DEBUG_FLAG_ALWAYS, __VA_ARGS__)
#define TraceDebug(...) TraceMessage(DRVR_LVL_DBG, DEBUG_FLAG_ALWAYS, __VA_ARGS__)

static __inline VOID
OutputMessage(
    __drv_formatString(printf) __in PCSTR DebugMessage,
    ...
    )
{
    va_list ArgList;
    _crt_va_start(ArgList, DebugMessage);

    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "%s", "OPTEE: ");
    vDbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, DebugMessage, ArgList);

    _crt_va_end(ArgList);
    return;
}

#endif  // !WPP_TRACING

#define STRING_BUFFER_SIZE 255

static __inline VOID
WCSTOMBS(
    __inout CHAR* String,
    __in WCHAR* Wstring
    )
{
    size_t i;

    for (i = 0; i < STRING_BUFFER_SIZE; i++) {

        if (Wstring[i] == L'\0') {
            String[i] = '\0';
            break;
        }

        if (Wstring[i] >= L'A' && Wstring[i] <= L'Z')
            String[i] = (CHAR)('A' + Wstring[i] - L'A');

        else if (Wstring[i] >= L'a' && Wstring[i] <= L'z')
            String[i] = (CHAR)('a' + Wstring[i] - L'a');

        else if (Wstring[i] >= L'0' && Wstring[i] <= L'9')
            String[i] = (CHAR)('0' + Wstring[i] - L'0');

    }
}




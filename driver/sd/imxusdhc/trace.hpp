// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   trace.hpp
//
// Abstract:
//
//   Defines all WPP logging macros, definitions and logging customizations
//

#ifndef __TRACE_HPP__
#define __TRACE_HPP__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Defining control guids, including this is required to happen before
// including the tmh file (if the WppRecorder API is used)
//
#include <WppRecorder.h>

#define WPP_CHECK_FOR_NULL_STRING  // to prevent exceptions due to NULL strings

//
// Have a larger WPP Log Buffer in case of a checked build
//
#if DBG
#define USDHC_RECORDER_TOTAL_BUFFER_SIZE       (PAGE_SIZE * 4)
#define USDHC_RECORDER_ERROR_PARTITION_SIZE    (USDHC_RECORDER_TOTAL_BUFFER_SIZE / 4)
#else
#define USDHC_RECORDER_TOTAL_BUFFER_SIZE       (PAGE_SIZE)
#define USDHC_RECORDER_ERROR_PARTITION_SIZE    (USDHC_RECORDER_TOTAL_BUFFER_SIZE / 4)
#endif 

//
// ComponentId used to filter debug messages sent to kernel debugger
// Use IHVBUS in the registry to enable seeing debug messages
// from kernel debugger. Example:
// reg add "HKLM\SYSTEM\ControlSet001\Control\Session Manager\Debug Print Filter" /v "IHVBUS" /t REG_DWORD /d 0xF /f
//
#define USDHC_COMPONENT_ID          DPFLTR_IHVBUS_ID

//
// An identifier string to prefix debug print outputs
//
#define USDHC_DBG_PRINT_PREFIX      "IMXUSDHC"

//
// The identifier for each uSDHC controller log where %p is the controller
// physical address as supplied by Sdport
//
#define USDHC_RECORDER_LOG_ID       "USDHC#%p"

//
// Helpers used by tracing macros.
// NOTE: These are not intended to be called from anywhere else
//
extern RECORDER_LOG _SdhcLogTraceRecorder;
extern VOID _SdhcDebugPrint(
    ULONG DebugLevel,
    ULONG CurrentDebugLevel,
    _In_ PSTR DebugMessage,
    ...
    );

extern BOOLEAN _BreakOnError;
extern BOOLEAN _BreakOnDdiEnter;
extern BOOLEAN _BreakOnDdiExit;
extern ULONG _SdhcCrashdumpDebugLevel;

//
// Tracing GUID - D4CC5013-F4BB-4876-B958-50262866D41A
// Define 2 logging flags, default flag and another one to
// control the logging of DDI entry and exit
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(IMXUSDHC, (D4CC5013,F4BB,4876,B958,50262866D41A), \
        WPP_DEFINE_BIT(DEFAULT)     /* 0x00000001 */ \
        WPP_DEFINE_BIT(ENTEREXIT)   /* 0x00000002 */ \
    )

//
// Use for when the PrivateExtensionPtr is not available, i.e
// in DDIs that do not pass it as an argument like SDPORT_GET_SLOT_COUNT
//
#define NullPrivateExtensionPtr     nullptr
#define NullIfrLogHandle            NULL

//
// Utility macro to extract SDHC physical address from private extension pointer
//
#define PRIVEXTPTR_PA(PRIVEXTPTR)   ((PRIVEXTPTR) ? static_cast<USDHC_EXTENSION*>(PRIVEXTPTR)->PhysicalAddress : nullptr)

//
// SDPORT enums and types to use in logging MACROS
// Example: Using %!BUSSPEED! with value of 2 will get translated to
// SdBusSpeedHigh string by WPP trace preprocessor
//
// begin_wpp config 
// CUSTOM_TYPE(BUSSPEED, ItemEnum(_SDPORT_BUS_SPEED));
// CUSTOM_TYPE(RESETTYPE, ItemEnum(_SDPORT_RESET_TYPE));
// CUSTOM_TYPE(BUSOPERATIONTYPE, ItemEnum(_SDPORT_BUS_OPERATION_TYPE));
// CUSTOM_TYPE(REQUESTTYPE, ItemEnum(_SDPORT_REQUEST_TYPE));
// CUSTOM_TYPE(BUSVOLTAGE, ItemEnum(_SDPORT_BUS_VOLTAGE));
// end_wpp
//

//
// MACRO: USDHC_LOG_INFORMATION
//
// begin_wpp config
// USEPREFIX (USDHC_LOG_INFORMATION, "%!STDPREFIX!INFO [0x%p] %!FUNC!:", PRIVEXTPTR_PA(LOG_INFORMATION_PRIVEXTPTR));
// FUNC USDHC_LOG_INFORMATION{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=DEFAULT}(IFRLOG, LOG_INFORMATION_PRIVEXTPTR, MSG, ...);
// end_wpp
//
#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_INFORMATION_PRIVEXTPTR_FILTER(LEVEL, FLAGS, IFRLOG, IFRLOG_INFORMATION_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_FILTER(IFRLOG, LEVEL, FLAGS)

#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_INFORMATION_PRIVEXTPTR_ARGS(LEVEL, FLAGS, IFRLOG, LOG_INFORMATION_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_ARGS(IFRLOG, LEVEL, FLAGS)

//
// MACRO: USDHC_LOG_TRACE
//
// begin_wpp config
// USEPREFIX (USDHC_LOG_TRACE, "%!STDPREFIX!TRACE[0x%p] %!FUNC!:", PRIVEXTPTR_PA(LOG_TRACE_PRIVEXTPTR));
// FUNC USDHC_LOG_TRACE{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=DEFAULT}(IFRLOG, LOG_TRACE_PRIVEXTPTR, MSG, ...);
// end_wpp
//
#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_TRACE_PRIVEXTPTR_FILTER(LEVEL, FLAGS, IFRLOG, LOG_TRACE_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_FILTER(IFRLOG, LEVEL, FLAGS)

#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_TRACE_PRIVEXTPTR_ARGS(LEVEL, FLAGS, IFRLOG, LOG_TRACE_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_ARGS(IFRLOG, LEVEL, FLAGS)

//
// MACRO: USDHC_LOG_ERROR_STATUS
//
// begin_wpp config
// USEPREFIX (USDHC_LOG_ERROR_STATUS, "%!STDPREFIX!ERROR[0x%p] %!FUNC!:", PRIVEXTPTR_PA(LOG_ERROR_STATUS_PRIVEXTPTR));
// FUNC USDHC_LOG_ERROR_STATUS{LEVEL=TRACE_LEVEL_ERROR, FLAGS=DEFAULT}(IFRLOG, LOG_ERROR_STATUS_PRIVEXTPTR, STATUS, MSG, ...);
// USESUFFIX (USDHC_LOG_ERROR_STATUS, ". Status=%!STATUS!", STATUS);
// end_wpp
//
#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_ERROR_STATUS_PRIVEXTPTR_STATUS_ARGS(LEVEL, FLAGS, IFRLOG, LOG_ERROR_STATUS_PRIVEXTPTR, STATUS) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_ARGS(IFRLOG, LEVEL, FLAGS)

#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_ERROR_STATUS_PRIVEXTPTR_STATUS_FILTER(LEVEL, FLAGS, IFRLOG, LOG_ERROR_STATUS_PRIVEXTPTR, STATUS) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_FILTER(IFRLOG, LEVEL, FLAGS)

#define WPP_LEVEL_FLAGS_IFRLOG_LOG_ERROR_STATUS_PRIVEXTPTR_STATUS_MSG_PRE(LEVEL, FLAGS, IFRLOG, LOG_ERROR_STATUS_PRIVEXTPTR, STATUS, MSG) \
    { const USDHC_EXTENSION* _sdhcExtPtr = static_cast<USDHC_EXTENSION*>(LOG_ERROR_STATUS_PRIVEXTPTR);

#define WPP_LEVEL_FLAGS_IFRLOG_LOG_ERROR_STATUS_PRIVEXTPTR_STATUS_MSG_POST(LEVEL, FLAGS, IFRLOG, LOG_ERROR_STATUS_PRIVEXTPTR, STATUS, MSG) ; \
    if (_sdhcExtPtr && _sdhcExtPtr->CrashdumpMode) { \
        _SdhcDebugPrint(DPFLTR_ERROR_LEVEL, _SdhcCrashdumpDebugLevel, "ERROR[0x%p] " __FUNCTION__ ": %s. Status=%0x08x\n", PRIVEXTPTR_PA(LOG_ERROR_STATUS_PRIVEXTPTR), MSG, STATUS); } \
    (((_sdhcExtPtr && _sdhcExtPtr->BreakOnError) || (!_sdhcExtPtr && _BreakOnError)) ? DbgBreakPoint() : 1); }

//
// MACRO: USDHC_LOG_ERROR
//
// begin_wpp config
// USEPREFIX (USDHC_LOG_ERROR, "%!STDPREFIX!ERROR[0x%p] %!FUNC!:", PRIVEXTPTR_PA(LOG_ERROR_PRIVEXTPTR));
// FUNC USDHC_LOG_ERROR{LEVEL=TRACE_LEVEL_ERROR, FLAGS=DEFAULT}(IFRLOG, LOG_ERROR_PRIVEXTPTR, MSG, ...);
// end_wpp
//
#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_ERROR_PRIVEXTPTR_ARGS(LEVEL, FLAGS, IFRLOG, LOG_ERROR_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_ARGS(IFRLOG, LEVEL, FLAGS)

#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_LOG_ERROR_PRIVEXTPTR_FILTER(LEVEL, FLAGS, IFRLOG, LOG_ERROR_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_FILTER(IFRLOG, LEVEL, FLAGS)

#define WPP_LEVEL_FLAGS_LOG_IFRLOG_ERROR_PRIVEXTPTR_MSG_PRE(LEVEL, FLAGS, IFRLOG, LOG_ERROR_PRIVEXTPTR, MSG) \
    { const USDHC_EXTENSION* _sdhcExtPtr = static_cast<USDHC_EXTENSION*>(LOG_ERROR_PRIVEXTPTR);

#define WPP_LEVEL_FLAGS_LOG_IFRLOG_ERROR_PRIVEXTPTR_MSG_POST(LEVEL, FLAGS, IFRLOG, LOG_ERROR_PRIVEXTPTR, MSG) ; \
    if (_sdhcExtPtr && _sdhcExtPtr->CrashdumpMode) { \
            _SdhcDebugPrint(DPFLTR_ERROR_LEVEL, _SdhcCrashdumpDebugLevel, "ERROR[0x%p] " __FUNCTION__ ": %s\n", PRIVEXTPTR_PA(LOG_ERROR_PRIVEXTPTR), MSG); } \
    (((_sdhcExtPtr && _sdhcExtPtr->BreakOnError) || (!_sdhcExtPtr && _BreakOnError)) ? DbgBreakPoint() : 1); }

//
// MACRO: USDHC_DDI_ENTER
//
// begin_wpp config
// USEPREFIX (USDHC_DDI_ENTER, "%!STDPREFIX!ENTER[0x%p] ==> %!FUNC!:", PRIVEXTPTR_PA(DDI_ENTER_PRIVEXTPTR));
// FUNC USDHC_DDI_ENTER{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=ENTEREXIT}(IFRLOG, DDI_ENTER_PRIVEXTPTR, MSG, ...);
// end_wpp
//
#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_DDI_ENTER_PRIVEXTPTR_ARGS(LEVEL, FLAGS, IFRLOG, DDI_ENTER_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_ARGS(IFRLOG, LEVEL, FLAGS)

#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_DDI_ENTER_PRIVEXTPTR_FILTER(LEVEL, FLAGS, IFRLOG, DDI_ENTER_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_FILTER(IFRLOG, LEVEL, FLAGS)

#define WPP_LEVEL_FLAGS_IFRLOG_DDI_ENTER_PRIVEXTPTR_PRE(LEVEL, FLAGS, IFRLOG, DDI_ENTER_PRIVEXTPTR) \
    { const USDHC_EXTENSION* _sdhcExtPtr = static_cast<USDHC_EXTENSION*>(DDI_ENTER_PRIVEXTPTR);

#define WPP_LEVEL_FLAGS_IFRLOG_DDI_ENTER_PRIVEXTPTR_POST(LEVEL, FLAGS, IFRLOG, DDI_ENTER_PRIVEXTPTR) ; \
    if (_sdhcExtPtr && _sdhcExtPtr->CrashdumpMode) { \
        _SdhcDebugPrint(DPFLTR_TRACE_LEVEL, _SdhcCrashdumpDebugLevel, "ENTER[0x%p] ==> " __FUNCTION__ "\n", PRIVEXTPTR_PA(DDI_ENTER_PRIVEXTPTR)); } \
    (((_sdhcExtPtr && _sdhcExtPtr->BreakOnDdiEnter) || (!_sdhcExtPtr && _BreakOnDdiEnter)) ? DbgBreakPoint() : 1); }

//
// MACRO: USDHC_DDI_EXIT
//
// begin_wpp config
// USEPREFIX (USDHC_DDI_EXIT, "%!STDPREFIX!EXIT[0x%p] <== %!FUNC!:", PRIVEXTPTR_PA(DDI_EXIT_PRIVEXTPTR));
// FUNC USDHC_DDI_EXIT{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=ENTEREXIT}(IFRLOG, DDI_EXIT_PRIVEXTPTR, MSG, ...);
// end_wpp
//
#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_DDI_EXIT_PRIVEXTPTR_ARGS(LEVEL, FLAGS, IFRLOG, DDI_EXIT_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_ARGS(IFRLOG, LEVEL, FLAGS)

#define WPP_RECORDER_LEVEL_FLAGS_IFRLOG_DDI_EXIT_PRIVEXTPTR_FILTER(LEVEL, FLAGS, IFRLOG, DDI_EXIT_PRIVEXTPTR) \
    WPP_RECORDER_IFRLOG_LEVEL_FLAGS_FILTER(IFRLOG, LEVEL, FLAGS)

#define WPP_LEVEL_FLAGS_IFRLOG_DDI_EXIT_PRIVEXTPTR_PRE(LEVEL, FLAGS, IFRLOG, DDI_EXIT_PRIVEXTPTR) \
    { const USDHC_EXTENSION* _sdhcExtPtr = static_cast<USDHC_EXTENSION*>(DDI_EXIT_PRIVEXTPTR);

#define WPP_LEVEL_FLAGS_IFRLOG_DDI_EXIT_PRIVEXTPTR_POST(LEVEL, FLAGS, IFRLOG, DDI_EXIT_PRIVEXTPTR) ; \
    if (_sdhcExtPtr && _sdhcExtPtr->CrashdumpMode) { \
        _SdhcDebugPrint(DPFLTR_TRACE_LEVEL, _SdhcCrashdumpDebugLevel, "EXIT[0x%p] <== " __FUNCTION__ "\n", PRIVEXTPTR_PA(DDI_EXIT_PRIVEXTPTR)); } \
    (((_sdhcExtPtr && _sdhcExtPtr->BreakOnDdiExit) || (!_sdhcExtPtr && _BreakOnDdiExit)) ? DbgBreakPoint() : 1); }

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __TRACE_HPP__


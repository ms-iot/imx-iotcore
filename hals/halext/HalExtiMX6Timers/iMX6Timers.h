/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    iMX6Timers.h

Abstract:

    This file implements i.MX6 board specific register definitions.

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _IMX6TIMERS_H
#define _IMX6TIMERS_H

#pragma pack(push, iMX6Timers, 1)

//
// GPT area
// --------
//

//
// GPT timer register offsets
//
typedef enum _GPT_REGISTERS {

    GptControlReg      =  0x00, // GPT_CR
    GptPreScalerReg    =  0x04, // GPT_PR
    GptStatusReg       =  0x08, // GPT_SR
    GptInterruptReg    =  0x0C, // GPT_IR
    GptCounterReg      =  0x24, // GPT_CNT

    MaxGptReg

} GPT_REGISTERS, *PGPT_REGISTERS;


//
// GPT control register
//
typedef union _GPT_CR {

    struct {
        unsigned  EN         : 1; // #0
        unsigned  ENMOD      : 1; // #1
        unsigned  DBGEN      : 1; // #2
        unsigned  WAITEN     : 1; // #3
        unsigned  DOZEEN     : 1; // #4
        unsigned  STOPEN     : 1; // #5
        unsigned  CLKSRC     : 3; // #6-8
        unsigned  FRR        : 1; // #9
        unsigned  EN_24M     : 1; // #10 - IMX SOLO/DualLite only
        unsigned  Reserved   : 4; // #11-14
        unsigned  SWR        : 1; // #15
        unsigned  IM1        : 2; // #16-17
        unsigned  IM2        : 2; // #18-19
        unsigned  OM1        : 3; // #20-22
        unsigned  OM2        : 3; // #23-25
        unsigned  OM3        : 3; // #26-28
        unsigned  FO1        : 1; // #29
        unsigned  FO2        : 1; // #20
        unsigned  FO3        : 1; // #31
    } Bits;

    UINT32   Dword;

} GPT_CR, *PGPT_CR;

//
// GPT pre-scaler register
//
#define GPT_PRESCALER_MAX (1<<12)
//
typedef union _GPT_PR {

    struct {
        unsigned  PRESCALER  : 12; // #0-11
        unsigned  Reserved   : 20; // #12-31
    } Bits;

    UINT32   Dword;

} GPT_PR, *PGPT_PR;

//
// GPT interrupt status register
//
typedef union _GPT_SR {

    struct {
        unsigned  OF1       : 1;  // #0
        unsigned  OF2       : 1;  // #1
        unsigned  OF3       : 1;  // #2
        unsigned  IF1       : 1;  // #3
        unsigned  IF2       : 1;  // #4
        unsigned  ROV       : 1;  // #5
        unsigned  Reserved  : 26; // #6-31
    } Bits;

    struct {
        unsigned  AllInterrupts : 6;  // #0-#5
        unsigned  Reserved      : 26; // #6-31
    } Bits2;

    UINT32   Dword;

} GPT_SR, *PGPT_SR;

//
// GPT interrupt control register
//
typedef union _GPT_IR {

    struct {
        unsigned  OF1E       : 1;  // #0
        unsigned  OF2E       : 1;  // #1
        unsigned  OF3E       : 1;  // #2
        unsigned  IF1E       : 1;  // #3
        unsigned  IF2E       : 1;  // #4
        unsigned  ROVE       : 1;  // #5
        unsigned  Reserved   : 26; // #6-31
    } Bits;

    struct {
        unsigned  AllInterrupts : 6;  // #0-#5
        unsigned  Reserved      : 26; // #6-31
    } Bits2;

    UINT32   Dword;

} GPT_IR, *PGPT_IR;


//
// EPITx area
// ----------
//

//
// EPIT timer register offsets
//
typedef enum _EPIT_REGISTERS {

    EpitControlReg  = 0x00, // EPIT_CR
    EpitStatusReg   = 0x04, // EPIT_SR
    EpitLoadReg     = 0x08, // EPIT_LR
    EpitCompareReg  = 0x0C, // EPIT_CMPR
    EpitCounterReg  = 0x10, // EPIT_CNR

    MaxEpitReg

} EPIT_REGISTERS, *PEPIT_REGISTERS;


//
// EPIT timer output mode codes
//
typedef enum _EPIT_OUTPUT_MODE {

    EpitOutputModeDisconnect    = 0,
    EpitOutputModeToggle        = 1,
    EpitOutputModeClear         = 2,
    EpitOutputModeSet           = 3,

    MaxEpitOutputMode

} EPIT_OUTPUT_MODE, *PEPIT_OUTPUT_MODE;


//
// EPIT control register
//
typedef union _EPIT_CR {

    struct {
        unsigned  EN         : 1; // #0
        unsigned  ENMOD      : 1; // #1
        unsigned  OCIEN      : 1; // #2
        unsigned  RLD        : 1; // #3
        unsigned  PRESCALAR  : 12;// #4-15
        unsigned  SWR        : 1; // #16
        unsigned  IOVW       : 1; // #17
        unsigned  DBGEN      : 1; // #18
        unsigned  WAITEN     : 1; // #19
        unsigned  Reserved1  : 1; // #20
        unsigned  STOPEN     : 1; // #21
        unsigned  OM         : 2; // #22-23
        unsigned  CLKSRC     : 2; // #24-25
        unsigned  Reserved2  : 6; // #26-31
    } Bits;

    UINT32   Dword;

} EPIT_CR, *PEPIT_CR;


//
// EPIT interrupt status register
//
typedef union _EPIT_SR {

    struct {
        unsigned  OCIF      : 1;  // #0
        unsigned  Reserved  : 31; // #1-31
    } Bits;

    UINT32   Dword;

} EPIT_SR, *PEPIT_SR;

//
// SNVS LP RTC Registers
//

typedef enum _SNVS_REGISTERS {

    SnvsLpcr            = 0x04 + 0x34,
    SnvsLpsr            = 0x18 + 0x34,
    SnvsLpSrtcMr        = 0x1c + 0x34,
    SnvsLpSrtcLr        = 0x20 + 0x34,
    SnvsLpTar           = 0x24 + 0x34,
    SnvsLpPgdr          = 0x30 + 0x34,
    
    MaxSnvsReg

} SNVS_REGISTERS, *PSNVS_REGISTERS;

#define SNVS_LPCR_SRTC_ENV	(1 << 0)
#define SNVS_LPCR_LPTA_EN	(1 << 1)
#define SNVS_LPCR_LPWUI_EN	(1 << 3)
#define SNVS_LPSR_LPTA		(1 << 0)

#define SNVS_LPPGDR_INIT	0x41736166

#pragma pack (pop, iMX6Timers)

#endif // !_IMX6TIMERS_H

/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    iMX7Timers.h

Abstract:

    This file implements i.MX7 board specific register definitions.

Environment:

    Kernel mode only.

Revision History:

*/

#ifndef _IMX7TIMERS_H
#define _IMX7TIMERS_H

#pragma pack(push, 1)

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
    GptOutCompareReg1  =  0x10, // GPT_OCR1
    GptCounterReg      =  0x24, // GPT_CNT

    MaxGptReg

} GPT_REGISTERS, *PGPT_REGISTERS;


//
// GPT control register
//
typedef union _GPT_CR {

    UINT32   Dword;

    struct {
        unsigned  EN : 1; // #0
        unsigned  ENMOD : 1; // #1
        unsigned  DBGEN : 1; // #2
        unsigned  WAITEN : 1; // #3
        unsigned  DOZEEN : 1; // #4
        unsigned  STOPEN : 1; // #5
        unsigned  CLKSRC : 3; // #6-8
        unsigned  FRR : 1; // #9
        unsigned  EN_24M : 1; // #10
        unsigned  Reserved : 4; // #11-14
        unsigned  SWR : 1; // #15
        unsigned  IM1 : 2; // #16-17
        unsigned  IM2 : 2; // #18-19
        unsigned  OM1 : 3; // #20-22
        unsigned  OM2 : 3; // #23-25
        unsigned  OM3 : 3; // #26-28
        unsigned  FO1 : 1; // #29
        unsigned  FO2 : 1; // #20
        unsigned  FO3 : 1; // #31
    } Bits;

} GPT_CR, *PGPT_CR;

//
// GPT pre-scaler register
//
#define GPT_PRESCALER_MAX (1<<12)
//
typedef union _GPT_PR {

    UINT32   Dword;

    struct {
        unsigned  PRESCALER  : 12; // #0-11
        unsigned PRESCALER24M : 4; // #12-15
        unsigned Reserved : 16; // #16-32
    } Bits;

} GPT_PR, *PGPT_PR;

//
// GPT interrupt status register
//
typedef union _GPT_SR {

    UINT32   Dword;

    struct {
        unsigned  OF1 : 1;  // #0
        unsigned  OF2 : 1;  // #1
        unsigned  OF3 : 1;  // #2
        unsigned  IF1 : 1;  // #3
        unsigned  IF2 : 1;  // #4
        unsigned  ROV : 1;  // #5
        unsigned  Reserved  : 26; // #6-31
    } Bits;

    struct {
        unsigned  AllInterrupts : 6;  // #0-#5
        unsigned  Reserved      : 26; // #6-31
    } Bits2;

} GPT_SR, *PGPT_SR;

//
// GPT interrupt control register
//
typedef union _GPT_IR {

    UINT32   Dword;

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

} GPT_IR, *PGPT_IR;


#pragma pack (pop,)

#endif // !_IMX7TIMERS_H

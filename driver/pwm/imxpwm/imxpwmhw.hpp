// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   imxpwmhw.hpp
//
// Abstract:
//
//  i.MX PWM Hardware Register Definitions
//
// Environment:
//
//  Kernel mode only
//

#ifndef _IMXPWMHW_HPP_
#define _IMXPWMHW_HPP_

#pragma warning(push)
#pragma warning(disable:4201) // nameless struct/union

#include <pshpack1.h>

//
// PWMx Registers
//
typedef struct {
    ULONG PWMCR;
    ULONG PWMSR;
    ULONG PWMIR;
    ULONG PWMSAR;
    ULONG PWMPR;
    ULONG PWMCNR;
} IMXPWM_REGISTERS;

//
// PWM Control Register (PWMx_PWMCR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG EN        : 1;    // 0
        ULONG REPEAT    : 2;    // 1:2
        ULONG SWR       : 1;    // 3
        ULONG PRESCALER : 12;   // 4:15
        ULONG CLKSRC    : 2;    // 16:17
        ULONG POUTC     : 2;    // 18:19
        ULONG HCTR      : 1;    // 20
        ULONG BCTR      : 1;    // 21
        ULONG DBGEN     : 1;    // 22
        ULONG WAITEN    : 1;    // 23
        ULONG DOZEN     : 1;    // 24
        ULONG STOPEN   : 1;    // 25
        ULONG FWM       : 2;    // 26:27
        ULONG _Reserved : 4;    // 28:31
    };
} IMXPWM_PWMCR_REG;

enum IMXPWM_PWMCR_FWM : ULONG {
    IMXPWM_PWMCR_FWM_1,
    IMXPWM_PWMCR_FWM_2,
    IMXPWM_PWMCR_FWM_3,
    IMXPWM_PWMCR_FWM_4,
};

enum : ULONG {
    IMXPWM_FIFO_SAMPLE_COUNT = 4
};

enum : USHORT {
    IMXPWM_PWMCR_PRESCALER_MAX = 0xFFF,
};

enum IMXPWM_PWMCR_POUTC : ULONG {
    IMXPWM_PWMCR_POUTC_SET_ROV_CLR_CMP,
    IMXPWM_PWMCR_POUTC_CLR_ROV_SET_CMP,
};

enum IMXPWM_PWMCR_CLKSRC : ULONG {
    IMXPWM_PWMCR_CLKSRC_OFF,
    IMXPWM_PWMCR_CLKSRC_IPG_CLK,
    IMXPWM_PWMCR_CLKSRC_IPG_CLK_HIGHFREQ,
    IMXPWM_PWMCR_CLKSRC_IPG_CLK_32K
};

//
// PWM Status Register (PWMx_PWMSR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FIFOAV    : 3;    // 0:2
        ULONG FE        : 1;    // 3
        ULONG ROV       : 1;    // 4
        ULONG CMP       : 1;    // 5
        ULONG FWE       : 1;    // 6
        ULONG _Reserved : 25;   // 7:31
    };
} IMXPWM_PWMSR_REG;

enum {
    IMXPWM_PWMSR_INTERRUPT_MASK = 0x38
};

//
// PWM Interrupt Register (PWMx_PWMIR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG FIE       : 1;    // 0
        ULONG RIE       : 1;    // 1
        ULONG CIE       : 1;    // 2
        ULONG _Reserved : 29;   // 3:31
    };
} IMXPWM_PWMIR_REG;

//
// PWM Sample Register (PWMx_PWMSAR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG SAMPLE    : 16;   // 0:15
        ULONG _Reserved : 16;   // 16:31
    };
} IMXPWM_PWMSAR_REG;

enum : USHORT {
    IMXPWM_PWMSAR_SAMPLE_MAX = 0xFFFF
};

//
// PWM Period Register (PWMx_PWMPR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG PERIOD    : 16;   // 0:15
        ULONG _Reserved : 16;   // 16:31
    };
} IMXPWM_PWMPR_REG;

enum : USHORT {
    IMXPWM_PWMPR_PERIOD_MAX = 0xFFFE
};

//
// PWM Counter Register (PWMx_PWMCNR)
//
typedef union {
    ULONG AsUlong;
    struct {
        ULONG COUNT     : 16;   // 0:15
        ULONG _Reserved : 16;   // 16:31
    };
} IMXPWM_PWMCNR_REG;

#include <poppack.h> // pshpack1.h
#pragma warning(pop)

#endif // _IMXPWMHW_H_
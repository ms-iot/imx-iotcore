// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6clktree.h
//
// Abstract:
//
//   IMX6 Clock Tree Utilities
//

#ifndef _MX6CLKTREEHELPER_H_
#define _MX6CLKTREEHELPER_H_

struct MX6_CLK_INFO {
    ULONG Frequency;
    MX6_CLK Parent;
};

//
// Retrieves information about the clock tree from a CCM register dump
//
class Mx6ClockTreeHelper {
public:

    Mx6ClockTreeHelper (const MX6PEP_DUMP_REGISTERS_OUTPUT& RegisterDump) :
        registers(RegisterDump) {}

    HRESULT GetClockInfo (MX6_CLK ClockId, _Out_ MX6_CLK_INFO* ClockInfoPtr);

private:

    enum MX6_PLL_PFD {
        MX6_PLL_PFD0,
        MX6_PLL_PFD1,
        MX6_PLL_PFD2,
        MX6_PLL_PFD3,
    };

    static MX6_CLK MX6ClkFromBypassClkSource (
        MX6_PLL_BYPASS_CLK_SRC BypassClockSource
        );

    static void GetOsc24ClkInfo (_Out_ MX6_CLK_INFO* ClockInfo);

    HRESULT GetPll1MainClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetPll2MainClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT
    GetPll2PfdClkInfo (
        MX6_PLL_PFD PfdIndex,
        _Out_ MX6_CLK_INFO* ClockInfoPtr
        );

    HRESULT GetPll3MainClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT
    GetPll3PfdClkInfo (
        MX6_PLL_PFD PfdIndex,
        _Out_ MX6_CLK_INFO* ClockInfoPtr
        );

    HRESULT GetPll3SwClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetPll3_80MInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetAxiClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetPeriphClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetPrePeriphClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetPeriphClk2Info (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetArmClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetMmdcCh0ClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetAhbClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetIpgClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetPerclkClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetUsdhcClkRootInfo (ULONG Index, _Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetSsiClkRootInfo (ULONG Index, _Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetGpu2dAxiClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetGpu3dAxiClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetGpu2dCoreClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetGpu3dCoreClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetGpu3dShaderClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetVpuAxiClkInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetUartClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    HRESULT GetVideo27MClkRootInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr);

    const MX6PEP_DUMP_REGISTERS_OUTPUT& registers;
};

#endif // _MX6CLKTREEHELPER_H_

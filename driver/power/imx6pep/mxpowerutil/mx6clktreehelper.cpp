// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6clktreehelper.cpp
//
// Abstract:
//
//   IMX6 Clock Tree Utility
//

#include <windows.h>
#include <winioctl.h>
#include <mx6pephw.h>
#include <mx6pepioctl.h>
#include "util.h"
#include "mx6clktreehelper.h"

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetClockInfo (
    MX6_CLK ClockId,
    MX6_CLK_INFO* ClockInfoPtr
    )
{
    HRESULT hr;
    switch (ClockId) {
    case MX6_OSC_CLK:
        this->GetOsc24ClkInfo(ClockInfoPtr);
        hr = S_OK;
        break;
    case MX6_PLL1_MAIN_CLK:
        hr = this->GetPll1MainClkInfo(ClockInfoPtr);
        break;
    case MX6_PLL2_MAIN_CLK:
        hr = this->GetPll2MainClkInfo(ClockInfoPtr);
        break;
    case MX6_PLL2_PFD0:
        hr = this->GetPll2PfdClkInfo(MX6_PLL_PFD0, ClockInfoPtr);
        break;
    case MX6_PLL2_PFD1:
        hr = this->GetPll2PfdClkInfo(MX6_PLL_PFD1, ClockInfoPtr);
        break;
    case MX6_PLL2_PFD2:
        hr = this->GetPll2PfdClkInfo(MX6_PLL_PFD2, ClockInfoPtr);
        break;
    case MX6_PLL3_MAIN_CLK:
        hr = this->GetPll3MainClkInfo(ClockInfoPtr);
        break;
    case MX6_PLL3_PFD0:
        hr = this->GetPll3PfdClkInfo(MX6_PLL_PFD0, ClockInfoPtr);
        break;
    case MX6_PLL3_PFD1:
        hr = this->GetPll3PfdClkInfo(MX6_PLL_PFD1, ClockInfoPtr);
        break;
    case MX6_PLL3_PFD2:
        hr = this->GetPll3PfdClkInfo(MX6_PLL_PFD2, ClockInfoPtr);
        break;
    case MX6_PLL3_PFD3:
        hr = this->GetPll3PfdClkInfo(MX6_PLL_PFD3, ClockInfoPtr);
        break;
    case MX6_PLL3_SW_CLK:
        hr = this->GetPll3SwClkInfo(ClockInfoPtr);
        break;
    case MX6_PLL3_80M:
        hr = this->GetPll3_80MInfo(ClockInfoPtr);
        break;
    case MX6_AXI_CLK_ROOT:
        hr = this->GetAxiClkRootInfo(ClockInfoPtr);
        break;
    case MX6_PERIPH_CLK2:
        hr = this->GetPeriphClk2Info(ClockInfoPtr);
        break;
    case MX6_PERIPH_CLK:
        hr = this->GetPeriphClkInfo(ClockInfoPtr);
        break;
    case MX6_PRE_PERIPH_CLK:
        hr = this->GetPrePeriphClkInfo(ClockInfoPtr);
        break;
    case MX6_ARM_CLK_ROOT:
        hr = this->GetArmClkRootInfo(ClockInfoPtr);
        break;
    case MX6_MMDC_CH0_CLK_ROOT:
        hr = this->GetMmdcCh0ClkRootInfo(ClockInfoPtr);
        break;
    case MX6_AHB_CLK_ROOT:
        hr = this->GetAhbClkRootInfo(ClockInfoPtr);
        break;
    case MX6_IPG_CLK_ROOT:
        hr = this->GetIpgClkRootInfo(ClockInfoPtr);
        break;
    case MX6_PERCLK_CLK_ROOT:
        hr = this->GetPerclkClkRootInfo(ClockInfoPtr);
        break;
    case MX6_USDHC1_CLK_ROOT:
    case MX6_USDHC2_CLK_ROOT:
    case MX6_USDHC3_CLK_ROOT:
    case MX6_USDHC4_CLK_ROOT:
        hr = this->GetUsdhcClkRootInfo(
                ClockId - MX6_USDHC1_CLK_ROOT,
                ClockInfoPtr);

        break;
    case MX6_SSI1_CLK_ROOT:
    case MX6_SSI2_CLK_ROOT:
    case MX6_SSI3_CLK_ROOT:
        hr = this->GetSsiClkRootInfo(
                ClockId - MX6_SSI1_CLK_ROOT,
                ClockInfoPtr);

       break;
    case MX6_GPU2D_AXI_CLK_ROOT:
        hr = this->GetGpu2dAxiClkRootInfo(ClockInfoPtr);
        break;
    case MX6_GPU3D_AXI_CLK_ROOT:
        hr = this->GetGpu3dAxiClkRootInfo(ClockInfoPtr);
        break;
    case MX6_GPU2D_CORE_CLK_ROOT:
        hr = this->GetGpu2dCoreClkInfo(ClockInfoPtr);
        break;
    case MX6_GPU3D_CORE_CLK_ROOT:
        hr = this->GetGpu3dCoreClkInfo(ClockInfoPtr);
        break;
    case MX6_GPU3D_SHADER_CLK_ROOT:
        hr = this->GetGpu3dShaderClkInfo(ClockInfoPtr);
        break;
    case MX6_VPU_AXI_CLK_ROOT:
        hr = this->GetVpuAxiClkInfo(ClockInfoPtr);
        break;
    case MX6_UART_CLK_ROOT:
        hr = this->GetUartClkRootInfo(ClockInfoPtr);
        break;
    case MX6_VIDEO_27M_CLK_ROOT:
        hr = this->GetVideo27MClkRootInfo(ClockInfoPtr);
        break;
    default:
        return E_NOTIMPL;
    }

    return hr;
}

MX6_CLK Mx6ClockTreeHelper::MX6ClkFromBypassClkSource (
    MX6_PLL_BYPASS_CLK_SRC BypassClockSource
    )
{
    switch (BypassClockSource) {
    case MX6_PLL_BYPASS_CLK_SRC_REF_CLK_24M: return MX6_OSC_CLK;
    case MX6_PLL_BYPASS_CLK_SRC_CLK1: return MX6_CLK1;
    case MX6_PLL_BYPASS_CLK_SRC_CLK2: return MX6_CLK2;
    case MX6_PLL_BYPASS_CLK_SRC_XOR:
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"Invalid MX6_PLL_BYPASS_CLK_SRC value. (BypassClockSource = %d)",
            BypassClockSource);
    }
}

_Use_decl_annotations_
void Mx6ClockTreeHelper::GetOsc24ClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    ClockInfoPtr->Frequency = MX6_REF_CLK_24M_FREQ;
    ClockInfoPtr->Parent = MX6_CLK_NONE;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPll1MainClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_ANALOG_PLL_ARM_REG pllArmReg =
        {this->registers.Analog.PLL_ARM};

    const MX6_CLK parent = this->MX6ClkFromBypassClkSource(
            static_cast<MX6_PLL_BYPASS_CLK_SRC>(pllArmReg.BYPASS_CLK_SRC));

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    if (pllArmReg.BYPASS != 0) {
        ClockInfoPtr->Frequency = parentInfo.Frequency;
        ClockInfoPtr->Parent = parent;
        return S_OK;
    }

    ClockInfoPtr->Frequency = static_cast<UINT32>(
            (UINT64)parentInfo.Frequency * pllArmReg.DIV_SELECT / 2);

    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPll2MainClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_ANALOG_PLL_SYS_REG pllSysReg =
        {this->registers.Analog.PLL_SYS};

    // Determine the reference clock source
    const MX6_CLK parent = this->MX6ClkFromBypassClkSource (
        static_cast<MX6_PLL_BYPASS_CLK_SRC>(pllSysReg.BYPASS_CLK_SRC));

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    if (pllSysReg.BYPASS != 0) {
        ClockInfoPtr->Frequency = parentInfo.Frequency;
        ClockInfoPtr->Parent = parent;
        return S_OK;
    }

    if (pllSysReg.DIV_SELECT == 0) {
        ClockInfoPtr->Frequency = parentInfo.Frequency * 20;
    } else {
        ClockInfoPtr->Frequency = parentInfo.Frequency * 22;
    }

    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPll2PfdClkInfo (
    MX6_PLL_PFD PfdIndex,
    MX6_CLK_INFO* ClockInfoPtr
    )
{
    const MX6_CCM_PFD_528_REG pfd528Reg =
        {this->registers.Analog.PFD_528};

    UINT32 pfdFrac;
    switch (PfdIndex) {
    case MX6_PLL_PFD0:
        pfdFrac = pfd528Reg.PFD0_FRAC;
        break;
    case MX6_PLL_PFD1:
        pfdFrac = pfd528Reg.PFD1_FRAC;
        break;
    case MX6_PLL_PFD2:
        pfdFrac = pfd528Reg.PFD2_FRAC;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"Invalid PfdIndex. (PfdIndex = %d)",
            PfdIndex);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_PLL2_MAIN_CLK, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    // The resulting frequency shall be 528*18/PFDn_FRAC
    // where PFD0_FRAC is in the range 12-35.
    if ((pfdFrac < 12) || (pfdFrac > 35)) {
        throw wexception::make(
            E_INVALIDARG,
            L"PFD fractional divider is out of range. (pfdFrac = %d)",
            pfdFrac);
    }

    ClockInfoPtr->Frequency = static_cast<UINT32>(
            (UINT64)parentInfo.Frequency * 18 / pfdFrac);

    ClockInfoPtr->Parent = MX6_PLL2_MAIN_CLK;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPll3MainClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_ANALOG_PLL_USB1_REG pllUsb1Reg =
        {this->registers.Analog.PLL_USB1};

    const MX6_CLK parent = this->MX6ClkFromBypassClkSource (
        static_cast<MX6_PLL_BYPASS_CLK_SRC>(pllUsb1Reg.BYPASS_CLK_SRC));

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    if (pllUsb1Reg.DIV_SELECT == 0) {
        ClockInfoPtr->Frequency = parentInfo.Frequency * 20;
    } else {
        ClockInfoPtr->Frequency = parentInfo.Frequency * 22;
    }

    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPll3PfdClkInfo (
    MX6_PLL_PFD PfdIndex,
    MX6_CLK_INFO* ClockInfoPtr
    )
{
    const MX6_CCM_PFD_480_REG pfd480Reg =
        {this->registers.Analog.PFD_480};

    UINT32 pfdFrac;
    switch (PfdIndex) {
    case MX6_PLL_PFD0:
        pfdFrac = pfd480Reg.PFD0_FRAC;
        break;
    case MX6_PLL_PFD1:
        pfdFrac = pfd480Reg.PFD1_FRAC;
        break;
    case MX6_PLL_PFD2:
        pfdFrac = pfd480Reg.PFD2_FRAC;
        break;
    case MX6_PLL_PFD3:
        pfdFrac = pfd480Reg.PFD3_FRAC;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"Invalid PFD index. (PfdIndex = %d)",
            PfdIndex);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_PLL3_MAIN_CLK, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    // The resulting frequency shall be 480*18/PFDn_FRAC
    // where PFD0_FRAC is in the range 12-35.
    if ((pfdFrac < 12) || (pfdFrac > 35)) {
        throw wexception::make(
            E_INVALIDARG,
            L"PFD fractional divider is out of range. (pfdFrac = %d)",
            pfdFrac);
    }

    ClockInfoPtr->Frequency = static_cast<UINT32>(
            (UINT64)parentInfo.Frequency * 18 / pfdFrac);

    ClockInfoPtr->Parent = MX6_PLL3_MAIN_CLK;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPll3SwClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CCSR_REG ccsrReg =
        {this->registers.Ccm.CCSR};

    MX6_CLK parent;
    if (ccsrReg.pll3_sw_clk_sel == MX6_CCM_PLL3_SW_CLK_SEL_PLL3_MAIN_CLK) {
        parent = MX6_PLL3_MAIN_CLK;
    } else {
        return E_NOTIMPL;
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency;
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

HRESULT Mx6ClockTreeHelper::GetPll3_80MInfo (_Out_ MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_PLL3_SW_CLK, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency / 6;
    ClockInfoPtr->Parent = MX6_PLL3_SW_CLK;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetAxiClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCDR_REG cbcdrReg =
        {this->registers.Ccm.CBCDR};

    MX6_CLK parent;
    if (cbcdrReg.axi_sel == MX6_CCM_AXI_SEL_PERIPH_CLK) {
        parent = MX6_PERIPH_CLK;
    } else {
        parent = MX6_AXI_ALT;
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cbcdrReg.axi_podf);
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPeriphClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCDR_REG cbcdrReg =
        {this->registers.Ccm.CBCDR};

    MX6_CLK parent;

    // NOTE: periph_clk_sel is OR'd with PLL_bypass_en2 (from jtag) to
    //       produce the input value to the MUX. We assume PLL_bypass_en2 is 0.
    if (cbcdrReg.periph_clk_sel == 0) {
        parent = MX6_PRE_PERIPH_CLK;
    } else {
        parent = MX6_PERIPH_CLK2;
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cbcdrReg.mmdc_ch0_axi_podf);
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPrePeriphClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg =
        {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    switch (cbcmrReg.pre_periph_clk_sel) {
    case MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2:
        parent = MX6_PLL2_MAIN_CLK;
        break;
    case MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2_PFD2:
        parent = MX6_PLL2_PFD2;
        break;
    case MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2_PFD0:
        parent = MX6_PLL2_PFD0;
        break;
    case MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2_PFD2_DIV2:
        parent = MX6_PLL2_PFD2;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"cbcmrReg.pre_periph_clk_sel is invalid (%d)",
            cbcmrReg.pre_periph_clk_sel);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    if (cbcmrReg.pre_periph_clk_sel == MX6_CCM_PRE_PERIPH_CLK_SEL_PLL2_PFD2_DIV2) {
        ClockInfoPtr->Frequency = parentInfo.Frequency / 2;
    } else {
        ClockInfoPtr->Frequency = parentInfo.Frequency;
    }

    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPeriphClk2Info (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg =
        {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    switch (cbcmrReg.periph_clk2_sel) {
    case MX6_CCM_PERIPH_CLK2_SEL_PLL3_SW_CLK:
        parent = MX6_PLL3_SW_CLK;
        break;
    case MX6_CCM_PERIPH_CLK2_SEL_OSC_CLK:
        parent = MX6_OSC_CLK;
        break;
    case MX6_CCM_PERIPH_CLK2_SEL_PLL2:
        parent = MX6_PLL2_MAIN_CLK;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"cbcmrReg.periph_clk2_sel is invalid (%d)",
            cbcmrReg.periph_clk2_sel);
    }

    const MX6_CCM_CBCDR_REG cbcdrReg =
        {this->registers.Ccm.CBCDR};

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cbcdrReg.periph_clk2_podf);
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetArmClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO pll1Info;
    HRESULT hr = this->GetClockInfo(MX6_PLL1_MAIN_CLK, &pll1Info);
    if (FAILED(hr)) {
        return hr;
    }

    const MX6_CCM_CACRR_REG cacrrReg =
        {this->registers.Ccm.CACRR};

    ClockInfoPtr->Frequency = pll1Info.Frequency / (1 + cacrrReg.arm_podf);
    ClockInfoPtr->Parent = MX6_PLL1_MAIN_CLK;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetMmdcCh0ClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_PERIPH_CLK, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    const MX6_CCM_CBCDR_REG cbcdrReg =
        {this->registers.Ccm.CBCDR};

    ClockInfoPtr->Frequency =
        parentInfo.Frequency / (1 + cbcdrReg.mmdc_ch0_axi_podf);

    ClockInfoPtr->Parent = MX6_PERIPH_CLK;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetAhbClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_PERIPH_CLK, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    const MX6_CCM_CBCDR_REG cbcdrReg =
        {this->registers.Ccm.CBCDR};

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cbcdrReg.ahb_podf);
    ClockInfoPtr->Parent = MX6_PERIPH_CLK;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetIpgClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_AHB_CLK_ROOT, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    const MX6_CCM_CBCDR_REG cbcdrReg =
        {this->registers.Ccm.CBCDR};

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cbcdrReg.ipg_podf);
    ClockInfoPtr->Parent = MX6_AHB_CLK_ROOT;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetPerclkClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_IPG_CLK_ROOT, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    const MX6_CCM_CSCMR1_REG cscmr1Reg = {this->registers.Ccm.CSCMR1};

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cscmr1Reg.perclk_podf);
    ClockInfoPtr->Parent = MX6_IPG_CLK_ROOT;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetUsdhcClkRootInfo (
    ULONG Index,
    MX6_CLK_INFO* ClockInfoPtr
    )
{
    // USDHC1-4 (Index is zero-based)
    if (Index >= 4) {
        throw wexception::make(
            E_INVALIDARG,
            L"USDHC index is out of range (Index = %d)",
            Index);
    }

    // get muxing info from CSCMR1 : usdhcN_clk_sel
    const MX6_CCM_CSCMR1_REG cscmr1Reg =
        {this->registers.Ccm.CSCMR1};

    // get divider into from CSCDR1 : usdhcN_podf
    const MX6_CCM_CSCDR1_REG cscdr1Reg =
        {this->registers.Ccm.CSCDR1};

    ULONG clockSel;
    ULONG podf;
    MX6_CLK_GATE gate;
    switch (Index) {
    case 0:
        clockSel = cscmr1Reg.usdhc1_clk_sel;
        podf = cscdr1Reg.usdhc1_podf;
        gate = MX6_USDHC1_CLK_ENABLE;
        break;
    case 1:
        clockSel = cscmr1Reg.usdhc2_clk_sel;
        podf = cscdr1Reg.usdhc2_podf;
        gate = MX6_USDHC2_CLK_ENABLE;
        break;
    case 2:
        clockSel = cscmr1Reg.usdhc3_clk_sel;
        podf = cscdr1Reg.usdhc3_podf;
        gate = MX6_USDHC3_CLK_ENABLE;
        break;
    case 3:
        clockSel = cscmr1Reg.usdhc4_clk_sel;
        podf = cscdr1Reg.usdhc4_podf;
        gate = MX6_USDHC4_CLK_ENABLE;
        break;
    default:
        return E_FAIL;
    }

    MX6_CLK parent;
    if (clockSel == MX6_CCM_USDHC_CLK_SEL_PLL2_PFD2) {
        parent = MX6_PLL2_PFD2;
    } else {
        parent = MX6_PLL2_PFD0;
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + podf);
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetSsiClkRootInfo (
    ULONG Index,
    MX6_CLK_INFO* ClockInfoPtr
    )
{
    // SSI1-3 (Index is zero-based)
    if (Index >= 3) {
        throw wexception::make(
            E_INVALIDARG,
            L"SSI index is out of range. (Index = %d)",
            Index);
    }

    // Get muxing info from CSCMR1[ssiN_clk_sel]
    const MX6_CCM_CSCMR1_REG cscmr1Reg =
        {this->registers.Ccm.CSCMR1};

    ULONG clockSel;
    MX6_CLK_GATE gate;
    switch (Index) {
    case 0:
        clockSel = cscmr1Reg.ssi1_clk_sel;
        gate = MX6_SSI1_CLK_ENABLE;
        break;
    case 1:
        clockSel = cscmr1Reg.ssi2_clk_sel;
        gate = MX6_SSI2_CLK_ENABLE;
        break;
    case 2:
        clockSel = cscmr1Reg.ssi3_clk_sel;
        gate = MX6_SSI3_CLK_ENABLE;
        break;
    default:
        return E_FAIL;
    }

    // Get divider info from CS1,2CDR[ssiN_clk_pred] and CS1,2CDR[ssiN_clk_podf]
    ULONG clockPred;
    ULONG clockPodf;
    if (Index == 1) {
        const MX6_CCM_CS2CDR_REG cs2cdrReg =
            {this->registers.Ccm.CS2CDR};

        clockPred = cs2cdrReg.ssi2_clk_pred;
        clockPodf = cs2cdrReg.ssi2_clk_podf;
    } else {
        const MX6_CCM_CS1CDR_REG cs1cdrReg =
            {this->registers.Ccm.CS1CDR};

        if (Index == 0) {
            clockPred = cs1cdrReg.ssi1_clk_pred;
            clockPodf = cs1cdrReg.ssi1_clk_podf;
        } else {
            // Index == 2
            clockPred = cs1cdrReg.ssi3_clk_pred;
            clockPodf = cs1cdrReg.ssi3_clk_podf;
        }
    }

    MX6_CLK parent;
    switch (clockSel) {
    case MX6_CCM_SSI_CLK_SEL_PLL3_PFD2:
        parent = MX6_PLL3_PFD2;
        break;
    case MX6_CCM_SSI_CLK_SEL_PLL3_PFD3:
        parent = MX6_PLL3_PFD3;
        break;
    case MX6_CCM_SSI_CLK_SEL_PLL4:
        parent = MX6_PLL4_MAIN_CLK;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"Invalid clock selection value (clockSel = %u)",
            clockSel);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency =
        parentInfo.Frequency / (1 + clockPred) / (1 + clockPodf);

    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetGpu2dAxiClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg =
        {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    if (cbcmrReg.gpu2d_axi_clk_sel == MX6_CCM_GPU2D_AXI_CLK_SEL_AXI) {
        parent = MX6_AXI_CLK_ROOT;
    } else {
        parent = MX6_AHB_CLK_ROOT;
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency;
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetGpu3dAxiClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg = {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    if (cbcmrReg.gpu3d_axi_clk_sel == MX6_CCM_GPU3D_AXI_CLK_SEL_AXI) {
        parent = MX6_AXI_CLK_ROOT;
    } else {
        parent = MX6_AHB_CLK_ROOT;
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency;
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetGpu2dCoreClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg =
        {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    switch (cbcmrReg.gpu2d_core_clk_sel) {
    case MX6_CCM_GPU2D_CORE_CLK_SEL_AXI:
        parent = MX6_AXI_CLK_ROOT;
        break;
    case MX6_CCM_GPU2D_CORE_CLK_SEL_PLL3_SW:
        parent = MX6_PLL3_SW_CLK;
        break;
    case MX6_CCM_GPU2D_CORE_CLK_SEL_PLL2_PFD0:
        parent = MX6_PLL2_PFD0;
        break;
    case MX6_CCM_GPU2D_CORE_CLK_SEL_PLL2_PFD2:
        parent = MX6_PLL2_PFD2;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"cbcmrReg.gpu2d_core_clk_sel is invalid (%d)",
            cbcmrReg.gpu2d_core_clk_sel);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency =
        parentInfo.Frequency / (1 + cbcmrReg.gpu2d_core_clk_podf);

    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetGpu3dCoreClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg =
        {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    switch (cbcmrReg.gpu3d_core_clk_sel) {
    case MX6_CCM_GPU3D_CORE_CLK_SEL_MMDC_CH0_AXI:
        parent = MX6_MMDC_CH0_CLK_ROOT;
        break;
    case MX6_CCM_GPU3D_CORE_CLK_SEL_PLL3_SW:
        parent = MX6_PLL3_SW_CLK;
        break;
    case MX6_CCM_GPU3D_CORE_CLK_SEL_PLL2_PFD1:
        parent = MX6_PLL2_PFD1;
        break;
    case MX6_CCM_GPU3D_CORE_CLK_SEL_PLL2_PFD2:
        parent = MX6_PLL2_PFD2;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"cbcmrReg.gpu3d_core_clk_sel is invalid (%d)",
            cbcmrReg.gpu3d_core_clk_sel);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cbcmrReg.gpu3d_core_podf);
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetGpu3dShaderClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg =
        {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    switch (cbcmrReg.gpu3d_shader_clk_sel) {
    case MX6_CCM_GPU3D_SHADER_CLK_SEL_MMDC_CH0_AXI:
        parent = MX6_MMDC_CH0_CLK_ROOT;
        break;
    case MX6_CCM_GPU3D_SHADER_CLK_SEL_PLL3_SW:
        parent = MX6_PLL3_SW_CLK;
        break;
    case MX6_CCM_GPU3D_SHADER_CLK_SEL_PLL2_PFD1:
        parent = MX6_PLL2_PFD1;
        break;
    case MX6_CCM_GPU3D_SHADER_CLK_SEL_PLL3_PFD0:
        parent = MX6_PLL3_PFD0;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"cbcmrReg.gpu3d_shader_clk_sel is invalid (%d)",
            cbcmrReg.gpu3d_shader_clk_sel);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency =
        parentInfo.Frequency / (1 + cbcmrReg.gpu3d_shader_podf);

    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetUartClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_PLL3_80M, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    const MX6_CCM_CSCDR1_REG cscdr1Reg = {this->registers.Ccm.CSCDR1};

    ClockInfoPtr->Frequency =
        parentInfo.Frequency / (1 + cscdr1Reg.uart_clk_podf);

    ClockInfoPtr->Parent = MX6_PLL3_80M;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetVpuAxiClkInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    const MX6_CCM_CBCMR_REG cbcmrReg =
        {this->registers.Ccm.CBCMR};

    MX6_CLK parent;
    switch (cbcmrReg.vpu_axi_clk_sel) {
    case VPU_AXI_CLK_SEL_AXI_CLK_ROOT:
        parent = MX6_AXI_CLK_ROOT;
        break;
    case VPU_AXI_CLK_SEL_PLL2_PFD2:
        parent = MX6_PLL2_PFD2;
        break;
    case VPU_AXI_CLK_SEL_PLL2_PFD0:
        parent = MX6_PLL2_PFD0;
        break;
    default:
        throw wexception::make(
            E_INVALIDARG,
            L"cbcmrReg.vpu_axi_clk_sel is invalid. (%d)",
            cbcmrReg.vpu_axi_clk_sel);
    }

    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(parent, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    const MX6_CCM_CSCDR1_REG cscdr1Reg =
        {this->registers.Ccm.CSCDR1};

    ClockInfoPtr->Frequency = parentInfo.Frequency / (1 + cscdr1Reg.vpu_axi_podf);
    ClockInfoPtr->Parent = parent;

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mx6ClockTreeHelper::GetVideo27MClkRootInfo (MX6_CLK_INFO* ClockInfoPtr)
{
    MX6_CLK_INFO parentInfo;
    HRESULT hr = this->GetClockInfo(MX6_PLL3_PFD1, &parentInfo);
    if (FAILED(hr)) {
        return hr;
    }

    ClockInfoPtr->Frequency = parentInfo.Frequency / 20;
    ClockInfoPtr->Parent = MX6_PLL3_PFD1;

    return S_OK;
}


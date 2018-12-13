/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imx_hw.cpp

Abstract:

    This module contains the functions for accessing
    the hardware registers.

Environment:

    kernel-mode only

Revision History:

*/

#include "imxi2cinternal.h"
#include "imx_hw.tmh"

USHORT
HWREG<USHORT>::Read(VOID)
{
    USHORT v = (USHORT)0;
    volatile USHORT* addr = &m_Value;
    v = READ_REGISTER_USHORT((PUSHORT)addr);
    return v;
}

USHORT
HWREG<USHORT>::Write(_In_ USHORT Value)
{
    volatile USHORT* addr = &m_Value;
    WRITE_REGISTER_USHORT((PUSHORT)addr, Value);
    return Value;
}
/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imxi2chw.h

Abstract:

    This module contains the function definitions for
    the hardware registers.

Environment:

    kernel-mode only

Revision History:

*/

#ifndef _IMXI2CHW_H_
#define _IMXI2CHW_H_


template<typename T> struct HWREG
{
private:

    // Only one data member - this has to fit in the same space as the underlying type.

    T m_Value;

public:

    T Read(void);
    T Write(_In_ T value);

    // Operators with standard meanings.

    T operator=(_In_ T value)
    {
        return Write(value);
    }

    T operator|=(_In_ T value)
    {
        return Write(Read() | value);
    }

    T operator&=(_In_ T value)
    {
        return Write(value & Read());
    }

    operator T()
    {
        return Read();
    }

};

#endif // of _IMXI2CHW_H_
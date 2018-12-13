// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Ntddk.h>

extern "C" {
    #include <sdport.h>
    #include <acpiioct.h>
    #include <initguid.h>
} // extern "C"

#define NONPAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    //__pragma(code_seg(.text))

#define NONPAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define INIT_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("INIT"))

#define INIT_SEGMENT_END \
    __pragma(code_seg(pop))

#define ASSERT_MAX_IRQL(IRQL) NT_ASSERT(KeGetCurrentIrql() <= (IRQL))

template<class T>
__forceinline
T Min(
    _In_ const T& v1,
    _In_ const T& v2
    ) 
{
    if (v1 <= v2) {
        return v1;
    } else {
        return v2;
    }
}

template<class T>
__forceinline
T Max(
    _In_ const T& v1,
    _In_ const T& v2
) 
{
    if (v1 >= v2) {
        return v1;
    } else {
        return v2;
    }
}

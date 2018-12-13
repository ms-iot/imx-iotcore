// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   iMXUtility.hpp
//
// Abstract:
//
//   This file contains helpers used by imxgpio.
//
// Environment:
//
//  Kernel mode only
//

#ifndef _IMXUTILITY_HPP_
#define _IMXUTILITY_HPP_ 1

//
// Macros to be used for proper PAGED/NON-PAGED code placement
//

#define IMX_NONPAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    //__pragma(code_seg(.text))

#define IMX_NONPAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMX_PAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("PAGE"))

#define IMX_PAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define IMX_INIT_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("INIT"))

#define IMX_INIT_SEGMENT_END \
    __pragma(code_seg(pop))

//
// We have some non-paged functions that supposed to be called on low IRQL.
// The following macro defines unified assert to be put at the beginning of
// such functions.
//
// NOTE: We can't use standard PAGED_CODE macro as it requires function to be
// placed in paged segment during compilation.
//
#define IMX_ASSERT_MAX_IRQL(Irql) NT_ASSERT(KeGetCurrentIrql() <= (Irql))
#define IMX_ASSERT_LOW_IRQL() IMX_ASSERT_MAX_IRQL(APC_LEVEL)

//
// Default memory allocation and object construction for C++ modules
//

#pragma warning(push)
#pragma warning(disable:4595)

inline void* __cdecl operator new (
    size_t Size,
    POOL_TYPE PoolType,
    ULONG Tag
    ) throw ()
{
    if (!Size) Size = 1;
    return ExAllocatePoolWithTag(PoolType, Size, Tag);
} // operator new (size_t, POOL_TYPE, ULONG)

inline void __cdecl operator delete (
    void* Ptr,
    POOL_TYPE, // PoolType,
    ULONG Tag
    ) throw ()
{
    if (Ptr) ExFreePoolWithTag(Ptr, Tag);
} // operator delete (void*, POOL_TYPE, ULONG)

inline void __cdecl operator delete (
    void* Ptr
    ) throw ()
{
    if (Ptr) ExFreePool(Ptr);
} // operator delete (void*)

inline void* __cdecl operator new[] (
    size_t Size,
    POOL_TYPE PoolType,
    ULONG Tag
    ) throw ()
{
    if (!Size) Size = 1;
    return ExAllocatePoolWithTag(PoolType, Size, Tag);
} // operator new[] (size_t, POOL_TYPE, ULONG)

inline void __cdecl operator delete[] (
    void* Ptr,
    POOL_TYPE, // PoolType,
    ULONG Tag
    ) throw ()
{
    if (Ptr) ExFreePoolWithTag(Ptr, ULONG(Tag));
} // operator delete[] (void*, POOL_TYPE, ULONG)

inline void __cdecl operator delete[] (void* Ptr) throw ()
{
    if (Ptr) ExFreePool(Ptr);
} // operator delete[] (void*)

inline void* __cdecl operator new (size_t, void* Ptr) throw ()
{
    return Ptr;
} // operator new (size_t, void*)

inline void __cdecl operator delete (void*, void*) throw ()
{} // void operator delete (void*, void*)

inline void* __cdecl operator new[] (size_t, void* Ptr) throw ()
{
    return Ptr;
} // operator new[] (size_t, void*)

inline void __cdecl operator delete[] (void*, void*) throw ()
{} // void operator delete[] (void*, void*)

inline void __cdecl operator delete (void*, size_t) throw ()
{} // void operator delete (void*, size_t)

#pragma warning(pop)

//
// class INTERRUPT_BANK_LOCK
//
// Scoped lock object for acquiring and releasing GpioClx interrupt bank lock.
//
class INTERRUPT_BANK_LOCK {
public:
    _IRQL_raises_(DISPATCH_LEVEL+1)
    INTERRUPT_BANK_LOCK(PVOID GpioClxContextPtr, ULONG BankId) :
        contextPtr(GpioClxContextPtr),
        bankId(static_cast<BANK_ID>(BankId))
    {
        GPIO_CLX_AcquireInterruptLock(
            GpioClxContextPtr,
            static_cast<BANK_ID>(BankId));
    }

    ~INTERRUPT_BANK_LOCK()
    {
        GPIO_CLX_ReleaseInterruptLock(this->contextPtr, this->bankId);
    }

private:
    PVOID contextPtr;
    BANK_ID bankId;
};

// inline helper function to handle interlocked and operations for ULONGs
static inline ULONG InterlockedAnd (
    _Inout_ _Interlocked_operand_ ULONG volatile *Destination,
    _In_ ULONG Value
    )
{
    return static_cast<ULONG>(
        InterlockedAnd(
            reinterpret_cast<volatile LONG*>(Destination),
            static_cast<LONG>(Value)));
}

// inline helper function to handle interlocked and operations for ULONGs
static inline ULONG InterlockedOr (
    _Inout_ _Interlocked_operand_ ULONG volatile *Destination,
    _In_ ULONG Value
    )
{
    return static_cast<ULONG>(
        InterlockedOr(
            reinterpret_cast<volatile LONG*>(Destination),
            static_cast<LONG>(Value)));
}

#endif // _IMXUTILITY_HPP_

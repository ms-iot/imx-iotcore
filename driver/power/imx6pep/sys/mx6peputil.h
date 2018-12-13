// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//   mx6peputil.h
//
// Abstract:
//
//   IMX6 PEP Utilities
//

#ifndef _MX6PEPUTIL_H_
#define _MX6PEPUTIL_H_


//
// Macros to be used for proper PAGED/NON-PAGED code placement
//

#define MX6_NONPAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    //__pragma(code_seg(.text))

#define MX6_NONPAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define MX6_PAGED_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("PAGE"))

#define MX6_PAGED_SEGMENT_END \
    __pragma(code_seg(pop))

#define MX6_INIT_SEGMENT_BEGIN \
    __pragma(code_seg(push)) \
    __pragma(code_seg("INIT"))

#define MX6_INIT_SEGMENT_END \
    __pragma(code_seg(pop))

#define MX6_ASSERT_MAX_IRQL(Irql) NT_ASSERT_ASSUME(KeGetCurrentIrql() <= (Irql))

enum : ULONG { MX6PEP_POOL_TAG = 'PPxM' };

//
// class MX6_FINALLY
//
class MX6_FINALLY {
private:

    MX6_FINALLY () = delete;

    template < typename T_OP > class _FINALIZER {
        friend class MX6_FINALLY;
        T_OP op;
        __forceinline _FINALIZER ( T_OP Op ) throw () : op(Op) {}
    public:
        __forceinline ~_FINALIZER () { (void)this->op(); }
    }; // class _FINALIZER

    template < typename T_OP > class _FINALIZER_EX {
        friend class MX6_FINALLY;
        bool doNot;
        T_OP op;
        __forceinline _FINALIZER_EX ( T_OP Op, bool DoNot ) throw () : doNot(DoNot), op(Op) {}
    public:
        __forceinline ~_FINALIZER_EX () { if (!this->doNot) (void)this->op(); }
        __forceinline void DoNot (bool DoNot = true) throw () { this->doNot = DoNot; }
        __forceinline void DoNow () throw () { this->DoNot(); (void)this->op(); }
    }; // class _FINALIZER_EX

public:

    template < typename T_OP > __forceinline static _FINALIZER<T_OP> Do ( T_OP Op ) throw ()
    {
        return _FINALIZER<T_OP>(Op);
    }; // Do<...> (...)

    template < typename T_OP > __forceinline static _FINALIZER_EX<T_OP> DoUnless (
        T_OP Op,
        bool DoNot = false
        ) throw ()
    {
        return _FINALIZER_EX<T_OP>(Op, DoNot);
    }; // DoUnless<...> (...)
}; // class MX6_FINALLY

//
// class MX6_SPINLOCK_GUARD
//
// Utility for acquiring a spinlock and releasing it when the enclosing
// scope is exited. Objects of this type must be allocated in nonpaged memory.
//
class MX6_SPINLOCK_GUARD {
public:

    _Requires_lock_not_held_(this->lockHandle)
    _Acquires_lock_(this->lockHandle)
    _Post_same_lock_(*SpinLockPtr, this->lockHandle)
    _IRQL_requires_max_(DISPATCH_LEVEL)
    _IRQL_saves_global_(QueuedSpinLock,this->lockHandle)
    _IRQL_raises_(DISPATCH_LEVEL)
    __forceinline MX6_SPINLOCK_GUARD (KSPIN_LOCK* SpinLockPtr)
    {
        KeAcquireInStackQueuedSpinLock(SpinLockPtr, &this->lockHandle);
    }

    _Requires_lock_held_(this->lockHandle)
    _Releases_lock_(this->lockHandle)
    _IRQL_requires_(DISPATCH_LEVEL)
    _IRQL_restores_global_(QueuedSpinLock,this->lockHandle)
    __forceinline ~MX6_SPINLOCK_GUARD ()
    {
        KeReleaseInStackQueuedSpinLock(&this->lockHandle);
    }

private:
    KLOCK_QUEUE_HANDLE lockHandle;
};

// Get a ULONG* to a register at some byte offset from base
__forceinline volatile ULONG* MX6RegAddress (
    volatile void* BasePtr,
    ULONG OffsetInBytes
    )
{
    NT_ASSERT((OffsetInBytes % sizeof(ULONG)) == 0);
    return reinterpret_cast<volatile ULONG*>(
        static_cast<volatile unsigned char*>(BasePtr) + OffsetInBytes);
}

#endif // _MX6PEPUTIL_H_

/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    ImxCpuRev.h

Abstract:

    Contains utilities to detect IMX SOC flavor and version.
    We use the same terminology as U-Boot, where SOC_TYPE means IMX6 vs. IMX7,
    and CPU_TYPE means the specific model of CPU, like MX6Q.

Environment:

    Kernel mode only.

Revision History:

*/
#ifndef _IMXCPUREV_H_
#define _IMXCPUREV_H_

typedef enum _IMX_SOC {
    IMX_SOC_UNKNOWN = 0,
    IMX_SOC_MX6 = 0x60,
    IMX_SOC_MX7 = 0x70,
    IMX_SOC_MX8M = 0x80,
} IMX_SOC;

typedef enum _IMX_CPU {
    IMX_CPU_MX6SL = 0x60,
    IMX_CPU_MX6DL = 0x61,
    IMX_CPU_MX6SX = 0x62,
    IMX_CPU_MX6Q = 0x63,
    IMX_CPU_MX6UL = 0x64,
    IMX_CPU_MX6ULL = 0x65,
    IMX_CPU_MX6SOLO = 0x66,
    IMX_CPU_MX6SLL = 0x67,
    IMX_CPU_MX6D = 0x6A,
    IMX_CPU_MX6DP = 0x68,
    IMX_CPU_MX6QP = 0x69,
    IMX_CPU_MX7S = 0x71,
    IMX_CPU_MX7D = 0x72,
    IMX_CPU_MX8MQ = 0x82,
    IMX_CPU_MX8MM = 0x85,
    IMX_CPU_MX7ULP = 0xE1, // dummy value
} IMX_CPU;

// returns IMX_CPU_ value
#define IMX_CPU_TYPE(rev) ((IMX_CPU)(((rev) >> 12) & 0xff))
#define IMX_SOC_TYPE(rev) ((IMX_SOC)(((rev) >> 12) & 0xf0))

#ifdef HALEXT
#define MapIoSpace(Addr, Length) \
    HalMapIoSpace(Addr, Length, MmNonCached)

#define UnmapIoSpace HalUnmapIoSpace
#else
#define MapIoSpace(Addr, Length) \
    MmMapIoSpaceEx(Addr, Length, PAGE_READWRITE | PAGE_NOCACHE)

#define UnmapIoSpace MmUnmapIoSpace
#endif // HALEXT

#ifndef Add2Ptr
#define Add2Ptr(Base, Offset) ((void *)((char *)(Base) + (Offset)))
#endif

#ifdef _ARM64_
#define ARM64_MIDR_EL1   ARM64_SYSREG(3, 0, 0, 0, 0)   // Main ID Register
#define ARM64_CCSIDR_EL1 ARM64_SYSREG(3, 1, 0, 0, 0)   // Cache Size ID Register
#define ARM64_CSSELR_EL1 ARM64_SYSREG(3, 2, 0, 1, 0)   // Cache Size Selection Register
#else
#define IMX_CP15_MIDR   15, 0,  0,  0, 0     // Main ID Register
#define IMX_CP15_CCSIDR 15, 1,  0,  0, 0     // Cache Size ID Register
#define IMX_CP15_CSSELR 15, 2,  0,  0, 0     // Cache Size Selection Register
#endif

static inline UINT32 _Imx6GetCpuRevHelper (void *AnatopBase, void *ScuBase)
{
    enum {
        SCU_CONFIG = 0x8,
        ANATOP_DIGPROG = 0x260,
        ANATOP_DIGPROG_SOLOLITE = 0x280,
    };

    UINT32 reg = READ_REGISTER_NOFENCE_ULONG(
        (ULONG *)Add2Ptr(AnatopBase, ANATOP_DIGPROG_SOLOLITE));

    UINT32 type = ((reg >> 16) & 0xff);
    UINT32 major, cfg = 0;

    if (type != IMX_CPU_MX6SL) {
        reg = READ_REGISTER_NOFENCE_ULONG(
            (ULONG *)Add2Ptr(AnatopBase, ANATOP_DIGPROG));

        cfg = READ_REGISTER_NOFENCE_ULONG(
            (ULONG *)Add2Ptr(ScuBase, SCU_CONFIG)) & 3;

        type = ((reg >> 16) & 0xff);
        if (type == IMX_CPU_MX6DL) {
            if (!cfg)
                type = IMX_CPU_MX6SOLO;
        }

        if (type == IMX_CPU_MX6Q) {
            if (cfg == 1)
                type = IMX_CPU_MX6D;
        }

    }
    major = ((reg >> 8) & 0xff);
    if ((major >= 1) &&
        ((type == IMX_CPU_MX6Q) || (type == IMX_CPU_MX6D))) {
            major--;
            type = IMX_CPU_MX6QP;
            if (cfg == 1)
                type = IMX_CPU_MX6DP;
    }
    reg &= 0xff;            /* mx6 silicon revision */
    return (type << 12) | (reg + (0x10 * (major + 1)));
}

_IRQL_requires_max_(PASSIVE_LEVEL)
static inline NTSTATUS _Imx6GetCpuRev (_Out_ UINT32 *Rev)
{
    enum {
        SCU_BASE_ADDR = 0x00A00000,
        ANATOP_BASE_ADDR = 0x020C8000,
    };

    PHYSICAL_ADDRESS physAddr;
    NTSTATUS status;
    void *anatopBase = NULL;
    void *scuBase = NULL;

    physAddr.QuadPart = ANATOP_BASE_ADDR;
    anatopBase = MapIoSpace(physAddr, 0x1000);
    if (anatopBase == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    physAddr.QuadPart = SCU_BASE_ADDR;
    scuBase = MapIoSpace(physAddr, 0x1000);
    if (scuBase == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    *Rev = _Imx6GetCpuRevHelper(anatopBase, scuBase);
    status = STATUS_SUCCESS;

end:
    if (anatopBase != NULL)
        UnmapIoSpace(anatopBase, 0x1000);

    if (scuBase != NULL)
        UnmapIoSpace(scuBase, 0x1000);

    return status;
}

static inline BOOLEAN _IsImx7D (void *OcotpBase)
{
    enum {
        FUSE_BANK1_TESTER4 = 0x450,
    };

    ULONG val;

    val = READ_REGISTER_NOFENCE_ULONG(
        (ULONG *)Add2Ptr(OcotpBase, FUSE_BANK1_TESTER4));

    return (val & 1) == 0;
}

static inline UINT32 _Imx7GetCpuRevHelper (void *AnatopBase, void *OcotpBase)
{
    enum {
        ANATOP_DIGPROG = 0x800,
    };

    UINT32 reg = READ_REGISTER_NOFENCE_ULONG(
        (ULONG *)Add2Ptr(AnatopBase, ANATOP_DIGPROG));

    UINT32 type = (reg >> 16) & 0xff;

    if (!_IsImx7D(OcotpBase))
        type = IMX_CPU_MX7S;

    reg &= 0xff;
    return (type << 12) | reg;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
static inline NTSTATUS _Imx7GetCpuRev (_Out_ UINT32 *Rev)
{
    enum {
        OCOTP_BASE_ADDR = 0x30350000,
        ANATOP_BASE_ADDR = 0x30360000,
    };

    PHYSICAL_ADDRESS physAddr;
    NTSTATUS status;
    void *anatopBase = NULL;
    void *ocotpBase = NULL;

    physAddr.QuadPart = ANATOP_BASE_ADDR;
    anatopBase = MapIoSpace(physAddr, 0x1000);
    if (anatopBase == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    physAddr.QuadPart = OCOTP_BASE_ADDR;
    ocotpBase = MapIoSpace(physAddr, 0x1000);
    if (ocotpBase == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    *Rev = _Imx7GetCpuRevHelper(anatopBase, ocotpBase);
    status = STATUS_SUCCESS;

end:
    if (anatopBase != NULL)
        UnmapIoSpace(anatopBase, 0x1000);

    if (ocotpBase != NULL)
        UnmapIoSpace(ocotpBase, 0x1000);

    return status;
}

static inline UINT32 _Imx8GetCpuRevHelper (void *AnatopBase)
{
    enum {
        ANATOP_DIGPROG = 0x06C,
        ANATOP_DIGPROGMM = 0x800,
    };

    UINT32 reg = READ_REGISTER_NOFENCE_ULONG(
        (ULONG *)Add2Ptr(AnatopBase, ANATOP_DIGPROG));

    UINT32 type = (reg >> 16) & 0xff;

    // unrecognized SOC type, check alternate DIGPROC register on mini variant
    if (type != 0x82) {
        UINT32 reg2 = READ_REGISTER_NOFENCE_ULONG(
                      (ULONG *)Add2Ptr(AnatopBase, ANATOP_DIGPROGMM));
        if ((reg2 & 0x00ffff00) == 0x00824100) {
            type = IMX_CPU_MX8MM;
            reg = reg2;
        }
    }

    reg &= 0xff;  /* silicon revision */
    return (type << 12) | reg;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
static inline NTSTATUS _Imx8GetCpuRev (_Out_ UINT32 *Rev)
{
    enum {
        ANATOP_BASE_ADDR = 0x30360000,
    };

    PHYSICAL_ADDRESS physAddr;
    NTSTATUS status;
    void *anatopBase = NULL;

    physAddr.QuadPart = ANATOP_BASE_ADDR;
    anatopBase = MapIoSpace(physAddr, 0x1000);
    if (anatopBase == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    *Rev = _Imx8GetCpuRevHelper(anatopBase);
    status = STATUS_SUCCESS;

end:
    if (anatopBase != NULL)
        UnmapIoSpace(anatopBase, 0x1000);

    return status;
}

#ifdef _ARM64_
static inline UINT32 _ImxReadMIDR (void)
{
    return (UINT32)_ReadStatusReg(ARM64_MIDR_EL1);
}

static inline UINT32 _ImxReadCSSELR (void)
{
    return (UINT32)_ReadStatusReg(ARM64_CSSELR_EL1);
}

static inline UINT32 _ImxReadCCSIDR (void)
{
    return (UINT32)_ReadStatusReg(ARM64_CCSIDR_EL1);
}

static inline void _ImxWriteCSSELR (UINT32 value)
{
    _WriteStatusReg(ARM64_CSSELR_EL1, value);
}
#else
static inline UINT32 _ImxReadMIDR (void)
{
    return _MoveFromCoprocessor(IMX_CP15_MIDR);
}

static inline UINT32 _ImxReadCSSELR (void)
{
    return _MoveFromCoprocessor(IMX_CP15_CSSELR);
}

static inline UINT32 _ImxReadCCSIDR (void)
{
    return _MoveFromCoprocessor(IMX_CP15_CCSIDR);
}

static inline void _ImxWriteCSSELR (UINT32 value)
{
    _MoveToCoprocessor(value, IMX_CP15_CSSELR);
}
#endif


static inline UINT32 _ImxGetPrimaryPartNum (void)
{
    UINT32 midr;

    midr = _ImxReadMIDR();

    return (midr >> 4) & 0xFFF;
}

__pragma(code_seg(push))    // Nonpaged
static NTSTATUS _ImxGetCacheSize (UINT32 Level, _Out_ UINT32 *CacheSize)
{
    UINT32 ccsidr;
    UINT32 csselr;

#ifndef HALEXT
    KIRQL oldIrql;
#endif

    Level = (Level & 0x7) << 1;

#ifndef HALEXT
    //
    // The following operation cannot be interrupted. Stepping through
    // the following statements with the debugger will cause CSSELR to be
    // set before CCSIDR is read.
    //
    KeRaiseIrql(HIGH_LEVEL, &oldIrql);
#endif

    // Write to CP15 Cache Size Selection Register
    _ImxWriteCSSELR(Level);
    _InstructionSynchronizationBarrier();

    // Read from CP15 Cache Size ID Register
    ccsidr = _ImxReadCCSIDR();
    csselr = _ImxReadCSSELR();

#ifndef HALEXT
    KeLowerIrql(oldIrql);
#endif

    //
    // If CSSELR changed, we didn't get the right CCSIDR value.
    // Only the debugger should be able to interrupt the code above
    //
    if (csselr != Level) {
        NT_ASSERT(!"Failed to read CCSIDR. This should only happen if stepping "
                   "through with the debugger");

        return STATUS_UNSUCCESSFUL;
    }

    // Compute cache size in bytes
    *CacheSize = 4 * (1 << ((ccsidr & 0x3) + 2)) *    // line size
           (((ccsidr >> 3) & 0x3FF) + 1) *      // associativity
           (((ccsidr >> 13) & 0x7FFF) + 1);     // number of sets

    return STATUS_SUCCESS;
}
__pragma(code_seg(pop))

static inline IMX_SOC ImxGetSocType (void)
{
    NTSTATUS status;
    UINT32 partNum;
    UINT32 l2Size;

    partNum = _ImxGetPrimaryPartNum();
    switch (partNum) {
    case 0xC07:     // Cortex-A7

        //
        // If it's Cortex-A7, it's either IMX6UL, IMX6ULL, or IMX7
        // IMX6 variants have 128KB L2 cache whereas IMX7 has 256KB or more.
        //
        status = _ImxGetCacheSize(1, &l2Size);
        if (!NT_SUCCESS(status)) {
            // If it's Cortex-A7 it's most likely IMX7
            return IMX_SOC_MX7;
        }

        if (l2Size >= (256 * 1024)) {
            return IMX_SOC_MX7;
        } else {
            return IMX_SOC_MX6;
        }

    case 0xC09:     // Cortex-A9
        return IMX_SOC_MX6;

    case 0xD03:     // Cortex-A53
		return IMX_SOC_MX8M;

    case 0xC0F:     // Cortex-A15
    case 0xD04:     // Cortex-A35
    case 0xD05:     // Cortex-A55
    case 0xD09:     // Cortex-A73
    case 0xD0A:     // Cortex-A75
    default:
        return IMX_SOC_UNKNOWN;
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
static inline NTSTATUS ImxGetCpuRev (_Out_ UINT32 *Rev)
{
    IMX_SOC socType = ImxGetSocType();

    switch (socType) {
    case IMX_SOC_MX6:
        return _Imx6GetCpuRev(Rev);

    case IMX_SOC_MX7:
        return _Imx7GetCpuRev(Rev);

    case IMX_SOC_MX8M:
        return _Imx8GetCpuRev(Rev);

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

#undef MapIoSpace
#undef UnmapIoSpace
#undef IMX_CP15_MIDR
#undef IMX_CP15_CCSIDR
#undef IMX_CP15_CSSELR
#undef ARM64_MIDR_EL1
#undef ARM64_CCSIDR_EL1
#undef ARM64_CSSELR_EL1

#endif // _IMXCPUREV_H_

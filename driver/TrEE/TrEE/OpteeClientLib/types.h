#pragma once
#include <ntddk.h>

// ---------------------------------------------------------------- Definitions

#if defined(_WIN64)
typedef UINT64 UINTN;

#else
typedef UINT32 UINTN;

#endif

typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

typedef UINT8 BYTE;

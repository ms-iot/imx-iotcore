// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Checksum service
// GUID {af6127a7-86de-4822-a815-b5f5cc2bdc46}

//
// Calculates checksum of given data stream.
//
// State = 0;
// for (BYTE v in Data) { State += v; }
// return (UCHAR)((State >> 24) ^ (State >> 16) ^ (State >> 8) ^ State);
//

DEFINE_GUID(GUID_OPTEE_CHECKSUM_SERVICE,
    0xaf6127a7, 0x86de, 0x4822, 0xa8, 0x15, 0xb5, 0xf5, 0xcc, 0x2b, 0xdc, 0x46);

//
// Checksum session initialization.
//
// Input: None
// Output: None
//

#define CHECKSUM_FUNCTION_INITIALIZE        0

//
// Update checksum.
//
// Input: Data block to calculate checksum
// Output: None
//

#define CHECKSUM_FUNCTION_UPDATE            1

//
// Retrieve final checksum value and clear the internal state.
//
// Input: None
// Output: Checksum value (8-bit)
//

#define CHECKSUM_FUNCTION_FINALIZE          2

//
// Test builtin TEST service.
//
// Input: Array of ULONGs
// Output: Sum of input ULONGs
//

#define CHECKSUM_FUNCTION_CHECK_BUILTIN_TEST 3

//
// Test builtin RPMB service.
//
// Input: None
// Output: None
//

#define CHECKSUM_FUNCTION_CHECK_BUILTIN_RPMB 4

//
// Delayed completion test
//
// Input: ULONG, delay in ms
// Output: None
//

#define CHECKSUM_FUNCTION_DELAYED_COMPLETION 5

//
// Service other I/O example.
//
// Input: None
// Output: L"Hello, world!" null-terminated string
//
#define IOCTL_TR_CHECKSUM_HELLOWORLD        CTL_CODE(FILE_DEVICE_TRUST_ENV, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// RNG service
// GUID {967734EF-2421-45B3-85C0-78F537F2EF88}
//

DEFINE_GUID(GUID_OPTEE_RNG_SERVICE,
    0x967734ef, 0x2421, 0x45b3, 0x85, 0xc0, 0x78, 0xf5, 0x37, 0xf2, 0xef, 0x88);

//
// Seed the RNG with 32-bit value.
//
// Input: 32-bit value
// Output: None
//

#define RNG_FUNCTION_SEED                   0

//
// Discard random value.
//
// Input: Number of bytes to discard
// Output: None
//

#define RNG_FUNCTION_DISCARD                1

//
// Get random value.
//
// Input: None
// Output: Arbitrary length block filled with random values.
//

#define RNG_FUNCTION_GET_RANDOM             2

//
// Get random value.
// Available only from kernel mode, just because it can be enforced.
//
// Input: None
// Output: Arbitrary length block filled with random values.
//

#define RNG_FUNCTION_GET_RANDOM_KERNEL      3

//
// Delayed completion test
//
// Input: ULONG, delay in ms
// Output: None
//

#define RNG_FUNCTION_DELAYED_COMPLETION     4

//
// Concurrent request high mark
//
// Input: None
// Output: ULONG, highest number of concurrent requests seen
//

#define RNG_FUNCTION_CONCURRENCY_HIGH_MARK  5

//
// Clear concurrent request high mark
//
// Input: None
// Output: None
//

#define RNG_FUNCTION_CLEAR_HIGH_MARK        6

//
// OPTee Test Service
// {B1CC44AE-B9AF-4AAA-8BC1-54C49B24D5AD}
//

DEFINE_GUID(GUID_OPTEE_TEST_SERVICE,
    0xb1cc44ae, 0xb9af, 0x4aaa, 0x8b, 0xc1, 0x54, 0xc4, 0x9b, 0x24, 0xd5, 0xad);

#define TEST_FUNCTION_MEMALLOC              1
#define TEST_FUNCTION_BOGUS                 2


//
// Retrieves "Hello, world!" string.
// Input : None
// Output : WCHAR[14]
//
#define IOCTL_SAMPLE_HELLOWORLD     CTL_CODE(FILE_DEVICE_TRUST_ENV, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HELLOWORLD_STRING L"Hello, world!"

//
// Prints a string to debugger through OutputDebugMessageEx
// Input : NULL-terminated WCHAR[], size includes the terminating NULL character
// Output : None
//
#define IOCTL_SAMPLE_OutputDebugMessage       CTL_CODE(FILE_DEVICE_TRUST_ENV, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// OPTee OPM Service
// {20779E63-433C-4E7A-AFA6-7B8E71CE2208}
//

DEFINE_GUID(GUID_OPTEE_OPM_SERVICE,
    0x20779e63, 0x433c, 0x4e7a, 0xaf, 0xa6, 0x7b, 0x8e, 0x71, 0xce, 0x22, 0x08);

//
// Calls the OPM TA to perform RSA Decryption
// Input : Encrypted data blob
// Output : Decrypted data blob
//
#define IOCTL_OPM_RSA_DECRYPT        CTL_CODE(FILE_DEVICE_TRUST_ENV, 0x881, METHOD_BUFFERED, FILE_ANY_ACCESS)

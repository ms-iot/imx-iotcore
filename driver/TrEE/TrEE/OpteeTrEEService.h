// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
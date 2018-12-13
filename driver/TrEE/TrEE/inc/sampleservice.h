// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
//
// Memory allocation service
// GUID {2CF25EDE-7ACD-4118-AB5E-CB82F222B39D}
//
DEFINE_GUID(GUID_MEMALLOC_SERVICE,
    0x2cf25ede, 0x7acd, 0x4118, 0xab, 0x5e, 0xcb, 0x82, 0xf2, 0x22, 0xb3, 0x9d);

DECLARE_CONST_UNICODE_STRING(GUID_MEMALLOC_SERVICE_STRING,
                             L"{2CF25EDE-7ACD-4118-AB5E-CB82F222B39D}");

//
// Allocate a block of paged memory with given size
//
// Input: ULONG indicating size
// Output: PVOID pointing the allocated memory
//
#define MEMALLOC_SERVICE_ALLOCATE           1

//
// Frees the memory allocated by preceding function
//
// Input: PVOID pointing the allocated memory
// Output: None
//
#define MEMALLOC_SERVICE_FREE               2

//
// A bogus service
// {F9E63191-E96B-47FF-BED9-447DA029F8BA}
//
DEFINE_GUID(GUID_BOGUS_SERVICE, 
    0xf9e63191, 0xe96b, 0x47ff, 0xbe, 0xd9, 0x44, 0x7d, 0xa0, 0x29, 0xf8, 0xba);

DECLARE_CONST_UNICODE_STRING(GUID_BOGUS_SERVICE_STRING,
                             L"{F9E63191-E96B-47FF-BED9-447DA029F8BA}");

//
// Returns random quote
//
// Input: None
// Output: WCHAR[256]
//
#define BOGUS_SERVICE_GET_RANDOM_QUOTE           1

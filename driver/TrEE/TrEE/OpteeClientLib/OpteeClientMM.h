#pragma once 
#include <wdf.h>

struct _OPTEE_CLIENT_MM_HEADER
{
    PHYSICAL_ADDRESS BasePA;
    ULONG Length;
    PVOID BaseVA;
    PVOID BitMapAddress;
    RTL_BITMAP BitMapHeader;
};

typedef struct _OPTEE_CLIENT_MM_HEADER 
    OPTEE_MM_HEADER, *POPTEE_MM_HEADER;

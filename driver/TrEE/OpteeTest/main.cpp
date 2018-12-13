// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <windows.h>
#include <initguid.h>
#include <TrustedRT.h>
#include <memory>

#include "OpteeCalls.h"

//
// OP-TEE TA GUIDs
//

//{8AAAF200-2450-11E4-ABE2-0002A5D5C51B}

DEFINE_GUID(GUID_OPTEE_HELLO_WORLD_PTA_SERVICE,
    0x8AAAF200, 0x2450, 0x11E4, 0xAB, 0xE2, 0x00, 0x02, 0xA5, 0xD5, 0xC5, 0x1B);

DEFINE_GUID(GUID_OPTEE_HELLO_WORLD_TA_SERVICE,
    0xb1cc44ae, 0xb9af, 0x4aaa, 0x8b, 0xc1, 0x54, 0xc4, 0x9b, 0x24, 0xd5, 0xad);

DEFINE_GUID(GUID_EFI_VARIABLE_SERVICE,
    0x699aa2f1, 0xa42e, 0x40df, 0xba, 0xbe, 0x3a, 0xaa, 0xd2, 0xbb, 0x6a, 0x47);

// EF484CBB-E6B5-4667-95A3-173D2C883FF5
DEFINE_GUID(GUID_CYREP_TEST_TA_SERVICE,
    0xef484cbb, 0xe6b5, 0x4667, 0x95, 0xa3, 0x17, 0x3d, 0x2c, 0x88, 0x3f, 0xf5);

#define OPTEE_SERVICE_MAX_FUNCTIONS 10
#define OPTEE_SERVICE_MAX_SESSIONS 5
#define OPTEE_SERVICE_OPENED_BY_SYSTEM 0


//
// Service function handlers
//

typedef DWORD EVT_SERVICE_CMD_HANDLER(_In_ HANDLE ServiceHandle, _In_ int Function);
typedef EVT_SERVICE_CMD_HANDLER *PFN_SERVICE_CMD_HANDLER;

//
// Test TA command handlers
//
EVT_SERVICE_CMD_HANDLER HelloWorldPtaHandlerCmd;
EVT_SERVICE_CMD_HANDLER HelloWorldTaHandlerCmd;

// Cyrep Test TA command handlers
EVT_SERVICE_CMD_HANDLER CyrepTestTaGetPrivateKey;
EVT_SERVICE_CMD_HANDLER CyrepTestTaGetCertChain;

struct OPTEE_SERVICE_FUNC_DESC {
    PCWSTR NameWsz;
    PFN_SERVICE_CMD_HANDLER HandlerPtr;
};

struct OPTEE_SERVICE_DESC {
    PCWSTR NameWsz;
    LPCGUID GuidPtr;
    HANDLE Handles[OPTEE_SERVICE_MAX_SESSIONS];
    OPTEE_SERVICE_FUNC_DESC FunctionDescs[OPTEE_SERVICE_MAX_FUNCTIONS];
};


//
// AuthVar service test
//

#define VARIABLE_ATTRIBUTE_NON_VOLATILE                           0x00000001
#define VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS                     0x00000002
#define VARIABLE_ATTRIBUTE_RUNTIME_ACCESS                         0x00000004
#define VARIABLE_ATTRIBUTE_HARDWARE_ERROR_RECORD                  0x00000008
#define VARIABLE_ATTRIBUTE_AUTHENTICATED_WRITE_ACCESS             0x00000010
#define VARIABLE_ATTRIBUTE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS  0x00000020
#define VARIABLE_ATTRIBUTE_APPEND_WRITE                           0x00000040

#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE     (22L)
#define SE_CREATE_SYMBOLIC_LINK_PRIVILEGE   (35L)

//
// AuthVar service test configuration
//
#define AUTH_VAR_TEST_LOOP_COUNT 5

//
// AuthVar service handlers
//
EVT_SERVICE_CMD_HANDLER AuthVarTestHandler;

//
// OP-TEE Service descriptors
//

OPTEE_SERVICE_DESC OpteeServices[] = {

    // Test service 1
    {
        L"OP-TEE Test Service Hello World PTA",
        &GUID_OPTEE_HELLO_WORLD_PTA_SERVICE,
        { INVALID_HANDLE_VALUE },
        {
            { L"Hello World PTA Cmd1 (reverse buffer)", HelloWorldPtaHandlerCmd }, // Function 1
            { 0 }
        }
    },

    // Test service 2
    {
        L"OP-TEE Test Service Hello World TA",
        &GUID_OPTEE_HELLO_WORLD_TA_SERVICE,
        { INVALID_HANDLE_VALUE },
        {
            { L"Hello World TA Cmd1 (reverse buffer)", HelloWorldTaHandlerCmd }, // Function 1
            { L"Hello World TA Cmd2", HelloWorldTaHandlerCmd }, // Function 2
            { 0 }
        }
    },

    // AuthVar service
    {
        L"AuthVar Service",
        &GUID_EFI_VARIABLE_SERVICE,
        { OPTEE_SERVICE_OPENED_BY_SYSTEM },
        {
            { L"AuthVarTest", AuthVarTestHandler },
            { 0 }
        }
    },

    // Cyrep Test TA
    {
        L"Cyrep Test TA (for testing TA measurement)",
        &GUID_CYREP_TEST_TA_SERVICE,
        { INVALID_HANDLE_VALUE },
        {
            { L"Get Private Key", CyrepTestTaGetPrivateKey }, // Function 1
            { L"Get Certificate Chain", CyrepTestTaGetCertChain }, // Function 2
            { 0 }
        }
    },

    //
    // More services...
    //
};

inline unsigned int MyGetwch ()
{
    return _getwch() & 0x0000FFFF;
}

VOID
ListServices (
    _In_ int ServiceIndex
    )
{
    if (ServiceIndex == -1) {
        int serviceInx;

        wprintf(L"Service list:\n");
        for (serviceInx = 0;
             serviceInx < ARRAYSIZE(OpteeServices);
             ++serviceInx) {

            OPTEE_SERVICE_DESC* svcDescPtr = &OpteeServices[serviceInx];

            wprintf(L"%d) %ws , current sessions: ",
                serviceInx + 1,
                svcDescPtr->NameWsz);

            int sessions = 0;
            for (int sessionInx = 0;
                 sessionInx < ARRAYSIZE(svcDescPtr->Handles);
                 ++sessionInx) {

                HANDLE handle = svcDescPtr->Handles[sessionInx];

                if ((handle != INVALID_HANDLE_VALUE) &&
                    (handle != OPTEE_SERVICE_OPENED_BY_SYSTEM)) {

                    ++sessions;
                    wprintf(L"%d ", sessionInx);
                }
            }
            if (sessions == 0)
                wprintf(L"none\n");
            else
                wprintf(L"\n");
        }
    } else {
        if ((ServiceIndex == 0) || (ServiceIndex > ARRAYSIZE(OpteeServices))) {
            wprintf(L"Error: Invalid service index %d!'\n", ServiceIndex);
            return;
        }

        OPTEE_SERVICE_DESC* svcDescPtr = &OpteeServices[ServiceIndex - 1];

        wprintf(L"Service '%ws', current sessions: ", svcDescPtr->NameWsz);
        int sessions = 0;
        for (int sessionInx = 0;
             sessionInx < ARRAYSIZE(svcDescPtr->Handles);
             ++sessionInx) {

            HANDLE handle = svcDescPtr->Handles[sessionInx];

            if ((handle != INVALID_HANDLE_VALUE) &&
                (handle != OPTEE_SERVICE_OPENED_BY_SYSTEM)) {

                ++sessions;
                wprintf(L"%d ", sessionInx);
            }
        }
        if (sessions == 0)
            wprintf(L"none\n");
        else
            wprintf(L"\n");
        wprintf(L" function list:\n");

        for (int funcInx = 0; funcInx < OPTEE_SERVICE_MAX_FUNCTIONS; ++funcInx) {
            if (svcDescPtr->FunctionDescs[funcInx].NameWsz != nullptr) {
                wprintf(L"%d) %ws\n",
                    funcInx + 1,
                    svcDescPtr->FunctionDescs[funcInx].NameWsz);
            }
        }
    }
}

HANDLE
OpenServiceHandleByFilename (
    _In_ LPCGUID ServiceGuid,
    _In_ int SessionIndex
    )
{
    WCHAR interfaceGuid[1024];
    WCHAR interfaceSymlink[1024];
    HANDLE serviceHandle;

    serviceHandle = INVALID_HANDLE_VALUE;
    swprintf_s(
        interfaceGuid,
        L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        ServiceGuid->Data1,
        ServiceGuid->Data2,
        ServiceGuid->Data3,
        ServiceGuid->Data4[0],
        ServiceGuid->Data4[1],
        ServiceGuid->Data4[2],
        ServiceGuid->Data4[3],
        ServiceGuid->Data4[4],
        ServiceGuid->Data4[5],
        ServiceGuid->Data4[6],
        ServiceGuid->Data4[7]);

    swprintf_s(interfaceSymlink, L"\\\\.\\WindowsTrustedRT\\%ws", interfaceGuid);

    serviceHandle = CreateFileW(
        interfaceSymlink,
        FILE_READ_DATA | FILE_WRITE_DATA,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (serviceHandle == INVALID_HANDLE_VALUE) {
        wprintf(
            L"Error: Opening service: Guid=%ws, Symlink=%ws failed, "
            "status %d!\n",
            interfaceGuid,
            interfaceSymlink,
            GetLastError());

    } else {
        wprintf(
            L"Opening service: Guid=%ws succeeded, "
            L"service handle %p, session %d!\n",
            interfaceGuid,
            serviceHandle,
            SessionIndex);
    }

    return serviceHandle;
}

VOID
OpenServiceHandleByIndex (
    _In_ int ServiceIndex
    )
{
    if ((ServiceIndex == 0) || (ServiceIndex > ARRAYSIZE(OpteeServices))) {
        wprintf(L"Error: Invalid service index %d!'\n", ServiceIndex);
        return;
    }

    OPTEE_SERVICE_DESC* svcDescPtr = &OpteeServices[ServiceIndex - 1];

    HANDLE* handlePtr;
    int sessionIndex;
    for (sessionIndex = 0;
         sessionIndex < ARRAYSIZE(svcDescPtr->Handles);
         ++sessionIndex) {

        handlePtr = &svcDescPtr->Handles[sessionIndex];
        if ((*handlePtr == INVALID_HANDLE_VALUE) ||
            (*handlePtr == OPTEE_SERVICE_OPENED_BY_SYSTEM)) {
            break;
        }
    }

    if (*handlePtr == OPTEE_SERVICE_OPENED_BY_SYSTEM) {
        wprintf(
            L"Service '%ws' already opened by system\n",
            svcDescPtr->NameWsz);

        return;
    }

    if (*handlePtr != INVALID_HANDLE_VALUE) {
        wprintf(
            L"Cannot open more sessions for service '%ws'...\n",
            svcDescPtr->NameWsz);

        return;
    }

    *handlePtr = OpenServiceHandleByFilename(svcDescPtr->GuidPtr, sessionIndex);
}

VOID
CloseServiceHandleByIndex (
    _In_ int ServiceIndex,
    _In_ int SessionIndex
)
{
    if ((ServiceIndex == 0) || (ServiceIndex > ARRAYSIZE(OpteeServices))) {
        wprintf(L"Error: Invalid service index %d!'\n", ServiceIndex);
        return;
    }

    OPTEE_SERVICE_DESC* svcDescPtr = &OpteeServices[ServiceIndex - 1];

    if ((SessionIndex < 0) ||
        (SessionIndex >= ARRAYSIZE(svcDescPtr->Handles))) {

        wprintf(L"Error: Invalid session index %d!'\n", SessionIndex);
        return;
    }

    HANDLE* handlePtr = &svcDescPtr->Handles[SessionIndex];

    if (*handlePtr == OPTEE_SERVICE_OPENED_BY_SYSTEM) {
        wprintf(
            L"Service '%ws' cannot be closed, opened by system'\n",
            svcDescPtr->NameWsz);

        return;
    }

    if (*handlePtr != INVALID_HANDLE_VALUE) {
        if (CloseHandle(*handlePtr)) {
            wprintf(
                L"Service '%ws'  session %d successfully closed'\n",
                svcDescPtr->NameWsz,
                SessionIndex);

        } else {
            wprintf(
                L"Error: Service close failed! status %d!'\n",
                GetLastError());
        }
    } else {
        wprintf(
            L"Error: Service '%ws' session %d was not opened'\n",
            svcDescPtr->NameWsz,
            SessionIndex);
    }
    *handlePtr = INVALID_HANDLE_VALUE;
}


VOID
ExecuteServiceFunction (
    _In_ int ServiceIndex,
    _In_ int SessionIndex,
    _In_ int FunctionIndex
    )
{
    if ((ServiceIndex == 0) || (ServiceIndex > ARRAYSIZE(OpteeServices))) {
        wprintf(L"Error: Invalid service index %d!'\n", ServiceIndex);
        return;
    }

    OPTEE_SERVICE_DESC* svcDescPtr = &OpteeServices[ServiceIndex - 1];

    if ((SessionIndex < 0) ||
        (SessionIndex >= ARRAYSIZE(svcDescPtr->Handles))) {

        wprintf(L"Error: Invalid session index %d!'\n", SessionIndex);
        return;
    }

    HANDLE sessionHandle = svcDescPtr->Handles[SessionIndex];

    if (sessionHandle == INVALID_HANDLE_VALUE) {
        wprintf(
            L"Error: Service '%ws' is not opened, "
            "use 'o' to open target service!'\n",
            svcDescPtr->NameWsz);

        return;
    }

    if ((FunctionIndex == 0) ||
        (FunctionIndex > ARRAYSIZE(svcDescPtr->FunctionDescs)) ||
        (svcDescPtr->FunctionDescs[FunctionIndex-1].NameWsz == nullptr)) {

        wprintf(L"Error: Invalid function index %d'\n", FunctionIndex);
        return;
    }

    OPTEE_SERVICE_FUNC_DESC* funcDescPtr =
        &svcDescPtr->FunctionDescs[FunctionIndex-1];

    if (funcDescPtr->HandlerPtr == nullptr) {
        wprintf(L"Error: Service %ws does not support function index %d!'\n",
            svcDescPtr->NameWsz,
            FunctionIndex);

        return;
    }

    wprintf(L"Info: running service %ws, function %ws in thread ID 0x%08X\n",
        svcDescPtr->NameWsz,
        funcDescPtr->NameWsz,
        GetCurrentThreadId());

    DWORD status = funcDescPtr->HandlerPtr(sessionHandle, FunctionIndex);
    if (status != NO_ERROR) {
        wprintf(L"Error: service %ws, session %d, function %ws failed! Status %d\n",
            svcDescPtr->NameWsz,
            SessionIndex,
            funcDescPtr->NameWsz,
            status);

    } else {
        wprintf(L"Service %ws, session %d, function %ws succeeded\n",
            svcDescPtr->NameWsz,
            SessionIndex,
            funcDescPtr->NameWsz);
    }
}

struct SERVICE_FUNC_PARAMS {
    int Service;
    int Session;
    int Function;

    SERVICE_FUNC_PARAMS(
        _In_ int ServiceIndex,
        _In_ int SessionIndex,
        _In_ int FunctionIndex
        ) : Service(ServiceIndex)
          , Session(SessionIndex)
          , Function(FunctionIndex)
    {}
};


DWORD
WINAPI
ServiceFunctionThread (LPVOID ParameterPtr)
{
    SERVICE_FUNC_PARAMS* serviceFuncParams = (SERVICE_FUNC_PARAMS*)ParameterPtr;

    wprintf(L"\nInfo: running service function deferred, thread ID 0x%08X\n",
        GetCurrentThreadId());

    ExecuteServiceFunction(
        serviceFuncParams->Service,
        serviceFuncParams->Session,
        serviceFuncParams->Function);

    delete serviceFuncParams;

    return NO_ERROR;
}


DWORD
ExecuteServiceFunctionDeferred (
    _In_ int ServiceIndex,
    _In_ int SessionIndex,
    _In_ int FunctionIndex
    )
{
    SERVICE_FUNC_PARAMS* serviceFuncParams =
        new (std::nothrow) SERVICE_FUNC_PARAMS(ServiceIndex, SessionIndex, FunctionIndex);

    if (serviceFuncParams == nullptr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    HANDLE workerThread = CreateThread(
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)ServiceFunctionThread,
        serviceFuncParams,
        0,
        nullptr);

    if (workerThread == NULL) {
        DWORD status = GetLastError();
        wprintf(L"Error: Failed to create work thread! Status %d\n",
            status);

        delete serviceFuncParams;
        return status;
    }

    CloseHandle(workerThread);
    return NO_ERROR;
}


void Init(void)
{
    for (int serviceInx = 0;
         serviceInx < ARRAYSIZE(OpteeServices);
         ++serviceInx) {

        OPTEE_SERVICE_DESC* svcDescPtr = &OpteeServices[serviceInx];

        for (int sessionInx = 1;
             sessionInx < ARRAYSIZE(svcDescPtr->Handles);
             ++sessionInx) {

            svcDescPtr->Handles[sessionInx] = svcDescPtr->Handles[0];
        }
    }
}


PCWSTR HelpSwz =
    L"OpteeTest commands:\n"
    L"e - Enumerate services\n"
    L"l - list service functions. Usage l<service index>\n"
    L"      Use 'e' to get service list\n"
    L"o - Open service. Usage o<service index>\n"
    L"      Use 'e' to get service list\n"
    L"c - Close service. Usage c<service index><session index>\n"
    L"      Use 'e' to get service list\n"
    L"f - execute service Function. "
          L"Usage f<service index><session index><funtion index>\n"
    L"      Use 'e' to get service list\n"
    L"      Use 'l' to get service function list\n"
    L"F - Same as 'f' but deferred, in a separate thread. "
          "Usage F<service index><session index><funtion index>\n"
    L"h - Print this help message\n"
    L"q - Quit\n"
    ;

int __cdecl wmain (int , const wchar_t* )
{
    Init();

    wprintf(L"%ws\n", HelpSwz);

    bool isDone = false;
    while (!isDone) {
        wprintf(L"Command =>");
        unsigned int cmd = MyGetwch(); _putwch(WCHAR(cmd));

        bool isDeffered = false;
        switch (cmd) {
        case 'e':
            wprintf(L"\n");
            ListServices(-1);
            break;

        case 'l': {
            // Get service index
            cmd = MyGetwch(); _putwch(WCHAR(cmd)); wprintf(L"\n");
            int serviceIndex = atoi((const char*)&cmd);

            ListServices(serviceIndex);
            break;
        }

        case 'o': {
            // Get service index
            cmd = MyGetwch(); _putwch(WCHAR(cmd)); wprintf(L"\n");
            int serviceIndex = atoi((const char*)&cmd);

            OpenServiceHandleByIndex(serviceIndex);
            break;
        }

        case 'c': {
            // Get service index
            cmd = MyGetwch(); _putwch(WCHAR(cmd));
            int serviceIndex = atoi((const char*)&cmd);

            // Get session index
            cmd = MyGetwch(); _putwch(WCHAR(cmd)); wprintf(L"\n");
            int sessionIndex = atoi((const char*)&cmd);

            CloseServiceHandleByIndex(serviceIndex, sessionIndex);
            break;
        }

        case 'F':
            isDeffered = true;
            __fallthrough;
        case 'f': {
            // Get service index
            cmd = MyGetwch(); _putwch(WCHAR(cmd));
            int serviceIndex = atoi((const char*)&cmd);

            // Get session index
            cmd = MyGetwch(); _putwch(WCHAR(cmd));
            int sessionIndex = atoi((const char*)&cmd);

            // Get function index
            cmd = MyGetwch(); _putwch(WCHAR(cmd)); wprintf(L"\n");
            int functionIndex = atoi((const char*)&cmd);

            if (isDeffered) {
                ExecuteServiceFunctionDeferred(serviceIndex, sessionIndex, functionIndex);
            } else {
                ExecuteServiceFunction(serviceIndex, sessionIndex, functionIndex);
            }
            break;
        }

        case 'q':
        case 'Q':
            isDone = true;
            break;

        case 'h':
        case 'H':
        case '?':
            wprintf(L"\n%ws\n", HelpSwz);
            break;

        default:
            if ((cmd != '\n') && (cmd != '\r')) {
                wprintf(L"\nUnknown command '%c'\n", cmd);
            }
            break;
        }

        wprintf(L"\n");
    }

    return 0;
}


//
// Service function handlers
//

BOOL
CallTrEEService (
    _In_ HANDLE ServiceHandle,
    _In_ ULONG FunctionCode,
    _In_reads_bytes_(InputBufferLength) PVOID InputBufferPtr,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *BytesWritten) PVOID OutputBufferPtr,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG BytesWrittenPtr
    )
{
    TR_SERVICE_REQUEST request;
    TR_SERVICE_REQUEST_RESPONSE response;
    BOOL result;
    ULONG responseBytesWritten;

    request.FunctionCode = FunctionCode;
    request.InputBuffer = InputBufferPtr;
    request.InputBufferSize = InputBufferLength;
    request.OutputBuffer = OutputBufferPtr;
    request.OutputBufferSize = OutputBufferLength;

    result = DeviceIoControl(
        ServiceHandle,
        IOCTL_TR_EXECUTE_FUNCTION,
        &request,
        sizeof(request),
        &response,
        sizeof(response),
        &responseBytesWritten,
        NULL);

    if (responseBytesWritten >= sizeof(response)) {
        *BytesWrittenPtr = (ULONG)response.BytesWritten;
        return result;
    } else {

        *BytesWrittenPtr = 0;
        return FALSE;
    }
}


_Use_decl_annotations_
DWORD
HelloWorldPtaHandlerCmd (
    HANDLE ServiceHandle,
    int Function
    )
{
#define TEST_BUFFER_SIZE_BYTES 32

    UCHAR inBuffer[TEST_BUFFER_SIZE_BYTES];
    UCHAR outBuffer[TEST_BUFFER_SIZE_BYTES];
    for (UINT i = 0; i < ARRAYSIZE(inBuffer); ++i) {
        inBuffer[i] = (UCHAR)i;
    }

    UINT32 bytesWritten;
    BOOL isSuccess = CallOpteeCommand(
        ServiceHandle,
        Function,
        inBuffer, sizeof(inBuffer),
        outBuffer, sizeof(outBuffer),
        &bytesWritten,
        NULL, // No callback
        NULL);

    if (!isSuccess) {
        return GetLastError();
    }

    switch (Function) {
    case 1:
        for (int i = 0; i < TEST_BUFFER_SIZE_BYTES; ++i) {
            if (inBuffer[i] != outBuffer[TEST_BUFFER_SIZE_BYTES - i - 1]) {
                wprintf(L"Error: hello world PTA cmd1, "
                    "unexpected value at offset %d (0x%2X), "
                    "should be  0x%2X\n",
                    i, outBuffer[TEST_BUFFER_SIZE_BYTES - i - 1],
                    inBuffer[i]);
            }
        }
        break;

    default:
        break;
    }

    return NO_ERROR;
}


_Use_decl_annotations_
DWORD
HelloWorldTaHandlerCmd (
    HANDLE ServiceHandle,
    int Function
    )
{
#define TEST_BUFFER_SIZE_BYTES 32

    UCHAR inBuffer[TEST_BUFFER_SIZE_BYTES];
    UCHAR outBuffer[TEST_BUFFER_SIZE_BYTES];
    for (UINT i = 0; i < ARRAYSIZE(inBuffer); ++i) {
        inBuffer[i] = (UCHAR)i;
    }

    UINT32 bytesWritten;
    BOOL isSuccess = CallOpteeCommand(
        ServiceHandle,
        Function,
        inBuffer, sizeof(inBuffer),
        outBuffer, sizeof(outBuffer),
        &bytesWritten,
        NULL, // No callback
        NULL);

    if (!isSuccess) {
        return GetLastError();
    }

    switch (Function) {
    case 1:
        for (int i = 0; i < TEST_BUFFER_SIZE_BYTES; ++i) {
            if (inBuffer[i] != outBuffer[TEST_BUFFER_SIZE_BYTES - i - 1]) {
                wprintf(L"Error: hello world TA cmd1, "
                    "unexpected value at offset %d (0x%2X), "
                    "should be  0x%2X\n",
                    i, outBuffer[TEST_BUFFER_SIZE_BYTES - i - 1],
                    inBuffer[i]);
            }
        }
        break;

    default:
        break;
    }

    return NO_ERROR;
}

DWORD
CyrepTestTaGetSize (
    HANDLE ServiceHandle,
    int Function,
    _Out_ UINT32 *Size
    )
{
    BYTE inBuffer[1];

    UINT32 bytesWritten;
    BOOL isSuccess = CallOpteeCommand(
        ServiceHandle,
        Function,
        inBuffer,
        sizeof(inBuffer),
        Size,
        sizeof(UINT32),
        &bytesWritten,
        NULL, // No callback
        NULL);

    if (!isSuccess) {
        return GetLastError();
    }

    return NO_ERROR;
}

_Use_decl_annotations_
DWORD
CyrepTestTaGetPrivateKey (
    HANDLE ServiceHandle,
    int Function
    )
{
    const int PTA_CYREP_GET_PRIVATE_KEY_SIZE = 3;
    DWORD res;
    UINT32 size;
    res = CyrepTestTaGetSize(
            ServiceHandle,
            PTA_CYREP_GET_PRIVATE_KEY_SIZE,
            &size);

    if (res != NO_ERROR)
        return res;

    wprintf(L"Required buffer size: %d bytes\n", size);

    // dummy in buffer is required by generic service interface
    BYTE inBuffer[1];
    std::unique_ptr<char[]> outBuffer(new char[size]);

    UINT32 bytesWritten;
    BOOL isSuccess = CallOpteeCommand(
        ServiceHandle,
        Function,
        inBuffer,
        sizeof(inBuffer),
        outBuffer.get(),
        size,
        &bytesWritten,
        NULL, // No callback
        NULL);

    if (!isSuccess) {
        return GetLastError();
    }

    outBuffer.get()[size - 1] = '\0';

    wprintf(L"Private Key:\n");
    printf("%s\n", outBuffer.get());

    wprintf(L"\n");

    return NO_ERROR;
}

_Use_decl_annotations_
DWORD
CyrepTestTaGetCertChain (
    HANDLE ServiceHandle,
    int Function
    )
{
    const int PTA_CYREP_GET_CERT_CHAIN_SIZE = 4;
    DWORD res;
    UINT32 size;
    res = CyrepTestTaGetSize(
            ServiceHandle,
            PTA_CYREP_GET_CERT_CHAIN_SIZE,
            &size);

    if (res != NO_ERROR)
        return res;

    wprintf(L"Required buffer size: %d bytes\n", size);

    // dummy input buffer is required by generic service
    BYTE inBuffer[1];
    std::unique_ptr<char[]> outBuffer(new char[size]);

    UINT32 bytesWritten;
    BOOL isSuccess = CallOpteeCommand(
        ServiceHandle,
        Function,
        inBuffer,
        sizeof(inBuffer),
        outBuffer.get(),
        size,
        &bytesWritten,
        NULL, // No callback
        NULL);

    if (!isSuccess) {
        return GetLastError();
    }

    outBuffer.get()[size - 1] = '\0';

    wprintf(L"Cert Chain:\n");
    printf("%s\n", outBuffer.get());

    return NO_ERROR;
}

LUID
ConvertLongToLuid (
    _In_ LONG Long
    )
{
    LUID tempLuid;
    LARGE_INTEGER tempLi;

    tempLi.QuadPart = Long;
    tempLuid.LowPart = tempLi.u.LowPart;
    tempLuid.HighPart = tempLi.u.HighPart;

    return tempLuid;
}


DWORD
EnablePrivilege (
    _In_ DWORD NewPrivilege
    )
{
    TOKEN_PRIVILEGES newState;
    DWORD returnLength = 0;
    HRESULT status;
    HANDLE tokenHandle = NULL;

    if (!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ALL_ACCESS,
            &tokenHandle)) {
        //
        // Can't open the process token
        //
        status = GetLastError();
        wprintf(L"Failed to Open Process Token 0x%x\n", status);
        goto done;

    } else {
        //
        //  OpenProcessToken succeeded. Check if the token in null
        //
        if (tokenHandle == NULL) {
            status = GetLastError();
            wprintf(L"TokenHandle == NULL Error: 0x%x\n", status);
            goto done;
        }
    }

    newState.PrivilegeCount = 1;
    newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    newState.Privileges[0].Luid = ConvertLongToLuid(NewPrivilege);

    if (!AdjustTokenPrivileges(
            tokenHandle,
            FALSE,
            &newState,
            sizeof(TOKEN_PRIVILEGES),
            NULL,
            &returnLength)) {

        status = GetLastError();
        goto done;
    }

    status = NO_ERROR;

done:
    if (tokenHandle != NULL) {
        CloseHandle(tokenHandle);
    }

    return status;
}


_Use_decl_annotations_
DWORD
AuthVarTestHandler (
    HANDLE ServiceHandle,
    int Function
    )
{
    DWORD status;
    WCHAR testVarName[255];
    GUID testGuid;
    WCHAR testGUIDString[40];

    UNREFERENCED_PARAMETER(ServiceHandle);
    UNREFERENCED_PARAMETER(Function);

    CoCreateGuid(&testGuid);
    StringFromGUID2(testGuid, testGUIDString, _countof(testGUIDString));

    status = EnablePrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE);

    if (status != NO_ERROR) {
        wprintf(L"Enable Privilege Failed 0x%x\n", status);
        return status;
    }

    for (int i = 0; i < AUTH_VAR_TEST_LOOP_COUNT; i++) {

        UINT32 expData = 0x12345678;
        DWORD dataSize;
        UINT32 obsData = 0;

        swprintf_s(testVarName, 255, L"Test-Variable_%d", i);

        wprintf(L"\n[%d] =====Test Variable name: %s =====\n", i, testVarName);

        status = NO_ERROR;
        dataSize = sizeof(expData);
        if (!SetFirmwareEnvironmentVariableEx(
                testVarName,
                testGUIDString,
                &expData,
                dataSize,
                (VARIABLE_ATTRIBUTE_NON_VOLATILE |
                    VARIABLE_ATTRIBUTE_RUNTIME_ACCESS |
                    VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS))) {

            status = GetLastError();
        }

        wprintf(L"[%d] Set variable status = 0x%x\n", i, status);
        if (status != NO_ERROR) {
            break;
        }

        if (!GetFirmwareEnvironmentVariableEx(
                testVarName,
                testGUIDString,
                &obsData,
                dataSize,
                nullptr)) {

            status = GetLastError();
        }

        wprintf(L"[%d] Get variable status = 0x%x\n", i, status);
        if (status != NO_ERROR) {
            break;
        }

        wprintf(L"[%d] Observed data = 0x%x dataSize = %d\n", i, obsData, dataSize);
        if (expData != obsData) {
            wprintf(
                L"[%d] FAILED: Expected data=0x%x, Observed Data=0x%x\n",
                i,
                expData,
                obsData);

            status = ERROR_INVALID_DATA;
            break;
        }

        //
        // Deleting the variable by setting it to zero.
        //
        if (!SetFirmwareEnvironmentVariableEx(
                testVarName,
                testGUIDString,
                &expData,
                0,
                (VARIABLE_ATTRIBUTE_NON_VOLATILE |
                    VARIABLE_ATTRIBUTE_RUNTIME_ACCESS |
                    VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS))) {

            status = GetLastError();
        }

        wprintf(L"[%d] Delete variable status = 0x%x\n", i, status);
        if (status != NO_ERROR) {
            break;
        }

        BOOL isOK = GetFirmwareEnvironmentVariableEx(
            testVarName,
            testGUIDString,
            &obsData,
            dataSize,
            nullptr);

        //
        // Get variable here should fail returning 0.
        //
        wprintf(L"[%d] Get deleted variable status (should fail) = 0x%x\n",
            i,
            isOK);

        if (isOK) {
            status = ERROR_INVALID_STATE;
            break;
        }
    }

    return status;
}

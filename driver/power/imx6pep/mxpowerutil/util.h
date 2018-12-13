// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _MXPOWERUTIL_UTIL_H_
#define _MXPOWERUTIL_UTIL_H_

#include <strsafe.h>
#include <memory>

class wexception {
public:
    explicit wexception (_In_range_(<, 0) HRESULT Hr) noexcept : hr(Hr) {}

    wexception (wexception&& other) noexcept :
        hr(other.hr),
        msg(std::move(other.msg))
    {}

    static wexception make (
        HRESULT HResult,
        _In_ _Printf_format_string_ PCWSTR Format,
        ...
        ) noexcept
    {
        wexception ex(HResult);

        enum : ULONG { BUF_LEN = 512 };
        std::unique_ptr<WCHAR[]> buf(new (std::nothrow) WCHAR[BUF_LEN]);
        if (!buf) {
            return ex;
        }

        va_list argList;
        va_start(argList, Format);

        HRESULT printfHr = StringCchVPrintfW(
            buf.get(),
            BUF_LEN,
            Format,
            argList);

        va_end(argList);

        if (SUCCEEDED(printfHr)) {
            ex.msg = std::move(buf);
        }

        return ex;
    }

    const wchar_t* wwhat () const
    {
        return msg ? msg.get() : L"";
    }

    HRESULT HResult () const
    {
        return this->hr;
    }

    // disable copy
    // wexception (const wexception&) = delete;
    wexception& operator= (const wexception&) = delete;

private:
    HRESULT hr;
    std::unique_ptr<WCHAR[]> msg;
};

#endif // _MXPOWERUTIL_UTIL_H_
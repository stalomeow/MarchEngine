#pragma once

#include <stdint.h>
#include <string>
#include <fmt/core.h>
#include <fmt/xchar.h>
#include <fmt/chrono.h>

namespace march::StringUtils
{
    // size does not include the null terminator
    std::string Utf16ToBytes(const wchar_t* s, int32_t size, uint32_t codePage);

    // size does not include the null terminator
    std::string Utf16ToUtf8(const wchar_t* s, int32_t size);

    std::string Utf16ToUtf8(const std::wstring& s);

    // size does not include the null terminator
    std::string Utf16ToAnsi(const wchar_t* s, int32_t size);

    std::string Utf16ToAnsi(const std::wstring& s);

    // size does not include the null terminator
    std::wstring Utf8ToUtf16(const char* s, int32_t size);

    std::wstring Utf8ToUtf16(const std::string& s);

    std::string Utf8ToAnsi(const std::string& s);

    template <typename... _Args>
    __forceinline std::string Format(const std::string& format, _Args&&... args)
    {
        return ::fmt::format(format, std::forward<_Args>(args)...);
    }

    template <typename... _Args>
    __forceinline std::wstring Format(const std::wstring& format, _Args&&... args)
    {
        return ::fmt::format(format, std::forward<_Args>(args)...);
    }
}

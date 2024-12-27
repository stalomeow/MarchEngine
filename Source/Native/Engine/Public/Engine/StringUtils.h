#pragma once

#include <stdint.h>
#include <string>

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
}

#include "StringUtils.h"
#include <Windows.h>

namespace march::StringUtils
{
    std::string Utf16ToBytes(const wchar_t* s, int32_t size, uint32_t codePage)
    {
        // https://learn.microsoft.com/en-us/cpp/cpp/char-wchar-t-char16-t-char32-t?view=msvc-170
        // The wchar_t type is an implementation-defined wide character type.
        // In the Microsoft compiler, it represents a 16-bit wide character used to store Unicode encoded as UTF-16LE,
        // the native character type on Windows operating systems.

        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte

        int len = WideCharToMultiByte(static_cast<UINT>(codePage), 0, s, size, nullptr, 0, nullptr, nullptr);
        if (len == 0) { return ""; }

        std::string result(len, 0);
        WideCharToMultiByte(static_cast<UINT>(codePage), 0, s, size, result.data(), len, nullptr, nullptr);
        return result;
    }

    std::string Utf16ToUtf8(const wchar_t* s, int32_t size)
    {
        return Utf16ToBytes(s, size, CP_UTF8);
    }

    std::string Utf16ToUtf8(const std::wstring& s)
    {
        return Utf16ToUtf8(s.c_str(), static_cast<int32_t>(s.size()));
    }

    std::string Utf16ToAnsi(const wchar_t* s, int32_t size)
    {
        // the default locale/codepage for my system
        return Utf16ToBytes(s, size, CP_ACP);
    }

    std::string Utf16ToAnsi(const std::wstring& s)
    {
        return Utf16ToAnsi(s.c_str(), static_cast<int32_t>(s.size()));
    }

    std::wstring Utf8ToUtf16(const char* s, int32_t size)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar

        int len = MultiByteToWideChar(CP_UTF8, 0, s, size, nullptr, 0);
        if (len == 0) { return L""; }

        std::wstring result(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, s, size, result.data(), len);
        return result;
    }

    std::wstring Utf8ToUtf16(const std::string& s)
    {
        return Utf8ToUtf16(s.c_str(), static_cast<int32_t>(s.size()));
    }

    std::string Utf8ToAnsi(const std::string& s)
    {
        return Utf16ToAnsi(Utf8ToUtf16(s));
    }
}

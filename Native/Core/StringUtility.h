#pragma once

#include <string>
#include <Windows.h>
#include <stdint.h>

namespace dx12demo::StringUtility
{
    // size does not include the null terminator
    inline std::string Utf16ToBytes(const wchar_t* s, int32_t size, UINT codePage)
    {
        // https://learn.microsoft.com/en-us/cpp/cpp/char-wchar-t-char16-t-char32-t?view=msvc-170
        // The wchar_t type is an implementation-defined wide character type.
        // In the Microsoft compiler, it represents a 16-bit wide character used to store Unicode encoded as UTF-16LE,
        // the native character type on Windows operating systems.

        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte

        int len = WideCharToMultiByte(codePage, 0, s, size, nullptr, 0, nullptr, nullptr);
        if (len == 0) { return ""; }

        std::string result(len, 0);
        WideCharToMultiByte(codePage, 0, s, size, result.data(), len, nullptr, nullptr);
        return std::move(result);
    }

    // size does not include the null terminator
    inline std::string Utf16ToUtf8(const wchar_t* s, int32_t size)
    {
        return Utf16ToBytes(s, size, CP_UTF8);
    }

    inline std::string Utf16ToUtf8(const std::wstring& s)
    {
        return Utf16ToUtf8(s.c_str(), static_cast<int32_t>(s.size()));
    }

    // size does not include the null terminator
    inline std::string Utf16ToAnsi(const wchar_t* s, int32_t size)
    {
        // the default locale/codepage for my system
        return Utf16ToBytes(s, size, CP_ACP);
    }

    inline std::string Utf16ToAnsi(const std::wstring& s)
    {
        return Utf16ToAnsi(s.c_str(), static_cast<int32_t>(s.size()));
    }

    // size does not include the null terminator
    inline std::wstring Utf8ToUtf16(const char* s, int32_t size)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar

        int len = MultiByteToWideChar(CP_UTF8, 0, s, size, nullptr, 0);
        if (len == 0) { return L""; }

        std::wstring result(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, s, size, result.data(), len);
        return std::move(result);
    }

    inline std::wstring Utf8ToUtf16(const std::string& s)
    {
        return Utf8ToUtf16(s.c_str(), static_cast<int32_t>(s.size()));
    }

    inline std::string Utf8ToAnsi(const std::string& s)
    {
        return Utf16ToAnsi(Utf8ToUtf16(s));
    }

    template<typename ... Args>
    inline std::wstring Format(const std::wstring& format, Args ... args)
    {
        // https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

        int size = std::swprintf(nullptr, 0, format.c_str(), args ...);
        if (size < 0) { throw std::exception("Error during formatting."); }

        std::wstring result(size, 0);
        std::swprintf(result.data(), static_cast<size_t>(size) + 1, format.c_str(), args ...);
        return std::move(result);
    }

    template<typename ... Args>
    inline std::string Format(const std::string& format, Args ... args)
    {
        // https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

        int size = std::snprintf(nullptr, 0, format.c_str(), args ...);
        if (size < 0) { throw std::exception("Error during formatting."); }

        std::string result(size, 0);
        std::snprintf(result.data(), static_cast<size_t>(size) + 1, format.c_str(), args ...);
        return std::move(result);
    }
}

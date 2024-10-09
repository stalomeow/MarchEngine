#pragma once

#include <exception>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <type_traits>

namespace march::StringUtility
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

    template<typename ... Args>
    std::wstring Format(const std::wstring& format, Args ... args)
    {
        // https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

        int size = std::swprintf(nullptr, 0, format.c_str(), args ...);
        if (size < 0) { throw std::exception("Error during formatting."); }

        std::wstring result(size, 0);
        std::swprintf(result.data(), static_cast<size_t>(size) + 1, format.c_str(), args ...);
        return std::move(result);
    }

    template<typename ... Args>
    std::string Format(const std::string& format, Args ... args)
    {
        // https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

        int size = std::snprintf(nullptr, 0, format.c_str(), args ...);
        if (size < 0) { throw std::exception("Error during formatting."); }

        std::string result(size, 0);
        std::snprintf(result.data(), static_cast<size_t>(size) + 1, format.c_str(), args ...);
        return std::move(result);
    }
}

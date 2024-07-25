#pragma once

#include <string>
#include <codecvt>

namespace dx12demo::StringUtility
{
    inline std::string WToUtf8(const std::wstring& s)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(s);
    }

    template<typename ... Args>
    inline std::wstring Format(const std::wstring& format, Args ... args)
    {
        // https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

        int size_s = std::swprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if (size_s <= 0) { throw std::exception("Error during formatting."); }
        auto size = static_cast<size_t>(size_s);
        auto buf = std::make_unique<wchar_t[]>(size);
        std::swprintf(buf.get(), size, format.c_str(), args ...);
        return std::wstring(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    template<typename ... Args>
    inline std::string Format(const std::string& format, Args ... args)
    {
        // https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

        int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if (size_s <= 0) { throw std::exception("Error during formatting."); }
        auto size = static_cast<size_t>(size_s);
        auto buf = std::make_unique<char[]>(size);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }
}

#pragma once

#include <string>
#include <string_view>
#include <utf8.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

namespace march
{
    struct StringUtils
    {
        static std::string Utf16ToUtf8(std::u16string_view s)
        {
            return utf8::utf16to8(s);
        }

        static std::u16string Utf8ToUtf16(std::string_view s)
        {
            return utf8::utf8to16(s);
        }

        template <typename... Args>
        static std::string Format(::fmt::format_string<Args...> format, Args&&... args)
        {
            return ::fmt::format(format, std::forward<Args>(args)...);
        }

        template <typename T>
        static std::string ToString(const T& value)
        {
            return ::fmt::to_string(value);
        }

        static std::string FormatSize(size_t sizeInBytes)
        {
            if (sizeInBytes < 1024)
                return Format("{} B", sizeInBytes);
            else if (sizeInBytes < 1024 * 1024)
                return Format("{:.2f} KB", sizeInBytes / 1024.0);
            else if (sizeInBytes < 1024 * 1024 * 1024)
                return Format("{:.2f} MB", sizeInBytes / (1024.0 * 1024.0));
            else
                return Format("{:.2f} GB", sizeInBytes / (1024.0 * 1024.0 * 1024.0));
        }
    };
}

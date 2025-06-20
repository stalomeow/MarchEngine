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
    };
}

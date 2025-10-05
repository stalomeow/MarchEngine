#pragma once

#include "Engine/Memory/MemoryManager.h"
#include "Engine/STL/Core.h"
#include <string>
#include <string_view>
#include <utf8.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <iterator>

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

        static stl::string Utf16ToUtf8(MemoryLabel label, std::u16string_view s)
        {
            stl::string result(label);
            utf8::utf16to8(s.begin(), s.end(), std::back_inserter(result));
            return result;
        }

        static stl::u16string Utf8ToUtf16(MemoryLabel label, std::string_view s)
        {
            stl::u16string result(label);
            utf8::utf8to16(s.begin(), s.end(), std::back_inserter(result));
            return result;
        }

        // Ref: https://fmt.dev/11.2/api/#custom-allocators
        using fmt_memory_buffer = ::fmt::basic_memory_buffer<char, ::fmt::inline_buffer_size, stl::allocator<char>>;

        template <typename... Args>
        static std::string Format(::fmt::format_string<Args...> format, Args&&... args)
        {
            return ::fmt::format(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static stl::string Format(MemoryLabel label, ::fmt::string_view format, const Args&... args)
        {
            fmt_memory_buffer buffer(MemoryLabel::Temp);
            ::fmt::vformat_to(std::back_inserter(buffer), format, ::fmt::make_format_args(args...));
            return stl::string(buffer.data(), buffer.size(), label);
        }

        template <typename T>
        static stl::string ToString(MemoryLabel label, const T& value)
        {
            return Format(label, "{}", value);
        }

        static stl::string FormatSize(MemoryLabel label, size_t sizeInBytes)
        {
            if (sizeInBytes < 1024)
                return Format(label, "{} B", sizeInBytes);
            else if (sizeInBytes < 1024 * 1024)
                return Format(label, "{:.2f} KB", sizeInBytes / 1024.0);
            else if (sizeInBytes < 1024 * 1024 * 1024)
                return Format(label, "{:.2f} MB", sizeInBytes / (1024.0 * 1024.0));
            else
                return Format(label, "{:.2f} GB", sizeInBytes / (1024.0 * 1024.0 * 1024.0));
        }
    };
}

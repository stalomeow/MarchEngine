#pragma once

#include "Engine/StringUtils.h"
#include <string>
#include <time.h>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <deque>
#include <functional>
#include <fmt/core.h>
#include <fmt/xchar.h>

namespace march
{
    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
    };

    struct LogStackFrame
    {
        std::string Function{};
        std::string Filename{};
        int32_t Line{};
    };

    struct LogEntry
    {
        LogLevel Level{};
        time_t Time{};
        std::string Message{};
        std::vector<LogStackFrame> StackTrace{};
    };

    class Log
    {
    public:
        static LogLevel GetMinimumLevel();
        static void SetMinimumLevel(LogLevel level);
        static bool IsLevelEnabled(LogLevel level);

        static uint32_t GetCount(LogLevel level);
        static void Clear();
        static void ForEach(const std::function<void(int32_t, const LogEntry&)>& action);
        static bool ReadAt(int32_t i, const std::function<void(const LogEntry&)>& action);
        static bool ReadLast(const std::function<void(const LogEntry&)>& action);

        static void Message(LogLevel level, std::string&& message, std::vector<LogStackFrame>&& stackTrace);
        static void Message(LogLevel level, const std::string& message, std::vector<LogStackFrame>&& stackTrace);
        static void Message(LogLevel level, const std::wstring& message, std::vector<LogStackFrame>&& stackTrace);

    private:
        static LogLevel s_MinimumLevel;
        static std::deque<LogEntry> s_Entries;
        static uint32_t s_Counts[static_cast<int32_t>(LogLevel::Error) + 1];
        static std::mutex s_Mutex;
    };
}

#define LOG_MSG(level, f, ...) \
    if (::march::Log::IsLevelEnabled(level)) \
        ::march::Log::Message(level, ::fmt::format(f, __VA_ARGS__), { { __FUNCSIG__, __FILE__, __LINE__ } })

#define LOG_TRACE(format, ...)   LOG_MSG(::march::LogLevel::Trace, format, __VA_ARGS__)
#define LOG_DEBUG(format, ...)   LOG_MSG(::march::LogLevel::Debug, format, __VA_ARGS__)
#define LOG_INFO(format, ...)    LOG_MSG(::march::LogLevel::Info, format, __VA_ARGS__)
#define LOG_WARNING(format, ...) LOG_MSG(::march::LogLevel::Warning, format, __VA_ARGS__)
#define LOG_ERROR(format, ...)   LOG_MSG(::march::LogLevel::Error, format, __VA_ARGS__)

// https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
#ifdef _DEBUG
    #include <crtdbg.h>
    #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the allocations to be of _CLIENT_BLOCK type
#else
    #define DBG_NEW new
#endif

#pragma once

#include "Engine/STL/Core.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Memory/MemoryManager.h"
#include <time.h>
#include <stdint.h>
#include <mutex>
#include <functional>

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
        stl::string Function{};
        stl::string Filename{};
        int32_t Line{};
    };

    struct LogEntry
    {
        LogLevel Level{};
        time_t Time{};
        stl::string Message{};
        stl::vector<LogStackFrame> StackTrace{};
    };

    struct Log
    {
        static LogLevel GetMinimumLevel();
        static void SetMinimumLevel(LogLevel level);
        static bool IsLevelEnabled(LogLevel level);

        static uint32_t GetCount(LogLevel level);
        static void Clear();
        static void ForEach(const std::function<void(int32_t, const LogEntry&)>& action);
        static bool ReadAt(int32_t i, const std::function<void(const LogEntry&)>& action);
        static bool ReadLast(const std::function<void(const LogEntry&)>& action);

        static void Message(LogLevel level, stl::string&& message, const char* func, const char* file, int32_t line);
        static void Message(LogLevel level, const stl::string& message, const char* func, const char* file, int32_t line);
        static void Message(LogLevel level, stl::string&& message, stl::vector<LogStackFrame>&& stackTrace);
        static void Message(LogLevel level, const stl::string& message, stl::vector<LogStackFrame>&& stackTrace);
    };
}

#define LOG_MSG(level, f, ...) \
    if (::march::Log::IsLevelEnabled(level)) \
        ::march::Log::Message(level, ::march::StringUtils::Format(::march::MemoryLabel::Debug, f, __VA_ARGS__), __FUNCSIG__, __FILE__, __LINE__)

#define LOG_TRACE(format, ...)   LOG_MSG(::march::LogLevel::Trace, format, __VA_ARGS__)
#define LOG_DEBUG(format, ...)   LOG_MSG(::march::LogLevel::Debug, format, __VA_ARGS__)
#define LOG_INFO(format, ...)    LOG_MSG(::march::LogLevel::Info, format, __VA_ARGS__)
#define LOG_WARNING(format, ...) LOG_MSG(::march::LogLevel::Warning, format, __VA_ARGS__)
#define LOG_ERROR(format, ...)   LOG_MSG(::march::LogLevel::Error, format, __VA_ARGS__)

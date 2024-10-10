#pragma once

#include "StringUtility.h"
#include <string>
#include <deque>
#include <ctime>
#include <exception>
#include <vector>
#include <stdint.h>
#include <mutex>

namespace march
{
    class ConsoleWindow;

    enum class LogType
    {
        Info,
        Warn,
        Error,
        NumLogType
    };

    struct LogStackFrame
    {
        std::string Function;
        std::string Filename;
        int Line;
    };

    struct LogEntry
    {
        std::string Message;
        LogType Type;
        time_t Time;
        std::vector<LogStackFrame> StackTrace;
    };

    class Debug
    {
        friend ConsoleWindow;

    public:
        template<typename ... Args>
        static void Info(std::vector<LogStackFrame>&& stackTrace, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(std::move(stackTrace), message, LogType::Info);
        }

        template<typename ... Args>
        static void Info(std::vector<LogStackFrame>&& stackTrace, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(std::move(stackTrace), message, LogType::Info);
        }

        template<typename ... Args>
        static void Warn(std::vector<LogStackFrame>&& stackTrace, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(std::move(stackTrace), message, LogType::Warn);
        }

        template<typename ... Args>
        static void Warn(std::vector<LogStackFrame>&& stackTrace, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(std::move(stackTrace), message, LogType::Warn);
        }

        template<typename ... Args>
        static void Error(std::vector<LogStackFrame>&& stackTrace, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(std::move(stackTrace), message, LogType::Error);
        }

        template<typename ... Args>
        static void Error(std::vector<LogStackFrame>&& stackTrace, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(std::move(stackTrace), message, LogType::Error);
        }

        static void AddLog(std::vector<LogStackFrame>&& stackTrace, const std::wstring& message, LogType type);
        static void AddLog(std::vector<LogStackFrame>&& stackTrace, const std::string& message, LogType type);

    private:
        static int GetLogCount(LogType type);
        static std::string GetTypePrefix(LogType type);
        static std::string GetTimePrefix(time_t t);
        static void ClearLogs();

        static std::deque<LogEntry> s_Logs;
        static uint32_t s_LogCounts[static_cast<int>(LogType::NumLogType)];
        static std::mutex s_Mutex;
    };
}

#define DEBUG_LOG_INFO(message, ...)  march::Debug::Info ({ { __FUNCSIG__, __FILE__, __LINE__ } }, (message), __VA_ARGS__)
#define DEBUG_LOG_WARN(message, ...)  march::Debug::Warn ({ { __FUNCSIG__, __FILE__, __LINE__ } }, (message), __VA_ARGS__)
#define DEBUG_LOG_ERROR(message, ...) march::Debug::Error({ { __FUNCSIG__, __FILE__, __LINE__ } }, (message), __VA_ARGS__)

// https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
#ifdef _DEBUG
    #include <crtdbg.h>
    #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the allocations to be of _CLIENT_BLOCK type
#else
    #define DBG_NEW new
#endif

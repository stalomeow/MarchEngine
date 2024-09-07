#pragma once

#include "Core/StringUtility.h"
#include <string>
#include <deque>
#include <ctime>
#include <exception>
#include <vector>
#include <unordered_map>

namespace dx12demo
{
    enum class LogType
    {
        Info,
        Warn,
        Error
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
        friend class GameEditor;

    public:
        template<typename ... Args>
        static void Info(const std::vector<LogStackFrame>& stackTrace, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(stackTrace, message, LogType::Info);
        }

        template<typename ... Args>
        static void Info(const std::vector<LogStackFrame>& stackTrace, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(stackTrace, message, LogType::Info);
        }

        template<typename ... Args>
        static void Warn(const std::vector<LogStackFrame>& stackTrace, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(stackTrace, message, LogType::Warn);
        }

        template<typename ... Args>
        static void Warn(const std::vector<LogStackFrame>& stackTrace, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(stackTrace, message, LogType::Warn);
        }

        template<typename ... Args>
        static void Error(const std::vector<LogStackFrame>& stackTrace, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(stackTrace, message, LogType::Error);
        }

        template<typename ... Args>
        static void Error(const std::vector<LogStackFrame>& stackTrace, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(stackTrace, message, LogType::Error);
        }

        static void AddLog(const std::vector<LogStackFrame>& stackTrace, const std::wstring& message, LogType type);
        static void AddLog(const std::vector<LogStackFrame>& stackTrace, const std::string& message, LogType type);

    private:
        static int GetLogCount(LogType type);
        static std::string Debug::GetTypePrefix(LogType type);
        static std::string GetTimePrefix(time_t t);
        static void ClearLogs();

        static std::deque<LogEntry> s_Logs;
        static std::unordered_map<LogType, int> s_LogCounts;
    };
}

#define DEBUG_LOG_INFO(message, ...)  dx12demo::Debug::Info ({ { __FUNCSIG__, __FILE__, __LINE__ } }, (message), __VA_ARGS__)
#define DEBUG_LOG_WARN(message, ...)  dx12demo::Debug::Warn ({ { __FUNCSIG__, __FILE__, __LINE__ } }, (message), __VA_ARGS__)
#define DEBUG_LOG_ERROR(message, ...) dx12demo::Debug::Error({ { __FUNCSIG__, __FILE__, __LINE__ } }, (message), __VA_ARGS__)

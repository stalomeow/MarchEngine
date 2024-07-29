#pragma once

#include "Core/StringUtility.h"
#include <string>
#include <deque>
#include <ctime>
#include <exception>
#include <unordered_map>

namespace dx12demo
{
    class Debug
    {
        friend class GameEditor;

    public:
        template<typename ... Args>
        static void Info(const std::string& file, int line, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(file, line, message, LogType::Info);
        }

        template<typename ... Args>
        static void Info(const std::string& file, int line, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(file, line, message, LogType::Info);
        }

        template<typename ... Args>
        static void Warn(const std::string& file, int line, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(file, line, message, LogType::Warn);
        }

        template<typename ... Args>
        static void Warn(const std::string& file, int line, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(file, line, message, LogType::Warn);
        }

        template<typename ... Args>
        static void Error(const std::string& file, int line, const std::wstring& format, Args ... args)
        {
            std::wstring message = StringUtility::Format(format, args ...);
            AddLog(file, line, message, LogType::Error);
        }

        template<typename ... Args>
        static void Error(const std::string& file, int line, const std::string& format, Args ... args)
        {
            std::string message = StringUtility::Format(format, args ...);
            AddLog(file, line, message, LogType::Error);
        }

    private:
        enum class LogType
        {
            Info,
            Warn,
            Error
        };

        struct LogEntry
        {
            std::string Message;
            LogType Type;
            time_t Time;
            std::string File;
            int Line;
        };

        static void AddLog(const std::string& file, int line, const std::wstring& message, LogType type);
        static void AddLog(const std::string& file, int line, const std::string& message, LogType type);
        static int GetLogCount(LogType type);
        static std::string Debug::GetTypePrefix(Debug::LogType type);
        static std::string GetTimePrefix(time_t t);
        static void ClearLogs();

        static std::deque<LogEntry> s_Logs;
        static std::unordered_map<LogType, int> s_LogCounts;
    };
}

#define DEBUG_LOG_INFO(message, ...)  dx12demo::Debug::Info(__FILE__, __LINE__, (message), __VA_ARGS__)
#define DEBUG_LOG_WARN(message, ...)  dx12demo::Debug::Warn(__FILE__, __LINE__, (message), __VA_ARGS__)
#define DEBUG_LOG_ERROR(message, ...) dx12demo::Debug::Error(__FILE__, __LINE__, (message), __VA_ARGS__)

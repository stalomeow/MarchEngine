#pragma once

#include "Core/StringUtility.h"
#include "Scripting/ScriptTypes.h"
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

    namespace binding
    {
        CSHARP_API(void) Debug_Info(CSharpInt stackTraceFrameCount, CSharpString* methods, CSharpString* filenames, CSharpInt* lines, CSharpString message);
        CSHARP_API(void) Debug_Warn(CSharpInt stackTraceFrameCount, CSharpString* methods, CSharpString* filenames, CSharpInt* lines, CSharpString message);
        CSHARP_API(void) Debug_Error(CSharpInt stackTraceFrameCount, CSharpString* methods, CSharpString* filenames, CSharpInt* lines, CSharpString message);
    }

    class Debug
    {
        friend class GameEditor;
        friend CSHARP_API(void) binding::Debug_Info(CSharpInt stackTraceFrameCount, CSharpString* methods, CSharpString* filenames, CSharpInt* lines, CSharpString message);
        friend CSHARP_API(void) binding::Debug_Warn(CSharpInt stackTraceFrameCount, CSharpString* methods, CSharpString* filenames, CSharpInt* lines, CSharpString message);
        friend CSHARP_API(void) binding::Debug_Error(CSharpInt stackTraceFrameCount, CSharpString* methods, CSharpString* filenames, CSharpInt* lines, CSharpString message);

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

    private:
        static void AddLog(const std::vector<LogStackFrame>& stackTrace, const std::wstring& message, LogType type);
        static void AddLog(const std::vector<LogStackFrame>& stackTrace, const std::string& message, LogType type);
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

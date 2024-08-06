#include "Debug.h"
#include <memory>
#include <Windows.h>

namespace dx12demo
{
    std::deque<LogEntry> Debug::s_Logs{};
    std::unordered_map<LogType, int> Debug::s_LogCounts{};

    void Debug::AddLog(const std::vector<LogStackFrame>& stackTrace, const std::wstring& message, LogType type)
    {
        //while (s_Logs.size() > 2000)
        //{
        //    s_LogCounts[s_Logs.front().Type]--;
        //    s_Logs.pop_front();
        //}

        LogEntry entry = {};
        entry.Type = type;
        entry.Time = time(NULL);
        entry.Message = StringUtility::Utf16ToUtf8(message); // imgui 要求 utf8
        entry.StackTrace = stackTrace;
        s_LogCounts[entry.Type]++;
        s_Logs.push_back(entry);

#if defined(DEBUG) || defined(_DEBUG)
        OutputDebugStringA((
            GetTimePrefix(entry.Time) + " " +
            GetTypePrefix(entry.Type) + " " +
            StringUtility::Utf16ToAnsi(message) + // 转为 ansi 防止乱码
            "\n").c_str());
#endif
    }

    void Debug::AddLog(const std::vector<LogStackFrame>& stackTrace, const std::string& message, LogType type)
    {
        //while (s_Logs.size() > 2000)
        //{
        //    s_LogCounts[s_Logs.front().Type]--;
        //    s_Logs.pop_front();
        //}

        LogEntry entry = {};
        entry.Type = type;
        entry.Time = time(NULL);
        entry.Message = message; // imgui 要求 utf8
        entry.StackTrace = stackTrace;
        s_LogCounts[entry.Type]++;
        s_Logs.push_back(entry);

#if defined(DEBUG) || defined(_DEBUG)
        OutputDebugStringA((
            GetTimePrefix(entry.Time) + " " +
            GetTypePrefix(entry.Type) + " " +
            StringUtility::Utf8ToAnsi(message) + // 转为 ansi 防止乱码
            "\n").c_str());
#endif
    }

    int Debug::GetLogCount(LogType type)
    {
        return s_LogCounts[type];
    }

    std::string Debug::GetTimePrefix(time_t t)
    {
        char tmp[32] = { 0 };
        strftime(tmp, sizeof(tmp), "[%H:%M:%S]", localtime(&t));
        return std::string(tmp);
    }

    std::string Debug::GetTypePrefix(LogType type)
    {
        switch (type)
        {
        case dx12demo::LogType::Info: return "INFO";
        case dx12demo::LogType::Warn: return "WARN";
        case dx12demo::LogType::Error: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    void Debug::ClearLogs()
    {
        s_Logs.clear();
        s_LogCounts.clear();
    }

    namespace binding
    {
        CSHARP_API(void) Debug_Info(CSharpString message, CSharpLogStackFrame* pFrames, CSharpInt framCount)
        {
            std::vector<LogStackFrame> stackTrace;

            for (CSharpInt i = 0; i < framCount; i++)
            {
                stackTrace.push_back({ CSharpString_ToUtf8(pFrames[i].MethodName), CSharpString_ToUtf8(pFrames[i].Filename), pFrames[i].Line });
            }

            Debug::AddLog(stackTrace, CSharpString_ToUtf8(message), LogType::Info);
        }

        CSHARP_API(void) Debug_Warn(CSharpString message, CSharpLogStackFrame* pFrames, CSharpInt framCount)
        {
            std::vector<LogStackFrame> stackTrace;

            for (CSharpInt i = 0; i < framCount; i++)
            {
                stackTrace.push_back({ CSharpString_ToUtf8(pFrames[i].MethodName), CSharpString_ToUtf8(pFrames[i].Filename), pFrames[i].Line });
            }

            Debug::AddLog(stackTrace, CSharpString_ToUtf8(message), LogType::Warn);
        }

        CSHARP_API(void) Debug_Error(CSharpString message, CSharpLogStackFrame* pFrames, CSharpInt framCount)
        {
            std::vector<LogStackFrame> stackTrace;

            for (CSharpInt i = 0; i < framCount; i++)
            {
                stackTrace.push_back({ CSharpString_ToUtf8(pFrames[i].MethodName), CSharpString_ToUtf8(pFrames[i].Filename), pFrames[i].Line });
            }

            Debug::AddLog(stackTrace, CSharpString_ToUtf8(message), LogType::Error);
        }
    }
}

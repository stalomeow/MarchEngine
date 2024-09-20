#include "Debug.h"
#include <memory>
#include <Windows.h>

namespace march
{
    std::deque<LogEntry> Debug::s_Logs{};
    uint32_t Debug::s_LogCounts[]{};
    std::mutex Debug::s_Mutex{};

    void Debug::AddLog(const std::vector<LogStackFrame>& stackTrace, const std::wstring& message, LogType type)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

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
        s_LogCounts[static_cast<int>(entry.Type)]++;
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
        std::lock_guard<std::mutex> lock(s_Mutex);

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
        s_LogCounts[static_cast<int>(entry.Type)]++;
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
        std::lock_guard<std::mutex> lock(s_Mutex);

        return static_cast<int>(s_LogCounts[static_cast<int>(type)]);
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
        case march::LogType::Info: return "INFO";
        case march::LogType::Warn: return "WARN";
        case march::LogType::Error: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    void Debug::ClearLogs()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        s_Logs.clear();
        ZeroMemory(s_LogCounts, sizeof(s_LogCounts));
    }
}

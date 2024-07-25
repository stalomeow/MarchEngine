#include "Debug.h"
#include <memory>

namespace dx12demo
{
    std::deque<Debug::LogEntry> Debug::s_Logs{};
    std::unordered_map<Debug::LogType, int> Debug::s_LogCounts{};

    void Debug::AddLog(const std::string& file, int line, const std::wstring& message, LogType type)
    {
        while (s_Logs.size() > 2000)
        {
            s_LogCounts[s_Logs.front().Type]--;
            s_Logs.pop_front();
        }

        LogEntry entry = {};
        entry.Type = type;
        entry.Time = time(NULL);
        entry.Message = StringUtility::WToUtf8(message);
        entry.File = file;
        entry.Line = line;
        s_LogCounts[entry.Type]++;
        s_Logs.push_back(entry);
    }

    void Debug::AddLog(const std::string& file, int line, const std::string& message, LogType type)
    {
        while (s_Logs.size() > 2000)
        {
            s_LogCounts[s_Logs.front().Type]--;
            s_Logs.pop_front();
        }

        LogEntry entry = {};
        entry.Type = type;
        entry.Time = time(NULL);
        entry.Message = message;
        entry.File = file;
        entry.Line = line;
        s_LogCounts[entry.Type]++;
        s_Logs.push_back(entry);
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

    std::string Debug::GetTypePrefix(Debug::LogType type)
    {
        switch (type)
        {
        case dx12demo::Debug::LogType::Info: return "INFO";
        case dx12demo::Debug::LogType::Warn: return "WARN";
        case dx12demo::Debug::LogType::Error: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    void Debug::ClearLogs()
    {
        s_Logs.clear();
        s_LogCounts.clear();
    }
}

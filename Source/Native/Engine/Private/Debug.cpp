#include "pch.h"
#include "Engine/Debug.h"
#include <Windows.h>

namespace march
{
    LogLevel                Log::s_MinimumLevel = LogLevel::Trace;
    std::deque<LogEntry>    Log::s_Entries{};
    uint32_t                Log::s_Counts[]{};
    std::mutex              Log::s_Mutex{};

    LogLevel Log::GetMinimumLevel()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        return s_MinimumLevel;
    }

    void Log::SetMinimumLevel(LogLevel level)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        s_MinimumLevel = level;
    }

    bool Log::IsLevelEnabled(LogLevel level)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        return static_cast<int32_t>(level) >= static_cast<int32_t>(s_MinimumLevel);
    }

    uint32_t Log::GetCount(LogLevel level)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        return s_Counts[static_cast<int32_t>(level)];
    }

    void Log::Clear()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        s_Entries.clear();
        ZeroMemory(s_Counts, sizeof(s_Counts));
    }

    void Log::ForEach(const std::function<void(int32_t, const LogEntry&)>& action)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        for (size_t i = 0; i < s_Entries.size(); i++)
        {
            action(static_cast<int32_t>(i), s_Entries[i]);
        }
    }

    bool Log::ReadAt(int32_t i, const std::function<void(const LogEntry&)>& action)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (i < 0 || i >= s_Entries.size())
        {
            return false;
        }

        action(s_Entries[static_cast<size_t>(i)]);
        return true;
    }

    bool Log::ReadLast(const std::function<void(const LogEntry&)>& action)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (s_Entries.empty())
        {
            return false;
        }

        action(s_Entries.back());
        return true;
    }

    void Log::Message(LogLevel level, std::string&& message, std::vector<LogStackFrame>&& stackTrace)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (static_cast<int32_t>(level) < static_cast<int32_t>(s_MinimumLevel))
        {
            return;
        }

        while (s_Entries.size() > 9999)
        {
            s_Counts[static_cast<int32_t>(s_Entries.front().Level)]--;
            s_Entries.pop_front();
        }

        LogEntry& entry = s_Entries.emplace_back();
        s_Counts[static_cast<int32_t>(level)]++;

        entry.Level = level;
        entry.Time = time(NULL);
        entry.Message = std::move(message);
        entry.StackTrace = std::move(stackTrace);
    }

    void Log::Message(LogLevel level, const std::string& message, std::vector<LogStackFrame>&& stackTrace)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (static_cast<int32_t>(level) < static_cast<int32_t>(s_MinimumLevel))
        {
            return;
        }

        while (s_Entries.size() > 9999)
        {
            s_Counts[static_cast<int32_t>(s_Entries.front().Level)]--;
            s_Entries.pop_front();
        }

        LogEntry& entry = s_Entries.emplace_back();
        s_Counts[static_cast<int32_t>(level)]++;

        entry.Level = level;
        entry.Time = time(NULL);
        entry.Message = message;
        entry.StackTrace = std::move(stackTrace);
    }

    void Log::Message(LogLevel level, const std::wstring& message, std::vector<LogStackFrame>&& stackTrace)
    {
        Message(level, StringUtils::Utf16ToUtf8(message), std::move(stackTrace));
    }
}

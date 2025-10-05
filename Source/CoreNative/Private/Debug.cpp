#include "pch.h"
#include "Engine/Debug.h"
#include "Engine/Misc/StringUtils.h"
#include <Windows.h>

namespace march
{
    static LogLevel             g_MinimumLevel = LogLevel::Trace;
    static stl::deque<LogEntry> g_Entries(MemoryLabel::Debug);
    static uint32_t             g_Counts[static_cast<size_t>(LogLevel::Error) + 1]{};
    static std::mutex           g_Mutex{};

    LogLevel Log::GetMinimumLevel()
    {
        std::lock_guard lock(g_Mutex);

        return g_MinimumLevel;
    }

    void Log::SetMinimumLevel(LogLevel level)
    {
        std::lock_guard lock(g_Mutex);

        g_MinimumLevel = level;
    }

    bool Log::IsLevelEnabled(LogLevel level)
    {
        std::lock_guard lock(g_Mutex);

        return static_cast<int32_t>(level) >= static_cast<int32_t>(g_MinimumLevel);
    }

    uint32_t Log::GetCount(LogLevel level)
    {
        std::lock_guard lock(g_Mutex);

        return g_Counts[static_cast<int32_t>(level)];
    }

    void Log::Clear()
    {
        std::lock_guard lock(g_Mutex);

        g_Entries.clear();
        ZeroMemory(g_Counts, sizeof(g_Counts));
    }

    void Log::ForEach(const std::function<void(int32_t, const LogEntry&)>& action)
    {
        std::lock_guard lock(g_Mutex);

        for (size_t i = 0; i < g_Entries.size(); i++)
        {
            action(static_cast<int32_t>(i), g_Entries[i]);
        }
    }

    bool Log::ReadAt(int32_t i, const std::function<void(const LogEntry&)>& action)
    {
        std::lock_guard lock(g_Mutex);

        if (i < 0 || static_cast<size_t>(i) >= g_Entries.size())
        {
            return false;
        }

        action(g_Entries[static_cast<size_t>(i)]);
        return true;
    }

    bool Log::ReadLast(const std::function<void(const LogEntry&)>& action)
    {
        std::lock_guard lock(g_Mutex);

        if (g_Entries.empty())
        {
            return false;
        }

        action(g_Entries.back());
        return true;
    }

    void Log::Message(LogLevel level, stl::string&& message, stl::vector<LogStackFrame>&& stackTrace)
    {
        std::lock_guard lock(g_Mutex);

        if (static_cast<int32_t>(level) < static_cast<int32_t>(g_MinimumLevel))
        {
            return;
        }

        while (g_Entries.size() > 9999)
        {
            g_Counts[static_cast<int32_t>(g_Entries.front().Level)]--;
            g_Entries.pop_front();
        }

        LogEntry& entry = g_Entries.emplace_back();
        g_Counts[static_cast<int32_t>(level)]++;

        entry.Level = level;
        entry.Time = time(NULL);
        entry.Message = std::move(message);
        entry.StackTrace = std::move(stackTrace);
    }

    void Log::Message(LogLevel level, const stl::string& message, stl::vector<LogStackFrame>&& stackTrace)
    {
        std::lock_guard lock(g_Mutex);

        if (static_cast<int32_t>(level) < static_cast<int32_t>(g_MinimumLevel))
        {
            return;
        }

        while (g_Entries.size() > 9999)
        {
            g_Counts[static_cast<int32_t>(g_Entries.front().Level)]--;
            g_Entries.pop_front();
        }

        LogEntry& entry = g_Entries.emplace_back();
        g_Counts[static_cast<int32_t>(level)]++;

        entry.Level = level;
        entry.Time = time(NULL);
        entry.Message = message;
        entry.StackTrace = std::move(stackTrace);
    }
}

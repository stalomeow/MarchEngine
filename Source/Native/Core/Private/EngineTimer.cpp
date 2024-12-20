#include "EngineTimer.h"
#include <algorithm>

#undef min
#undef max

namespace march
{
    EngineTimer::EngineTimer()
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        m_Count2Seconds = 1.0 / freq.QuadPart;

        Reset();
    }

    void EngineTimer::Reset()
    {
        m_IsRunning = false;
        m_LastTickTimestamp = 0;
        m_Elapsed = 0;

        m_ElapsedTime = 0;
        m_DeltaTime = 0;

        m_FrameCount = 0;
    }

    void EngineTimer::Start()
    {
        if (m_IsRunning)
        {
            return;
        }

        m_IsRunning = true;
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_LastTickTimestamp));
    }

    void EngineTimer::Restart()
    {
        Reset();
        Start();
    }

    void EngineTimer::Stop()
    {
        m_IsRunning = false;
    }

    bool EngineTimer::Tick()
    {
        if (!m_IsRunning)
        {
            m_DeltaTime = 0;
            return false;
        }

        LONGLONG timestamp;
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&timestamp));

        LONGLONG delta = std::max(timestamp - m_LastTickTimestamp, 0LL);
        m_Elapsed += delta;

        m_ElapsedTime = static_cast<float>(m_Elapsed * m_Count2Seconds);
        m_DeltaTime = static_cast<float>(delta * m_Count2Seconds);
        m_LastTickTimestamp = timestamp;

        m_FrameCount++;
        return true;
    }
}

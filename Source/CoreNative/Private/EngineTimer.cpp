#include "pch.h"
#include "Engine/EngineTimer.h"
#include <algorithm>

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

        m_FPSCounterElapsedTime = 0;
        m_FPSCounterFrameCount = 0;
        m_FPSCounterFPS = 0;
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
            m_FPSCounterFPS = 0;
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
        m_FPSCounterFrameCount++;

        // Compute averages over one second period.
        if ((m_ElapsedTime - m_FPSCounterElapsedTime) >= 1.0f)
        {
            m_FPSCounterFPS = m_FPSCounterFrameCount; // fps = frameCnt / 1

            // Reset for next average.
            m_FPSCounterFrameCount = 0;
            m_FPSCounterElapsedTime += 1.0f;
        }

        return true;
    }
}

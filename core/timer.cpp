#include "timer.h"

dx12demo::Timer::Timer()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    m_Count2Seconds = 1.0 / freq.QuadPart;

    Reset();
}

void dx12demo::Timer::Reset()
{
    m_IsRunning = false;
    m_LastTickTimestamp = 0;
    m_Elapsed = 0;

    m_ElapsedTime = 0;
    m_DeltaTime = 0;
}

void dx12demo::Timer::Start()
{
    if (m_IsRunning)
    {
        return;
    }

    m_IsRunning = true;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_LastTickTimestamp));
}

void dx12demo::Timer::Restart()
{
    Reset();
    Start();
}

void dx12demo::Timer::Stop()
{
    m_IsRunning = false;
}

bool dx12demo::Timer::Tick()
{
    if (!m_IsRunning)
    {
        m_DeltaTime = 0;
        return false;
    }

    LONGLONG timestamp;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&timestamp));

    LONGLONG delta = max(timestamp - m_LastTickTimestamp, 0);
    m_Elapsed += delta;

    m_ElapsedTime = static_cast<float>(m_Elapsed * m_Count2Seconds);
    m_DeltaTime = static_cast<float>(delta * m_Count2Seconds);
    m_LastTickTimestamp = timestamp;
    return true;
}

float dx12demo::Timer::DeltaTime() const
{
    return m_DeltaTime;
}

float dx12demo::Timer::ElapsedTime() const
{
    return m_ElapsedTime;
}

bool dx12demo::Timer::IsRunning() const
{
    return m_IsRunning;
}

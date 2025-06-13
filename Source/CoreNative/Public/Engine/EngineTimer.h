#pragma once

#include <Windows.h>
#include <stdint.h>

namespace march
{
    class EngineTimer
    {
    public:
        EngineTimer();

        void Reset();
        void Start();
        void Restart();
        void Stop();
        bool Tick();

        float GetDeltaTime() const { return m_DeltaTime; }
        float GetElapsedTime() const { return m_ElapsedTime; }
        uint64_t GetFrameCount() const { return m_FrameCount; }
        uint32_t GetFPS() const { return m_FPSCounterFPS; }
        bool GetIsRunning() const { return m_IsRunning; }

    private:
        double m_Count2Seconds;

        bool m_IsRunning;
        LONGLONG m_LastTickTimestamp;
        LONGLONG m_Elapsed;

        float m_ElapsedTime;
        float m_DeltaTime;

        uint64_t m_FrameCount;

        float m_FPSCounterElapsedTime;
        uint32_t m_FPSCounterFrameCount;
        uint32_t m_FPSCounterFPS;
    };
}

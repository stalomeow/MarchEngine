#pragma once

#include <Windows.h>
#include <stdint.h>

namespace march
{
    class GameTimer
    {
    public:
        GameTimer();

        void Reset();
        void Start();
        void Restart();
        void Stop();
        bool Tick();

        float GetDeltaTime() const { return m_DeltaTime; }
        float GetElapsedTime() const { return m_ElapsedTime; }
        uint64_t GetFrameCount() const { return m_FrameCount; }
        bool GetIsRunning() const { return m_IsRunning; }

    private:
        double m_Count2Seconds;

        bool m_IsRunning;
        LONGLONG m_LastTickTimestamp;
        LONGLONG m_Elapsed;

        float m_ElapsedTime;
        float m_DeltaTime;

        uint64_t m_FrameCount;
    };
}

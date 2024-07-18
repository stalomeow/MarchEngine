#pragma once

#include <Windows.h>

namespace dx12demo
{
    class Timer
    {
    public:
        Timer();

        void Reset();
        void Start();
        void Restart();
        void Stop();
        bool Tick();

        float DeltaTime() const;
        float ElapsedTime() const;
        bool IsRunning() const;

    private:
        double m_Count2Seconds;

        bool m_IsRunning;
        LONGLONG m_LastTickTimestamp;
        LONGLONG m_Elapsed;

        float m_ElapsedTime;
        float m_DeltaTime;
    };
}

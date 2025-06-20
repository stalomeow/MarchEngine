#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <stdint.h>
#include <Windows.h>
#include <chrono>

namespace march
{
    class BusyProgressBar
    {
        const std::string m_Title;
        const uint32_t m_CheckIntervalMilliseconds;

        std::atomic_int m_EnableCounter; // 允许小于 0，表示强制关闭
        std::atomic_bool m_IsUserAlive;
        std::atomic_bool m_ShouldQuit;

        HWND m_WindowHandle;                                   // 别在主线程使用
        std::chrono::steady_clock::time_point m_BusyStartTime; // 别在主线程使用
        std::thread m_Thread;

        void ThreadProc();
        void Show(const std::chrono::steady_clock::time_point& currentTime);
        void Hide();

    public:
        BusyProgressBar(const std::string& title, uint32_t checkIntervalMilliseconds);
        ~BusyProgressBar();

        void ReportAlive();
        void BeginEnabledScope();
        void EndEnabledScope();
    };
}

#include "pch.h"
#include "BusyProgressBar.h"
#include "Engine/Misc/StringUtils.h"
#include <commctrl.h>
#include <processthreadsapi.h>

namespace march
{
    static constexpr LPCWSTR BusyProgressBarWindowClassName = L"BusyProgressBarWindow";

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
        {
            RECT screenRect;
            GetClientRect(GetDesktopWindow(), &screenRect);

            const int screenWidth = screenRect.right - screenRect.left;
            const int screenHeight = screenRect.bottom - screenRect.top;
            const int progressBarWidth = int(screenWidth * 0.25f);
            const int progressBarHeight = int(progressBarWidth * 0.05f);
            const int margin = 10;

            // 使用进度不确定的进度条（PBS_MARQUEE）
            HWND hProgressBar = CreateWindowW(
                PROGRESS_CLASS,
                NULL,
                WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
                margin,
                margin,
                progressBarWidth,
                progressBarHeight,
                hWnd,
                NULL,
                GetModuleHandleW(NULL),
                NULL);
            SendMessageW(hProgressBar, PBM_SETMARQUEE, TRUE, 0);

            RECT windowRect;
            RECT clientRect;
            GetWindowRect(hWnd, &windowRect); // 获取窗口的外部矩形（包括边框和标题栏）
            GetClientRect(hWnd, &clientRect); // 获取窗口的客户区矩形（不包括边框和标题栏）

            const int borderWidth = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
            const int borderHeight = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

            // 显示在屏幕中央，所有窗口之上
            const int width = borderWidth + progressBarWidth + margin * 2;
            const int height = borderHeight + progressBarHeight + margin * 2;
            const int x = (screenWidth - width) / 2;
            const int y = (screenHeight - height) / 2;
            SetWindowPos(hWnd, HWND_TOPMOST, x, y, width, height, NULL);
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            // 不要退出线程
            // PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    BusyProgressBar::BusyProgressBar(const std::string& title, uint32_t checkIntervalMilliseconds)
        : m_Title(StringUtils::Utf8ToUtf16(title))
        , m_CheckIntervalMilliseconds(checkIntervalMilliseconds)
        , m_EnableCounter(0)
        , m_IsUserAlive(false)
        , m_ShouldQuit(false)
        , m_WindowHandle(NULL)
        , m_BusyStartTime{}
    {
        HMODULE hInstance = GetModuleHandleW(NULL);

        if (WNDCLASSW wc{}; !GetClassInfoW(hInstance, BusyProgressBarWindowClassName, &wc))
        {
            ZeroMemory(&wc, sizeof(wc));
            wc.lpfnWndProc = WndProc;
            wc.hInstance = hInstance;
            wc.lpszClassName = BusyProgressBarWindowClassName;
            RegisterClassW(&wc);
        }

        m_Thread = std::thread(&BusyProgressBar::ThreadProc, this);
    }

    BusyProgressBar::~BusyProgressBar()
    {
        m_ShouldQuit = true;

        if (m_Thread.joinable())
        {
            m_Thread.join();
        }
    }

    void BusyProgressBar::ReportAlive()
    {
        m_IsUserAlive = true;
    }

    void BusyProgressBar::BeginEnabledScope()
    {
        ++m_EnableCounter;
    }

    void BusyProgressBar::EndEnabledScope()
    {
        --m_EnableCounter;
    }

    void BusyProgressBar::ThreadProc()
    {
        SetThreadDescription(GetCurrentThread(), L"BusyProgressBar");

        auto checkAlive = [this](std::chrono::steady_clock::time_point& lastTime, bool force)
        {
            auto currentTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);

            if (force || duration.count() >= static_cast<std::chrono::milliseconds::rep>(m_CheckIntervalMilliseconds))
            {
                if (m_EnableCounter > 0 && !m_IsUserAlive.exchange(false))
                {
                    Show(currentTime);
                }
                else
                {
                    Hide();
                }

                lastTime = currentTime;
            }
        };

        MSG msg{};

        auto lastCheckTime = std::chrono::steady_clock::now();
        checkAlive(lastCheckTime, /* force */ true);

        while (!m_ShouldQuit)
        {
            DWORD timeout = static_cast<DWORD>(m_CheckIntervalMilliseconds);

            // 等待消息，或者超时后检查 alive
            if (MsgWaitForMultipleObjects(0, NULL, FALSE, timeout, QS_ALLINPUT) == WAIT_OBJECT_0)
            {
                while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }

            checkAlive(lastCheckTime, /* force */ false);
        }

        if (m_WindowHandle)
        {
            DestroyWindow(m_WindowHandle);
            m_WindowHandle = NULL;
        }
    }

    void BusyProgressBar::Show(const std::chrono::steady_clock::time_point& currentTime)
    {
        if (m_WindowHandle)
        {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - m_BusyStartTime);

            if (duration.count() > 0)
            {
                SetWindowTextW(m_WindowHandle, StringUtils::Format(L"{} (busy for {}s) ...", m_Title, duration.count()).c_str());
            }
        }
        else
        {
            // 这个窗口不能抢占主窗口的焦点，否则主窗口无法更新，所以使用 WS_EX_NOACTIVATE
            // 使用 WS_EX_NOACTIVATE 后，窗口经常显示在所有窗口的下面，所以使用 WS_EX_TOPMOST 强制置顶
            m_WindowHandle = CreateWindowExW(
                WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                BusyProgressBarWindowClassName,
                StringUtils::Format(L"{} ...", m_Title).c_str(),
                WS_OVERLAPPED | WS_CAPTION, // 有标题栏，无关闭按钮
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                NULL,
                NULL,
                GetModuleHandleW(NULL),
                NULL);

            ShowWindow(m_WindowHandle, SW_SHOW);
            UpdateWindow(m_WindowHandle);
            m_BusyStartTime = currentTime;
        }
    }

    void BusyProgressBar::Hide()
    {
        if (m_WindowHandle)
        {
            SendMessageW(m_WindowHandle, WM_CLOSE, 0, 0);
            m_WindowHandle = NULL;
        }
    }
}

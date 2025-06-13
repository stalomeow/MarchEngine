#include "pch.h"
#include "Engine/Application.h"
#include "Engine/EngineTimer.h"
#include "Engine/Misc/DeferFunc.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Rendering/RenderPipeline.h"
#include <WindowsX.h>
#include <dwmapi.h>
#include <stdexcept>
#include <assert.h>

namespace march
{
    static Application* g_Application = nullptr;

    Application::Application()
        : m_IsStarted(false)
        , m_IsTicking(false)
        , m_InstanceHandle(NULL)
        , m_WindowHandle(NULL)
    {
        m_Timer = std::make_unique<EngineTimer>();
        m_RenderPipeline = nullptr;
    }

    Application::~Application() = default;

    uint32_t Application::GetClientWidth() const
    {
        RECT clientRect = {};
        GetClientRect(m_WindowHandle, &clientRect);
        return static_cast<uint32_t>(clientRect.right - clientRect.left);
    }

    uint32_t Application::GetClientHeight() const
    {
        RECT clientRect = {};
        GetClientRect(m_WindowHandle, &clientRect);
        return static_cast<uint32_t>(clientRect.bottom - clientRect.top);
    }

    float Application::GetClientAspectRatio() const
    {
        float width = static_cast<float>(GetClientWidth());
        float height = static_cast<float>(GetClientHeight());
        return width / height;
    }

    float Application::GetDisplayScale() const
    {
        UINT dpi = GetDpiForWindow(m_WindowHandle);
        return static_cast<float>(dpi) / 96.0f;
    }

    RenderPipeline* Application::GetRenderPipeline() const
    {
        return m_RenderPipeline.get();
    }

    void ApplicationManagedOnlyAPI::InitRenderPipeline(Application* app)
    {
        app->m_RenderPipeline = std::make_unique<RenderPipeline>();
    }

    void ApplicationManagedOnlyAPI::ReleaseRenderPipeline(Application* app)
    {
        app->m_RenderPipeline.reset();
    }

    HINSTANCE Application::GetInstanceHandle() const
    {
        return m_InstanceHandle;
    }

    HWND Application::GetWindowHandle() const
    {
        return m_WindowHandle;
    }

    void Application::SetWindowTitle(const std::string& title) const
    {
        std::wstring wTitle = StringUtils::Utf8ToUtf16(title);
        SetWindowTextW(m_WindowHandle, wTitle.c_str());
    }

    float Application::GetDeltaTime() const
    {
        return m_Timer->GetDeltaTime();
    }

    float Application::GetElapsedTime() const
    {
        return m_Timer->GetElapsedTime();
    }

    uint64_t Application::GetFrameCount() const
    {
        return m_Timer->GetFrameCount();
    }

    uint32_t Application::GetFPS() const
    {
        return m_Timer->GetFPS();
    }

    int Application::Run(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
    {
        m_InstanceHandle = hInstance;
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        if (!InitWindow(nCmdShow))
        {
            return 0;
        }

#if !defined(_DEBUG)
        try
        {
#endif

            g_Application = this;
            return RunImpl(lpCmdLine);

#if !defined(_DEBUG)
        }
        catch (const std::exception& e)
        {
            OnQuit();
            CrashWithMessage(e.what());
            return 0;
        }
#endif
    }

    void Application::Quit(int exitCode)
    {
        PostQuitMessage(exitCode);
    }

    bool Application::InitWindow(int nCmdShow)
    {
        WNDCLASSW wc = {};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = m_InstanceHandle;
        wc.lpszClassName = L"MarchEngineMainWindow";
        wc.hbrBackground = CreateSolidBrush(GetBackgroundColor());
        wc.hIcon = GetIcon();

        if (!RegisterClassW(&wc))
        {
            CrashWithMessage("Register Window Class Failed");
            return false;
        }

        m_WindowHandle = CreateWindowW(
            wc.lpszClassName,
            L"March Engine",
            WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            m_InstanceHandle,
            this);

        if (!m_WindowHandle)
        {
            CrashWithMessage("Create Window Failed");
            return false;
        }

        // Dark Mode Window
        // https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
        BOOL dark = TRUE;
        DwmSetWindowAttribute(m_WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

        // ShowWindow(m_WindowHandle, nCmdShow);
        ShowWindow(m_WindowHandle, SW_SHOWMAXIMIZED); // 强制最大化显示
        UpdateWindow(m_WindowHandle);
        return true;
    }

    static std::vector<std::string> ParseCommandLineArgs(LPWSTR lpCmdLine)
    {
        int numArgs = 0;
        LPWSTR* args = CommandLineToArgvW(lpCmdLine, &numArgs);

        if (args == nullptr)
        {
            throw std::runtime_error("Failed to parse command line arguments.");
        }

        std::vector<std::string> results(numArgs);

        for (int i = 0; i < numArgs; i++)
        {
            results[i] = StringUtils::Utf16ToUtf8(args[i], static_cast<int32_t>(wcslen(args[i])));
        }

        LocalFree(args);
        return results;
    }

    int Application::RunImpl(LPWSTR lpCmdLine)
    {
        m_Timer->Restart();
        OnStart(ParseCommandLineArgs(lpCmdLine));
        m_IsStarted = true;

        MSG msg = {};
        uint8_t msgCount = 0;

        while (msg.message != WM_QUIT)
        {
            bool gotMsg;

            if (m_Timer->GetIsRunning())
            {
                gotMsg = PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
            }
            else
            {
                // 游戏暂停时，阻塞等待
                gotMsg = GetMessageW(&msg, NULL, 0, 0);
            }

            if (gotMsg)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
                msgCount++;

                if (msgCount < 100)
                {
                    continue;
                }
            }

            msgCount = 0;
            Tick(/* willQuit */ false);
        }

        Tick(/* willQuit */ true);
        OnQuit();
        return static_cast<int>(msg.wParam);
    }

    void Application::Tick(bool willQuit)
    {
        if (m_IsTicking)
        {
            return;
        }

        // 一些 C# 的线程同步工具在阻塞线程时会处理 Win32 消息，比如 WM_PAINT
        // 如果在 Tick 中阻塞主线程，期间可能处理 WM_PAINT，而 WM_PAINT 中又调用 Tick 更新画面的话，会不停递归调用 Tick，一直 BeginFrame() 而没有 EndFrame()
        // 必须避免递归调用 Tick
        m_IsTicking = true;
        DEFER_FUNC() { m_IsTicking = false; };

        // 如果要退出的话，需要强制 Tick 一次
        if (m_Timer->Tick() || willQuit)
        {
            OnTick(willQuit);
        }
    }

    void Application::CrashWithMessage(const std::string& message, bool debugBreak)
    {
        CrashWithMessage("Fatal Error", message, debugBreak);
    }

    void Application::CrashWithMessage(const std::string& title, const std::string& message, bool debugBreak)
    {
        std::wstring wTitle = StringUtils::Utf8ToUtf16(title);
        std::wstring wMessage = StringUtils::Utf8ToUtf16(message);
        MessageBoxW(NULL, wMessage.c_str(), wTitle.c_str(), MB_OK | MB_ICONERROR);

#if defined(_DEBUG)
        if (debugBreak && IsDebuggerPresent())
        {
            DebugBreak();
        }
#endif

        // 强制退出
        // 此时很多对象的状态已经损坏了，不应该执行任何析构函数（可能报错），所以不能用 std::exit
        TerminateProcess(GetCurrentProcess(), EXIT_FAILURE);
    }

    LRESULT Application::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // 不处理 WM_ACTIVATE，失去焦点时也要继续渲染，否则无法处理 DragDrop 等交互

        switch (msg)
        {
        case WM_DPICHANGED:
        {
            RECT* const prcNewWindow = (RECT*)lParam;
            SetWindowPos(m_WindowHandle,
                NULL,
                prcNewWindow->left,
                prcNewWindow->top,
                prcNewWindow->right - prcNewWindow->left,
                prcNewWindow->bottom - prcNewWindow->top,
                SWP_NOZORDER | SWP_NOACTIVATE);

            OnDisplayScaleChange();
            return 0;
        }

        case WM_PAINT:
        {
            OnPaint();
            ValidateRect(m_WindowHandle, nullptr);
            return 0;
        }

        case WM_SIZE:
        {
            if (wParam == SIZE_MINIMIZED)
            {
                m_Timer->Stop();
                OnPause();
            }
            else
            {
                if (m_Timer->GetIsRunning())
                {
                    OnResize();
                }
                else
                {
                    m_Timer->Start();
                    OnResume();
                }
            }
            return 0;
        }

        // The WM_MENUCHAR message is sent when a menu is active and the user presses 
        // a key that does not correspond to any mnemonic or accelerator key. 
        case WM_MENUCHAR:
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);

        // Catch this message so to prevent the window from becoming too small.
        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
            return 0;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_KEYDOWN:
        {
            OnKeyDown(wParam);
            return 0;
        }

        case WM_KEYUP:
        {
            OnKeyUp(wParam);
            return 0;
        }

        // WM_DESTROY is sent when the window is being destroyed.
        case WM_DESTROY:
            Quit(0);
            return 0;
        }

        return DefWindowProcW(m_WindowHandle, msg, wParam, lParam);
    }

    LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Application* pThis = nullptr;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<Application*>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else
        {
            LONG_PTR ptr = GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            pThis = reinterpret_cast<Application*>(ptr);
        }

        if (pThis != nullptr && pThis->m_IsStarted && pThis->GetWindowHandle() != nullptr)
        {
            assert(hWnd == pThis->GetWindowHandle());
            return pThis->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    Application* GetApp() { return g_Application; }
}

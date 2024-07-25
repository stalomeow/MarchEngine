#include "App/WinApplication.h"
#include "Rendering/DxException.h"
#include <assert.h>
#include <WindowsX.h>

namespace dx12demo
{
    namespace
    {
        WinApplication g_App;

        void ShowErrorMessageBoxA(const std::string& message)
        {
            MessageBoxA(NULL, message.c_str(), "Error", MB_OK);
        }

        void ShowErrorMessageBoxW(const std::wstring& message)
        {
            MessageBoxW(NULL, message.c_str(), L"Error", MB_OK);
        }
    }

    bool WinApplication::Initialize(HINSTANCE hInstance, int nCmdShow, int clientWidth, int clientHeight)
    {
        m_InstanceHandle = hInstance;
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        return InitWindow(nCmdShow, clientWidth, clientHeight);
    }

    bool WinApplication::InitWindow(int nCmdShow, int clientWidth, int clientHeight)
    {
        WNDCLASS wc = { 0 };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = m_InstanceHandle;
        wc.lpszClassName = TEXT("DX12DemoWindow");

        if (!RegisterClass(&wc))
        {
            ShowErrorMessageBoxA("Register Window Class Failed");
            return false;
        }

        // Compute window rectangle dimensions based on requested client area dimensions.
        RECT rect = { 0, 0, clientWidth, clientHeight };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

        m_WindowHandle = CreateWindow(
            wc.lpszClassName,
            TEXT("DX12 Demo"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            NULL,
            NULL,
            m_InstanceHandle,
            this);

        if (!m_WindowHandle)
        {
            ShowErrorMessageBoxA("Create Window Failed");
            return false;
        }

        ShowWindow(m_WindowHandle, nCmdShow);
        UpdateWindow(m_WindowHandle);
        return true;
    }

    int WinApplication::Run()
    {
        try
        {
            MSG msg = { 0 };
            m_Timer.Restart();
            InvokeEvent([](auto listener) { listener->OnAppStart(); });

            while (msg.message != WM_QUIT)
            {
                bool gotMsg;

                if (m_Timer.GetIsRunning())
                {
                    gotMsg = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
                }
                else
                {
                    // 游戏暂停时，阻塞等待
                    gotMsg = GetMessage(&msg, NULL, 0, 0);
                }

                if (gotMsg)
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    continue; // 优先处理窗体消息
                }

                if (m_Timer.Tick())
                {
                    InvokeEvent([](auto listener) { listener->OnAppTick(); });
                }
            }

            InvokeEvent([](auto listener) { listener->OnAppQuit(); });
            return (int)msg.wParam;
        }
        catch (const std::exception& e)
        {
            ShowErrorMessageBoxA(e.what());
            InvokeEvent([](auto listener) { listener->OnAppQuit(); });
            return 0;
        }
        catch (const DxException& e)
        {
            ShowErrorMessageBoxW(e.ToString());
            InvokeEvent([](auto listener) { listener->OnAppQuit(); });
            return 0;
        }
    }

    void WinApplication::Quit(int exitCode)
    {
        PostQuitMessage(exitCode);
    }

    void WinApplication::InvokeEvent(std::function<void(IApplicationEventListener* const)> invoke)
    {
        for (IApplicationEventListener* const listener : m_EventListeners)
        {
            invoke(listener);
        }
    }

    LRESULT WinApplication::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        for (IApplicationEventListener* const listener : m_EventListeners)
        {
            LRESULT result = 0;
            if (listener->OnAppMessage(msg, wParam, lParam, result))
            {
                return result;
            }
        }

        switch (msg)
        {
        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                m_Timer.Stop();
                InvokeEvent([](auto listener) { listener->OnAppPaused(); });
            }
            else
            {
                m_Timer.Start();
                InvokeEvent([](auto listener) { listener->OnAppResumed(); });
            }
            return 0;
        }

        case WM_DPICHANGED:
        {
            RECT* const prcNewWindow = (RECT*)lParam;
            SetWindowPos(GetHWND(),
                NULL,
                prcNewWindow->left,
                prcNewWindow->top,
                prcNewWindow->right - prcNewWindow->left,
                prcNewWindow->bottom - prcNewWindow->top,
                SWP_NOZORDER | SWP_NOACTIVATE);

            InvokeEvent([](auto listener) { listener->OnAppDisplayScaleChanged(); });
            return 0;
        }

        case WM_PAINT:
        {
            InvokeEvent([](auto listener) { listener->OnAppPaint(); });
            ValidateRect(GetHWND(), nullptr);
            return 0;
        }

        case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                InvokeEvent([](auto listener) { listener->OnAppResized(); });
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
            InvokeEvent([wParam, lParam](auto listener) {
                listener->OnAppMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); });
            return 0;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            InvokeEvent([wParam, lParam](auto listener) {
                listener->OnAppMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); });
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            InvokeEvent([wParam, lParam](auto listener) {
                listener->OnAppMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); });
            return 0;
        }

        case WM_KEYDOWN:
        {
            InvokeEvent([wParam](auto listener) {listener->OnAppKeyDown(wParam); });
            return 0;
        }

        case WM_KEYUP:
        {
            InvokeEvent([wParam](auto listener) {listener->OnAppKeyUp(wParam); });
            return 0;
        }

        // WM_DESTROY is sent when the window is being destroyed.
        case WM_DESTROY:
            Quit();
            return 0;
        }

        return DefWindowProc(GetHWND(), msg, wParam, lParam);
    }

    LRESULT CALLBACK WinApplication::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        WinApplication* pthis = nullptr;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pthis = reinterpret_cast<WinApplication*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pthis));
        }
        else
        {
            auto ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
            pthis = reinterpret_cast<WinApplication*>(ptr);
        }

        if (pthis && pthis->GetHWND() != nullptr)
        {
            assert(hWnd == pthis->GetHWND());
            return pthis->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    WinApplication& GetApp()
    {
        return g_App;
    }
}

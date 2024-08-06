#pragma once

#include "Core/IEngine.h"
#include "Core/GameTimer.h"
#include "Scripting/ScriptTypes.h"
#include <Windows.h>
#include <string>
#include <tuple>

namespace dx12demo
{
    class WinApplication final
    {
    public:
        WinApplication() = default;
        ~WinApplication() = default;

    public:
        bool Initialize(HINSTANCE hInstance, int nCmdShow, int clientWidth = 800, int clientHeight = 600);
        int RunEngine(IEngine* engine);
        void Quit(int exitCode = 0);

        float GetDeltaTime() const { return m_Timer.GetDeltaTime(); }

        float GetElapsedTime() const { return m_Timer.GetElapsedTime(); }

        void SetTitle(const std::wstring& title) const
        {
            SetWindowTextW(GetHWND(), title.c_str());
        }

        std::tuple<int, int> GetClientWidthAndHeight() const
        {
            RECT clientRect = {};
            GetClientRect(GetHWND(), &clientRect);

            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            return std::make_tuple(width, height);
        }

        float GetAspectRatio() const
        {
            auto [width, height] = GetClientWidthAndHeight();
            return static_cast<float>(width) / static_cast<float>(height);
        }

        float GetDisplayScale() const
        {
            UINT dpi = GetDpiForWindow(GetHWND());
            return static_cast<float>(dpi) / 96.0f;
        }

        HINSTANCE GetHINSTANCE() const { return m_InstanceHandle; }
        HWND GetHWND() const { return m_WindowHandle; }
        IEngine* GetEngine() const { return m_Engine; }

    private:
        bool InitWindow(int nCmdShow, int clientWidth, int clientHeight);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        IEngine* m_Engine = nullptr;
        GameTimer m_Timer{};
        HINSTANCE m_InstanceHandle = nullptr;
        HWND m_WindowHandle = nullptr;
    };

    WinApplication& GetApp();

    namespace binding
    {
        inline CSHARP_API(CSharpFloat) Application_GetDeltaTime()
        {
            return GetApp().GetDeltaTime();
        }

        inline CSHARP_API(CSharpFloat) Application_GetElapsedTime()
        {
            return GetApp().GetElapsedTime();
        }

        inline CSHARP_API(IEngine*) Application_GetEngine()
        {
            return GetApp().GetEngine();
        }
    }
}

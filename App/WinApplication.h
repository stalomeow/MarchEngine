#pragma once

#include "App/IApplicationEventListener.h"
#include "Core/GameTimer.h"
#include <Windows.h>
#include <string>
#include <tuple>
#include <vector>
#include <functional>

namespace dx12demo
{
    class WinApplication final
    {
    public:
        WinApplication() = default;
        ~WinApplication() = default;

    public:
        bool Initialize(HINSTANCE hInstance, int nCmdShow, int clientWidth = 800, int clientHeight = 600);
        int Run();
        void Quit(int exitCode = 0);

        void AddEventListener(IApplicationEventListener* const listener)
        {
            m_EventListeners.push_back(listener);
        }

        void RemoveEventListener(IApplicationEventListener* const listener)
        {
            auto begin = std::remove(m_EventListeners.begin(), m_EventListeners.end(), listener);
            m_EventListeners.erase(begin, m_EventListeners.end());
        }

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

    private:
        bool InitWindow(int nCmdShow, int clientWidth, int clientHeight);
        void InvokeEvent(std::function<void(IApplicationEventListener* const)> invoke);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        std::vector<IApplicationEventListener*> m_EventListeners{};
        GameTimer m_Timer{};
        HINSTANCE m_InstanceHandle = nullptr;
        HWND m_WindowHandle = nullptr;
    };

    WinApplication& GetApp();
}

#pragma once

#include <Windows.h>
#include <string>
#include <memory>
#include <vector>
#include <stdint.h>

namespace march
{
    class EngineTimer;
    class RenderPipeline;

    class Application
    {
    public:
        virtual ~Application();

        int Run(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow);
        void Quit(int exitCode);

        uint32_t GetClientWidth() const;
        uint32_t GetClientHeight() const;
        float GetClientAspectRatio() const;
        float GetDisplayScale() const;

        HINSTANCE GetInstanceHandle() const;
        HWND GetWindowHandle() const;
        void SetWindowTitle(const std::string& title) const;

        float GetDeltaTime() const;
        float GetElapsedTime() const;
        uint64_t GetFrameCount() const;

        static void ShowErrorMessageBox(const std::string& message);

        virtual const std::string& GetDataPath() const = 0;
        virtual RenderPipeline* GetRenderPipeline() const = 0;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

    protected:
        Application();

        virtual void OnStart(const std::vector<std::string>& args) {}
        virtual void OnTick() {}
        virtual void OnQuit() {}

        virtual void OnResize() {}
        virtual void OnDisplayScaleChange() {}
        virtual void OnPaint() {}

        virtual void OnPause() {}
        virtual void OnResume() {}

        virtual void OnMouseDown(WPARAM btnState, int32_t x, int32_t y) { }
        virtual void OnMouseUp(WPARAM btnState, int32_t x, int32_t y) { }
        virtual void OnMouseMove(WPARAM btnState, int32_t x, int32_t y) { }

        virtual void OnKeyDown(WPARAM btnState) { }
        virtual void OnKeyUp(WPARAM btnState) { }

        virtual bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult) { return false; }

    private:
        bool InitWindow(int nCmdShow);
        int RunImpl(LPWSTR lpCmdLine);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

        bool m_IsStarted;
        std::unique_ptr<EngineTimer> m_Timer;
        HINSTANCE m_InstanceHandle;
        HWND m_WindowHandle;
    };

    Application* GetApp();
}

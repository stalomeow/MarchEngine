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
        friend struct ApplicationManagedOnlyAPI;

    public:
        virtual ~Application();

        int Run(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow);
        void Quit(int exitCode);

        uint32_t GetClientWidth() const;
        uint32_t GetClientHeight() const;
        float GetClientAspectRatio() const;
        float GetDisplayScale() const;

        RenderPipeline* GetRenderPipeline() const;
        HINSTANCE GetInstanceHandle() const;
        HWND GetWindowHandle() const;
        void SetWindowTitle(const std::string& title) const;

        float GetDeltaTime() const;
        float GetElapsedTime() const;
        uint64_t GetFrameCount() const;

        void CrashWithMessage(const std::string& message, bool debugBreak = false);
        virtual void CrashWithMessage(const std::string& title, const std::string& message, bool debugBreak = false);

        virtual const std::string& GetProjectName() const = 0;
        virtual const std::string& GetDataPath() const = 0;
        virtual const std::string& GetEngineResourcePath() const = 0;
        virtual const std::string& GetEngineShaderPath() const = 0;
        virtual const std::string& GetShaderCachePath() const = 0;
        virtual bool IsEngineResourceEditable() const = 0;
        virtual bool IsEngineShaderEditable() const = 0;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

    protected:
        Application();

        virtual void OnStart(const std::vector<std::string>& args) {}
        virtual void OnTick(bool willQuit) {}
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

        virtual HICON GetIcon() { return NULL; }

        virtual LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        bool InitWindow(int nCmdShow);
        int RunImpl(LPWSTR lpCmdLine);
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

        bool m_IsStarted;
        std::unique_ptr<EngineTimer> m_Timer;
        std::unique_ptr<RenderPipeline> m_RenderPipeline;
        HINSTANCE m_InstanceHandle;
        HWND m_WindowHandle;
    };

    // 仅限 C# 调用
    struct ApplicationManagedOnlyAPI
    {
        static void InitRenderPipeline(Application* app);
        static void ReleaseRenderPipeline(Application* app);
    };

    Application* GetApp();
}

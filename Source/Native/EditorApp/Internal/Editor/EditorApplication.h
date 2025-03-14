#pragma once

#include "Engine/Application.h"
#include <vector>
#include <memory>
#include <string>

namespace march
{
    class BusyProgressBar;
    class GfxSwapChain;
    class RenderPipeline;
    enum class ManagedMethod;

    class EditorApplication : public Application
    {
    public:
        EditorApplication();
        ~EditorApplication() override;

        const std::string& GetDataPath() const override { return m_DataPath; }
        const std::string& GetEngineResourcePath() const override { return m_EngineResourcePath; }
        const std::string& GetEngineShaderPath() const override { return m_EngineShaderPath; }
        bool IsEngineResourceEditable() const override;
        bool IsEngineShaderEditable() const override;
        RenderPipeline* GetRenderPipeline() const override { return m_RenderPipeline.get(); }
        std::string SaveFilePanelInProject(const std::string& title, const std::string& defaultName, const std::string& extension, const std::string& path) const;

    protected:
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
        void OnStart(const std::vector<std::string>& args) override;
        void OnQuit() override;
        void OnTick(bool willQuit) override;
        void OnResize() override;
        void OnDisplayScaleChange() override;
        void OnPaint() override;
        void OnPause() override;
        void OnResume() override;

    private:
        void InitImGui();
        void DrawBaseImGui();
        void CalculateFrameStats();
        void ReloadFonts();
        void InitPaths();

        std::unique_ptr<GfxSwapChain> m_SwapChain;
        std::unique_ptr<RenderPipeline> m_RenderPipeline;
        std::unique_ptr<BusyProgressBar> m_ProgressBar;

        std::string m_DataPath;
        std::string m_EngineResourcePath;
        std::string m_EngineShaderPath;
        std::string m_ImGuiIniFilename;

        bool m_IsInitialized;

        const float m_FontSizeLatin = 15.0f;
        const float m_FontSizeCJK = 19.0f;
        const float m_FontSizeIcon = 13.0f;
    };
}

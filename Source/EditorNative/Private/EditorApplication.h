#pragma once

#include "Engine/Application.h"
#include <vector>
#include <memory>
#include <string>

namespace march
{
    class GfxSwapChain;
    class BusyProgressBar;

    class EditorApplication : public Application
    {
    public:
        EditorApplication();
        ~EditorApplication() override;

        const std::string& GetProjectName() const override { return m_ProjectName; }
        const std::string& GetDataPath() const override { return m_DataPath; }
        const std::string& GetEngineResourcePath() const override { return m_EngineResourcePath; }
        const std::string& GetEngineShaderPath() const override { return m_EngineShaderPath; }
        const std::string& GetShaderCachePath() const override { return m_ShaderCachePath; }
        bool IsEngineResourceEditable() const override;
        bool IsEngineShaderEditable() const override;
        std::string SaveFilePanelInProject(const std::string& title, const std::string& defaultName, const std::string& extension, const std::string& path) const;

        void CrashWithMessage(const std::string& title, const std::string& message, bool debugBreak = false) override;

    protected:
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
        void OnStart(const std::vector<std::string>& args) override;
        void OnQuit() override;
        void OnTick(bool willQuit) override;
        void OnResize() override;
        void OnDisplayScaleChange() override;
        void OnPaint() override;
        HICON GetIcon() override;
        COLORREF GetBackgroundColor() override;

    private:
        void InitProject(const std::string& path);
        void InitImGui();
        void DrawBaseImGui();
        void ReloadFonts();

        std::unique_ptr<GfxSwapChain> m_SwapChain;
        std::unique_ptr<BusyProgressBar> m_ProgressBar;

        std::string m_ProjectName;
        std::string m_DataPath;
        std::string m_EngineResourcePath;
        std::string m_EngineShaderPath;
        std::string m_ShaderCachePath;
        std::string m_ImGuiIniFilename;

        bool m_IsInitialized;
    };
}

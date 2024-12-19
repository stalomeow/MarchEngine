#pragma once

#include "Application.h"
#include <directx/d3dx12.h>
#include "RenderPipeline.h"
#include "GfxDescriptor.h"
#include <vector>
#include <wrl.h>
#include <memory>
#include <string>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "RenderGraph.h"
#include "AssetManger.h"
#include "Shader.h"
#include "Material.h"

namespace march
{
    class EditorApplication : public Application
    {
    public:
        const std::string& GetDataPath() const override;
        RenderPipeline* GetRenderPipeline() const override;

    protected:
        bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult) override;
        void OnStart(const std::vector<std::string>& args) override;
        void OnQuit() override;
        void OnTick() override;
        void OnResize() override;
        void OnDisplayScaleChange() override;
        void OnPaint() override;

    private:
        void InitImGui();
        void DrawBaseImGui();
        void CalculateFrameStats();
        std::string GetFontPath(std::string fontName) const;
        std::string GetFontAwesomePath(std::string fontName) const;
        void ReloadFonts();

        void CreateImGuiTempTexture();

        void BeginFrame();
        void EndFrame(bool discardImGui);

        std::unique_ptr<RenderPipeline> m_RenderPipeline = nullptr;
        std::unique_ptr<GfxRenderTexture> m_TempImGuiTexture = nullptr;

        std::string m_DataPath;
        std::string m_ImGuiIniFilename{};

        const float m_FontSizeLatin = 15.0f;
        const float m_FontSizeCJK = 19.0f;
        const float m_FontSizeIcon = 13.0f;
    };
}

#pragma once

#include <directx/d3dx12.h>
#include "IEngine.h"
#include "RenderPipeline.h"
#include "GfxDescriptorHeap.h"
#include <vector>
#include <wrl.h>
#include <memory>
#include <string>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

namespace march
{
    class GameEditor : public IEngine
    {
    public:
        bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult) override;
        void OnStart(const std::vector<std::string>& args) override;
        void OnQuit() override;
        void OnTick(bool willQuit) override;
        void OnResized() override;
        void OnDisplayScaleChanged() override;
        void OnPaint() override;

    public:
        RenderPipeline* GetRenderPipeline() override { return m_RenderPipeline.get(); }

    private:
        void InitImGui();
        void DrawBaseImGui();
        void CalculateFrameStats();
        std::string GetFontPath(std::string fontName) const;
        std::string GetFontAwesomePath(std::string fontName) const;
        void ReloadFonts();

    private:
        std::unique_ptr<RenderPipeline> m_RenderPipeline = nullptr;
        GfxDescriptorTable m_StaticDescriptorViewTable;

        std::string m_ImGuiIniFilename{};
        bool m_IsScriptInitialized = false;

        const float m_FontSizeLatin = 15.0f;
        const float m_FontSizeCJK = 19.0f;
        const float m_FontSizeIcon = 13.0f;
    };
}

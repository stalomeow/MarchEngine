#pragma once

#include "Application.h"
#include <directx/d3dx12.h>
#include "RenderPipeline.h"
#include "GfxDescriptorHeap.h"
#include <vector>
#include <wrl.h>
#include <memory>
#include <string>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "imgui_impl_dx12.h"
#include "RenderGraph.h"
#include "AssetManger.h"
#include "Shader.h"
#include "Material.h"

namespace march
{
    class GameEditor : public Application
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

        void BeginFrame();
        void EndFrame(bool discardImGui);

        void DrawImGuiRenderGraph(GfxDevice* device, int32_t renderTargetId);
        void BlitImGuiToBackBuffer(GfxDevice* device, int32_t srcId, int32_t backBufferId);
        GfxMesh* GetFullScreenTriangleMesh();

        asset_ptr<Shader> m_BlitImGuiShader = nullptr;
        std::unique_ptr<Material> m_BlitImGuiMaterial = nullptr;
        std::unique_ptr<RenderPipeline> m_RenderPipeline = nullptr;
        std::unique_ptr<RenderGraph> m_ImGuiRenderGraph = nullptr;
        GfxDescriptorTable m_StaticDescriptorViewTable;

        std::string m_DataPath = "C:/Users/10247/Desktop/TestProj";
        std::string m_ImGuiIniFilename{};

        const DXGI_FORMAT m_ImGuiRtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        const float m_FontSizeLatin = 15.0f;
        const float m_FontSizeCJK = 19.0f;
        const float m_FontSizeIcon = 13.0f;
    };
}

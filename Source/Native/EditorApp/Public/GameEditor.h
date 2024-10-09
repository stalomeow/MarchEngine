#pragma once

#include <directx/d3dx12.h>
#include "IEngine.h"
#include "RenderPipeline.h"
#include "GfxDescriptorHeap.h"
#include "RenderDoc.h"
#include "DotNetRuntime.h"
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
        void OnTick() override;
        void OnResized() override;
        void OnDisplayScaleChanged() override;
        void OnPaint() override;

    public:
        RenderPipeline* GetRenderPipeline() override { return m_RenderPipeline.get(); }

    private:
        void InitImGui();
        void DrawImGui(Camera* sceneCamera);
        void ResizeRenderPipeline(Camera* sceneCamera, int width, int height);
        void SetSceneViewSrv(Camera* sceneCamera);
        void DrawDebugDescriptorTableAllocator(const std::string& name, GfxDescriptorTableAllocator* allocator);
        void DrawConsoleWindow();
        void CalculateFrameStats();
        std::string GetFontPath();

    private:
        std::unique_ptr<RenderPipeline> m_RenderPipeline = nullptr;
        GfxDescriptorTable m_StaticDescriptorViewTable;

        bool m_ShowDemoWindow = true;
        bool m_ShowConsoleWindow = true;
        bool m_ConsoleWindowAutoScroll = true;
        bool m_ConsoleWindowScrollToBottom = true;
        bool m_ShowDescriptorHeapWindow = true;

        RenderDoc m_RenderDoc{};
        std::unique_ptr<IDotNetRuntime> m_DotNet{};

    private:
        float m_FontSize = 15.0f;
    };
}

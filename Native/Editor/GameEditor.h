#pragma once

#include <directx/d3dx12.h>
#include "Core/IEngine.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/DescriptorHeap.h"
#include "Rendering/RenderDoc.h"
#include "Scripting/DotNet.h"
#include <d3d12.h>
#include <vector>
#include <wrl.h>
#include <memory>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

namespace dx12demo
{
    class GameEditor : public IEngine
    {
    public:
        bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult) override;
        void OnStart() override;
        void OnQuit() override;
        void OnTick() override;
        void OnResized() override;
        void OnDisplayScaleChanged() override;
        void OnPaint() override;

    public:
        RenderPipeline* GetRenderPipeline() override { return m_RenderPipeline.get(); }

    private:
        void CreateDescriptorHeaps();
        void InitImGui();
        void DrawImGui();
        void ResizeRenderPipeline(int width, int height);
        void DrawConsoleWindow();
        void CalculateFrameStats();

    private:
        std::unique_ptr<DescriptorHeap> m_SrvHeap = nullptr;
        std::unique_ptr<RenderPipeline> m_RenderPipeline = nullptr;

        int m_LastSceneViewWidth = 0;
        int m_LastSceneViewHeight = 0;

        bool m_ShowDemoWindow = true;
        bool m_ShowAnotherWindow = true;
        bool m_ShowConsoleWindow = true;
        bool m_ConsoleWindowAutoScroll = true;
        bool m_ConsoleWindowScrollToBottom = true;
        bool m_ShowHierarchyWindow = true;
        int m_SelectedGameObjectIndex = 0;
        ImVec4 m_ImGUIClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        RenderDoc m_RenderDoc{};
        DotNetEnv m_DotNet{};

    private:
        float m_FontSize = 15.0f;
        const char* m_FontPath = "C:\\Projects\\Graphics\\dx12-demo\\fonts\\Inter-Regular.otf";
    };
}

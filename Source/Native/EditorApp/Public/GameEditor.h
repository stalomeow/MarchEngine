#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include "IEngine.h"
#include "RenderPipeline.h"
#include "DescriptorHeap.h"
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
        void OnStart() override;
        void OnQuit() override;
        void OnTick() override;
        void OnResized() override;
        void OnDisplayScaleChanged() override;
        void OnPaint() override;

    public:
        RenderPipeline* GetRenderPipeline() override { return m_RenderPipeline.get(); }

    private:
        void InitImGui();
        void DrawImGui();
        void ResizeRenderPipeline(int width, int height);
        void DrawDebugDescriptorTableAllocator(const std::string& name, DescriptorTableAllocator* allocator);
        void DrawConsoleWindow();
        void CalculateFrameStats();
        std::string GetFontPath();

    private:
        std::unique_ptr<RenderPipeline> m_RenderPipeline = nullptr;
        DescriptorTable m_StaticDescriptorViewTable;

        int m_LastSceneViewWidth = 0;
        int m_LastSceneViewHeight = 0;

        bool m_ShowDemoWindow = true;
        bool m_ShowAnotherWindow = true;
        bool m_ShowConsoleWindow = true;
        bool m_ConsoleWindowAutoScroll = true;
        bool m_ConsoleWindowScrollToBottom = true;
        bool m_ShowHierarchyWindow = true;
        bool m_ShowDescriptorHeapWindow = true;

        RenderDoc m_RenderDoc{};
        std::unique_ptr<IDotNetRuntime> m_DotNet{};

    private:
        float m_FontSize = 15.0f;
    };
}

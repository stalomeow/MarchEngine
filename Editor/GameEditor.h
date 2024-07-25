#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include <vector>
#include <wrl.h>
#include <memory>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include "App/IApplicationEventListener.h"
#include "Rendering/RenderPipeline.h"
#include "Core/GameObject.h"

namespace dx12demo
{
    class GameEditor : public IApplicationEventListener
    {
    public:
        bool OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult) override;
        void OnAppStart() override;
        void OnAppQuit() override;
        void OnAppTick() override;
        void OnAppResized() override;
        void OnAppDisplayScaleChanged() override;
        void OnAppPaint() override;

    private:
        void CreateSwapChain();
        void CreateDescriptorHeaps();
        void InitImGui();
        void DrawImGui();
        void ResizeRenderPipeline(int width, int height);
        void ResizeSwapChain();
        void SwapBackBuffer();

        void DrawConsoleWindow();

        void CalculateFrameStats();
        void LogAdapters();
        void LogAdapterOutputs(IDXGIAdapter* adapter);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

        ID3D12Resource* GetBackBuffer() const
        {
            return m_SwapChainBuffers[m_CurrentBackBufferIndex].Get();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView() const
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
                m_CurrentBackBufferIndex,
                m_RtvDescriptorSize);
        }

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device = nullptr;

        Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
        static const int m_SwapChainBufferCount = 2;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_SwapChainBuffers[m_SwapChainBufferCount];
        int m_CurrentBackBufferIndex = 0;

        UINT m_RtvDescriptorSize;
        UINT m_CbvSrvUavDescriptorSize;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvHeap = nullptr;

        std::vector<std::unique_ptr<GameObject>> m_GameObjects{};
        std::unique_ptr<RenderPipeline> m_RenderPipeline = nullptr;

        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> m_DebugInfoQueue = nullptr;

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

    private:
        float m_FontSize = 15.0f;
        const char* m_FontPath = "C:\\Projects\\Graphics\\dx12-demo\\fonts\\Inter-Regular.otf";
    };
}

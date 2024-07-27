#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <memory>
#include <tuple>
#include <functional>
#include "Rendering/FrameResouce.h"
#include "Rendering/Mesh.hpp"
#include "Rendering/DxMathHelper.h"
#include "Rendering/DescriptorHeap.h"
#include "Core/GameObject.h"

namespace dx12demo
{
    class RenderPipeline
    {
    public:
        RenderPipeline(int width, int height, int meshCount);
        ~RenderPipeline() = default;

        void Resize(int width, int height);
        void Render(const std::vector<std::unique_ptr<GameObject>>& gameObjects, std::function<void(ID3D12GraphicsCommandList*)> action);

        bool GetEnableMSAA() const { return m_EnableMSAA; }
        void SetEnableMSAA(bool value)
        {
            m_EnableMSAA = value;
            Resize(m_RenderTargetWidth, m_RenderTargetHeight);
        }

        bool GetIsWireframe() const { return m_IsWireframe; }
        void SetIsWireframe(bool value) { m_IsWireframe = value; }

        ID3D12Resource* GetResolvedColorTarget() const { return m_ResolvedColorTarget.Get(); }

        int GetFrameResourceCount() const { return m_FrameResources.size(); }

    private:
        void CheckMSAAQuailty();
        void CreateCommandObjects();
        void CreateFrameResources();
        void CreateDescriptorHeaps();
        void CreateRootSignature();
        void CreateShaderAndPSO();

        void ExecuteCommandList();
        D3D12_CPU_DESCRIPTOR_HANDLE GetColorRenderTargetView();
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilTargetView();

    private:
        int m_MeshCount;

        bool m_EnableMSAA = false;
        UINT m_MSAAQuality;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

        std::unique_ptr<DescriptorHeap> m_RtvHeap;
        std::unique_ptr<DescriptorHeap> m_DsvHeap;

        Microsoft::WRL::ComPtr<ID3DBlob> m_VSByteCode;
        Microsoft::WRL::ComPtr<ID3DBlob> m_PSByteCode;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSONormal;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSOWireframe;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSONormalMSAA;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSOWireframeMSAA;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_ColorTarget;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_ResolvedColorTarget;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilTarget;
        D3D12_RESOURCE_STATES m_LastColorTargetState;
        D3D12_RESOURCE_STATES m_LastResolvedColorTargetState;

        int m_RenderTargetWidth;
        int m_RenderTargetHeight;
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;

        bool m_IsWireframe = false;

        std::vector<std::unique_ptr<FrameResource>> m_FrameResources{};
        int m_CurrentFrameResourceIndex = 0;

        std::vector<ComPtr<ID3D12Resource>> m_TempResources{};

        float m_Theta = 1.5f * DirectX::XM_PI;
        float m_Phi = DirectX::XM_PIDIV4;
        float m_Radius = 5.0f;

    private:
        DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        UINT m_MSAASampleCount = 4;
    };
};

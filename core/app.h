#pragma once

#include <directx/d3dx12.h>
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <string>
#include <DirectXMath.h>
#include "timer.h"
#include "dx_exception.h"
#include "mesh.hpp"
#include "upload_buffer.hpp"
#include "math_helper.h"

using Microsoft::WRL::ComPtr;

namespace dx12demo
{
    struct ObjConsts
    {
        DirectX::XMFLOAT4X4 MatrixMVP;
    };

    class BaseWinApp
    {
    protected:
        BaseWinApp(HINSTANCE hInstance);
        BaseWinApp(const BaseWinApp& rhs) = delete;
        BaseWinApp& operator=(const BaseWinApp& rhs) = delete;
        virtual ~BaseWinApp();

    public:
        static void ShowError(const std::wstring& message);

        bool Initialize(int nCmdShow);
        int Run();
        virtual LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

        HINSTANCE AppHandle() const { return m_InstanceHandle; }
        HWND WindowHandle() const { return m_WindowHandle; }
        float DeltaTime() const { return m_Timer.DeltaTime(); }
        float ElapsedTime() const { return m_Timer.ElapsedTime(); }
        D3D12_CPU_DESCRIPTOR_HANDLE BackBufferView() const
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
                m_CurrentBackBufferIndex,
                m_RtvDescriptorSize);
        }
        D3D12_CPU_DESCRIPTOR_HANDLE OffScreenRenderTargetBufferView() const
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
                m_SwapChainBufferCount, // the last one
                m_RtvDescriptorSize);
        }
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const
        {
            return m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
        }
        ID3D12Resource* BackBuffer() const
        {
            return m_SwapChainBuffers[m_CurrentBackBufferIndex].Get();
        }

        void SetMSAAState(bool enable)
        {
            if (m_EnableMSAA == enable)
            {
                return;
            }

            m_EnableMSAA = enable;
            OutputDebugStringW((L"MSAA State: " + std::to_wstring(enable) + L"\n").c_str());

            // Recreate the swapchain and buffers with new multisample settings.
            // CreateSwapChain();
            OnResize();
        }
        bool GetMSAAState() const { return m_EnableMSAA; }
        float AspectRatio() const { return static_cast<float>(m_ClientWidth) / m_ClientHeight; }

    private:
        bool InitWindow(int nCmdShow);
        bool InitDirect3D();
        void CreateCommandObjects();
        void CreateSwapChain();
        void CreateDescriptorHeaps();
        void CreateRootSignature();
        void CreateShaderAndPSO(std::shared_ptr<dx12demo::Mesh> mesh);
        void FlushCommandQueue();
        void ExecuteCommandList();
        void SwapBackBuffer();
        void CalculateFrameStats();
        void LogAdapters();
        void LogAdapterOutputs(IDXGIAdapter* adapter);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    protected:
        virtual void OnRender();
        virtual void OnResize();
        virtual void OnUpdate();
        virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
        virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
        virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

    private:
        HINSTANCE m_InstanceHandle;
        HWND m_WindowHandle;

        Timer m_Timer{};

        ComPtr<IDXGIFactory4> m_Factory;
        ComPtr<ID3D12Device> m_Device;
        ComPtr<ID3D12Fence> m_Fence;
        UINT64 m_FenceValue = 0;

        UINT m_RtvDescriptorSize;
        UINT m_DsvDescriptorSize;
        UINT m_CbvSrvUavDescriptorSize;

        bool m_EnableMSAA = true;
        UINT m_MSAAQuality;

        ComPtr<ID3D12CommandQueue> m_CommandQueue;
        ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        ComPtr<IDXGISwapChain> m_SwapChain;
        static const int m_SwapChainBufferCount = 2;
        ComPtr<ID3D12Resource> m_SwapChainBuffers[m_SwapChainBufferCount];
        ComPtr<ID3D12Resource> m_OffScreenRenderTargetBuffer;
        D3D12_RESOURCE_STATES m_LastOffScreenRenderTargetBufferState;
        ComPtr<ID3D12Resource> m_DepthStencilBuffer;

        ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
        ComPtr<ID3D12DescriptorHeap> m_CbvHeap;
        int m_CurrentBackBufferIndex = 0;

        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;
        bool m_IsResizing = false;
        bool m_IsMinimized = false;
        bool m_IsMaximized = false;

        std::vector<std::shared_ptr<Mesh>> m_Meshes{};
        std::unique_ptr<UploadBuffer<ObjConsts>> m_PerObjConstsBuffer;
        ComPtr<ID3D12RootSignature> m_RootSignature;
        ComPtr<ID3DBlob> m_VSByteCode;
        ComPtr<ID3DBlob> m_PSByteCode;
        ComPtr<ID3D12PipelineState> m_PSO;

        DirectX::XMFLOAT4X4 m_World = dx12demo::MathHelper::Identity4x4();
        DirectX::XMFLOAT4X4 m_View = dx12demo::MathHelper::Identity4x4();
        DirectX::XMFLOAT4X4 m_Proj = dx12demo::MathHelper::Identity4x4();

        float m_Theta = 1.5f * DirectX::XM_PI;
        float m_Phi = DirectX::XM_PIDIV4;
        float m_Radius = 5.0f;

        POINT m_LastMousePos;

        DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        int m_ClientWidth = 800;
        int m_ClientHeight = 600;
        std::wstring m_WindowTitle = L"DX12 Demo";
    };
}

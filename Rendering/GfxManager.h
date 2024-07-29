#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <Windows.h>
#include <memory>
#include <queue>
#include <vector>

namespace dx12demo
{
    class GfxManager
    {
    public:
        GfxManager() = default;
        ~GfxManager();

        void Initialize(HWND window, int width, int height);
        UINT64 SignalNextFenceValue();
        void WaitForFence(UINT64 fence);
        void WaitForGpuIdle();
        void ResizeBackBuffer(int width, int height);
        void WaitForFameLatency() const;
        void Present();
        void LogAdapters(DXGI_FORMAT format);

        IDXGIFactory4* GetFactory() const { return m_Factory.Get(); }
        ID3D12Device* GetDevice() const { return m_Device.Get(); }
        ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }

        UINT64 GetCompletedFenceValue() const { return m_Fence->GetCompletedValue(); }
        UINT64 GetCurrentFenceValue() const { return m_FenceCurrentValue; }
        UINT64 GetNextFenceValue() const { return m_FenceCurrentValue + 1; }

        ID3D12Resource* GetBackBuffer() const { return m_SwapChainBuffers[m_CurrentBackBufferIndex].Get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView() const
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
            return handle.Offset(m_CurrentBackBufferIndex, m_RtvDescriptorSize);
        }

        DXGI_FORMAT GetBackBufferFormat() const { return m_BackBufferFormat; }
        D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_CommandListType; }

    private:
        void InitDeviceAndFactory();
        void InitDebugInfoCallback();
        void InitCommandObjectsAndFence();
        void InitDescriptorHeaps();
        void InitSwapChain(HWND window, int width, int height);

        void LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device = nullptr;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> m_DebugInfoQueue = nullptr;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
        UINT64 m_FenceCurrentValue = 0;
        HANDLE m_FenceEventHandle;

        UINT m_RtvDescriptorSize;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap = nullptr;

        static const int s_SwapChainBufferCount = 3; // TODO: why 2 will make msaa broken
        Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_SwapChainBuffers[s_SwapChainBufferCount];
        int m_CurrentBackBufferIndex = 0;
        HANDLE m_FrameLatencyWaitEventHandle;

        const DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        const D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    };

    GfxManager& GetGfxManager();
}

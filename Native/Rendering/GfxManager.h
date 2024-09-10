#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include "Rendering/DescriptorHeap.h"
#include <dxgi1_4.h>
#include <wrl.h>
#include <Windows.h>
#include <memory>
#include <queue>
#include <vector>
#include <stdexcept>

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

        void SafeReleaseObject(ID3D12Object* obj);

        IDXGIFactory4* GetFactory() const { return m_Factory.Get(); }
        ID3D12Device* GetDevice() const { return m_Device.Get(); }
        ID3D12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
        {
            if (type != GetCommandListType())
            {
                throw std::runtime_error("Command list type is not supported");
            }

            return m_CommandQueue.Get();
        }

        UINT64 GetCompletedFenceValue() const { return m_Fence->GetCompletedValue(); }
        UINT64 GetCurrentFenceValue() const { return m_FenceCurrentValue; }
        UINT64 GetNextFenceValue() const { return m_FenceCurrentValue + 1; }

        ID3D12Resource* GetBackBuffer() const { return m_SwapChainBuffers[m_CurrentBackBufferIndex].Get(); }

        DXGI_FORMAT GetBackBufferFormat() const { return m_BackBufferFormat; }
        D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_CommandListType; }
        UINT GetMaxFrameLatency() const { return m_MaxFrameLatency; }

    private:
        void InitDeviceAndFactory();
        void InitDebugInfoCallback();
        void InitCommandObjectsAndFence();
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

        static const int s_SwapChainBufferCount = 2;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_SwapChainBuffers[s_SwapChainBufferCount];
        int m_CurrentBackBufferIndex = 0;
        HANDLE m_FrameLatencyWaitEventHandle;

        std::queue<std::pair<ID3D12Object*, UINT64>> m_ReleaseQueue{};

        std::unique_ptr<DescriptorTableAllocator> m_ViewHeapAllocator;
        std::unique_ptr<DescriptorTableAllocator> m_SamplerHeapAllocator;

        const DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        const D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
        const UINT m_MaxFrameLatency = 3;
    };

    GfxManager& GetGfxManager();
}

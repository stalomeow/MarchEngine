#pragma once

#include "GfxDescriptorHeap.h"
#include <directx/d3dx12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <Windows.h>
#include <stdint.h>

namespace march
{
    class GfxDevice;

    class GfxSwapChain
    {
    public:
        GfxSwapChain(GfxDevice* device, HWND hWnd, uint32_t width, uint32_t height);
        ~GfxSwapChain();

        // 调用 Resize 前，必须保证 GPU Idle
        void Resize(uint32_t width, uint32_t height);
        void WaitForFrameLatency() const;
        void Present();

        ID3D12Resource* GetBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex].Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRtv() const { return m_BackBufferRtvHandles[m_CurrentBackBufferIndex].GetCpuHandle(); }

    private:
        void CreateBackBuffers();

    public:
        static const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        static const uint32_t BackBufferCount = 2;
        static const uint32_t MaxFrameLatency = 3;

    private:
        GfxDevice* m_Device;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain;
        HANDLE m_FrameLatencyHandle;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_BackBuffers[BackBufferCount];
        GfxDescriptorHandle m_BackBufferRtvHandles[BackBufferCount];
        uint32_t m_CurrentBackBufferIndex;
    };
}

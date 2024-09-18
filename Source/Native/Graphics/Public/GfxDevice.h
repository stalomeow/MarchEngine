#pragma once

#include <directx/d3dx12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <memory>
#include <queue>
#include <stdint.h>
#include <Windows.h>

namespace march
{
    class GfxCommandQueue;
    class GfxFence;
    class GfxCommandAllocatorPool;
    class GfxCommandList;
    class GfxUploadMemory;
    class GfxUploadMemoryAllocator;
    class GfxSwapChain;

    class GfxDevice
    {
    public:
        GfxDevice(HWND hWnd, uint32_t width, uint32_t height);
        ~GfxDevice();

        IDXGIFactory4* GetDXGIFactory() const { return m_Factory.Get(); }
        ID3D12Device4* GetD3D12Device() const { return m_Device.Get(); }
        GfxCommandQueue* GetGraphicsCommandQueue() const { return m_GraphicsCommandQueue.get(); }
        GfxFence* GetGraphicsFence() const { return m_GraphicsFence.get(); }
        GfxCommandList* GetGraphicsCommandList() const { return m_GraphicsCommandList.get(); }

        void BeginFrame();
        void EndFrame();
        void ReleaseD3D12Object(ID3D12Object* object);
        void WaitForIdle();

        GfxUploadMemory AllocateTransientUploadMemory(uint32_t size, uint32_t count = 1, uint32_t alignment = 1);

    protected:
        void ProcessReleaseQueue();

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> m_DebugInfoQueue;

        std::unique_ptr<GfxCommandQueue> m_GraphicsCommandQueue;
        std::unique_ptr<GfxFence> m_GraphicsFence;
        std::unique_ptr<GfxCommandAllocatorPool> m_GraphicsCommandAllocatorPool;
        std::unique_ptr<GfxCommandList> m_GraphicsCommandList;
        std::unique_ptr<GfxUploadMemoryAllocator> m_UploadMemoryAllocator;
        std::unique_ptr<GfxSwapChain> m_SwapChain;

        std::queue<std::pair<uint64_t, ID3D12Object*>> m_ReleaseQueue;
    };

    GfxDevice* GetGfxDevice();
    void InitGfxDevice(HWND hWnd, uint32_t width, uint32_t height);
}

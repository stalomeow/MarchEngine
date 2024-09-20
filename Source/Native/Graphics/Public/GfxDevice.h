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
    class GfxDescriptorHandle;
    class GfxDescriptorAllocator;
    class GfxDescriptorTable;
    class GfxDescriptorTableAllocator;
    class GfxSwapChain;

    enum class GfxDescriptorTableType;

    struct GfxDeviceDesc
    {
        HWND WindowHandle;
        uint32_t WindowWidth;
        uint32_t WindowHeight;
        uint32_t ViewTableStaticDescriptorCount;
        uint32_t ViewTableDynamicDescriptorCapacity;
        uint32_t SamplerTableStaticDescriptorCount;
        uint32_t SamplerTableDynamicDescriptorCapacity;
    };

    class GfxDevice
    {
    public:
        GfxDevice(const GfxDeviceDesc& desc);
        ~GfxDevice();

        IDXGIFactory4* GetDXGIFactory() const { return m_Factory.Get(); }
        ID3D12Device4* GetD3D12Device() const { return m_Device.Get(); }
        GfxCommandQueue* GetGraphicsCommandQueue() const { return m_GraphicsCommandQueue.get(); }
        GfxCommandList* GetGraphicsCommandList() const { return m_GraphicsCommandList.get(); }
        GfxDescriptorTableAllocator* GetViewDescriptorTableAllocator() const { return m_ViewDescriptorTableAllocator.get(); }
        GfxDescriptorTableAllocator* GetSamplerDescriptorTableAllocator() const { return m_SamplerDescriptorTableAllocator.get(); }

        void BeginFrame();
        void EndFrame();
        void ReleaseD3D12Object(ID3D12Object* object);
        bool IsGraphicsFenceCompleted(uint64_t fenceValue);
        void WaitForIdle();

        void ResizeBackBuffer(uint32_t width, uint32_t height);
        void SetBackBufferAsRenderTarget();
        DXGI_FORMAT GetBackBufferFormat() const;
        uint32_t GetMaxFrameLatency() const;

        GfxDescriptorHandle AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);
        void FreeDescriptor(const GfxDescriptorHandle& handle);

        GfxUploadMemory AllocateTransientUploadMemory(uint32_t size, uint32_t count = 1, uint32_t alignment = 1);
        GfxDescriptorTable AllocateTransientDescriptorTable(GfxDescriptorTableType type, uint32_t descriptorCount);
        GfxDescriptorTable GetStaticDescriptorTable(GfxDescriptorTableType type);

        void LogAdapters(DXGI_FORMAT format);

    protected:
        void ProcessReleaseQueue();
        void LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> m_DebugInfoQueue;

        std::unique_ptr<GfxCommandQueue> m_GraphicsCommandQueue;
        std::unique_ptr<GfxFence> m_GraphicsFence;
        std::unique_ptr<GfxCommandAllocatorPool> m_GraphicsCommandAllocatorPool;
        std::unique_ptr<GfxCommandList> m_GraphicsCommandList;
        std::unique_ptr<GfxUploadMemoryAllocator> m_UploadMemoryAllocator;
        std::unique_ptr<GfxDescriptorAllocator> m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        std::unique_ptr<GfxDescriptorTableAllocator> m_ViewDescriptorTableAllocator;
        std::unique_ptr<GfxDescriptorTableAllocator> m_SamplerDescriptorTableAllocator;
        std::unique_ptr<GfxSwapChain> m_SwapChain;

        std::queue<std::pair<uint64_t, ID3D12Object*>> m_ReleaseQueue;
    };

    GfxDevice* GetGfxDevice();
    void InitGfxDevice(const GfxDeviceDesc& desc);
}

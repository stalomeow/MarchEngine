#pragma once

#include "Engine/Memory/RefCounting.h"
#include <directx/d3dx12.h>
#include <dxgi1_5.h>
#include <wrl.h>
#include <memory>
#include <queue>
#include <stdint.h>

namespace march
{
    enum class GfxCommandType;
    class GfxCommandManager;
    class GfxCommandContext;

    class GfxOfflineDescriptorAllocator;
    class GfxOnlineDescriptorMultiAllocator;

    class GfxResourceAllocator;
    class GfxBufferSubAllocator;

    struct GfxDeviceDesc
    {
        bool EnableDebugLayer;
        uint32_t OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        uint32_t OnlineViewDescriptorHeapSize;
        uint32_t OnlineSamplerDescriptorHeapSize;
    };

    class GfxDevice final
    {
    public:
        GfxDevice(const GfxDeviceDesc& desc);
        ~GfxDevice();

        IDXGIFactory5* GetDXGIFactory() const { return m_Factory.Get(); }
        ID3D12Device4* GetD3DDevice4() const { return m_Device.Get(); }

        void CleanupResources();

        GfxCommandManager* GetCommandManager() const { return m_CommandManager.get(); }
        GfxCommandContext* RequestContext(GfxCommandType type);

        uint64_t GetCompletedFence();
        bool IsFenceCompleted(uint64_t fence);
        uint64_t GetNextFence() const;

        GfxOfflineDescriptorAllocator* GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const { return m_OfflineDescriptorAllocators[type].get(); }
        GfxOnlineDescriptorMultiAllocator* GetOnlineViewDescriptorAllocator() const { return m_OnlineViewAllocator.get(); }
        GfxOnlineDescriptorMultiAllocator* GetOnlineSamplerDescriptorAllocator() const { return m_OnlineSamplerAllocator.get(); }

        GfxResourceAllocator* GetCommittedAllocator(D3D12_HEAP_TYPE heapType) const;
        GfxResourceAllocator* GetPlacedBufferAllocator(D3D12_HEAP_TYPE heapType) const;
        GfxResourceAllocator* GetDefaultHeapPlacedTextureAllocator(bool render, bool msaa) const;
        GfxBufferSubAllocator* GetUploadHeapBufferSubAllocator(bool fastOneFrame) const;

        void DeferredRelease(RefCountPtr<ThreadSafeRefCountedObject> obj);

        uint32_t GetMSAAQuality(DXGI_FORMAT format, uint32_t sampleCount);

        void LogAdapters(DXGI_FORMAT format);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory5> m_Factory;
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> m_DebugInfoQueue;

        std::unique_ptr<GfxCommandManager> m_CommandManager;

        std::unique_ptr<GfxOfflineDescriptorAllocator> m_OfflineDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        std::unique_ptr<GfxOnlineDescriptorMultiAllocator> m_OnlineViewAllocator;
        std::unique_ptr<GfxOnlineDescriptorMultiAllocator> m_OnlineSamplerAllocator;

        std::unique_ptr<GfxResourceAllocator> m_DefaultHeapCommittedAllocator;
        std::unique_ptr<GfxResourceAllocator> m_UploadHeapCommittedAllocator;
        std::unique_ptr<GfxResourceAllocator> m_DefaultHeapPlacedAllocatorBuffer;
        std::unique_ptr<GfxResourceAllocator> m_DefaultHeapPlacedAllocatorTexture;
        std::unique_ptr<GfxResourceAllocator> m_DefaultHeapPlacedAllocatorRenderTexture;
        std::unique_ptr<GfxResourceAllocator> m_DefaultHeapPlacedAllocatorRenderTextureMS;
        std::unique_ptr<GfxResourceAllocator> m_UploadHeapPlacedAllocatorBuffer;
        std::unique_ptr<GfxBufferSubAllocator> m_UploadHeapBufferSubAllocator;
        std::unique_ptr<GfxBufferSubAllocator> m_UploadHeapBufferSubAllocatorFastOneFrame;

        std::queue<std::pair<uint64_t, RefCountPtr<ThreadSafeRefCountedObject>>> m_ReleaseQueue;

        void LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
    };

    GfxDevice* GetGfxDevice();
    GfxDevice* InitGfxDevice(const GfxDeviceDesc& desc);
    void DestroyGfxDevice();
}

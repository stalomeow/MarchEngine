#pragma once

#include <directx/d3dx12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <memory>
#include <queue>
#include <string>
#include <exception>
#include <stdint.h>
#include <Windows.h>

namespace march
{
    enum class GfxCommandType;
    class GfxCommandManager;
    class GfxCommandContext;

    class GfxOfflineDescriptorAllocator;
    class GfxOnlineDescriptorMultiAllocator;

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

        IDXGIFactory4* GetDXGIFactory() const { return m_Factory.Get(); }
        ID3D12Device4* GetD3DDevice4() const { return m_Device.Get(); }

        void EndFrame();
        void WaitForGpuIdle(bool releaseUnusedObjects = true);

        GfxCommandManager* GetCommandManager() const { return m_CommandManager.get(); }
        GfxCommandContext* RequestContext(GfxCommandType type);

        uint64_t GetCompletedFence(bool useCache = false);
        bool IsFenceCompleted(uint64_t fence, bool useCache = false);
        uint64_t GetNextFence() const;

        GfxOfflineDescriptorAllocator* GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const { return m_OfflineDescriptorAllocators[type].get(); }
        GfxOnlineDescriptorMultiAllocator* GetOnlineViewDescriptorAllocator() const { return m_OnlineViewAllocator.get(); }
        GfxOnlineDescriptorMultiAllocator* GetOnlineSamplerDescriptorAllocator() const { return m_OnlineSamplerAllocator.get(); }

        GfxCompleteResourceAllocator* GetResourceAllocator(GfxAllocator allocator, GfxAllocation allocation) const;
        GfxBufferSubAllocator* GetResourceAllocator(GfxSubAllocator subAllocator) const;

        uint32_t GetMSAAQuality(DXGI_FORMAT format, uint32_t sampleCount);

        void LogAdapters(DXGI_FORMAT format);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> m_DebugInfoQueue;

        std::unique_ptr<GfxCommandManager> m_CommandManager;
        std::queue<std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12Object>>> m_ReleaseQueue;

        // 下面的 allocator 在析构时会使用 m_ReleaseQueue

        std::unique_ptr<GfxOfflineDescriptorAllocator> m_OfflineDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        std::unique_ptr<GfxOnlineDescriptorMultiAllocator> m_OnlineViewAllocator;
        std::unique_ptr<GfxOnlineDescriptorMultiAllocator> m_OnlineSamplerAllocator;

        std::unique_ptr<GfxCompleteResourceAllocator> m_CommittedDefaultAllocator;
        std::unique_ptr<GfxCompleteResourceAllocator> m_CommittedUploadAllocator;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedDefaultAllocatorBuffer;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedDefaultAllocatorTexture;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedDefaultAllocatorRenderTexture;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedDefaultAllocatorRenderTextureMS;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedUploadAllocatorBuffer;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedUploadAllocatorTexture;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedUploadAllocatorRenderTexture;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedUploadAllocatorRenderTextureMS;
        std::unique_ptr<GfxBufferSubAllocator> m_TempUploadSubAllocator;
        std::unique_ptr<GfxBufferSubAllocator> m_PersistentUploadSubAllocator;

        void ProcessReleaseQueue();
        void LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
    };

    class GfxException : public std::exception
    {
    public:
        explicit GfxException(const std::string& message);

        char const* what() const override;

    private:
        std::string m_Message;
    };

    class GfxHResultException : public std::exception
    {
    public:
        GfxHResultException(HRESULT hr, const std::string& expr, const std::string& filename, int line);

        char const* what() const override;

    private:
        std::string m_Message;
    };

    GfxDevice* GetGfxDevice();
    GfxDevice* InitGfxDevice(const GfxDeviceDesc& desc);
    void DestroyGfxDevice();
}

#define GFX_HR(x) \
{ \
    HRESULT ___hr___ = (x); \
    if (FAILED(___hr___)) { throw ::march::GfxHResultException(___hr___, #x, __FILE__, __LINE__); } \
}

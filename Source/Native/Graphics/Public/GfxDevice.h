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
#include <functional>

namespace march
{
    class GfxSwapChain;
    class GfxRenderTexture;

    class GfxOfflineDescriptorAllocator;
    class GfxOnlineDescriptorMultiAllocator;

    class GfxCompleteResourceAllocator;
    class GfxBufferSubAllocator;
    enum class GfxAllocator;
    enum class GfxSubAllocator;

    enum class GfxCommandType;
    class GfxCommandManager;
    class GfxCommandContext;

    struct GfxDeviceDesc
    {
        bool EnableDebugLayer;
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
        ID3D12Device4* GetD3DDevice4() const { return m_Device.Get(); }

        GfxCommandManager* GetCommandManager() const;
        GfxCommandContext* RequestContext(GfxCommandType type);

        uint64_t GetCompletedFrameFence(bool useCache = false);
        bool IsFrameFenceCompleted(uint64_t fence, bool useCache = false);
        uint64_t GetNextFrameFence() const;

        void BeginFrame();
        void EndFrame();
        void DeferredRelease(Microsoft::WRL::ComPtr<ID3D12Object> obj);
        void WaitForIdle();
        void WaitForIdleAndReleaseUnusedD3D12Objects();

        void ResizeBackBuffer(uint32_t width, uint32_t height);
        GfxRenderTexture* GetBackBuffer() const;
        uint32_t GetMaxFrameLatency() const;

        GfxOfflineDescriptorAllocator* GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type);
        GfxOnlineDescriptorMultiAllocator* GetOnlineViewDescriptorAllocator() const;
        GfxOnlineDescriptorMultiAllocator* GetOnlineSamplerDescriptorAllocator() const;

        GfxCompleteResourceAllocator* GetResourceAllocator(GfxAllocator allocator) const;
        GfxBufferSubAllocator* GetResourceAllocator(GfxSubAllocator subAllocator) const;

        uint32_t GetMSAAQuality(DXGI_FORMAT format, uint32_t sampleCount);

        void LogAdapters(DXGI_FORMAT format);

    protected:
        void ProcessReleaseQueue();
        void LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> m_DebugInfoQueue;

        std::unique_ptr<GfxCommandManager> m_CommandManager;
        std::unique_ptr<GfxSwapChain> m_SwapChain;
        std::queue<std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12Object>>> m_ReleaseQueue;

        std::unique_ptr<GfxOfflineDescriptorAllocator> m_OfflineDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        std::unique_ptr<GfxOnlineDescriptorMultiAllocator> m_OnlineViewAllocator;
        std::unique_ptr<GfxOnlineDescriptorMultiAllocator> m_OnlineSamplerAllocator;

        std::unique_ptr<GfxCompleteResourceAllocator> m_CommittedDefaultAllocator;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedDefaultAllocator;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedDefaultMSAllocatorMS;
        std::unique_ptr<GfxCompleteResourceAllocator> m_CommittedUploadAllocator;
        std::unique_ptr<GfxCompleteResourceAllocator> m_PlacedUploadAllocator;
        std::unique_ptr<GfxBufferSubAllocator> m_TempUploadSubAllocator;
        std::unique_ptr<GfxBufferSubAllocator> m_PersistentUploadSubAllocator;
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
    void InitGfxDevice(const GfxDeviceDesc& desc);
    void DestroyGfxDevice();
}

#define GFX_HR(x) \
{ \
    HRESULT ___hr___ = (x); \
    if (FAILED(___hr___)) { throw ::march::GfxHResultException(___hr___, #x, __FILE__, __LINE__); } \
}

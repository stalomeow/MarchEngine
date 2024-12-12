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
    class GfxCommandQueue;
    class GfxCommandList;
    class GfxUploadMemory;
    class GfxUploadMemoryAllocator;
    class GfxDescriptorHandle;
    class GfxDescriptorAllocator;
    class GfxDescriptorTable;
    class GfxDescriptorTableAllocator;
    class GfxSwapChain;
    class GfxRenderTexture;
    class GfxResourceAllocator;
    class GfxSubBufferMultiBuddyAllocator;

    enum class GfxDescriptorTableType;

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
        GfxDescriptorTableAllocator* GetViewDescriptorTableAllocator() const { return m_ViewDescriptorTableAllocator.get(); }
        GfxDescriptorTableAllocator* GetSamplerDescriptorTableAllocator() const { return m_SamplerDescriptorTableAllocator.get(); }

        GfxCommandManager* GetCommandManager() const { return m_CommandManager.get(); }
        GfxCommandContext* RequestContext(GfxCommandType type);

        void BeginFrame();
        void EndFrame();
        void DeferredRelease(const std::function<void()>& callback, Microsoft::WRL::ComPtr<ID3D12Object> obj = nullptr);
        bool IsGraphicsFenceCompleted(uint64_t fenceValue);
        void WaitForIdle();
        void WaitForIdleAndReleaseUnusedD3D12Objects();

        void ResizeBackBuffer(uint32_t width, uint32_t height);
        GfxRenderTexture* GetBackBuffer() const;
        uint32_t GetMaxFrameLatency() const;

        GfxDescriptorHandle AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);
        void FreeDescriptor(const GfxDescriptorHandle& handle);

        GfxUploadMemory AllocateTransientUploadMemory(uint32_t size, uint32_t count = 1, uint32_t alignment = 1);
        GfxDescriptorTable AllocateTransientDescriptorTable(GfxDescriptorTableType type, uint32_t descriptorCount);
        GfxDescriptorTable GetStaticDescriptorTable(GfxDescriptorTableType type);

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

        std::unique_ptr<GfxUploadMemoryAllocator> m_UploadMemoryAllocator;
        std::unique_ptr<GfxDescriptorAllocator> m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        std::unique_ptr<GfxDescriptorTableAllocator> m_ViewDescriptorTableAllocator;
        std::unique_ptr<GfxDescriptorTableAllocator> m_SamplerDescriptorTableAllocator;
        std::unique_ptr<GfxSwapChain> m_SwapChain;

        std::unique_ptr<GfxResourceAllocator> m_DefaultBufferAllocator;
        std::unique_ptr<GfxResourceAllocator> m_UploadBufferAllocator;
        std::unique_ptr<GfxResourceAllocator> m_ExternalTextureAllocator;
        std::unique_ptr<GfxResourceAllocator> m_RenderTextureAllocatorMSAA;
        std::unique_ptr<GfxResourceAllocator> m_RenderTextureAllocator;

        std::unique_ptr<GfxSubBufferMultiBuddyAllocator> m_UploadConstantBufferAllocator;

        std::unique_ptr<GfxCommandManager> m_CommandManager;

        std::queue<std::pair<uint64_t, ID3D12Object*>> m_ReleaseQueue;
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

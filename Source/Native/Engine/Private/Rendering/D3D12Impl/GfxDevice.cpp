#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Rendering/D3D12Impl/GfxBuffer.h"
#include "Engine/Rendering/D3D12Impl/GfxResource.h"
#include "Engine/Rendering/D3D12Impl/GfxDescriptor.h"
#include "Engine/Debug.h"
#include <assert.h>

using namespace Microsoft::WRL;

namespace march
{
    static void __stdcall D3D12DebugMessageCallback(
        D3D12_MESSAGE_CATEGORY Category,
        D3D12_MESSAGE_SEVERITY Severity,
        D3D12_MESSAGE_ID ID,
        LPCSTR pDescription,
        void* pContext);

    GfxDevice::GfxDevice(const GfxDeviceDesc& desc)
        : m_ReleaseQueue{}
    {
        // 开启调试层
        if (desc.EnableDebugLayer)
        {
            ComPtr<ID3D12Debug> debugController = nullptr;
            GFX_HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
            debugController->EnableDebugLayer();
            LOG_WARNING("D3D12 Debug Layer Enabled");
        }

        GFX_HR(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

        // 默认设备
        HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device));

        // 回退到 WARP 设备
        if (FAILED(hardwareResult))
        {
            ComPtr<IDXGIAdapter> warpAdapter;
            GFX_HR(m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
            GFX_HR(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)));
        }

        // 获取 D3D12 输出的调试信息
        if (desc.EnableDebugLayer)
        {
            if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugInfoQueue))))
            {
                DWORD callbackCookie = 0;

#pragma warning( disable : 6387 )
                GFX_HR(m_DebugInfoQueue->RegisterMessageCallback(D3D12DebugMessageCallback,
                    D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callbackCookie));
#pragma warning( default : 6387 )

                if (callbackCookie == 0)
                {
                    LOG_WARNING("Failed to register D3D12 debug message callback");
                }
            }
            else
            {
                m_DebugInfoQueue = nullptr;
                LOG_WARNING("Failed to get D3D12 debug info queue");
            }
        }
        else
        {
            m_DebugInfoQueue = nullptr;
        }

        m_CommandManager = std::make_unique<GfxCommandManager>(this);

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
        {
            D3D12_DESCRIPTOR_HEAP_TYPE type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
            uint32_t pageSize = desc.OfflineDescriptorPageSizes[i];
            m_OfflineDescriptorAllocators[i] = std::make_unique<GfxOfflineDescriptorAllocator>(this, type, pageSize);
        }

        m_OnlineViewAllocator = std::make_unique<GfxOnlineDescriptorMultiAllocator>(this, [maxSize = desc.OnlineViewDescriptorHeapSize](GfxDevice* device)
        {
            return std::make_unique<GfxOnlineViewDescriptorAllocator>(device, maxSize);
        });
        m_OnlineSamplerAllocator = std::make_unique<GfxOnlineDescriptorMultiAllocator>(this, [maxSize = desc.OnlineSamplerDescriptorHeapSize](GfxDevice* device)
        {
            return std::make_unique<GfxOnlineSamplerDescriptorAllocator>(device, maxSize);
        });

        GfxCommittedResourceAllocatorDesc defaultHeapCommittedDesc{};
        defaultHeapCommittedDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapCommittedDesc.HeapFlags = D3D12_HEAP_FLAG_NONE;
        m_DefaultHeapCommittedAllocator = std::make_unique<GfxCommittedResourceAllocator>(this, defaultHeapCommittedDesc);

        GfxCommittedResourceAllocatorDesc uploadHeapCommittedDesc{};
        uploadHeapCommittedDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapCommittedDesc.HeapFlags = D3D12_HEAP_FLAG_NONE;
        m_UploadHeapCommittedAllocator = std::make_unique<GfxCommittedResourceAllocator>(this, uploadHeapCommittedDesc);

        GfxPlacedResourceAllocatorDesc defaultHeapPlacedBufferDesc{};
        defaultHeapPlacedBufferDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        defaultHeapPlacedBufferDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapPlacedBufferDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
        defaultHeapPlacedBufferDesc.MSAA = false;
        m_DefaultHeapPlacedAllocatorBuffer = std::make_unique<GfxPlacedResourceAllocator>(this, "DefaultHeapPlacedBufferAllocator", defaultHeapPlacedBufferDesc);

        GfxPlacedResourceAllocatorDesc defaultHeapPlacedTextureDesc{};
        defaultHeapPlacedTextureDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        defaultHeapPlacedTextureDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapPlacedTextureDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        defaultHeapPlacedTextureDesc.MSAA = false;
        m_DefaultHeapPlacedAllocatorTexture = std::make_unique<GfxPlacedResourceAllocator>(this, "DefaultHeapPlacedTextureAllocator", defaultHeapPlacedTextureDesc);

        GfxPlacedResourceAllocatorDesc defaultHeapPlacedRenderTextureDesc{};
        defaultHeapPlacedRenderTextureDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        defaultHeapPlacedRenderTextureDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapPlacedRenderTextureDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        defaultHeapPlacedRenderTextureDesc.MSAA = false;
        m_DefaultHeapPlacedAllocatorRenderTexture = std::make_unique<GfxPlacedResourceAllocator>(this, "DefaultHeapPlacedRenderTextureAllocator", defaultHeapPlacedRenderTextureDesc);

        GfxPlacedResourceAllocatorDesc defaultHeapPlacedRenderTextureMSDesc{};
        defaultHeapPlacedRenderTextureMSDesc.DefaultMaxBlockSize = 64 * 1024 * 1024; // 64MB
        defaultHeapPlacedRenderTextureMSDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapPlacedRenderTextureMSDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        defaultHeapPlacedRenderTextureMSDesc.MSAA = true;
        m_DefaultHeapPlacedAllocatorRenderTextureMS = std::make_unique<GfxPlacedResourceAllocator>(this, "DefaultHeapPlacedRenderTextureMultisampleAllocator", defaultHeapPlacedRenderTextureMSDesc);

        GfxPlacedResourceAllocatorDesc uploadHeapPlacedBufferDesc{};
        uploadHeapPlacedBufferDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        uploadHeapPlacedBufferDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapPlacedBufferDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
        uploadHeapPlacedBufferDesc.MSAA = false;
        m_UploadHeapPlacedAllocatorBuffer = std::make_unique<GfxPlacedResourceAllocator>(this, "UploadHeapPlacedBufferAllocator", uploadHeapPlacedBufferDesc);

        GfxBufferMultiBuddySubAllocatorDesc uploadHeapSubBufferDesc{};
        uploadHeapSubBufferDesc.MinBlockSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // 目前主要用来分配 constant buffer
        uploadHeapSubBufferDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        m_UploadHeapBufferSubAllocator = std::make_unique<GfxBufferMultiBuddySubAllocator>("UploadHeapBufferSubAllocator", uploadHeapSubBufferDesc,
            /* page allocator */ m_UploadHeapCommittedAllocator.get());

        GfxBufferLinearSubAllocatorDesc uploadHeapSubBufferFastOneFrameDesc{};
        uploadHeapSubBufferFastOneFrameDesc.PageSize = 16 * 1024 * 1024; // 16MB
        m_UploadHeapBufferSubAllocatorFastOneFrame = std::make_unique<GfxBufferLinearSubAllocator>("UploadHeapBufferSubAllocatorFastOneFrame", uploadHeapSubBufferFastOneFrameDesc,
            /* page allocator */ m_UploadHeapCommittedAllocator.get(),
            /* large page allocator */ m_UploadHeapPlacedAllocatorBuffer.get());
    }

    GfxDevice::~GfxDevice()
    {
        WaitForGpuIdle(true);
    }

    void GfxDevice::EndFrame()
    {
        RefreshCompletedFrameFenceAndProcessReleaseQueue();

        m_OnlineViewAllocator->CleanUpAllocations();
        m_OnlineSamplerAllocator->CleanUpAllocations();
        m_UploadHeapBufferSubAllocator->CleanUpAllocations();
        m_UploadHeapBufferSubAllocatorFastOneFrame->CleanUpAllocations();

        m_CommandManager->SignalNextFrameFence();
    }

    void GfxDevice::WaitForGpuIdle(bool releaseUnusedObjects)
    {
        m_CommandManager->WaitForGpuIdle();

        if (releaseUnusedObjects)
        {
            RefreshCompletedFrameFenceAndProcessReleaseQueue();
            assert(m_ReleaseQueue.empty());
        }
    }

    void GfxDevice::RefreshCompletedFrameFenceAndProcessReleaseQueue()
    {
        m_CommandManager->RefreshCompletedFrameFence();

        while (!m_ReleaseQueue.empty() && m_CommandManager->IsFrameFenceCompleted(m_ReleaseQueue.front().first))
        {
            m_ReleaseQueue.pop();
        }
    }

    GfxCommandContext* GfxDevice::RequestContext(GfxCommandType type)
    {
        return m_CommandManager->RequestAndOpenContext(type);
    }

    uint64_t GfxDevice::GetCompletedFence()
    {
        return m_CommandManager->GetCompletedFrameFence();
    }

    bool GfxDevice::IsFenceCompleted(uint64_t fence)
    {
        return m_CommandManager->IsFrameFenceCompleted(fence);
    }

    uint64_t GfxDevice::GetNextFence() const
    {
        return m_CommandManager->GetNextFrameFence();
    }

    GfxResourceAllocator* GfxDevice::GetCommittedAllocator(D3D12_HEAP_TYPE heapType) const
    {
        switch (heapType)
        {
        case D3D12_HEAP_TYPE_DEFAULT:
            return m_DefaultHeapCommittedAllocator.get();
        case D3D12_HEAP_TYPE_UPLOAD:
            return m_UploadHeapCommittedAllocator.get();
        default:
            throw std::invalid_argument("GfxDevice::GetCommittedAllocator: Invalid heap type");
        }
    }

    GfxResourceAllocator* GfxDevice::GetPlacedBufferAllocator(D3D12_HEAP_TYPE heapType) const
    {
        switch (heapType)
        {
        case D3D12_HEAP_TYPE_DEFAULT:
            return m_DefaultHeapPlacedAllocatorBuffer.get();
        case D3D12_HEAP_TYPE_UPLOAD:
            return m_UploadHeapPlacedAllocatorBuffer.get();
        default:
            throw std::invalid_argument("GfxDevice::GetPlacedBufferAllocator: Invalid heap type");
        }
    }

    GfxResourceAllocator* GfxDevice::GetDefaultHeapPlacedTextureAllocator(bool render, bool msaa) const
    {
        if (render)
        {
            if (msaa)
            {
                return m_DefaultHeapPlacedAllocatorRenderTextureMS.get();
            }

            return m_DefaultHeapPlacedAllocatorRenderTexture.get();
        }
        else
        {
            if (msaa)
            {
                throw std::invalid_argument("GfxDevice::GetDefaultHeapPlacedTextureAllocator: MSAA is not supported for non-render textures");
            }

            return m_DefaultHeapPlacedAllocatorTexture.get();
        }
    }

    GfxBufferSubAllocator* GfxDevice::GetUploadHeapBufferSubAllocator(bool fastOneFrame) const
    {
        if (fastOneFrame)
        {
            return m_UploadHeapBufferSubAllocatorFastOneFrame.get();
        }

        return m_UploadHeapBufferSubAllocator.get();
    }

    void GfxDevice::DeferredRelease(RefCountPtr<ThreadSafeRefCountedObject> obj)
    {
        m_ReleaseQueue.emplace(m_CommandManager->GetNextFrameFence(), obj);
    }

    uint32_t GfxDevice::GetMSAAQuality(DXGI_FORMAT format, uint32_t sampleCount)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = {};
        levels.Format = format;
        levels.SampleCount = static_cast<UINT>(sampleCount);
        levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

        GFX_HR(m_Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels)));
        return static_cast<uint32_t>(levels.NumQualityLevels - 1);
    }

    void GfxDevice::LogAdapters(DXGI_FORMAT format)
    {
        UINT i = 0;
        IDXGIAdapter* adapter = nullptr;

        while (m_Factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC desc;
            GFX_HR(adapter->GetDesc(&desc));

            LOG_INFO(L"***Adapter: {}", desc.Description);

            LogAdapterOutputs(adapter, format);
            adapter->Release();
            ++i;
        }
    }

    void GfxDevice::LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format)
    {
        UINT i = 0;
        IDXGIOutput* output = nullptr;
        while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_OUTPUT_DESC desc;
            GFX_HR(output->GetDesc(&desc));

            LOG_INFO(L"***Output: {}", desc.DeviceName);

            LogOutputDisplayModes(output, format);
            output->Release();
            ++i;
        }
    }

    void GfxDevice::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
    {
        UINT count = 0;
        UINT flags = 0;

        // Call with nullptr to get list count.
        GFX_HR(output->GetDisplayModeList(format, flags, &count, nullptr));

        auto modeList = std::make_unique<DXGI_MODE_DESC[]>(count);
        GFX_HR(output->GetDisplayModeList(format, flags, &count, modeList.get()));

        for (UINT i = 0; i < count; i++)
        {
            auto& x = modeList[i];
            UINT n = x.RefreshRate.Numerator;
            UINT d = x.RefreshRate.Denominator;

            LOG_INFO(L"Width = {}, Height = {}, Refresh = {}/{}", x.Width, x.Height, n, d);
        }
    }

    static void __stdcall D3D12DebugMessageCallback(
        D3D12_MESSAGE_CATEGORY Category,
        D3D12_MESSAGE_SEVERITY Severity,
        D3D12_MESSAGE_ID ID,
        LPCSTR pDescription,
        void* pContext)
    {
        switch (Severity)
        {
        case D3D12_MESSAGE_SEVERITY_INFO:
        case D3D12_MESSAGE_SEVERITY_MESSAGE:
            LOG_INFO("{}", pDescription);
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            LOG_WARNING("{}", pDescription);
            break;

        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        case D3D12_MESSAGE_SEVERITY_ERROR:
            LOG_ERROR("{}", pDescription);
            break;

        default:
            LOG_WARNING("Unknown D3D12_MESSAGE_SEVERITY: {}; {}", static_cast<int>(Severity), pDescription);
            break;
        }
    }

    static std::unique_ptr<GfxDevice> g_GfxDevice = nullptr;

    GfxDevice* GetGfxDevice()
    {
        return g_GfxDevice.get();
    }

    GfxDevice* InitGfxDevice(const GfxDeviceDesc& desc)
    {
        g_GfxDevice = std::make_unique<GfxDevice>(desc);
        return g_GfxDevice.get();
    }

    void DestroyGfxDevice()
    {
        g_GfxDevice.reset();
    }
}

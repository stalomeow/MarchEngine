#include "pch.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxCommand.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/GfxDescriptor.h"
#include "Engine/Debug.h"

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
            LOG_INFO("D3D12 Debug Layer Enabled");
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

        GfxCommittedResourceAllocatorDesc committedDefaultDesc{};
        committedDefaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        committedDefaultDesc.HeapFlags = D3D12_HEAP_FLAG_NONE;
        m_CommittedDefaultAllocator = std::make_unique<GfxCommittedResourceAllocator>(this, committedDefaultDesc);

        GfxCommittedResourceAllocatorDesc committedUploadDesc{};
        committedUploadDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        committedUploadDesc.HeapFlags = D3D12_HEAP_FLAG_NONE;
        m_CommittedUploadAllocator = std::make_unique<GfxCommittedResourceAllocator>(this, committedUploadDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedDefaultBufferDesc{};
        placedDefaultBufferDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        placedDefaultBufferDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        placedDefaultBufferDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
        placedDefaultBufferDesc.MSAA = false;
        m_PlacedDefaultAllocatorBuffer = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedDefaultAllocatorBuffer", placedDefaultBufferDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedDefaultTextureDesc{};
        placedDefaultTextureDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        placedDefaultTextureDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        placedDefaultTextureDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        placedDefaultTextureDesc.MSAA = false;
        m_PlacedDefaultAllocatorTexture = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedDefaultAllocatorTexture", placedDefaultTextureDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedDefaultRenderTextureDesc{};
        placedDefaultRenderTextureDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        placedDefaultRenderTextureDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        placedDefaultRenderTextureDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        placedDefaultRenderTextureDesc.MSAA = false;
        m_PlacedDefaultAllocatorRenderTexture = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedDefaultAllocatorRenderTexture", placedDefaultRenderTextureDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedDefaultRenderTextureMSDesc{};
        placedDefaultRenderTextureMSDesc.DefaultMaxBlockSize = 64 * 1024 * 1024; // 64MB
        placedDefaultRenderTextureMSDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        placedDefaultRenderTextureMSDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        placedDefaultRenderTextureMSDesc.MSAA = true;
        m_PlacedDefaultAllocatorRenderTextureMS = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedDefaultAllocatorRenderTextureMS", placedDefaultRenderTextureMSDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedUploadBufferDesc{};
        placedUploadBufferDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        placedUploadBufferDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        placedUploadBufferDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
        placedUploadBufferDesc.MSAA = false;
        m_PlacedUploadAllocatorBuffer = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedUploadAllocatorBuffer", placedUploadBufferDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedUploadTextureDesc{};
        placedUploadTextureDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        placedUploadTextureDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        placedUploadTextureDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        placedUploadTextureDesc.MSAA = false;
        m_PlacedUploadAllocatorTexture = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedUploadAllocatorTexture", placedUploadTextureDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedUploadRenderTextureDesc{};
        placedUploadRenderTextureDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        placedUploadRenderTextureDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        placedUploadRenderTextureDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        placedUploadRenderTextureDesc.MSAA = false;
        m_PlacedUploadAllocatorRenderTexture = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedUploadAllocatorRenderTexture", placedUploadRenderTextureDesc);

        GfxPlacedResourceMultiBuddyAllocatorDesc placedUploadRenderTextureMSDesc{};
        placedUploadRenderTextureMSDesc.DefaultMaxBlockSize = 64 * 1024 * 1024; // 64MB
        placedUploadRenderTextureMSDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        placedUploadRenderTextureMSDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        placedUploadRenderTextureMSDesc.MSAA = true;
        m_PlacedUploadAllocatorRenderTextureMS = std::make_unique<GfxPlacedResourceMultiBuddyAllocator>(this, "PlacedUploadAllocatorRenderTextureMS", placedUploadRenderTextureMSDesc);

        GfxBufferLinearSubAllocatorDesc tempUploadDesc{};
        tempUploadDesc.PageSize = 16 * 1024 * 1024; // 16MB
        tempUploadDesc.UnorderedAccess = false;
        tempUploadDesc.InitialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
        m_TempUploadSubAllocator = std::make_unique<GfxBufferLinearSubAllocator>("TempUploadSubAllocator", tempUploadDesc,
            /* page allocator */ m_CommittedUploadAllocator.get(),
            /* large page allocator */ m_PlacedUploadAllocatorBuffer.get());

        GfxBufferMultiBuddySubAllocatorDesc persistentUploadDesc{};
        persistentUploadDesc.MinBlockSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // 主要用来分配 constant buffer
        persistentUploadDesc.DefaultMaxBlockSize = 16 * 1024 * 1024; // 16MB
        persistentUploadDesc.UnorderedAccess = false;
        persistentUploadDesc.InitialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
        m_PersistentUploadSubAllocator = std::make_unique<GfxBufferMultiBuddySubAllocator>("PersistentUploadSubAllocator", persistentUploadDesc,
            /* buffer allocator */ m_CommittedUploadAllocator.get());
    }

    GfxDevice::~GfxDevice()
    {
        WaitForGpuIdle(true);
        ProcessReleaseQueue();
    }

    void GfxDevice::EndFrame()
    {
        ProcessReleaseQueue(); // 这里会更新 frame fence cache，放在最前面

        m_OnlineViewAllocator->CleanUpAllocations();
        m_OnlineSamplerAllocator->CleanUpAllocations();
        m_CommittedDefaultAllocator->CleanUpAllocations();
        m_CommittedUploadAllocator->CleanUpAllocations();
        m_PlacedDefaultAllocatorBuffer->CleanUpAllocations();
        m_PlacedDefaultAllocatorTexture->CleanUpAllocations();
        m_PlacedDefaultAllocatorRenderTexture->CleanUpAllocations();
        m_PlacedDefaultAllocatorRenderTextureMS->CleanUpAllocations();
        m_PlacedUploadAllocatorBuffer->CleanUpAllocations();
        m_PlacedUploadAllocatorTexture->CleanUpAllocations();
        m_PlacedUploadAllocatorRenderTexture->CleanUpAllocations();
        m_PlacedUploadAllocatorRenderTextureMS->CleanUpAllocations();
        m_TempUploadSubAllocator->CleanUpAllocations();
        m_PersistentUploadSubAllocator->CleanUpAllocations();

        m_CommandManager->SignalNextFrameFence();
    }

    void GfxDevice::DeferredRelease(ComPtr<ID3D12Object> obj)
    {
        m_ReleaseQueue.emplace(m_CommandManager->GetNextFrameFence(), obj);
    }

    void GfxDevice::WaitForGpuIdle(bool releaseUnusedObjects)
    {
        m_CommandManager->WaitForGpuIdle();

        if (releaseUnusedObjects)
        {
            ProcessReleaseQueue();
        }
    }

    void GfxDevice::ProcessReleaseQueue()
    {
        bool useFenceCache = false;

        while (!m_ReleaseQueue.empty() && IsFenceCompleted(m_ReleaseQueue.front().first, useFenceCache))
        {
            // TODO 释放资源较多时，输出名称太卡了！

            wchar_t name[256];
            UINT size = sizeof(name);
            if (SUCCEEDED(m_ReleaseQueue.front().second->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name)))
            {
                LOG_TRACE(L"Release D3D12Object {}", name);
            }

            m_ReleaseQueue.pop();
            useFenceCache = true;
        }
    }

    GfxCommandContext* GfxDevice::RequestContext(GfxCommandType type)
    {
        return m_CommandManager->RequestAndOpenContext(type);
    }

    uint64_t GfxDevice::GetCompletedFence(bool useCache)
    {
        return m_CommandManager->GetCompletedFrameFence(useCache);
    }

    bool GfxDevice::IsFenceCompleted(uint64_t fence, bool useCache)
    {
        return m_CommandManager->IsFrameFenceCompleted(fence, useCache);
    }

    uint64_t GfxDevice::GetNextFence() const
    {
        return m_CommandManager->GetNextFrameFence();
    }

    GfxCompleteResourceAllocator* GfxDevice::GetResourceAllocator(GfxAllocator allocator, GfxAllocation allocation) const
    {
        switch (allocator)
        {
        case GfxAllocator::CommittedDefault:
            return m_CommittedDefaultAllocator.get();
        case GfxAllocator::CommittedUpload:
            return m_CommittedUploadAllocator.get();
        case GfxAllocator::PlacedDefault:
        {
            switch (allocation)
            {
            case GfxAllocation::Buffer:
                return m_PlacedDefaultAllocatorBuffer.get();
            case GfxAllocation::Texture:
                return m_PlacedDefaultAllocatorTexture.get();
            case GfxAllocation::RenderTexture:
                return m_PlacedDefaultAllocatorRenderTexture.get();
            case GfxAllocation::RenderTextureMS:
                return m_PlacedDefaultAllocatorRenderTextureMS.get();
            default:
                throw std::invalid_argument("Invalid GfxAllocation");
            }
        }
        case GfxAllocator::PlacedUpload:
        {
            switch (allocation)
            {
            case GfxAllocation::Buffer:
                return m_PlacedUploadAllocatorBuffer.get();
            case GfxAllocation::Texture:
                return m_PlacedUploadAllocatorTexture.get();
            case GfxAllocation::RenderTexture:
                return m_PlacedUploadAllocatorRenderTexture.get();
            case GfxAllocation::RenderTextureMS:
                return m_PlacedUploadAllocatorRenderTextureMS.get();
            default:
                throw std::invalid_argument("Invalid GfxAllocation");
            }
        }
        default:
            throw std::invalid_argument("Invalid GfxAllocator");
        }
    }

    GfxBufferSubAllocator* GfxDevice::GetResourceAllocator(GfxSubAllocator subAllocator) const
    {
        switch (subAllocator)
        {
        case GfxSubAllocator::TempUpload:
            return m_TempUploadSubAllocator.get();
        case GfxSubAllocator::PersistentUpload:
            return m_PersistentUploadSubAllocator.get();
        default:
            throw std::invalid_argument("Invalid GfxSubAllocator");
        }
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

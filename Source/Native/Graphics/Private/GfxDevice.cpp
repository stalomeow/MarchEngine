#include "GfxDevice.h"
#include "GfxCommandList.h"
#include "GfxCommandQueue.h"
#include "GfxFence.h"
#include "GfxBuffer.h"
#include "GfxSwapChain.h"
#include "GfxDescriptorHeap.h"
#include "GfxExcept.h"
#include "Debug.h"
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
        : m_DescriptorAllocators{}, m_ReleaseQueue{}
    {
        // 开启调试层
#if defined(DEBUG) || defined(_DEBUG)
        ComPtr<ID3D12Debug> debugController;
        GFX_HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
        DEBUG_LOG_INFO("D3D12 Debug Layer Enabled");
#endif

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

#pragma warning( disable : 6387 )
        // 获取 D3D12 输出的调试信息
        DWORD callbackCookie = 0;
        GFX_HR(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugInfoQueue)));
        GFX_HR(m_DebugInfoQueue->RegisterMessageCallback(D3D12DebugMessageCallback,
            D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callbackCookie));
#pragma warning( default : 6387 )

        if (callbackCookie == 0)
        {
            DEBUG_LOG_WARN("Failed to register D3D12 debug message callback");
        }

        m_GraphicsCommandQueue = std::make_unique<GfxCommandQueue>(this, GfxCommandListType::Graphics, "GraphicsCommandQueue");
        m_GraphicsFence = std::make_unique<GfxFence>(this, "GraphicsFence");
        m_GraphicsCommandAllocatorPool = std::make_unique<GfxCommandAllocatorPool>(this, GfxCommandListType::Graphics);
        m_GraphicsCommandList = std::make_unique<GfxCommandList>(this, GfxCommandListType::Graphics, "GraphicsCommandList");
        m_UploadMemoryAllocator = std::make_unique<GfxUploadMemoryAllocator>(this);

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
        {
            m_DescriptorAllocators[i] = std::make_unique<GfxDescriptorAllocator>(this, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        }

        m_ViewDescriptorTableAllocator = std::make_unique<GfxDescriptorTableAllocator>(this, GfxDescriptorTableType::CbvSrvUav, desc.ViewTableStaticDescriptorCount, desc.ViewTableDynamicDescriptorCapacity);
        m_SamplerDescriptorTableAllocator = std::make_unique<GfxDescriptorTableAllocator>(this, GfxDescriptorTableType::Sampler, desc.SamplerTableStaticDescriptorCount, desc.SamplerTableDynamicDescriptorCapacity);
        m_SwapChain = std::make_unique<GfxSwapChain>(this, desc.WindowHandle, desc.WindowWidth, desc.WindowHeight);

//#if defined(DEBUG) || defined(_DEBUG)
//        LogAdapters(GfxSwapChain::BackBufferFormat);
//#endif
    }

    GfxDevice::~GfxDevice()
    {
        WaitForIdle(); // 防止崩溃
        ProcessReleaseQueue();

        assert(m_ReleaseQueue.empty());
    }

    void GfxDevice::BeginFrame()
    {
        m_SwapChain->WaitForFrameLatency();
        ProcessReleaseQueue();

        m_GraphicsCommandAllocatorPool->BeginFrame();
        m_UploadMemoryAllocator->BeginFrame();

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
        {
            m_DescriptorAllocators[i]->BeginFrame();
        }

        m_ViewDescriptorTableAllocator->BeginFrame();
        m_SamplerDescriptorTableAllocator->BeginFrame();

        ID3D12DescriptorHeap* const descriptorHeaps[] =
        {
            m_ViewDescriptorTableAllocator->GetD3D12DescriptorHeap(),
            m_SamplerDescriptorTableAllocator->GetD3D12DescriptorHeap(),
        };
        m_GraphicsCommandList->Begin(m_GraphicsCommandAllocatorPool->Get(),
            static_cast<uint32_t>(std::size(descriptorHeaps)), descriptorHeaps);
    }

    void GfxDevice::EndFrame()
    {
        m_SwapChain->PreparePresent(m_GraphicsCommandList.get());
        m_GraphicsCommandList->End();
        m_GraphicsCommandQueue->ExecuteCommandList(m_GraphicsCommandList.get());

        uint64_t fenceValue = m_GraphicsCommandQueue->SignalNextValue(m_GraphicsFence.get());

        m_ViewDescriptorTableAllocator->EndFrame(fenceValue);
        m_SamplerDescriptorTableAllocator->EndFrame(fenceValue);

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
        {
            m_DescriptorAllocators[i]->EndFrame(fenceValue);
        }

        m_GraphicsCommandAllocatorPool->EndFrame(fenceValue);
        m_UploadMemoryAllocator->EndFrame(fenceValue);

        m_SwapChain->Present();
    }

    void GfxDevice::ReleaseD3D12Object(ID3D12Object* object)
    {
        m_ReleaseQueue.emplace(m_GraphicsFence->GetNextValue(), object);
    }

    void GfxDevice::ProcessReleaseQueue()
    {
        while (!m_ReleaseQueue.empty() && m_GraphicsFence->IsCompleted(m_ReleaseQueue.front().first))
        {
            ID3D12Object* object = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();

            // 释放资源较多时，太卡了！
            wchar_t name[256];
            UINT size = sizeof(name);
            if (SUCCEEDED(object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name)))
            {
                DEBUG_LOG_INFO(L"Release D3D12Object %s", name);
            }

            // 保证彻底释放
            while (object->Release() > 0) {}
        }
    }

    bool GfxDevice::IsGraphicsFenceCompleted(uint64_t fenceValue)
    {
        return m_GraphicsFence->IsCompleted(fenceValue);
    }

    void GfxDevice::WaitForIdle()
    {
        m_GraphicsCommandQueue->SignalNextValue(m_GraphicsFence.get());
        m_GraphicsFence->Wait();
    }

    void GfxDevice::ResizeBackBuffer(uint32_t width, uint32_t height)
    {
        WaitForIdle();
        m_SwapChain->Resize(width, height);
    }

    void GfxDevice::SetBackBufferAsRenderTarget()
    {
        m_SwapChain->SetRenderTarget(m_GraphicsCommandList.get());
    }

    DXGI_FORMAT GfxDevice::GetBackBufferFormat() const
    {
        return GfxSwapChain::BackBufferFormat;
    }

    uint32_t GfxDevice::GetMaxFrameLatency() const
    {
        return GfxSwapChain::MaxFrameLatency;
    }

    GfxDescriptorHandle GfxDevice::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type)
    {
        return m_DescriptorAllocators[static_cast<int>(type)]->Allocate();
    }

    void GfxDevice::FreeDescriptor(const GfxDescriptorHandle& handle)
    {
        int index = static_cast<int>(handle.GetType());
        m_DescriptorAllocators[index]->Free(handle);
    }

    GfxUploadMemory GfxDevice::AllocateTransientUploadMemory(uint32_t size, uint32_t count, uint32_t alignment)
    {
        return m_UploadMemoryAllocator->Allocate(size, count, alignment);
    }

    GfxDescriptorTable GfxDevice::AllocateTransientDescriptorTable(GfxDescriptorTableType type, uint32_t descriptorCount)
    {
        switch (type)
        {
        case GfxDescriptorTableType::CbvSrvUav:
            return m_ViewDescriptorTableAllocator->AllocateDynamicTable(descriptorCount);

        case GfxDescriptorTableType::Sampler:
            return m_SamplerDescriptorTableAllocator->AllocateDynamicTable(descriptorCount);

        default:
            throw GfxException("Invalid D3D12_DESCRIPTOR_HEAP_TYPE");
        }
    }

    GfxDescriptorTable GfxDevice::GetStaticDescriptorTable(GfxDescriptorTableType type)
    {
        switch (type)
        {
        case GfxDescriptorTableType::CbvSrvUav:
            return m_ViewDescriptorTableAllocator->GetStaticTable();

        case GfxDescriptorTableType::Sampler:
            return m_SamplerDescriptorTableAllocator->GetStaticTable();

        default:
            throw GfxException("Invalid D3D12_DESCRIPTOR_HEAP_TYPE");
        }
    }

    void GfxDevice::LogAdapters(DXGI_FORMAT format)
    {
        UINT i = 0;
        IDXGIAdapter* adapter = nullptr;

        while (m_Factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC desc;
            adapter->GetDesc(&desc);

            DEBUG_LOG_INFO(L"***Adapter: %s", desc.Description);

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
            output->GetDesc(&desc);

            DEBUG_LOG_INFO(L"***Output: %s", desc.DeviceName);

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
        output->GetDisplayModeList(format, flags, &count, nullptr);

        auto modeList = std::make_unique<DXGI_MODE_DESC[]>(count);
        output->GetDisplayModeList(format, flags, &count, modeList.get());

        for (UINT i = 0; i < count; i++)
        {
            auto& x = modeList[i];
            UINT n = x.RefreshRate.Numerator;
            UINT d = x.RefreshRate.Denominator;

            DEBUG_LOG_INFO(L"Width = %d, Height = %d, Refresh = %d/%d", x.Width, x.Height, n, d);
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
            DEBUG_LOG_INFO("%s", pDescription);
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            DEBUG_LOG_WARN("%s", pDescription);
            break;

        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        case D3D12_MESSAGE_SEVERITY_ERROR:
            DEBUG_LOG_ERROR("%s", pDescription);
            break;

        default:
            DEBUG_LOG_WARN("Unknown D3D12_MESSAGE_SEVERITY: %d; %s", Severity, pDescription);
            break;
        }
    }

    static std::unique_ptr<GfxDevice> g_GfxDevice = nullptr;

    GfxDevice* GetGfxDevice()
    {
        return g_GfxDevice.get();
    }

    void InitGfxDevice(const GfxDeviceDesc& desc)
    {
        g_GfxDevice = std::make_unique<GfxDevice>(desc);
    }
}

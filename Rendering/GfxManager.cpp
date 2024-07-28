#include "Rendering/GfxManager.h"
#include "Rendering/DxException.h"
#include "Core/Debug.h"

using namespace Microsoft::WRL;

namespace dx12demo
{
    namespace
    {
        GfxManager g_GfxManager;
    }

    GfxManager::~GfxManager()
    {
        if (m_Device != nullptr)
        {
            // 等待 GPU 完成所有命令，防止崩溃
            WaitForGpuIdle();
            CloseHandle(m_FenceEventHandle);
            CloseHandle(m_FrameLatencyWaitEventHandle);
        }
    }

    void GfxManager::Initialize(HWND window, int width, int height)
    {
        // 开启调试层
#if defined(DEBUG) || defined(_DEBUG)
        ComPtr<ID3D12Debug> debugController;
        THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
#endif

        InitDeviceAndFactory();
        InitDebugInfoCallback();
        InitCommandObjectsAndFence();
        InitUploadHeapAllocator();
        InitDescriptorHeaps();
        InitSwapChain(window, width, height);

#if defined(DEBUG) || defined(_DEBUG)
        LogAdapters(m_BackBufferFormat);
#endif
    }

    void GfxManager::InitDeviceAndFactory()
    {
        THROW_IF_FAILED(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

        // 默认设备
        HRESULT hardwareResult = D3D12CreateDevice(
            nullptr,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_Device));

        // 回退到 WARP 设备
        if (FAILED(hardwareResult))
        {
            ComPtr<IDXGIAdapter> warpAdapter;
            THROW_IF_FAILED(m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
            THROW_IF_FAILED(D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_Device)));
        }
    }

    void GfxManager::InitDebugInfoCallback()
    {
        THROW_IF_FAILED(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugInfoQueue)));

        DWORD cookie;
        THROW_IF_FAILED(m_DebugInfoQueue->RegisterMessageCallback([](D3D12_MESSAGE_CATEGORY Category,
            D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
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
            }, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie));
    }

    void GfxManager::InitCommandObjectsAndFence()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = m_CommandListType;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        THROW_IF_FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

        m_CommandAllocatorPool = std::make_unique<CommandAllocatorPool>(m_Device, m_CommandListType);

        THROW_IF_FAILED(m_Device->CreateFence(m_FenceCurrentValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
        m_FenceEventHandle = CreateEventExW(NULL, NULL, NULL, EVENT_ALL_ACCESS);
    }

    void GfxManager::InitUploadHeapAllocator()
    {
        m_UploadHeapAllocator = std::make_unique<UploadHeapAllocator>(4096);
    }

    void GfxManager::InitDescriptorHeaps()
    {
        m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = s_SwapChainBufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)));
    }

    void GfxManager::InitSwapChain(HWND window, int width, int height)
    {
        // The ability to create an MSAA DXGI swap chain is only supported for the older "bit-blt" style presentation modes,
        // specifically DXGI_SWAP_EFFECT_DISCARD or DXGI_SWAP_EFFECT_SEQUENTIAL.
        // The newer "flip" style presentation modes DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD
        // required for Universal Windows Platform (UWP) apps and Direct3D 12 doesn't support
        // creating MSAA swap chains--attempts to create a swap chain with SampleDesc.Count > 1 will fail.
        // Instead, you create your own MSAA render target and explicitly resolve
        // to the DXGI back-buffer for presentation as shown here.

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = m_BackBufferFormat;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = s_SwapChainBufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Starting with Direct3D 11.1, we recommend not to use CreateSwapChain anymore to create a swap chain.
        // Instead, use CreateSwapChainForHwnd, CreateSwapChainForCoreWindow,
        // or CreateSwapChainForComposition depending on how you want to create the swap chain.
        // THROW_IF_FAILED(m_Factory->CreateSwapChain(m_CommandQueue.Get(), &swapChainDesc, &m_SwapChain));
        THROW_IF_FAILED(m_Factory->CreateSwapChainForHwnd(m_CommandQueue.Get(),
            window, &swapChainDesc, nullptr, nullptr, &m_SwapChain));

        ComPtr<IDXGISwapChain2> s;
        THROW_IF_FAILED(m_SwapChain.As(&s));
        THROW_IF_FAILED(s->SetMaximumFrameLatency(3));
        m_FrameLatencyWaitEventHandle = s->GetFrameLatencyWaitableObject();

        ResizeBackBuffer(width, height);
    }

    void GfxManager::SignalNextFenceValue()
    {
        m_FenceCurrentValue++;
        THROW_IF_FAILED(m_CommandQueue->Signal(m_Fence.Get(), m_FenceCurrentValue));
    }

    CommandContext* GfxManager::GetCommandContext()
    {
        ID3D12CommandAllocator* allocator = m_CommandAllocatorPool->Get(GetCompletedFenceValue());

        if (!m_CommandContextPool.empty())
        {
            CommandContext* context = m_CommandContextPool.front();
            m_CommandContextPool.pop();

            context->Reset(allocator);
            return context;
        }

        auto pCtx = std::make_unique<CommandContext>(m_CommandListType);
        pCtx->Initialize(m_Device.Get(), allocator);
        m_CommandContextRefs.push_back(std::move(pCtx));
        return m_CommandContextRefs.back().get();
    }

    void GfxManager::ExecuteAndRelease(CommandContext* context, bool waitForCompletion)
    {
        ID3D12CommandAllocator* allocator = context->Close();
        ID3D12CommandList* list = context->GetList();
        m_CommandQueue->ExecuteCommandLists(1, &list);
        m_CommandContextPool.push(context);

        SignalNextFenceValue();
        m_CommandAllocatorPool->Release(allocator, GetCurrentFenceValue());

        if (waitForCompletion)
        {
            WaitForFence(GetCurrentFenceValue());
        }
    }

    void GfxManager::WaitForFence(UINT64 fenceValue)
    {
        if (m_Fence->GetCompletedValue() < fenceValue)
        {
            THROW_IF_FAILED(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEventHandle));
            WaitForSingleObject(m_FenceEventHandle, INFINITE);
        }
    }

    void GfxManager::WaitForGpuIdle()
    {
        SignalNextFenceValue();
        WaitForFence(GetCurrentFenceValue());
    }

    void GfxManager::ResizeBackBuffer(int width, int height)
    {
        WaitForGpuIdle();

        // 先释放之前的 buffer 引用，然后才能 resize
        for (int i = 0; i < s_SwapChainBufferCount; i++)
        {
            m_SwapChainBuffers[i].Reset();
        }

        m_SwapChain->ResizeBuffers(s_SwapChainBufferCount, width, height, m_BackBufferFormat,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
        m_CurrentBackBufferIndex = 0;

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; i < s_SwapChainBufferCount; i++)
        {
            THROW_IF_FAILED(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_SwapChainBuffers[i])));
            m_Device->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
            rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
        }
    }

    void GfxManager::WaitForFameLatency() const
    {
        WaitForSingleObjectEx(m_FrameLatencyWaitEventHandle, INFINITE, FALSE);
    }

    void GfxManager::Present()
    {
        THROW_IF_FAILED(m_SwapChain->Present(0, 0)); // No vsync

        // swap the back and front buffers
        m_CurrentBackBufferIndex++;
        m_CurrentBackBufferIndex %= s_SwapChainBufferCount;
    }

    void GfxManager::LogAdapters(DXGI_FORMAT format)
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

    void GfxManager::LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format)
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

    void GfxManager::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
    {
        UINT count = 0;
        UINT flags = 0;

        // Call with nullptr to get list count.
        output->GetDisplayModeList(format, flags, &count, nullptr);

        auto modeList = std::make_unique<DXGI_MODE_DESC[]>(count);
        output->GetDisplayModeList(format, flags, &count, modeList.get());

        for (int i = 0; i < count; i++)
        {
            auto& x = modeList[i];
            UINT n = x.RefreshRate.Numerator;
            UINT d = x.RefreshRate.Denominator;

            DEBUG_LOG_INFO(L"Width = %d, Height = %d, Refresh = %d/%d", x.Width, x.Height, n, d);
        }
    }

    GfxManager& GetGfxManager()
    {
        return g_GfxManager;
    }
}

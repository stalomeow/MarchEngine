#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxSwapChain.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"

using namespace Microsoft::WRL;

namespace march
{
    // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space
    // SwapChain Resource 的 Format 不能有 _SRGB 后缀，只能在创建 RTV 时加上 _SRGB
    static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr GfxCommandType CommandType = GfxCommandType::Direct;

    static bool CheckTearingSupport(IDXGIFactory5* factory)
    {
        // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays
        BOOL allowTearing = FALSE;
        HRESULT hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        return SUCCEEDED(hr) && allowTearing;
    }

    GfxSwapChain::GfxSwapChain(GfxDevice* device, HWND hWnd, uint32_t width, uint32_t height)
        : m_Device(device)
        , m_PublicBackBuffer(nullptr)
        , m_PrivateBackBuffers{}
        , m_CurrentPrivateBackBufferIndex(0)
    {
        // https://github.com/microsoft/DirectXTK/wiki/Line-drawing-and-anti-aliasing#technical-note
        // The ability to create an MSAA DXGI swap chain is only supported for the older "bit-blt" style presentation modes,
        // specifically DXGI_SWAP_EFFECT_DISCARD or DXGI_SWAP_EFFECT_SEQUENTIAL.
        // The newer "flip" style presentation modes DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD
        // required for Universal Windows Platform (UWP) apps and Direct3D 12 doesn't support
        // creating MSAA swap chains--attempts to create a swap chain with SampleDesc.Count > 1 will fail.
        // Instead, you create your own MSAA render target and explicitly resolve
        // to the DXGI back-buffer for presentation as shown here.

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = static_cast<UINT>(width);
        swapChainDesc.Height = static_cast<UINT>(height);
        swapChainDesc.Format = BackBufferFormat;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = static_cast<UINT>(GfxSettings::BackBufferCount);
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        IDXGIFactory5* factory = device->GetDXGIFactory();

        if ((m_SupportTearing = CheckTearingSupport(factory)))
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        ID3D12CommandQueue* queue = device->GetCommandManager()->GetQueue(CommandType)->GetQueue();

        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-createswapchain
        // Starting with Direct3D 11.1, we recommend not to use CreateSwapChain anymore to create a swap chain.
        // Instead, use CreateSwapChainForHwnd, CreateSwapChainForCoreWindow,
        // or CreateSwapChainForComposition depending on how you want to create the swap chain.
        CHECK_HR(factory->CreateSwapChainForHwnd(queue, hWnd, &swapChainDesc, nullptr, nullptr, &m_SwapChain));

        // https://developer.nvidia.com/blog/advanced-api-performance-swap-chains/
        ComPtr<IDXGISwapChain2> swapChain2;
        CHECK_HR(m_SwapChain.As(&swapChain2));
        CHECK_HR(swapChain2->SetMaximumFrameLatency(static_cast<UINT>(GfxSettings::MaxFrameLatency)));
        m_FrameLatencyHandle = swapChain2->GetFrameLatencyWaitableObject();

        CreateBackBuffers(width, height);
    }

    GfxSwapChain::~GfxSwapChain()
    {
        CloseHandle(m_FrameLatencyHandle);
    }

    uint32_t GfxSwapChain::GetPixelWidth() const
    {
        return m_PublicBackBuffer->GetDesc().Width;
    }

    uint32_t GfxSwapChain::GetPixelHeight() const
    {
        return m_PublicBackBuffer->GetDesc().Height;
    }

    GfxRenderTexture* GfxSwapChain::GetBackBuffer() const
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
        // You can't resize a swap chain unless you release all outstanding references to its back buffers.
        // You must release all of its direct and indirect references on the back buffers in order for ResizeBuffers to succeed.

        // 为了方便 Resize，这里不会把 SwapChain 的 Back Buffer 暴露出去，而是返回一个自己创建的 Back Buffer
        return m_PublicBackBuffer.get();
    }

    void GfxSwapChain::WaitForFrameLatency() const
    {
        WaitForSingleObjectEx(m_FrameLatencyHandle, INFINITE, FALSE);
    }

    void GfxSwapChain::Present()
    {
        PrepareCurrentPrivateBackBuffer();

        UINT syncInterval = static_cast<UINT>(GfxSettings::VerticalSyncInterval);
        UINT flags = 0;

        // TODO Handle Fullscreen
        if (m_SupportTearing && syncInterval == 0)
        {
            flags |= DXGI_PRESENT_ALLOW_TEARING; // 禁用垂直同步
        }

        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
        // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#multithread-considerations
        // When you use DXGI in an application with multiple threads, you need to be careful to avoid creating a deadlock,
        // where two different threads are waiting on each other to complete. There are two situations where this can occur.
        // - The rendering thread is not the message-pump thread.
        // - The thread executing a DXGI API is not the same thread that created the window.
        // Be careful that you never have the message-pump thread wait on the render thread when you use full-screen swap chains.
        // For instance, calling IDXGISwapChain1::Present1 (from the render thread) may cause the render thread to wait on the message-pump thread.
        // When a mode change occurs, this scenario is possible if Present1 calls ::SetWindowPos() or ::SetWindowStyle() and either of these methods call ::SendMessage().
        // In this scenario, if the message-pump thread has a critical section guarding it or if the render thread is blocked, then the two threads will deadlock.
        CHECK_HR(m_SwapChain->Present(syncInterval, flags));

        m_CurrentPrivateBackBufferIndex = (m_CurrentPrivateBackBufferIndex + 1) % GfxSettings::BackBufferCount;
    }

    void GfxSwapChain::PrepareCurrentPrivateBackBuffer()
    {
        ComPtr<ID3D12Resource>& privateBackBuffer = m_PrivateBackBuffers[m_CurrentPrivateBackBufferIndex];

        GfxCommandContext* cmd = m_Device->RequestContext(CommandType);
        cmd->BeginEvent("PrepareBackBuffer");

        {
            cmd->TransitionResource(m_PublicBackBuffer->GetUnderlyingResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
            cmd->TransitionResource(privateBackBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
            cmd->FlushResourceBarriers();

            // 将 Public Back Buffer 的内容复制到 Private Back Buffer
            cmd->GetList()->CopyResource(privateBackBuffer.Get(), m_PublicBackBuffer->GetUnderlyingD3DResource());

            cmd->TransitionResource(privateBackBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
            cmd->FlushResourceBarriers();
        }

        cmd->EndEvent();
        cmd->SubmitAndRelease();
    }

    void GfxSwapChain::Resize(uint32_t width, uint32_t height)
    {
        if (width == GetPixelWidth() && height == GetPixelHeight())
        {
            return;
        }

        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
        // You can't resize a swap chain unless you release all outstanding references to its back buffers.
        // You must release all of its direct and indirect references on the back buffers in order for ResizeBuffers to succeed.

        // Private Back Buffer 没有暴露给外部，只在这个 queue 上使用过
        GfxCommandQueue* queue = m_Device->GetCommandManager()->GetQueue(CommandType);

        // 等待一下，确保使用完毕
        queue->CreateSyncPoint().WaitOnCpu();

        // 释放引用
        for (ComPtr<ID3D12Resource>& backBuffer : m_PrivateBackBuffers)
        {
            backBuffer = nullptr;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        CHECK_HR(m_SwapChain->GetDesc1(&swapChainDesc));
        CHECK_HR(m_SwapChain->ResizeBuffers(
            swapChainDesc.BufferCount,
            static_cast<UINT>(width),
            static_cast<UINT>(height),
            swapChainDesc.Format,
            swapChainDesc.Flags));

        CreateBackBuffers(width, height);
    }

    void GfxSwapChain::CreateBackBuffers(uint32_t width, uint32_t height)
    {
        GfxTextureDesc desc{};
        desc.SetResDXGIFormat(BackBufferFormat, /* updateFlags */ false);
        desc.Flags = GfxTextureFlags::SRGB;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = width;
        desc.Height = height;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Point;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0.0f;

        m_PublicBackBuffer = std::make_unique<GfxRenderTexture>(m_Device, "PublicBackBuffer", desc, GfxTextureAllocStrategy::DefaultHeapCommitted);

        for (uint32_t i = 0; i < GfxSettings::BackBufferCount; i++)
        {
            ComPtr<ID3D12Resource>& backBuffer = m_PrivateBackBuffers[i];
            CHECK_HR(m_SwapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&backBuffer)));
            GfxUtils::SetName(backBuffer.Get(), "PrivateBackBuffer" + std::to_string(i));
        }

        m_CurrentPrivateBackBufferIndex = 0;
    }
}

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
    static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr GfxCommandType CommandType = GfxCommandType::Direct;

    GfxSwapChain::GfxSwapChain(GfxDevice* device, HWND hWnd, uint32_t width, uint32_t height)
        : m_Device(device), m_BackBuffers{}, m_CurrentBackBufferIndex(0)
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
        swapChainDesc.BufferCount = static_cast<UINT>(BackBufferCount);
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        IDXGIFactory4* factory = device->GetDXGIFactory();
        ID3D12CommandQueue* commandQueue = device->GetCommandManager()->GetQueue(CommandType)->GetQueue();

        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-createswapchain
        // Starting with Direct3D 11.1, we recommend not to use CreateSwapChain anymore to create a swap chain.
        // Instead, use CreateSwapChainForHwnd, CreateSwapChainForCoreWindow,
        // or CreateSwapChainForComposition depending on how you want to create the swap chain.
        CHECK_HR(factory->CreateSwapChainForHwnd(commandQueue, hWnd, &swapChainDesc, nullptr, nullptr, &m_SwapChain));

        // https://developer.nvidia.com/blog/advanced-api-performance-swap-chains/
        ComPtr<IDXGISwapChain2> swapChain2;
        CHECK_HR(m_SwapChain.As(&swapChain2));
        CHECK_HR(swapChain2->SetMaximumFrameLatency(static_cast<UINT>(MaxFrameLatency)));
        m_FrameLatencyHandle = swapChain2->GetFrameLatencyWaitableObject();

        CreateBackBuffers();
    }

    GfxSwapChain::~GfxSwapChain()
    {
        CloseHandle(m_FrameLatencyHandle);
    }

    uint32_t GfxSwapChain::GetPixelWidth() const
    {
        return m_BackBuffers[0]->GetDesc().Width;
    }

    uint32_t GfxSwapChain::GetPixelHeight() const
    {
        return m_BackBuffers[0]->GetDesc().Height;
    }

    void GfxSwapChain::NewFrame(uint32_t width, uint32_t height, bool willQuit)
    {
        bool needResize;

        if (willQuit)
        {
            needResize = false;
        }
        else
        {
            if (width == GetPixelWidth() && height == GetPixelHeight())
            {
                needResize = false;
            }
            else
            {
                needResize = true;

                // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
                // You can't resize a swap chain unless you release all outstanding references to its back buffers.
                // You must release all of its direct and indirect references on the back buffers in order for ResizeBuffers to succeed.
                for (uint32_t i = 0; i < BackBufferCount; i++)
                {
                    m_BackBuffers[i].reset();
                }
            }

            // Wait for frame latency
            WaitForSingleObjectEx(m_FrameLatencyHandle, INFINITE, FALSE);
        }

        // 如果要 Resize，则必须等待旧的 Back Buffer 使用完毕
        m_Device->GetCommandManager()->SignalNextFrameFence(/* waitForGpuIdle */ needResize);
        m_Device->CleanupResources();

        if (needResize)
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
            CHECK_HR(m_SwapChain->GetDesc1(&swapChainDesc));
            CHECK_HR(m_SwapChain->ResizeBuffers(swapChainDesc.BufferCount,
                static_cast<UINT>(width), static_cast<UINT>(height), swapChainDesc.Format, swapChainDesc.Flags));

            m_CurrentBackBufferIndex = 0;
            CreateBackBuffers();
        }
    }

    void GfxSwapChain::Present(bool willQuit)
    {
        if (willQuit)
        {
            return;
        }

        // 需要外部保证当前的 back buffer 是在 D3D12_RESOURCE_STATE_PRESENT 状态
        assert(GetBackBuffer()->GetUnderlyingResource()->AreAllStatesEqualTo(D3D12_RESOURCE_STATE_PRESENT));

        CHECK_HR(m_SwapChain->Present(0, 0)); // No vsync
        m_CurrentBackBufferIndex = (m_CurrentBackBufferIndex + 1) % BackBufferCount;
    }

    void GfxSwapChain::CreateBackBuffers()
    {
        GfxTextureResourceDesc desc{};
        desc.IsCube = false;
        desc.State = D3D12_RESOURCE_STATE_COMMON;
        desc.Flags = GfxTextureFlags::SRGB | GfxTextureFlags::SwapChain;
        desc.Filter = GfxTextureFilterMode::Point;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0.0f;

        for (uint32_t i = 0; i < BackBufferCount; i++)
        {
            ComPtr<ID3D12Resource> backBuffer = nullptr;
            CHECK_HR(m_SwapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&backBuffer)));
            GfxUtils::SetName(backBuffer.Get(), "BackBuffer" + std::to_string(i));

            m_BackBuffers[i] = std::make_unique<GfxRenderTexture>(m_Device, backBuffer, desc);
        }
    }
}

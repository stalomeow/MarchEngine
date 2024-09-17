#include "GfxSwapChain.h"
#include "GfxCommandQueue.h"
#include "GfxDevice.h"
#include "GfxExcept.h"

using namespace Microsoft::WRL;

namespace march
{
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
        swapChainDesc.BufferCount = BackBufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        IDXGIFactory4* factory = device->GetDXGIFactory();
        ID3D12CommandQueue* commandQueue = device->GetGraphicsCommandQueue()->GetD3D12CommandQueue();

        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-createswapchain
        // Starting with Direct3D 11.1, we recommend not to use CreateSwapChain anymore to create a swap chain.
        // Instead, use CreateSwapChainForHwnd, CreateSwapChainForCoreWindow,
        // or CreateSwapChainForComposition depending on how you want to create the swap chain.
        GFX_HR(factory->CreateSwapChainForHwnd(commandQueue, hWnd, &swapChainDesc, nullptr, nullptr, &m_SwapChain));

        // https://developer.nvidia.com/blog/advanced-api-performance-swap-chains/
        ComPtr<IDXGISwapChain2> swapChain2;
        GFX_HR(m_SwapChain.As(&swapChain2));
        GFX_HR(swapChain2->SetMaximumFrameLatency(static_cast<UINT>(MaxFrameLatency)));
        m_FrameLatencyHandle = swapChain2->GetFrameLatencyWaitableObject();

        CreateBackBuffers();
    }

    GfxSwapChain::~GfxSwapChain()
    {
        CloseHandle(m_FrameLatencyHandle);
    }

    void GfxSwapChain::Resize(uint32_t width, uint32_t height)
    {
        // 先释放之前的 buffer 引用，然后才能 resize
        for (int i = 0; i < BackBufferCount; i++)
        {
            m_BackBuffers[i].Reset();
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        GFX_HR(m_SwapChain->GetDesc1(&swapChainDesc));
        GFX_HR(m_SwapChain->ResizeBuffers(swapChainDesc.BufferCount,
            static_cast<UINT>(width), static_cast<UINT>(height),
            swapChainDesc.Format, swapChainDesc.Flags));

        m_CurrentBackBufferIndex = 0;
        CreateBackBuffers();
    }

    void GfxSwapChain::CreateBackBuffers()
    {
        for (int32_t i = 0; i < BackBufferCount; i++)
        {
            GFX_HR(m_SwapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&m_BackBuffers[i])));

            //m_RtvHandles[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            //m_Device->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, m_RtvHandles[i].GetCpuHandle());
        }
    }

    void GfxSwapChain::WaitForFrameLatency() const
    {
        WaitForSingleObjectEx(m_FrameLatencyHandle, INFINITE, FALSE);
    }

    void GfxSwapChain::Preset()
    {
        GFX_HR(m_SwapChain->Present(0, 0)); // No vsync
        m_CurrentBackBufferIndex = (m_CurrentBackBufferIndex + 1) % BackBufferCount;
    }
}

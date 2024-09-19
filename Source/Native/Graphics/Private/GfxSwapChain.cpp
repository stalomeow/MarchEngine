#include "GfxSwapChain.h"
#include "GfxCommandQueue.h"
#include "GfxCommandList.h"
#include "GfxDevice.h"
#include "GfxExcept.h"

using namespace Microsoft::WRL;

namespace march
{
    GfxSwapChain::GfxSwapChain(GfxDevice* device, HWND hWnd, uint32_t width, uint32_t height)
        : m_Device(device), m_BackBuffers{}, m_BackBufferRtvHandles{}, m_CurrentBackBufferIndex(0)
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

        for (uint32_t i = 0; i < BackBufferCount; i++)
        {
            m_BackBufferRtvHandles[i] = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        CreateBackBuffers();
    }

    GfxSwapChain::~GfxSwapChain()
    {
        CloseHandle(m_FrameLatencyHandle);

        for (uint32_t i = 0; i < BackBufferCount; i++)
        {
            m_Device->FreeDescriptor(m_BackBufferRtvHandles[i]);
        }
    }

    void GfxSwapChain::Resize(uint32_t width, uint32_t height)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
        // You can't resize a swap chain unless you release all outstanding references to its back buffers.
        // You must release all of its direct and indirect references on the back buffers in order for ResizeBuffers to succeed.
        for (uint32_t i = 0; i < BackBufferCount; i++)
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
        for (uint32_t i = 0; i < BackBufferCount; i++)
        {
            GFX_HR(m_SwapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&m_BackBuffers[i])));

            ID3D12Device4* device = m_Device->GetD3D12Device();
            device->CreateRenderTargetView(m_BackBuffers[i].Get(), nullptr, m_BackBufferRtvHandles[i].GetCpuHandle());
        }
    }

    void GfxSwapChain::Begin() const
    {
        WaitForSingleObjectEx(m_FrameLatencyHandle, INFINITE, FALSE);

        ID3D12GraphicsCommandList* cmdList = m_Device->GetGraphicsCommandList()->GetD3D12CommandList();
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    }

    void GfxSwapChain::Present()
    {
        ID3D12GraphicsCommandList* cmdList = m_Device->GetGraphicsCommandList()->GetD3D12CommandList();
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

        GFX_HR(m_SwapChain->Present(0, 0)); // No vsync
        m_CurrentBackBufferIndex = (m_CurrentBackBufferIndex + 1) % BackBufferCount;
    }
}

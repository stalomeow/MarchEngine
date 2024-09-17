#include "GfxDevice.h"
#include "GfxCommandQueue.h"
#include "GfxFence.h"
#include "GfxSwapChain.h"
#include "GfxExcept.h"
#include "Debug.h"
#include <assert.h>

using namespace Microsoft::WRL;

namespace march
{
    GfxDevice::GfxDevice(HWND hWnd, uint32_t width, uint32_t height)
        : m_ReleaseQueue{}
    {
        // 开启调试层
#if defined(DEBUG) || defined(_DEBUG)
        ComPtr<ID3D12Debug> debugController;
        GFX_HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
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

        GFX_HR(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugInfoQueue)));

        m_GraphicsCommandQueue = std::make_unique<GfxCommandQueue>(this, GfxCommandQueueType::Graphics, "GraphicsCommandQueue");
        m_GraphicsFence = std::make_unique<GfxFence>(this, "GraphicsFence");
        m_SwapChain = std::make_unique<GfxSwapChain>(this, hWnd, width, height);

//#if defined(DEBUG) || defined(_DEBUG)
//        LogAdapters(m_BackBufferFormat);
//#endif
    }

    GfxDevice::~GfxDevice()
    {
        WaitForIdle();
        ProcessReleaseQueue();

        assert(m_ReleaseQueue.empty());
    }

    void GfxDevice::BeginFrame()
    {
        m_SwapChain->WaitForFrameLatency();
    }

    void GfxDevice::EndFrame()
    {
        m_SwapChain->Preset();
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

    void GfxDevice::WaitForIdle()
    {
        m_GraphicsCommandQueue->SignalNextValue(m_GraphicsFence.get());
        m_GraphicsFence->Wait();
    }

    static std::unique_ptr<GfxDevice> g_GfxDevice = nullptr;

    GfxDevice* GetGfxDevice()
    {
        return g_GfxDevice.get();
    }

    void InitGfxDevice(HWND hWnd, uint32_t width, uint32_t height)
    {
        g_GfxDevice = std::make_unique<GfxDevice>(hWnd, width, height);
    }
}

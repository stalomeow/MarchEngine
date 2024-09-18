#include "GfxDevice.h"
#include "GfxCommandList.h"
#include "GfxCommandQueue.h"
#include "GfxFence.h"
#include "GfxSwapChain.h"
#include "GfxCommandAllocatorPool.h"
#include "GfxUploadMemoryAllocator.h"
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
        ProcessReleaseQueue();

        m_GraphicsCommandAllocatorPool->BeginFrame();
        m_UploadMemoryAllocator->BeginFrame();
        m_GraphicsCommandList->Begin(m_GraphicsCommandAllocatorPool->Get());
    }

    void GfxDevice::EndFrame()
    {
        m_GraphicsCommandList->End();
        m_GraphicsCommandQueue->ExecuteCommandList(m_GraphicsCommandList.get());

        uint64_t fenceValue = m_GraphicsCommandQueue->SignalNextValue(m_GraphicsFence.get());
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

    GfxUploadMemory GfxDevice::AllocateTransientUploadMemory(uint32_t size, uint32_t count, uint32_t alignment)
    {
        return m_UploadMemoryAllocator->Allocate(size, count, alignment);
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

    void InitGfxDevice(HWND hWnd, uint32_t width, uint32_t height)
    {
        g_GfxDevice = std::make_unique<GfxDevice>(hWnd, width, height);
    }
}

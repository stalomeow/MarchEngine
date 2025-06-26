#pragma once

#include "Engine/Rendering/D3D12Impl/GfxSettings.h"
#include <d3dx12.h>
#include <dxgi1_5.h>
#include <wrl.h>
#include <Windows.h>
#include <stdint.h>
#include <memory>

namespace march
{
    class GfxDevice;
    class GfxRenderTexture;

    class GfxSwapChain
    {
    public:
        GfxSwapChain(GfxDevice* device, HWND hWnd, uint32_t width, uint32_t height);
        ~GfxSwapChain();

        uint32_t GetPixelWidth() const;
        uint32_t GetPixelHeight() const;
        GfxRenderTexture* GetBackBuffer() const;

        void WaitForFrameLatency() const;
        void Present();
        void Resize(uint32_t width, uint32_t height);

    private:
        GfxDevice* m_Device;
        bool m_SupportTearing;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain;
        HANDLE m_FrameLatencyHandle;

        std::unique_ptr<GfxRenderTexture> m_PublicBackBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_PrivateBackBuffers[GfxSettings::BackBufferCount];
        uint32_t m_CurrentPrivateBackBufferIndex;

        void PrepareCurrentPrivateBackBuffer();
        void CreateBackBuffers(uint32_t width, uint32_t height);
    };
}

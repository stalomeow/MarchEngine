#pragma once

#include <directx/d3dx12.h>
#include <dxgi1_4.h>
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

        void NewFrame(uint32_t width, uint32_t height, bool willQuit);
        void Present(bool willQuit);

        GfxRenderTexture* GetBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex].get(); }

        static constexpr uint32_t BackBufferCount = 2;
        static constexpr uint32_t MaxFrameLatency = 3;

    private:
        GfxDevice* m_Device;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain;
        HANDLE m_FrameLatencyHandle;

        std::unique_ptr<GfxRenderTexture> m_BackBuffers[BackBufferCount];
        uint32_t m_CurrentBackBufferIndex;

        void CreateBackBuffers();
    };
}

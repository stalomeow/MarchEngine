#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>
#include <memory>
#include <string>

namespace march
{
    class GfxDevice;
    class GfxRenderTexture;

    class Display
    {
    public:
        Display(GfxDevice* device, const std::string& name, uint32_t width, uint32_t height);

        bool GetEnableMSAA() const;
        void SetEnableMSAA(bool value);
        uint32_t GetCurrentMSAAQuality() const; // 如果关闭 MSAA，返回 0
        uint32_t GetCurrentMSAASampleCount() const; // 如果关闭 MSAA，返回 1

        uint32_t GetPixelWidth() const;
        uint32_t GetPixelHeight() const;
        void Resize(uint32_t width, uint32_t height);

        DXGI_FORMAT GetColorFormat() const;
        DXGI_FORMAT GetDepthStencilFormat() const;
        GfxRenderTexture* GetColorBuffer() const;
        GfxRenderTexture* GetDepthStencilBuffer() const;
        GfxRenderTexture* GetResolvedColorBuffer() const;
        GfxRenderTexture* GetResolvedDepthStencilBuffer() const;

        static Display* GetMainDisplay();
        static void CreateMainDisplay(GfxDevice* device, uint32_t width, uint32_t height);
        static void DestroyMainDisplay();

    private:
        void CreateBuffers(uint32_t width, uint32_t height);

        GfxDevice* m_Device;
        std::string m_Name;
        bool m_EnableMSAA;
        uint32_t m_MSAAQuality;

        std::unique_ptr<GfxRenderTexture> m_ColorBuffer;
        std::unique_ptr<GfxRenderTexture> m_DepthStencilBuffer;
        std::unique_ptr<GfxRenderTexture> m_ResolvedColorBuffer;
        std::unique_ptr<GfxRenderTexture> m_ResolvedDepthStencilBuffer;

        static const uint32_t s_MSAASampleCount = 4;
        static const DXGI_FORMAT s_ColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        static const DXGI_FORMAT s_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        static std::unique_ptr<Display> s_MainDisplay;
    };
}

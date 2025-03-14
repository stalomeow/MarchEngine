#pragma once

#include <stdint.h>
#include <memory>
#include <string>

namespace march
{
    enum class GfxTextureFormat;
    class GfxRenderTexture;

    class Display
    {
    public:
        Display(const std::string& name, uint32_t width, uint32_t height);

        bool GetEnableMSAA() const;
        void SetEnableMSAA(bool value);
        uint32_t GetCurrentMSAASampleCount() const; // 如果关闭 MSAA，返回 1

        uint32_t GetPixelWidth() const;
        uint32_t GetPixelHeight() const;
        void Resize(uint32_t width, uint32_t height);

        GfxTextureFormat GetColorFormat() const;
        GfxTextureFormat GetDepthStencilFormat() const;
        GfxRenderTexture* GetColorBuffer() const;
        GfxRenderTexture* GetHistoryColorBuffer() const;
        GfxRenderTexture* GetDepthStencilBuffer() const;
        GfxRenderTexture* GetResolvedColorBuffer() const;
        GfxRenderTexture* GetResolvedDepthStencilBuffer() const;

        static Display* GetMainDisplay();
        static void CreateMainDisplay(uint32_t width, uint32_t height);
        static void DestroyMainDisplay();

    private:
        void CreateBuffers(uint32_t width, uint32_t height);

        std::string m_Name;
        bool m_EnableMSAA;

        std::unique_ptr<GfxRenderTexture> m_ColorBuffer;
        std::unique_ptr<GfxRenderTexture> m_HistoryColorBuffer;
        std::unique_ptr<GfxRenderTexture> m_DepthStencilBuffer;
        std::unique_ptr<GfxRenderTexture> m_ResolvedColorBuffer;
        std::unique_ptr<GfxRenderTexture> m_ResolvedDepthStencilBuffer;

        static std::unique_ptr<Display> s_MainDisplay;
    };
}

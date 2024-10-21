#include "Display.h"
#include "GfxDevice.h"
#include "GfxTexture.h"
#include "GfxHelpers.h"

namespace march
{
    Display::Display(GfxDevice* device, const std::string& name, uint32_t width, uint32_t height)
        : m_Device(device)
        , m_Name(name)
        , m_EnableMSAA(false)
        , m_ColorBuffer(nullptr)
        , m_DepthStencilBuffer(nullptr)
        , m_ResolvedColorBuffer(nullptr)
        , m_ResolvedDepthStencilBuffer(nullptr)
    {
        m_MSAAQuality = GfxHelpers::GetMSAAQuality(device, s_ColorFormat, s_MSAASampleCount);
        CreateBuffers(width, height);
    }

    bool Display::GetEnableMSAA() const
    {
        return m_EnableMSAA;
    }

    void Display::SetEnableMSAA(bool value)
    {
        if (m_EnableMSAA == value)
        {
            return;
        }

        m_EnableMSAA = value;
        CreateBuffers(GetPixelWidth(), GetPixelHeight());
    }

    uint32_t Display::GetCurrentMSAAQuality() const
    {
        return m_EnableMSAA ? m_MSAAQuality : 0;
    }

    uint32_t Display::GetCurrentMSAASampleCount() const
    {
        return m_EnableMSAA ? s_MSAASampleCount : 1;
    }

    uint32_t Display::GetPixelWidth() const
    {
        return m_ColorBuffer->GetWidth();
    }

    uint32_t Display::GetPixelHeight() const
    {
        return m_ColorBuffer->GetHeight();
    }

    void Display::Resize(uint32_t width, uint32_t height)
    {
        if (width == GetPixelWidth() && height == GetPixelHeight())
        {
            return;
        }

        CreateBuffers(width, height);
    }

    DXGI_FORMAT Display::GetColorFormat() const
    {
        return s_ColorFormat;
    }

    DXGI_FORMAT Display::GetDepthStencilFormat() const
    {
        return s_DepthStencilFormat;
    }

    GfxRenderTexture* Display::GetColorBuffer() const
    {
        return m_ColorBuffer.get();
    }

    GfxRenderTexture* Display::GetDepthStencilBuffer() const
    {
        return m_DepthStencilBuffer.get();
    }

    GfxRenderTexture* Display::GetResolvedColorBuffer() const
    {
        return m_ResolvedColorBuffer.get();
    }

    GfxRenderTexture* Display::GetResolvedDepthStencilBuffer() const
    {
        return m_ResolvedDepthStencilBuffer.get();
    }

    void Display::CreateBuffers(uint32_t width, uint32_t height)
    {
        m_ColorBuffer = std::make_unique<GfxRenderTexture>(m_Device, m_Name + "DisplayColorBuffer",
            s_ColorFormat, width, height,
            m_EnableMSAA ? s_MSAASampleCount : 1,
            m_EnableMSAA ? m_MSAAQuality : 0);

        m_DepthStencilBuffer = std::make_unique<GfxRenderTexture>(m_Device, m_Name + "DisplayDepthStencilBuffer",
            s_DepthStencilFormat, width, height,
            m_EnableMSAA ? s_MSAASampleCount : 1,
            m_EnableMSAA ? m_MSAAQuality : 0);

        if (m_EnableMSAA)
        {
            m_ResolvedColorBuffer = std::make_unique<GfxRenderTexture>(m_Device,
                m_Name + "DisplayResolvedColorBuffer", s_ColorFormat, width, height);

            m_ResolvedDepthStencilBuffer = std::make_unique<GfxRenderTexture>(m_Device,
                m_Name + "DisplayResolvedDepthStencilBuffer", s_DepthStencilFormat, width, height);
        }
        else
        {
            m_ResolvedColorBuffer = nullptr;
            m_ResolvedDepthStencilBuffer = nullptr;
        }
    }

    std::unique_ptr<Display> Display::s_MainDisplay = nullptr;

    Display* Display::GetMainDisplay()
    {
        return s_MainDisplay.get();
    }

    void Display::CreateMainDisplay(GfxDevice* device, uint32_t width, uint32_t height)
    {
        s_MainDisplay = std::make_unique<Display>(device, "Main", width, height);
    }

    void Display::DestroyMainDisplay()
    {
        s_MainDisplay.reset();
    }
}

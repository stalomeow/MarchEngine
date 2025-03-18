#include "pch.h"
#include "Engine/Rendering/Display.h"
#include "Engine/Rendering/D3D12.h"

namespace march
{
    static const uint32_t MSAASampleCount = 4;
    static const GfxTextureFormat ColorFormat = GfxTextureFormat::R16G16B16A16_Float;
    static const GfxTextureFormat DepthStencilFormat = GfxTextureFormat::D32_Float_S8_UInt;

    Display::Display(const std::string& name, uint32_t width, uint32_t height)
        : m_Name(name)
        , m_EnableMSAA(false)
        , m_ColorBuffer(nullptr)
        , m_HistoryColorBuffer(nullptr)
        , m_DepthStencilBuffer(nullptr)
        , m_ResolvedColorBuffer(nullptr)
        , m_ResolvedDepthStencilBuffer(nullptr)
    {
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

    uint32_t Display::GetCurrentMSAASampleCount() const
    {
        return m_EnableMSAA ? MSAASampleCount : 1;
    }

    uint32_t Display::GetPixelWidth() const
    {
        return m_ColorBuffer->GetDesc().Width;
    }

    uint32_t Display::GetPixelHeight() const
    {
        return m_ColorBuffer->GetDesc().Height;
    }

    void Display::Resize(uint32_t width, uint32_t height)
    {
        if (width == GetPixelWidth() && height == GetPixelHeight())
        {
            return;
        }

        CreateBuffers(width, height);
    }

    GfxTextureFormat Display::GetColorFormat() const
    {
        return ColorFormat;
    }

    GfxTextureFormat Display::GetDepthStencilFormat() const
    {
        return DepthStencilFormat;
    }

    GfxRenderTexture* Display::GetColorBuffer() const
    {
        return m_ColorBuffer.get();
    }

    GfxRenderTexture* Display::GetHistoryColorBuffer() const
    {
        return m_HistoryColorBuffer.get();
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
        GfxDevice* device = GetGfxDevice();

        GfxTextureDesc colorDesc{};
        colorDesc.Format = ColorFormat;
        colorDesc.Flags = m_EnableMSAA ? GfxTextureFlags::None : GfxTextureFlags::UnorderedAccess;
        colorDesc.Dimension = GfxTextureDimension::Tex2D;
        colorDesc.Width = width;
        colorDesc.Height = height;
        colorDesc.DepthOrArraySize = 1;
        colorDesc.MSAASamples = GetCurrentMSAASampleCount();
        colorDesc.Filter = GfxTextureFilterMode::Bilinear;
        colorDesc.Wrap = GfxTextureWrapMode::Clamp;
        colorDesc.MipmapBias = 0.0f;

        m_ColorBuffer = std::make_unique<GfxRenderTexture>(device, m_Name + "DisplayColor", colorDesc, GfxTextureAllocStrategy::DefaultHeapCommitted);

        if (!m_EnableMSAA)
        {
            m_HistoryColorBuffer = std::make_unique<GfxRenderTexture>(device, m_Name + "HistoryColor", colorDesc, GfxTextureAllocStrategy::DefaultHeapCommitted);
        }
        else
        {
            m_HistoryColorBuffer = nullptr;
        }

        GfxTextureDesc dsDesc{};
        dsDesc.Format = DepthStencilFormat;
        dsDesc.Flags = GfxTextureFlags::None;
        dsDesc.Dimension = GfxTextureDimension::Tex2D;
        dsDesc.Width = width;
        dsDesc.Height = height;
        dsDesc.DepthOrArraySize = 1;
        dsDesc.MSAASamples = GetCurrentMSAASampleCount();
        dsDesc.Filter = GfxTextureFilterMode::Bilinear;
        dsDesc.Wrap = GfxTextureWrapMode::Clamp;
        dsDesc.MipmapBias = 0.0f;

        m_DepthStencilBuffer = std::make_unique<GfxRenderTexture>(device, m_Name + "DisplayDepthStencil", dsDesc, GfxTextureAllocStrategy::DefaultHeapCommitted);

        if (m_EnableMSAA)
        {
            colorDesc.MSAASamples = 1;
            dsDesc.MSAASamples = 1;

            m_ResolvedColorBuffer = std::make_unique<GfxRenderTexture>(device, m_Name + "DisplayColorResolved", colorDesc, GfxTextureAllocStrategy::DefaultHeapCommitted);
            m_ResolvedDepthStencilBuffer = std::make_unique<GfxRenderTexture>(device, m_Name + "DisplayDepthStencilResolved", dsDesc, GfxTextureAllocStrategy::DefaultHeapCommitted);
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

    void Display::CreateMainDisplay(uint32_t width, uint32_t height)
    {
        s_MainDisplay = std::make_unique<Display>("Main", width, height);
    }

    void Display::DestroyMainDisplay()
    {
        s_MainDisplay.reset();
    }
}

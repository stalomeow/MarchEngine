#pragma once

#include "GfxResource.h"
#include "GfxDescriptorHeap.h"
#include <directx/d3dx12.h>
#include <DirectXTex.h>
#include <string>
#include <stdint.h>
#include <memory>

namespace march
{
    class GfxDevice;
    class GfxTexture2D;

    enum class GfxFilterMode
    {
        Point = 0,
        Bilinear = 1,
        Trilinear = 2,
    };

    enum class GfxWrapMode
    {
        Repeat = 0,
        Clamp = 1,
        Mirror = 2,
    };

    class GfxTexture : public GfxResource
    {
    protected:
        GfxTexture(GfxDevice* device);

    public:
        GfxTexture(const GfxTexture&) = delete;
        GfxTexture& operator=(const GfxTexture&) = delete;

        virtual ~GfxTexture();
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        void SetFilterMode(GfxFilterMode mode);
        void SetWrapMode(GfxWrapMode mode);
        void SetFilterAndWrapMode(GfxFilterMode filterMode, GfxWrapMode wrapMode);

        GfxFilterMode GetFilterMode() const { return m_FilterMode; }
        GfxWrapMode GetWrapMode() const { return m_WrapMode; }

        D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCpuDescriptorHandle() const { return m_SrvDescriptorHandle.GetCpuHandle(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerCpuDescriptorHandle() const { return m_SamplerDescriptorHandle.GetCpuHandle(); }

        static GfxTexture* GetDefaultBlack();
        static GfxTexture* GetDefaultWhite();

    private:
        void UpdateSampler() const;

        GfxFilterMode m_FilterMode;
        GfxWrapMode m_WrapMode;

        GfxDescriptorHandle m_SrvDescriptorHandle;
        GfxDescriptorHandle m_SamplerDescriptorHandle;

        static std::unique_ptr<GfxTexture2D> s_pBlackTexture;
        static std::unique_ptr<GfxTexture2D> s_pWhiteTexture;
    };

    enum class GfxTexture2DSourceType
    {
        DDS = 0,
        WIC = 1,
    };

    class GfxTexture2D : public GfxTexture
    {
    public:
        GfxTexture2D(GfxDevice* device);
        virtual ~GfxTexture2D() = default;

        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;
        bool GetIsSRGB() const;
        void SetIsSRGB(bool isSRGB);
        void LoadFromSource(const std::string& name, GfxTexture2DSourceType sourceType, const void* pSource, uint32_t size);

        const DirectX::TexMetadata& GetMetaData() const { return m_MetaData; }

    protected:
        bool m_IsSRGB;
        DirectX::TexMetadata m_MetaData;
    };

    struct GfxRenderTextureDesc
    {
        DXGI_FORMAT Format;
        uint32_t Width;
        uint32_t Height;
        uint32_t SampleCount;
        uint32_t SampleQuality;

        bool IsCompatibleWith(const GfxRenderTextureDesc& other) const;
    };

    class GfxRenderTexture : public GfxTexture
    {
    public:
        GfxRenderTexture(GfxDevice* device, const std::string& name, const GfxRenderTextureDesc& desc);
        GfxRenderTexture(GfxDevice* device, const std::string& name, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t sampleCount = 1, uint32_t sampleQuality = 0);
        virtual ~GfxRenderTexture();

        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;
        virtual DXGI_FORMAT GetFormat() const;
        virtual GfxRenderTextureDesc GetDesc() const;
        bool IsDepthStencilTexture() const;

        D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsvCpuDescriptorHandle() const { return m_RtvDsvDescriptorHandle.GetCpuHandle(); }

    protected:
        static bool IsDepthStencilFormat(DXGI_FORMAT format);
        static DXGI_FORMAT GetDepthStencilFormat(DXGI_FORMAT resFormat);
        static DXGI_FORMAT GetDepthStencilResFormat(DXGI_FORMAT format);
        static DXGI_FORMAT GetDepthStencilSRVFormat(DXGI_FORMAT format);

        GfxRenderTexture(GfxDevice* device, ID3D12Resource* resource, bool isDepthStencilTexture);

        GfxDescriptorHandle m_RtvDsvDescriptorHandle;
        bool m_IsDepthStencilTexture;
    };

    class GfxRenderTextureSwapChain : public GfxRenderTexture
    {
    public:
        GfxRenderTextureSwapChain(GfxDevice* device, ID3D12Resource* resource);
        virtual ~GfxRenderTextureSwapChain() = default;

        DXGI_FORMAT GetFormat() const override;
        GfxRenderTextureDesc GetDesc() const override;
    };
}

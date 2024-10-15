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

    class GfxTexture2D : public GfxTexture
    {
    public:
        GfxTexture2D(GfxDevice* device);
        virtual ~GfxTexture2D() = default;

        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;
        void LoadFromDDS(const std::string& name, const void* pSourceDDS, uint32_t size);

        const DirectX::TexMetadata& GetMetaData() const { return m_MetaData; }

    protected:
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
        GfxRenderTextureDesc GetDesc() const;
        bool IsDepthStencilTexture() const;

        D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsvCpuDescriptorHandle() const { return m_RtvDsvDescriptorHandle.GetCpuHandle(); }

    protected:
        static bool IsDepthStencilFormat(DXGI_FORMAT format);
        static DXGI_FORMAT GetDepthStencilResFormat(DXGI_FORMAT format);
        static DXGI_FORMAT GetDepthStencilSRVFormat(DXGI_FORMAT format);

        GfxDescriptorHandle m_RtvDsvDescriptorHandle;
    };
}

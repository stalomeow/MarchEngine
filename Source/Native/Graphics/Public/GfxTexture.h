#pragma once

#include "GfxResource.h"
#include "GfxDescriptor.h"
#include <directx/d3dx12.h>
#include <DirectXTex.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>

namespace march
{
    class GfxDevice;

    enum class GfxTextureFormat
    {
        R32G32B32A32_Float,
        R32G32B32A32_UInt,
        R32G32B32A32_SInt,
        R32G32B32_Float,
        R32G32B32_UInt,
        R32G32B32_SInt,
        R32G32_Float,
        R32G32_UInt,
        R32G32_SInt,
        R32_Float,
        R32_UInt,
        R32_SInt,

        R16G16B16A16_Float,
        R16G16B16A16_UNorm,
        R16G16B16A16_UInt,
        R16G16B16A16_SNorm,
        R16G16B16A16_SInt,
        R16G16_Float,
        R16G16_UNorm,
        R16G16_UInt,
        R16G16_SNorm,
        R16G16_SInt,
        R16_Float,
        R16_UNorm,
        R16_UInt,
        R16_SNorm,
        R16_SInt,

        R8G8B8A8_UNorm,
        R8G8B8A8_UINT,
        R8G8B8A8_SNorm,
        R8G8B8A8_SInt,
        R8G8_UNorm,
        R8G8_UInt,
        R8G8_SNorm,
        R8G8_SInt,
        R8_UNorm,
        R8_UInt,
        R8_SNorm,
        R8_SInt,
        A8_UNorm,

        R11G11B10_Float,
        R10G10B10A2_UNorm,
        R10G10B10A2_UInt,

        B5G6R5_UNorm,
        B5G5R5A1_UNorm,
        B8G8R8A8_UNorm,
        B8G8R8_UNorm,
        B4G4R4A4_UNorm,

        BC1_UNorm,
        BC2_UNorm,
        BC3_UNorm,
        BC4_UNorm,
        BC4_SNorm,
        BC5_UNorm,
        BC5_SNorm,
        BC6H_UF16,
        BC6H_SF16,
        BC7_UNorm,

        D32_Float_S8_UInt,
        D32_Float,
        D24_UNorm_S8_UInt,
        D16_UNorm,
    };

    enum class GfxTextureFlags
    {
        None = 0,
        SRGB = 1 << 0,
        MipMap = 1 << 1,
        Render = 1 << 2,
        UnorderedAccess = 1 << 3,
    };

    DEFINE_ENUM_FLAG_OPERATORS(GfxTextureFlags);

    enum class GfxTextureDimension
    {
        Unknown,
        Tex2D,
        Tex3D,
        Cube,
        Tex2DArray,
        CubeArray,
    };

    enum class GfxFilterMode
    {
        Point,
        Bilinear,
        Trilinear,
        Shadow,

        Anisotropic1,
        Anisotropic2,
        Anisotropic3,
        Anisotropic4,
        Anisotropic5,
        Anisotropic6,
        Anisotropic7,
        Anisotropic8,
        Anisotropic9,
        Anisotropic10,
        Anisotropic11,
        Anisotropic12,
        Anisotropic13,
        Anisotropic14,
        Anisotropic15,
        Anisotropic16,

        // Alias
        AnisotropicMin = Anisotropic1,
        AnisotropicMax = Anisotropic16,
    };

    enum class GfxWrapMode
    {
        Repeat,
        Clamp,
        Mirror,
        MirrorOnce,
    };

    enum class GfxTextureElement
    {
        Default, // 根据 Format 选择 Color 或 Depth
        Color,
        Depth,
        Stencil,
    };

    enum class CubemapFace
    {
        PositiveX = 0,
        NegativeX = 1,
        PositiveY = 2,
        NegativeY = 3,
        PositiveZ = 4,
        NegativeZ = 5,
    };

    struct GfxTextureDesc
    {
        GfxTextureFormat Format;
        GfxTextureFlags Flags;

        GfxTextureDimension Dimension;
        uint32_t Width;
        uint32_t Height;
        uint32_t DepthOrArraySize;
        uint32_t MSAASamples;

        GfxFilterMode Filter;
        GfxWrapMode Wrap;
        float MipMapBias;

        bool IsCompatibleWith(const GfxTextureDesc& other) const;
    };

    class GfxTexture : public GfxResource
    {
    protected:
        GfxTexture(GfxDevice* device, const GfxTextureDesc& desc);

    public:
        virtual ~GfxTexture();

        const GfxTextureDesc& GetDesc() const { return m_Desc; }

        D3D12_CPU_DESCRIPTOR_HANDLE GetSrv(GfxTextureElement element = GfxTextureElement::Default);
        D3D12_CPU_DESCRIPTOR_HANDLE GetUav(GfxTextureElement element = GfxTextureElement::Default);
        D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsv(uint32_t wOrArraySlice = 0, uint32_t wOrArraySize = 1, uint32_t mipSlice = 0);
        D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsv(CubemapFace face, uint32_t faceCount = 1, uint32_t arraySlice = 0, uint32_t mipSlice = 0);
        D3D12_CPU_DESCRIPTOR_HANDLE GetSampler();

        static GfxTexture* GetDefaultBlack();
        static GfxTexture* GetDefaultWhite();
        static GfxTexture* GetDefaultBump();
        static void DestroyDefaultTextures();

        GfxTexture(const GfxTexture&) = delete;
        GfxTexture& operator=(const GfxTexture&) = delete;

    private:
        GfxTextureDesc m_Desc;

        struct RtvDsvQuery
        {
            uint32_t WOrArraySlice;
            uint32_t WOrArraySize;
            uint32_t MipSlice;

            bool operator == (const RtvDsvQuery& other) const
            {
                return WOrArraySlice == other.WOrArraySlice && WOrArraySize == other.WOrArraySize && MipSlice == other.MipSlice;
            }
        };

        // TODO Hash function for RtvDsvQuery

        // Lazy creation
        std::pair<GfxDescriptorHandle, bool> m_SrvHandles[2];
        std::pair<GfxDescriptorHandle, bool> m_UavHandles[2];
        std::unordered_map<RtvDsvQuery, GfxDescriptorHandle> m_RtvDsvHandles;
        std::pair<GfxDescriptorHandle, bool> m_SamplerHandle;
    };

    enum class GfxTexture2DSourceType
    {
        DDS = 0,
        WIC = 1,
    };

    class GfxEnternTexture : public GfxTexture
    {

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

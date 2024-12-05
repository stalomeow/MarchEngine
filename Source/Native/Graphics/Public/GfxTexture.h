#pragma once

#include "GfxResource.h"
#include "GfxDescriptor.h"
#include <directx/d3dx12.h>
#include <DirectXTex.h>
#include <string>
#include <stdint.h>
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
        R8G8B8A8_UInt,
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
        Mipmaps = 1 << 1,
        UnorderedAccess = 1 << 2,
    };

    DEFINE_ENUM_FLAG_OPERATORS(GfxTextureFlags);

    enum class GfxTextureDimension
    {
        Tex2D,
        Tex3D,
        Cube,
        Tex2DArray,
        CubeArray,
    };

    enum class GfxTextureFilterMode
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

    enum class GfxTextureWrapMode
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

    enum class GfxCubemapFace
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
        uint32_t DepthOrArraySize; // Cubemap 时为 1，CubemapArray 时为 Cubemap 的数量，都不用乘 6
        uint32_t MSAASamples;

        GfxTextureFilterMode Filter;
        GfxTextureWrapMode Wrap;
        float MipmapBias;

        uint32_t GetDepthBits() const noexcept;
        bool HasStencil() const noexcept;
        bool IsDepthStencil() const noexcept;

        bool HasFlag(GfxTextureFlags flag) const noexcept;
        bool IsCompatibleWith(const GfxTextureDesc& other) const noexcept;

        // 如果 updateFlags 为 true，会根据 format 更新 Flags，例如 sRGB
        void SetDXGIFormat(DXGI_FORMAT format, bool updateFlags = false);
    };

    enum class GfxDefaultTexture
    {
        Black, // RGBA: 0, 0, 0, 1
        White, // RGBA: 1, 1, 1, 1
        Bump,  // RGBA: 0.5, 0.5, 1, 1
    };

    class GfxTexture : public GfxResource
    {
    protected:
        GfxTexture(GfxDevice* device, const GfxTextureDesc& desc);

    public:
        virtual ~GfxTexture();

        uint32_t GetMipLevels() const;
        const GfxTextureDesc& GetDesc() const { return m_Desc; }

        D3D12_CPU_DESCRIPTOR_HANDLE GetSrv(GfxTextureElement element = GfxTextureElement::Default);
        D3D12_CPU_DESCRIPTOR_HANDLE GetUav(GfxTextureElement element = GfxTextureElement::Default);
        D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsv(uint32_t wOrArraySlice = 0, uint32_t wOrArraySize = 1, uint32_t mipSlice = 0);
        D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsv(GfxCubemapFace face, uint32_t faceCount = 1, uint32_t arraySlice = 0, uint32_t mipSlice = 0);
        D3D12_CPU_DESCRIPTOR_HANDLE GetSampler();

        static GfxTexture* GetDefault(GfxDefaultTexture texture);

        virtual bool AllowRendering() const = 0;

        GfxTexture(const GfxTexture&) = delete;
        GfxTexture& operator=(const GfxTexture&) = delete;

    protected:
        virtual void Reset();
        void Reset(const GfxTextureDesc& desc);

    private:
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

        struct RtvDsvQueryHash
        {
            size_t operator()(const RtvDsvQuery& query) const
            {
                return std::hash<uint32_t>()(query.WOrArraySlice) ^ std::hash<uint32_t>()(query.WOrArraySize) ^ std::hash<uint32_t>()(query.MipSlice);
            }
        };

        void CreateRtvDsv(const RtvDsvQuery& query, GfxDescriptorHandle& handle);

        GfxTextureDesc m_Desc;

        // Lazy creation
        std::pair<GfxDescriptorHandle, bool> m_SrvHandles[2];
        std::pair<GfxDescriptorHandle, bool> m_UavHandles[2];
        std::unordered_map<RtvDsvQuery, GfxDescriptorHandle, RtvDsvQueryHash> m_RtvDsvHandles;
        std::pair<GfxDescriptorHandle, bool> m_SamplerHandle;
    };

    struct LoadTextureFileArgs
    {
        GfxTextureFlags Flags;
        GfxTextureFilterMode Filter;
        GfxTextureWrapMode Wrap;
        float MipmapBias;
        bool Compress;
    };

    class GfxExternalTexture : public GfxTexture
    {
    public:
        GfxExternalTexture(GfxDevice* device);
        void LoadFromPixels(const std::string& name, const GfxTextureDesc& desc, void* pixelsData, size_t pixelsSize, uint32_t mipLevels);
        void LoadFromFile(const std::string& name, const std::string& filePath, const LoadTextureFileArgs& args);

        const std::string& GetName() const { return m_Name; }
        bool AllowRendering() const override { return false; }
        uint8_t* GetPixelsData() const { return m_Image.GetPixels(); }
        size_t GetPixelsSize() const { return m_Image.GetPixelsSize(); }

    private:
        void UploadImage();

        std::string m_Name;
        DirectX::ScratchImage m_Image;
    };

    class GfxRenderTexture : public GfxTexture
    {
    public:
        GfxRenderTexture(GfxDevice* device, const std::string& name, const GfxTextureDesc& desc);

        // 这个构造方法会接管 resource 的所有权
        GfxRenderTexture(GfxDevice* device, const GfxTextureDesc& desc, ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

        bool AllowRendering() const override { return true; }
    };
}

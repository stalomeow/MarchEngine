#include "GfxTexture.h"
#include "GfxDevice.h"
#include "GfxBuffer.h"
#include "GfxCommand.h"
#include "GfxSettings.h"
#include "GfxUtils.h"
#include "StringUtils.h"
#include "DotNetRuntime.h"
#include "DotNetMarshal.h"
#include <DirectXColors.h>
#include <filesystem>

#undef min
#undef max

using namespace DirectX;
namespace fs = std::filesystem;

namespace march
{
    uint32_t GfxTextureDesc::GetDepthBits() const noexcept
    {
        switch (Format)
        {
        case GfxTextureFormat::D32_Float_S8_UInt:
        case GfxTextureFormat::D32_Float:
            return 32;
        case GfxTextureFormat::D24_UNorm_S8_UInt:
            return 24;
        case GfxTextureFormat::D16_UNorm:
            return 16;
        default:
            return 0;
        }
    }

    bool GfxTextureDesc::HasStencil() const noexcept
    {
        switch (Format)
        {
        case GfxTextureFormat::D32_Float_S8_UInt:
        case GfxTextureFormat::D24_UNorm_S8_UInt:
            return true;
        default:
            return false;
        }
    }

    bool GfxTextureDesc::IsDepthStencil() const noexcept
    {
        return GetDepthBits() > 0;
    }

    bool GfxTextureDesc::HasFlag(GfxTextureFlags flag) const noexcept
    {
        return (Flags & flag) == flag;
    }

    // TODO optimize for different dimensions
    bool GfxTextureDesc::IsCompatibleWith(const GfxTextureDesc& other) const noexcept
    {
        return Format == other.Format
            && Flags == other.Flags
            && Dimension == other.Dimension
            && Width == other.Width
            && Height == other.Height
            && DepthOrArraySize == other.DepthOrArraySize
            && MSAASamples == other.MSAASamples
            && Filter == other.Filter
            && Wrap == other.Wrap
            && MipmapBias == other.MipmapBias;
    }

    static DXGI_FORMAT GetResDXGIFormat(GfxTextureFormat format, bool sRGB, bool swapChain) noexcept
    {
        if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Linear)
        {
            // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space
            // SwapChain Resource 的 Format 不能有 _SRGB 后缀，只能在创建 RTV 时加上 _SRGB

            sRGB &= !swapChain;
        }
        else
        {
            sRGB = false; // 强制关闭 sRGB 转换
        }

        switch (format)
        {
        case GfxTextureFormat::R32G32B32A32_Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case GfxTextureFormat::R32G32B32A32_UInt:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case GfxTextureFormat::R32G32B32A32_SInt:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case GfxTextureFormat::R32G32B32_Float:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case GfxTextureFormat::R32G32B32_UInt:
            return DXGI_FORMAT_R32G32B32_UINT;
        case GfxTextureFormat::R32G32B32_SInt:
            return DXGI_FORMAT_R32G32B32_SINT;
        case GfxTextureFormat::R32G32_Float:
            return DXGI_FORMAT_R32G32_FLOAT;
        case GfxTextureFormat::R32G32_UInt:
            return DXGI_FORMAT_R32G32_UINT;
        case GfxTextureFormat::R32G32_SInt:
            return DXGI_FORMAT_R32G32_SINT;
        case GfxTextureFormat::R32_Float:
            return DXGI_FORMAT_R32_FLOAT;
        case GfxTextureFormat::R32_UInt:
            return DXGI_FORMAT_R32_UINT;
        case GfxTextureFormat::R32_SInt:
            return DXGI_FORMAT_R32_SINT;

        case GfxTextureFormat::R16G16B16A16_Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case GfxTextureFormat::R16G16B16A16_UNorm:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case GfxTextureFormat::R16G16B16A16_UInt:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case GfxTextureFormat::R16G16B16A16_SNorm:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case GfxTextureFormat::R16G16B16A16_SInt:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case GfxTextureFormat::R16G16_Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        case GfxTextureFormat::R16G16_UNorm:
            return DXGI_FORMAT_R16G16_UNORM;
        case GfxTextureFormat::R16G16_UInt:
            return DXGI_FORMAT_R16G16_UINT;
        case GfxTextureFormat::R16G16_SNorm:
            return DXGI_FORMAT_R16G16_SNORM;
        case GfxTextureFormat::R16G16_SInt:
            return DXGI_FORMAT_R16G16_SINT;
        case GfxTextureFormat::R16_Float:
            return DXGI_FORMAT_R16_FLOAT;
        case GfxTextureFormat::R16_UNorm:
            return DXGI_FORMAT_R16_UNORM;
        case GfxTextureFormat::R16_UInt:
            return DXGI_FORMAT_R16_UINT;
        case GfxTextureFormat::R16_SNorm:
            return DXGI_FORMAT_R16_SNORM;
        case GfxTextureFormat::R16_SInt:
            return DXGI_FORMAT_R16_SINT;

        case GfxTextureFormat::R8G8B8A8_UNorm:
            return sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        case GfxTextureFormat::R8G8B8A8_UInt:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case GfxTextureFormat::R8G8B8A8_SNorm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case GfxTextureFormat::R8G8B8A8_SInt:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case GfxTextureFormat::R8G8_UNorm:
            return DXGI_FORMAT_R8G8_UNORM;
        case GfxTextureFormat::R8G8_UInt:
            return DXGI_FORMAT_R8G8_UINT;
        case GfxTextureFormat::R8G8_SNorm:
            return DXGI_FORMAT_R8G8_SNORM;
        case GfxTextureFormat::R8G8_SInt:
            return DXGI_FORMAT_R8G8_SINT;
        case GfxTextureFormat::R8_UNorm:
            return DXGI_FORMAT_R8_UNORM;
        case GfxTextureFormat::R8_UInt:
            return DXGI_FORMAT_R8_UINT;
        case GfxTextureFormat::R8_SNorm:
            return DXGI_FORMAT_R8_SNORM;
        case GfxTextureFormat::R8_SInt:
            return DXGI_FORMAT_R8_SINT;
        case GfxTextureFormat::A8_UNorm:
            return DXGI_FORMAT_A8_UNORM;

        case GfxTextureFormat::R11G11B10_Float:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case GfxTextureFormat::R10G10B10A2_UNorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case GfxTextureFormat::R10G10B10A2_UInt:
            return DXGI_FORMAT_R10G10B10A2_UINT;

        case GfxTextureFormat::B5G6R5_UNorm:
            return DXGI_FORMAT_B5G6R5_UNORM;
        case GfxTextureFormat::B5G5R5A1_UNorm:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case GfxTextureFormat::B8G8R8A8_UNorm:
            return sRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        case GfxTextureFormat::B8G8R8_UNorm:
            return sRGB ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
        case GfxTextureFormat::B4G4R4A4_UNorm:
            return DXGI_FORMAT_B4G4R4A4_UNORM;

        case GfxTextureFormat::BC1_UNorm:
            return sRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
        case GfxTextureFormat::BC2_UNorm:
            return sRGB ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
        case GfxTextureFormat::BC3_UNorm:
            return sRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
        case GfxTextureFormat::BC4_UNorm:
            return DXGI_FORMAT_BC4_UNORM;
        case GfxTextureFormat::BC4_SNorm:
            return DXGI_FORMAT_BC4_SNORM;
        case GfxTextureFormat::BC5_UNorm:
            return DXGI_FORMAT_BC5_UNORM;
        case GfxTextureFormat::BC5_SNorm:
            return DXGI_FORMAT_BC5_SNORM;
        case GfxTextureFormat::BC6H_UF16:
            return DXGI_FORMAT_BC6H_UF16;
        case GfxTextureFormat::BC6H_SF16:
            return DXGI_FORMAT_BC6H_SF16;
        case GfxTextureFormat::BC7_UNorm:
            return sRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;

        case GfxTextureFormat::D32_Float_S8_UInt:
            return DXGI_FORMAT_R32G8X24_TYPELESS;
        case GfxTextureFormat::D32_Float:
            return DXGI_FORMAT_R32_TYPELESS;
        case GfxTextureFormat::D24_UNorm_S8_UInt:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case GfxTextureFormat::D16_UNorm:
            return DXGI_FORMAT_R16_TYPELESS;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    DXGI_FORMAT GfxTextureDesc::GetResDXGIFormat() const noexcept
    {
        bool sRGB = HasFlag(GfxTextureFlags::SRGB);
        bool swapChain = HasFlag(GfxTextureFlags::SwapChain);
        return ::march::GetResDXGIFormat(Format, sRGB, swapChain);
    }

    DXGI_FORMAT GfxTextureDesc::GetRtvDsvDXGIFormat() const noexcept
    {
        if (IsDepthStencil())
        {
            switch (Format)
            {
            case GfxTextureFormat::D32_Float_S8_UInt:
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            case GfxTextureFormat::D32_Float:
                return DXGI_FORMAT_D32_FLOAT;
            case GfxTextureFormat::D24_UNorm_S8_UInt:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case GfxTextureFormat::D16_UNorm:
                return DXGI_FORMAT_D16_UNORM;
            }
        }
        else
        {
            // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space
            // SwapChain Resource 的 Format 不能有 _SRGB 后缀，只能在创建 RTV 时加上 _SRGB

            bool sRGB = HasFlag(GfxTextureFlags::SRGB);
            return ::march::GetResDXGIFormat(Format, sRGB, /* swapChain */ false);
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    DXGI_FORMAT GfxTextureDesc::GetSrvUavDXGIFormat(GfxTextureElement element) const noexcept
    {
        if (IsDepthStencil())
        {
            if (element == GfxTextureElement::Default || element == GfxTextureElement::Depth)
            {
                switch (Format)
                {
                case GfxTextureFormat::D32_Float_S8_UInt:
                    return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                case GfxTextureFormat::D32_Float:
                    return DXGI_FORMAT_R32_FLOAT;
                case GfxTextureFormat::D24_UNorm_S8_UInt:
                    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                case GfxTextureFormat::D16_UNorm:
                    return DXGI_FORMAT_R16_UNORM;
                }
            }
            else if (element == GfxTextureElement::Stencil)
            {
                switch (Format)
                {
                case GfxTextureFormat::D32_Float_S8_UInt:
                    return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
                case GfxTextureFormat::D24_UNorm_S8_UInt:
                    return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
                }
            }
        }
        else if (element == GfxTextureElement::Default || element == GfxTextureElement::Color)
        {
            return GetResDXGIFormat();
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    D3D12_RESOURCE_FLAGS GfxTextureDesc::GetResFlags(bool allowRendering) const noexcept
    {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if (allowRendering)
        {
            if (IsDepthStencil())
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            else
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
        }

        if (HasFlag(GfxTextureFlags::UnorderedAccess))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        return flags;
    }

    void GfxTextureDesc::SetResDXGIFormat(DXGI_FORMAT format, bool updateFlags)
    {
        bool sRGB = false;

        switch (format)
        {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            Format = GfxTextureFormat::R32G32B32A32_Float;
            break;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            Format = GfxTextureFormat::R32G32B32A32_UInt;
            break;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            Format = GfxTextureFormat::R32G32B32A32_SInt;
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            Format = GfxTextureFormat::R32G32B32_Float;
            break;
        case DXGI_FORMAT_R32G32B32_UINT:
            Format = GfxTextureFormat::R32G32B32_UInt;
            break;
        case DXGI_FORMAT_R32G32B32_SINT:
            Format = GfxTextureFormat::R32G32B32_SInt;
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            Format = GfxTextureFormat::R32G32_Float;
            break;
        case DXGI_FORMAT_R32G32_UINT:
            Format = GfxTextureFormat::R32G32_UInt;
            break;
        case DXGI_FORMAT_R32G32_SINT:
            Format = GfxTextureFormat::R32G32_SInt;
            break;
        case DXGI_FORMAT_R32_FLOAT:
            Format = GfxTextureFormat::R32_Float;
            break;
        case DXGI_FORMAT_R32_UINT:
            Format = GfxTextureFormat::R32_UInt;
            break;
        case DXGI_FORMAT_R32_SINT:
            Format = GfxTextureFormat::R32_SInt;
            break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            Format = GfxTextureFormat::R16G16B16A16_Float;
            break;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            Format = GfxTextureFormat::R16G16B16A16_UNorm;
            break;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            Format = GfxTextureFormat::R16G16B16A16_UInt;
            break;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            Format = GfxTextureFormat::R16G16B16A16_SNorm;
            break;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            Format = GfxTextureFormat::R16G16B16A16_SInt;
            break;
        case DXGI_FORMAT_R16G16_FLOAT:
            Format = GfxTextureFormat::R16G16_Float;
            break;
        case DXGI_FORMAT_R16G16_UNORM:
            Format = GfxTextureFormat::R16G16_UNorm;
            break;
        case DXGI_FORMAT_R16G16_UINT:
            Format = GfxTextureFormat::R16G16_UInt;
            break;
        case DXGI_FORMAT_R16G16_SNORM:
            Format = GfxTextureFormat::R16G16_SNorm;
            break;
        case DXGI_FORMAT_R16G16_SINT:
            Format = GfxTextureFormat::R16G16_SInt;
            break;
        case DXGI_FORMAT_R16_FLOAT:
            Format = GfxTextureFormat::R16_Float;
            break;
        case DXGI_FORMAT_R16_UNORM:
            Format = GfxTextureFormat::R16_UNorm;
            break;
        case DXGI_FORMAT_R16_UINT:
            Format = GfxTextureFormat::R16_UInt;
            break;
        case DXGI_FORMAT_R16_SNORM:
            Format = GfxTextureFormat::R16_SNorm;
            break;
        case DXGI_FORMAT_R16_SINT:
            Format = GfxTextureFormat::R16_SInt;
            break;

        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            sRGB = true;
            [[fallthrough]];
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            Format = GfxTextureFormat::R8G8B8A8_UNorm;
            break;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            Format = GfxTextureFormat::R8G8B8A8_UInt;
            break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            Format = GfxTextureFormat::R8G8B8A8_SNorm;
            break;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            Format = GfxTextureFormat::R8G8B8A8_SInt;
            break;
        case DXGI_FORMAT_R8G8_UNORM:
            Format = GfxTextureFormat::R8G8_UNorm;
            break;
        case DXGI_FORMAT_R8G8_UINT:
            Format = GfxTextureFormat::R8G8_UInt;
            break;
        case DXGI_FORMAT_R8G8_SNORM:
            Format = GfxTextureFormat::R8G8_SNorm;
            break;
        case DXGI_FORMAT_R8G8_SINT:
            Format = GfxTextureFormat::R8G8_SInt;
            break;
        case DXGI_FORMAT_R8_UNORM:
            Format = GfxTextureFormat::R8_UNorm;
            break;
        case DXGI_FORMAT_R8_UINT:
            Format = GfxTextureFormat::R8_UInt;
            break;
        case DXGI_FORMAT_R8_SNORM:
            Format = GfxTextureFormat::R8_SNorm;
            break;
        case DXGI_FORMAT_R8_SINT:
            Format = GfxTextureFormat::R8_SInt;
            break;
        case DXGI_FORMAT_A8_UNORM:
            Format = GfxTextureFormat::A8_UNorm;
            break;

        case DXGI_FORMAT_R11G11B10_FLOAT:
            Format = GfxTextureFormat::R11G11B10_Float;
            break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            Format = GfxTextureFormat::R10G10B10A2_UNorm;
            break;
        case DXGI_FORMAT_R10G10B10A2_UINT:
            Format = GfxTextureFormat::R10G10B10A2_UInt;
            break;

        case DXGI_FORMAT_B5G6R5_UNORM:
            Format = GfxTextureFormat::B5G6R5_UNorm;
            break;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            Format = GfxTextureFormat::B5G5R5A1_UNorm;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            sRGB = true;
            [[fallthrough]];
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            Format = GfxTextureFormat::B8G8R8A8_UNorm;
            break;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            sRGB = true;
            [[fallthrough]];
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            Format = GfxTextureFormat::B8G8R8_UNorm;
            break;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            Format = GfxTextureFormat::B4G4R4A4_UNorm;
            break;

        case DXGI_FORMAT_BC1_UNORM_SRGB:
            sRGB = true;
            [[fallthrough]];
        case DXGI_FORMAT_BC1_UNORM:
            Format = GfxTextureFormat::BC1_UNorm;
            break;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            sRGB = true;
            [[fallthrough]];
        case DXGI_FORMAT_BC2_UNORM:
            Format = GfxTextureFormat::BC2_UNorm;
            break;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            sRGB = true;
            [[fallthrough]];
        case DXGI_FORMAT_BC3_UNORM:
            Format = GfxTextureFormat::BC3_UNorm;
            break;
        case DXGI_FORMAT_BC4_UNORM:
            Format = GfxTextureFormat::BC4_UNorm;
            break;
        case DXGI_FORMAT_BC4_SNORM:
            Format = GfxTextureFormat::BC4_SNorm;
            break;
        case DXGI_FORMAT_BC5_UNORM:
            Format = GfxTextureFormat::BC5_UNorm;
            break;
        case DXGI_FORMAT_BC5_SNORM:
            Format = GfxTextureFormat::BC5_SNorm;
            break;
        case DXGI_FORMAT_BC6H_UF16:
            Format = GfxTextureFormat::BC6H_UF16;
            break;
        case DXGI_FORMAT_BC6H_SF16:
            Format = GfxTextureFormat::BC6H_SF16;
            break;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            sRGB = true;
            [[fallthrough]];
        case DXGI_FORMAT_BC7_UNORM:
            Format = GfxTextureFormat::BC7_UNorm;
            break;

        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            Format = GfxTextureFormat::D32_Float_S8_UInt;
            break;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_TYPELESS:
            Format = GfxTextureFormat::D32_Float;
            break;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            Format = GfxTextureFormat::D24_UNorm_S8_UInt;
            break;
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_TYPELESS:
            Format = GfxTextureFormat::D16_UNorm;
            break;

        default:
            throw GfxException("Invalid DXGI_FORMAT");
        }

        if (updateFlags)
        {
            if (sRGB) Flags |= GfxTextureFlags::SRGB;
            else      Flags &= ~GfxTextureFlags::SRGB;
        }
    }

    GfxTexture::GfxTexture(GfxDevice* device, const GfxTextureDesc& desc)
        : GfxResource(device, D3D12_RESOURCE_STATE_COMMON)
        , m_Desc(desc)
        , m_SrvHandles{}
        , m_UavHandles{}
        , m_RtvDsvHandles{}
        , m_SamplerHandle{}
    {
    }

    GfxTexture::~GfxTexture() { Reset(); }

    uint32_t GfxTexture::GetMipLevels() const
    {
        return static_cast<uint32_t>(m_Resource->GetDesc().MipLevels);
    }

    static size_t GetSrvUavIndex(const GfxTextureDesc& desc, GfxTextureElement element)
    {
        if (desc.IsDepthStencil())
        {
            if (element == GfxTextureElement::Default || element == GfxTextureElement::Depth)
            {
                return 0;
            }
            else if (element == GfxTextureElement::Stencil)
            {
                return 1;
            }
        }
        else if (element == GfxTextureElement::Default || element == GfxTextureElement::Color)
        {
            return 0;
        }

        throw GfxException("Invalid texture element");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetSrv(GfxTextureElement element)
    {
        std::pair<GfxDescriptorHandle, bool>& handle = m_SrvHandles[GetSrvUavIndex(m_Desc, element)];

        if (!handle.second)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = m_Desc.GetSrvUavDXGIFormat(element);
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            if (m_Desc.MSAASamples > 1)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Tex2DArray:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid srv dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = -1; // Set to -1 to indicate all the mipmap levels from MostDetailedMip on down to least detailed.
                    srvDesc.Texture2D.PlaneSlice = 0;
                    srvDesc.Texture2D.ResourceMinLODClamp = 0;
                    break;
                case GfxTextureDimension::Tex3D:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                    srvDesc.Texture3D.MostDetailedMip = 0;
                    srvDesc.Texture3D.MipLevels = -1;
                    srvDesc.Texture3D.ResourceMinLODClamp = 0;
                    break;
                case GfxTextureDimension::Cube:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    srvDesc.TextureCube.MostDetailedMip = 0;
                    srvDesc.TextureCube.MipLevels = -1;
                    srvDesc.TextureCube.ResourceMinLODClamp = 0;
                    break;
                case GfxTextureDimension::Tex2DArray:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.MostDetailedMip = 0;
                    srvDesc.Texture2DArray.MipLevels = -1;
                    srvDesc.Texture2DArray.ResourceMinLODClamp = 0;
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    srvDesc.Texture2DArray.PlaneSlice = 0;
                    break;
                case GfxTextureDimension::CubeArray:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    srvDesc.TextureCubeArray.MostDetailedMip = 0;
                    srvDesc.TextureCubeArray.MipLevels = -1;
                    srvDesc.TextureCubeArray.ResourceMinLODClamp = 0;
                    srvDesc.TextureCubeArray.First2DArrayFace = 0;
                    srvDesc.TextureCubeArray.NumCubes = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid srv dimension");
                }
            }

            handle.first = m_Device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.second = true;
            m_Device->GetD3D12Device()->CreateShaderResourceView(m_Resource, &srvDesc, handle.first.GetCpuHandle());
        }

        return handle.first.GetCpuHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetUav(GfxTextureElement element)
    {
        if (!m_Desc.HasFlag(GfxTextureFlags::UnorderedAccess))
        {
            throw GfxException("Texture is not created with UnorderedAccess flag");
        }

        std::pair<GfxDescriptorHandle, bool>& handle = m_UavHandles[GetSrvUavIndex(m_Desc, element)];

        if (!handle.second)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = m_Desc.GetSrvUavDXGIFormat(element);

            if (m_Desc.MSAASamples > 1)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Cube:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                    uavDesc.Texture2DMSArray.FirstArraySlice = 0;
                    uavDesc.Texture2DMSArray.ArraySize = 6;
                    break;
                case GfxTextureDimension::Tex2DArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                    uavDesc.Texture2DMSArray.FirstArraySlice = 0;
                    uavDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                case GfxTextureDimension::CubeArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                    uavDesc.Texture2DMSArray.FirstArraySlice = 0;
                    uavDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize) * 6;
                    break;
                default:
                    throw GfxException("Invalid uav dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = 0;
                    uavDesc.Texture2D.PlaneSlice = 0;
                    break;
                case GfxTextureDimension::Tex3D:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                    uavDesc.Texture3D.MipSlice = 0;
                    uavDesc.Texture3D.FirstWSlice = 0;
                    uavDesc.Texture3D.WSize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                case GfxTextureDimension::Cube:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = 0;
                    uavDesc.Texture2DArray.FirstArraySlice = 0;
                    uavDesc.Texture2DArray.ArraySize = 6;
                    uavDesc.Texture2DArray.PlaneSlice = 0;
                    break;
                case GfxTextureDimension::Tex2DArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = 0;
                    uavDesc.Texture2DArray.FirstArraySlice = 0;
                    uavDesc.Texture2DArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    uavDesc.Texture2DArray.PlaneSlice = 0;
                    break;
                case GfxTextureDimension::CubeArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = 0;
                    uavDesc.Texture2DArray.FirstArraySlice = 0;
                    uavDesc.Texture2DArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize) * 6;
                    uavDesc.Texture2DArray.PlaneSlice = 0;
                    break;
                default:
                    throw GfxException("Invalid uav dimension");
                }
            }

            handle.first = m_Device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.second = true;
            m_Device->GetD3D12Device()->CreateUnorderedAccessView(m_Resource, nullptr, &uavDesc, handle.first.GetCpuHandle());
        }

        return handle.first.GetCpuHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetRtvDsv(uint32_t wOrArraySlice, uint32_t wOrArraySize, uint32_t mipSlice)
    {
        if (!AllowRendering())
        {
            throw GfxException("Texture is not allowed for rendering");
        }

        RtvDsvQuery query = { wOrArraySlice, wOrArraySize, mipSlice };
        auto [it, isNew] = m_RtvDsvHandles.try_emplace(query);

        if (isNew)
        {
            CreateRtvDsv(query, it->second);
        }

        return it->second.GetCpuHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetRtvDsv(GfxCubemapFace face, uint32_t faceCount, uint32_t arraySlice, uint32_t mipSlice)
    {
        uint32_t wOrArraySlice = static_cast<uint32_t>(face) + arraySlice * 6u; // 展开为 Texture2DArray
        uint32_t wOrArraySize = faceCount;
        return GetRtvDsv(wOrArraySlice, wOrArraySize, mipSlice);
    }

    void GfxTexture::CreateRtvDsv(const RtvDsvQuery& query, GfxDescriptorHandle& handle)
    {
        ID3D12Device4* d3d12Device = m_Device->GetD3D12Device();

        if (m_Desc.IsDepthStencil())
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = m_Desc.GetRtvDsvDXGIFormat();
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            if (m_Desc.MSAASamples > 1)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    dsvDesc.Texture2DMSArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    dsvDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid depth stencil dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                    dsvDesc.Texture2D.MipSlice = static_cast<UINT>(query.MipSlice);
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsvDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    dsvDesc.Texture2DArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    dsvDesc.Texture2DArray.MipSlice = static_cast<UINT>(query.MipSlice);
                    break;
                default:
                    throw GfxException("Invalid depth stencil dimension");
                }
            }

            handle = m_Device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            d3d12Device->CreateDepthStencilView(m_Resource, &dsvDesc, handle.GetCpuHandle());
        }
        else
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = m_Desc.GetRtvDsvDXGIFormat();

            if (m_Desc.Dimension == GfxTextureDimension::Tex3D)
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.FirstWSlice = static_cast<UINT>(query.WOrArraySlice);
                rtvDesc.Texture3D.WSize = static_cast<UINT>(query.WOrArraySize);
                rtvDesc.Texture3D.MipSlice = static_cast<UINT>(query.MipSlice);
            }
            else if (m_Desc.MSAASamples > 1)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    rtvDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid render target dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = static_cast<UINT>(query.MipSlice);
                    rtvDesc.Texture2D.PlaneSlice = 0;
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    rtvDesc.Texture2DArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    rtvDesc.Texture2DArray.MipSlice = static_cast<UINT>(query.MipSlice);
                    rtvDesc.Texture2DArray.PlaneSlice = 0;
                    break;
                default:
                    throw GfxException("Invalid render target dimension");
                }
            }

            handle = m_Device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            d3d12Device->CreateRenderTargetView(m_Resource, &rtvDesc, handle.GetCpuHandle());
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetSampler()
    {
        if (!m_SamplerHandle.second)
        {
            D3D12_SAMPLER_DESC samplerDesc = {};
            samplerDesc.MipLODBias = m_Desc.MipmapBias;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

            if (m_Desc.Filter >= GfxTextureFilterMode::AnisotropicMin && m_Desc.Filter <= GfxTextureFilterMode::AnisotropicMax)
            {
                samplerDesc.MaxAnisotropy = static_cast<UINT>(m_Desc.Filter) - static_cast<UINT>(GfxTextureFilterMode::AnisotropicMin) + 1;
                samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
            }
            else
            {
                samplerDesc.MaxAnisotropy = 1;

                // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_filter
                // Note  If you use different filter types for min versus mag filter,
                // undefined behavior occurs in certain cases where the choice between whether magnification or minification happens is ambiguous.
                // To prevent this undefined behavior, use filter modes that use similar filter operations for both min and mag
                // (or use anisotropic filtering, which avoids the issue as well).
                switch (m_Desc.Filter)
                {
                case GfxTextureFilterMode::Point:
                    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                    break;

                case GfxTextureFilterMode::Bilinear:
                    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    break;

                case GfxTextureFilterMode::Trilinear:
                    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                    break;

                case GfxTextureFilterMode::Shadow:
                    samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
                    samplerDesc.ComparisonFunc = GfxSettings::UseReversedZBuffer ? D3D12_COMPARISON_FUNC_GREATER_EQUAL : D3D12_COMPARISON_FUNC_LESS_EQUAL;
                    break;

                default:
                    throw GfxException("Invalid filter mode");
                }
            }

            switch (m_Desc.Wrap)
            {
            case GfxTextureWrapMode::Repeat:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                break;

            case GfxTextureWrapMode::Clamp:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                break;

            case GfxTextureWrapMode::Mirror:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                break;

            case GfxTextureWrapMode::MirrorOnce:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
                break;

            default:
                throw GfxException("Invalid wrap mode");
            }

            m_SamplerHandle.first = m_Device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            m_SamplerHandle.second = true;
            m_Device->GetD3D12Device()->CreateSampler(&samplerDesc, m_SamplerHandle.first.GetCpuHandle());
        }

        return m_SamplerHandle.first.GetCpuHandle();
    }

    void GfxTexture::Reset()
    {
        ReleaseD3D12Resource();

        for (size_t i = 0; i < std::size(m_SrvHandles); i++)
        {
            if (m_SrvHandles[i].second)
            {
                m_Device->FreeDescriptor(m_SrvHandles[i].first);
                m_SrvHandles[i].second = false;
            }
        }

        for (size_t i = 0; i < std::size(m_UavHandles); i++)
        {
            if (m_UavHandles[i].second)
            {
                m_Device->FreeDescriptor(m_UavHandles[i].first);
                m_UavHandles[i].second = false;
            }
        }

        for (auto& kv : m_RtvDsvHandles)
        {
            m_Device->FreeDescriptor(kv.second);
        }
        m_RtvDsvHandles.clear();

        if (m_SamplerHandle.second)
        {
            m_Device->FreeDescriptor(m_SamplerHandle.first);
            m_SamplerHandle.second = false;
        }
    }

    void GfxTexture::Reset(const GfxTextureDesc& desc)
    {
        Reset();
        m_Desc = desc;
    }

    GfxTexture* GfxTexture::GetDefault(GfxDefaultTexture texture, GfxTextureDimension dimension)
    {
        cs<GfxDefaultTexture> csTexture{};
        csTexture.assign(texture);
        cs<GfxTextureDimension> csDimension{};
        csDimension.assign(dimension);
        return DotNet::RuntimeInvoke<GfxTexture*>(ManagedMethod::Texture_NativeGetDefault, csTexture, csDimension);
    }

    GfxExternalTexture::GfxExternalTexture(GfxDevice* device) : GfxTexture(device, {}), m_Name{}, m_Image{} {}

    void GfxExternalTexture::LoadFromPixels(const std::string& name, const GfxTextureDesc& desc, void* pixelsData, size_t pixelsSize, uint32_t mipLevels)
    {
        m_Name = name;
        Reset(desc);

        DXGI_FORMAT format = desc.GetResDXGIFormat();
        size_t width = static_cast<size_t>(desc.Width);
        size_t height = static_cast<size_t>(desc.Height);
        size_t depthOrArraySize = static_cast<size_t>(desc.DepthOrArraySize);

        switch (desc.Dimension)
        {
        case GfxTextureDimension::Tex2D:
        case GfxTextureDimension::Tex2DArray:
            GFX_HR(m_Image.Initialize2D(format, width, height, depthOrArraySize, static_cast<size_t>(mipLevels), CP_FLAGS_NONE));
            break;
        case GfxTextureDimension::Tex3D:
            GFX_HR(m_Image.Initialize3D(format, width, height, depthOrArraySize, static_cast<size_t>(mipLevels), CP_FLAGS_NONE));
            break;
        case GfxTextureDimension::Cube:
        case GfxTextureDimension::CubeArray:
            GFX_HR(m_Image.InitializeCube(format, width, height, depthOrArraySize, static_cast<size_t>(mipLevels), CP_FLAGS_NONE));
            break;
        default:
            throw GfxException("Invalid texture dimension");
        }

        if (m_Image.GetPixelsSize() != pixelsSize)
        {
            throw GfxException("Invalid pixel size");
        }

        memcpy(m_Image.GetPixels(), pixelsData, pixelsSize);

        ID3D12Device4* d3d12Device = m_Device->GetD3D12Device();
        D3D12_RESOURCE_FLAGS resFlags = desc.GetResFlags(false);
        GFX_HR(CreateTextureEx(d3d12Device, m_Image.GetMetadata(), resFlags, CREATETEX_DEFAULT, &m_Resource));
        m_State = D3D12_RESOURCE_STATE_COMMON; // CreateTextureEx 使用的 state
        SetD3D12ResourceName(name);

        UploadImage();
    }

    static DXGI_FORMAT GetCompressedFormat(const ScratchImage& image, GfxTextureCompression compression)
    {
        // https://docs.unity3d.com/6000.0/Documentation/Manual/texture-choose-format-by-platform.html
        // https://docs.unity3d.com/6000.0/Documentation/Manual/texture-formats-reference.html

        DXGI_FORMAT format = image.GetMetadata().format;

        if (IsCompressed(format))
        {
            throw GfxException("Texture format is already compressed");
        }

        DXGI_FORMAT result;

        if (HasAlpha(format) && !image.IsAlphaAllOpaque())
        {
            switch (compression)
            {
            case GfxTextureCompression::NormalQuality:
                result = DXGI_FORMAT_BC3_UNORM;
                break;
            case GfxTextureCompression::HighQuality:
                result = DXGI_FORMAT_BC7_UNORM;
                break;
            case GfxTextureCompression::LowQuality:
                result = DXGI_FORMAT_BC3_UNORM;
                break;
            default:
                throw GfxException("Invalid texture compression");
            }
        }
        else
        {
            switch (compression)
            {
            case GfxTextureCompression::NormalQuality:
                result = DXGI_FORMAT_BC1_UNORM;
                break;
            case GfxTextureCompression::HighQuality:
                result = DXGI_FORMAT_BC7_UNORM;
                break;
            case GfxTextureCompression::LowQuality:
                result = DXGI_FORMAT_BC1_UNORM;
                break;
            default:
                throw GfxException("Invalid texture compression");
            }
        }

        if (IsSRGB(format))
        {
            result = MakeSRGB(result);
        }

        return result;
    }

    void GfxExternalTexture::LoadFromFile(const std::string& name, const std::string& filePath, const LoadTextureFileArgs& args)
    {
        GfxTextureDesc desc = {};
        desc.Flags = args.Flags;
        desc.MSAASamples = 1;
        desc.Filter = args.Filter;
        desc.Wrap = args.Wrap;
        desc.MipmapBias = args.MipmapBias;

        std::wstring wFilePath = StringUtils::Utf8ToUtf16(filePath);
        if (fs::path(filePath).extension() == ".dds")
        {
            GFX_HR(LoadFromDDSFile(wFilePath.c_str(), DDS_FLAGS_NONE, nullptr, m_Image));
        }
        else
        {
            GFX_HR(LoadFromWICFile(wFilePath.c_str(), WIC_FLAGS_NONE, nullptr, m_Image));
        }

        if (IsCompressed(m_Image.GetMetadata().format))
        {
            ScratchImage decompressed;
            GFX_HR(Decompress(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), DXGI_FORMAT_UNKNOWN, decompressed));
            m_Image = std::move(decompressed);
        }

        if (desc.HasFlag(GfxTextureFlags::Mipmaps))
        {
            if (m_Image.GetMetadata().mipLevels == 1 && (m_Image.GetMetadata().width > 1 || m_Image.GetMetadata().height > 1))
            {
                ScratchImage mipChain;

                if (m_Image.GetMetadata().dimension == TEX_DIMENSION_TEXTURE3D)
                {
                    // https://github.com/microsoft/DirectXTex/wiki/GenerateMipMaps3D
                    // This function does not operate directly on block compressed images.
                    GFX_HR(GenerateMipMaps3D(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), TEX_FILTER_BOX, 0, mipChain));
                }
                else
                {
                    // https://github.com/microsoft/DirectXTex/wiki/GenerateMipMaps
                    // This function does not operate directly on block compressed images.
                    GFX_HR(GenerateMipMaps(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), TEX_FILTER_BOX, 0, mipChain));
                }

                m_Image = std::move(mipChain);
            }
        }
        else if (m_Image.GetMetadata().mipLevels > 1)
        {
            TexMetadata metadata = m_Image.GetMetadata();
            metadata.mipLevels = 1; // Remove mipmaps

            ScratchImage level0;
            GFX_HR(level0.Initialize(metadata, CP_FLAGS_NONE));

            if (metadata.dimension == TEX_DIMENSION_TEXTURE3D)
            {
                for (size_t i = 0; i < metadata.depth; i++)
                {
                    const Image* src = m_Image.GetImage(0, 0, i);
                    const Image* dst = level0.GetImage(0, 0, i);
                    memcpy(dst->pixels, src->pixels, src->slicePitch);
                }
            }
            else
            {
                for (size_t i = 0; i < metadata.arraySize; i++)
                {
                    const Image* src = m_Image.GetImage(0, i, 0);
                    const Image* dst = level0.GetImage(0, i, 0);
                    memcpy(dst->pixels, src->pixels, src->slicePitch);
                }
            }

            m_Image = std::move(level0);
        }

        if (args.Compression != GfxTextureCompression::None)
        {
            ScratchImage compressed;

            // TODO 目前 BC7 压缩速度巨慢
            TEX_COMPRESS_FLAGS flags = TEX_COMPRESS_BC7_QUICK | TEX_COMPRESS_PARALLEL;

            if (!desc.HasFlag(GfxTextureFlags::SRGB))
            {
                // By default, BC1-3 uses a perceptual weighting.
                // By using this flag, the perceptual weighting is disabled which can be useful
                // when using the RGB channels for other data.
                flags |= TEX_COMPRESS_UNIFORM;
            }

            DXGI_FORMAT targetFormat = GetCompressedFormat(m_Image, args.Compression);
            GFX_HR(Compress(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), targetFormat, flags, TEX_THRESHOLD_DEFAULT, compressed));
            m_Image = std::move(compressed);
        }

        const TexMetadata& metadata = m_Image.GetMetadata();
        desc.SetResDXGIFormat(metadata.format);
        desc.Width = static_cast<uint32_t>(metadata.width);
        desc.Height = static_cast<uint32_t>(metadata.height);

        switch (metadata.dimension)
        {
        case TEX_DIMENSION_TEXTURE2D:
            if (metadata.IsCubemap())
            {
                desc.DepthOrArraySize = static_cast<uint32_t>(metadata.arraySize / 6);
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::CubeArray : GfxTextureDimension::Cube;
            }
            else
            {
                desc.DepthOrArraySize = static_cast<uint32_t>(metadata.arraySize);
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::Tex2DArray : GfxTextureDimension::Tex2D;
            }
            break;
        case TEX_DIMENSION_TEXTURE3D:
            desc.DepthOrArraySize = static_cast<uint32_t>(metadata.depth);
            desc.Dimension = GfxTextureDimension::Tex3D;
            break;
        default:
            throw GfxException("Invalid texture dimension");
        }

        // https://github.com/microsoft/DirectXTex/wiki/CreateTexture
        // The CREATETEX_SRGB flag provides an option for working around gamma issues with content
        // that is in the sRGB or similar color space but is not encoded explicitly as an SRGB format.
        // This will force the resource format be one of the of DXGI_FORMAT_*_SRGB formats if it exist.
        // Note that no pixel data conversion takes place.
        // The CREATETEX_IGNORE_SRGB flag does the opposite;
        // it will force the resource format to not have the _*_SRGB version.
        CREATETEX_FLAGS createFlags;

        if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Linear)
        {
            createFlags = desc.HasFlag(GfxTextureFlags::SRGB) ? CREATETEX_FORCE_SRGB : CREATETEX_IGNORE_SRGB;
        }
        else
        {
            // shader 中采样时不进行任何转换
            createFlags = CREATETEX_IGNORE_SRGB;
        }

        m_Name = name;
        Reset(desc);

        // https://github.com/microsoft/DirectXTex/wiki/CreateTexture#directx-12

        ID3D12Device4* d3d12Device = m_Device->GetD3D12Device();
        D3D12_RESOURCE_FLAGS resFlags = desc.GetResFlags(false);
        GFX_HR(CreateTextureEx(d3d12Device, metadata, resFlags, createFlags, &m_Resource));
        m_State = D3D12_RESOURCE_STATE_COMMON; // CreateTextureEx 使用的 state
        SetD3D12ResourceName(name);

        UploadImage();
    }

    void GfxExternalTexture::UploadImage()
    {
        std::vector<D3D12_SUBRESOURCE_DATA> subresources{};
        GFX_HR(PrepareUpload(m_Device->GetD3D12Device(), m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), subresources));

        // upload is implemented by application developer. Here's one solution using <d3dx12.h>
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_Resource, 0, static_cast<UINT>(subresources.size()));

        GfxUploadMemory m = m_Device->AllocateTransientUploadMemory(static_cast<uint32_t>(uploadBufferSize));
        UINT64 mOffset = static_cast<UINT64>(m.GetD3D12ResourceOffset(0));
        UpdateSubresources(m_Device->GetGraphicsCommandList()->GetD3D12CommandList(), m_Resource,
            m.GetD3D12Resource(), mOffset, 0, static_cast<UINT>(subresources.size()), subresources.data());
    }

    GfxRenderTexture::GfxRenderTexture(GfxDevice* device, const std::string& name, const GfxTextureDesc& desc)
        : GfxTexture(device, desc)
    {
        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Alignment = 0;
        resDesc.Width = static_cast<UINT64>(std::max(1u, desc.Width));
        resDesc.Height = static_cast<UINT>(std::max(1u, desc.Height));
        resDesc.DepthOrArraySize = static_cast<UINT16>(desc.DepthOrArraySize);
        resDesc.MipLevels = 1;
        resDesc.Format = desc.GetResDXGIFormat();
        resDesc.SampleDesc.Count = static_cast<UINT>(desc.MSAASamples);
        resDesc.SampleDesc.Quality = static_cast<UINT>(device->GetMSAAQuality(resDesc.Format, resDesc.SampleDesc.Count));
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resDesc.Flags = desc.GetResFlags(true);

        switch (desc.Dimension)
        {
        case GfxTextureDimension::Tex2D:
        case GfxTextureDimension::Tex2DArray:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case GfxTextureDimension::Cube:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.DepthOrArraySize = 6u;
            break;
        case GfxTextureDimension::CubeArray:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.DepthOrArraySize *= 6u;
            break;
        case GfxTextureDimension::Tex3D:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
        default:
            throw GfxException("Invalid texture dimension");
        }

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = desc.GetRtvDsvDXGIFormat();

        if (desc.IsDepthStencil())
        {
            clearValue.DepthStencil.Depth = GfxUtils::FarClipPlaneDepth;
            clearValue.DepthStencil.Stencil = 0;

            m_State = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
        else
        {
            memcpy(clearValue.Color, Colors::Black, sizeof(clearValue.Color));

            m_State = D3D12_RESOURCE_STATE_COMMON;
        }

        GFX_HR(device->GetD3D12Device()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &resDesc, m_State, &clearValue, IID_PPV_ARGS(&m_Resource)));
        SetD3D12ResourceName(name);
    }

    GfxRenderTexture::GfxRenderTexture(GfxDevice* device, ID3D12Resource* resource, const GfxTextureResourceDesc& resDesc)
        : GfxTexture(device, {})
    {
        D3D12_RESOURCE_DESC d3d12Desc = resource->GetDesc();

        GfxTextureDesc desc{};
        desc.SetResDXGIFormat(d3d12Desc.Format);
        desc.Flags = resDesc.Flags;
        desc.Width = static_cast<uint32_t>(d3d12Desc.Width);
        desc.Height = static_cast<uint32_t>(d3d12Desc.Height);
        desc.DepthOrArraySize = static_cast<uint32_t>(d3d12Desc.DepthOrArraySize);
        desc.MSAASamples = static_cast<uint32_t>(d3d12Desc.SampleDesc.Count);
        desc.Filter = resDesc.Filter;
        desc.Wrap = resDesc.Wrap;
        desc.MipmapBias = resDesc.MipmapBias;

        switch (d3d12Desc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resDesc.IsCube)
            {
                desc.DepthOrArraySize /= 6u;
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::CubeArray : GfxTextureDimension::Cube;
            }
            else
            {
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::Tex2DArray : GfxTextureDimension::Tex2D;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.Dimension = GfxTextureDimension::Tex3D;
            break;
        default:
            throw GfxException("Invalid resource dimension");
        }

        Reset(desc);
        m_Resource = resource;
        m_State = resDesc.State;
    }
}

#include "pch.h"
#include "Engine/Graphics/GfxTexture.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxSettings.h"

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
}

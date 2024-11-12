#pragma once

#include "GfxSettings.h"
#include <directx/d3d12.h>

namespace march
{
    struct GfxUtils final
    {
        static constexpr float NearClipPlaneDepth = GfxSettings::UseReversedZBuffer ? 1.0f : 0.0f;
        static constexpr float FarClipPlaneDepth = GfxSettings::UseReversedZBuffer ? 0.0f : 1.0f;

        static float SRGBToLinearSpace(float x);
        static float LinearToSRGBSpace(float x);

        static void ReportLiveObjects();

        static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(D3D12_PRIMITIVE_TOPOLOGY topology)
        {
            switch (topology)
            {
            case D3D_PRIMITIVE_TOPOLOGY_UNDEFINED:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

            case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

            case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
            case D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
            case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
            case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            default:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
            }
        }

        template <typename T>
        static T GetShaderColor(const T& color, bool sRGB = true)
        {
            if (GfxSettings::ColorSpace == GfxColorSpace::Linear && sRGB)
            {
                float r = SRGBToLinearSpace(color.x);
                float g = SRGBToLinearSpace(color.y);
                float b = SRGBToLinearSpace(color.z);
                return T(r, g, b, color.w);
            }

            return color;
        }

        static constexpr DXGI_FORMAT GetShaderColorTextureFormat(DXGI_FORMAT format, bool sRGB = true)
        {
            if (GfxSettings::ColorSpace == GfxColorSpace::Linear && sRGB)
            {
                switch (format)
                {
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                case DXGI_FORMAT_BC1_UNORM:
                    return DXGI_FORMAT_BC1_UNORM_SRGB;
                case DXGI_FORMAT_BC2_UNORM:
                    return DXGI_FORMAT_BC2_UNORM_SRGB;
                case DXGI_FORMAT_BC3_UNORM:
                    return DXGI_FORMAT_BC3_UNORM_SRGB;
                case DXGI_FORMAT_B8G8R8A8_UNORM:
                    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
                case DXGI_FORMAT_B8G8R8X8_UNORM:
                    return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
                case DXGI_FORMAT_BC7_UNORM:
                    return DXGI_FORMAT_BC7_UNORM_SRGB;
                }
            }

            return format;
        }
    };
}

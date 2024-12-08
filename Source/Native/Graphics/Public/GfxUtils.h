#pragma once

#include "GfxSettings.h"
#include <directx/d3d12.h>
#include <string>

namespace march
{
    struct GfxUtils final
    {
        static constexpr float NearClipPlaneDepth = GfxSettings::UseReversedZBuffer ? 1.0f : 0.0f;
        static constexpr float FarClipPlaneDepth = GfxSettings::UseReversedZBuffer ? 0.0f : 1.0f;

        static float SRGBToLinearSpace(float x);
        static float LinearToSRGBSpace(float x);

        static void ReportLiveObjects();
        static void SetName(ID3D12Object* obj, const std::string& name);

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
    };
}

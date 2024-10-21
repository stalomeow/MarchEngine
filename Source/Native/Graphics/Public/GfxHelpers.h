#pragma once

#include "GfxSettings.h"
#include <directx/d3d12.h>
#include <stdint.h>

namespace march
{
    class GfxDevice;

    struct GfxHelpers final
    {
        static uint32_t GetMSAAQuality(GfxDevice* device, DXGI_FORMAT format, uint32_t sampleCount);

        static float SRGBToLinearSpace(float x);
        static float LinearToSRGBSpace(float x);

        static constexpr float GetNearClipPlaneDepth() { return GfxSettings::UseReversedZBuffer() ? 1.0f : 0.0f; }
        static constexpr float GetFarClipPlaneDepth()  { return GfxSettings::UseReversedZBuffer() ? 0.0f : 1.0f; }

        template <typename T>
        static T GetShaderColor(const T& color, bool sRGB = true)
        {
            if (GfxSettings::GetColorSpace() == GfxColorSpace::Linear && sRGB)
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
            if (GfxSettings::GetColorSpace() == GfxColorSpace::Linear && sRGB)
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

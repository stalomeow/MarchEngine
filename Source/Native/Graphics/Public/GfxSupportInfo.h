#pragma once

#include <directx/d3d12.h>
#include <stdint.h>
#include <cmath>

namespace march
{
    class GfxDevice;

    enum class GfxColorSpace
    {
        Linear,
        Gamma,
    };

    class GfxSupportInfo final
    {
    public:
        // -----------------------------------------------
        // Depth
        // -----------------------------------------------

        static constexpr bool UseReversedZBuffer()
        {
            return true;
        }

        static constexpr float GetNearClipPlaneDepth()
        {
            return UseReversedZBuffer() ? 1.0f : 0.0f;
        }

        static constexpr float GetFarClipPlaneDepth()
        {
            return UseReversedZBuffer() ? 0.0f : 1.0f;
        }

        // -----------------------------------------------
        // Color Space
        // -----------------------------------------------

        static constexpr GfxColorSpace GetColorSpace()
        {
            return GfxColorSpace::Linear;
        }

        static float SRGBToLinearSpace(float x)
        {
            // Approximately pow(x, 2.2)
            return (x < 0.04045f) ? (x / 12.92f) : std::powf((x + 0.055f) / 1.055f, 2.4f);
        }

        static float LinearToSRGBSpace(float x)
        {
            // Approximately pow(x, 1.0 / 2.2)
            return (x < 0.0031308f) ? (12.92f * x) : (1.055f * std::powf(x, 1.0f / 2.4f) - 0.055f);
        }

        template <typename T>
        static T ToShaderColor(const T& color, bool sRGB = true)
        {
            if (GetColorSpace() == GfxColorSpace::Linear && sRGB)
            {
                float r = SRGBToLinearSpace(color.x);
                float g = SRGBToLinearSpace(color.y);
                float b = SRGBToLinearSpace(color.z);
                return T(r, g, b, color.w);
            }

            return color;
        }

        static constexpr DXGI_FORMAT ToShaderFormat(DXGI_FORMAT format, bool sRGB = true)
        {
            if (GetColorSpace() == GfxColorSpace::Linear && sRGB)
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

        // -----------------------------------------------
        // MSAA
        // -----------------------------------------------

        static uint32_t GetMSAAQuality(GfxDevice* device, DXGI_FORMAT format, uint32_t sampleCount);
    };
}

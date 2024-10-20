#include "GfxHelpers.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include <cmath>

namespace march
{
    uint32_t GfxHelpers::GetMSAAQuality(GfxDevice* device, DXGI_FORMAT format, uint32_t sampleCount)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = {};
        levels.Format = format;
        levels.SampleCount = static_cast<UINT>(sampleCount);
        levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

        ID3D12Device4* d3d12Device = device->GetD3D12Device();
        GFX_HR(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels)));
        return static_cast<uint32_t>(levels.NumQualityLevels - 1);
    }

    float GfxHelpers::SRGBToLinearSpace(float x)
    {
        // Approximately pow(x, 2.2)
        return (x < 0.04045f) ? (x / 12.92f) : std::powf((x + 0.055f) / 1.055f, 2.4f);
    }

    float GfxHelpers::LinearToSRGBSpace(float x)
    {
        // Approximately pow(x, 1.0 / 2.2)
        return (x < 0.0031308f) ? (12.92f * x) : (1.055f * std::powf(x, 1.0f / 2.4f) - 0.055f);
    }
}

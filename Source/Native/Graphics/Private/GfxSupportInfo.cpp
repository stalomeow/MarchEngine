#include "GfxSupportInfo.h"
#include "GfxDevice.h"
#include "GfxExcept.h"

namespace march
{
    uint32_t GfxSupportInfo::GetMSAAQuality(GfxDevice* device, DXGI_FORMAT format, uint32_t sampleCount)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = {};
        levels.Format = format;
        levels.SampleCount = static_cast<UINT>(sampleCount);
        levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

        ID3D12Device4* d3d12Device = device->GetD3D12Device();
        GFX_HR(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels)));
        return static_cast<uint32_t>(levels.NumQualityLevels - 1);
    }
}

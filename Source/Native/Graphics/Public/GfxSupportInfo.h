#pragma once

#include <directx/d3d12.h>
#include <stdint.h>

namespace march
{
    class GfxDevice;

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
        // MSAA
        // -----------------------------------------------

        static uint32_t GetMSAAQuality(GfxDevice* device, DXGI_FORMAT format, uint32_t sampleCount);
    };
}

#pragma once

#include <stdint.h>

namespace march
{
    enum class GfxColorSpace
    {
        Linear,
        Gamma,
    };

    struct GfxSettings final
    {
        static constexpr bool UseReversedZBuffer = true;
        static constexpr GfxColorSpace ColorSpace = GfxColorSpace::Linear;

        static constexpr uint32_t BackBufferCount = 2;      // The number of buffers in the swap chain
        static constexpr uint32_t MaxFrameLatency = 3;      // The number of frames that the swap chain is allowed to queue for rendering
        static constexpr uint32_t VerticalSyncInterval = 0; // 垂直同步的间隔，0 表示关闭
    };
}

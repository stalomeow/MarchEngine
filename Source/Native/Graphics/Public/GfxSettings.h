#pragma once

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
    };
}

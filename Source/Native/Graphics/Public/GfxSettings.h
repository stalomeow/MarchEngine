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
        static constexpr bool UseReversedZBuffer() { return true; }
        static constexpr GfxColorSpace GetColorSpace() { return GfxColorSpace::Linear; }
    };
}

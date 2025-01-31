#pragma once

#include "Engine/Ints.h"

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

        static inline int32 ShadowDepthBias = 10000;
        static inline float ShadowSlopeScaledDepthBias = 1.0f;
        static inline float ShadowDepthBiasClamp = 0.0f;
    };
}

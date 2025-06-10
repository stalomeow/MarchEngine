#pragma once

#include "Engine/Ints.h"
#include <DirectXMath.h>

namespace march
{
    struct SequenceUtils
    {
        static inline float RadicalInverse(uint32 index, uint32 radix)
        {
            float result = 0.0f;
            float weight = 1.0f / radix;

            while (index > 0)
            {
                result += (index % radix) * weight;
                weight /= radix;
                index /= radix;
            }

            return result;
        }

        static inline DirectX::XMFLOAT2 Halton(uint32 index)
        {
            return { RadicalInverse(index, 2), RadicalInverse(index, 3) };
        }
    };
}

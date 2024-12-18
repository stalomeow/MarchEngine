#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <stdint.h>

namespace march::MathUtils
{
    const DirectX::XMFLOAT4X4& Identity4x4();

    uint32_t AlignUp(uint32_t size, uint32_t alignment);

    // https://github.com/microsoft/DirectX-Graphics-Samples

    template <typename T>
    __forceinline bool IsPowerOfTwo(T value)
    {
        return 0 == (value & (value - 1));
    }

    template <typename T>
    __forceinline bool IsDivisible(T value, T divisor)
    {
        return (value / divisor) * divisor == value;
    }

    __forceinline uint8_t Log2(uint64_t value)
    {
        unsigned long mssb; // most significant set bit
        unsigned long lssb; // least significant set bit

        // If perfect power of two (only one set bit), return index of bit.  Otherwise round up
        // fractional log by adding 1 to most signicant set bit's index.
        if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
            return uint8_t(mssb + (mssb == lssb ? 0 : 1));
        else
            return 0;
    }

    template <typename T>
    __forceinline T AlignPowerOfTwo(T value)
    {
        return value == 0 ? 0 : 1 << Log2(value);
    }
}

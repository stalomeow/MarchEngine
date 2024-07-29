#pragma once

#include <DirectXMath.h>
#include <assert.h>
#include <Windows.h>

namespace dx12demo::MathHelper
{
    inline DirectX::XMFLOAT4X4 Identity4x4()
    {
        static DirectX::XMFLOAT4X4 I =
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        return I;
    }

    inline UINT AlignUp(UINT size, UINT alignment)
    {
        UINT mask = alignment - 1;
        assert((alignment & mask) == 0); // ensure power of 2
        return (size + mask) & ~mask;
    }
}

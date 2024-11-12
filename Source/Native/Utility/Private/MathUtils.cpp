#include "MathUtils.h"
#include <assert.h>

namespace march::MathUtils
{
    DirectX::XMFLOAT4X4 Identity4x4()
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

    uint32_t AlignUp(uint32_t size, uint32_t alignment)
    {
        uint32_t mask = alignment - 1;
        assert((alignment & mask) == 0); // ensure power of 2
        return (size + mask) & ~mask;
    }
}

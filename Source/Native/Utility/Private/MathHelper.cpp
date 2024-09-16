#include "MathHelper.h"
#include <assert.h>

namespace march::MathHelper
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

    UINT AlignUp(UINT size, UINT alignment)
    {
        UINT mask = alignment - 1;
        assert((alignment & mask) == 0); // ensure power of 2
        return (size + mask) & ~mask;
    }
}

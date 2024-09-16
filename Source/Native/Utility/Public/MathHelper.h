#pragma once

#include <DirectXMath.h>
#include <Windows.h>

namespace march::MathHelper
{
    DirectX::XMFLOAT4X4 Identity4x4();

    UINT AlignUp(UINT size, UINT alignment);
}

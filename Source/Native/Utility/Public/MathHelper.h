#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <stdint.h>

namespace march::MathHelper
{
    DirectX::XMFLOAT4X4 Identity4x4();

    uint32_t AlignUp(uint32_t size, uint32_t alignment);
}

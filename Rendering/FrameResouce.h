#pragma once

#include "Rendering/Resource/GpuBuffer.h"
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>

namespace dx12demo
{
    struct PerObjConstants
    {
        DirectX::XMFLOAT4X4 WorldMatrix;
    };

    struct PerDrawConstants
    {
        DirectX::XMFLOAT4X4 ViewMatrix;
        DirectX::XMFLOAT4X4 ProjectionMatrix;
        DirectX::XMFLOAT4X4 ViewProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewMatrix;
        DirectX::XMFLOAT4X4 InvProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewProjectionMatrix;
        DirectX::XMFLOAT4 Time; // elapsed time, delta time, unused, unused
    };
}

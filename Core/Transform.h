#pragma once

#include <DirectXMath.h>

namespace dx12demo
{
    class Transform
    {
    public:
        DirectX::XMFLOAT4X4 GetWorldMatrix() const
        {
            DirectX::XMVECTOR translation = DirectX::XMLoadFloat3(&Position);
            DirectX::XMVECTOR rotation = DirectX::XMLoadFloat4(&Rotation);
            DirectX::XMVECTOR scaling = DirectX::XMLoadFloat3(&Scale);
            DirectX::XMMATRIX world = DirectX::XMMatrixAffineTransformation(
                scaling, DirectX::XMVectorZero(), rotation, translation);

            DirectX::XMFLOAT4X4 ret;
            DirectX::XMStoreFloat4x4(&ret, world);
            return ret;
        }

    public:
        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 RotationEuler = { 0.0f, 0.0f, 0.0f };
    };
}

#include "Rendering/RenderObject.h"

namespace dx12demo
{
    RenderObject::RenderObject()
    {

    }

    DirectX::XMFLOAT4X4 RenderObject::GetWorldMatrix() const
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
}

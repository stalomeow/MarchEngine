#include "Rendering/RenderObject.h"

namespace dx12demo
{
    RenderObject::RenderObject()
    {
        m_MaterialBuffer = std::make_unique<ConstantBuffer<MaterialData>>(L"RenderObject Material", 1, true);
        m_MaterialBuffer->SetData(0, MaterialData());
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

    MaterialData& RenderObject::GetMaterialData() const
    {
        return *(m_MaterialBuffer->GetPointer(0));
    }
}

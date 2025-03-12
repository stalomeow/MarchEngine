#include "pch.h"
#include "Engine/Rendering/D3D12Impl/MeshRenderer.h"
#include "Engine/Rendering/D3D12Impl/GfxMesh.h"
#include "Engine/Misc/MathUtils.h"
#include "Engine/Transform.h"

using namespace DirectX;

namespace march
{
    MeshRenderer::MeshRenderer()
        : Mesh(nullptr)
        , Materials{}
        , m_PrevLocalToWorldMatrix(MathUtils::Identity4x4())
    {
    }

    BoundingBox MeshRenderer::GetBounds() const
    {
        BoundingBox result = {};

        if (Mesh != nullptr)
        {
            Mesh->GetBounds().Transform(result, GetTransform()->LoadLocalToWorldMatrix());
        }

        return result;
    }

    XMFLOAT4X4 MeshRenderer::GetPrevLocalToWorldMatrix() const
    {
        return m_PrevLocalToWorldMatrix;
    }

    XMMATRIX MeshRenderer::LoadPrevLocalToWorldMatrix() const
    {
        return XMLoadFloat4x4(&m_PrevLocalToWorldMatrix);
    }

    void MeshRenderer::PrepareFrameData()
    {
        // 记录上一帧的 LocalToWorldMatrix
        m_PrevLocalToWorldMatrix = GetTransform()->GetLocalToWorldMatrix();
    }
}

#pragma once

#include "Engine/Component.h"
#include <vector>
#include <DirectXCollision.h>
#include <DirectXMath.h>

namespace march
{
    class GfxMesh;
    class Material;

    class MeshRenderer : public Component
    {
    public:
        GfxMesh* Mesh;
        std::vector<Material*> Materials;

        MeshRenderer();

        // 获取世界空间 Bounds
        DirectX::BoundingBox GetBounds() const;

        DirectX::XMFLOAT4X4 GetPrevLocalToWorldMatrix() const;
        DirectX::XMMATRIX LoadPrevLocalToWorldMatrix() const;

        void PrepareFrameData();

    private:
        DirectX::XMFLOAT4X4 m_PrevLocalToWorldMatrix;
    };
}

#pragma once

#include "Engine/Component.h"
#include <vector>
#include <DirectXCollision.h>

namespace march
{
    class GfxMesh;
    class Material;

    class MeshRenderer : public Component
    {
    public:
        MeshRenderer() = default;

        GfxMesh* Mesh = nullptr;
        std::vector<Material*> Materials{};

        // 获取世界空间 Bounds
        DirectX::BoundingBox GetBounds() const;
    };
}

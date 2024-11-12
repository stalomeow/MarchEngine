#pragma once

#include "Component.h"
#include <vector>
#include <DirectXCollision.h>

namespace march
{
    class GfxMesh;
    class Material;

    class RenderObject : public Component
    {
    public:
        RenderObject() = default;

        GfxMesh* Mesh = nullptr;
        std::vector<Material*> Materials{};

        // 获取世界空间 Bounds
        DirectX::BoundingBox GetBounds() const;

    protected:
        void OnMount() override;
        void OnUnmount() override;
    };
}

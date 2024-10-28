#pragma once

#include "Component.h"
#include <vector>

namespace march
{
    class GfxMesh;
    class Material;

    class RenderObject : public Component
    {
    public:
        RenderObject() = default;
        ~RenderObject() = default;

        GfxMesh* Mesh = nullptr;
        std::vector<Material*> Materials{};

    protected:
        void OnMount() override;
        void OnUnmount() override;
    };
}

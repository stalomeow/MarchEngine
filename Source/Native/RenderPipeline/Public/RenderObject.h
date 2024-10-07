#pragma once

#include "Component.h"

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
        Material* Mat = nullptr;

    protected:
        void OnMount() override;
        void OnUnmount() override;
    };
}

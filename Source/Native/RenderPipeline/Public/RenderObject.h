#pragma once

#include "Component.h"
#include "PipelineState.h"

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
        MeshRendererDesc Desc = {};

    protected:
        void OnMount() override;
        void OnUnmount() override;
    };
}

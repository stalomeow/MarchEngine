#include "pch.h"
#include "Engine/Graphics/MeshRenderer.h"
#include "Engine/Graphics/GfxMesh.h"
#include "Engine/Transform.h"

using namespace DirectX;

namespace march
{
    BoundingBox MeshRenderer::GetBounds() const
    {
        BoundingBox result = {};

        if (Mesh != nullptr)
        {
            Mesh->GetBounds().Transform(result, GetTransform()->LoadLocalToWorldMatrix());
        }

        return result;
    }
}

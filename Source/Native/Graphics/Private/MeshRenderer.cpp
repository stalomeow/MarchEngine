#include "MeshRenderer.h"
#include "Transform.h"
#include "GfxMesh.h"

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

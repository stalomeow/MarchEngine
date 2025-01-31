#include "pch.h"
#include "Engine/Rendering/D3D12Impl/MeshRenderer.h"
#include "Engine/Rendering/D3D12Impl/GfxMesh.h"
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

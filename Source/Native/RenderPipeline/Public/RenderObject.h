#pragma once

#include "Material.h"
#include "PipelineState.h"
#include <DirectXMath.h>
#include <memory>

namespace march
{
    class GfxMesh;

    class RenderObject
    {
    public:
        RenderObject();
        ~RenderObject() = default;

        DirectX::XMFLOAT4X4 GetWorldMatrix() const;

        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
        GfxMesh* Mesh = nullptr;
        Material* Mat = nullptr;
        MeshRendererDesc Desc = {};

        bool IsActive = false;
    };
}

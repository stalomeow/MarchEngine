#pragma once

#include "Rendering/Mesh.hpp"
#include "Rendering/Resource/GpuBuffer.h"
#include "Rendering/Material.h"
#include "Rendering/PipelineState.h"
#include "Scripting/ScriptTypes.h"
#include <DirectXMath.h>
#include <memory>

namespace dx12demo
{
    class RenderObject
    {
    public:
        RenderObject();
        ~RenderObject() = default;

        DirectX::XMFLOAT4X4 GetWorldMatrix() const;

        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
        Mesh* Mesh = nullptr;
        Material* Mat = nullptr;
        MeshRendererDesc Desc = {};

        bool IsActive = false;
    };

    namespace binding
    {
        inline CSHARP_API(RenderObject*) RenderObject_New()
        {
            return new RenderObject();
        }

        inline CSHARP_API(void) RenderObject_Delete(RenderObject* pObject)
        {
            delete pObject;
        }

        inline CSHARP_API(void) RenderObject_SetPosition(RenderObject* pObject, CSharpVector3 value)
        {
            pObject->Position = ToXMFLOAT3(value);
        }

        inline CSHARP_API(void) RenderObject_SetRotation(RenderObject* pObject, CSharpQuaternion value)
        {
            pObject->Rotation = ToXMFLOAT4(value);
        }

        inline CSHARP_API(void) RenderObject_SetScale(RenderObject* pObject, CSharpVector3 value)
        {
            pObject->Scale = ToXMFLOAT3(value);
        }

        inline CSHARP_API(Mesh*) RenderObject_GetMesh(RenderObject* pObject)
        {
            return pObject->Mesh;
        }

        inline CSHARP_API(void) RenderObject_SetMesh(RenderObject* pObject, Mesh* pMesh)
        {
            pObject->Mesh = pMesh;
            pObject->Desc.InputLayout = pMesh->VertexInputLayout();
            pObject->Desc.PrimitiveTopologyType = pMesh->TopologyType();
        }

        inline CSHARP_API(CSharpBool) RenderObject_GetIsActive(RenderObject* pObject)
        {
            return CSHARP_MARSHAL_BOOL(pObject->IsActive);
        }

        inline CSHARP_API(void) RenderObject_SetIsActive(RenderObject* pObject, CSharpBool value)
        {
            pObject->IsActive = CSHARP_UNMARSHAL_BOOL(value);
        }

        inline CSHARP_API(void) RenderObject_SetMaterial(RenderObject* pObject, Material* pMaterial)
        {
            pObject->Mat = pMaterial;
        }
    }
}

#pragma once

#include "Rendering/Mesh.hpp"
#include "Rendering/Resource/GpuBuffer.h"
#include "Scripting/ScriptTypes.h"
#include <DirectXMath.h>
#include <memory>

namespace dx12demo
{
    struct MaterialData
    {
        DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
        float Roughness = 0.25f;
    };

    class RenderObject
    {
    public:
        RenderObject();
        ~RenderObject() = default;

        DirectX::XMFLOAT4X4 GetWorldMatrix() const;
        MaterialData& GetMaterialData() const;
        ConstantBuffer<MaterialData>* GetMaterialBuffer() const { return m_MaterialBuffer.get(); }

        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
        Mesh* Mesh = nullptr;
        bool IsActive = false;

    private:
        std::unique_ptr<ConstantBuffer<MaterialData>> m_MaterialBuffer;
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
        }

        inline CSHARP_API(CSharpBool) RenderObject_GetIsActive(RenderObject* pObject)
        {
            return CSHARP_MARSHAL_BOOL(pObject->IsActive);
        }

        inline CSHARP_API(void) RenderObject_SetIsActive(RenderObject* pObject, CSharpBool value)
        {
            pObject->IsActive = CSHARP_UNMARSHAL_BOOL(value);
        }
    }
}

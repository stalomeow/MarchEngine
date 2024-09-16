#include "RenderObject.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(RenderObject*) RenderObject_New()
{
    return new RenderObject();
}

NATIVE_EXPORT(void) RenderObject_Delete(RenderObject* pObject)
{
    delete pObject;
}

NATIVE_EXPORT(void) RenderObject_SetPosition(RenderObject* pObject, CSharpVector3 value)
{
    pObject->Position = ToXMFLOAT3(value);
}

NATIVE_EXPORT(void) RenderObject_SetRotation(RenderObject* pObject, CSharpQuaternion value)
{
    pObject->Rotation = ToXMFLOAT4(value);
}

NATIVE_EXPORT(void) RenderObject_SetScale(RenderObject* pObject, CSharpVector3 value)
{
    pObject->Scale = ToXMFLOAT3(value);
}

NATIVE_EXPORT(Mesh*) RenderObject_GetMesh(RenderObject* pObject)
{
    return pObject->Mesh;
}

NATIVE_EXPORT(void) RenderObject_SetMesh(RenderObject* pObject, Mesh* pMesh)
{
    pObject->Mesh = pMesh;
    pObject->Desc.InputLayout = pMesh->VertexInputLayout();
    pObject->Desc.PrimitiveTopologyType = pMesh->TopologyType();
}

NATIVE_EXPORT(CSharpBool) RenderObject_GetIsActive(RenderObject* pObject)
{
    return CSHARP_MARSHAL_BOOL(pObject->IsActive);
}

NATIVE_EXPORT(void) RenderObject_SetIsActive(RenderObject* pObject, CSharpBool value)
{
    pObject->IsActive = CSHARP_UNMARSHAL_BOOL(value);
}

NATIVE_EXPORT(void) RenderObject_SetMaterial(RenderObject* pObject, Material* pMaterial)
{
    pObject->Mat = pMaterial;
}

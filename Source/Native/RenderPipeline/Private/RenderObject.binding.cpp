#include "RenderObject.h"
#include "InteropServices.h"
#include "GfxMesh.h"

using namespace march;

NATIVE_EXPORT(RenderObject*) RenderObject_New()
{
    return new RenderObject();
}

NATIVE_EXPORT(void) RenderObject_Delete(RenderObject* pObject)
{
    delete pObject;
}

NATIVE_EXPORT(GfxMesh*) RenderObject_GetMesh(RenderObject* pObject)
{
    return pObject->Mesh;
}

NATIVE_EXPORT(void) RenderObject_SetMesh(RenderObject* pObject, GfxMesh* pMesh)
{
    pObject->Mesh = pMesh;
    pObject->Desc.InputLayout = pMesh->GetVertexInputLayout();
    pObject->Desc.PrimitiveTopologyType = pMesh->GetTopologyType();
}

NATIVE_EXPORT(void) RenderObject_SetMaterial(RenderObject* pObject, Material* pMaterial)
{
    pObject->Mat = pMaterial;
}

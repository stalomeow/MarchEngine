#include "RenderObject.h"
#include "InteropServices.h"
#include "GfxMesh.h"

NATIVE_EXPORT_AUTO RenderObject_New()
{
    retcs new RenderObject();
}

NATIVE_EXPORT_AUTO RenderObject_Delete(RenderObject* pObject)
{
    delete pObject;
}

NATIVE_EXPORT_AUTO RenderObject_GetMesh(RenderObject* pObject)
{
    retcs pObject->Mesh;
}

NATIVE_EXPORT_AUTO RenderObject_SetMesh(RenderObject* pObject, GfxMesh* pMesh)
{
    pObject->Mesh = pMesh;
    pObject->Desc.InputLayout = pMesh->GetVertexInputLayout();
    pObject->Desc.PrimitiveTopologyType = pMesh->GetTopologyType();
}

NATIVE_EXPORT_AUTO RenderObject_SetMaterial(RenderObject* pObject, Material* pMaterial)
{
    pObject->Mat = pMaterial;
}

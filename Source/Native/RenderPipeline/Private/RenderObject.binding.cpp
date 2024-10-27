#include "RenderObject.h"
#include "InteropServices.h"
#include "GfxMesh.h"

NATIVE_EXPORT_AUTO RenderObject_New()
{
    retcs DBG_NEW RenderObject();
}

NATIVE_EXPORT_AUTO RenderObject_Delete(RenderObject* pObject)
{
    delete pObject;
}

NATIVE_EXPORT_AUTO RenderObject_SetMesh(RenderObject* pObject, GfxMesh* pMesh)
{
    pObject->Mesh = pMesh;
}

NATIVE_EXPORT_AUTO RenderObject_SetMaterial(RenderObject* pObject, Material* pMaterial)
{
    pObject->Mat = pMaterial;
}

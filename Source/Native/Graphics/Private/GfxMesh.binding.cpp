#include "GfxMesh.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO GfxMesh_New()
{
    retcs DBG_NEW GfxMesh();
}

NATIVE_EXPORT_AUTO GfxMesh_Delete(cs<GfxMesh*> pObject)
{
    delete pObject;
}

NATIVE_EXPORT_AUTO GfxMesh_ClearSubMeshes(cs<GfxMesh*> pObject)
{
    pObject->ClearSubMeshes();
}

NATIVE_EXPORT_AUTO GfxMesh_AddSubMeshCube(cs<GfxMesh*> pObject)
{
    pObject->AddSubMeshCube(1, 1, 1);
}

NATIVE_EXPORT_AUTO GfxMesh_AddSubMeshSphere(cs<GfxMesh*> pObject, cs_float radius, cs_uint sliceCount, cs_uint stackCount)
{
    pObject->AddSubMeshSphere(radius, sliceCount, stackCount);
}

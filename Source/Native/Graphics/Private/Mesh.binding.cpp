#include "GfxMesh.h"
#include "GfxDevice.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO SimpleMesh_New()
{
    retcs CreateSimpleGfxMesh(GetGfxDevice());
}

NATIVE_EXPORT_AUTO SimpleMesh_Delete(cs<GfxMesh*> pObject)
{
    delete pObject;
}

NATIVE_EXPORT_AUTO SimpleMesh_ClearSubMeshes(cs<GfxMesh*> pObject)
{
    pObject->ClearSubMeshes();
}

NATIVE_EXPORT_AUTO SimpleMesh_AddSubMeshCube(cs<GfxMesh*> pObject)
{
    pObject->AddSubMeshCube(1, 1, 1);
}

NATIVE_EXPORT_AUTO SimpleMesh_AddSubMeshSphere(cs<GfxMesh*> pObject, cs_float radius, cs_uint sliceCount, cs_uint stackCount)
{
    pObject->AddSubMeshSphere(radius, sliceCount, stackCount);
}

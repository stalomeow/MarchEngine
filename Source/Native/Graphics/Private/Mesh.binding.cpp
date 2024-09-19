#include "GfxMesh.h"
#include "GfxDevice.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(GfxMesh*) SimpleMesh_New()
{
    return CreateSimpleGfxMesh(GetGfxDevice());
}

NATIVE_EXPORT(void) SimpleMesh_Delete(GfxMesh* pObject)
{
    delete pObject;
}

NATIVE_EXPORT(void) SimpleMesh_ClearSubMeshes(GfxMesh* pObject)
{
    pObject->ClearSubMeshes();
}

NATIVE_EXPORT(void) SimpleMesh_AddSubMeshCube(GfxMesh* pObject)
{
    pObject->AddSubMeshCube(1, 1, 1);
}

NATIVE_EXPORT(void) SimpleMesh_AddSubMeshSphere(GfxMesh* pObject, CSharpFloat radius, CSharpUInt sliceCount, CSharpUInt stackCount)
{
    pObject->AddSubMeshSphere(radius, sliceCount, stackCount);
}

#include "Mesh.hpp"
#include "ScriptTypes.h"

using namespace march;

NATIVE_EXPORT(SimpleMesh*) SimpleMesh_New()
{
    return new SimpleMesh();
}

NATIVE_EXPORT(void) SimpleMesh_Delete(SimpleMesh* pObject)
{
    delete pObject;
}

NATIVE_EXPORT(void) SimpleMesh_ClearSubMeshes(SimpleMesh* pObject)
{
    pObject->ClearSubMeshes();
}

NATIVE_EXPORT(void) SimpleMesh_AddSubMeshCube(SimpleMesh* pObject)
{
    pObject->AddSubMeshCube();
}

NATIVE_EXPORT(void) SimpleMesh_AddSubMeshSphere(SimpleMesh* pObject, CSharpFloat radius, CSharpUInt sliceCount, CSharpUInt stackCount)
{
    pObject->AddSubMeshSphere(radius, sliceCount, stackCount);
}

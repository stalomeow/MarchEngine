#include "RenderObject.h"
#include "InteropServices.h"
#include "GfxMesh.h"

NATIVE_EXPORT_AUTO RenderObject_New()
{
    retcs DBG_NEW RenderObject();
}

NATIVE_EXPORT_AUTO RenderObject_Delete(cs<RenderObject*> pObject)
{
    delete pObject;
}

NATIVE_EXPORT_AUTO RenderObject_SetMesh(cs<RenderObject*> pObject, cs<GfxMesh*> pMesh)
{
    pObject->Mesh = pMesh;
}

NATIVE_EXPORT_AUTO RenderObject_SetMaterials(cs<RenderObject*> pObject, cs<cs<Material*>[]> materials)
{
    pObject->Materials.clear();

    for (int32_t i = 0; i < materials.size(); i++)
    {
        pObject->Materials.push_back(materials[i]);
    }
}

NATIVE_EXPORT_AUTO RenderObject_GetBounds(cs<RenderObject*> pObject)
{
    retcs pObject->GetBounds();
}

#include "pch.h"
#include "Engine/Graphics/MeshRenderer.h"
#include "Engine/Graphics/GfxMesh.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO MeshRenderer_New()
{
    retcs MARCH_NEW MeshRenderer();
}

NATIVE_EXPORT_AUTO MeshRenderer_SetMesh(cs<MeshRenderer*> self, cs<GfxMesh*> pMesh)
{
    self->Mesh = pMesh;
}

NATIVE_EXPORT_AUTO MeshRenderer_SetMaterials(cs<MeshRenderer*> self, cs<cs<Material*>[]> materials)
{
    self->Materials.clear();

    for (int32_t i = 0; i < materials.size(); i++)
    {
        self->Materials.push_back(materials[i]);
    }
}

NATIVE_EXPORT_AUTO MeshRenderer_GetBounds(cs<MeshRenderer*> self)
{
    retcs self->GetBounds();
}

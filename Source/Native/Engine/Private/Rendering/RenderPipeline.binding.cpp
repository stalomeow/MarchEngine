#include "pch.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Scripting/InteropServices.h"
#include "Engine/Application.h"

NATIVE_EXPORT_AUTO RenderPipeline_AddMeshRenderer(cs<MeshRenderer*> renderer)
{
    GetApp()->GetRenderPipeline()->AddMeshRenderer(renderer);
}

NATIVE_EXPORT_AUTO RenderPipeline_RemoveMeshRenderer(cs<MeshRenderer*> renderer)
{
    GetApp()->GetRenderPipeline()->RemoveMeshRenderer(renderer);
}

NATIVE_EXPORT_AUTO RenderPipeline_AddLight(cs<Light*> pLight)
{
    GetApp()->GetRenderPipeline()->AddLight(pLight);
}

NATIVE_EXPORT_AUTO RenderPipeline_RemoveLight(cs<Light*> pLight)
{
    GetApp()->GetRenderPipeline()->RemoveLight(pLight);
}

NATIVE_EXPORT_AUTO RenderPipeline_SetSkyboxMaterial(cs<Material*> material)
{
    GetApp()->GetRenderPipeline()->SetSkyboxMaterial(material);
}

NATIVE_EXPORT_AUTO RenderPipeline_BakeEnvLight(cs<GfxTexture*> radianceMap, cs_float diffuseIntensityMultiplier, cs_float specularIntensityMultiplier)
{
    GetApp()->GetRenderPipeline()->BakeEnvLight(radianceMap, diffuseIntensityMultiplier, specularIntensityMultiplier);
}

NATIVE_EXPORT_AUTO RenderPipeline_Initialize()
{
    ApplicationManagedOnlyAPI::InitRenderPipeline(GetApp());
}

NATIVE_EXPORT_AUTO RenderPipeline_Release()
{
    ApplicationManagedOnlyAPI::ReleaseRenderPipeline(GetApp());
}

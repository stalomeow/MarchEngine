#include "pch.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Graphics/Material.h"
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

NATIVE_EXPORT_AUTO RenderPipeline_Render(cs<Camera*> camera, cs<Material*> gridGizmoMaterial)
{
    GetApp()->GetRenderPipeline()->Render(camera, gridGizmoMaterial);
}

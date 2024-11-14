#include "RenderPipeline.h"
#include "Material.h"
#include "Application.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO RenderPipeline_AddRenderObject(cs<RenderObject*> pObject)
{
    GetApp()->GetRenderPipeline()->AddRenderObject(pObject);
}

NATIVE_EXPORT_AUTO RenderPipeline_RemoveRenderObject(cs<RenderObject*> pObject)
{
    GetApp()->GetRenderPipeline()->RemoveRenderObject(pObject);
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

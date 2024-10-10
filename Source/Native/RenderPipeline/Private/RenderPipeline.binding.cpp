#include "RenderPipeline.h"
#include "Material.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO RenderPipeline_AddRenderObject(cs<RenderPipeline*> pPipeline, cs<RenderObject*> pObject)
{
    pPipeline->AddRenderObject(pObject);
}

NATIVE_EXPORT_AUTO RenderPipeline_RemoveRenderObject(cs<RenderPipeline*> pPipeline, cs<RenderObject*> pObject)
{
    pPipeline->RemoveRenderObject(pObject);
}

NATIVE_EXPORT_AUTO RenderPipeline_AddLight(cs<RenderPipeline*> pPipeline, cs<Light*> pLight)
{
    pPipeline->AddLight(pLight);
}

NATIVE_EXPORT_AUTO RenderPipeline_RemoveLight(cs<RenderPipeline*> pPipeline, cs<Light*> pLight)
{
    pPipeline->RemoveLight(pLight);
}

NATIVE_EXPORT_AUTO RenderPipeline_Render(cs<RenderPipeline*> pPipeline, cs<Camera*> camera, cs<Material*> gridGizmoMaterial)
{
    pPipeline->Render(camera, gridGizmoMaterial);
}

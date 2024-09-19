#include "RenderPipeline.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(void) RenderPipeline_AddRenderObject(RenderPipeline* pPipeline, RenderObject* pObject)
{
    pPipeline->AddRenderObject(pObject);
}

NATIVE_EXPORT(void) RenderPipeline_RemoveRenderObject(RenderPipeline* pPipeline, RenderObject* pObject)
{
    pPipeline->RemoveRenderObject(pObject);
}

NATIVE_EXPORT(void) RenderPipeline_AddLight(RenderPipeline* pPipeline, Light* pLight)
{
    pPipeline->AddLight(pLight);
}

NATIVE_EXPORT(void) RenderPipeline_RemoveLight(RenderPipeline* pPipeline, Light* pLight)
{
    pPipeline->RemoveLight(pLight);
}

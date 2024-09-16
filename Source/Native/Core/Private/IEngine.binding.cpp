#include "IEngine.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(RenderPipeline*) IEngine_GetRenderPipeline(IEngine* pEngine)
{
    return pEngine->GetRenderPipeline();
}

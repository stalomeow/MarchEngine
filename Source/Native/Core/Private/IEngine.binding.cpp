#include "IEngine.h"
#include "ScriptTypes.h"

using namespace march;

NATIVE_EXPORT(RenderPipeline*) IEngine_GetRenderPipeline(IEngine* pEngine)
{
    return pEngine->GetRenderPipeline();
}

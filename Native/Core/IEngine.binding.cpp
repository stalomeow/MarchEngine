#include "Core/IEngine.h"
#include "Scripting/ScriptTypes.h"

using namespace dx12demo;

NATIVE_EXPORT(RenderPipeline*) IEngine_GetRenderPipeline(IEngine* pEngine)
{
    return pEngine->GetRenderPipeline();
}

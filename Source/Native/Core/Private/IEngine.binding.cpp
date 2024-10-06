#include "IEngine.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO IEngine_GetRenderPipeline(cs<IEngine*> pEngine)
{
    retcs pEngine->GetRenderPipeline();
}

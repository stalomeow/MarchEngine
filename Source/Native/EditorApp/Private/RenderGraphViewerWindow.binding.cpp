#include "pch.h"
#include "RenderGraphViewerWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO RenderGraphViewerWindow_New()
{
    retcs MARCH_NEW RenderGraphViewerWindow();
}

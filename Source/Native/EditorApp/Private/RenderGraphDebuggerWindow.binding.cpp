#include "pch.h"
#include "Editor/RenderGraphDebuggerWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO RenderGraphDebuggerWindow_New()
{
    retcs MARCH_NEW RenderGraphDebuggerWindow();
}

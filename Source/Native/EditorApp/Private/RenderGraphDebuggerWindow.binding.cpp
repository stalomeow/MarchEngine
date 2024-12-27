#include "pch.h"
#include "Editor/RenderGraphDebuggerWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO RenderGraphDebuggerWindow_New()
{
    retcs DBG_NEW RenderGraphDebuggerWindow();
}

NATIVE_EXPORT_AUTO RenderGraphDebuggerWindow_Delete(cs<RenderGraphDebuggerWindow*> w)
{
    delete w;
}

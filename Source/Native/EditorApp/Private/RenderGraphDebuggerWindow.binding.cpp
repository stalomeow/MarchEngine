#include "RenderGraphDebuggerWindow.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO RenderGraphDebuggerWindow_New()
{
    retcs DBG_NEW RenderGraphDebuggerWindow();
}

NATIVE_EXPORT_AUTO RenderGraphDebuggerWindow_Delete(cs<RenderGraphDebuggerWindow*> w)
{
    delete w;
}

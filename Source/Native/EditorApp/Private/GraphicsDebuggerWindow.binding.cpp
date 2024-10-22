#include "GraphicsDebuggerWindow.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO GraphicsDebuggerWindow_New()
{
    retcs DBG_NEW GraphicsDebuggerWindow();
}

NATIVE_EXPORT_AUTO GraphicsDebuggerWindow_Delete(cs<GraphicsDebuggerWindow*> w)
{
    delete w;
}

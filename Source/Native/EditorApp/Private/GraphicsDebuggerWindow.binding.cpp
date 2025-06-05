#include "pch.h"
#include "GraphicsDebuggerWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO GraphicsDebuggerWindow_New()
{
    retcs MARCH_NEW GraphicsDebuggerWindow();
}

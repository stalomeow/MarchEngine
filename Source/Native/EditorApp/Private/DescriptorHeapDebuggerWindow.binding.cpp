#include "pch.h"
#include "Editor/DescriptorHeapDebuggerWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO DescriptorHeapDebuggerWindow_New()
{
    retcs DBG_NEW DescriptorHeapDebuggerWindow();
}

NATIVE_EXPORT_AUTO DescriptorHeapDebuggerWindow_Delete(cs<DescriptorHeapDebuggerWindow*> w)
{
    delete w;
}

#include "pch.h"
#include "Editor/DescriptorHeapDebuggerWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO DescriptorHeapDebuggerWindow_New()
{
    retcs MARCH_NEW DescriptorHeapDebuggerWindow();
}

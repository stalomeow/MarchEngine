#include "pch.h"
#include "MemoryProfilerWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO MemoryProfilerWindow_New()
{
    retcs MARCH_NEW(MemoryProfilerWindow, MemoryLabel::Default)();
}

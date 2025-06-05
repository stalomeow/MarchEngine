#include "pch.h"
#include "ImGuiDemoWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO ImGuiDemoWindow_New()
{
    retcs MARCH_NEW ImGuiDemoWindow();
}

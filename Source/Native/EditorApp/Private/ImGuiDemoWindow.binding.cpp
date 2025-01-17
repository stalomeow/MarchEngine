#include "pch.h"
#include "Editor/ImGuiDemoWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO ImGuiDemoWindow_New()
{
    retcs MARCH_NEW ImGuiDemoWindow();
}

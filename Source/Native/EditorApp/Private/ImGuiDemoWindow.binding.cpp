#include "ImGuiDemoWindow.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO ImGuiDemoWindow_New()
{
    retcs DBG_NEW ImGuiDemoWindow();
}

NATIVE_EXPORT_AUTO ImGuiDemoWindow_Delete(cs<ImGuiDemoWindow*> w)
{
    delete w;
}

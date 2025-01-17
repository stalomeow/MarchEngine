#include "pch.h"
#include "Editor/ConsoleWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO ConsoleWindow_New()
{
    retcs MARCH_NEW ConsoleWindow();
}

NATIVE_EXPORT_AUTO ConsoleWindow_GetLogTypeFilter(cs<ConsoleWindow*> w)
{
    retcs ConsoleWindowInternalUtility::GetLogTypeFilter(w);
}

NATIVE_EXPORT_AUTO ConsoleWindow_SetLogTypeFilter(cs<ConsoleWindow*> w, cs_int value)
{
    ConsoleWindowInternalUtility::SetLogTypeFilter(w, value);
}

NATIVE_EXPORT_AUTO ConsoleWindow_GetAutoScroll(cs<ConsoleWindow*> w)
{
    retcs ConsoleWindowInternalUtility::GetAutoScroll(w);
}

NATIVE_EXPORT_AUTO ConsoleWindow_SetAutoScroll(cs<ConsoleWindow*> w, cs_bool value)
{
    ConsoleWindowInternalUtility::SetAutoScroll(w, value);
}

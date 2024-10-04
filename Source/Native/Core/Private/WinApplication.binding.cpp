#include "WinApplication.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO Application_GetDeltaTime()
{
    return to_cs(GetApp().GetDeltaTime());
}

NATIVE_EXPORT_AUTO Application_GetElapsedTime()
{
    return to_cs(GetApp().GetElapsedTime());
}

NATIVE_EXPORT_AUTO Application_GetFrameCount()
{
    return to_cs(GetApp().GetFrameCount());
}

NATIVE_EXPORT_AUTO Application_GetEngine()
{
    return to_cs(GetApp().GetEngine());
}

NATIVE_EXPORT_AUTO Application_GetDataPath()
{
    return to_cs(GetApp().GetDataPath());
}

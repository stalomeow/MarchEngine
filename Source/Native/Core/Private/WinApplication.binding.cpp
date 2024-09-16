#include "WinApplication.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(CSharpFloat) Application_GetDeltaTime()
{
    return GetApp().GetDeltaTime();
}

NATIVE_EXPORT(CSharpFloat) Application_GetElapsedTime()
{
    return GetApp().GetElapsedTime();
}

NATIVE_EXPORT(CSharpULong) Application_GetFrameCount()
{
    return GetApp().GetFrameCount();
}

NATIVE_EXPORT(IEngine*) Application_GetEngine()
{
    return GetApp().GetEngine();
}

NATIVE_EXPORT(CSharpString) Application_GetDataPath()
{
    return CSharpString_FromUtf8(GetApp().GetDataPath());
}

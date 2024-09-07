#include "App/WinApplication.h"
#include "Scripting/ScriptTypes.h"

using namespace dx12demo;

NATIVE_EXPORT(CSharpFloat) Application_GetDeltaTime()
{
    return GetApp().GetDeltaTime();
}

NATIVE_EXPORT(CSharpFloat) Application_GetElapsedTime()
{
    return GetApp().GetElapsedTime();
}

NATIVE_EXPORT(IEngine*) Application_GetEngine()
{
    return GetApp().GetEngine();
}

NATIVE_EXPORT(CSharpString) Application_GetDataPath()
{
    return CSharpString_FromUtf8(GetApp().GetDataPath());
}

#include "pch.h"
#include "Engine/Application.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO Application_GetDeltaTime()
{
    retcs GetApp()->GetDeltaTime();
}

NATIVE_EXPORT_AUTO Application_GetElapsedTime()
{
    retcs GetApp()->GetElapsedTime();
}

NATIVE_EXPORT_AUTO Application_GetFrameCount()
{
    retcs GetApp()->GetFrameCount();
}

NATIVE_EXPORT_AUTO Application_GetProjectName()
{
    retcs GetApp()->GetProjectName();
}

NATIVE_EXPORT_AUTO Application_GetDataPath()
{
    retcs GetApp()->GetDataPath();
}

NATIVE_EXPORT_AUTO Application_GetEngineResourcePath()
{
    retcs GetApp()->GetEngineResourcePath();
}

NATIVE_EXPORT_AUTO Application_GetEngineShaderPath()
{
    retcs GetApp()->GetEngineShaderPath();
}

NATIVE_EXPORT_AUTO Application_GetShaderCachePath()
{
    retcs GetApp()->GetShaderCachePath();
}

NATIVE_EXPORT_AUTO Application_GetIsEngineResourceEditable()
{
    retcs GetApp()->IsEngineResourceEditable();
}

NATIVE_EXPORT_AUTO Application_GetIsEngineShaderEditable()
{
    retcs GetApp()->IsEngineShaderEditable();
}

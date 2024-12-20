#include "Application.h"
#include "InteropServices.h"

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

NATIVE_EXPORT_AUTO Application_GetDataPath()
{
    retcs GetApp()->GetDataPath();
}

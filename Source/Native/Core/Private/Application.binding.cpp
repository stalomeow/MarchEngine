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

NATIVE_EXPORT_AUTO Application_SaveFilePanelInProject(cs_string title, cs_string defaultName, cs_string extension, cs_string path)
{
    retcs GetApp()->SaveFilePanelInProject(title, defaultName, extension, path);
}

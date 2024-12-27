#include "pch.h"
#include "Editor/EditorApplication.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO EditorApplication_SaveFilePanelInProject(cs_string title, cs_string defaultName, cs_string extension, cs_string path)
{
    EditorApplication* app = static_cast<EditorApplication*>(GetApp());
    retcs app->SaveFilePanelInProject(title, defaultName, extension, path);
}

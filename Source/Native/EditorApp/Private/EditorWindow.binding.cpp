#include "pch.h"
#include "Editor/EditorWindow.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO EditorWindow_NewDefault()
{
    retcs MARCH_NEW EditorWindow();
}

NATIVE_EXPORT_AUTO EditorWindow_GetTitle(cs<EditorWindow*> w)
{
    retcs w->GetTitle();
}

NATIVE_EXPORT_AUTO EditorWindow_SetTitle(cs<EditorWindow*> w, cs_string title)
{
    EditorWindowInternalUtility::SetTitle(w, title);
}

NATIVE_EXPORT_AUTO EditorWindow_GetId(cs<EditorWindow*> w)
{
    retcs w->GetId();
}

NATIVE_EXPORT_AUTO EditorWindow_SetId(cs<EditorWindow*> w, cs_string id)
{
    EditorWindowInternalUtility::SetId(w, id);
}

NATIVE_EXPORT_AUTO EditorWindow_GetDefaultSize(cs<EditorWindow*> w)
{
    const ImVec2& size = w->GetDefaultSize();
    retcs XMFLOAT2(size.x, size.y);
}

NATIVE_EXPORT_AUTO EditorWindow_SetDefaultSize(cs<EditorWindow*> w, cs_vec2 size)
{
    EditorWindowInternalUtility::SetDefaultSize(w, ImVec2(size.data.x, size.data.y));
}

NATIVE_EXPORT_AUTO EditorWindow_GetIsOpen(cs<EditorWindow*> w)
{
    retcs w->GetIsOpen();
}

NATIVE_EXPORT_AUTO EditorWindow_SetIsOpen(cs<EditorWindow*> w, cs_bool value)
{
    EditorWindowInternalUtility::SetIsOpen(w, value);
}

NATIVE_EXPORT_AUTO EditorWindow_Begin(cs<EditorWindow*> w)
{
    retcs EditorWindowInternalUtility::InvokeBegin(w);
}

NATIVE_EXPORT_AUTO EditorWindow_End(cs<EditorWindow*> w)
{
    EditorWindowInternalUtility::InvokeEnd(w);
}

NATIVE_EXPORT_AUTO EditorWindow_OnOpen(cs<EditorWindow*> w)
{
    EditorWindowInternalUtility::InvokeOnOpen(w);
}

NATIVE_EXPORT_AUTO EditorWindow_OnClose(cs<EditorWindow*> w)
{
    EditorWindowInternalUtility::InvokeOnClose(w);
}

NATIVE_EXPORT_AUTO EditorWindow_OnDraw(cs<EditorWindow*> w)
{
    EditorWindowInternalUtility::InvokeOnDraw(w);
}

#include "EditorWindow.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO EditorWindow_CreateDefault()
{
    retcs new EditorWindow();
}

NATIVE_EXPORT_AUTO EditorWindow_DeleteDefault(cs<EditorWindow*> w)
{
    delete w;
}

NATIVE_EXPORT_AUTO EditorWindow_GetTitle(cs<EditorWindow*> w)
{
    retcs w->GetTitle();
}

NATIVE_EXPORT_AUTO EditorWindow_SetTitle(cs<EditorWindow*> w, cs_string title)
{
    EditorWindowInternalUtility::SetTitle(w, title);
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

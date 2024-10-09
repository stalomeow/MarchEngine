#include "SceneWindow.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO SceneWindow_New()
{
    retcs new SceneWindow();
}

NATIVE_EXPORT_AUTO SceneWindow_Delete(cs<SceneWindow*> w)
{
    delete w;
}

NATIVE_EXPORT_AUTO SceneWindow_DrawMenuBar(cs<SceneWindow*> w, cs<cs_bool*> wireframe)
{
    bool enableWireframe = wireframe;
    SceneWindowInternalUtility::DrawMenuBar(w, enableWireframe);
    wireframe->assign(enableWireframe);
}

NATIVE_EXPORT_AUTO SceneWindow_UpdateDisplay(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::UpdateDisplay(w);
}

NATIVE_EXPORT_AUTO SceneWindow_DrawSceneView(cs<SceneWindow*> w)
{
    SceneWindowInternalUtility::DrawSceneView(w);
}

NATIVE_EXPORT_AUTO SceneWindow_TravelScene(cs<SceneWindow*> w, cs<cs_vec3*> cameraPosition, cs<cs_quat*> cameraRotation)
{
    XMFLOAT3 pos = *cameraPosition;
    XMFLOAT4 rot = *cameraRotation;
    SceneWindowInternalUtility::TravelScene(w, pos, rot);
    cameraPosition->assign(pos);
    cameraRotation->assign(rot);
}

NATIVE_EXPORT_AUTO SceneWindow_GetMouseSensitivity(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetMouseSensitivity(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetMouseSensitivity(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetMouseSensitivity(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetRotateDegSpeed(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetRotateDegSpeed(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetRotateDegSpeed(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetRotateDegSpeed(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetNormalMoveSpeed(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetNormalMoveSpeed(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetNormalMoveSpeed(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetNormalMoveSpeed(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetFastMoveSpeed(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetFastMoveSpeed(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetFastMoveSpeed(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetFastMoveSpeed(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetPanSpeed(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetPanSpeed(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetPanSpeed(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetPanSpeed(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetZoomSpeed(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetZoomSpeed(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetZoomSpeed(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetZoomSpeed(w, value);
}

#include "SceneWindow.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO SceneWindow_New()
{
    retcs DBG_NEW SceneWindow();
}

NATIVE_EXPORT_AUTO SceneWindow_Delete(cs<SceneWindow*> w)
{
    delete w;
}

NATIVE_EXPORT_AUTO SceneWindow_DrawMenuBar(cs<SceneWindow*> w)
{
    SceneWindowInternalUtility::DrawMenuBar(w);
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

NATIVE_EXPORT_AUTO SceneWindow_ManipulateTransform(cs<SceneWindow*> w, cs<Camera*> camera, cs<cs_mat4*> localToWorldMatrix)
{
    XMFLOAT4X4 m = *localToWorldMatrix;
    bool changed = SceneWindowInternalUtility::ManipulateTransform(w, camera, m);
    if (changed) localToWorldMatrix->assign(m);
    retcs changed;
}

NATIVE_EXPORT_AUTO SceneWindow_DrawGizmoTexts(cs<SceneWindow*> w, cs<Camera*> camera)
{
    SceneWindowInternalUtility::DrawGizmoTexts(w, camera);
}

NATIVE_EXPORT_AUTO SceneWindow_DrawWindowSettings(cs<SceneWindow*> w)
{
    SceneWindowInternalUtility::DrawWindowSettings(w);
}

NATIVE_EXPORT_AUTO SceneWindow_GetEnableMSAA(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetEnableMSAA(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetEnableMSAA(cs<SceneWindow*> w, cs_bool value)
{
    SceneWindowInternalUtility::SetEnableMSAA(w, value);
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

NATIVE_EXPORT_AUTO SceneWindow_GetGizmoOperation(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetGizmoOperation(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetGizmoOperation(cs<SceneWindow*> w, cs<SceneGizmoOperation> value)
{
    SceneWindowInternalUtility::SetGizmoOperation(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetGizmoMode(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetGizmoMode(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetGizmoMode(cs<SceneWindow*> w, cs<SceneGizmoMode> value)
{
    SceneWindowInternalUtility::SetGizmoMode(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetGizmoSnap(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetGizmoSnap(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetGizmoSnap(cs<SceneWindow*> w, cs_bool value)
{
    SceneWindowInternalUtility::SetGizmoSnap(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetGizmoTranslationSnapValue(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetGizmoTranslationSnapValue(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetGizmoTranslationSnapValue(cs<SceneWindow*> w, cs_vec3 value)
{
    SceneWindowInternalUtility::SetGizmoTranslationSnapValue(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetGizmoRotationSnapValue(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetGizmoRotationSnapValue(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetGizmoRotationSnapValue(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetGizmoRotationSnapValue(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetGizmoScaleSnapValue(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetGizmoScaleSnapValue(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetGizmoScaleSnapValue(cs<SceneWindow*> w, cs_float value)
{
    SceneWindowInternalUtility::SetGizmoScaleSnapValue(w, value);
}

NATIVE_EXPORT_AUTO SceneWindow_GetWindowMode(cs<SceneWindow*> w)
{
    retcs SceneWindowInternalUtility::GetWindowMode(w);
}

NATIVE_EXPORT_AUTO SceneWindow_SetWindowMode(cs<SceneWindow*> w, cs<SceneWindowMode> value)
{
    SceneWindowInternalUtility::SetWindowMode(w, value);
}

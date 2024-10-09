#include "Camera.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO Camera_New()
{
    retcs DBG_NEW Camera();
}

NATIVE_EXPORT_AUTO Camera_Delete(cs<Camera*> camera)
{
    delete camera;
}

NATIVE_EXPORT_AUTO Camera_GetPixelWidth(cs<Camera*> camera)
{
    retcs static_cast<int32_t>(camera->GetPixelWidth());
}

NATIVE_EXPORT_AUTO Camera_GetPixelHeight(cs<Camera*> camera)
{
    retcs static_cast<int32_t>(camera->GetPixelHeight());
}

NATIVE_EXPORT_AUTO Camera_GetAspectRatio(cs<Camera*> camera)
{
    retcs camera->GetAspectRatio();
}

NATIVE_EXPORT_AUTO Camera_GetEnableMSAA(cs<Camera*> camera)
{
    retcs camera->GetEnableMSAA();
}

NATIVE_EXPORT_AUTO Camera_SetEnableMSAA(cs<Camera*> camera, cs_bool value)
{
    camera->SetEnableMSAA(value);
}

NATIVE_EXPORT_AUTO Camera_GetVerticalFieldOfView(cs<Camera*> camera)
{
    retcs camera->GetVerticalFieldOfView();
}

NATIVE_EXPORT_AUTO Camera_SetVerticalFieldOfView(cs<Camera*> camera, cs_float value)
{
    CameraInternalUtility::SetVerticalFieldOfView(camera, value);
}

NATIVE_EXPORT_AUTO Camera_GetHorizontalFieldOfView(cs<Camera*> camera)
{
    retcs camera->GetHorizontalFieldOfView();
}

NATIVE_EXPORT_AUTO Camera_SetHorizontalFieldOfView(cs<Camera*> camera, cs_float value)
{
    CameraInternalUtility::SetHorizontalFieldOfView(camera, value);
}

NATIVE_EXPORT_AUTO Camera_GetNearClipPlane(cs<Camera*> camera)
{
    retcs camera->GetNearClipPlane();
}

NATIVE_EXPORT_AUTO Camera_SetNearClipPlane(cs<Camera*> camera, cs_float value)
{
    CameraInternalUtility::SetNearClipPlane(camera, value);
}

NATIVE_EXPORT_AUTO Camera_GetFarClipPlane(cs<Camera*> camera)
{
    retcs camera->GetFarClipPlane();
}

NATIVE_EXPORT_AUTO Camera_SetFarClipPlane(cs<Camera*> camera, cs_float value)
{
    CameraInternalUtility::SetFarClipPlane(camera, value);
}

NATIVE_EXPORT_AUTO Camera_GetEnableWireframe(cs<Camera*> camera)
{
    retcs camera->GetEnableWireframe();
}

NATIVE_EXPORT_AUTO Camera_SetEnableWireframe(cs<Camera*> camera, cs_bool value)
{
    CameraInternalUtility::SetEnableWireframe(camera, value);
}

NATIVE_EXPORT_AUTO Camera_GetEnableGizmos(cs<Camera*> camera)
{
    retcs camera->GetEnableGizmos();
}

NATIVE_EXPORT_AUTO Camera_SetEnableGizmos(cs<Camera*> camera, cs_bool value)
{
    CameraInternalUtility::SetEnableGizmos(camera, value);
}

NATIVE_EXPORT_AUTO Camera_GetViewMatrix(cs<Camera*> camera)
{
    retcs camera->GetViewMatrix();
}

NATIVE_EXPORT_AUTO Camera_GetProjectionMatrix(cs<Camera*> camera)
{
    retcs camera->GetProjectionMatrix();
}

NATIVE_EXPORT_AUTO Camera_SetCustomTargetDisplay(cs<Camera*> camera, cs<Display*> display)
{
    CameraInternalUtility::SetCustomTargetDisplay(camera, display);
}

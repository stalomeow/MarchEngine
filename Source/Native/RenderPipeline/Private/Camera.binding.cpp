#include "Camera.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(Camera*) Camera_New()
{
    return new Camera();
}

NATIVE_EXPORT(void) Camera_Delete(Camera* camera)
{
    delete camera;
}

NATIVE_EXPORT(CSharpInt) Camera_GetPixelWidth(Camera* camera)
{
    return static_cast<CSharpInt>(camera->GetPixelWidth());
}

NATIVE_EXPORT(CSharpInt) Camera_GetPixelHeight(Camera* camera)
{
    return static_cast<CSharpInt>(camera->GetPixelHeight());
}

NATIVE_EXPORT(CSharpFloat) Camera_GetAspectRatio(Camera* camera)
{
    return camera->GetAspectRatio();
}

NATIVE_EXPORT(CSharpBool) Camera_GetEnableMSAA(Camera* camera)
{
    return CSHARP_MARSHAL_BOOL(camera->GetEnableMSAA());
}

NATIVE_EXPORT(void) Camera_SetEnableMSAA(Camera* camera, CSharpBool value)
{
    camera->SetEnableMSAA(CSHARP_UNMARSHAL_BOOL(value));
}

NATIVE_EXPORT(CSharpFloat) Camera_GetVerticalFieldOfView(Camera* camera)
{
    return camera->GetVerticalFieldOfView();
}

NATIVE_EXPORT(void) Camera_SetVerticalFieldOfView(Camera* camera, CSharpFloat value)
{
    CameraInternalUtility::SetVerticalFieldOfView(camera, value);
}

NATIVE_EXPORT(CSharpFloat) Camera_GetHorizontalFieldOfView(Camera* camera)
{
    return camera->GetHorizontalFieldOfView();
}

NATIVE_EXPORT(void) Camera_SetHorizontalFieldOfView(Camera* camera, CSharpFloat value)
{
    CameraInternalUtility::SetHorizontalFieldOfView(camera, value);
}

NATIVE_EXPORT(CSharpFloat) Camera_GetNearClipPlane(Camera* camera)
{
    return camera->GetNearClipPlane();
}

NATIVE_EXPORT(void) Camera_SetNearClipPlane(Camera* camera, CSharpFloat value)
{
    CameraInternalUtility::SetNearClipPlane(camera, value);
}

NATIVE_EXPORT(CSharpFloat) Camera_GetFarClipPlane(Camera* camera)
{
    return camera->GetFarClipPlane();
}

NATIVE_EXPORT(void) Camera_SetFarClipPlane(Camera* camera, CSharpFloat value)
{
    CameraInternalUtility::SetFarClipPlane(camera, value);
}

NATIVE_EXPORT(CSharpBool) Camera_GetEnableWireframe(Camera* camera)
{
    return CSHARP_MARSHAL_BOOL(camera->GetEnableWireframe());
}

NATIVE_EXPORT(void) Camera_SetEnableWireframe(Camera* camera, CSharpBool value)
{
    CameraInternalUtility::SetEnableWireframe(camera, CSHARP_UNMARSHAL_BOOL(value));
}

NATIVE_EXPORT(CSharpBool) Camera_GetIsEditorSceneCamera(Camera* camera)
{
    return CSHARP_MARSHAL_BOOL(camera->GetIsEditorSceneCamera());
}

NATIVE_EXPORT(void) Camera_SetIsEditorSceneCamera(Camera* camera, CSharpBool value)
{
    CameraInternalUtility::SetIsEditorSceneCamera(camera, CSHARP_UNMARSHAL_BOOL(value));
}

NATIVE_EXPORT(CSharpMatrix4x4) Camera_GetViewMatrix(Camera* camera)
{
    return ToCSharpMatrix4x4(camera->GetViewMatrix());
}

NATIVE_EXPORT(CSharpMatrix4x4) Camera_GetProjectionMatrix(Camera* camera)
{
    return ToCSharpMatrix4x4(camera->GetProjectionMatrix());
}

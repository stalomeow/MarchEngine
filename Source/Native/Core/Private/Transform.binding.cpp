#include "Transform.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(Transform*) Transform_Create()
{
    return new Transform();
}

NATIVE_EXPORT(void) Transform_Delete(Transform* transform)
{
    delete transform;
}

NATIVE_EXPORT(void) Transform_SetParent(Transform* transform, Transform* parent)
{
    TransformInternalUtility::SetParent(transform, parent);
}

NATIVE_EXPORT(CSharpVector3) Transform_GetLocalPosition(Transform* transform)
{
    return ToCSharpVector3(transform->GetLocalPosition());
}

NATIVE_EXPORT(void) Transform_SetLocalPosition(Transform* transform, CSharpVector3 value)
{
    TransformInternalUtility::SetLocalPosition(transform, ToXMFLOAT3(value));
}

NATIVE_EXPORT(CSharpQuaternion) Transform_GetLocalRotation(Transform* transform)
{
    return ToCSharpQuaternion(transform->GetLocalRotation());
}

NATIVE_EXPORT(void) Transform_SetLocalRotation(Transform* transform, CSharpQuaternion value)
{
    TransformInternalUtility::SetLocalRotation(transform, ToXMFLOAT4(value));
}

NATIVE_EXPORT(void) Transform_SetLocalRotationWithoutSyncEulerAngles(Transform* transform, CSharpQuaternion value)
{
    TransformInternalUtility::SetLocalRotationWithoutSyncEulerAngles(transform, ToXMFLOAT4(value));
}

NATIVE_EXPORT(CSharpVector3) Transform_GetLocalEulerAngles(Transform* transform)
{
    return ToCSharpVector3(transform->GetLocalEulerAngles());
}

NATIVE_EXPORT(void) Transform_SetLocalEulerAngles(Transform* transform, CSharpVector3 value)
{
    TransformInternalUtility::SetLocalEulerAngles(transform, ToXMFLOAT3(value));
}

NATIVE_EXPORT(void) Transform_SetLocalEulerAnglesWithoutSyncRotation(Transform* transform, CSharpVector3 value)
{
    TransformInternalUtility::SetLocalEulerAnglesWithoutSyncRotation(transform, ToXMFLOAT3(value));
}

NATIVE_EXPORT(CSharpVector3) Transform_GetLocalScale(Transform* transform)
{
    return ToCSharpVector3(transform->GetLocalScale());
}

NATIVE_EXPORT(void) Transform_SetLocalScale(Transform* transform, CSharpVector3 value)
{
    TransformInternalUtility::SetLocalScale(transform, ToXMFLOAT3(value));
}

NATIVE_EXPORT(CSharpVector3) Transform_GetPosition(Transform* transform)
{
    return ToCSharpVector3(transform->GetPosition());
}

NATIVE_EXPORT(void) Transform_SetPosition(Transform* transform, CSharpVector3 value)
{
    TransformInternalUtility::SetPosition(transform, ToXMFLOAT3(value));
}

NATIVE_EXPORT(CSharpQuaternion) Transform_GetRotation(Transform* transform)
{
    return ToCSharpQuaternion(transform->GetRotation());
}

NATIVE_EXPORT(void) Transform_SetRotation(Transform* transform, CSharpQuaternion value)
{
    TransformInternalUtility::SetRotation(transform, ToXMFLOAT4(value));
}

NATIVE_EXPORT(CSharpVector3) Transform_GetEulerAngles(Transform* transform)
{
    return ToCSharpVector3(transform->GetEulerAngles());
}

NATIVE_EXPORT(void) Transform_SetEulerAngles(Transform* transform, CSharpVector3 value)
{
    TransformInternalUtility::SetEulerAngles(transform, ToXMFLOAT3(value));
}

NATIVE_EXPORT(CSharpVector3) Transform_GetLossyScale(Transform* transform)
{
    return ToCSharpVector3(transform->GetLossyScale());
}

NATIVE_EXPORT(CSharpMatrix4x4) Transform_GetLocalToWorldMatrix(Transform* transform)
{
    return ToCSharpMatrix4x4(transform->GetLocalToWorldMatrix());
}

NATIVE_EXPORT(CSharpMatrix4x4) Transform_GetWorldToLocalMatrix(Transform* transform)
{
    return ToCSharpMatrix4x4(transform->GetWorldToLocalMatrix());
}

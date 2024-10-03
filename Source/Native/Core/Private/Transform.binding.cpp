#include "Transform.h"
#include "InteropServices.h"

using namespace march;
using namespace DirectX;

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

NATIVE_EXPORT(CSharpVector3) Transform_GetForward(Transform* transform)
{
    return ToCSharpVector3(transform->GetForward());
}

NATIVE_EXPORT(CSharpVector3) Transform_GetRight(Transform* transform)
{
    return ToCSharpVector3(transform->GetRight());
}

NATIVE_EXPORT(CSharpVector3) Transform_GetUp(Transform* transform)
{
    return ToCSharpVector3(transform->GetUp());
}

NATIVE_EXPORT(CSharpVector3) Transform_TransformVector(Transform* transform, CSharpVector3 vector)
{
    XMFLOAT3 float3 = ToXMFLOAT3(vector);
    XMStoreFloat3(&float3, transform->TransformVector(XMLoadFloat3(&float3)));
    return ToCSharpVector3(float3);
}

NATIVE_EXPORT(CSharpVector3) Transform_TransformDirection(Transform* transform, CSharpVector3 direction)
{
    XMFLOAT3 float3 = ToXMFLOAT3(direction);
    XMStoreFloat3(&float3, transform->TransformDirection(XMLoadFloat3(&float3)));
    return ToCSharpVector3(float3);
}

NATIVE_EXPORT(CSharpVector3) Transform_TransformPoint(Transform* transform, CSharpVector3 point)
{
    XMFLOAT3 float3 = ToXMFLOAT3(point);
    XMStoreFloat3(&float3, transform->TransformPoint(XMLoadFloat3(&float3)));
    return ToCSharpVector3(float3);
}

NATIVE_EXPORT(CSharpVector3) Transform_InverseTransformVector(Transform* transform, CSharpVector3 vector)
{
    XMFLOAT3 float3 = ToXMFLOAT3(vector);
    XMStoreFloat3(&float3, transform->InverseTransformVector(XMLoadFloat3(&float3)));
    return ToCSharpVector3(float3);
}

NATIVE_EXPORT(CSharpVector3) Transform_InverseTransformDirection(Transform* transform, CSharpVector3 direction)
{
    XMFLOAT3 float3 = ToXMFLOAT3(direction);
    XMStoreFloat3(&float3, transform->InverseTransformDirection(XMLoadFloat3(&float3)));
    return ToCSharpVector3(float3);
}

NATIVE_EXPORT(CSharpVector3) Transform_InverseTransformPoint(Transform* transform, CSharpVector3 point)
{
    XMFLOAT3 float3 = ToXMFLOAT3(point);
    XMStoreFloat3(&float3, transform->InverseTransformPoint(XMLoadFloat3(&float3)));
    return ToCSharpVector3(float3);
}

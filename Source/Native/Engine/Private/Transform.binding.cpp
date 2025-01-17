#include "pch.h"
#include "Engine/Transform.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO Transform_New()
{
    retcs MARCH_NEW Transform();
}

NATIVE_EXPORT_AUTO Transform_SetParent(cs<Transform*> transform, cs<Transform*> parent)
{
    TransformInternalUtility::SetParent(transform, parent);
}

NATIVE_EXPORT_AUTO Transform_GetLocalPosition(cs<Transform*> transform)
{
    retcs transform->GetLocalPosition();
}

NATIVE_EXPORT_AUTO Transform_SetLocalPosition(cs<Transform*> transform, cs_vec3 value)
{
    TransformInternalUtility::SetLocalPosition(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLocalRotation(cs<Transform*> transform)
{
    retcs transform->GetLocalRotation();
}

NATIVE_EXPORT_AUTO Transform_SetLocalRotation(cs<Transform*> transform, cs_vec4 value)
{
    TransformInternalUtility::SetLocalRotation(transform, value);
}

NATIVE_EXPORT_AUTO Transform_SetLocalRotationWithoutSyncEulerAngles(cs<Transform*> transform, cs_vec4 value)
{
    TransformInternalUtility::SetLocalRotationWithoutSyncEulerAngles(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLocalEulerAngles(cs<Transform*> transform)
{
    retcs transform->GetLocalEulerAngles();
}

NATIVE_EXPORT_AUTO Transform_SetLocalEulerAngles(cs<Transform*> transform, cs_vec3 value)
{
    TransformInternalUtility::SetLocalEulerAngles(transform, value);
}

NATIVE_EXPORT_AUTO Transform_SetLocalEulerAnglesWithoutSyncRotation(cs<Transform*> transform, cs_vec3 value)
{
    TransformInternalUtility::SetLocalEulerAnglesWithoutSyncRotation(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLocalScale(cs<Transform*> transform)
{
    retcs transform->GetLocalScale();
}

NATIVE_EXPORT_AUTO Transform_SetLocalScale(cs<Transform*> transform, cs_vec3 value)
{
    TransformInternalUtility::SetLocalScale(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetPosition(cs<Transform*> transform)
{
    retcs transform->GetPosition();
}

NATIVE_EXPORT_AUTO Transform_SetPosition(cs<Transform*> transform, cs_vec3 value)
{
    TransformInternalUtility::SetPosition(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetRotation(cs<Transform*> transform)
{
    retcs transform->GetRotation();
}

NATIVE_EXPORT_AUTO Transform_SetRotation(cs<Transform*> transform, cs_quat value)
{
    TransformInternalUtility::SetRotation(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetEulerAngles(cs<Transform*> transform)
{
    retcs transform->GetEulerAngles();
}

NATIVE_EXPORT_AUTO Transform_SetEulerAngles(cs<Transform*> transform, cs_vec3 value)
{
    TransformInternalUtility::SetEulerAngles(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLossyScale(cs<Transform*> transform)
{
    retcs transform->GetLossyScale();
}

NATIVE_EXPORT_AUTO Transform_GetLocalToWorldMatrix(cs<Transform*> transform)
{
    retcs transform->GetLocalToWorldMatrix();
}

NATIVE_EXPORT_AUTO Transform_SetLocalToWorldMatrix(cs<Transform*> transform, cs_mat4 value)
{
    TransformInternalUtility::SetLocalToWorldMatrix(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetWorldToLocalMatrix(cs<Transform*> transform)
{
    retcs transform->GetWorldToLocalMatrix();
}

NATIVE_EXPORT_AUTO Transform_SetWorldToLocalMatrix(cs<Transform*> transform, cs_mat4 value)
{
    TransformInternalUtility::SetWorldToLocalMatrix(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetForward(cs<Transform*> transform)
{
    retcs transform->GetForward();
}

NATIVE_EXPORT_AUTO Transform_GetRight(cs<Transform*> transform)
{
    retcs transform->GetRight();
}

NATIVE_EXPORT_AUTO Transform_GetUp(cs<Transform*> transform)
{
    retcs transform->GetUp();
}

NATIVE_EXPORT_AUTO Transform_TransformVector(cs<Transform*> transform, cs_vec3 vector)
{
    XMFLOAT3 float3 = vector;
    XMStoreFloat3(&float3, transform->TransformVector(XMLoadFloat3(&float3)));
    retcs float3;
}

NATIVE_EXPORT_AUTO Transform_TransformDirection(cs<Transform*> transform, cs_vec3 direction)
{
    XMFLOAT3 float3 = direction;
    XMStoreFloat3(&float3, transform->TransformDirection(XMLoadFloat3(&float3)));
    retcs float3;
}

NATIVE_EXPORT_AUTO Transform_TransformPoint(cs<Transform*> transform, cs_vec3 point)
{
    XMFLOAT3 float3 = point;
    XMStoreFloat3(&float3, transform->TransformPoint(XMLoadFloat3(&float3)));
    retcs float3;
}

NATIVE_EXPORT_AUTO Transform_InverseTransformVector(cs<Transform*> transform, cs_vec3 vector)
{
    XMFLOAT3 float3 = vector;
    XMStoreFloat3(&float3, transform->InverseTransformVector(XMLoadFloat3(&float3)));
    retcs float3;
}

NATIVE_EXPORT_AUTO Transform_InverseTransformDirection(cs<Transform*> transform, cs_vec3 direction)
{
    XMFLOAT3 float3 = direction;
    XMStoreFloat3(&float3, transform->InverseTransformDirection(XMLoadFloat3(&float3)));
    retcs float3;
}

NATIVE_EXPORT_AUTO Transform_InverseTransformPoint(cs<Transform*> transform, cs_vec3 point)
{
    XMFLOAT3 float3 = point;
    XMStoreFloat3(&float3, transform->InverseTransformPoint(XMLoadFloat3(&float3)));
    retcs float3;
}

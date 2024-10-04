#include "Transform.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO Transform_Create()
{
    return to_cs(new Transform());
}

NATIVE_EXPORT_AUTO Transform_Delete(cs<Transform*> transform)
{
    delete transform;
}

NATIVE_EXPORT_AUTO Transform_SetParent(cs<Transform*> transform, cs<Transform*> parent)
{
    TransformInternalUtility::SetParent(transform, parent);
}

NATIVE_EXPORT_AUTO Transform_GetLocalPosition(cs<Transform*> transform)
{
    return to_cs(transform->GetLocalPosition());
}

NATIVE_EXPORT_AUTO Transform_SetLocalPosition(cs<Transform*> transform, cs_vector3 value)
{
    TransformInternalUtility::SetLocalPosition(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLocalRotation(cs<Transform*> transform)
{
    return to_cs(transform->GetLocalRotation());
}

NATIVE_EXPORT_AUTO Transform_SetLocalRotation(cs<Transform*> transform, cs_vector4 value)
{
    TransformInternalUtility::SetLocalRotation(transform, value);
}

NATIVE_EXPORT_AUTO Transform_SetLocalRotationWithoutSyncEulerAngles(cs<Transform*> transform, cs_vector4 value)
{
    TransformInternalUtility::SetLocalRotationWithoutSyncEulerAngles(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLocalEulerAngles(cs<Transform*> transform)
{
    return to_cs(transform->GetLocalEulerAngles());
}

NATIVE_EXPORT_AUTO Transform_SetLocalEulerAngles(cs<Transform*> transform, cs_vector3 value)
{
    TransformInternalUtility::SetLocalEulerAngles(transform, value);
}

NATIVE_EXPORT_AUTO Transform_SetLocalEulerAnglesWithoutSyncRotation(cs<Transform*> transform, cs_vector3 value)
{
    TransformInternalUtility::SetLocalEulerAnglesWithoutSyncRotation(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLocalScale(cs<Transform*> transform)
{
    return to_cs(transform->GetLocalScale());
}

NATIVE_EXPORT_AUTO Transform_SetLocalScale(cs<Transform*> transform, cs_vector3 value)
{
    TransformInternalUtility::SetLocalScale(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetPosition(cs<Transform*> transform)
{
    return to_cs(transform->GetPosition());
}

NATIVE_EXPORT_AUTO Transform_SetPosition(cs<Transform*> transform, cs_vector3 value)
{
    TransformInternalUtility::SetPosition(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetRotation(cs<Transform*> transform)
{
    return to_cs(transform->GetRotation());
}

NATIVE_EXPORT_AUTO Transform_SetRotation(cs<Transform*> transform, cs_quaternion value)
{
    TransformInternalUtility::SetRotation(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetEulerAngles(cs<Transform*> transform)
{
    return to_cs(transform->GetEulerAngles());
}

NATIVE_EXPORT_AUTO Transform_SetEulerAngles(cs<Transform*> transform, cs_vector3 value)
{
    TransformInternalUtility::SetEulerAngles(transform, value);
}

NATIVE_EXPORT_AUTO Transform_GetLossyScale(cs<Transform*> transform)
{
    return to_cs(transform->GetLossyScale());
}

NATIVE_EXPORT_AUTO Transform_GetLocalToWorldMatrix(cs<Transform*> transform)
{
    return to_cs(transform->GetLocalToWorldMatrix());
}

NATIVE_EXPORT_AUTO Transform_GetWorldToLocalMatrix(cs<Transform*> transform)
{
    return to_cs(transform->GetWorldToLocalMatrix());
}

NATIVE_EXPORT_AUTO Transform_GetForward(cs<Transform*> transform)
{
    return to_cs(transform->GetForward());
}

NATIVE_EXPORT_AUTO Transform_GetRight(cs<Transform*> transform)
{
    return to_cs(transform->GetRight());
}

NATIVE_EXPORT_AUTO Transform_GetUp(cs<Transform*> transform)
{
    return to_cs(transform->GetUp());
}

NATIVE_EXPORT_AUTO Transform_TransformVector(cs<Transform*> transform, cs_vector3 vector)
{
    XMFLOAT3 float3 = vector;
    XMStoreFloat3(&float3, transform->TransformVector(XMLoadFloat3(&vector)));
    return to_cs(float3);
}

NATIVE_EXPORT_AUTO Transform_TransformDirection(cs<Transform*> transform, cs_vector3 direction)
{
    XMFLOAT3 float3 = direction;
    XMStoreFloat3(&float3, transform->TransformDirection(XMLoadFloat3(&float3)));
    return to_cs(float3);
}

NATIVE_EXPORT_AUTO Transform_TransformPoint(cs<Transform*> transform, cs_vector3 point)
{
    XMFLOAT3 float3 = point;
    XMStoreFloat3(&float3, transform->TransformPoint(XMLoadFloat3(&float3)));
    return to_cs(float3);
}

NATIVE_EXPORT_AUTO Transform_InverseTransformVector(cs<Transform*> transform, cs_vector3 vector)
{
    XMFLOAT3 float3 = vector;
    XMStoreFloat3(&float3, transform->InverseTransformVector(XMLoadFloat3(&float3)));
    return to_cs(float3);
}

NATIVE_EXPORT_AUTO Transform_InverseTransformDirection(cs<Transform*> transform, cs_vector3 direction)
{
    XMFLOAT3 float3 = direction;
    XMStoreFloat3(&float3, transform->InverseTransformDirection(XMLoadFloat3(&float3)));
    return to_cs(float3);
}

NATIVE_EXPORT_AUTO Transform_InverseTransformPoint(cs<Transform*> transform, cs_vector3 point)
{
    XMFLOAT3 float3 = point;
    XMStoreFloat3(&float3, transform->InverseTransformPoint(XMLoadFloat3(&float3)));
    return to_cs(float3);
}

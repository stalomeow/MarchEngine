#include "Transform.h"
#include <math.h>

using namespace DirectX;

namespace march
{
    Transform::Transform()
        : m_Parent(nullptr)
        , m_LocalPosition(0, 0, 0)
        , m_LocalRotation(0, 0, 0, 1)
        , m_LocalEulerAngles(0, 0, 0)
        , m_LocalScale(1, 1, 1)
    {
    }

    Transform* Transform::GetParent() const
    {
        return m_Parent;
    }

    XMFLOAT3 Transform::GetLocalPosition() const
    {
        return m_LocalPosition;
    }

    XMFLOAT4 Transform::GetLocalRotation() const
    {
        return m_LocalRotation;
    }

    XMFLOAT3 Transform::GetLocalEulerAngles() const
    {
        return m_LocalEulerAngles;
    }

    XMFLOAT3 Transform::GetLocalScale() const
    {
        return m_LocalScale;
    }

    XMFLOAT3 Transform::GetPosition() const
    {
        XMFLOAT3 result = {};
        XMStoreFloat3(&result, LoadPosition());
        return result;
    }

    XMFLOAT4 Transform::GetRotation() const
    {
        XMFLOAT4 result = {};
        XMStoreFloat4(&result, LoadRotation());
        return result;
    }

    XMFLOAT3 Transform::GetEulerAngles() const
    {
        return QuaternionToEulerAngles(GetRotation());
    }

    XMFLOAT3 Transform::GetLossyScale() const
    {
        XMFLOAT3 result = {};
        XMStoreFloat3(&result, LoadLossyScale());
        return result;
    }

    XMFLOAT4X4 Transform::GetLocalToWorldMatrix() const
    {
        XMFLOAT4X4 result = {};
        XMStoreFloat4x4(&result, LoadLocalToWorldMatrix());
        return result;
    }

    XMFLOAT4X4 Transform::GetWorldToLocalMatrix() const
    {
        XMFLOAT4X4 result = {};
        XMStoreFloat4x4(&result, LoadWorldToLocalMatrix());
        return result;
    }

    XMVECTOR Transform::LoadLocalPosition() const
    {
        return XMLoadFloat3(&m_LocalPosition);
    }

    XMVECTOR Transform::LoadLocalRotation() const
    {
        return XMLoadFloat4(&m_LocalRotation);
    }

    XMVECTOR Transform::LoadLocalEulerAngles() const
    {
        return XMLoadFloat3(&m_LocalEulerAngles);
    }

    XMVECTOR Transform::LoadLocalScale() const
    {
        return XMLoadFloat3(&m_LocalScale);
    }

    XMVECTOR Transform::LoadPosition() const
    {
        const Transform* trans = this;
        XMVECTOR result = XMVectorZero();

        while (trans != nullptr)
        {
            result = XMVectorAdd(result, trans->LoadLocalPosition());
            trans = trans->GetParent();
        }

        return result;
    }

    XMVECTOR Transform::LoadRotation() const
    {
        const Transform* trans = this;
        XMVECTOR result = XMQuaternionIdentity();

        while (trans != nullptr)
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/directxmath/nf-directxmath-xmquaternionmultiply
            // The result represents the rotation Q1 followed by the rotation Q2 to be consistent with XMMatrixMultiply concatenation
            // since this function is typically used to concatenate quaternions that represent rotations
            // (i.e. it returns Q2*Q1).
            result = XMQuaternionMultiply(result, trans->LoadLocalRotation());
            trans = trans->GetParent();
        }

        return result;
    }

    XMVECTOR Transform::LoadEulerAngles() const
    {
        return XMLoadFloat3(&GetEulerAngles());
    }

    XMVECTOR Transform::LoadLossyScale() const
    {
        // https://docs.unity3d.com/ScriptReference/Transform-lossyScale.html
        // Please note that if you have a parent transform with scale and a child that is arbitrarily rotated, the scale will be skewed.
        // Thus scale can not be represented correctly in a 3 component vector but only a 3x3 matrix.
        // Such a representation is quite inconvenient to work with however.
        // lossyScale is a convenience property that attempts to match the actual world scale as much as it can.
        // If your objects are not skewed the value will be completely correct and most likely the value will not be very different if it contains skew too.

        const Transform* trans = this;
        XMVECTOR result = XMVectorSplatOne();

        while (trans != nullptr)
        {
            result = XMVectorMultiply(result, trans->LoadLocalScale());
            trans = trans->GetParent();
        }

        return result;
    }

    XMMATRIX Transform::LoadLocalToWorldMatrix() const
    {
        const Transform* trans = this;
        const XMVECTOR rotationOrigin = XMVectorZero();
        XMMATRIX result = XMMatrixIdentity();

        while (trans != nullptr)
        {
            XMVECTOR translation = LoadLocalPosition();
            XMVECTOR rotation = LoadLocalRotation();
            XMVECTOR scale = LoadLocalScale(); // 不能直接用 lossyScale，因为它不准确
            XMMATRIX mat = XMMatrixAffineTransformation(scale, rotationOrigin, rotation, translation);

            result = XMMatrixMultiply(result, mat); // DirectX 中使用的是行向量
            trans = trans->GetParent();
        }

        return result;
    }

    XMMATRIX Transform::LoadWorldToLocalMatrix() const
    {
        return XMMatrixInverse(nullptr, LoadLocalToWorldMatrix());
    }

    XMFLOAT4 Transform::EulerAnglesToQuaternion(XMFLOAT3 eulerAngles)
    {
        float pitch = XMConvertToRadians(eulerAngles.x);
        float yaw = XMConvertToRadians(eulerAngles.y);
        float roll = XMConvertToRadians(eulerAngles.z);

        // https://learn.microsoft.com/en-us/windows/win32/api/directxmath/nf-directxmath-xmquaternionrotationrollpitchyaw

        XMFLOAT4 result = {};
        XMStoreFloat4(&result, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
        return result;
    }

    XMFLOAT3 Transform::QuaternionToEulerAngles(XMFLOAT4 quaternion)
    {
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, XMMatrixTranspose(XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion))));

        // YXZ Order https://www.geometrictools.com/Documentation/EulerAngles.pdf
        XMFLOAT3 euler(0, 0, 0);

        if (m(1, 2) < 1)
        {
            if (m(1, 2) > -1)
            {
                euler.x = asinf(-m(1, 2));
                euler.y = atan2f(m(0, 2), m(2, 2));
                euler.z = atan2f(m(1, 0), m(1, 1));
            }
            else // m(1, 2) == -1
            {
                // Not a unique solution: euler.z - euler.y = atan2f(-m(0, 1), m(0, 0))
                euler.x = XM_PIDIV2;
                euler.y = -atan2f(-m(0, 1), m(0, 0));
                euler.z = 0;
            }
        }
        else // m(1, 2) == +1
        {
            // Not a unique solution: euler.z + euler.y = atan2f(-m(0, 1), m(0, 0))
            euler.x = -XM_PIDIV2;
            euler.y = atan2f(-m(0, 1), m(0, 0));
            euler.z = 0;
        }

        euler.x = XMConvertToDegrees(euler.x);
        euler.y = XMConvertToDegrees(euler.y);
        euler.z = XMConvertToDegrees(euler.z);

        // 调整角度，让它看起来更符合直觉

        if (euler.y < 0) euler.y += 360;
        if (euler.z < 0) euler.z += 360;

        if (euler.y >= 180 && euler.z >= 180)
        {
            euler.x = 180 - euler.x;
            euler.y -= 180;
            euler.z -= 180;
        }

        if (euler.x < 0) euler.x += 360;
        if (fabsf(euler.x) < FLT_EPSILON) euler.x = 0;
        if (fabsf(euler.y) < FLT_EPSILON) euler.y = 0;
        if (fabsf(euler.z) < FLT_EPSILON) euler.z = 0;

        return euler;
    }

    void TransformInternalUtility::SetParent(Transform* transform, Transform* parent)
    {
        transform->m_Parent = parent;
    }

    void TransformInternalUtility::SetLocalPosition(Transform* transform, const XMFLOAT3& value)
    {
        transform->m_LocalPosition = value;
    }

    void TransformInternalUtility::SetLocalRotation(Transform* transform, const XMFLOAT4& value)
    {
        transform->m_LocalRotation = value;
        SyncLocalEulerAngles(transform);
    }

    void TransformInternalUtility::SetLocalRotationWithoutSyncEulerAngles(Transform* transform, const XMFLOAT4& value)
    {
        transform->m_LocalRotation = value;
    }

    void TransformInternalUtility::SetLocalEulerAngles(Transform* transform, const XMFLOAT3& value)
    {
        transform->m_LocalRotation = Transform::EulerAnglesToQuaternion(value);
        transform->m_LocalEulerAngles = value;
    }

    void TransformInternalUtility::SetLocalEulerAnglesWithoutSyncRotation(Transform* transform, const XMFLOAT3& value)
    {
        transform->m_LocalEulerAngles = value;
    }

    void TransformInternalUtility::SetLocalScale(Transform* transform, const XMFLOAT3& value)
    {
        transform->m_LocalScale = value;
    }

    void TransformInternalUtility::SetPosition(Transform* transform, const XMFLOAT3& value)
    {
        const Transform* trans = transform->GetParent();
        XMVECTOR result = XMLoadFloat3(&value);

        while (trans != nullptr)
        {
            result = XMVectorSubtract(result, trans->LoadLocalPosition());
            trans = trans->GetParent();
        }

        XMStoreFloat3(&transform->m_LocalPosition, result);
    }

    void TransformInternalUtility::SetRotation(Transform* transform, const XMFLOAT4& value)
    {
        const Transform* trans = transform->GetParent();
        XMVECTOR parentRotation = XMQuaternionIdentity();

        while (trans != nullptr)
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/directxmath/nf-directxmath-xmquaternionmultiply
            // The result represents the rotation Q1 followed by the rotation Q2 to be consistent with XMMatrixMultiply concatenation
            // since this function is typically used to concatenate quaternions that represent rotations
            // (i.e. it returns Q2*Q1).
            parentRotation = XMQuaternionMultiply(parentRotation, trans->LoadLocalRotation());
            trans = trans->GetParent();
        }

        XMVECTOR result = XMLoadFloat4(&value);
        result = XMQuaternionMultiply(result, XMQuaternionInverse(parentRotation));
        XMStoreFloat4(&transform->m_LocalRotation, result);
        SyncLocalEulerAngles(transform);
    }

    void TransformInternalUtility::SetEulerAngles(Transform* transform, const XMFLOAT3& value)
    {
        SetRotation(transform, Transform::EulerAnglesToQuaternion(value));
    }

    void TransformInternalUtility::SyncLocalEulerAngles(Transform* transform)
    {
        XMFLOAT4 localRotation = transform->m_LocalRotation;
        transform->m_LocalEulerAngles = Transform::QuaternionToEulerAngles(localRotation);
    }
}

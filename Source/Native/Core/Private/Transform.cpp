#include "Transform.h"

using namespace DirectX;

namespace march
{
    Transform::Transform()
        : m_Parent(nullptr)
        , m_LocalPosition(0, 0, 0)
        , m_LocalRotation(0, 0, 0, 1)
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
        XMStoreFloat3(&transform->m_LocalPosition, result);
    }
}

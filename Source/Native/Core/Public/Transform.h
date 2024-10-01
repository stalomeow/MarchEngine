#pragma once

#include "Component.h"
#include <DirectXMath.h>

namespace march
{
    class TransformInternalUtility;

    class Transform : public Component
    {
        friend TransformInternalUtility;

    public:
        Transform();

        Transform* GetParent() const;

        DirectX::XMFLOAT3 GetLocalPosition() const;
        DirectX::XMFLOAT4 GetLocalRotation() const;
        DirectX::XMFLOAT3 GetLocalScale() const;
        DirectX::XMFLOAT3 GetPosition() const;
        DirectX::XMFLOAT4 GetRotation() const;
        DirectX::XMFLOAT3 GetLossyScale() const;
        DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const;
        DirectX::XMFLOAT4X4 GetWorldToLocalMatrix() const;

        DirectX::XMVECTOR LoadLocalPosition() const;
        DirectX::XMVECTOR LoadLocalRotation() const;
        DirectX::XMVECTOR LoadLocalScale() const;
        DirectX::XMVECTOR LoadPosition() const;
        DirectX::XMVECTOR LoadRotation() const;
        DirectX::XMVECTOR LoadLossyScale() const;
        DirectX::XMMATRIX LoadLocalToWorldMatrix() const;
        DirectX::XMMATRIX LoadWorldToLocalMatrix() const;

    private:
        Transform* m_Parent;
        DirectX::XMFLOAT3 m_LocalPosition;
        DirectX::XMFLOAT4 m_LocalRotation; // quaternion
        DirectX::XMFLOAT3 m_LocalScale;
    };

    // C++ 侧提供给 C# 侧的接口，不要用它
    class TransformInternalUtility
    {
    public:
        static void SetParent(Transform* transform, Transform* parent) { transform->m_Parent = parent; }
        static void SetLocalPosition(Transform* transform, const DirectX::XMFLOAT3& value) { transform->m_LocalPosition = value; }
        static void SetLocalRotation(Transform* transform, const DirectX::XMFLOAT4& value) { transform->m_LocalRotation = value; }
        static void SetLocalScale(Transform* transform, const DirectX::XMFLOAT3& value) { transform->m_LocalScale = value; }
        static void SetPosition(Transform* transform, const DirectX::XMFLOAT3& value);
        static void SetRotation(Transform* transform, const DirectX::XMFLOAT4& value);
    };
}

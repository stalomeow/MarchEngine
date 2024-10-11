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
        DirectX::XMFLOAT3 GetLocalEulerAngles() const;
        DirectX::XMFLOAT3 GetLocalScale() const;
        DirectX::XMFLOAT3 GetPosition() const;
        DirectX::XMFLOAT4 GetRotation() const;
        DirectX::XMFLOAT3 GetEulerAngles() const;
        DirectX::XMFLOAT3 GetLossyScale() const;
        DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const;
        DirectX::XMFLOAT4X4 GetWorldToLocalMatrix() const;
        DirectX::XMFLOAT3 GetForward() const;
        DirectX::XMFLOAT3 GetRight() const;
        DirectX::XMFLOAT3 GetUp() const;

        DirectX::XMVECTOR LoadLocalPosition() const;
        DirectX::XMVECTOR LoadLocalRotation() const;
        DirectX::XMVECTOR LoadLocalEulerAngles() const;
        DirectX::XMVECTOR LoadLocalScale() const;
        DirectX::XMVECTOR LoadPosition() const;
        DirectX::XMVECTOR LoadRotation() const;
        DirectX::XMVECTOR LoadEulerAngles() const;
        DirectX::XMVECTOR LoadLossyScale() const;
        DirectX::XMMATRIX LoadLocalToWorldMatrix() const;
        DirectX::XMMATRIX LoadWorldToLocalMatrix() const;
        DirectX::XMVECTOR LoadForward() const;
        DirectX::XMVECTOR LoadRight() const;
        DirectX::XMVECTOR LoadUp() const;

        // 变换一个向量，受 rotation 和 scale 影响
        DirectX::XMVECTOR XM_CALLCONV TransformVector(DirectX::FXMVECTOR vector) const;

        // 变换一个方向，只受 rotation 影响
        DirectX::XMVECTOR XM_CALLCONV TransformDirection(DirectX::FXMVECTOR direction) const;

        // 变换一个点，受 rotation、scale 和 position 影响
        DirectX::XMVECTOR XM_CALLCONV TransformPoint(DirectX::FXMVECTOR point) const;

        // 逆变换一个向量，受 rotation 和 scale 影响
        DirectX::XMVECTOR XM_CALLCONV InverseTransformVector(DirectX::FXMVECTOR vector) const;

        // 逆变换一个方向，只受 rotation 影响
        DirectX::XMVECTOR XM_CALLCONV InverseTransformDirection(DirectX::FXMVECTOR direction) const;

        // 逆变换一个点，受 rotation、scale 和 position 影响
        DirectX::XMVECTOR XM_CALLCONV InverseTransformPoint(DirectX::FXMVECTOR point) const;

        static DirectX::XMFLOAT4 EulerAnglesToQuaternion(DirectX::XMFLOAT3 eulerAngles);
        static DirectX::XMFLOAT3 QuaternionToEulerAngles(DirectX::XMFLOAT4 quaternion);

    private:
        Transform* m_Parent;
        DirectX::XMFLOAT3 m_LocalPosition;
        DirectX::XMFLOAT4 m_LocalRotation; // quaternion
        DirectX::XMFLOAT3 m_LocalEulerAngles; // in degrees
        DirectX::XMFLOAT3 m_LocalScale;
    };

    // C++ 侧提供给 C# 侧的接口，不要用它
    class TransformInternalUtility
    {
    public:
        static void SetParent(Transform* transform, Transform* parent);
        static void SetLocalPosition(Transform* transform, const DirectX::XMFLOAT3& value);
        static void SetLocalRotation(Transform* transform, const DirectX::XMFLOAT4& value);
        static void SetLocalRotationWithoutSyncEulerAngles(Transform* transform, const DirectX::XMFLOAT4& value);
        static void SetLocalEulerAngles(Transform* transform, const DirectX::XMFLOAT3& value);
        static void SetLocalEulerAnglesWithoutSyncRotation(Transform* transform, const DirectX::XMFLOAT3& value);
        static void SetLocalScale(Transform* transform, const DirectX::XMFLOAT3& value);
        static void SetPosition(Transform* transform, const DirectX::XMFLOAT3& value);
        static void SetRotation(Transform* transform, const DirectX::XMFLOAT4& value);
        static void SetEulerAngles(Transform* transform, const DirectX::XMFLOAT3& value);
        static void SetLocalToWorldMatrix(Transform* transform, const DirectX::XMFLOAT4X4& value);
        static void SetWorldToLocalMatrix(Transform* transform, const DirectX::XMFLOAT4X4& value);

    private:
        static void SyncLocalEulerAngles(Transform* transform);
    };
}

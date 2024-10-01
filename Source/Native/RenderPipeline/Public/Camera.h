#pragma once

#include <DirectXMath.h>

namespace march
{
    class Camera
    {
    public:
        DirectX::XMMATRIX GetViewMatrix() const;
        DirectX::XMMATRIX GetProjectionMatrix() const;

        DirectX::XMFLOAT4X4 GetViewMatrix1() const;
        DirectX::XMFLOAT4X4 GetProjectionMatrix1() const;

    private:
        float m_Fov;
        float m_AspectRatio;
        float m_ZNear;
        float m_ZFar;
    };
}

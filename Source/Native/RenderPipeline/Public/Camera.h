#pragma once

#include "Component.h"
#include <DirectXMath.h>

namespace march
{
    class Camera : public Component
    {
    public:
        DirectX::XMFLOAT4X4 GetViewMatrix() const;
        DirectX::XMFLOAT4X4 GetProjectionMatrix() const;

        DirectX::XMMATRIX LoadViewMatrix() const;
        DirectX::XMMATRIX LoadProjectionMatrix() const;

    private:
        float m_Fov;
        float m_AspectRatio;
        float m_ZNear;
        float m_ZFar;
    };
}

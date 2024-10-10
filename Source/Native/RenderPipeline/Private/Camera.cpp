#include "Camera.h"
#include "Transform.h"
#include "Display.h"
#include <algorithm>

#undef max

using namespace DirectX;

namespace march
{
    std::vector<Camera*> Camera::s_AllCameras{};

    Camera::Camera()
        : m_FovY(XM_PI / 6.0f) // 默认 30 度
        , m_NearZ(0.1f)
        , m_FarZ(1000.0f)
        , m_EnableWireframe(false)
        , m_EnableGizmos(false)
        , m_CustomTargetDisplay(nullptr)
    {
    }

    void Camera::OnEnable()
    {
        Component::OnEnable();
        s_AllCameras.push_back(this);
    }

    void Camera::OnDisable()
    {
        if (auto it = std::find(s_AllCameras.begin(), s_AllCameras.end(), this); it != s_AllCameras.end())
        {
            s_AllCameras.erase(it);
        }

        Component::OnDisable();
    }

    Display* Camera::GetTargetDisplay() const
    {
        return m_CustomTargetDisplay != nullptr ? m_CustomTargetDisplay : Display::GetMainDisplay();
    }

    uint32_t Camera::GetPixelWidth() const
    {
        return GetTargetDisplay()->GetPixelWidth();
    }

    uint32_t Camera::GetPixelHeight() const
    {
        return GetTargetDisplay()->GetPixelHeight();
    }

    float Camera::GetAspectRatio() const
    {
        float width = static_cast<float>(GetPixelWidth());
        float height = static_cast<float>(GetPixelHeight());
        return width / height;
    }

    bool Camera::GetEnableMSAA() const
    {
        return GetTargetDisplay()->GetEnableMSAA();
    }

    float Camera::GetVerticalFieldOfView() const
    {
        return XMConvertToDegrees(m_FovY); // 为了外面使用方便，返回角度制
    }

    float Camera::GetHorizontalFieldOfView() const
    {
        float h = tanf(m_FovY * 0.5f);
        float w = h * GetAspectRatio();
        float fovX = 2.0f * atanf(w);
        return XMConvertToDegrees(fovX); // 为了外面使用方便，返回角度制
    }

    float Camera::GetNearClipPlane() const
    {
        return m_NearZ;
    }

    float Camera::GetFarClipPlane() const
    {
        return m_FarZ;
    }

    bool Camera::GetEnableWireframe() const
    {
        return m_EnableWireframe;
    }

    bool Camera::GetEnableGizmos() const
    {
        return m_EnableGizmos;
    }

    XMFLOAT4X4 Camera::GetViewMatrix() const
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, LoadViewMatrix());
        return result;
    }

    XMFLOAT4X4 Camera::GetProjectionMatrix() const
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, LoadProjectionMatrix());
        return result;
    }

    XMMATRIX Camera::LoadViewMatrix() const
    {
        const Transform* trans = GetTransform();
        const XMVECTOR scaling = XMVectorSplatOne(); // view matrix 忽略缩放
        const XMVECTOR rotationOrigin = XMVectorZero();
        XMMATRIX result = XMMatrixIdentity();

        while (trans != nullptr)
        {
            XMVECTOR translation = trans->LoadLocalPosition();
            XMVECTOR rotation = trans->LoadLocalRotation();
            XMMATRIX mat = XMMatrixAffineTransformation(scaling, rotationOrigin, rotation, translation);

            result = XMMatrixMultiply(result, mat); // DirectX 中使用的是行向量
            trans = trans->GetParent();
        }

        return XMMatrixInverse(nullptr, result);
    }

    XMMATRIX Camera::LoadProjectionMatrix() const
    {
        return XMMatrixPerspectiveFovLH(m_FovY, GetAspectRatio(), m_NearZ, m_FarZ);
    }

    const std::vector<Camera*>& Camera::GetAllCameras()
    {
        return s_AllCameras;
    }

    void CameraInternalUtility::SetVerticalFieldOfView(Camera* camera, float value)
    {
        camera->m_FovY = XMConvertToRadians(std::clamp(value, 1.0f, 179.0f));
    }

    void CameraInternalUtility::SetHorizontalFieldOfView(Camera* camera, float value)
    {
        float fovX = XMConvertToRadians(value);
        float w = tanf(fovX * 0.5f);
        float h = w / camera->GetAspectRatio();
        camera->m_FovY = std::clamp(2.0f * atanf(h), 1.0f, 179.0f);
    }

    void CameraInternalUtility::SetNearClipPlane(Camera* camera, float value)
    {
        // 不能为 0，否则算投影矩阵时会除以 0
        camera->m_NearZ = std::clamp(value, 0.001f, camera->m_FarZ);
    }

    void CameraInternalUtility::SetFarClipPlane(Camera* camera, float value)
    {
        camera->m_FarZ = std::max(value, camera->m_NearZ);
    }

    void CameraInternalUtility::SetEnableWireframe(Camera* camera, bool value)
    {
        camera->m_EnableWireframe = value;
    }

    void CameraInternalUtility::SetEnableGizmos(Camera* camera, bool value)
    {
        camera->m_EnableGizmos = value;
    }

    void CameraInternalUtility::SetCustomTargetDisplay(Camera* camera, Display* value)
    {
        camera->m_CustomTargetDisplay = value;
    }
}

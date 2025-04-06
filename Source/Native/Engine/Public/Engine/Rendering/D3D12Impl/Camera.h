#pragma once

#include "Engine/Ints.h"
#include "Engine/Component.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <stdint.h>
#include <vector>

namespace march
{
    class Display;
    class CameraInternalUtility;

    class Camera : public Component
    {
        friend CameraInternalUtility;

    public:
        Camera();

        Display* GetTargetDisplay() const;
        uint32_t GetPixelWidth() const;
        uint32_t GetPixelHeight() const;
        float GetAspectRatio() const;
        bool GetEnableMSAA() const;

        float GetVerticalFieldOfView() const; // in degrees
        float GetHorizontalFieldOfView() const; // in degrees
        float GetNearClipPlane() const;
        float GetFarClipPlane() const;
        bool GetEnableWireframe() const;
        bool GetEnableGizmos() const;

        uint32 GetTAAFrameIndex() const;

        DirectX::XMFLOAT4X4 GetViewMatrix() const;
        DirectX::XMFLOAT4X4 GetProjectionMatrix() const;
        DirectX::XMFLOAT4X4 GetViewProjectionMatrix() const;
        DirectX::XMFLOAT4X4 GetNonJitteredProjectionMatrix() const;
        DirectX::XMFLOAT4X4 GetNonJitteredViewProjectionMatrix() const;
        DirectX::XMFLOAT4X4 GetPrevNonJitteredViewProjectionMatrix() const;

        DirectX::XMMATRIX LoadViewMatrix() const;
        DirectX::XMMATRIX LoadProjectionMatrix() const;
        DirectX::XMMATRIX LoadViewProjectionMatrix() const;
        DirectX::XMMATRIX LoadNonJitteredProjectionMatrix() const;
        DirectX::XMMATRIX LoadNonJitteredViewProjectionMatrix() const;
        DirectX::XMMATRIX LoadPrevNonJitteredViewProjectionMatrix() const;

        static DirectX::BoundingFrustum XM_CALLCONV CreateWorldFrustum(DirectX::FXMMATRIX viewMatrix, DirectX::CXMMATRIX projectionMatrix);

        // 获取世界空间的视锥体
        DirectX::BoundingFrustum GetFrustum() const;

        // 获取世界空间的视锥体
        DirectX::BoundingFrustum GetNonJitteredFrustum() const;

        void PrepareFrameData();

        static const std::vector<Camera*>& GetAllCameras();

    protected:
        void OnEnable() override;
        void OnDisable() override;

    private:
        float m_FovY; // in radians
        float m_NearZ;
        float m_FarZ;
        bool m_EnableWireframe;
        bool m_EnableGizmos;
        Display* m_CustomTargetDisplay;
        DirectX::XMFLOAT4X4 m_PrevNonJitteredViewProjectionMatrix;

        static std::vector<Camera*> s_AllCameras;
    };

    // C++ 侧提供给 C# 侧的接口，不要用它
    class CameraInternalUtility
    {
    public:
        static void SetVerticalFieldOfView(Camera* camera, float value);
        static void SetHorizontalFieldOfView(Camera* camera, float value);
        static void SetNearClipPlane(Camera* camera, float value);
        static void SetFarClipPlane(Camera* camera, float value);
        static void SetEnableWireframe(Camera* camera, bool value);
        static void SetEnableGizmos(Camera* camera, bool value);
        static void SetCustomTargetDisplay(Camera* camera, Display* value);
    };
}

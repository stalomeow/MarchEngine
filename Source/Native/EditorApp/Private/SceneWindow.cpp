#include "SceneWindow.h"
#include "WinApplication.h"
#include "Display.h"
#include "GfxDevice.h"
#include "GfxTexture.h"
#include "GfxDescriptorHeap.h"
#include <stdint.h>

#undef min

using namespace DirectX;

namespace march
{
    SceneWindow::SceneWindow()
        : m_Display(nullptr)
        , m_IsRotatingAndMoving(false)
        , m_IsPanning(false)
        , m_MouseSensitivity(0.00833f) // 鼠标相关操作需要乘灵敏度，不能乘 deltaTime
        , m_RotateDegSpeed(16.0f)
        , m_NormalMoveSpeed(8.0f)
        , m_FastMoveSpeed(24.0f)
        , m_PanSpeed(1.2f)
        , m_ZoomSpeed(10.0f)
    {
    }

    ImGuiWindowFlags SceneWindow::GetWindowFlags() const
    {
        return ImGuiWindowFlags_MenuBar;
    }

    void SceneWindow::DrawMenuBar(bool& wireframe)
    {
        if (!ImGui::BeginMenuBar())
        {
            return;
        }

        if (ImGui::RadioButton("Play", false))
        {
        }

        if (m_Display == nullptr)
        {
            ImGui::RadioButton("MSAA", false);
        }
        else
        {
            if (ImGui::RadioButton("MSAA", m_Display->GetEnableMSAA()))
            {
                m_Display->SetEnableMSAA(!m_Display->GetEnableMSAA());
            }
        }

        ImGui::Spacing();

        if (ImGui::RadioButton("Wireframe", wireframe))
        {
            wireframe = !wireframe;
        }

        ImGui::EndMenuBar();
    }

    Display* SceneWindow::UpdateDisplay()
    {
        GfxDevice* device = GetGfxDevice();
        ImVec2 size = ImGui::GetContentRegionAvail();

        uint32_t width = static_cast<uint32_t>(size.x);
        uint32_t height = static_cast<uint32_t>(size.y);

        if (m_Display == nullptr)
        {
            m_Display = std::make_unique<Display>(device, "EditorSceneView", width, height);
        }
        else if (m_Display->GetPixelWidth() != width || m_Display->GetPixelHeight() != height)
        {
            m_Display->Resize(width, height);
        }

        return m_Display.get();
    }

    void SceneWindow::DrawSceneView()
    {
        GfxDevice* device = GetGfxDevice();
        ImVec2 size = ImGui::GetContentRegionAvail();

        GfxRenderTexture* colorBuffer = m_Display->GetEnableMSAA() ? m_Display->GetResolvedColorBuffer() : m_Display->GetColorBuffer();
        GfxDescriptorTable srv = device->AllocateTransientDescriptorTable(GfxDescriptorTableType::CbvSrvUav, 1);
        srv.Copy(0, colorBuffer->GetSrvCpuDescriptorHandle());

        // TODO image 不能有 alpha
        ImGui::Image(reinterpret_cast<ImTextureID>(srv.GetCpuHandle(0).ptr), size);
    }

    void SceneWindow::TravelScene(XMFLOAT3& cameraPosition, XMFLOAT4& cameraRotation)
    {
        XMVECTOR cameraPos = XMLoadFloat3(&cameraPosition);
        XMVECTOR cameraRot = XMLoadFloat4(&cameraRotation);
        XMVECTOR cameraForward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), cameraRot);
        XMVECTOR cameraRight = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), cameraRot);
        XMVECTOR cameraUp = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), cameraRot);

        bool isWindowHovered = ImGui::IsWindowHovered();
        float mouseDeltaX = ImGui::GetIO().MouseDelta.x * m_MouseSensitivity;
        float mouseDeltaY = ImGui::GetIO().MouseDelta.y * m_MouseSensitivity;
        float mouseWheel = ImGui::GetIO().MouseWheel * m_MouseSensitivity;
        float deltaTime = GetApp().GetDeltaTime();

        // Rotate & Move
        if ((m_IsRotatingAndMoving || isWindowHovered) && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            m_IsRotatingAndMoving = true;

            float rotateSpeed = XMConvertToRadians(m_RotateDegSpeed);
            float translation = (ImGui::IsKeyDown(ImGuiMod_Shift) ? m_FastMoveSpeed : m_NormalMoveSpeed) * deltaTime;

            // 绕本地空间 X 轴旋转，再绕世界空间 Y 轴旋转
            XMVECTOR rotX = XMQuaternionRotationRollPitchYaw(mouseDeltaY * rotateSpeed, 0, 0);
            XMVECTOR rotY = XMQuaternionRotationRollPitchYaw(0, mouseDeltaX * rotateSpeed, 0);
            cameraRot = XMQuaternionMultiply(XMQuaternionMultiply(rotX, cameraRot), rotY);

            if (ImGui::IsKeyDown(ImGuiKey_W))
            {
                cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraForward, translation));
            }

            if (ImGui::IsKeyDown(ImGuiKey_S))
            {
                cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraForward, -translation));
            }

            if (ImGui::IsKeyDown(ImGuiKey_D))
            {
                cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraRight, translation));
            }

            if (ImGui::IsKeyDown(ImGuiKey_A))
            {
                cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraRight, -translation));
            }
        }
        else
        {
            m_IsRotatingAndMoving = false;
        }

        // Pan
        if ((m_IsPanning || isWindowHovered) && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            m_IsPanning = true;

            cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraRight, -mouseDeltaX * m_PanSpeed));
            cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraUp, mouseDeltaY * m_PanSpeed));
        }
        else
        {
            m_IsPanning = false;
        }

        // Zoom
        if (isWindowHovered)
        {
            cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraForward, mouseWheel * m_ZoomSpeed));
        }

        XMStoreFloat3(&cameraPosition, cameraPos);
        XMStoreFloat4(&cameraRotation, cameraRot);
    }

    void SceneWindowInternalUtility::DrawMenuBar(SceneWindow* window, bool& wireframe)
    {
        window->DrawMenuBar(wireframe);
    }

    Display* SceneWindowInternalUtility::UpdateDisplay(SceneWindow* window)
    {
        return window->UpdateDisplay();
    }

    void SceneWindowInternalUtility::DrawSceneView(SceneWindow* window)
    {
        window->DrawSceneView();
    }

    void SceneWindowInternalUtility::TravelScene(SceneWindow* window, XMFLOAT3& cameraPosition, XMFLOAT4& cameraRotation)
    {
        window->TravelScene(cameraPosition, cameraRotation);
    }

    float SceneWindowInternalUtility::GetMouseSensitivity(SceneWindow* window)
    {
        return window->m_MouseSensitivity;
    }

    void SceneWindowInternalUtility::SetMouseSensitivity(SceneWindow* window, float value)
    {
        window->m_MouseSensitivity = value;
    }

    float SceneWindowInternalUtility::GetRotateDegSpeed(SceneWindow* window)
    {
        return window->m_RotateDegSpeed;
    }

    void SceneWindowInternalUtility::SetRotateDegSpeed(SceneWindow* window, float value)
    {
        window->m_RotateDegSpeed = value;
    }

    float SceneWindowInternalUtility::GetNormalMoveSpeed(SceneWindow* window)
    {
        return window->m_NormalMoveSpeed;
    }

    void SceneWindowInternalUtility::SetNormalMoveSpeed(SceneWindow* window, float value)
    {
        window->m_NormalMoveSpeed = value;
    }

    float SceneWindowInternalUtility::GetFastMoveSpeed(SceneWindow* window)
    {
        return window->m_FastMoveSpeed;
    }

    void SceneWindowInternalUtility::SetFastMoveSpeed(SceneWindow* window, float value)
    {
        window->m_FastMoveSpeed = value;
    }

    float SceneWindowInternalUtility::GetPanSpeed(SceneWindow* window)
    {
        return window->m_PanSpeed;
    }

    void SceneWindowInternalUtility::SetPanSpeed(SceneWindow* window, float value)
    {
        window->m_PanSpeed = value;
    }

    float SceneWindowInternalUtility::GetZoomSpeed(SceneWindow* window)
    {
        return window->m_ZoomSpeed;
    }

    void SceneWindowInternalUtility::SetZoomSpeed(SceneWindow* window, float value)
    {
        window->m_ZoomSpeed = value;
    }
}

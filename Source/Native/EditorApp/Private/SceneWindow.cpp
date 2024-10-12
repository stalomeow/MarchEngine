#include "SceneWindow.h"
#include "WinApplication.h"
#include "Display.h"
#include "Camera.h"
#include "GfxDevice.h"
#include "GfxTexture.h"
#include "GfxDescriptorHeap.h"
#include "Debug.h"
#include "IconsFontAwesome6.h"
#include "EditorGUI.h"
#include <stdint.h>
#include <ImGuizmo.h>
#include <string>

#undef min
#undef max

using namespace DirectX;

namespace march
{
    // Scene Window 几个要点：
    // 1. 开多个 Scene Window 时，各种操作不能相互影响，尤其是一个 Scene Window 显示在另一个上面时
    // 2. 必须在 Scene View Image 上滚动鼠标滚轮才能移动相机
    // 3. 必须从 Scene View Image 上**开始**拖拽才能转动相机，之后鼠标可以拖出窗口
    // 4. 如果窗口正在移动，不允许操作相机
    // 5. ImGuizmo 必须要设置 ID，否则多个 Scene Window 会相互影响

    SceneWindow::SceneWindow()
        : m_EnableMSAA(true)
        , m_Display(nullptr)
        , m_MouseSensitivity(0.00833f) // 鼠标相关操作需要乘灵敏度，不能乘 deltaTime
        , m_RotateDegSpeed(16.0f)
        , m_NormalMoveSpeed(8.0f)
        , m_FastMoveSpeed(24.0f)
        , m_PanSpeed(1.5f)
        , m_ZoomSpeed(40.0f)
        , m_GizmoOperation(SceneGizmoOperation::Pan)
        , m_GizmoMode(SceneGizmoMode::Local)
        , m_GizmoSnap(false)
        , m_GizmoTranslationSnapValue(1.0f, 1.0f, 1.0f)
        , m_GizmoRotationSnapValue(15)
        , m_GizmoScaleSnapValue(1.0f)
        , m_WindowMode(SceneWindowMode::SceneView)
    {
    }

    ImGuiWindowFlags SceneWindow::GetWindowFlags() const
    {
        return base::GetWindowFlags() | ImGuiWindowFlags_MenuBar;
    }

    void SceneWindow::DrawMenuBar()
    {
        if (!ImGui::BeginMenuBar())
        {
            return;
        }

        DrawMenuGizmoModeCombo();

        ImGui::TextUnformatted("Tool");

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        if (ToggleButton(ICON_FA_HAND, "View Tool", m_GizmoOperation == SceneGizmoOperation::Pan))
        {
            m_GizmoOperation = SceneGizmoOperation::Pan;
        }

        if (ToggleButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, "Move Tool", m_GizmoOperation == SceneGizmoOperation::Translate))
        {
            m_GizmoOperation = SceneGizmoOperation::Translate;
        }

        if (ToggleButton(ICON_FA_ARROWS_ROTATE, "Rotate Tool", m_GizmoOperation == SceneGizmoOperation::Rotate))
        {
            m_GizmoOperation = SceneGizmoOperation::Rotate;
        }

        if (ToggleButton(ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER, "Scale Tool", m_GizmoOperation == SceneGizmoOperation::Scale))
        {
            m_GizmoOperation = SceneGizmoOperation::Scale;
        }

        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::TextUnformatted("Snap");

        if (IsForceGizmoSnapByKeyboardShortcut())
        {
            ImGui::BeginDisabled();
            ToggleButton(ICON_FA_MAGNET, "", true);
            ImGui::EndDisabled();
        }
        else if (ToggleButton(ICON_FA_MAGNET, "", m_GizmoSnap))
        {
            m_GizmoSnap = !m_GizmoSnap;
        }

        DrawMenuRightButtons();

        ImGui::EndMenuBar();
    }

    Display* SceneWindow::UpdateDisplay()
    {
        GfxDevice* device = GetGfxDevice();
        ImVec2 size = ImGui::GetContentRegionAvail();

        uint32_t width = std::max(static_cast<uint32_t>(size.x), 16u);
        uint32_t height = std::max(static_cast<uint32_t>(size.y), 16u);

        if (m_Display == nullptr)
        {
            m_Display = std::make_unique<Display>(device, "EditorSceneView", width, height);
            m_Display->SetEnableMSAA(m_EnableMSAA);
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

        GfxRenderTexture* colorBuffer = m_EnableMSAA ? m_Display->GetResolvedColorBuffer() : m_Display->GetColorBuffer();
        GfxDescriptorTable srv = device->AllocateTransientDescriptorTable(GfxDescriptorTableType::CbvSrvUav, 1);
        srv.Copy(0, colorBuffer->GetSrvCpuDescriptorHandle());

        // TODO image 不能有 alpha
        ImGui::Image(reinterpret_cast<ImTextureID>(srv.GetGpuHandle(0).ptr), size);
    }

    void SceneWindow::TravelScene(XMFLOAT3& cameraPosition, XMFLOAT4& cameraRotation)
    {
        if (!AllowTravellingScene())
        {
            return;
        }

        XMVECTOR cameraPos = XMLoadFloat3(&cameraPosition);
        XMVECTOR cameraRot = XMLoadFloat4(&cameraRotation);

        float mouseDeltaX = ImGui::GetIO().MouseDelta.x * m_MouseSensitivity;
        float mouseDeltaY = ImGui::GetIO().MouseDelta.y * m_MouseSensitivity;
        float mouseWheel = ImGui::GetIO().MouseWheel * m_MouseSensitivity;
        float deltaTime = GetApp().GetDeltaTime();

        bool isRotating = false;

        // Rotate
        if (IsMouseDraggingAndFromSceneViewImage(ImGuiMouseButton_Right))
        {
            isRotating = true;
            float rotateSpeed = XMConvertToRadians(m_RotateDegSpeed);

            // 绕本地空间 X 轴旋转，再绕世界空间 Y 轴旋转
            XMVECTOR rotX = XMQuaternionRotationRollPitchYaw(mouseDeltaY * rotateSpeed, 0, 0);
            XMVECTOR rotY = XMQuaternionRotationRollPitchYaw(0, mouseDeltaX * rotateSpeed, 0);
            cameraRot = XMQuaternionMultiply(XMQuaternionMultiply(rotX, cameraRot), rotY);
        }

        // 必须在 Rotate 之后计算方向
        XMVECTOR cameraForward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), cameraRot);
        XMVECTOR cameraRight = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), cameraRot);
        XMVECTOR cameraUp = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), cameraRot);

        // Move
        if (isRotating)
        {
            float translation = (ImGui::IsKeyDown(ImGuiMod_Shift) ? m_FastMoveSpeed : m_NormalMoveSpeed) * deltaTime;

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

            if (ImGui::IsKeyDown(ImGuiKey_E))
            {
                cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraUp, translation));
            }

            if (ImGui::IsKeyDown(ImGuiKey_Q))
            {
                cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraUp, -translation));
            }
        }
        else
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Q))
            {
                m_GizmoOperation = SceneGizmoOperation::Pan;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_W))
            {
                m_GizmoOperation = SceneGizmoOperation::Translate;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_E))
            {
                m_GizmoOperation = SceneGizmoOperation::Rotate;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_R))
            {
                m_GizmoOperation = SceneGizmoOperation::Scale;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_X))
            {
                m_GizmoMode = (m_GizmoMode == SceneGizmoMode::Local) ?
                    SceneGizmoMode::World : SceneGizmoMode::Local;
            }
        }

        // Zoom
        if (IsSceneViewImageHovered())
        {
            cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraForward, mouseWheel * m_ZoomSpeed));
        }

        // Pan
        if (IsMouseDraggingAndFromSceneViewImage(ImGuiMouseButton_Middle) ||
            (m_GizmoOperation == SceneGizmoOperation::Pan && IsMouseDraggingAndFromSceneViewImage(ImGuiMouseButton_Left)))
        {
            cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraRight, -mouseDeltaX * m_PanSpeed));
            cameraPos = XMVectorAdd(cameraPos, XMVectorScale(cameraUp, mouseDeltaY * m_PanSpeed));
        }

        XMStoreFloat3(&cameraPosition, cameraPos);
        XMStoreFloat4(&cameraRotation, cameraRot);
    }

    bool SceneWindow::ManipulateTransform(const Camera* camera, XMFLOAT4X4& localToWorldMatrix)
    {
        if (m_GizmoOperation == SceneGizmoOperation::Pan)
        {
            return false;
        }

        ImGuizmo::OPERATION operation;
        ImGuizmo::MODE mode;

        switch (m_GizmoOperation)
        {
        case SceneGizmoOperation::Translate:
            operation = ImGuizmo::TRANSLATE;
            break;
        case SceneGizmoOperation::Rotate:
            operation = ImGuizmo::ROTATE;
            break;
        case SceneGizmoOperation::Scale:
            operation = ImGuizmo::SCALE;
            break;
        default:
            DEBUG_LOG_ERROR("Unknown gizmo operation: %d", static_cast<int>(m_GizmoOperation));
            return false;
        }

        switch (m_GizmoMode)
        {
        case SceneGizmoMode::Local:
            mode = ImGuizmo::LOCAL;
            break;
        case SceneGizmoMode::World:
            mode = ImGuizmo::WORLD;
            break;
        default:
            DEBUG_LOG_ERROR("Unknown gizmo mode: %d", static_cast<int>(m_GizmoMode));
            return false;
        }

        ImGuizmo::SetDrawlist();

        ImRect contentRegion = GetSceneViewImageRect();
        ImGuizmo::SetRect(contentRegion.Min.x, contentRegion.Min.y, contentRegion.GetWidth(), contentRegion.GetHeight());

        XMFLOAT4X4 viewMatrix = camera->GetViewMatrix();
        XMFLOAT4X4 projMatrix = camera->GetProjectionMatrix();

        const float* view = reinterpret_cast<float*>(viewMatrix.m);
        const float* proj = reinterpret_cast<float*>(projMatrix.m);
        float* matrix = reinterpret_cast<float*>(localToWorldMatrix.m);
        const float* snap = nullptr;

        if (m_GizmoSnap || IsForceGizmoSnapByKeyboardShortcut())
        {
            switch (m_GizmoOperation)
            {
            case SceneGizmoOperation::Translate:
                snap = &m_GizmoTranslationSnapValue.x;
                break;
            case SceneGizmoOperation::Rotate:
                snap = &m_GizmoRotationSnapValue;
                break;
            case SceneGizmoOperation::Scale:
                snap = &m_GizmoScaleSnapValue;
                break;
            }
        }

        // 避免多个窗口相互干扰
        ImGuizmo::SetID(static_cast<int>(GetImGuiID())); // TODO: 换成 ImGuizmo::PushID 和 ImGuizmo::PopID，假如之后它们被公开的话
        return ImGuizmo::Manipulate(view, proj, operation, mode, matrix, nullptr, snap);
    }

    void SceneWindow::DrawWindowSettings()
    {
        EditorGUI::SeparatorText("Settings");
        EditorGUI::Space();

        if (EditorGUI::Foldout("Display", ""))
        {
            EditorGUI::Indent(2);
            if (EditorGUI::Checkbox("MSAA", "", m_EnableMSAA))
            {
                if (m_Display != nullptr)
                {
                    m_Display->SetEnableMSAA(m_EnableMSAA);
                }
            }
            EditorGUI::Unindent(2);
        }

        EditorGUI::Space();

        if (EditorGUI::Foldout("Tool", ""))
        {
            EditorGUI::Indent(2);
            EditorGUI::FloatField("Mouse Sensitivity", "", &m_MouseSensitivity);
            EditorGUI::FloatField("Rotate Speed (deg)", "", &m_RotateDegSpeed);
            EditorGUI::FloatField("Normal Move Speed", "", &m_NormalMoveSpeed);
            EditorGUI::FloatField("Fast Move Speed", "", &m_FastMoveSpeed);
            EditorGUI::FloatField("Pan Speed", "", &m_PanSpeed);
            EditorGUI::FloatField("Zoom Speed", "", &m_ZoomSpeed);
            EditorGUI::Unindent(2);
        }

        EditorGUI::Space();

        if (EditorGUI::Foldout("Snap", ""))
        {
            EditorGUI::Indent(2);
            EditorGUI::Vector3Field("Translation", "", &m_GizmoTranslationSnapValue.x);
            EditorGUI::FloatField("Rotation (deg)", "", &m_GizmoRotationSnapValue);
            EditorGUI::FloatField("Scale", "", &m_GizmoScaleSnapValue);
            EditorGUI::Unindent(2);
        }

        EditorGUI::Space();
    }

    void SceneWindow::DrawMenuGizmoModeCombo()
    {
        ImGui::TextUnformatted("Mode");

        std::string localLabel = ICON_FA_CUBE + std::string(" Local");
        std::string worldLabel = ICON_FA_GLOBE + std::string(" Global");

        float w1 = ImGui::CalcTextSize(localLabel.c_str()).x;
        float w2 = ImGui::CalcTextSize(worldLabel.c_str()).x;
        float comboWidth = std::max(w1, w2) * 2.0f + ImGui::GetStyle().FramePadding.x * 2.0f;

        int current = static_cast<int>(m_GizmoMode);
        const char* labels[] = { localLabel.c_str(), worldLabel.c_str() };
        ImGui::SetNextItemWidth(comboWidth);

        if (ImGui::Combo("##GizmoMode", &current, labels, std::size(labels)))
        {
            m_GizmoMode = static_cast<SceneGizmoMode>(current);
        }
    }

    void SceneWindow::DrawMenuRightButtons()
    {
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcToggleButtonWidth(ICON_FA_GEAR));

        if (ToggleButton(ICON_FA_GEAR, "", m_WindowMode == SceneWindowMode::Settings))
        {
            m_WindowMode = (m_WindowMode == SceneWindowMode::Settings) ?
                SceneWindowMode::SceneView : SceneWindowMode::Settings;
        }
    }

    float SceneWindow::CalcToggleButtonWidth(const std::string& name, float widthScale)
    {
        return EditorGUI::CalcButtonWidth(name.c_str()) * widthScale;
    }

    bool SceneWindow::ToggleButton(const std::string& name, const std::string& tooltip, bool isOn, float widthScale)
    {
        float width = CalcToggleButtonWidth(name, widthScale);
        ImVec2 size = ImVec2(width, ImGui::GetFrameHeight());

        bool isClicked;

        if (isOn)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            isClicked = ImGui::Button(name.c_str(), size);
            ImGui::PopStyleColor();
        }
        else
        {
            isClicked = ImGui::Button(name.c_str(), size);
        }

        if (!tooltip.empty())
        {
            ImGui::SetItemTooltip("%s", tooltip.c_str());
        }

        return isClicked;
    }

    bool SceneWindow::IsForceGizmoSnapByKeyboardShortcut()
    {
        return ImGui::IsWindowFocused() && ImGui::IsKeyDown(ImGuiMod_Ctrl);
    }

    bool SceneWindow::AllowTravellingScene()
    {
        if (IsWindowMoving())
        {
            return false;
        }

        if (ImGui::IsWindowFocused())
        {
            return true;
        }

        if (AllowFocusingWindow())
        {
            ImGui::SetWindowFocus();
            return true;
        }

        return false;
    }

    bool SceneWindow::AllowFocusingWindow()
    {
        if (IsSceneViewImageHovered())
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left, false))
            {
                return true;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right, false))
            {
                return true;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false))
            {
                return true;
            }

            float mouseWheel = ImGui::GetIO().MouseWheel;
            if (mouseWheel > 0 || mouseWheel < 0)
            {
                return true;
            }
        }

        return false;
    }

    bool SceneWindow::IsMouseDraggingAndFromSceneViewImage(ImGuiMouseButton button)
    {
        if (!ImGui::IsMouseDragging(button))
        {
            return false;
        }

        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 mouseDragDelta = ImGui::GetMouseDragDelta(button);
        ImVec2 mouseOrigin = ImVec2(mousePos.x - mouseDragDelta.x, mousePos.y - mouseDragDelta.y);
        return IsPointInsideSceneViewImage(mouseOrigin);
    }

    bool SceneWindow::IsWindowMoving()
    {
        return ImGui::GetCurrentContext()->MovingWindow == ImGui::GetCurrentWindowRead();
    }

    bool SceneWindow::IsSceneViewImageHovered()
    {
        // 光标必须在 scene view 的那张 image 上，不能是 title bar 或者 menu bar
        return ImGui::IsWindowHovered() && IsPointInsideSceneViewImage(ImGui::GetMousePos());
    }

    bool SceneWindow::IsPointInsideSceneViewImage(const ImVec2& p)
    {
        return GetSceneViewImageRect().Contains(p);
    }

    ImRect SceneWindow::GetSceneViewImageRect()
    {
        // ContentRegionRect 不考虑滚动条，不过理论上 scene view 永远没有滚动条
        return ImGui::GetCurrentWindowRead()->ContentRegionRect;
    }

    void SceneWindowInternalUtility::DrawMenuBar(SceneWindow* window)
    {
        window->DrawMenuBar();
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

    bool SceneWindowInternalUtility::ManipulateTransform(SceneWindow* window, const Camera* camera, XMFLOAT4X4& localToWorldMatrix)
    {
        return window->ManipulateTransform(camera, localToWorldMatrix);
    }

    void SceneWindowInternalUtility::DrawWindowSettings(SceneWindow* window)
    {
        window->DrawWindowSettings();
    }

    bool SceneWindowInternalUtility::GetEnableMSAA(SceneWindow* window)
    {
        return window->m_EnableMSAA;
    }

    void SceneWindowInternalUtility::SetEnableMSAA(SceneWindow* window, bool value)
    {
        window->m_EnableMSAA = value;

        if (window->m_Display != nullptr)
        {
            window->m_Display->SetEnableMSAA(value);
        }
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

    SceneGizmoOperation SceneWindowInternalUtility::GetGizmoOperation(SceneWindow* window)
    {
        return window->m_GizmoOperation;
    }

    void SceneWindowInternalUtility::SetGizmoOperation(SceneWindow* window, SceneGizmoOperation value)
    {
        window->m_GizmoOperation = value;
    }

    SceneGizmoMode SceneWindowInternalUtility::GetGizmoMode(SceneWindow* window)
    {
        return window->m_GizmoMode;
    }

    void SceneWindowInternalUtility::SetGizmoMode(SceneWindow* window, SceneGizmoMode value)
    {
        window->m_GizmoMode = value;
    }

    bool SceneWindowInternalUtility::GetGizmoSnap(SceneWindow* window)
    {
        return window->m_GizmoSnap;
    }

    void SceneWindowInternalUtility::SetGizmoSnap(SceneWindow* window, bool value)
    {
        window->m_GizmoSnap = value;
    }

    const XMFLOAT3& SceneWindowInternalUtility::GetGizmoTranslationSnapValue(SceneWindow* window)
    {
        return window->m_GizmoTranslationSnapValue;
    }

    void SceneWindowInternalUtility::SetGizmoTranslationSnapValue(SceneWindow* window, const XMFLOAT3& value)
    {
        window->m_GizmoTranslationSnapValue = value;
    }

    float SceneWindowInternalUtility::GetGizmoRotationSnapValue(SceneWindow* window)
    {
        return window->m_GizmoRotationSnapValue;
    }

    void SceneWindowInternalUtility::SetGizmoRotationSnapValue(SceneWindow* window, float value)
    {
        window->m_GizmoRotationSnapValue = value;
    }

    float SceneWindowInternalUtility::GetGizmoScaleSnapValue(SceneWindow* window)
    {
        return window->m_GizmoScaleSnapValue;
    }

    void SceneWindowInternalUtility::SetGizmoScaleSnapValue(SceneWindow* window, float value)
    {
        window->m_GizmoScaleSnapValue = value;
    }

    SceneWindowMode SceneWindowInternalUtility::GetWindowMode(SceneWindow* window)
    {
        return window->m_WindowMode;
    }

    void SceneWindowInternalUtility::SetWindowMode(SceneWindow* window, SceneWindowMode value)
    {
        window->m_WindowMode = value;
    }
}

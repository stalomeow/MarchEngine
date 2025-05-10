#pragma once

#include "Editor/EditorWindow.h"
#include <DirectXMath.h>
#include <memory>
#include "imgui.h"
#include "imgui_internal.h"

namespace march
{
    class Display;
    class Camera;
    class SceneWindowInternalUtility;

    enum class SceneGizmoOperation
    {
        Pan = 0,
        Translate = 1,
        Rotate = 2,
        Scale = 3
    };

    enum class SceneGizmoMode
    {
        Local = 0,
        World = 1
    };

    enum class SceneWindowMode
    {
        SceneView = 0,
        Settings = 1
    };

    class SceneWindow : public EditorWindow
    {
        using base = typename EditorWindow;
        friend SceneWindowInternalUtility;

    public:
        SceneWindow();
        virtual ~SceneWindow() = default;

    protected:
        ImGuiWindowFlags GetWindowFlags() const override;

    private:
        void DrawMenuBar();
        Display* UpdateDisplay();
        void DrawSceneView();
        void TravelScene(DirectX::XMFLOAT3& cameraPosition, DirectX::XMFLOAT4& cameraRotation);
        bool AllowTravellingScene();
        bool ManipulateTransform(const Camera* camera, DirectX::XMFLOAT4X4& localToWorldMatrix);
        void BeginGizmosGUI(const Camera* camera);
        void EndGizmosGUI();
        void DrawWindowSettings();
        void DrawMenuGizmoModeCombo();
        void DrawMenuRightButtons();

        static float CalcToggleButtonWidth(const std::string& name, float widthScale = 1.5f);
        static bool ToggleButton(const std::string& name, const std::string& tooltip, bool isOn, float widthScale = 1.5f);
        static bool IsForceGizmoSnapByKeyboardShortcut();
        static bool AllowFocusingWindow();
        static bool IsMouseDraggingAndFromSceneViewImage(ImGuiMouseButton button);
        static bool IsWindowMoving();
        static bool IsSceneViewImageHovered();
        static bool IsPointInsideSceneViewImage(const ImVec2& p);
        static ImRect GetSceneViewImageRect();

        ImVec2 m_LastWindowSize;

        bool m_EnableMSAA;
        std::unique_ptr<Display> m_Display;

        float m_MouseSensitivity;
        float m_RotateDegSpeed;
        float m_NormalMoveSpeed;
        float m_FastMoveSpeed;
        float m_PanSpeed;
        float m_ZoomSpeed;

        SceneGizmoOperation m_GizmoOperation;
        SceneGizmoMode m_GizmoMode;
        bool m_GizmoSnap;
        DirectX::XMFLOAT3 m_GizmoTranslationSnapValue;
        float m_GizmoRotationSnapValue;
        float m_GizmoScaleSnapValue;

        SceneWindowMode m_WindowMode;
    };

    class SceneWindowInternalUtility
    {
    public:
        static void DrawMenuBar(SceneWindow* window);
        static Display* UpdateDisplay(SceneWindow* window);
        static void DrawSceneView(SceneWindow* window);
        static void TravelScene(SceneWindow* window, DirectX::XMFLOAT3& cameraPosition, DirectX::XMFLOAT4& cameraRotation);
        static bool ManipulateTransform(SceneWindow* window, const Camera* camera, DirectX::XMFLOAT4X4& localToWorldMatrix);
        static void BeginGizmosGUI(SceneWindow* window, const Camera* camera);
        static void EndGizmosGUI(SceneWindow* window);
        static void DrawWindowSettings(SceneWindow* window);

        static bool GetEnableMSAA(SceneWindow* window);
        static void SetEnableMSAA(SceneWindow* window, bool value);
        static float GetMouseSensitivity(SceneWindow* window);
        static void SetMouseSensitivity(SceneWindow* window, float value);
        static float GetRotateDegSpeed(SceneWindow* window);
        static void SetRotateDegSpeed(SceneWindow* window, float value);
        static float GetNormalMoveSpeed(SceneWindow* window);
        static void SetNormalMoveSpeed(SceneWindow* window, float value);
        static float GetFastMoveSpeed(SceneWindow* window);
        static void SetFastMoveSpeed(SceneWindow* window, float value);
        static float GetPanSpeed(SceneWindow* window);
        static void SetPanSpeed(SceneWindow* window, float value);
        static float GetZoomSpeed(SceneWindow* window);
        static void SetZoomSpeed(SceneWindow* window, float value);
        static SceneGizmoOperation GetGizmoOperation(SceneWindow* window);
        static void SetGizmoOperation(SceneWindow* window, SceneGizmoOperation value);
        static SceneGizmoMode GetGizmoMode(SceneWindow* window);
        static void SetGizmoMode(SceneWindow* window, SceneGizmoMode value);
        static bool GetGizmoSnap(SceneWindow* window);
        static void SetGizmoSnap(SceneWindow* window, bool value);
        static const DirectX::XMFLOAT3& GetGizmoTranslationSnapValue(SceneWindow* window);
        static void SetGizmoTranslationSnapValue(SceneWindow* window, const DirectX::XMFLOAT3& value);
        static float GetGizmoRotationSnapValue(SceneWindow* window);
        static void SetGizmoRotationSnapValue(SceneWindow* window, float value);
        static float GetGizmoScaleSnapValue(SceneWindow* window);
        static void SetGizmoScaleSnapValue(SceneWindow* window, float value);
        static SceneWindowMode GetWindowMode(SceneWindow* window);
        static void SetWindowMode(SceneWindow* window, SceneWindowMode value);
    };
}

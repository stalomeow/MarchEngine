#pragma once

#include "EditorWindow.h"
#include <DirectXMath.h>
#include <memory>
#include <imgui.h>

namespace march
{
    class Display;
    class SceneWindowInternalUtility;

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
        void DrawMenuBar(bool& wireframe);
        Display* UpdateDisplay();
        void DrawSceneView();
        void TravelScene(DirectX::XMFLOAT3& cameraPosition, DirectX::XMFLOAT4& cameraRotation);
        bool IsMouseDraggingAndFromCurrentWindow(ImGuiMouseButton button) const;

        std::unique_ptr<Display> m_Display;

        float m_MouseSensitivity;
        float m_RotateDegSpeed;
        float m_NormalMoveSpeed;
        float m_FastMoveSpeed;
        float m_PanSpeed;
        float m_ZoomSpeed;
    };

    class SceneWindowInternalUtility
    {
    public:
        static void DrawMenuBar(SceneWindow* window, bool& wireframe);
        static Display* UpdateDisplay(SceneWindow* window);
        static void DrawSceneView(SceneWindow* window);
        static void TravelScene(SceneWindow* window, DirectX::XMFLOAT3& cameraPosition, DirectX::XMFLOAT4& cameraRotation);

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
    };
}

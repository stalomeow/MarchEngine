#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include <DirectXMath.h>
#include <string>
#include <stdint.h>

#undef DrawText

namespace march
{
    class Camera;

    struct Gizmos final
    {
        static bool IsGUIMode();
        static void BeginGUI(ImDrawList* drawList, const ImRect& canvasRect, const Camera* camera);
        static void EndGUI();

        static void Clear();

        static void PushMatrix(const DirectX::XMFLOAT4X4& matrix);
        static void PopMatrix();
        static void PushColor(const DirectX::XMFLOAT4& color);
        static void PopColor();

        static float GetGUIScale(const DirectX::XMFLOAT3& position); // text 不需要乘这个 scale

        static void DrawLine(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2);
        static void DrawWireArc(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT3& startDir, float radians, float radius);
        static void DrawWireDisc(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& normal, float radius);
        static void DrawWireSphere(const DirectX::XMFLOAT3& center, float radius);
        static void DrawWireCube(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size);
        static void DrawText(const DirectX::XMFLOAT3& center, const std::string& text);

        static void Render();
    };

    // 给 C# 的接口
    struct GizmosManagedOnlyAPI
    {
        static void InitResources();
        static void ReleaseResources();
    };
}

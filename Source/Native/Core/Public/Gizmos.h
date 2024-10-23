#pragma once

#include <DirectXMath.h>
#include <string>
#include <imgui.h>
#include <imgui_internal.h>

#undef DrawText

namespace march
{
    class Gizmos final
    {
    public:
        static void SetContext(const ImRect& canvasRect, const DirectX::XMFLOAT4X4& viewProjectionMatrix);
        static void DrawText(const DirectX::XMFLOAT3& centerWS, const std::string& text, const DirectX::XMFLOAT4& color);
        static void DrawWireArc(const DirectX::XMFLOAT3& centerWS, const DirectX::XMFLOAT3& normalWS, const DirectX::XMFLOAT3& startDirWS, float angleDeg, float radius, const DirectX::XMFLOAT4& color);
        static void DrawWireCircle(const DirectX::XMFLOAT3& centerWS, const DirectX::XMFLOAT3& normalWS, float radius, const DirectX::XMFLOAT4& color);
        static void DrawWireSphere(const DirectX::XMFLOAT3& centerWS, float radius, const DirectX::XMFLOAT4& color);
        static void DrawLine(const DirectX::XMFLOAT3& posWS1, const DirectX::XMFLOAT3& posWS2, const DirectX::XMFLOAT4& color);

    private:
        static ImVec2 WorldToScreenPosition(const DirectX::XMFLOAT3& posWS, bool* outVisible = nullptr);
        static ImU32 ColorToImU32(const DirectX::XMFLOAT4& color);

        static ImRect CanvasRect;
        static DirectX::XMFLOAT4X4 ViewProjectionMatrix;
    };
}

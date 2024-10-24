#pragma once

#include <DirectXMath.h>
#include <string>
#include <imgui_internal.h>
#include <stdint.h>

#undef DrawText

namespace march
{
    class RenderGraph;

    struct Gizmos final
    {
        static void Clear();

        static void PushMatrix(const DirectX::XMFLOAT4X4& matrix);
        static void PopMatrix();

        static void PushColor(const DirectX::XMFLOAT4& color);
        static void PopColor();

        static void AddLine(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2);
        static void FlushAndDrawLines();

        static void DrawWireArc(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT3& startDir, float radians, float radius);
        static void DrawWireCircle(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& normal, float radius);
        static void DrawWireSphere(const DirectX::XMFLOAT3& center, float radius);
        static void DrawWireCube(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size);

        static void InitResources();
        static void ReleaseResources();
        static void AddRenderGraphPass(RenderGraph* graph, int32_t colorTargetId, int32_t depthStencilTargetId);
    };

    struct GizmosGUI final
    {
        static void SetContext(const ImRect& canvasRect, const DirectX::XMFLOAT4X4& viewProjectionMatrix);

        static void PushMatrix(const DirectX::XMFLOAT4X4& matrix);
        static void PopMatrix();

        static void PushColor(const DirectX::XMFLOAT4& color);
        static void PopColor();

        static void DrawText(const DirectX::XMFLOAT3& center, const std::string& text);
    };
}

#include "Gizmos.h"
#include "WinApplication.h"
#include "RenderPipeline.h"
#include <math.h>

#undef DrawText

using namespace DirectX;

namespace march
{
    void Gizmos::SetContext(const ImRect& canvasRect, const XMFLOAT4X4& viewProjectionMatrix)
    {
        //CanvasRect = canvasRect;
        //ViewProjectionMatrix = viewProjectionMatrix;
    }

    void Gizmos::DrawText(const XMFLOAT3& centerWS, const std::string& text, const XMFLOAT4& color)
    {
        RenderPipeline* rp = GetApp().GetEngine()->GetRenderPipeline();
        rp->AddGizmoText(centerWS, text, color);

        //bool visible = false;
        //ImVec2 pos = WorldToScreenPosition(centerWS, &visible);

        //if (!visible)
        //{
        //    return;
        //}

        //ImDrawList* drawList = ImGui::GetWindowDrawList();
        //ImVec2 size = ImGui::CalcTextSize(text.c_str());

        //pos.x -= size.x * 0.5f;
        //pos.y -= size.y * 0.5f;

        //drawList->AddText(pos, ColorToImU32(color), text.c_str());
    }

    void Gizmos::DrawWireArc(const XMFLOAT3& centerWS, const XMFLOAT3& normalWS, const XMFLOAT3& startDirWS, float angleDeg, float radius, const XMFLOAT4& color)
    {
        // 坐标系
        XMVECTOR up = XMVector3Normalize(XMLoadFloat3(&normalWS));
        XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&startDirWS));
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
        XMVECTOR center = XMLoadFloat3(&centerWS);

        const float segmentPerRadians = 60.0f / XM_2PI;
        float rad = XMConvertToRadians(angleDeg);
        int numSegments = static_cast<int>(ceilf(fabsf(rad) * segmentPerRadians));

        RenderPipeline* rp = GetApp().GetEngine()->GetRenderPipeline();
        XMFLOAT3 prevPos = {};

        rp->BeginGizmoLineList();
        for (int i = 0; i < numSegments + 1; i++)
        {
            float sinValue = 0;
            float cosValue = 0;
            XMScalarSinCos(&sinValue, &cosValue, rad / static_cast<float>(numSegments) * i);

            // 从 forward 开始顺时针旋转
            XMVECTOR dir = XMVectorAdd(XMVectorScale(right, sinValue), XMVectorScale(forward, cosValue));

            XMFLOAT3 pos = {};
            XMStoreFloat3(&pos, XMVectorAdd(center, XMVectorScale(dir, radius)));

            if (i > 0)
            {
                rp->AddGizmoLine(prevPos, pos, color);
            }

            prevPos = pos;
        }
        rp->EndGizmoLineList();
    }

    void Gizmos::DrawWireCircle(const XMFLOAT3& centerWS, const XMFLOAT3& normalWS, float radius, const XMFLOAT4& color)
    {
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&normalWS));
        XMVECTOR rotateAxis = XMVector3Cross(up, normal);

        if (XMVectorGetX(XMVector3Length(rotateAxis)) < 0.001f)
        {
            // normal 和 up 平行，需要旋转 0 度或 180 度
            // 因为是圆环，所以可以不转
            DrawWireArc(centerWS, normalWS, XMFLOAT3(0, 0, 1), 360, radius, color);
        }
        else
        {
            XMVECTOR rotation = XMQuaternionRotationAxis(rotateAxis, XMVectorGetX(XMVector2AngleBetweenNormals(up, normal)));

            XMFLOAT3 startDir = {};
            XMStoreFloat3(&startDir, XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotation));
            DrawWireArc(centerWS, normalWS, startDir, 360, radius, color);
        }
    }

    void Gizmos::DrawWireSphere(const XMFLOAT3& centerWS, float radius, const XMFLOAT4& color)
    {
        DrawWireCircle(centerWS, XMFLOAT3(1, 0, 0), radius, color);
        DrawWireCircle(centerWS, XMFLOAT3(0, 1, 0), radius, color);
        DrawWireCircle(centerWS, XMFLOAT3(0, 0, 1), radius, color);
    }

    void Gizmos::DrawLine(const DirectX::XMFLOAT3& posWS1, const DirectX::XMFLOAT3& posWS2, const DirectX::XMFLOAT4& color)
    {
        RenderPipeline* rp = GetApp().GetEngine()->GetRenderPipeline();
        rp->BeginGizmoLineList();
        rp->AddGizmoLine(posWS1, posWS2, color);
        rp->EndGizmoLineList();
    }

    ImVec2 Gizmos::WorldToScreenPosition(const XMFLOAT3& posWS, bool* outVisible)
    {
        // XMVector3TransformCoord ignores the w component of the input vector, and uses a value of 1.0 instead.
        // The w component of the returned vector will always be 1.0.
        XMVECTOR posNDC = XMVector3TransformCoord(XMLoadFloat3(&posWS), XMLoadFloat4x4(&ViewProjectionMatrix));

        if (outVisible != nullptr)
        {
            *outVisible = XMVectorGetZ(posNDC) >= 0.0f && XMVectorGetZ(posNDC) <= 1.0f;
        }

        XMFLOAT2 posViewport = {};
        XMVECTOR half = XMVectorReplicate(0.5f);
        XMStoreFloat2(&posViewport, XMVectorMultiplyAdd(posNDC, half, half)); // NDC XY 范围 [-1, 1] 转到 [0, 1]

        float x = posViewport.x * CanvasRect.GetWidth() + CanvasRect.Min.x;
        float y = (1 - posViewport.y) * CanvasRect.GetHeight() + CanvasRect.Min.y; // NDC Y 向上，ImGui Y 向下
        return ImVec2(x, y);
    }

    ImU32 Gizmos::ColorToImU32(const XMFLOAT4& color)
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w));
    }

    ImRect Gizmos::CanvasRect = ImRect();
    XMFLOAT4X4 Gizmos::ViewProjectionMatrix = XMFLOAT4X4
    (
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
}

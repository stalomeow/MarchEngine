#include "Gizmos.h"
#include "GfxUtils.h"
#include "GfxPipelineState.h"
#include "GfxMesh.h"
#include "RenderGraph.h"
#include "Debug.h"
#include "AssetManger.h"
#include "Camera.h"
#include "Material.h"
#include "Application.h"
#include "Shader.h"
#include <memory>
#include <vector>
#include <deque>

#undef DrawText
#undef min
#undef max

using namespace DirectX;

namespace march
{
    struct GizmosVertex
    {
        XMFLOAT3 PositionWS;
        XMFLOAT4 Color;

        constexpr GizmosVertex(const XMFLOAT3& positionWS, const XMFLOAT4& color) : PositionWS(positionWS), Color(color) {}

        static const GfxInputDesc& GetInputDesc()
        {
            static GfxInputDesc desc(D3D_PRIMITIVE_TOPOLOGY_LINELIST,
                {
                    GfxInputElement(GfxSemantic::Position, DXGI_FORMAT_R32G32B32_FLOAT),
                    GfxInputElement(GfxSemantic::Color, DXGI_FORMAT_R32G32B32A32_FLOAT),
                });
            return desc;
        }
    };

    // Gizmos 由一组 LineList 构成
    static std::vector<GizmosVertex> g_LineListVertices{};
    static std::unique_ptr<GfxBasicMesh<GizmosVertex>> g_LineListMesh = nullptr;
    static asset_ptr<Shader> g_LineListShader = nullptr;
    static std::unique_ptr<Material> g_LineListMaterial = nullptr;

    // ImGui
    static size_t g_GUIModeCounter = 0;
    static ImDrawList* g_GUIDrawList = nullptr;
    static const Camera* g_GUICamera = nullptr;

    // Stacks
    static std::deque<XMFLOAT4X4> g_MatrixStack{};
    static std::deque<XMFLOAT4> g_ColorStack{};

    bool Gizmos::IsGUIMode()
    {
        return g_GUIModeCounter > 0;
    }

    void Gizmos::BeginGUI(ImDrawList* drawList, const ImRect& canvasRect, const Camera* camera)
    {
        // 保证错误调用时也能正常绘制
        g_GUIModeCounter++;

        if (g_GUIModeCounter == 1)
        {
            g_GUIDrawList = drawList;
            g_GUIDrawList->PushClipRect(canvasRect.Min, canvasRect.Max);
            g_GUICamera = camera;
        }
        else
        {
            LOG_ERROR("Gizmos is already in GUI mode");
        }
    }

    void Gizmos::EndGUI()
    {
        // 保证错误调用时也能正常绘制
        g_GUIModeCounter--;

        if (g_GUIModeCounter == 0)
        {
            g_GUIDrawList->PopClipRect();
            g_GUIDrawList = nullptr;
            g_GUICamera = nullptr;
        }
    }

    static ImRect GetGUICanvasRect()
    {
        ImVec2 min = g_GUIDrawList->GetClipRectMin();
        ImVec2 max = g_GUIDrawList->GetClipRectMax();
        return ImRect(min, max);
    }

    void Gizmos::Clear()
    {
        g_LineListVertices.clear();
        g_LineListMesh->ClearSubMeshes();
    }

    void Gizmos::PushMatrix(const XMFLOAT4X4& matrix)
    {
        g_MatrixStack.emplace_back(matrix);
    }

    void Gizmos::PopMatrix()
    {
        g_MatrixStack.pop_back();
    }

    void Gizmos::PushColor(const XMFLOAT4& color)
    {
        g_ColorStack.emplace_back(color);
    }

    void Gizmos::PopColor()
    {
        g_ColorStack.pop_back();
    }

    static XMVECTOR LoadTransformedPosition(const XMFLOAT3& position)
    {
        XMVECTOR p = XMLoadFloat3(&position);

        for (auto it = g_MatrixStack.rbegin(); it != g_MatrixStack.rend(); ++it)
        {
            XMMATRIX m = XMLoadFloat4x4(&*it);

            // XMVector3TransformCoord ignores the w component of the input vector, and uses a value of 1.0 instead.
            // The w component of the returned vector will always be 1.0.
            p = XMVector3TransformCoord(p, m);
        }

        return p;
    }

    float Gizmos::GetGUIScale(const XMFLOAT3& position)
    {
        if (!IsGUIMode())
        {
            LOG_WARNING("Gizmos::GetGUIScale should only be called in GUI mode");
            return 1.0f;
        }

        XMVECTOR p = LoadTransformedPosition(position);

        // XMVector3Transform ignores the w component of the input vector, and uses a value of 1 instead.
        // The w component of the returned vector may be non-homogeneous (!= 1.0).
        float linearDepth = XMVectorGetZ(XMVector3Transform(p, g_GUICamera->LoadViewMatrix()));
        return fmaxf(linearDepth, 0.0001f) * GetApp()->GetDisplayScale() * 0.1f;
    }

    static XMFLOAT4 GetCurrentLineListVertexColor()
    {
        if (g_ColorStack.empty())
        {
            // 白色在 sRGB 和 Linear 中都一样
            return XMFLOAT4(1, 1, 1, 1);
        }

        return GfxUtils::GetShaderColor(g_ColorStack.back());
    }

    static ImU32 GetCurrentImGuiColor()
    {
        if (g_ColorStack.empty())
        {
            return IM_COL32_WHITE;
        }

        // ImGui 的颜色不用转换空间，统一使用 sRGB
        const XMFLOAT4& c = g_ColorStack.back();
        return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w));
    }

    static void FlushLineListIfNeeded(bool force)
    {
        size_t meshVertexCount = g_LineListVertices.size();

        // Gizmos 使用 uint16_t 类型的 index，顶点数量不能超过 65535
        if (meshVertexCount >= 60000 || (force && meshVertexCount > 0))
        {
            static std::vector<uint16_t> indices{};

            while (indices.size() < meshVertexCount)
            {
                indices.push_back(static_cast<uint16_t>(indices.size()));
            }

            uint32_t vertexCount = static_cast<uint32_t>(meshVertexCount);
            g_LineListMesh->AddSubMesh(vertexCount, g_LineListVertices.data(), vertexCount, indices.data());
            g_LineListVertices.clear();
        }
    }

    static ImVec2 GetImGuiScreenPosition(const XMFLOAT3& position, bool* outVisible)
    {
        XMVECTOR p = LoadTransformedPosition(position);
        XMVECTOR posNDC = XMVector3TransformCoord(p, g_GUICamera->LoadViewProjectionMatrix());

        if (outVisible != nullptr)
        {
            *outVisible = XMVectorGetZ(posNDC) >= 0.0f && XMVectorGetZ(posNDC) <= 1.0f;
        }

        XMFLOAT2 posViewport = {};
        XMVECTOR half = XMVectorReplicate(0.5f);
        XMStoreFloat2(&posViewport, XMVectorMultiplyAdd(posNDC, half, half)); // NDC XY 范围 [-1, 1] 转到 [0, 1]

        ImRect canvas = GetGUICanvasRect();
        float x = posViewport.x * canvas.GetWidth() + canvas.Min.x;
        float y = (1 - posViewport.y) * canvas.GetHeight() + canvas.Min.y; // NDC Y 向上，ImGui Y 向下
        return ImVec2(x, y);
    }

    void Gizmos::DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2)
    {
        if (IsGUIMode())
        {
            bool visible1 = false;
            ImVec2 pos1 = GetImGuiScreenPosition(p1, &visible1);

            bool visible2 = false;
            ImVec2 pos2 = GetImGuiScreenPosition(p2, &visible2);

            if (visible1 || visible2)
            {
                g_GUIDrawList->AddLine(pos1, pos2, GetCurrentImGuiColor());
            }
        }
        else
        {
            XMFLOAT3 p1Transformed = {};
            XMFLOAT3 p2Transformed = {};
            XMStoreFloat3(&p1Transformed, LoadTransformedPosition(p1));
            XMStoreFloat3(&p2Transformed, LoadTransformedPosition(p2));

            XMFLOAT4 color = GetCurrentLineListVertexColor();
            g_LineListVertices.emplace_back(p1Transformed, color);
            g_LineListVertices.emplace_back(p2Transformed, color);
            FlushLineListIfNeeded(false);
        }
    }

    void Gizmos::DrawWireArc(const XMFLOAT3& center, const XMFLOAT3& normal, const XMFLOAT3& startDir, float radians, float radius)
    {
        XMFLOAT4X4 matrix = {};
        XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&center), XMLoadFloat3(&startDir), XMLoadFloat3(&normal));
        XMStoreFloat4x4(&matrix, XMMatrixInverse(nullptr, view));
        PushMatrix(matrix);

        const float segmentPerRadians = 60.0f / XM_2PI;
        int numSegments = static_cast<int>(ceilf(fabsf(radians) * segmentPerRadians));

        XMFLOAT3 prevPos = {};

        for (int i = 0; i < numSegments + 1; i++)
        {
            float sinValue = 0;
            float cosValue = 0;
            XMScalarSinCos(&sinValue, &cosValue, radians / static_cast<float>(numSegments) * i);

            // 顺时针旋转
            XMFLOAT3 pos(radius * sinValue, 0, radius * cosValue);
            if (i > 0) DrawLine(prevPos, pos);
            prevPos = pos;
        }

        PopMatrix();
    }

    void Gizmos::DrawWireDisc(const XMFLOAT3& center, const XMFLOAT3& normal, float radius)
    {
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&normal));
        XMVECTOR rotateAxis = XMVector3Cross(up, n);
        XMFLOAT3 startDir;

        if (XMVectorGetX(XMVector3Length(rotateAxis)) < 0.001f)
        {
            // normal 和 up 平行，需要旋转 0 度或 180 度，但圆环可以不转
            startDir = XMFLOAT3(0, 0, 1);
        }
        else
        {
            float angle = XMVectorGetX(XMVector2AngleBetweenNormals(up, n));
            XMVECTOR rotation = XMQuaternionRotationAxis(rotateAxis, angle);
            XMStoreFloat3(&startDir, XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotation));
        }

        DrawWireArc(center, normal, startDir, XM_2PI, radius);
    }

    void Gizmos::DrawWireSphere(const XMFLOAT3& center, float radius)
    {
        DrawWireDisc(center, XMFLOAT3(1, 0, 0), radius);
        DrawWireDisc(center, XMFLOAT3(0, 1, 0), radius);
        DrawWireDisc(center, XMFLOAT3(0, 0, 1), radius);
    }

    void Gizmos::DrawWireCube(const XMFLOAT3& center, const XMFLOAT3& size)
    {
        XMFLOAT3 half(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);

        XMFLOAT3 vertices[8] =
        {
            XMFLOAT3(center.x - half.x, center.y - half.y, center.z - half.z),
            XMFLOAT3(center.x + half.x, center.y - half.y, center.z - half.z),
            XMFLOAT3(center.x + half.x, center.y + half.y, center.z - half.z),
            XMFLOAT3(center.x - half.x, center.y + half.y, center.z - half.z),
            XMFLOAT3(center.x - half.x, center.y - half.y, center.z + half.z),
            XMFLOAT3(center.x + half.x, center.y - half.y, center.z + half.z),
            XMFLOAT3(center.x + half.x, center.y + half.y, center.z + half.z),
            XMFLOAT3(center.x - half.x, center.y + half.y, center.z + half.z),
        };

        DrawLine(vertices[0], vertices[1]);
        DrawLine(vertices[1], vertices[2]);
        DrawLine(vertices[2], vertices[3]);
        DrawLine(vertices[3], vertices[0]);
        DrawLine(vertices[4], vertices[5]);
        DrawLine(vertices[5], vertices[6]);
        DrawLine(vertices[6], vertices[7]);
        DrawLine(vertices[7], vertices[4]);
        DrawLine(vertices[0], vertices[4]);
        DrawLine(vertices[1], vertices[5]);
        DrawLine(vertices[2], vertices[6]);
        DrawLine(vertices[3], vertices[7]);
    }

    void Gizmos::DrawText(const XMFLOAT3& center, const std::string& text)
    {
        if (!IsGUIMode())
        {
            LOG_WARNING("Gizmos::DrawText should only be called in GUI mode");
            return;
        }

        bool visible = false;
        ImVec2 pos = GetImGuiScreenPosition(center, &visible);

        if (visible)
        {
            ImVec2 size = ImGui::CalcTextSize(text.c_str());
            pos.x -= size.x * 0.5f;
            pos.y -= size.y * 0.5f;
            g_GUIDrawList->AddText(pos, GetCurrentImGuiColor(), text.c_str());
        }
    }

    void Gizmos::InitResources()
    {
        g_LineListMesh = std::make_unique<GfxBasicMesh<GizmosVertex>>(GfxSubAllocator::PersistentUpload);
        g_LineListShader.reset("Engine/Shaders/Gizmos.shader");
        g_LineListMaterial = std::make_unique<Material>();
        g_LineListMaterial->SetShader(g_LineListShader.get());
    }

    void Gizmos::ReleaseResources()
    {
        g_LineListMesh.reset();
        g_LineListMaterial.reset();
        g_LineListShader.reset();
    }

    void Gizmos::AddRenderGraphPass(RenderGraph* graph, int32_t colorTargetId, int32_t depthStencilTargetId)
    {
        FlushLineListIfNeeded(true);

        auto builder = graph->AddPass("Gizmos");

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            // Visible part
            for (uint32_t i = 0; i < g_LineListMesh->GetSubMeshCount(); i++)
            {
                context.DrawMesh(g_LineListMesh->GetSubMeshDesc(i), g_LineListMaterial.get(), 0);
            }

            // Invisible part
            for (uint32_t i = 0; i < g_LineListMesh->GetSubMeshCount(); i++)
            {
                context.DrawMesh(g_LineListMesh->GetSubMeshDesc(i), g_LineListMaterial.get(), 1);
            }
        });
    }
}

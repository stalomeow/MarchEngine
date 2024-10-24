#include "Gizmos.h"
#include "GfxHelpers.h"
#include "RenderGraph.h"
#include "Debug.h"
#include "AssetManger.h"
#include "Material.h"
#include <memory>
#include <math.h>
#include <vector>
#include <deque>
#include <imgui.h>

#undef DrawText

using namespace DirectX;

namespace march
{
    struct Vertex
    {
        XMFLOAT3 PositionWS;
        XMFLOAT4 Color;

        constexpr Vertex(const XMFLOAT3& positionWS, const XMFLOAT4& color) : PositionWS(positionWS), Color(color) {}
    };

    static const D3D12_INPUT_ELEMENT_DESC InputDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, 0 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Gizmos 由一组 LineList 构成
    static std::vector<Vertex> g_LineListVertices{};
    static std::vector<size_t> g_LineListVertexEnds{};
    static std::deque<XMFLOAT4X4> g_LineListMatrixStack{};
    static std::deque<XMFLOAT4> g_LineListShaderColorStack{};
    static asset_ptr<Shader> g_GizmosShader = nullptr;
    static std::unique_ptr<Material> g_GizmosMaterial = nullptr;

    void Gizmos::Clear()
    {
        if (!g_LineListMatrixStack.empty())
        {
            DEBUG_LOG_WARN("Gizmos Matrix stack is not empty");
        }

        if (!g_LineListShaderColorStack.empty())
        {
            DEBUG_LOG_WARN("Gizmos Color stack is not empty");
        }

        g_LineListVertices.clear();
        g_LineListVertexEnds.clear();
        g_LineListMatrixStack.clear();
        g_LineListShaderColorStack.clear();
    }

    void Gizmos::PushMatrix(const XMFLOAT4X4& matrix)
    {
        g_LineListMatrixStack.emplace_back(matrix);
    }

    void Gizmos::PopMatrix()
    {
        g_LineListMatrixStack.pop_back();
    }

    void Gizmos::PushColor(const XMFLOAT4& color)
    {
        // 颜色在压栈时就进行空间转换，避免之后重复计算
        g_LineListShaderColorStack.emplace_back(GfxHelpers::GetShaderColor(color));
    }

    void Gizmos::PopColor()
    {
        g_LineListShaderColorStack.pop_back();
    }

    void Gizmos::AddLine(const XMFLOAT3& p1, const XMFLOAT3& p2)
    {
        XMVECTOR v1 = XMLoadFloat3(&p1);
        XMVECTOR v2 = XMLoadFloat3(&p2);

        for (auto it = g_LineListMatrixStack.rbegin(); it != g_LineListMatrixStack.rend(); ++it)
        {
            XMMATRIX m = XMLoadFloat4x4(&*it);

            // XMVector3TransformCoord ignores the w component of the input vector, and uses a value of 1.0 instead.
            // The w component of the returned vector will always be 1.0.
            v1 = XMVector3TransformCoord(v1, m);
            v2 = XMVector3TransformCoord(v2, m);
        }

        XMFLOAT3 p1Transformed = {};
        XMFLOAT3 p2Transformed = {};
        XMStoreFloat3(&p1Transformed, v1);
        XMStoreFloat3(&p2Transformed, v2);

        // 白色在 sRGB 和 Linear 中都一样
        XMFLOAT4 color = g_LineListShaderColorStack.empty() ? XMFLOAT4(1, 1, 1, 1) : g_LineListShaderColorStack.back();

        g_LineListVertices.emplace_back(p1Transformed, color);
        g_LineListVertices.emplace_back(p2Transformed, color);
    }

    void Gizmos::FlushAndDrawLines()
    {
        size_t size = g_LineListVertices.size();

        if (g_LineListVertexEnds.empty())
        {
            if (size == 0)
            {
                return;
            }
        }
        else
        {
            if (g_LineListVertexEnds.back() == size)
            {
                return;
            }
        }

        g_LineListVertexEnds.push_back(g_LineListVertices.size());
    }

    void Gizmos::DrawWireArc(const XMFLOAT3& center, const XMFLOAT3& normal, const XMFLOAT3& startDir, float radians, float radius)
    {
        XMFLOAT4X4 matrix = {};
        XMStoreFloat4x4(&matrix, XMMatrixLookToLH(XMLoadFloat3(&center), XMLoadFloat3(&startDir), XMLoadFloat3(&normal)));
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
            if (i > 0) AddLine(prevPos, pos);
            prevPos = pos;
        }

        FlushAndDrawLines();
        PopMatrix();
    }

    void Gizmos::DrawWireCircle(const XMFLOAT3& center, const XMFLOAT3& normal, float radius)
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
        DrawWireCircle(center, XMFLOAT3(1, 0, 0), radius);
        DrawWireCircle(center, XMFLOAT3(0, 1, 0), radius);
        DrawWireCircle(center, XMFLOAT3(0, 0, 1), radius);
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

        AddLine(vertices[0], vertices[1]);
        AddLine(vertices[1], vertices[2]);
        AddLine(vertices[2], vertices[3]);
        AddLine(vertices[3], vertices[0]);
        AddLine(vertices[4], vertices[5]);
        AddLine(vertices[5], vertices[6]);
        AddLine(vertices[6], vertices[7]);
        AddLine(vertices[7], vertices[4]);
        AddLine(vertices[0], vertices[4]);
        AddLine(vertices[1], vertices[5]);
        AddLine(vertices[2], vertices[6]);
        AddLine(vertices[3], vertices[7]);
        FlushAndDrawLines();
    }

    void Gizmos::InitResources()
    {
        g_GizmosShader.reset("Engine/Shaders/Gizmos.shader");
        g_GizmosMaterial = std::make_unique<Material>();
        g_GizmosMaterial->SetShader(g_GizmosShader.get());
    }

    void Gizmos::ReleaseResources()
    {
        g_GizmosMaterial.reset();
        g_GizmosShader.reset();
    }

    void Gizmos::AddRenderGraphPass(RenderGraph* graph, int32_t colorTargetId, int32_t depthStencilTargetId)
    {
        auto builder = graph->AddPass("Gizmos");

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            std::vector<MeshBufferDesc> meshes{};
            std::vector<uint16_t> indices{};
            size_t vertexOffset = 0;

            D3D12_INPUT_LAYOUT_DESC inputLayout = {};
            inputLayout.NumElements = static_cast<UINT>(std::size(InputDesc));
            inputLayout.pInputElementDescs = InputDesc;

            for (size_t i = 0; i < g_LineListVertexEnds.size(); i++)
            {
                size_t vertexEnd = g_LineListVertexEnds[i];

                if (vertexOffset == vertexEnd)
                {
                    continue;
                }

                size_t vertexCount = vertexEnd - vertexOffset;

                while (indices.size() < vertexCount)
                {
                    indices.push_back(static_cast<uint16_t>(indices.size()));
                }

                MeshBufferDesc& bufferDesc = meshes.emplace_back();
                bufferDesc.VertexBufferView = context.CreateTransientVertexBuffer(vertexCount, sizeof(Vertex), alignof(Vertex), g_LineListVertices.data() + vertexOffset);
                bufferDesc.IndexBufferView = context.CreateTransientIndexBuffer(indices.size(), indices.data());
                vertexOffset = vertexEnd;
            }

            // Visible part
            for (const MeshBufferDesc& mesh : meshes)
            {
                context.DrawMesh(&inputLayout, D3D_PRIMITIVE_TOPOLOGY_LINELIST, &mesh, g_GizmosMaterial.get(), false, 0);
            }

            // Invisible part
            for (const MeshBufferDesc& mesh : meshes)
            {
                context.DrawMesh(&inputLayout, D3D_PRIMITIVE_TOPOLOGY_LINELIST, &mesh, g_GizmosMaterial.get(), false, 1);
            }
        });
    }

    static ImRect g_GUICanvasRect = ImRect();
    static XMFLOAT4X4 g_GUIViewProjectionMatrix = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1); // Identity
    static std::deque<XMFLOAT4X4> g_GUIMatrixStack{};
    static std::deque<ImU32> g_GUIColorStack{};

    void GizmosGUI::SetContext(const ImRect& canvasRect, const XMFLOAT4X4& viewProjectionMatrix)
    {
        if (g_GUIMatrixStack.size() > 0)
        {
            DEBUG_LOG_WARN("GizmosGUI Matrix stack is not empty");
        }

        if (g_GUIColorStack.size() > 0)
        {
            DEBUG_LOG_WARN("GizmosGUI Color stack is not empty");
        }

        g_GUICanvasRect = canvasRect;
        g_GUIViewProjectionMatrix = viewProjectionMatrix;
        g_GUIMatrixStack.clear();
        g_GUIColorStack.clear();
    }

    void GizmosGUI::PushMatrix(const XMFLOAT4X4& matrix)
    {
        g_GUIMatrixStack.emplace_back(matrix);
    }

    void GizmosGUI::PopMatrix()
    {
        g_GUIMatrixStack.pop_back();
    }

    void GizmosGUI::PushColor(const XMFLOAT4& color)
    {
        // ImGui 的颜色不用转换空间，统一使用 sRGB
        g_GUIColorStack.push_back(ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w)));
    }

    void GizmosGUI::PopColor()
    {
        g_GUIColorStack.pop_back();
    }

    static ImVec2 GetImGUIScreenPosition(const XMFLOAT3& position, bool* outVisible)
    {
        XMVECTOR p = XMLoadFloat3(&position);

        for (auto it = g_GUIMatrixStack.rbegin(); it != g_GUIMatrixStack.rend(); ++it)
        {
            XMMATRIX m = XMLoadFloat4x4(&*it);

            // XMVector3TransformCoord ignores the w component of the input vector, and uses a value of 1.0 instead.
            // The w component of the returned vector will always be 1.0.
            p = XMVector3TransformCoord(p, m);
        }

        XMVECTOR posNDC = XMVector3TransformCoord(p, XMLoadFloat4x4(&g_GUIViewProjectionMatrix));

        if (outVisible != nullptr)
        {
            *outVisible = XMVectorGetZ(posNDC) >= 0.0f && XMVectorGetZ(posNDC) <= 1.0f;
        }

        XMFLOAT2 posViewport = {};
        XMVECTOR half = XMVectorReplicate(0.5f);
        XMStoreFloat2(&posViewport, XMVectorMultiplyAdd(posNDC, half, half)); // NDC XY 范围 [-1, 1] 转到 [0, 1]

        float x = posViewport.x * g_GUICanvasRect.GetWidth() + g_GUICanvasRect.Min.x;
        float y = (1 - posViewport.y) * g_GUICanvasRect.GetHeight() + g_GUICanvasRect.Min.y; // NDC Y 向上，ImGui Y 向下
        return ImVec2(x, y);
    }

    void GizmosGUI::DrawText(const XMFLOAT3& center, const std::string& text)
    {
        bool visible = false;
        ImVec2 pos = GetImGUIScreenPosition(center, &visible);

        if (!visible)
        {
            return;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 size = ImGui::CalcTextSize(text.c_str());

        pos.x -= size.x * 0.5f;
        pos.y -= size.y * 0.5f;

        ImU32 color = g_GUIColorStack.empty() ? IM_COL32_WHITE : g_GUIColorStack.back();
        drawList->AddText(pos, color, text.c_str());
    }
}

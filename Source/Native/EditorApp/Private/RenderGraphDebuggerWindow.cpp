#include "RenderGraphDebuggerWindow.h"
#include "IconsFontAwesome6.h"
#include <math.h>

namespace march
{
    // 参考
    // https://github.com/ocornut/imgui/issues/306
    // https://gist.github.com/ocornut/7e9b3ec566a333d725d4

    static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
    static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

    RenderPassNode::RenderPassNode(const std::string& name) : Name(name), Resources{}
    {
    }

    RenderPassLink::RenderPassLink(int32_t srcNodeIndex, int32_t srcResourceIndex, int32_t dstNodeIndex, int32_t dstResourceIndex)
        : SrcNodeIndex(srcNodeIndex), SrcResourceIndex(srcResourceIndex), DstNodeIndex(dstNodeIndex), DstResourceIndex(dstResourceIndex)
    {
    }

    struct RenderPassTempData
    {
        int32_t NodeIndex = -1; // pass 对应的 node 的 index
        std::unordered_map<int32_t, int32_t> InputIndexMap{};  // 输入资源 id -> node resources index
        std::unordered_map<int32_t, int32_t> OutputIndexMap{}; // 输出资源 id -> node resources index
    };

    void RenderGraphDebuggerWindow::OnGraphCompiled(const RenderGraph& graph, const std::vector<int32_t>& sortedPasses)
    {
        m_Nodes.clear();
        m_Links.clear();

        ImVec2 nextNodePos = ImVec2(40, 50);
        std::unordered_map<int32_t, RenderPassTempData> tempMap{}; // pass index -> temp data

        // 添加 node
        for (int32_t passIndex : sortedPasses)
        {
            const RenderGraphPass& pass = graph.GetPass(passIndex);
            RenderPassTempData& tempData = tempMap[passIndex];
            RenderPassNode& node = m_Nodes.emplace_back(pass.Name);
            tempData.NodeIndex = static_cast<int32_t>(m_Nodes.size() - 1);

            // 给新 node 分配位置
            if (m_NodeStates.count(pass.Name) == 0)
            {
                m_NodeStates[pass.Name].Position = nextNodePos;
                nextNodePos.x += 250;
            }

            for (int32_t i = 0; i < pass.NumColorTargets; i++)
            {
                if (!pass.ColorTargets[i].IsSet)
                {
                    continue;
                }

                node.Resources.push_back(Shader::GetIdName(pass.ColorTargets[i].Id) + " (T)");
                tempData.OutputIndexMap[pass.ColorTargets[i].Id] = static_cast<int32_t>(node.Resources.size() - 1);

                if (pass.ColorTargets[i].Load)
                {
                    tempData.InputIndexMap[pass.ColorTargets[i].Id] = static_cast<int32_t>(node.Resources.size() - 1);
                }
            }

            if (pass.DepthStencilTarget.IsSet)
            {
                node.Resources.push_back(Shader::GetIdName(pass.DepthStencilTarget.Id) + " (T)");
                tempData.OutputIndexMap[pass.DepthStencilTarget.Id] = static_cast<int32_t>(node.Resources.size() - 1);

                if (pass.DepthStencilTarget.Load)
                {
                    tempData.InputIndexMap[pass.DepthStencilTarget.Id] = static_cast<int32_t>(node.Resources.size() - 1);
                }
            }

            for (auto& kv : pass.ResourcesRead)
            {
                node.Resources.push_back(Shader::GetIdName(kv.first) + " (R)");
                tempData.InputIndexMap[kv.first] = static_cast<int32_t>(node.Resources.size() - 1);
            }

            for (auto& kv : pass.ResourcesWritten)
            {
                node.Resources.push_back(Shader::GetIdName(kv.first) + " (W)");
                tempData.OutputIndexMap[kv.first] = static_cast<int32_t>(node.Resources.size() - 1);
            }
        }

        // 添加 link
        for (int32_t passIndex : sortedPasses)
        {
            const RenderGraphPass& pass = graph.GetPass(passIndex);
            const RenderPassTempData& srcData = tempMap[passIndex];

            for (auto& kv : srcData.OutputIndexMap)
            {
                for (int32_t nextPassIndex : pass.NextPasses)
                {
                    const RenderPassTempData& dstData = tempMap[nextPassIndex];

                    if (auto it = dstData.InputIndexMap.find(kv.first); it != dstData.InputIndexMap.end())
                    {
                        m_Links.emplace_back(srcData.NodeIndex, kv.second, dstData.NodeIndex, it->second);
                        break;
                    }
                }
            }
        }
    }

    bool RenderGraphDebuggerWindow::Begin()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        bool ret = base::Begin();
        ImGui::PopStyleVar();
        return ret;
    }

    void RenderGraphDebuggerWindow::OnOpen()
    {
        base::OnOpen();
        RenderGraph::AddGraphCompiledEventListener(this);
    }

    void RenderGraphDebuggerWindow::OnClose()
    {
        RenderGraph::RemoveGraphCompiledEventListener(this);
        base::OnClose();
    }

    void RenderGraphDebuggerWindow::OnDraw()
    {
        base::OnDraw();

        DrawSidebar();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::SameLine();
        ImGui::PopStyleVar();

        DrawCanvas();
    }

    void RenderGraphDebuggerWindow::DrawSidebar()
    {
        ImVec2 totalSize = ImGui::GetContentRegionAvail();
        ImVec2 minSize = ImVec2(totalSize.x * 0.10f, totalSize.y);
        ImVec2 maxSize = ImVec2(totalSize.x * 0.20f, totalSize.y);
        ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

        if (ImGui::BeginChild("node_list", ImVec2(0, 0), ImGuiChildFlags_ResizeX | ImGuiChildFlags_AlwaysUseWindowPadding))
        {
            ImGui::TextUnformatted("Execution Order");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            for (int i = 0; i < m_Nodes.size(); i++)
            {
                const RenderPassNode& node = m_Nodes[i];

                ImGui::PushID(i);

                if (ImGui::Selectable(node.Name.c_str(), m_SelectedNodeIndex == i))
                {
                    m_SelectedNodeIndex = i;
                }

                if (ImGui::IsItemHovered())
                {
                    m_HoveredNodeIndex = i;
                }

                ImGui::PopID();
            }
        }

        ImGui::EndChild();
    }

    void RenderGraphDebuggerWindow::DrawCanvas()
    {
        ImGui::BeginGroup();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_DockingEmptyBg));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        bool child = ImGui::BeginChild("scrolling_region", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        if (child)
        {
            const ImVec2 offset = ImGui::GetCursorScreenPos() + m_ScrollPos;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            DrawGrid(drawList);

            drawList->ChannelsSplit(2);
            DrawLinks(drawList, offset);
            DrawNodes(drawList, offset);
            drawList->ChannelsMerge();

            // Scrolling
            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
            {
                m_ScrollPos = m_ScrollPos + ImGui::GetIO().MouseDelta;
            }
        }

        ImGui::EndChild();
        ImGui::EndGroup();
    }

    void RenderGraphDebuggerWindow::DrawGrid(ImDrawList* drawList) const
    {
        const float gridSize = 64.0f;
        const ImU32 gridColor = ImGui::GetColorU32(ImGuiCol_Border);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetWindowSize();

        for (float x = fmodf(m_ScrollPos.x, gridSize); x < canvasSize.x; x += gridSize)
        {
            drawList->AddLine(pos + ImVec2(x, 0.0f), pos + ImVec2(x, canvasSize.y), gridColor);
        }

        for (float y = fmodf(m_ScrollPos.y, gridSize); y < canvasSize.y; y += gridSize)
        {
            drawList->AddLine(pos + ImVec2(0.0f, y), pos + ImVec2(canvasSize.x, y), gridColor);
        }
    }

    void RenderGraphDebuggerWindow::DrawLinks(ImDrawList* drawList, const ImVec2& offset) const
    {
        const float slotRadius = 4.0f;
        const float linkThickness = 3.0f;
        const ImU32 linkColor = ImGui::GetColorU32(ImGuiCol_TextLink);

        for (const auto& link : m_Links)
        {
            ImVec2 p1 = offset + GetLinkSrcPos(link.SrcNodeIndex, link.SrcResourceIndex);
            ImVec2 p2 = offset + GetLinkDstPos(link.DstNodeIndex, link.DstResourceIndex);

            drawList->ChannelsSetCurrent(0); // Background
            drawList->AddBezierCubic(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, linkColor, linkThickness);

            drawList->ChannelsSetCurrent(1); // Foreground
            drawList->AddCircleFilled(p1, slotRadius, linkColor);
            drawList->AddCircleFilled(p2, slotRadius, linkColor);
        }
    }

    ImVec2 RenderGraphDebuggerWindow::GetLinkSrcPos(int32_t nodeIndex, int32_t resourceIndex) const
    {
        const RenderPassNodeState& state = m_NodeStates.at(m_Nodes[nodeIndex].Name);

        int32_t textLineCount = 1 + resourceIndex; // node 标题 + slotIndex 个 resource
        float offset = m_NodeWindowPadding.y + ImGui::GetTextLineHeightWithSpacing() * textLineCount;
        return ImVec2(state.Position.x + state.Size.x, state.Position.y + offset + ImGui::GetTextLineHeight() * 0.5f);
    }

    ImVec2 RenderGraphDebuggerWindow::GetLinkDstPos(int32_t nodeIndex, int32_t resourceIndex) const
    {
        const RenderPassNodeState& state = m_NodeStates.at(m_Nodes[nodeIndex].Name);

        int32_t textLineCount = 1 + resourceIndex; // node 标题 + slotIndex 个 resource
        float offset = m_NodeWindowPadding.y + ImGui::GetTextLineHeightWithSpacing() * textLineCount;
        return ImVec2(state.Position.x, state.Position.y + offset + ImGui::GetTextLineHeight() * 0.5f);
    }

    void RenderGraphDebuggerWindow::DrawNodes(ImDrawList* drawList, const ImVec2& offset)
    {
        const float nodeRounding = 4.0f;
        const ImU32 nodeColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
        const ImU32 resourceColor = ImGui::GetColorU32(ImGuiCol_TextLink);
        const ImU32 borderHoverColor = ImGui::GetColorU32(ImGuiCol_SeparatorHovered);
        const float borderHoverThickness = 2.0f;
        const ImU32 borderActiveColor = ImGui::GetColorU32(ImGuiCol_SeparatorActive);
        const float borderActiveThickness = 2.0f;

        for (int i = 0; i < m_Nodes.size(); i++)
        {
            const RenderPassNode& node = m_Nodes[i];
            RenderPassNodeState& state = m_NodeStates.at(node.Name);

            ImGui::PushID(i);

            ImVec2 minRect = offset + state.Position;
            bool oldIsAnyItemActive = ImGui::IsAnyItemActive();
            ImGui::SetCursorScreenPos(minRect + m_NodeWindowPadding);

            // Display node contents first
            drawList->ChannelsSetCurrent(1); // Foreground
            ImGui::BeginGroup(); // Lock horizontal position
            {
                // Title
                ImGui::TextUnformatted((ICON_FA_FLORIN_SIGN + std::string(" ") + node.Name).c_str());

                // Resources
                ImGui::PushStyleColor(ImGuiCol_Text, resourceColor);
                for (const std::string& res : node.Resources)
                {
                    ImGui::TextUnformatted(res.c_str());
                }
                ImGui::PopStyleColor();
            }
            ImGui::EndGroup();

            // Save the size of what we have emitted and whether any of the widgets are being used
            bool isNodeActive = (!oldIsAnyItemActive && ImGui::IsAnyItemActive());
            state.Size = ImGui::GetItemRectSize() + m_NodeWindowPadding + m_NodeWindowPadding;
            ImVec2 maxRect = minRect + state.Size;

            // Display node box
            drawList->ChannelsSetCurrent(0); // Background
            ImGui::SetCursorScreenPos(minRect);
            ImGui::InvisibleButton("node", state.Size);

            if (ImGui::IsItemHovered())
            {
                m_HoveredNodeIndex = i;
            }

            bool allowMovingNode = ImGui::IsItemActive();

            if (isNodeActive || allowMovingNode)
            {
                m_SelectedNodeIndex = i;
            }

            if (allowMovingNode && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                state.Position = state.Position + ImGui::GetIO().MouseDelta;
            }

            drawList->AddRectFilled(minRect, maxRect, nodeColor, nodeRounding);

            if (m_SelectedNodeIndex == i)
            {
                drawList->AddRect(minRect, maxRect, borderActiveColor, nodeRounding, ImDrawFlags_None, borderActiveThickness);
            }
            else if (m_HoveredNodeIndex == i)
            {
                drawList->AddRect(minRect, maxRect, borderHoverColor, nodeRounding, ImDrawFlags_None, borderHoverThickness);
            }

            ImGui::PopID();
        }
    }
}

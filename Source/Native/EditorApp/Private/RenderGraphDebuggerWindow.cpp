#include "RenderGraphDebuggerWindow.h"
#include <imgui.h>
#include <math.h>
#include <unordered_map>
#include <string>

namespace march
{
    // https://github.com/ocornut/imgui/issues/306
    // https://gist.github.com/ocornut/7e9b3ec566a333d725d4

    static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
    static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

    static std::unordered_map<std::string, ImVec2> posMap{};
    static std::unordered_map<std::string, ImVec2> sizeMap{};
    const float NODE_SLOT_RADIUS = 4.0f;
    const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

    struct Node
    {
        int ID;
        std::string Name;
        std::vector<int32_t> Inputs;
        std::vector<int32_t> Outputs;
        std::vector<int32_t> Slots;

        Node(int id, const std::string& name) : Inputs{}, Outputs{}, Slots{}
        {
            ID = id;
            Name = name;
        }

        int GetInputSlotNo(int32_t id)
        {
            auto it = std::find(Slots.begin(), Slots.end(), id);
            return it == Slots.end() ? -1 : it - Slots.begin();
        }

        int GetOutputSlotNo(int32_t id)
        {
            auto it = std::find(Slots.begin(), Slots.end(), id);
            return it == Slots.end() ? -1 : it - Slots.begin();
        }

        void AddSlot(int32_t id)
        {
            if (std::find(Slots.begin(), Slots.end(), id) == Slots.end())
            {
                Slots.push_back(id);
            }
        }

        ImVec2& GetPos() const
        {
            return posMap[Name];
        }

        ImVec2& GetSize() const
        {
            return sizeMap[Name];
        }

        ImVec2 GetInputSlotPos(int slot_no) const
        {
            const ImVec2& Pos = GetPos();
            float padding = ImGui::GetTextLineHeightWithSpacing() * (1 + slot_no) + NODE_WINDOW_PADDING.y + ImGui::GetTextLineHeight() * 0.5f;
            return ImVec2(Pos.x, Pos.y + padding);
        }

        ImVec2 GetOutputSlotPos(int slot_no) const
        {
            const ImVec2& Pos = GetPos();
            float padding = ImGui::GetTextLineHeightWithSpacing() * (1 + slot_no) + NODE_WINDOW_PADDING.y + ImGui::GetTextLineHeight() * 0.5f;
            return ImVec2(Pos.x + GetSize().x, Pos.y + padding);
        }
    };

    struct NodeLink
    {
        int InputIdx, InputSlot, OutputIdx, OutputSlot;

        NodeLink(int input_idx, int input_slot, int output_idx, int output_slot)
        {
            InputIdx = input_idx;
            InputSlot = input_slot;
            OutputIdx = output_idx;
            OutputSlot = output_slot;
        }
    };

    static std::vector<Node> nodes;
    static std::vector<NodeLink> links;

    void RenderGraphDebuggerWindow::OnGraphCompiled(const RenderGraph& graph, const std::vector<int32_t>& sortedPasses)
    {
        nodes.clear();
        links.clear();

        std::unordered_map<int32_t, int> idxMap{};

        for (int32_t index : sortedPasses)
        {
            const RenderGraphPass& pass = graph.GetPass(index);
            nodes.push_back(Node(index, pass.Name));
            idxMap[index] = nodes.size() - 1;

            if (posMap.count(pass.Name) == 0)
            {
                posMap[pass.Name] = ImVec2(40, 50);
            }

            for (auto& kv : pass.ResourcesRead)
            {
                nodes.back().Inputs.push_back(kv.first);
                nodes.back().AddSlot(kv.first);
            }

            for (auto& kv : pass.ResourcesWritten)
            {
                nodes.back().Outputs.push_back(kv.first);
                nodes.back().AddSlot(kv.first);
            }

            if (pass.HasRenderTargets)
            {
                for (size_t i = 0; i < pass.NumColorTargets; i++)
                {
                    nodes.back().Outputs.push_back(pass.ColorTargets[i]);
                    nodes.back().AddSlot(pass.ColorTargets[i]);

                    if ((pass.RenderTargetsLoadFlags & LoadFlags::DiscardColors) == LoadFlags::None)
                    {
                        nodes.back().Inputs.push_back(pass.ColorTargets[i]);
                    }
                }

                if (pass.HasDepthStencilTarget)
                {
                    nodes.back().Outputs.push_back(pass.DepthStencilTarget);
                    nodes.back().AddSlot(pass.DepthStencilTarget);

                    if ((pass.RenderTargetsLoadFlags & LoadFlags::DiscardDepthStencil) == LoadFlags::None)
                    {
                        nodes.back().Inputs.push_back(pass.DepthStencilTarget);
                    }
                }
            }
        }

        for (int32_t index : sortedPasses)
        {
            const RenderGraphPass& pass = graph.GetPass(index);
            Node& node = nodes[idxMap[index]];

            for (int32_t nextIndex : pass.NextPasses)
            {
                Node& nextNode = nodes[idxMap[nextIndex]];

                for (int32_t i = 0; i < node.Outputs.size(); i++)
                {
                    if (int slot = nextNode.GetInputSlotNo(node.Outputs[i]); slot != -1)
                    {
                        links.push_back(NodeLink(idxMap[index], node.GetOutputSlotNo(node.Outputs[i]), idxMap[nextIndex], slot));
                    }
                }
            }
        }

        for (int32_t i = 0; i < graph.GetPassCount(); i++)
        {
            const RenderGraphPass& pass = graph.GetPass(i);
            if (pass.SortState == RenderGraphPassSortState::Culled)
            {
                nodes.push_back(Node(i, pass.Name + " (Culled)"));

                if (posMap.count(nodes.back().Name) == 0)
                {
                    posMap[pass.Name] = ImVec2(40, 50);
                }
            }
        }
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

        // State
        static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
        static bool show_grid = true;
        static int node_selected = -1;

        if (nodes.empty() || links.empty())
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        // Draw a list of nodes on the left side
        int node_hovered_in_list = -1;
        int node_hovered_in_scene = -1;
        ImGui::BeginChild("node_list", ImVec2(200, 0));
        ImGui::Text("Passes");
        ImGui::Separator();
        for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
        {
            Node* node = &nodes[node_idx];
            ImGui::PushID(node->ID);
            if (ImGui::Selectable(node->Name.c_str(), node->ID == node_selected))
                node_selected = node->ID;
            if (ImGui::IsItemHovered())
            {
                node_hovered_in_list = node->ID;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginGroup();

        // Create our child canvas
        ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        ImGui::Checkbox("Show grid", &show_grid);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
        ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        ImGui::PopStyleVar(); // WindowPadding
        ImGui::PushItemWidth(120.0f);

        const ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Display grid
        if (show_grid)
        {
            ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
            float GRID_SZ = 64.0f;
            ImVec2 win_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_sz = ImGui::GetWindowSize();
            for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
                draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
            for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
                draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
        }

        // Display links
        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(0); // Background
        for (int link_idx = 0; link_idx < links.size(); link_idx++)
        {
            NodeLink* link = &links[link_idx];
            Node* node_inp = &nodes[link->InputIdx];
            Node* node_out = &nodes[link->OutputIdx];
            ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
            ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);
            draw_list->AddBezierCubic(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
        }

        // Display nodes
        for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
        {
            Node* node = &nodes[node_idx];
            ImGui::PushID(node->ID);
            ImVec2 node_rect_min = offset + node->GetPos();

            // Display node contents first
            draw_list->ChannelsSetCurrent(1); // Foreground
            bool old_any_active = ImGui::IsAnyItemActive();
            ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
            ImGui::BeginGroup(); // Lock horizontal position
            ImGui::Text("%s", node->Name.c_str());

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 160));
            for (int slot_idx = 0; slot_idx < node->Slots.size(); slot_idx++)
            {
                ImGui::TextUnformatted(Shader::GetIdName(node->Slots[slot_idx]).c_str());
            }
            ImGui::PopStyleColor();

            ImGui::EndGroup();

            // Save the size of what we have emitted and whether any of the widgets are being used
            bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
            node->GetSize() = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
            ImVec2 node_rect_max = node_rect_min + node->GetSize();

            // Display node box
            draw_list->ChannelsSetCurrent(0); // Background
            ImGui::SetCursorScreenPos(node_rect_min);
            ImGui::InvisibleButton("node", node->GetSize());
            if (ImGui::IsItemHovered())
            {
                node_hovered_in_scene = node->ID;
            }
            bool node_moving_active = ImGui::IsItemActive();
            if (node_widgets_active || node_moving_active)
                node_selected = node->ID;
            if (node_moving_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                node->GetPos() = node->GetPos() + io.MouseDelta;

            ImU32 node_bg_color = (node_hovered_in_list == node->ID || node_hovered_in_scene == node->ID || (node_hovered_in_list == -1 && node_selected == node->ID)) ? IM_COL32(75, 75, 75, 255) : IM_COL32(60, 60, 60, 255);
            draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
            draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);

            for (int slot_idx = 0; slot_idx < node->Inputs.size(); slot_idx++)
            {
                draw_list->AddCircleFilled(offset + node->GetInputSlotPos(node->GetInputSlotNo(node->Inputs[slot_idx])), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
            }

            for (int slot_idx = 0; slot_idx < node->Outputs.size(); slot_idx++)
            {
                draw_list->AddCircleFilled(offset + node->GetOutputSlotPos(node->GetOutputSlotNo(node->Outputs[slot_idx])), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
            }

            ImGui::PopID();
        }
        draw_list->ChannelsMerge();

        // Scrolling
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
            scrolling = scrolling + io.MouseDelta;

        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::EndGroup();
    }
}

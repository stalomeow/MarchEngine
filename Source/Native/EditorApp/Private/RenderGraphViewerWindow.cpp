#include "pch.h"
#include "Editor/RenderGraphViewerWindow.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/ImGui/IconsFontAwesome6.h"
#include <imgui.h>
#include <DirectXMath.h>

using namespace DirectX;

namespace march
{
    void RenderGraphViewerWindow::OnGraphCompiled(const std::vector<RenderGraphPass>& passes, const RenderGraphResourceManager* resourceManager)
    {
        m_Passes.clear();
        m_Resources.clear();

        for (size_t resourceIndex = 0; resourceIndex < resourceManager->GetNumResources(); resourceIndex++)
        {
            ResourceData& data = m_Resources.emplace_back();
            data.Name = ShaderUtils::GetStringFromId(resourceManager->GetResourceId(resourceIndex));
            data.IsExternal = resourceManager->IsExternalResource(resourceIndex);

            if (auto lifetime = resourceManager->GetLifetimePassIndexRange(resourceIndex))
            {
                data.HasLifetime = true;
                data.LifetimeMinIndex = lifetime->first;
                data.LifetimeMaxIndex = lifetime->second;
            }
            else
            {
                data.HasLifetime = false;
            }
        }

        for (size_t passIndex = 0; passIndex < passes.size(); passIndex++)
        {
            const RenderGraphPass& pass = passes[passIndex];

            PassData& data = m_Passes.emplace_back();
            data.Name = pass.Name;
            data.IsCulled = pass.IsCulled;
            data.IsAsyncCompute = pass.IsAsyncCompute;

            if (pass.IsCulled)
            {
                data.Tooltip = "Pass is culled and won't be executed";
            }
            else if (pass.IsAsyncCompute)
            {
                data.Tooltip = "Pass will be asynchronously executed";
            }
            else if (pass.PassIndexToWait)
            {
                data.Tooltip = StringUtils::Format("This is the deadline for '{}', by which it must be completed", passes[*pass.PassIndexToWait].Name);
                m_Passes[*pass.PassIndexToWait].AsyncComputeDeadlinePass = passIndex;
            }
            else
            {
                data.Tooltip = "";
            }

            for (size_t resourceIndex : pass.ResourcesIn)
            {
                m_Resources[resourceIndex].PassAccessFlags[passIndex] |= ResourceAccessFlags::Read;
            }

            for (size_t resourceIndex : pass.ResourcesOut)
            {
                m_Resources[resourceIndex].PassAccessFlags[passIndex] |= ResourceAccessFlags::Write;
            }
        }
    }

    bool RenderGraphViewerWindow::Begin()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        bool ret = base::Begin();
        ImGui::PopStyleVar();
        return ret;
    }

    void RenderGraphViewerWindow::OnOpen()
    {
        base::OnOpen();
        RenderGraph::AddGraphCompiledEventListener(this);
    }

    void RenderGraphViewerWindow::OnClose()
    {
        RenderGraph::RemoveGraphCompiledEventListener(this);
        base::OnClose();
    }

    void RenderGraphViewerWindow::OnDraw()
    {
        base::OnDraw();

        DrawSidebar();

        if (m_Passes.empty())
        {
            return;
        }

        // Table 和 Sidebar 在同一行
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::SameLine();
        ImGui::PopStyleVar();

        constexpr ImGuiTableFlags tableFlags
            = ImGuiTableFlags_SizingFixedFit
            | ImGuiTableFlags_ScrollX
            | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_BordersInner
            | ImGuiTableFlags_BordersOuterV
            | ImGuiTableFlags_HighlightHoveredColumn
            | ImGuiTableFlags_Resizable;
        constexpr ImGuiTableColumnFlags columnFlags
            = ImGuiTableColumnFlags_AngledHeader
            | ImGuiTableColumnFlags_WidthFixed
            | ImGuiTableColumnFlags_NoHeaderWidth;
        constexpr int numFrozenColumns = 1;
        constexpr int numFrozenRows = 2;

        int numColumns = static_cast<int>(m_Passes.size() + 1); // 最前面加一列显示资源名
        if (ImGui::BeginTable("RenderGraphTable", numColumns, tableFlags))
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_None, 200); // 额外的一列显示资源名
            for (int i = 0; i < m_Passes.size(); i++)
            {
                ImGui::TableSetupColumn(m_Passes[i].Name.c_str(), columnFlags);
            }

            ImGui::TableSetupScrollFreeze(numFrozenColumns, numFrozenRows);

            // Draw angled headers for all columns with the ImGuiTableColumnFlags_AngledHeader flag.
            ImGui::PushStyleVar(ImGuiStyleVar_TableAngledHeadersAngle, XMConvertToRadians(45));
            ImGui::TableAngledHeadersRow();
            ImGui::PopStyleVar();

            // Draw headers
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            for (int i = 0; i < ImGui::TableGetColumnCount(); i++)
            {
                if (!ImGui::TableSetColumnIndex(i))
                {
                    continue;
                }

                ImGui::PushID(i);

                // 如果当前 pass 有 tooltip，则在 column 中间显示一个省略号
                if (int passIndex = i - 1; passIndex >= 0 && !m_Passes[passIndex].Tooltip.empty())
                {
                    constexpr const char* ellipsis = ICON_FA_ELLIPSIS;

                    float columnWidth = ImGui::GetColumnWidth();
                    float textWidth = ImGui::CalcTextSize(ellipsis).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
                    ImGui::TableHeader(ellipsis);
                    ImGui::SetItemTooltip(m_Passes[passIndex].Tooltip.c_str());
                }
                else
                {
                    ImGui::TableHeader("");
                }

                ImGui::PopID();
            }

            // 绘制资源行
            for (int row = 0; row < m_Resources.size(); row++)
            {
                const ResourceData& res = m_Resources[row];

                ImGui::PushID(row);
                ImGui::TableNextRow();

                for (int column = 0; column < numColumns; column++)
                {
                    if (!ImGui::TableSetColumnIndex(column))
                    {
                        continue;
                    }

                    ImGui::PushID(column);

                    if (column == 0)
                    {
                        // 显示资源名
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted(res.Name.c_str());
                    }
                    else
                    {
                        size_t passIndex = static_cast<size_t>(column) - 1;

                        if (res.HasLifetime && passIndex >= res.LifetimeMinIndex && passIndex <= res.LifetimeMaxIndex)
                        {
                            ResourceAccessFlags accessFlags = ResourceAccessFlags::None;

                            if (auto it = res.PassAccessFlags.find(passIndex); it != res.PassAccessFlags.end())
                            {
                                accessFlags = it->second;
                            }

                            DrawAccessSquare(accessFlags);
                        }
                    }

                    ImGui::PopID();
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderGraphViewerWindow::DrawAccessSquare(ResourceAccessFlags accessFlags)
    {
        constexpr ImU32 green = IM_COL32(169, 209, 54, 255);
        constexpr ImU32 red = IM_COL32(255, 93, 69, 255);
        constexpr ImU32 gray = IM_COL32(125, 125, 125, 255);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float size = ImGui::GetFrameHeight();
        const char* tooltip;

        if (accessFlags == ResourceAccessFlags::ReadWrite)
        {
            // 左上绿色
            drawList->AddTriangleFilled(
                pos,
                ImVec2(pos.x + size, pos.y),
                ImVec2(pos.x, pos.y + size),
                green);

            // 右下红色
            drawList->AddTriangleFilled(
                ImVec2(pos.x + size, pos.y),
                ImVec2(pos.x + size, pos.y + size),
                ImVec2(pos.x, pos.y + size),
                red);

            tooltip = "Read/Write access to this resource";
        }
        else
        {
            ImU32 color;

            if (accessFlags == ResourceAccessFlags::Read)
            {
                color = green;
                tooltip = "Read access to this resource";
            }
            else if (accessFlags == ResourceAccessFlags::Write)
            {
                color = red;
                tooltip = "Write access to this resource";
            }
            else
            {
                color = gray;
                tooltip = "Resource is alive but not used by this pass";
            }

            drawList->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), color);
        }

        ImGui::Dummy(ImVec2(size, size));
        ImGui::SetItemTooltip(tooltip);
    }

    void RenderGraphViewerWindow::DrawSidebar()
    {
        const ImVec2 totalSize = ImGui::GetContentRegionAvail();
        const ImVec2 minSize = ImVec2(totalSize.x * 0.20f, totalSize.y);
        const ImVec2 maxSize = ImVec2(totalSize.x * 0.50f, totalSize.y);
        const ImVec2 defaultSize = ImVec2(totalSize.x * 0.25f, totalSize.y);
        ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

        if (ImGui::BeginChild("Sidebar", defaultSize, ImGuiChildFlags_ResizeX | ImGuiChildFlags_AlwaysUseWindowPadding))
        {
            if (ImGui::CollapsingHeader("Resource List"))
            {
                for (int i = 0; i < m_Resources.size(); i++)
                {
                    const ResourceData& res = m_Resources[i];

                    ImGui::PushID(i);

                    if (ImGui::TreeNodeEx(res.Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        ImGui::BulletText(StringUtils::Format("External: {}", res.IsExternal).c_str());
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
            }

            if (ImGui::CollapsingHeader("Pass List"))
            {
                for (int i = 0; i < m_Passes.size(); i++)
                {
                    const PassData& pass = m_Passes[i];

                    ImGui::PushID(i);

                    if (ImGui::TreeNodeEx(pass.Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        ImGui::BulletText(StringUtils::Format("Culled: {}", pass.IsCulled).c_str());
                        ImGui::BulletText(StringUtils::Format("Async Compute: {}", pass.IsAsyncCompute).c_str());

                        if (pass.IsAsyncCompute && pass.AsyncComputeDeadlinePass)
                        {
                            size_t deadline = *pass.AsyncComputeDeadlinePass;
                            ImGui::BulletText(StringUtils::Format("Deadline: '{}'", m_Passes[deadline].Name).c_str());
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
            }
        }

        ImGui::EndChild();
    }
}

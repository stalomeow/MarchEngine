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

            if (pass.IsCulled)
            {
                data.Status = PassStatus::Culled;
            }
            else if (pass.IsAsyncCompute)
            {
                data.Status = PassStatus::AsyncCompute;
            }
            else if (pass.PassIndexToWait)
            {
                data.Status = PassStatus::Deadline;
                data.DeadlineOwnerPassName = m_Passes[*pass.PassIndexToWait].Name;
            }
            else
            {
                data.Status = PassStatus::Normal;
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

        if (m_Passes.empty())
        {
            return;
        }

        constexpr ImGuiTableFlags tableFlags
            = ImGuiTableFlags_SizingFixedFit
            | ImGuiTableFlags_ScrollX
            | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_BordersInner
            | ImGuiTableFlags_PadOuterX
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

                if (int passIndex = i - 1; passIndex < 0)
                {
                    // 不是 pass 列
                    ImGui::TableHeader("");
                }
                else
                {
                    const PassData& pass = m_Passes[passIndex];
                    char* passIcon;
                    std::string tooltip;

                    switch (pass.Status)
                    {
                    case PassStatus::Culled:
                        passIcon = ICON_FA_XMARK;
                        tooltip = "Pass is culled and won't be executed";
                        break;
                    case PassStatus::AsyncCompute:
                        passIcon = ICON_FA_ARROWS_TURN_RIGHT;
                        tooltip = "Pass will be executed asynchronously";
                        break;
                    case PassStatus::Deadline:
                        passIcon = ICON_FA_HOURGLASS_END;
                        tooltip = StringUtils::Format("This is the deadline for '{}', by which it must be completed", pass.DeadlineOwnerPassName);
                        break;
                    default:
                        passIcon = ICON_FA_ARROW_RIGHT_LONG;
                        tooltip = "";
                        break;
                    }

                    float columnWidth = ImGui::GetColumnWidth();
                    float textWidth = ImGui::CalcTextSize(passIcon).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
                    ImGui::TableHeader(passIcon);

                    // 显示 pass 的详细信息
                    if (ImGui::BeginItemTooltip())
                    {
                        ImGui::TextUnformatted(pass.Name.c_str());

                        if (!tooltip.empty())
                        {
                            ImGui::BulletText(tooltip.c_str());
                        }

                        ImGui::EndTooltip();
                    }
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

                        // 显示资源的详细信息
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::TextUnformatted(res.Name.c_str());
                            ImGui::BulletText(StringUtils::Format("External: {}", res.IsExternal).c_str());
                            ImGui::EndTooltip();
                        }
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
}

#include "pch.h"
#include "MemoryProfilerWindow.h"
#include "Engine/Memory/MemoryManager.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Misc/PlatformUtils.h"
#include "Engine/Debug.h"
#include "imgui.h"

namespace march
{
    void MemoryProfilerWindow::OnDraw()
    {
        if (ImGui::CollapsingHeader("System", ImGuiTreeNodeFlags_DefaultOpen))
        {
            size_t physicalSize = PlatformUtils::GetProcessPhysicalMemorySizeInBytes(PlatformUtils::GetCurrentProcessHandle());
            ImGui::BulletText("%s", StringUtils::Format("Physical Memory: {}", StringUtils::FormatSize(physicalSize)).c_str());
        }

        if (ImGui::CollapsingHeader("Native Memory Manager", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginTable("MemoryLabelTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("Label");
                ImGui::TableSetupColumn("Size");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < static_cast<size_t>(MemoryLabel::_Count); i++)
                {
                    MemoryLabel label = static_cast<MemoryLabel>(i);
                    size_t size = MemoryManager::GetAllocatedSizeInBytes(label);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(StringUtils::ToString(label).c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(StringUtils::FormatSize(size).c_str());
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();

            if (ImGui::Button("Log Active Allocations"))
            {
                for (const MemoryAllocation& alloc : MemoryManager::GetActiveAllocations())
                {
                    LOG_INFO("Alloc: Ptr={}, Size={}, Alignment={}, Label={}, Location={}({})",
                        alloc.Pointer,
                        StringUtils::FormatSize(alloc.SizeInBytes),
                        alloc.Alignment,
                        alloc.Label,
                        alloc.File,
                        alloc.Line);
                }
            }
        }
    }
}

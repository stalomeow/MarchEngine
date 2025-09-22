#include "pch.h"
#include "MemoryProfilerWindow.h"
#include "Engine/Memory/MemoryManager.h"
#include "Engine/Misc/StringUtils.h"
#include "imgui.h"

namespace march
{
    void MemoryProfilerWindow::OnDraw()
    {
        ImGui::Text("Default: %s", StringUtils::FormatSize(MemoryManager::GetAllocatedSizeInBytes(MemoryLabel::Default)).c_str());
        ImGui::Text("ImGui: %s", StringUtils::FormatSize(MemoryManager::GetAllocatedSizeInBytes(MemoryLabel::Default)).c_str());

        if (ImGui::Button("Log Active Allocations"))
        {
            MemoryManager::LogActiveAllocations(false);
        }
    }
}

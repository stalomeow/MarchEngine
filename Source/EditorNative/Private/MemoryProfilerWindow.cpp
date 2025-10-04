#include "pch.h"
#include "MemoryProfilerWindow.h"
#include "Engine/Memory/MemoryManager.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Debug.h"
#include "imgui.h"

namespace march
{
    void MemoryProfilerWindow::OnDraw()
    {
        for (size_t i = 0; i < static_cast<size_t>(MemoryLabel::_Count); i++)
        {
            MemoryLabel label = static_cast<MemoryLabel>(i);
            size_t size = MemoryManager::GetAllocatedSizeInBytes(label);
            ImGui::TextUnformatted(StringUtils::Format("{}: {}", label, StringUtils::FormatSize(size)).c_str());
        }

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

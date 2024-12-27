#include "pch.h"
#include "Editor/DescriptorHeapDebuggerWindow.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxDescriptor.h"
#include "Engine/Application.h"
#include "Engine/StringUtils.h"
#include <imgui.h>
#include <stdint.h>

namespace march
{
    void DescriptorHeapDebuggerWindow::OnDraw()
    {
        base::OnDraw();

        GfxDevice* device = GetGfxDevice();
        DrawHeapInfo("CBV, SRV, UAV", reinterpret_cast<GfxOnlineViewDescriptorAllocator*>(device->GetOnlineViewDescriptorAllocator()->GetCurrentAllocator()));
    }

    void DescriptorHeapDebuggerWindow::DrawHeapInfo(const std::string& name, GfxOnlineViewDescriptorAllocator* allocator)
    {
        if (!ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            return;
        }

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = 50.0f; // 固定高度

        const uint64_t currentFrame = GetApp()->GetFrameCount();
        const UINT capacity = allocator->GetNumMaxDescriptors();
        const float columnWidth = width / static_cast<float>(capacity);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 动态区域用绿色表示
        drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + capacity * columnWidth, p.y + height), IM_COL32(0, 255, 0, 80));

        uint32_t front = allocator->GetFront();
        uint32_t rear = allocator->GetRear();

        if (front < rear)
        {
            float x0 = p.x + front * columnWidth;
            float x1 = p.x + rear * columnWidth;

            ImU32 color = IM_COL32(255, 0, 0, 255);
            drawList->AddRectFilled(ImVec2(x0, p.y), ImVec2(x1, p.y + height), color);
        }
        else if (front > rear)
        {
            float x0 = p.x + rear * columnWidth;
            float x1 = p.x + front * columnWidth;

            ImU32 color = IM_COL32(255, 0, 0, 255);
            drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(x0, p.y + height), color);
            drawList->AddRectFilled(ImVec2(x1, p.y), ImVec2(p.x + width, p.y + height), color);
        }

        int descriptorCount = static_cast<int>((rear + capacity - front) % capacity);

        // 让 ImGui 知道这个区域是有内容的
        ImGui::Dummy(ImVec2(width, height));

        float descriptorUsage = descriptorCount / static_cast<float>(capacity) * 100;
        std::string label1 = StringUtils::Format("Capacity: %d / %d (%.2f%% Used)", descriptorCount, static_cast<int>(capacity), descriptorUsage);
        ImGui::TextUnformatted(label1.c_str());

        ImGui::TreePop();
    }
}

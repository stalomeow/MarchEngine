#include "DescriptorHeapDebuggerWindow.h"
#include "GfxDevice.h"
#include "GfxDescriptorHeap.h"
#include "Application.h"
#include "StringUtility.h"
#include <imgui.h>
#include <stdint.h>

namespace march
{
    void DescriptorHeapDebuggerWindow::OnDraw()
    {
        base::OnDraw();

        GfxDevice* device = GetGfxDevice();
        DrawHeapInfo("CBV, SRV, UAV", device->GetViewDescriptorTableAllocator());
        ImGui::Spacing();
        DrawHeapInfo("Sampler", device->GetSamplerDescriptorTableAllocator());
    }

    void DescriptorHeapDebuggerWindow::DrawHeapInfo(const std::string& name, GfxDescriptorTableAllocator* allocator)
    {
        if (!ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            return;
        }

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = 50.0f; // 固定高度

        const uint64_t currentFrame = GetApp()->GetFrameCount();
        const UINT dynamicCapacity = allocator->GetDynamicDescriptorCapacity();
        const UINT staticCount = allocator->GetStaticDescriptorCount();
        const UINT capacity = dynamicCapacity + staticCount;
        const float columnWidth = width / static_cast<float>(capacity);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 动态区域用绿色表示，静态区域用灰色表示
        drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + dynamicCapacity * columnWidth, p.y + height), IM_COL32(0, 255, 0, 80));
        drawList->AddRectFilled(ImVec2(p.x + dynamicCapacity * columnWidth, p.y), ImVec2(p.x + width, p.y + height), IM_COL32(192, 192, 192, 80));

        uint32_t front = allocator->GetDynamicFront();
        uint32_t rear = allocator->GetDynamicRear();

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

        int dynamicDescriptorCount = static_cast<int>((rear + dynamicCapacity - front) % dynamicCapacity);

        // 让 ImGui 知道这个区域是有内容的
        ImGui::Dummy(ImVec2(width, height));

        float dynamicDescriptorUsage = dynamicDescriptorCount / static_cast<float>(dynamicCapacity) * 100;
        std::string label1 = StringUtility::Format("Dynamic Capacity: %d / %d (%.2f%% Used)", dynamicDescriptorCount, static_cast<int>(dynamicCapacity), dynamicDescriptorUsage);
        std::string label2 = StringUtility::Format("Static Count: %d", staticCount);

        float startX = ImGui::GetCursorPosX();
        ImGui::TextUnformatted(label1.c_str());
        ImGui::SameLine();
        ImVec2 label2Size = ImGui::CalcTextSize(label2.c_str());
        ImGui::SetCursorPosX(startX + width - label2Size.x);
        ImGui::TextUnformatted(label2.c_str());

        ImGui::TreePop();
    }
}

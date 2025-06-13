#include "pch.h"
#include "GraphicsDebuggerWindow.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Profiling/FrameDebugger.h"
#include "Engine/Profiling/NsightAftermath.h"
#include "Engine/Application.h"
#include "Engine/Misc/StringUtils.h"
#include "imgui.h"
#include <string>

namespace march
{
    static void DrawKeyValueText(const std::string& key, const std::string& value)
    {
        ImGui::BeginDisabled();
        ImGui::TextUnformatted((key + ":").c_str());
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::TextUnformatted(value.c_str());
    }

    void GraphicsDebuggerWindow::OnDraw()
    {
        uint32_t fps = GetApp()->GetFPS();
        DrawKeyValueText("FPS", StringUtils::Format("{} / {:.1f} ms", fps, 1000.0f / static_cast<float>(fps)));

        ImGui::Separator();

        DrawKeyValueText("API", "DirectX 12");
        DrawOnlineViewDescriptorAllocatorInfo();
        DrawOnlineSamplerDescriptorAllocatorInfo();

        ImGui::Separator();

        if (std::optional<FrameDebuggerPlugin> plugin = FrameDebugger::GetLoadedPlugin())
        {
            DrawKeyValueText("Frame Debugger", StringUtils::Format("{}", *plugin));
        }
        else
        {
            DrawKeyValueText("Frame Debugger", "None");
        }

        switch (NsightAftermath::GetState())
        {
        case NsightAftermathState::Uninitialized:
            DrawKeyValueText("Nsight Aftermath", "Not Loaded");
            break;
        case NsightAftermathState::MinimalFeatures:
            DrawKeyValueText("Nsight Aftermath", "Minimal Features");
            break;
        case NsightAftermathState::FullFeatures:
            DrawKeyValueText("Nsight Aftermath", "Full Features");
            break;
        }
    }

    static void DrawOnlineDescriptorAllocatorUsageText(GfxOnlineDescriptorAllocator* allocator, const std::string& name)
    {
        uint32_t capacity = allocator->GetNumMaxDescriptors();
        uint32_t usedCount = allocator->GetNumAllocatedDescriptors();
        float usage = usedCount / static_cast<float>(capacity) * 100;
        DrawKeyValueText(name, StringUtils::Format("{:.1f}% Used", usage));
    }

    static void DrawOnlineViewDescriptorAllocatorRingBuffer(GfxOnlineViewDescriptorAllocator* allocator)
    {
        uint32_t capacity = allocator->GetNumMaxDescriptors();
        uint32_t front = allocator->GetFront();
        uint32_t rear = allocator->GetRear();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = 10.0f * GetApp()->GetDisplayScale(); // 固定高度
        const float columnWidth = width / static_cast<float>(capacity);

        // 空闲区域用绿色表示
        drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + capacity * columnWidth, p.y + height), IM_COL32(0, 255, 0, 80));

        // 忙碌区域用红色表示
        constexpr ImU32 busyColor = IM_COL32(255, 0, 0, 255);

        if (front < rear)
        {
            float x0 = p.x + front * columnWidth;
            float x1 = p.x + rear * columnWidth;
            drawList->AddRectFilled(ImVec2(x0, p.y), ImVec2(x1, p.y + height), busyColor);
        }
        else if (front > rear)
        {
            float x0 = p.x + rear * columnWidth;
            float x1 = p.x + front * columnWidth;
            drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(x0, p.y + height), busyColor);
            drawList->AddRectFilled(ImVec2(x1, p.y), ImVec2(p.x + width, p.y + height), busyColor);
        }

        // 让 ImGui 知道这个区域是有内容的
        ImGui::Dummy(ImVec2(width, height));
    }

    void GraphicsDebuggerWindow::DrawOnlineViewDescriptorAllocatorInfo()
    {
        GfxOnlineViewDescriptorAllocator* allocator = static_cast<GfxOnlineViewDescriptorAllocator*>(
            GetGfxDevice()->GetOnlineViewDescriptorAllocator()->GetCurrentAllocator());

        DrawOnlineDescriptorAllocatorUsageText(allocator, "Online CBV SRV UAV Heap");
        //DrawOnlineViewDescriptorAllocatorRingBuffer(allocator);
    }

    void GraphicsDebuggerWindow::DrawOnlineSamplerDescriptorAllocatorInfo()
    {
        GfxOnlineSamplerDescriptorAllocator* allocator = static_cast<GfxOnlineSamplerDescriptorAllocator*>(
            GetGfxDevice()->GetOnlineSamplerDescriptorAllocator()->GetCurrentAllocator());

        DrawOnlineDescriptorAllocatorUsageText(allocator, "Online Sampler Heap");
    }
}

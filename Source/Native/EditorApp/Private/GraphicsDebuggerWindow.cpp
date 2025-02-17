#include "pch.h"
#include "Editor/GraphicsDebuggerWindow.h"
#include "Editor/EditorGUI.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Profiling/FrameDebugger.h"
#include "Engine/Misc/StringUtils.h"
#include <imgui.h>
#include <string>

namespace march
{
    void GraphicsDebuggerWindow::OnDraw()
    {
        if (ImGui::CollapsingHeader("Configuration"))
        {
            ImGui::BulletText(StringUtils::Format("Reversed Z: {}", GfxSettings::UseReversedZBuffer).c_str());

            if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Linear)
            {
                ImGui::BulletText("Color Space: Linear");
            }
            else if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Gamma)
            {
                ImGui::BulletText("Color Space: Gamma");
            }
            else
            {
                ImGui::BulletText("Color Space: Unknown");
            }
        }

        if (ImGui::CollapsingHeader("Online Descriptor Allocator"))
        {
            DrawOnlineViewDescriptorAllocatorInfo();
            DrawOnlineSamplerDescriptorAllocatorInfo();
        }

        if (ImGui::CollapsingHeader("Frame Debugger"))
        {
            if (std::optional<FrameDebuggerPlugin> plugin = FrameDebugger::GetLoadedPlugin())
            {
                EditorGUI::LabelField("Plugin", "", StringUtils::Format("{}", *plugin));
            }
            else
            {
                EditorGUI::LabelField("Plugin", "", "None");
            }

            ImGui::BeginDisabled(!FrameDebugger::IsCaptureAvailable());

            if (int v = FrameDebugger::NumFramesToCapture; EditorGUI::IntField("Num Frames To Capture", "", &v, 1, 1, 10))
            {
                FrameDebugger::NumFramesToCapture = v;
            }

            ImGui::EndDisabled();
        }

        if (ImGui::CollapsingHeader("Shadow"))
        {
            EditorGUI::IntField("Depth Bias", "", &GfxSettings::ShadowDepthBias);
            EditorGUI::FloatField("Slope Bias", "", &GfxSettings::ShadowSlopeScaledDepthBias);
            EditorGUI::FloatField("Bias Clamp", "", &GfxSettings::ShadowDepthBiasClamp);
        }
    }

    static void DrawOnlineDescriptorAllocatorUsageBulletText(GfxOnlineDescriptorAllocator* allocator, const std::string& name)
    {
        uint32_t capacity = allocator->GetNumMaxDescriptors();
        uint32_t usedCount = allocator->GetNumAllocatedDescriptors();
        float usage = usedCount / static_cast<float>(capacity) * 100;
        ImGui::BulletText("%s", StringUtils::Format("{} Capacity: {} / {} ({:.2f}% Used)", name, usedCount, capacity, usage).c_str());
    }

    void GraphicsDebuggerWindow::DrawOnlineViewDescriptorAllocatorInfo()
    {
        GfxOnlineViewDescriptorAllocator* allocator = static_cast<GfxOnlineViewDescriptorAllocator*>(
            GetGfxDevice()->GetOnlineViewDescriptorAllocator()->GetCurrentAllocator());

        DrawOnlineDescriptorAllocatorUsageBulletText(allocator, "View");

        // 可视化 ring buffer
        ImGui::Indent();
        {
            uint32_t capacity = allocator->GetNumMaxDescriptors();
            uint32_t front = allocator->GetFront();
            uint32_t rear = allocator->GetRear();

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float width = ImGui::GetContentRegionAvail().x;
            const float height = 50.0f; // 固定高度
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
        ImGui::Unindent();
    }

    void GraphicsDebuggerWindow::DrawOnlineSamplerDescriptorAllocatorInfo()
    {
        GfxOnlineSamplerDescriptorAllocator* allocator = static_cast<GfxOnlineSamplerDescriptorAllocator*>(
            GetGfxDevice()->GetOnlineSamplerDescriptorAllocator()->GetCurrentAllocator());

        DrawOnlineDescriptorAllocatorUsageBulletText(allocator, "Sampler");
    }
}

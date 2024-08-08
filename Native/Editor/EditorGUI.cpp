#include "Editor/EditorGUI.h"
#include <utility>
#include <imgui_stdlib.h>

namespace dx12demo
{
    void EditorGUI::PrefixLabel(const std::string& label)
    {
        ImGui::TextUnformatted(label.c_str());
        ImGui::SameLine(MaxLabelWidth);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    }

    bool EditorGUI::FloatField(const std::string& label, float v[1], float speed, float min, float max)
    {
        PrefixLabel(label);
        return ImGui::DragFloat(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector2Field(const std::string& label, float v[2], float speed, float min, float max)
    {
        PrefixLabel(label);
        return ImGui::DragFloat2(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector3Field(const std::string& label, float v[3], float speed, float min, float max)
    {
        PrefixLabel(label);
        return ImGui::DragFloat3(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector4Field(const std::string& label, float v[4], float speed, float min, float max)
    {
        PrefixLabel(label);
        return ImGui::DragFloat4(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::ColorField(const std::string& label, float v[4])
    {
        PrefixLabel(label);
        return ImGui::ColorEdit4(("##" + label).c_str(), v, ImGuiColorEditFlags_Float);
    }

    bool EditorGUI::FloatSliderField(const std::string& label, float v[1], float min, float max)
    {
        PrefixLabel(label);
        return ImGui::SliderFloat(("##" + label).c_str(), v, min, max);
    }

    bool EditorGUI::CollapsingHeader(const std::string& label, bool defaultOpen)
    {
        return ImGui::CollapsingHeader(label.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    }

    bool EditorGUI::Combo(const std::string& label, int* currentItem, const std::string& itemsSeparatedByZeros)
    {
        PrefixLabel(label);
        return ImGui::Combo(("##" + label).c_str(), currentItem, itemsSeparatedByZeros.c_str());
    }

    bool EditorGUI::CenterButton(const std::string& label, float width)
    {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth = ImGui::CalcTextSize(label.c_str()).x;
        auto padding = (min(windowWidth, width) - textWidth) * 0.5f;
        auto cursorPosX = (windowWidth - max(textWidth, width)) * 0.5f;

        ImGui::SetCursorPosX(max(0, cursorPosX));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(max(0, padding), ImGui::GetStyle().FramePadding.y));
        bool ret = ImGui::Button(label.c_str(), ImVec2(0, 0));
        ImGui::PopStyleVar();
        return ret;
    }

    void EditorGUI::Space()
    {
        ImGui::Spacing();
    }

    void EditorGUI::SeparatorText(const std::string& label)
    {
        ImGui::SeparatorText(label.c_str());
    }

    bool EditorGUI::TextField(const std::string& label, std::string& text)
    {
        PrefixLabel(label);
        return ImGui::InputText(("##" + label).c_str(), &text);
    }

    bool EditorGUI::Checkbox(const std::string& label, bool& value)
    {
        PrefixLabel(label);
        return ImGui::Checkbox(("##" + label).c_str(), &value);
    }

    void EditorGUI::BeginDisabled(bool disabled)
    {
        ImGui::BeginDisabled(disabled);
    }

    void EditorGUI::EndDisabled()
    {
        ImGui::EndDisabled();
    }

    void EditorGUI::LabelField(const std::string& label1, const std::string& label2)
    {
        PrefixLabel(label1);
        ImGui::LabelText(("##" + label1).c_str(), "%s", label2.c_str());
    }
}

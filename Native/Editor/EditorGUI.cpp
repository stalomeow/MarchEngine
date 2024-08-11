#include "Editor/EditorGUI.h"
#include <utility>
#include <imgui_stdlib.h>

namespace dx12demo
{
    namespace
    {
        bool IsHiddenLabel(const std::string& label)
        {
            return label.size() >= 2 && label[0] == '#' && label[1] == '#';
        }
    }

    void EditorGUI::PrefixLabel(const std::string& label, const std::string& tooltip)
    {
        ImGui::TextUnformatted(label.c_str());

        if (!tooltip.empty())
        {
            ImGui::SetItemTooltip(tooltip.c_str());
        }

        ImGui::SameLine(MaxLabelWidth);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    }

    bool EditorGUI::FloatField(const std::string& label, const std::string& tooltip, float v[1], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::DragFloat(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector2Field(const std::string& label, const std::string& tooltip, float v[2], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::DragFloat2(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat2(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector3Field(const std::string& label, const std::string& tooltip, float v[3], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::DragFloat3(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat3(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector4Field(const std::string& label, const std::string& tooltip, float v[4], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::DragFloat4(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat4(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::ColorField(const std::string& label, const std::string& tooltip, float v[4])
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::ColorEdit4(label.c_str(), v, ImGuiColorEditFlags_Float);
        }

        PrefixLabel(label, tooltip);
        return ImGui::ColorEdit4(("##" + label).c_str(), v, ImGuiColorEditFlags_Float);
    }

    bool EditorGUI::FloatSliderField(const std::string& label, const std::string& tooltip, float v[1], float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::SliderFloat(label.c_str(), v, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::SliderFloat(("##" + label).c_str(), v, min, max);
    }

    bool EditorGUI::CollapsingHeader(const std::string& label, bool defaultOpen)
    {
        return ImGui::CollapsingHeader(label.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    }

    bool EditorGUI::Combo(const std::string& label, const std::string& tooltip, int* currentItem, const std::string& itemsSeparatedByZeros)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::Combo(label.c_str(), currentItem, itemsSeparatedByZeros.c_str());
        }

        PrefixLabel(label, tooltip);
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

    bool EditorGUI::TextField(const std::string& label, const std::string& tooltip, std::string& text)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::InputText(label.c_str(), &text);
        }

        PrefixLabel(label, tooltip);
        return ImGui::InputText(("##" + label).c_str(), &text);
    }

    bool EditorGUI::Checkbox(const std::string& label, const std::string& tooltip, bool& value)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::Checkbox(label.c_str(), &value);
        }

        PrefixLabel(label, tooltip);
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

    void EditorGUI::LabelField(const std::string& label1, const std::string& tooltip, const std::string& label2)
    {
        if (IsHiddenLabel(label1))
        {
            ImGui::LabelText(label1.c_str(), "%s", label2.c_str());
            return;
        }

        PrefixLabel(label1, tooltip);
        ImGui::LabelText(("##" + label1).c_str(), "%s", label2.c_str());
    }

    void EditorGUI::PushID(const std::string& id)
    {
        ImGui::PushID(id.c_str());
    }

    void EditorGUI::PushID(int id)
    {
        ImGui::PushID(id);
    }

    void EditorGUI::PopID()
    {
        ImGui::PopID();
    }

    bool EditorGUI::Foldout(const std::string& label)
    {
        // 加上 ImGuiTreeNodeFlags_NoTreePushOnOpen 就不用调用 TreePop() 了
        return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen);
    }

    void EditorGUI::Indent(std::uint32_t count)
    {
        if (count <= 0)
        {
            return;
        }

        float spacing = ImGui::GetStyle().IndentSpacing;
        ImGui::Indent(static_cast<float>(count * spacing));
    }

    void EditorGUI::Unindent(std::uint32_t count)
    {
        if (count <= 0)
        {
            return;
        }

        float spacing = ImGui::GetStyle().IndentSpacing;
        ImGui::Unindent(static_cast<float>(count * spacing));
    }

    void EditorGUI::SameLine(float offsetFromStartX, float spacing)
    {
        ImGui::SameLine(offsetFromStartX, spacing);
    }

    DirectX::XMFLOAT2 EditorGUI::GetContentRegionAvail()
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        return { avail.x, avail.y };
    }

    void EditorGUI::SetNextItemWidth(float width)
    {
        ImGui::SetNextItemWidth(width);
    }

    void EditorGUI::Separator()
    {
        ImGui::Separator();
    }

    bool EditorGUI::BeginPopup(const std::string& id)
    {
        return ImGui::BeginPopup(id.c_str());
    }

    void EditorGUI::EndPopup()
    {
        ImGui::EndPopup();
    }

    bool EditorGUI::MenuItem(const std::string& label, bool selected, bool enabled)
    {
        return ImGui::MenuItem(label.c_str(), nullptr, selected, enabled);
    }

    bool EditorGUI::BeginMenu(const std::string& label, bool enabled)
    {
        return ImGui::BeginMenu(label.c_str(), enabled);
    }

    void EditorGUI::EndMenu()
    {
        ImGui::EndMenu();
    }

    void EditorGUI::OpenPopup(const std::string& id)
    {
        ImGui::OpenPopup(id.c_str());
    }

    bool EditorGUI::FloatRangeField(const std::string& label, const std::string& tooltip, float& currentMin, float& currentMax, float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            return ImGui::DragFloatRange2(label.c_str(), &currentMin, &currentMax, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloatRange2(("##" + label).c_str(), &currentMin, &currentMax, speed, min, max);
    }

    bool EditorGUI::BeginTreeNode(const std::string& label, bool isLeaf, bool openOnArrow, bool openOnDoubleClick, bool selected, bool showBackground, bool defaultOpen, bool spanWidth)
    {
        ImGuiTreeNodeFlags flags = 0;

        if (isLeaf)             flags |= ImGuiTreeNodeFlags_Leaf;
        if (openOnArrow)        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        if (openOnDoubleClick)  flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selected)           flags |= ImGuiTreeNodeFlags_Selected;
        if (showBackground)     flags |= ImGuiTreeNodeFlags_Framed;
        if (defaultOpen)        flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (spanWidth)          flags |= ImGuiTreeNodeFlags_SpanFullWidth; // ImGuiTreeNodeFlags_SpanAvailWidth;

        return ImGui::TreeNodeEx(label.c_str(), flags);
    }

    void EditorGUI::EndTreeNode()
    {
        ImGui::TreePop();
    }

    bool EditorGUI::IsItemClicked()
    {
        return ImGui::IsItemClicked();
    }

    bool EditorGUI::BeginPopupContextWindow()
    {
        return ImGui::BeginPopupContextWindow();
    }

    bool EditorGUI::BeginPopupContextItem()
    {
        return ImGui::BeginPopupContextItem();
    }
}

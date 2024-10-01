#include "EditorGUI.h"
#include "GfxDevice.h"
#include "GfxDescriptorHeap.h"
#include "GfxTexture.h"
#include <utility>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

namespace march
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
        float width = ImGui::GetContentRegionMax().x;
        float fieldWidth = min(max(width - MinLabelWidth, 0), MaxFieldWidth);
        float labelWidth = max(width - fieldWidth, 0);

        ImVec2 pos = ImGui::GetCursorPos();

        ImGui::PushTextWrapPos(labelWidth - ImGui::GetStyle().FramePadding.x);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopTextWrapPos();

        if (!tooltip.empty())
        {
            ImGui::SetItemTooltip(tooltip.c_str());
        }

        ImGui::SetCursorPos({ labelWidth, pos.y });
        ImGui::SetNextItemWidth(fieldWidth);
    }

    bool EditorGUI::IntField(const std::string& label, const std::string& tooltip, int v[1], float speed, int min, int max)
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
            return ImGui::DragInt(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragInt(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::FloatField(const std::string& label, const std::string& tooltip, float v[1], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
            return ImGui::DragFloat(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector2Field(const std::string& label, const std::string& tooltip, float v[2], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
            return ImGui::DragFloat2(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat2(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector3Field(const std::string& label, const std::string& tooltip, float v[3], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
            return ImGui::DragFloat3(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat3(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector4Field(const std::string& label, const std::string& tooltip, float v[4], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
            return ImGui::DragFloat4(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat4(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::ColorField(const std::string& label, const std::string& tooltip, float v[4])
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
            return ImGui::ColorEdit4(label.c_str(), v, ImGuiColorEditFlags_Float);
        }

        PrefixLabel(label, tooltip);
        return ImGui::ColorEdit4(("##" + label).c_str(), v, ImGuiColorEditFlags_Float);
    }

    bool EditorGUI::FloatSliderField(const std::string& label, const std::string& tooltip, float v[1], float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
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
            ImGui::SetNextItemWidth(-1);
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
            // https://github.com/ocornut/imgui/issues/623
            ImGui::SetNextItemWidth(-1);
            return ImGui::InputText(label.c_str(), &text);
        }

        PrefixLabel(label, tooltip);
        return ImGui::InputText(("##" + label).c_str(), &text);
    }

    bool EditorGUI::Checkbox(const std::string& label, const std::string& tooltip, bool& value)
    {
        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
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
            ImGui::SetNextItemWidth(-1);
            ImGui::LabelText(label1.c_str(), "%s", label2.c_str());
            return;
        }

        PrefixLabel(label1, tooltip);

        ImGui::PushID(label1.c_str());
        ImGui::TextUnformatted(label2.c_str());
        ImGui::PopID();
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

    bool EditorGUI::Foldout(const std::string& label, const std::string& tooltip)
    {
        // 缩短箭头两边的空白
        ImVec2 framePadding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, framePadding.y });

        // 加上 ImGuiTreeNodeFlags_NoTreePushOnOpen 就不用调用 TreePop() 了
        bool result = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
        ImGui::PopStyleVar();

        if (!tooltip.empty())
        {
            ImGui::SetItemTooltip(tooltip.c_str());
        }

        return result;
    }

    bool EditorGUI::FoldoutClosable(const std::string& label, const std::string& tooltip, bool* pVisible)
    {
        // 修改自 bool ImGui::CollapsingHeader(const char* label, bool* p_visible, ImGuiTreeNodeFlags flags)

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        if (pVisible && !*pVisible)
            return false;

        // 缩短箭头两边的空白
        ImVec2 framePadding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, framePadding.y });

        ImGuiID id = window->GetID(label.c_str());
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader & ~ImGuiTreeNodeFlags_Framed; // 不要背景
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth; // 撑满宽度
        if (pVisible)
            flags |= ImGuiTreeNodeFlags_AllowOverlap | (ImGuiTreeNodeFlags)ImGuiTreeNodeFlags_ClipLabelForTrailingButton;
        bool is_open = ImGui::TreeNodeBehavior(id, flags, label.c_str());
        if (pVisible != NULL)
        {
            // Create a small overlapping close button
            // FIXME: We can evolve this into user accessible helpers to add extra buttons on title bars, headers, etc.
            // FIXME: CloseButton can overlap into text, need find a way to clip the text somehow.
            ImGuiContext& g = *GImGui;
            ImGuiLastItemData last_item_backup = g.LastItemData;
            float button_size = g.FontSize;
            float button_x = ImMax(g.LastItemData.Rect.Min.x, g.LastItemData.Rect.Max.x /* - g.Style.FramePadding.x */ - button_size);
            float button_y = g.LastItemData.Rect.Min.y; // + g.Style.FramePadding.y;
            ImGuiID close_button_id = ImGui::GetIDWithSeed("#CLOSE", NULL, id);
            if (ImGui::CloseButton(close_button_id, ImVec2(button_x, button_y)))
                *pVisible = false;
            g.LastItemData = last_item_backup;
        }

        ImGui::PopStyleVar();

        if (!tooltip.empty())
        {
            ImGui::SetItemTooltip(tooltip.c_str());
        }

        return is_open;
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
            ImGui::SetNextItemWidth(-1);
            return ImGui::DragFloatRange2(label.c_str(), &currentMin, &currentMax, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloatRange2(("##" + label).c_str(), &currentMin, &currentMax, speed, min, max);
    }

    bool EditorGUI::BeginTreeNode(const std::string& label, bool isLeaf, bool openOnArrow, bool openOnDoubleClick, bool selected, bool showBackground, bool defaultOpen, bool spanWidth)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;

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

    bool EditorGUI::IsItemClicked(ImGuiMouseButton button, bool ignorePopup)
    {
        // return ImGui::IsItemClicked(button);

        ImGuiHoveredFlags hoveredFlags = ImGuiHoveredFlags_None;
        if (ignorePopup) hoveredFlags |= ImGuiHoveredFlags_AllowWhenBlockedByPopup;

        // return ImGui::IsMouseClicked(button) && ImGui::IsItemHovered(hoveredFlags);

        // https://github.com/ocornut/imgui/issues/7879
        // 实现 down -> release 再触发 click 的效果
        return ImGui::IsMouseReleased(button) && !ImGui::IsMouseDragPastThreshold(button) && ImGui::IsItemHovered(hoveredFlags);
    }

    bool EditorGUI::BeginPopupContextWindow()
    {
        return ImGui::BeginPopupContextWindow();
    }

    bool EditorGUI::BeginPopupContextItem(const std::string& id)
    {
        const char* idStr = id.empty() ? nullptr : id.c_str();
        return ImGui::BeginPopupContextItem(idStr);
    }

    void EditorGUI::DrawTexture(GfxTexture* texture)
    {
        GfxDescriptorTable table = GetGfxDevice()->AllocateTransientDescriptorTable(GfxDescriptorTableType::CbvSrvUav, 1);
        table.Copy(0, texture->GetSrvCpuDescriptorHandle());
        auto id = (ImTextureID)table.GetGpuHandle(0).ptr;
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImVec2 size = { region.x, static_cast<float>(texture->GetHeight()) / texture->GetWidth() * region.x };
        ImGui::Image(id, size);
    }

    bool EditorGUI::Button(const std::string& label)
    {
        return ImGui::Button(label.c_str());
    }

    void EditorGUI::BeginGroup()
    {
        ImGui::BeginGroup();
    }

    void EditorGUI::EndGroup()
    {
        ImGui::EndGroup();
    }

    float EditorGUI::CalcButtonWidth(const std::string& label)
    {
        return ImGui::CalcTextSize(label.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    }

    DirectX::XMFLOAT2 EditorGUI::GetItemSpacing()
    {
        ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        return { spacing.x, spacing.y };
    }

    float EditorGUI::GetCursorPosX()
    {
        return ImGui::GetCursorPosX();
    }

    void EditorGUI::SetCursorPosX(float localX)
    {
        ImGui::SetCursorPosX(localX);
    }

    bool EditorGUI::BeginAssetTreeNode(const std::string& label, const std::string& assetPath, bool isLeaf, bool openOnArrow, bool openOnDoubleClick, bool selected, bool showBackground, bool defaultOpen, bool spanWidth)
    {
        bool result = BeginTreeNode(label, isLeaf, openOnArrow, openOnDoubleClick, selected, showBackground, defaultOpen, spanWidth);

        // https://github.com/ocornut/imgui/issues/1931

        if (!assetPath.empty() && ImGui::BeginDragDropSource())
        {
            // 显示 tooltip
            ImGui::TextUnformatted(assetPath.c_str());

            // 拷贝 payload 时，需要加上最后一个 '\0'
            ImGui::SetDragDropPayload(DragDropPayloadType_AssetPath, assetPath.c_str(), assetPath.size() + 1);
            ImGui::EndDragDropSource();
        }

        return result;
    }

    bool EditorGUI::MarchObjectField(const std::string& label, const std::string& tooltip, const std::string& type, std::string& persistentPath, MarchObjectState currentObjectState)
    {
        std::string displayValue;

        switch (currentObjectState)
        {
        case MarchObjectState::Null:
            displayValue = "None (" + type + ")";
            break;
        case MarchObjectState::Persistent:
            displayValue = persistentPath;
            break;
        case MarchObjectState::Temporary:
            displayValue = "Runtime Object (" + type + ")";
            break;
        default:
            displayValue = "Unknown";
            break;
        }

        if (IsHiddenLabel(label))
        {
            ImGui::SetNextItemWidth(-1);
            ImGui::LabelText(label.c_str(), "%s", displayValue.c_str());
        }
        else
        {
            PrefixLabel(label, tooltip);

            ImGui::PushID(label.c_str());
            ImGui::TextUnformatted(displayValue.c_str());
            ImGui::PopID();
        }

        bool isChanged = false;

        if (ImGui::BeginDragDropTarget())
        {
            // 没法根据 payload type + asset type 来判断是否接受，这样处理不了多态
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragDropPayloadType_AssetPath))
            {
                const char* newAssetPath = reinterpret_cast<char*>(payload->Data);

                if (currentObjectState != MarchObjectState::Persistent || newAssetPath != persistentPath)
                {
                    persistentPath.assign(newAssetPath);
                    isChanged = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        return isChanged;
    }
}

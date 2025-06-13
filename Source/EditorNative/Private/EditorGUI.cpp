#include "pch.h"
#include "EditorGUI.h"
#include "Engine/Rendering/D3D12.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include <utility>
#include <algorithm>

using namespace DirectX;

namespace march
{
    namespace
    {
        bool IsHiddenLabel(const std::string& label)
        {
            return label.size() >= 2 && label[0] == '#' && label[1] == '#';
        }

        void SetNextItemWidthIfNot(float width)
        {
            ImGuiContext* context = ImGui::GetCurrentContext();

            if ((context->NextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth) != ImGuiNextItemDataFlags_HasWidth)
            {
                ImGui::SetNextItemWidth(-1);
            }
        }
    }

    void EditorGUI::PrefixLabel(const std::string& label, const std::string& tooltip)
    {
        float width = ImGui::GetContentRegionMax().x;
        float fieldWidth = std::min(std::max(width - MinLabelWidth, 0.0f), MaxFieldWidth);
        float labelWidth = std::max(width - fieldWidth, 0.0f);

        ImVec2 pos = ImGui::GetCursorPos();

        //ImGui::PushTextWrapPos(labelWidth - ImGui::GetStyle().FramePadding.x);
        ImGui::TextUnformatted(label.c_str());
        //ImGui::PopTextWrapPos();

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
            SetNextItemWidthIfNot(-1);
            return ImGui::DragInt(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragInt(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::FloatField(const std::string& label, const std::string& tooltip, float v[1], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            return ImGui::DragFloat(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector2Field(const std::string& label, const std::string& tooltip, float v[2], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            return ImGui::DragFloat2(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat2(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector3Field(const std::string& label, const std::string& tooltip, float v[3], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            return ImGui::DragFloat3(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat3(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::Vector4Field(const std::string& label, const std::string& tooltip, float v[4], float speed, float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            return ImGui::DragFloat4(label.c_str(), v, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloat4(("##" + label).c_str(), v, speed, min, max);
    }

    bool EditorGUI::ColorField(const std::string& label, const std::string& tooltip, float v[4], bool alpha, bool hdr)
    {
        ImGuiColorEditFlags buttonFlags
            = ImGuiColorEditFlags_Float;
        ImGuiColorEditFlags pickerFlags
            = ImGuiColorEditFlags_Float
            | ImGuiColorEditFlags_InputRGB
            | ImGuiColorEditFlags_DisplayRGB
            | ImGuiColorEditFlags_DisplayHSV
            | ImGuiColorEditFlags_DisplayHex
            | ImGuiColorEditFlags_PickerHueWheel
            | ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_NoSmallPreview;
        ImGuiColorEditFlags historyColorFlags
            = ImGuiColorEditFlags_Float
            | ImGuiColorEditFlags_NoTooltip
            | ImGuiColorEditFlags_NoDragDrop
            | ImGuiColorEditFlags_NoBorder;

        if (!alpha)
        {
            buttonFlags |= ImGuiColorEditFlags_NoAlpha;
            pickerFlags |= ImGuiColorEditFlags_NoAlpha;
            historyColorFlags |= ImGuiColorEditFlags_NoAlpha;
        }

        if (hdr)
        {
            buttonFlags |= ImGuiColorEditFlags_HDR;
            pickerFlags |= ImGuiColorEditFlags_HDR;
            historyColorFlags |= ImGuiColorEditFlags_HDR;
        }

        bool isButtonClicked;
        ImVec4 buttonColor = ImVec4(v[0], v[1], v[2], v[3]);

        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            isButtonClicked = ImGui::ColorButton(label.c_str(), buttonColor, buttonFlags);
        }
        else
        {
            PrefixLabel(label, tooltip);
            float width = ImGui::GetContentRegionAvail().x;
            float height = ImGui::GetFrameHeight();
            isButtonClicked = ImGui::ColorButton(("##" + label).c_str(), buttonColor, buttonFlags, ImVec2(width, height));
        }

        ImGui::PushID(label.c_str());

        // 打开 color picker 时，记录最初的颜色
        static ImVec4 originalColor{};

        if (isButtonClicked)
        {
            ImGui::OpenPopup("##ColorPopup");
            originalColor = buttonColor;
        }

        bool isChanged = false;

        if (ImGui::BeginPopup("##ColorPopup"))
        {
            ImGui::SeparatorText("Color");

            // 显示左右两个颜色按钮，左边是最初的颜色，右边是当前颜色
            constexpr ImVec2 historyColorButtonSize{ 45, 25 };
            ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - historyColorButtonSize.x * 2);
            if (ImGui::ColorButton("##Previous", originalColor, historyColorFlags, historyColorButtonSize))
            {
                // 恢复最初的颜色
                for (size_t i = 0; i < 4; i++)
                {
                    if (float c = (&originalColor.x)[i]; c != v[i])
                    {
                        v[i] = c;
                        isChanged = true;
                    }
                }
            }
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::SameLine();
            ImGui::PopStyleVar();
            ImGui::ColorButton("##Current", ImVec4(v[0], v[1], v[2], v[3]), historyColorFlags, historyColorButtonSize);

            ImGui::Spacing();

            isChanged |= ImGui::ColorPicker4("##ColorPicker", v, pickerFlags);

            if (hdr)
            {
                ImGui::SeparatorText("HDR");

                /*float exposure = 1.0f;
                PrefixLabel("Exposure", "");
                ImGui::DragFloat("##Exposure", &exposure);*/

                // 实现时可以参考
                // https://github.com/Unity-Technologies/UnityCsReference/blob/b42ec0031fc505c35aff00b6a36c25e67d81e59e/Editor/Mono/GUI/ColorMutator.cs#L23
                // https://github.com/Unity-Technologies/UnityCsReference/blob/master/Editor/Mono/GUI/ColorPicker.cs
                // 注：Unity 的 Color Picker 中 Intensity 就是 Exposure
                // 注：ImGui 的 ImGuiColorEditFlags_HDR 目前只是去掉了 [0, 1] 的限制，没有实现 HDR 的具体功能

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("TODO");
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();

        return isChanged;
    }

    bool EditorGUI::FloatSliderField(const std::string& label, const std::string& tooltip, float v[1], float min, float max)
    {
        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            return ImGui::SliderFloat(label.c_str(), v, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::SliderFloat(("##" + label).c_str(), v, min, max);
    }

    bool EditorGUI::CollapsingHeader(const std::string& label, bool defaultOpen)
    {
        return CollapsingHeader(label, nullptr, defaultOpen);
    }

    bool EditorGUI::CollapsingHeader(const std::string& label, bool* pVisible, bool defaultOpen)
    {
        return ImGui::CollapsingHeader(label.c_str(), pVisible, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    }

    bool EditorGUI::Combo(const std::string& label, const std::string& tooltip, int* currentItem, const std::string& itemsSeparatedByZeros)
    {
        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            return ImGui::Combo(label.c_str(), currentItem, itemsSeparatedByZeros.c_str());
        }

        PrefixLabel(label, tooltip);
        return ImGui::Combo(("##" + label).c_str(), currentItem, itemsSeparatedByZeros.c_str());
    }

    bool EditorGUI::CenterButton(const std::string& label, float width)
    {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth = ImGui::CalcTextSize(label.c_str()).x;
        auto padding = (std::min(windowWidth, width) - textWidth) * 0.5f;
        auto cursorPosX = (windowWidth - std::max(textWidth, width)) * 0.5f;

        ImGui::SetCursorPosX(std::max(0.0f, cursorPosX));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(std::max(0.0f, padding), ImGui::GetStyle().FramePadding.y));
        bool ret = ImGui::Button(label.c_str(), ImVec2(0, 0));
        ImGui::PopStyleVar();
        return ret;
    }

    void EditorGUI::CenterText(const std::string& text)
    {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
        auto cursorPosX = (windowWidth - textWidth) * 0.5f;

        ImGui::SetCursorPosX(std::max(0.0f, cursorPosX));
        ImGui::TextUnformatted(text.c_str());
    }

    void EditorGUI::Space()
    {
        ImGui::Spacing();
    }

    void EditorGUI::SeparatorText(const std::string& label)
    {
        ImGui::SeparatorText(label.c_str());
    }

    static int TextFieldCharFilter(ImGuiInputTextCallbackData* data)
    {
        std::string* blacklist = static_cast<std::string*>(data->UserData);
        return blacklist->find(static_cast<char>(data->EventChar)) == std::string::npos ? 0 : 1;
    }

    bool EditorGUI::TextField(const std::string& label, const std::string& tooltip, std::string& text, const std::string& charBlacklist)
    {
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackCharFilter;
        void* filterUserData = const_cast<std::string*>(&charBlacklist);

        if (IsHiddenLabel(label))
        {
            // https://github.com/ocornut/imgui/issues/623
            SetNextItemWidthIfNot(-1);
            return ImGui::InputText(label.c_str(), &text, flags, TextFieldCharFilter, filterUserData);
        }

        PrefixLabel(label, tooltip);
        return ImGui::InputText(("##" + label).c_str(), &text, flags, TextFieldCharFilter, filterUserData);
    }

    bool EditorGUI::Checkbox(const std::string& label, const std::string& tooltip, bool& value)
    {
        if (IsHiddenLabel(label))
        {
            SetNextItemWidthIfNot(-1);
            return ImGui::Checkbox(label.c_str(), &value);
        }

        PrefixLabel(label, tooltip);
        return ImGui::Checkbox(("##" + label).c_str(), &value);
    }

    void EditorGUI::BeginDisabled(bool disabled, bool allowInteraction)
    {
        if (allowInteraction)
        {
            ImGuiCol col = disabled ? ImGuiCol_TextDisabled : ImGuiCol_Text;
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(col));
        }
        else
        {
            ImGui::BeginDisabled(disabled);
        }
    }

    void EditorGUI::EndDisabled(bool allowInteraction)
    {
        if (allowInteraction)
        {
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::EndDisabled();
        }
    }

    void EditorGUI::LabelField(const std::string& label1, const std::string& tooltip, const std::string& label2)
    {
        if (IsHiddenLabel(label1))
        {
            SetNextItemWidthIfNot(-1);
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

    bool EditorGUI::Foldout(const std::string& label, const std::string& tooltip, bool defaultOpen)
    {
        // 加上 ImGuiTreeNodeFlags_NoTreePushOnOpen 就不用调用 TreePop() 了
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

        // 缩短箭头两边的空白
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, ImGui::GetStyle().FramePadding.y });
        bool result = ImGui::TreeNodeEx(label.c_str(), flags);
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

    XMFLOAT2 EditorGUI::GetContentRegionAvail()
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
            SetNextItemWidthIfNot(-1);
            return ImGui::DragFloatRange2(label.c_str(), &currentMin, &currentMax, speed, min, max);
        }

        PrefixLabel(label, tooltip);
        return ImGui::DragFloatRange2(("##" + label).c_str(), &currentMin, &currentMax, speed, min, max);
    }

    bool EditorGUI::BeginTreeNode(const std::string& label, int32_t index, bool isLeaf, bool selected, bool defaultOpen)
    {
        ImGuiTreeNodeFlags flags
            = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_OpenOnDoubleClick
            | ImGuiTreeNodeFlags_SpanFullWidth
            | ImGuiTreeNodeFlags_HideNavCursor;

        if (isLeaf)      flags |= ImGuiTreeNodeFlags_Leaf;
        if (selected)    flags |= ImGuiTreeNodeFlags_Selected;
        if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

        // 记录 tree node 的 index
        ImGui::SetNextItemSelectionUserData(static_cast<ImGuiSelectionUserData>(index));
        return ImGui::TreeNodeEx(label.c_str(), flags);
    }

    void EditorGUI::EndTreeNode()
    {
        ImGui::TreePop();
    }

    void* EditorGUI::BeginTreeNodeMultiSelect()
    {
        constexpr ImGuiMultiSelectFlags flags
            = ImGuiMultiSelectFlags_NoAutoClearOnReselect
            | ImGuiMultiSelectFlags_ClearOnClickVoid
            | ImGuiMultiSelectFlags_ScopeRect // 一个 Window 里可能有多个 MultiSelect
            | ImGuiMultiSelectFlags_SelectOnClickRelease;
        return ImGui::BeginMultiSelect(flags);
    }

    void* EditorGUI::EndTreeNodeMultiSelect()
    {
        return ImGui::EndMultiSelect();
    }

    int32_t EditorGUI::GetMultiSelectRequestCount(void* handle)
    {
        ImGuiMultiSelectIO* io = static_cast<ImGuiMultiSelectIO*>(handle);
        return static_cast<int32_t>(io->Requests.size());
    }

    void EditorGUI::GetMultiSelectRequests(void* handle, SelectionRequest* pOutRequests)
    {
        ImGuiMultiSelectIO* io = static_cast<ImGuiMultiSelectIO*>(handle);

        for (int i = 0; i < io->Requests.size(); i++)
        {
            const ImGuiSelectionRequest& req = io->Requests[i];

            if (req.Type == ImGuiSelectionRequestType_SetAll)
            {
                pOutRequests[i].Type = req.Selected ? SelectionRequestType::SetAll : SelectionRequestType::ClearAll;
                pOutRequests[i].StartIndex = -1;
                pOutRequests[i].EndIndex = -1;
            }
            else if (req.Type == ImGuiSelectionRequestType_SetRange)
            {
                pOutRequests[i].Type = req.Selected ? SelectionRequestType::SetRange : SelectionRequestType::ClearRange;
                pOutRequests[i].StartIndex = static_cast<int32_t>(req.RangeFirstItem);
                pOutRequests[i].EndIndex = static_cast<int32_t>(req.RangeLastItem);
            }
        }
    }

    bool EditorGUI::IsTreeNodeOpen(const std::string& id, bool defaultValue)
    {
        //return ImGui::TreeNodeGetOpen(ImGui::GetID(id.c_str()));

        // https://github.com/ocornut/imgui/blob/71c77c081ac36841e682498229088e7678207112/imgui_widgets.cpp#L6399
        ImGuiStorage* storage = ImGui::GetCurrentWindowRead()->DC.StateStorage;
        return storage->GetInt(ImGui::GetID(id.c_str()), defaultValue ? 1 : 0) != 0;
    }

    bool EditorGUI::IsWindowClicked(ImGuiMouseButton button)
    {
        // https://github.com/ocornut/imgui/issues/7879
        // 实现 down -> release 再触发 click 的效果
        return ImGui::IsMouseReleased(button)
            && !ImGui::IsMouseDragPastThreshold(button)
            && ImGui::IsWindowHovered();
    }

    bool EditorGUI::IsNothingClickedOnWindow()
    {
        ImGuiContext* context = ImGui::GetCurrentContext();

        return (IsWindowClicked(ImGuiMouseButton_Left) || IsWindowClicked(ImGuiMouseButton_Right))
            && context->ActiveId == 0
            && context->HoveredId == 0;
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
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImVec2 size = { region.x, static_cast<float>(texture->GetDesc().Height) / texture->GetDesc().Width * region.x};
        ImGui::Image(reinterpret_cast<ImTextureID>(texture), size);
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

    XMFLOAT2 EditorGUI::GetItemSpacing()
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

    float EditorGUI::GetCollapsingHeaderOuterExtend()
    {
        // https://github.com/ocornut/imgui/blob/master/imgui_widgets.cpp
        // bool ImGui::TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end)
        // ...
        // const float outer_extend = IM_TRUNC(window->WindowPadding.x * 0.5f); // Framed header expand a little outside of current limits
        // ...

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        return IM_TRUNC(window->WindowPadding.x * 0.5f);
    }

    bool EditorGUI::BeginMainMenuBar()
    {
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyleColorVec4(ImGuiCol_DockingEmptyBg));
        bool sideBar = BeginMainViewportSideBar("##MainMenuBar", ImGuiDir_Up, ImGui::GetFrameHeight(), ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleColor();

        if (!sideBar)
        {
            EndMainViewportSideBar();
            return false;
        }

        if (!ImGui::BeginMenuBar())
        {
            EndMainViewportSideBar();
            return false;
        }

        return true;
    }

    void EditorGUI::EndMainMenuBar()
    {
        ImGui::EndMenuBar();
        EndMainViewportSideBar();
    }

    bool EditorGUI::BeginMainViewportSideBar(const std::string& name, ImGuiDir dir, float contentHeight, ImGuiWindowFlags extraFlags)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_DockingEmptyBg));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float height = contentHeight + ImGui::GetStyle().WindowPadding.y * 2;

        ImGuiWindowFlags flags
            = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoMove
            | extraFlags;

        bool ret = ImGui::BeginViewportSideBar(name.c_str(), viewport, dir, height, flags);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        return ret;
    }

    void EditorGUI::EndMainViewportSideBar()
    {
        ImGui::End();
    }

    void EditorGUI::BulletLabel(const std::string& label, const std::string& tooltip)
    {
        ImGui::BulletText("%s", label.c_str());

        if (!tooltip.empty())
        {
            ImGui::SetItemTooltip(tooltip.c_str());
        }
    }

    void EditorGUI::Dummy(float width, float height)
    {
        ImGui::Dummy(ImVec2(width, height));
    }

    void EditorGUI::PushItemSpacing(const XMFLOAT2& value)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(value.x, value.y));
    }

    void EditorGUI::PopItemSpacing()
    {
        ImGui::PopStyleVar();
    }
}

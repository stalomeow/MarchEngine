#include "pch.h"
#include "Editor/EditorGUI.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO EditorGUI_PrefixLabel(cs_string label, cs_string tooltip)
{
    EditorGUI::PrefixLabel(label, tooltip);
}

NATIVE_EXPORT_AUTO EditorGUI_IntField(cs_string label, cs_string tooltip, cs<cs_int_t*> v, cs_int speed, cs_int minValue, cs_int maxValue)
{
    retcs EditorGUI::IntField(label, tooltip, v, static_cast<float>(speed.data), minValue, maxValue);
}

NATIVE_EXPORT_AUTO EditorGUI_FloatField(cs_string label, cs_string tooltip, cs<cs_float_t*> v, cs_float speed, cs_float minValue, cs_float maxValue)
{
    retcs EditorGUI::FloatField(label, tooltip, v, speed, minValue, maxValue);
}

NATIVE_EXPORT_AUTO EditorGUI_Vector2Field(cs_string label, cs_string tooltip, cs<cs_vec2_t*> v, cs_float speed, cs_float minValue, cs_float maxValue)
{
    retcs EditorGUI::Vector2Field(label, tooltip, reinterpret_cast<float*>(v.data), speed, minValue, maxValue);
}

NATIVE_EXPORT_AUTO EditorGUI_Vector3Field(cs_string label, cs_string tooltip, cs<cs_vec3_t*> v, cs_float speed, cs_float minValue, cs_float maxValue)
{
    retcs EditorGUI::Vector3Field(label, tooltip, reinterpret_cast<float*>(v.data), speed, minValue, maxValue);
}

NATIVE_EXPORT_AUTO EditorGUI_Vector4Field(cs_string label, cs_string tooltip, cs<cs_vec4_t*> v, cs_float speed, cs_float minValue, cs_float maxValue)
{
    retcs EditorGUI::Vector4Field(label, tooltip, reinterpret_cast<float*>(v.data), speed, minValue, maxValue);
}

NATIVE_EXPORT_AUTO EditorGUI_ColorField(cs_string label, cs_string tooltip, cs<cs_color_t*> v, cs_bool alpha, cs_bool hdr)
{
    retcs EditorGUI::ColorField(label, tooltip, reinterpret_cast<float*>(v.data), alpha, hdr);
}

NATIVE_EXPORT_AUTO EditorGUI_FloatSliderField(cs_string label, cs_string tooltip, cs<cs_float_t*> v, cs_float minValue, cs_float maxValue)
{
    retcs EditorGUI::FloatSliderField(label, tooltip, v, minValue, maxValue);
}

NATIVE_EXPORT_AUTO EditorGUI_CollapsingHeader(cs_string label, cs_bool defaultOpen)
{
    retcs EditorGUI::CollapsingHeader(label, defaultOpen);
}

NATIVE_EXPORT_AUTO EditorGUI_Combo(cs_string label, cs_string tooltip, cs<cs_int_t*> currentItem, cs_string itemsSeparatedByZeros)
{
    retcs EditorGUI::Combo(label, tooltip, reinterpret_cast<int*>(currentItem.data), itemsSeparatedByZeros);
}

NATIVE_EXPORT_AUTO EditorGUI_CenterButton(cs_string label, cs_float width)
{
    retcs EditorGUI::CenterButton(label, width);
}

NATIVE_EXPORT_AUTO EditorGUI_CenterText(cs_string text)
{
    EditorGUI::CenterText(text);
}

NATIVE_EXPORT_AUTO EditorGUI_Space()
{
    EditorGUI::Space();
}

NATIVE_EXPORT_AUTO EditorGUI_SeparatorText(cs_string label)
{
    EditorGUI::SeparatorText(label);
}

NATIVE_EXPORT_AUTO EditorGUI_TextField(cs_string label, cs_string tooltip, cs_string text, cs<cs_string*> outNewText, cs_string charBlacklist)
{
    std::string textContext = text;

    if (EditorGUI::TextField(label, tooltip, textContext, charBlacklist))
    {
        outNewText->assign(std::move(textContext));
        retcs true;
    }

    retcs false;
}

NATIVE_EXPORT_AUTO EditorGUI_Checkbox(cs_string label, cs_string tooltip, cs<cs_bool*> value)
{
    bool v = *value;

    if (EditorGUI::Checkbox(label, tooltip, v))
    {
        value->assign(v);
        retcs true;
    }

    retcs false;
}

NATIVE_EXPORT_AUTO EditorGUI_BeginDisabled(cs_bool disabled, cs_bool allowInteraction)
{
    EditorGUI::BeginDisabled(disabled, allowInteraction);
}

NATIVE_EXPORT_AUTO EditorGUI_EndDisabled(cs_bool allowInteraction)
{
    EditorGUI::EndDisabled(allowInteraction);
}

NATIVE_EXPORT_AUTO EditorGUI_LabelField(cs_string label1, cs_string tooltip, cs_string label2)
{
    EditorGUI::LabelField(label1, tooltip, label2);
}

NATIVE_EXPORT_AUTO EditorGUI_PushIDString(cs_string id)
{
    EditorGUI::PushID(id);
}

NATIVE_EXPORT_AUTO EditorGUI_PushIDInt(cs_int id)
{
    EditorGUI::PushID(static_cast<int>(id));
}

NATIVE_EXPORT_AUTO EditorGUI_PopID()
{
    EditorGUI::PopID();
}

NATIVE_EXPORT_AUTO EditorGUI_Foldout(cs_string label, cs_string tooltip, cs_bool defaultOpen)
{
    retcs EditorGUI::Foldout(label, tooltip, defaultOpen);
}

NATIVE_EXPORT_AUTO EditorGUI_FoldoutClosable(cs_string label, cs_string tooltip, cs<cs_bool*> pVisible)
{
    bool visible = *pVisible;
    bool open = EditorGUI::FoldoutClosable(label, tooltip, &visible);
    pVisible->assign(visible);
    retcs open;
}

NATIVE_EXPORT_AUTO EditorGUI_Indent(cs_uint count)
{
    EditorGUI::Indent(count);
}

NATIVE_EXPORT_AUTO EditorGUI_Unindent(cs_uint count)
{
    EditorGUI::Unindent(count);
}

NATIVE_EXPORT_AUTO EditorGUI_SameLine(cs_float offsetFromStartX, cs_float spacing)
{
    EditorGUI::SameLine(offsetFromStartX, spacing);
}

NATIVE_EXPORT_AUTO EditorGUI_GetContentRegionAvail()
{
    retcs EditorGUI::GetContentRegionAvail();
}

NATIVE_EXPORT_AUTO EditorGUI_SetNextItemWidth(cs_float width)
{
    EditorGUI::SetNextItemWidth(width);
}

NATIVE_EXPORT_AUTO EditorGUI_Separator()
{
    EditorGUI::Separator();
}

NATIVE_EXPORT_AUTO EditorGUI_BeginPopup(cs_string id)
{
    retcs EditorGUI::BeginPopup(id);
}

NATIVE_EXPORT_AUTO EditorGUI_EndPopup()
{
    EditorGUI::EndPopup();
}

NATIVE_EXPORT_AUTO EditorGUI_MenuItem(cs_string label, cs_bool selected, cs_bool enabled)
{
    retcs EditorGUI::MenuItem(label, selected, enabled);
}

NATIVE_EXPORT_AUTO EditorGUI_BeginMenu(cs_string label, cs_bool enabled)
{
    retcs EditorGUI::BeginMenu(label, enabled);
}

NATIVE_EXPORT_AUTO EditorGUI_EndMenu()
{
    EditorGUI::EndMenu();
}

NATIVE_EXPORT_AUTO EditorGUI_OpenPopup(cs_string id)
{
    EditorGUI::OpenPopup(id);
}

NATIVE_EXPORT_AUTO EditorGUI_FloatRangeField(cs_string label, cs_string tooltip, cs<cs_float_t*> currentMin, cs<cs_float_t*> currentMax, cs_float speed, cs_float minValue, cs_float maxValue)
{
    retcs EditorGUI::FloatRangeField(label, tooltip, *currentMin, *currentMax, speed, minValue, maxValue);
}

NATIVE_EXPORT_AUTO EditorGUI_BeginTreeNode(cs_string label, cs_bool isLeaf, cs_bool openOnArrow, cs_bool openOnDoubleClick, cs_bool selected, cs_bool showBackground, cs_bool defaultOpen, cs_bool spanWidth)
{
    retcs EditorGUI::BeginTreeNode(label, isLeaf, openOnArrow, openOnDoubleClick, selected, showBackground, defaultOpen, spanWidth);
}

NATIVE_EXPORT_AUTO EditorGUI_EndTreeNode()
{
    EditorGUI::EndTreeNode();
}

NATIVE_EXPORT_AUTO EditorGUI_IsTreeNodeOpen(cs_string id, cs_bool defaultValue)
{
    retcs EditorGUI::IsTreeNodeOpen(id, defaultValue);
}

NATIVE_EXPORT_AUTO EditorGUI_IsItemClicked(cs<ImGuiMouseButton> button, cs<EditorGUI::ItemClickOptions> options)
{
    retcs EditorGUI::IsItemClicked(button, options);
}

NATIVE_EXPORT_AUTO EditorGUI_IsWindowClicked(cs<ImGuiMouseButton> button, cs_bool ignorePopup)
{
    retcs EditorGUI::IsWindowClicked(button, ignorePopup);
}

NATIVE_EXPORT_AUTO EditorGUI_BeginPopupContextWindow()
{
    retcs EditorGUI::BeginPopupContextWindow();
}

NATIVE_EXPORT_AUTO EditorGUI_BeginPopupContextItem(cs_string id)
{
    retcs EditorGUI::BeginPopupContextItem(id);
}

NATIVE_EXPORT_AUTO EditorGUI_DrawTexture(GfxTexture* texture)
{
    EditorGUI::DrawTexture(texture);
}

NATIVE_EXPORT_AUTO EditorGUI_Button(cs_string label)
{
    retcs EditorGUI::Button(label);
}

NATIVE_EXPORT_AUTO EditorGUI_BeginGroup()
{
    EditorGUI::BeginGroup();
}

NATIVE_EXPORT_AUTO EditorGUI_EndGroup()
{
    EditorGUI::EndGroup();
}

NATIVE_EXPORT_AUTO EditorGUI_CalcButtonWidth(cs_string label)
{
    retcs EditorGUI::CalcButtonWidth(label);
}

NATIVE_EXPORT_AUTO EditorGUI_GetItemSpacing()
{
    retcs EditorGUI::GetItemSpacing();
}

NATIVE_EXPORT_AUTO EditorGUI_GetCursorPosX()
{
    retcs EditorGUI::GetCursorPosX();
}

NATIVE_EXPORT_AUTO EditorGUI_SetCursorPosX(cs_float localX)
{
    EditorGUI::SetCursorPosX(localX);
}

NATIVE_EXPORT_AUTO EditorGUI_GetCollapsingHeaderOuterExtend()
{
    retcs EditorGUI::GetCollapsingHeaderOuterExtend();
}

NATIVE_EXPORT_AUTO EditorGUI_BeginMainMenuBar()
{
    retcs EditorGUI::BeginMainMenuBar();
}

NATIVE_EXPORT_AUTO EditorGUI_EndMainMenuBar()
{
    EditorGUI::EndMainMenuBar();
}

NATIVE_EXPORT_AUTO EditorGUI_BulletLabel(cs_string label, cs_string tooltip)
{
    EditorGUI::BulletLabel(label, tooltip);
}

#include "Editor/EditorGUI.h"
#include "Scripting/ScriptTypes.h"

using namespace dx12demo;

NATIVE_EXPORT(void) EditorGUI_PrefixLabel(CSharpString label, CSharpString tooltip)
{
    EditorGUI::PrefixLabel(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_IntField(CSharpString label, CSharpString tooltip, CSharpInt* v, CSharpFloat speed, CSharpInt minValue, CSharpInt maxValue)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::IntField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v, speed, minValue, maxValue));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_FloatField(CSharpString label, CSharpString tooltip, CSharpFloat* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::FloatField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v, speed, minValue, maxValue));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_Vector2Field(CSharpString label, CSharpString tooltip, CSharpVector2* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::Vector2Field(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v), speed, minValue, maxValue));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_Vector3Field(CSharpString label, CSharpString tooltip, CSharpVector3* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::Vector3Field(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v), speed, minValue, maxValue));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_Vector4Field(CSharpString label, CSharpString tooltip, CSharpVector4* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::Vector4Field(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v), speed, minValue, maxValue));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_ColorField(CSharpString label, CSharpString tooltip, CSharpColor* v)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::ColorField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v)));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_FloatSliderField(CSharpString label, CSharpString tooltip, CSharpFloat* v, CSharpFloat minValue, CSharpFloat maxValue)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::FloatSliderField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v, minValue, maxValue));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_CollapsingHeader(CSharpString label, CSharpBool defaultOpen)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::CollapsingHeader(CSharpString_ToUtf8(label), CSHARP_UNMARSHAL_BOOL(defaultOpen)));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_Combo(CSharpString label, CSharpString tooltip, CSharpInt* currentItem, CSharpString itemsSeparatedByZeros)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::Combo(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<int*>(currentItem), CSharpString_ToUtf8(itemsSeparatedByZeros)));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_CenterButton(CSharpString label, CSharpFloat width)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::CenterButton(CSharpString_ToUtf8(label), width));
}

NATIVE_EXPORT(void) EditorGUI_Space()
{
    EditorGUI::Space();
}

NATIVE_EXPORT(void) EditorGUI_SeparatorText(CSharpString label)
{
    EditorGUI::SeparatorText(CSharpString_ToUtf8(label));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_TextField(CSharpString label, CSharpString tooltip, CSharpString text, CSharpString* outNewText)
{
    std::string textContext = CSharpString_ToUtf8(text);

    if (EditorGUI::TextField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), textContext))
    {
        *outNewText = CSharpString_FromUtf8(textContext);
        return CSHARP_MARSHAL_BOOL(true);
    }

    *outNewText = nullptr;
    return CSHARP_MARSHAL_BOOL(false);
}

NATIVE_EXPORT(CSharpBool) EditorGUI_Checkbox(CSharpString label, CSharpString tooltip, CSharpBool* value)
{
    bool v = CSHARP_UNMARSHAL_BOOL(*value);

    if (EditorGUI::Checkbox(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v))
    {
        *value = CSHARP_MARSHAL_BOOL(v);
        return CSHARP_MARSHAL_BOOL(true);
    }

    return CSHARP_MARSHAL_BOOL(false);
}

NATIVE_EXPORT(void) EditorGUI_BeginDisabled(CSharpBool disabled)
{
    EditorGUI::BeginDisabled(CSHARP_UNMARSHAL_BOOL(disabled));
}

NATIVE_EXPORT(void) EditorGUI_EndDisabled()
{
    EditorGUI::EndDisabled();
}

NATIVE_EXPORT(void) EditorGUI_LabelField(CSharpString label1, CSharpString tooltip, CSharpString label2)
{
    EditorGUI::LabelField(CSharpString_ToUtf8(label1), CSharpString_ToUtf8(tooltip), CSharpString_ToUtf8(label2));
}

NATIVE_EXPORT(void) EditorGUI_PushIDString(CSharpString id)
{
    EditorGUI::PushID(CSharpString_ToUtf8(id));
}

NATIVE_EXPORT(void) EditorGUI_PushIDInt(CSharpInt id)
{
    EditorGUI::PushID(static_cast<int>(id));
}

NATIVE_EXPORT(void) EditorGUI_PopID()
{
    EditorGUI::PopID();
}

NATIVE_EXPORT(CSharpBool) EditorGUI_Foldout(CSharpString label, CSharpString tooltip)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::Foldout(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip)));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_FoldoutClosable(CSharpString label, CSharpString tooltip, CSharpBool* pVisible)
{
    bool visible = CSHARP_UNMARSHAL_BOOL(*pVisible);
    bool open = EditorGUI::FoldoutClosable(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), &visible);
    *pVisible = CSHARP_MARSHAL_BOOL(visible);
    return CSHARP_MARSHAL_BOOL(open);
}

NATIVE_EXPORT(void) EditorGUI_Indent(CSharpUInt count)
{
    EditorGUI::Indent(static_cast<std::uint32_t>(count));
}

NATIVE_EXPORT(void) EditorGUI_Unindent(CSharpUInt count)
{
    EditorGUI::Unindent(static_cast<std::uint32_t>(count));
}

NATIVE_EXPORT(void) EditorGUI_SameLine(CSharpFloat offsetFromStartX, CSharpFloat spacing)
{
    EditorGUI::SameLine(offsetFromStartX, spacing);
}

NATIVE_EXPORT(CSharpVector2) EditorGUI_GetContentRegionAvail()
{
    DirectX::XMFLOAT2 avail = EditorGUI::GetContentRegionAvail();
    return ToCSharpVector2(avail);
}

NATIVE_EXPORT(void) EditorGUI_SetNextItemWidth(CSharpFloat width)
{
    EditorGUI::SetNextItemWidth(width);
}

NATIVE_EXPORT(void) EditorGUI_Separator()
{
    EditorGUI::Separator();
}

NATIVE_EXPORT(CSharpBool) EditorGUI_BeginPopup(CSharpString id)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::BeginPopup(CSharpString_ToUtf8(id)));
}

NATIVE_EXPORT(void) EditorGUI_EndPopup()
{
    EditorGUI::EndPopup();
}

NATIVE_EXPORT(CSharpBool) EditorGUI_MenuItem(CSharpString label, CSharpBool selected, CSharpBool enabled)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::MenuItem(CSharpString_ToUtf8(label), CSHARP_UNMARSHAL_BOOL(selected), CSHARP_UNMARSHAL_BOOL(enabled)));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_BeginMenu(CSharpString label, CSharpBool enabled)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::BeginMenu(CSharpString_ToUtf8(label), CSHARP_UNMARSHAL_BOOL(enabled)));
}

NATIVE_EXPORT(void) EditorGUI_EndMenu()
{
    EditorGUI::EndMenu();
}

NATIVE_EXPORT(void) EditorGUI_OpenPopup(CSharpString id)
{
    EditorGUI::OpenPopup(CSharpString_ToUtf8(id));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_FloatRangeField(CSharpString label, CSharpString tooltip, CSharpFloat* currentMin, CSharpFloat* currentMax, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::FloatRangeField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), *currentMin, *currentMax, speed, minValue, maxValue));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_BeginTreeNode(CSharpString label, CSharpBool isLeaf, CSharpBool openOnArrow, CSharpBool openOnDoubleClick, CSharpBool selected, CSharpBool showBackground, CSharpBool defaultOpen, CSharpBool spanWidth)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::BeginTreeNode(CSharpString_ToUtf8(label),
        CSHARP_UNMARSHAL_BOOL(isLeaf), CSHARP_UNMARSHAL_BOOL(openOnArrow), CSHARP_UNMARSHAL_BOOL(openOnDoubleClick), CSHARP_UNMARSHAL_BOOL(selected),
        CSHARP_UNMARSHAL_BOOL(showBackground), CSHARP_UNMARSHAL_BOOL(defaultOpen), CSHARP_UNMARSHAL_BOOL(spanWidth)));
}

NATIVE_EXPORT(void) EditorGUI_EndTreeNode()
{
    EditorGUI::EndTreeNode();
}

NATIVE_EXPORT(CSharpBool) EditorGUI_IsItemClicked(CSharpInt buttton, CSharpBool ignorePopup)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::IsItemClicked(static_cast<ImGuiMouseButton>(buttton), CSHARP_UNMARSHAL_BOOL(ignorePopup)));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_BeginPopupContextWindow()
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::BeginPopupContextWindow());
}

NATIVE_EXPORT(CSharpBool) EditorGUI_BeginPopupContextItem(CSharpString id)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::BeginPopupContextItem(CSharpString_ToUtf8(id)));
}

NATIVE_EXPORT(void) EditorGUI_DrawTexture(Texture* texture)
{
    EditorGUI::DrawTexture(texture);
}

NATIVE_EXPORT(CSharpBool) EditorGUI_Button(CSharpString label)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::Button(CSharpString_ToUtf8(label)));
}

NATIVE_EXPORT(void) EditorGUI_BeginGroup()
{
    EditorGUI::BeginGroup();
}

NATIVE_EXPORT(void) EditorGUI_EndGroup()
{
    EditorGUI::EndGroup();
}

NATIVE_EXPORT(CSharpFloat) EditorGUI_CalcButtonWidth(CSharpString label)
{
    return EditorGUI::CalcButtonWidth(CSharpString_ToUtf8(label));
}

NATIVE_EXPORT(CSharpVector2) EditorGUI_GetItemSpacing()
{
    DirectX::XMFLOAT2 spacing = EditorGUI::GetItemSpacing();
    return ToCSharpVector2(spacing);
}

NATIVE_EXPORT(CSharpFloat) EditorGUI_GetCursorPosX()
{
    return EditorGUI::GetCursorPosX();
}

NATIVE_EXPORT(void) EditorGUI_SetCursorPosX(CSharpFloat localX)
{
    EditorGUI::SetCursorPosX(localX);
}

NATIVE_EXPORT(CSharpBool) EditorGUI_BeginAssetTreeNode(CSharpString label, CSharpString assetPath, CSharpBool isLeaf, CSharpBool openOnArrow, CSharpBool openOnDoubleClick, CSharpBool selected, CSharpBool showBackground, CSharpBool defaultOpen, CSharpBool spanWidth)
{
    return CSHARP_MARSHAL_BOOL(EditorGUI::BeginAssetTreeNode(CSharpString_ToUtf8(label), CSharpString_ToUtf8(assetPath),
        CSHARP_UNMARSHAL_BOOL(isLeaf), CSHARP_UNMARSHAL_BOOL(openOnArrow), CSHARP_UNMARSHAL_BOOL(openOnDoubleClick), CSHARP_UNMARSHAL_BOOL(selected),
        CSHARP_UNMARSHAL_BOOL(showBackground), CSHARP_UNMARSHAL_BOOL(defaultOpen), CSHARP_UNMARSHAL_BOOL(spanWidth)));
}

NATIVE_EXPORT(CSharpBool) EditorGUI_AssetField(CSharpString label, CSharpString tooltip, CSharpString path, CSharpString* outNewPath)
{
    std::string pathContext = CSharpString_ToUtf8(path);

    if (EditorGUI::AssetField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), pathContext))
    {
        *outNewPath = CSharpString_FromUtf8(pathContext);
        return CSHARP_MARSHAL_BOOL(true);
    }

    *outNewPath = nullptr;
    return CSHARP_MARSHAL_BOOL(false);
}

#pragma once

#include "Scripting/ScriptTypes.h"
#include "Rendering/Resource/Texture.h"
#include "Rendering/DescriptorHeap.h"
#include <string>
#include <imgui.h>
#include <DirectXMath.h>
#include <memory>

namespace dx12demo
{
    class EditorGUI final
    {
    public:
        static void PrefixLabel(const std::string& label, const std::string& tooltip);

        static bool IntField(const std::string& label, const std::string& tooltip, int v[1], float speed = 1.0f, int min = 0, int max = 0);
        static bool FloatField(const std::string& label, const std::string& tooltip, float v[1], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector2Field(const std::string& label, const std::string& tooltip, float v[2], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector3Field(const std::string& label, const std::string& tooltip, float v[3], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector4Field(const std::string& label, const std::string& tooltip, float v[4], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool ColorField(const std::string& label, const std::string& tooltip, float v[4]);

        static bool FloatSliderField(const std::string& label, const std::string& tooltip, float v[1], float min, float max);

        static bool CollapsingHeader(const std::string& label, bool defaultOpen = false);
        static bool Combo(const std::string& label, const std::string& tooltip, int* currentItem, const std::string& itemsSeparatedByZeros);
        static bool CenterButton(const std::string& label, float width = 0.0f);
        static void Space();
        static void SeparatorText(const std::string& label);
        static bool TextField(const std::string& label, const std::string& tooltip, std::string& text);
        static bool Checkbox(const std::string& label, const std::string& tooltip, bool& value);
        static void BeginDisabled(bool disabled = true);
        static void EndDisabled();
        static void LabelField(const std::string& label1, const std::string& tooltip, const std::string& label2);
        static void PushID(const std::string& id);
        static void PushID(int id);
        static void PopID();
        static bool Foldout(const std::string& label, const std::string& tooltip);
        static void Indent(std::uint32_t count = 1);
        static void Unindent(std::uint32_t count = 1);
        static void SameLine(float offsetFromStartX = 0.0f, float spacing = -1.0f);
        static DirectX::XMFLOAT2 GetContentRegionAvail();
        static void SetNextItemWidth(float width);
        static void Separator();
        static bool BeginPopup(const std::string& id);
        // only call EndPopup() if BeginPopupXXX() returns true!
        static void EndPopup();
        static bool MenuItem(const std::string& label, bool selected = false, bool enabled = true);
        static bool BeginMenu(const std::string& label, bool enabled = true);
        // only call EndMenu() if BeginMenu() returns true!
        static void EndMenu();
        static void OpenPopup(const std::string& id);
        static bool FloatRangeField(const std::string& label, const std::string& tooltip, float& currentMin, float& currentMax, float speed = 0.1f, float min = 0.0f, float max = 0.0f);

        static bool BeginTreeNode(const std::string& label, bool isLeaf = false, bool openOnArrow = false, bool openOnDoubleClick = false, bool selected = false, bool showBackground = false, bool defaultOpen = false, bool spanWidth = true);
        // only call EndTreeNode() if BeginTreeNode() returns true!
        static void EndTreeNode();
        static bool IsItemClicked(ImGuiMouseButton button = ImGuiMouseButton_Left, bool ignorePopup = false);
        static bool BeginPopupContextWindow();
        static bool BeginPopupContextItem(const std::string& id = "");

        static void DrawTexture(Texture* texture);
        static bool Button(const std::string& label);

        static void BeginGroup();
        static void EndGroup();
        static float CalcButtonWidth(const std::string& label);
        static DirectX::XMFLOAT2 GetItemSpacing();
        static float GetCursorPosX();
        static void SetCursorPosX(float localX);

        inline static void SetSrvHeap(DescriptorHeap* heap)
        {
            SrvHeap = heap;
        }

    public:
        static DescriptorHeap* SrvHeap;
        static constexpr float MinLabelWidth = 140.0f;
        static constexpr float MaxFieldWidth = 320.0f;
    };

    namespace binding
    {
        inline CSHARP_API(void) EditorGUI_PrefixLabel(CSharpString label, CSharpString tooltip)
        {
            EditorGUI::PrefixLabel(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_IntField(CSharpString label, CSharpString tooltip, CSharpInt* v, CSharpFloat speed, CSharpInt minValue, CSharpInt maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::IntField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v, speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_FloatField(CSharpString label, CSharpString tooltip, CSharpFloat* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::FloatField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v, speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Vector2Field(CSharpString label, CSharpString tooltip, CSharpVector2* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Vector2Field(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v), speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Vector3Field(CSharpString label, CSharpString tooltip, CSharpVector3* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Vector3Field(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v), speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Vector4Field(CSharpString label, CSharpString tooltip, CSharpVector4* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Vector4Field(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v), speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_ColorField(CSharpString label, CSharpString tooltip, CSharpColor* v)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::ColorField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<float*>(v)));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_FloatSliderField(CSharpString label, CSharpString tooltip, CSharpFloat* v, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::FloatSliderField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_CollapsingHeader(CSharpString label, CSharpBool defaultOpen)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::CollapsingHeader(CSharpString_ToUtf8(label), CSHARP_UNMARSHAL_BOOL(defaultOpen)));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Combo(CSharpString label, CSharpString tooltip, CSharpInt* currentItem, CSharpString itemsSeparatedByZeros)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Combo(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), reinterpret_cast<int*>(currentItem), CSharpString_ToUtf8(itemsSeparatedByZeros)));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_CenterButton(CSharpString label, CSharpFloat width)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::CenterButton(CSharpString_ToUtf8(label), width));
        }

        inline CSHARP_API(void) EditorGUI_Space()
        {
            EditorGUI::Space();
        }

        inline CSHARP_API(void) EditorGUI_SeparatorText(CSharpString label)
        {
            EditorGUI::SeparatorText(CSharpString_ToUtf8(label));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_TextField(CSharpString label, CSharpString tooltip, CSharpString text, CSharpString* outNewText)
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

        inline CSHARP_API(CSharpBool) EditorGUI_Checkbox(CSharpString label, CSharpString tooltip, CSharpBool* value)
        {
            bool v = CSHARP_UNMARSHAL_BOOL(*value);

            if (EditorGUI::Checkbox(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), v))
            {
                *value = CSHARP_MARSHAL_BOOL(v);
                return CSHARP_MARSHAL_BOOL(true);
            }

            return CSHARP_MARSHAL_BOOL(false);
        }

        inline CSHARP_API(void) EditorGUI_BeginDisabled(CSharpBool disabled)
        {
            EditorGUI::BeginDisabled(CSHARP_UNMARSHAL_BOOL(disabled));
        }

        inline CSHARP_API(void) EditorGUI_EndDisabled()
        {
            EditorGUI::EndDisabled();
        }

        inline CSHARP_API(void) EditorGUI_LabelField(CSharpString label1, CSharpString tooltip, CSharpString label2)
        {
            EditorGUI::LabelField(CSharpString_ToUtf8(label1), CSharpString_ToUtf8(tooltip), CSharpString_ToUtf8(label2));
        }

        inline CSHARP_API(void) EditorGUI_PushIDString(CSharpString id)
        {
            EditorGUI::PushID(CSharpString_ToUtf8(id));
        }

        inline CSHARP_API(void) EditorGUI_PushIDInt(CSharpInt id)
        {
            EditorGUI::PushID(static_cast<int>(id));
        }

        inline CSHARP_API(void) EditorGUI_PopID()
        {
            EditorGUI::PopID();
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Foldout(CSharpString label, CSharpString tooltip)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Foldout(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip)));
        }

        inline CSHARP_API(void) EditorGUI_Indent(CSharpUInt count)
        {
            EditorGUI::Indent(static_cast<std::uint32_t>(count));
        }

        inline CSHARP_API(void) EditorGUI_Unindent(CSharpUInt count)
        {
            EditorGUI::Unindent(static_cast<std::uint32_t>(count));
        }

        inline CSHARP_API(void) EditorGUI_SameLine(CSharpFloat offsetFromStartX, CSharpFloat spacing)
        {
            EditorGUI::SameLine(offsetFromStartX, spacing);
        }

        inline CSHARP_API(CSharpVector2) EditorGUI_GetContentRegionAvail()
        {
            DirectX::XMFLOAT2 avail = EditorGUI::GetContentRegionAvail();
            return ToCSharpVector2(avail);
        }

        inline CSHARP_API(void) EditorGUI_SetNextItemWidth(CSharpFloat width)
        {
            EditorGUI::SetNextItemWidth(width);
        }

        inline CSHARP_API(void) EditorGUI_Separator()
        {
            EditorGUI::Separator();
        }

        inline CSHARP_API(CSharpBool) EditorGUI_BeginPopup(CSharpString id)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::BeginPopup(CSharpString_ToUtf8(id)));
        }

        inline CSHARP_API(void) EditorGUI_EndPopup()
        {
            EditorGUI::EndPopup();
        }

        inline CSHARP_API(CSharpBool) EditorGUI_MenuItem(CSharpString label, CSharpBool selected, CSharpBool enabled)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::MenuItem(CSharpString_ToUtf8(label), CSHARP_UNMARSHAL_BOOL(selected), CSHARP_UNMARSHAL_BOOL(enabled)));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_BeginMenu(CSharpString label, CSharpBool enabled)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::BeginMenu(CSharpString_ToUtf8(label), CSHARP_UNMARSHAL_BOOL(enabled)));
        }

        inline CSHARP_API(void) EditorGUI_EndMenu()
        {
            EditorGUI::EndMenu();
        }

        inline CSHARP_API(void) EditorGUI_OpenPopup(CSharpString id)
        {
            EditorGUI::OpenPopup(CSharpString_ToUtf8(id));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_FloatRangeField(CSharpString label, CSharpString tooltip, CSharpFloat* currentMin, CSharpFloat* currentMax, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::FloatRangeField(CSharpString_ToUtf8(label), CSharpString_ToUtf8(tooltip), *currentMin, *currentMax, speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_BeginTreeNode(CSharpString label, CSharpBool isLeaf, CSharpBool openOnArrow, CSharpBool openOnDoubleClick, CSharpBool selected, CSharpBool showBackground, CSharpBool defaultOpen, CSharpBool spanWidth)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::BeginTreeNode(CSharpString_ToUtf8(label),
                CSHARP_UNMARSHAL_BOOL(isLeaf), CSHARP_UNMARSHAL_BOOL(openOnArrow), CSHARP_UNMARSHAL_BOOL(openOnDoubleClick), CSHARP_UNMARSHAL_BOOL(selected),
                CSHARP_UNMARSHAL_BOOL(showBackground), CSHARP_UNMARSHAL_BOOL(defaultOpen), CSHARP_UNMARSHAL_BOOL(spanWidth)));
        }

        inline CSHARP_API(void) EditorGUI_EndTreeNode()
        {
            EditorGUI::EndTreeNode();
        }

        inline CSHARP_API(CSharpBool) EditorGUI_IsItemClicked(CSharpInt buttton, CSharpBool ignorePopup)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::IsItemClicked(static_cast<ImGuiMouseButton>(buttton), CSHARP_UNMARSHAL_BOOL(ignorePopup)));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_BeginPopupContextWindow()
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::BeginPopupContextWindow());
        }

        inline CSHARP_API(CSharpBool) EditorGUI_BeginPopupContextItem(CSharpString id)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::BeginPopupContextItem(CSharpString_ToUtf8(id)));
        }

        inline CSHARP_API(void) EditorGUI_DrawTexture(Texture* texture)
        {
            EditorGUI::DrawTexture(texture);
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Button(CSharpString label)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Button(CSharpString_ToUtf8(label)));
        }

        inline CSHARP_API(void) EditorGUI_BeginGroup()
        {
            EditorGUI::BeginGroup();
        }

        inline CSHARP_API(void) EditorGUI_EndGroup()
        {
            EditorGUI::EndGroup();
        }

        inline CSHARP_API(CSharpFloat) EditorGUI_CalcButtonWidth(CSharpString label)
        {
            return EditorGUI::CalcButtonWidth(CSharpString_ToUtf8(label));
        }

        inline CSHARP_API(CSharpVector2) EditorGUI_GetItemSpacing()
        {
            DirectX::XMFLOAT2 spacing = EditorGUI::GetItemSpacing();
            return ToCSharpVector2(spacing);
        }

        inline CSHARP_API(CSharpFloat) EditorGUI_GetCursorPosX()
        {
            return EditorGUI::GetCursorPosX();
        }

        inline CSHARP_API(void) EditorGUI_SetCursorPosX(CSharpFloat localX)
        {
            EditorGUI::SetCursorPosX(localX);
        }
    }
}

#pragma once

#include "Scripting/ScriptTypes.h"
#include <string>
#include <imgui.h>

namespace dx12demo
{
    class EditorGUI final
    {
    public:
        static void PrefixLabel(const std::string& label);

        static bool FloatField(const std::string& label, float v[1], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector2Field(const std::string& label, float v[2], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector3Field(const std::string& label, float v[3], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector4Field(const std::string& label, float v[4], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool ColorField(const std::string& label, float v[4]);

        static bool FloatSliderField(const std::string& label, float v[1], float min, float max);

        static bool CollapsingHeader(const std::string& label, bool defaultOpen = false);
        static bool Combo(const std::string& label, int* currentItem, const std::string& itemsSeparatedByZeros);
        static bool CenterButton(const std::string& label, float width = 0.0f);
        static void Space();
        static void SeparatorText(const std::string& label);
        static bool TextField(const std::string& label, std::string& text);
        static bool Checkbox(const std::string& label, bool& value);
        static void BeginDisabled(bool disabled = true);
        static void EndDisabled();
        static void LabelField(const std::string& label1, const std::string& label2);

    public:
        static constexpr float MaxLabelWidth = 120.0f;
    };

    namespace binding
    {
        inline CSHARP_API(void) EditorGUI_PrefixLabel(CSharpString label)
        {
            EditorGUI::PrefixLabel(CSharpString_ToUtf8(label));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_FloatField(CSharpString label, CSharpFloat* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::FloatField(CSharpString_ToUtf8(label), v, speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Vector2Field(CSharpString label, CSharpVector2* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Vector2Field(CSharpString_ToUtf8(label), reinterpret_cast<float*>(v), speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Vector3Field(CSharpString label, CSharpVector3* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Vector3Field(CSharpString_ToUtf8(label), reinterpret_cast<float*>(v), speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Vector4Field(CSharpString label, CSharpVector4* v, CSharpFloat speed, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Vector4Field(CSharpString_ToUtf8(label), reinterpret_cast<float*>(v), speed, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_ColorField(CSharpString label, CSharpColor* v)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::ColorField(CSharpString_ToUtf8(label), reinterpret_cast<float*>(v)));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_FloatSliderField(CSharpString label, CSharpFloat* v, CSharpFloat minValue, CSharpFloat maxValue)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::FloatSliderField(CSharpString_ToUtf8(label), v, minValue, maxValue));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_CollapsingHeader(CSharpString label, CSharpBool defaultOpen)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::CollapsingHeader(CSharpString_ToUtf8(label), CSHARP_UNMARSHAL_BOOL(defaultOpen)));
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Combo(CSharpString label, CSharpInt* currentItem, CSharpString itemsSeparatedByZeros)
        {
            return CSHARP_MARSHAL_BOOL(EditorGUI::Combo(CSharpString_ToUtf8(label), reinterpret_cast<int*>(currentItem), CSharpString_ToUtf8(itemsSeparatedByZeros)));
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

        inline CSHARP_API(CSharpBool) EditorGUI_TextField(CSharpString label, CSharpString text, CSharpString* outNewText)
        {
            std::string textContext = CSharpString_ToUtf8(text);

            if (EditorGUI::TextField(CSharpString_ToUtf8(label), textContext))
            {
                *outNewText = CSharpString_FromUtf8(textContext);
                return CSHARP_MARSHAL_BOOL(true);
            }

            *outNewText = nullptr;
            return CSHARP_MARSHAL_BOOL(false);
        }

        inline CSHARP_API(CSharpBool) EditorGUI_Checkbox(CSharpString label, CSharpBool* value)
        {
            bool v = CSHARP_UNMARSHAL_BOOL(*value);

            if (EditorGUI::Checkbox(CSharpString_ToUtf8(label), v))
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

        inline CSHARP_API(void) EditorGUI_LabelField(CSharpString label1, CSharpString label2)
        {
            EditorGUI::LabelField(CSharpString_ToUtf8(label1), CSharpString_ToUtf8(label2));
        }
    }
}

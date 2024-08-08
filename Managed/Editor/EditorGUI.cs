using DX12Demo.Core;
using DX12Demo.Core.Binding;
using System.Numerics;

namespace DX12Demo.Editor
{
    public static unsafe partial class EditorGUI
    {
        public static void PrefixLabel(string label)
        {
            using NativeString l = label;
            EditorGUI_PrefixLabel(l.Data);
        }

        public static bool FloatField(string label, ref float value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;

            fixed (float* v = &value)
            {
                return EditorGUI_FloatField(l.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool Vector2Field(string label, ref Vector2 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;

            fixed (Vector2* v = &value)
            {
                return EditorGUI_Vector2Field(l.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool Vector3Field(string label, ref Vector3 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;

            fixed (Vector3* v = &value)
            {
                return EditorGUI_Vector3Field(l.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool Vector4Field(string label, ref Vector4 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;

            fixed (Vector4* v = &value)
            {
                return EditorGUI_Vector4Field(l.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool ColorField(string label, ref Color value)
        {
            using NativeString l = label;

            fixed (Color* v = &value)
            {
                return EditorGUI_ColorField(l.Data, v);
            }
        }

        public static bool FloatSliderField(string label, ref float value, float minValue, float maxValue)
        {
            using NativeString l = label;

            fixed (float* v = &value)
            {
                return EditorGUI_FloatSliderField(l.Data, v, minValue, maxValue);
            }
        }

        public static bool CollapsingHeader(string label, bool defaultOpen = false)
        {
            using NativeString l = label;
            return EditorGUI_CollapsingHeader(l.Data, defaultOpen);
        }

        public static bool EnumField(string label, ref object value)
        {
            Type enumType = value.GetType();
            if (!enumType.IsEnum)
            {
                throw new ArgumentException("Type must be an enum", nameof(value));
            }

            string[] names = Enum.GetNames(enumType);
            Array values = Enum.GetValues(enumType);
            int index = Array.IndexOf(values, value);

            using NativeString l = label;
            using NativeString items = string.Join('\0', names) + "\0\0";

            if (EditorGUI_Combo(l.Data, &index, items.Data))
            {
                value = values.GetValue(index)!;
                return true;
            }

            return false;
        }

        public static bool EnumField<T>(string label, ref T value) where T : struct, Enum
        {
            string[] names = Enum.GetNames<T>();
            T[] values = Enum.GetValues<T>();
            int index = Array.IndexOf(values, value);

            using NativeString l = label;
            using NativeString items = string.Join('\0', names) + "\0\0";

            if (EditorGUI_Combo(l.Data, &index, items.Data))
            {
                value = values[index];
                return true;
            }

            return false;
        }

        public static bool CenterButton(string label, float width)
        {
            using NativeString l = label;
            return EditorGUI_CenterButton(l.Data, width);
        }

        public static void SeparatorText(string label)
        {
            using NativeString l = label;
            EditorGUI_SeparatorText(l.Data);
        }

        public static bool TextField(string label, ref string text)
        {
            using NativeString l = label;
            using NativeString t = text;
            nint newText = nint.Zero;

            if (EditorGUI_TextField(l.Data, t.Data, &newText))
            {
                text = NativeString.Get(newText);
                NativeString.Free(newText);
                return true;
            }

            return false;
        }

        public static bool Checkbox(string label, ref bool value)
        {
            using NativeString l = label;

            fixed (bool* v = &value)
            {
                return EditorGUI_Checkbox(l.Data, v);
            }
        }

        public readonly ref struct DisabledScope
        {
            public DisabledScope() : this(true) { }

            public DisabledScope(bool disabled)
            {
                EditorGUI_BeginDisabled(disabled);
            }

            public void Dispose()
            {
                EditorGUI_EndDisabled();
            }
        }

        public static void LabelField(string label1, string label2)
        {
            using NativeString l1 = label1;
            using NativeString l2 = label2;
            EditorGUI_LabelField(l1.Data, l2.Data);
        }

        #region Native

        [NativeFunction]
        private static partial void EditorGUI_PrefixLabel(nint label);

        [NativeFunction]
        private static partial bool EditorGUI_FloatField(nint label, float* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_Vector2Field(nint label, Vector2* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_Vector3Field(nint label, Vector3* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_Vector4Field(nint label, Vector4* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_ColorField(nint label, Color* v);

        [NativeFunction]
        private static partial bool EditorGUI_FloatSliderField(nint label, float* v, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_CollapsingHeader(nint label, bool defaultOpen);

        [NativeFunction]
        private static partial bool EditorGUI_Combo(nint label, int* currentItem, nint itemsSeparatedByZeros);

        [NativeFunction]
        private static partial bool EditorGUI_CenterButton(nint label, float width);

        [NativeFunction(Name = "EditorGUI_Space")]
        public static partial void Space();

        [NativeFunction]
        private static partial void EditorGUI_SeparatorText(nint label);

        [NativeFunction]
        private static partial bool EditorGUI_TextField(nint label, nint text, nint* outNewText);

        [NativeFunction]
        private static partial bool EditorGUI_Checkbox(nint label, bool* v);

        [NativeFunction]
        private static partial void EditorGUI_BeginDisabled(bool disabled);

        [NativeFunction]
        private static partial void EditorGUI_EndDisabled();

        [NativeFunction]
        private static partial void EditorGUI_LabelField(nint label1, nint label2);

        #endregion
    }
}

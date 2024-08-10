using DX12Demo.Core;
using DX12Demo.Core.Binding;
using Newtonsoft.Json.Serialization;
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

        public static bool EnumField(string label, ref Enum value)
        {
            Type enumType = value.GetType();
            string[] names = Enum.GetNames(enumType);
            Array values = Enum.GetValues(enumType);
            int index = Array.IndexOf(values, value);

            using NativeString l = label;
            using NativeString items = string.Join('\0', names) + "\0\0";

            if (EditorGUI_Combo(l.Data, &index, items.Data))
            {
                value = (Enum)values.GetValue(index)!;
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

        public static void LabelField(string label1, string label2)
        {
            using NativeString l1 = label1;
            using NativeString l2 = label2;
            EditorGUI_LabelField(l1.Data, l2.Data);
        }

        public static bool Foldout(string label)
        {
            using NativeString l = label;
            return EditorGUI_Foldout(l.Data);
        }

        internal static bool BeginPopup(string id)
        {
            using NativeString i = id;
            return EditorGUI_BeginPopup(i.Data);
        }

        internal static bool MenuItem(string label, bool selected = false, bool enabled = true)
        {
            using NativeString l = label;
            return EditorGUI_MenuItem(l.Data, selected, enabled);
        }

        internal static bool BeginMenu(string label, bool enabled = true)
        {
            using NativeString l = label;
            return EditorGUI_BeginMenu(l.Data, enabled);
        }

        internal static void OpenPopup(string id)
        {
            using NativeString i = id;
            EditorGUI_OpenPopup(i.Data);
        }

        #region PropertyField

        private static readonly DrawerCache<IPropertyDrawer> s_PropertyDrawerCache = new(typeof(IPropertyDrawerFor<>));

        private static bool PropertyFieldImpl(string label, object target, JsonProperty property, int depth)
        {
            Type propertyType = property.PropertyType ?? throw new ArgumentException("Property type is null", nameof(property));

            if (s_PropertyDrawerCache.TryGet(propertyType, out IPropertyDrawer? drawer))
            {
                return drawer.Draw(label, target, property);
            }

            if (PersistentManager.ResolveJsonContract(propertyType) is JsonObjectContract contract)
            {
                // Draw as a nested object
                object? nestedTarget = property.GetValue<object>(target);

                if (nestedTarget == null)
                {
                    try
                    {
                        nestedTarget = Activator.CreateInstance(propertyType);

                        if (nestedTarget == null)
                        {
                            goto Fallback;
                        }

                        property.SetValue(target, nestedTarget);
                    }
                    catch
                    {
                        goto Fallback;
                    }
                }

                if (!Foldout(label))
                {
                    return false;
                }

                using (new IDScope(depth))
                using (new IndentedScope())
                {
                    bool changed = false;

                    foreach (var nestedProp in contract.Properties)
                    {
                        if (nestedProp.Ignored)
                        {
                            continue;
                        }

                        changed |= PropertyFieldImpl(nestedProp.GetDisplayName(), nestedTarget, nestedProp, depth + 1);
                    }

                    if (changed && propertyType.IsValueType)
                    {
                        // 值类型转成 object 会装箱生成一份拷贝，所以需要重新设置回去，把在拷贝上的修改同步到原对象上
                        property.SetValue(target, nestedTarget);
                    }

                    return changed;
                }
            }

        Fallback:
            using (new DisabledScope())
            {
                LabelField(label, $"Type {propertyType} is not supported");
                return false;
            }
        }

        public static bool PropertyField(string label, object target, JsonProperty property)
        {
            return PropertyFieldImpl(label, target, property, 0);
        }

        public static bool PropertyField(object target, JsonProperty property)
        {
            return PropertyField(property.GetDisplayName(), target, property);
        }

        #endregion

        #region Scope

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

        public readonly ref struct IDScope
        {
            [Obsolete("Use other constructors", error: true)]
            public IDScope() { }

            public IDScope(string id)
            {
                using NativeString i = id;
                EditorGUI_PushIDString(i.Data);
            }

            public IDScope(int id)
            {
                EditorGUI_PushIDInt(id);
            }

            public void Dispose()
            {
                EditorGUI_PopID();
            }
        }

        public readonly ref struct IndentedScope
        {
            private readonly uint m_IndentCount;

            public IndentedScope() : this(1) { }

            public IndentedScope(uint count)
            {
                m_IndentCount = count;
                EditorGUI_Indent(count);
            }

            public void Dispose()
            {
                EditorGUI_Unindent(m_IndentCount);
            }
        }

        #endregion

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

        [NativeFunction]
        private static partial void EditorGUI_PushIDString(nint id);

        [NativeFunction]
        private static partial void EditorGUI_PushIDInt(int id);

        [NativeFunction]
        private static partial void EditorGUI_PopID();

        [NativeFunction]
        private static partial bool EditorGUI_Foldout(nint label);

        [NativeFunction]
        private static partial void EditorGUI_Indent(uint count);

        [NativeFunction]
        private static partial void EditorGUI_Unindent(uint count);

        [NativeFunction(Name = "EditorGUI_SameLine")]
        public static partial void SameLine(float offsetFromStartX = 0.0f, float spacing = -1.0f);

        [NativeFunction(Name = "EditorGUI_GetContentRegionAvail")]
        public static partial Vector2 GetContentRegionAvailable();

        [NativeFunction(Name = "EditorGUI_SetNextItemWidth")]
        public static partial void SetNextItemWidth(float width);

        [NativeFunction(Name = "EditorGUI_Separator")]
        public static partial void Separator();

        [NativeFunction]
        private static partial bool EditorGUI_BeginPopup(nint id);

        /// <summary>
        /// only call EndPopup() if BeginPopupXXX() returns true!
        /// </summary>
        [NativeFunction(Name = "EditorGUI_EndPopup")]
        internal static partial void EndPopup();

        [NativeFunction]
        private static partial bool EditorGUI_MenuItem(nint label, bool selected, bool enabled);

        [NativeFunction]
        private static partial bool EditorGUI_BeginMenu(nint label, bool enabled);

        /// <summary>
        /// only call EndMenu() if BeginMenu() returns true!
        /// </summary>
        [NativeFunction(Name = "EditorGUI_EndMenu")]
        internal static partial void EndMenu();

        [NativeFunction]
        private static partial void EditorGUI_OpenPopup(nint id);

        #endregion
    }
}

using March.Core;
using March.Core.Binding;
using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace March.Editor
{
    public static unsafe partial class EditorGUI
    {
        public enum MouseButton : int
        {
            Left = 0,
            Right = 1,
            Middle = 2,
        }

        public enum ItemClickOptions : int
        {
            None = 0,
            IgnorePopup = 1 << 0,
            TreeNodeItem = 1 << 1,
            TreeNodeIsOpen = 1 << 2,
            TreeNodeIsLeaf = 1 << 3,
        }

        public enum ItemClickResult : int
        {
            False = 0,
            TreeNodeArrow = 1,
            True = 2,
        }

        public static void PrefixLabel(StringView label, StringView tooltip)
        {
            using NativeString l = label;
            using NativeString t = tooltip;
            EditorGUI_PrefixLabel(l.Data, t.Data);
        }

        public static bool IntField(StringView label, StringView tooltip, ref int value, int speed = 1, int minValue = 0, int maxValue = 0)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (int* v = &value)
            {
                return EditorGUI_IntField(l.Data, t.Data, v, (float)speed, minValue, maxValue);
            }
        }

        public static bool FloatField(StringView label, StringView tooltip, ref float value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (float* v = &value)
            {
                return EditorGUI_FloatField(l.Data, t.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool Vector2Field(StringView label, StringView tooltip, ref Vector2 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (Vector2* v = &value)
            {
                return EditorGUI_Vector2Field(l.Data, t.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool Vector3Field(StringView label, StringView tooltip, ref Vector3 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (Vector3* v = &value)
            {
                return EditorGUI_Vector3Field(l.Data, t.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool Vector4Field(StringView label, StringView tooltip, ref Vector4 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (Vector4* v = &value)
            {
                return EditorGUI_Vector4Field(l.Data, t.Data, v, speed, minValue, maxValue);
            }
        }

        public static bool ColorField(StringView label, StringView tooltip, ref Color value)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (Color* v = &value)
            {
                return EditorGUI_ColorField(l.Data, t.Data, v);
            }
        }

        public static bool FloatSliderField(StringView label, StringView tooltip, ref float value, float minValue, float maxValue)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (float* v = &value)
            {
                return EditorGUI_FloatSliderField(l.Data, t.Data, v, minValue, maxValue);
            }
        }

        public static bool CollapsingHeader(StringView label, bool defaultOpen = false)
        {
            using NativeString l = label;
            return EditorGUI_CollapsingHeader(l.Data, defaultOpen);
        }

        public static bool EnumField(StringView label, StringView tooltip, ref Enum value)
        {
            Type enumType = value.GetType();
            string[] names = Enum.GetNames(enumType);
            Array values = Enum.GetValues(enumType);
            int index = Array.IndexOf(values, value);

            using NativeString l = label;
            using NativeString t = tooltip;
            using NativeString items = string.Join('\0', names) + "\0\0";

            if (EditorGUI_Combo(l.Data, t.Data, &index, items.Data))
            {
                value = (Enum)values.GetValue(index)!;
                return true;
            }

            return false;
        }

        public static bool EnumField<T>(StringView label, StringView tooltip, ref T value) where T : struct, Enum
        {
            string[] names = Enum.GetNames<T>();
            T[] values = Enum.GetValues<T>();
            int index = Array.IndexOf(values, value);

            using NativeString l = label;
            using NativeString t = tooltip;
            using NativeString items = string.Join('\0', names) + "\0\0";

            if (EditorGUI_Combo(l.Data, t.Data, &index, items.Data))
            {
                value = values[index];
                return true;
            }

            return false;
        }

        public static bool CenterButton(StringView label, float width)
        {
            using NativeString l = label;
            return EditorGUI_CenterButton(l.Data, width);
        }

        public static void SeparatorText(StringView label)
        {
            using NativeString l = label;
            EditorGUI_SeparatorText(l.Data);
        }

        public static bool TextField(StringView label, StringView tooltip, ref string text, string charBlacklist = "")
        {
            using NativeString l = label;
            using NativeString tp = tooltip;
            using NativeString te = text;
            using NativeString cb = charBlacklist;
            nint newText = nint.Zero;

            if (EditorGUI_TextField(l.Data, tp.Data, te.Data, &newText, cb.Data))
            {
                text = NativeString.GetAndFree(newText);
                return true;
            }

            return false;
        }

        public static bool Checkbox(StringView label, StringView tooltip, ref bool value)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (bool* v = &value)
            {
                return EditorGUI_Checkbox(l.Data, t.Data, v);
            }
        }

        public static void LabelField(StringView label1, StringView tooltip, StringView label2)
        {
            using NativeString l1 = label1;
            using NativeString l2 = label2;
            using NativeString t = tooltip;
            EditorGUI_LabelField(l1.Data, t.Data, l2.Data);
        }

        public static bool Foldout(StringView label, StringView tooltip, bool defaultOpen = false)
        {
            using NativeString l = label;
            using NativeString t = tooltip;
            return EditorGUI_Foldout(l.Data, t.Data, defaultOpen);
        }

        public static bool FoldoutClosable(StringView label, StringView tooltip, ref bool visible)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (bool* v = &visible)
            {
                return EditorGUI_FoldoutClosable(l.Data, t.Data, v);
            }
        }

        internal static bool BeginPopup(StringView id)
        {
            using NativeString i = id;
            return EditorGUI_BeginPopup(i.Data);
        }

        internal static bool MenuItem(StringView label, bool selected = false, bool enabled = true)
        {
            using NativeString l = label;
            return EditorGUI_MenuItem(l.Data, selected, enabled);
        }

        internal static bool BeginMenu(StringView label, bool enabled = true)
        {
            using NativeString l = label;
            return EditorGUI_BeginMenu(l.Data, enabled);
        }

        internal static void OpenPopup(StringView id)
        {
            using NativeString i = id;
            EditorGUI_OpenPopup(i.Data);
        }

        public static bool FloatRangeField(StringView label, StringView tooltip, ref float currentMin, ref float currentMax, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f)
        {
            using NativeString l = label;
            using NativeString t = tooltip;

            fixed (float* min = &currentMin)
            fixed (float* max = &currentMax)
            {
                return EditorGUI_FloatRangeField(l.Data, t.Data, min, max, speed, minValue, maxValue);
            }
        }

        public static bool BeginTreeNode(StringView label, bool isLeaf = false, bool openOnArrow = false, bool openOnDoubleClick = false, bool selected = false, bool showBackground = false, bool defaultOpen = false, bool spanWidth = true)
        {
            using NativeString l = label;
            return EditorGUI_BeginTreeNode(l.Data, isLeaf, openOnArrow, openOnDoubleClick, selected, showBackground, defaultOpen, spanWidth);
        }

        public static bool IsTreeNodeOpen(StringView id)
        {
            using NativeString i = id;
            return EditorGUI_IsTreeNodeOpen(i.Data);
        }

        public static ItemClickResult IsItemClicked(MouseButton button, ItemClickOptions options)
        {
            return EditorGUI_IsItemClicked((int)button, options);
        }

        public static ItemClickResult IsTreeNodeClicked(bool isOpen, bool isLeaf)
        {
            ItemClickOptions options = ItemClickOptions.TreeNodeItem;
            if (isOpen) options |= ItemClickOptions.TreeNodeIsOpen;
            if (isLeaf) options |= ItemClickOptions.TreeNodeIsLeaf;

            ItemClickResult result1 = IsItemClicked(MouseButton.Left, options);
            ItemClickResult result2 = IsItemClicked(MouseButton.Right, options | ItemClickOptions.IgnorePopup); // 在 item 上打开 context menu 也算 click
            return (ItemClickResult)Math.Max((int)result1, (int)result2);
        }

        public static bool IsWindowClicked(MouseButton button, bool ignorePopup)
        {
            return EditorGUI_IsWindowClicked((int)button, ignorePopup);
        }

        public static bool IsWindowClicked()
        {
            return IsWindowClicked(MouseButton.Left, ignorePopup: false)
                || IsWindowClicked(MouseButton.Right, ignorePopup: true); // 在 window 上打开 context menu 也算 click
        }

        internal static bool BeginPopupContextItem(string id = "")
        {
            using NativeString i = id;
            return EditorGUI_BeginPopupContextItem(i.Data);
        }

        public static void DrawTexture(Texture texture)
        {
            EditorGUI_DrawTexture(texture.NativePtr);
        }

        public static bool Button(StringView label)
        {
            using NativeString l = label;
            return EditorGUI_Button(l.Data);
        }

        public static bool ButtonRight(StringView label)
        {
            using NativeString l = label;

            // https://github.com/ocornut/imgui/issues/4157

            float width = EditorGUI_CalcButtonWidth(l.Data);
            EditorGUI_SetCursorPosX(EditorGUI_GetCursorPosX() + EditorGUI_GetContentRegionAvail().X - width);
            return EditorGUI_Button(l.Data);
        }

        public static float CalcButtonWidth(StringView label)
        {
            using NativeString l = label;
            return EditorGUI_CalcButtonWidth(l.Data);
        }

        public static Vector2 ContentRegionAvailable => EditorGUI_GetContentRegionAvail();

        public static Vector2 ItemSpacing => EditorGUI_GetItemSpacing();

        public static float CursorPosX
        {
            get => EditorGUI_GetCursorPosX();
            set => EditorGUI_SetCursorPosX(value);
        }

        public static bool BeginAssetTreeNode(StringView label, StringView assetPath, StringView assetGuid, bool isLeaf = false, bool openOnArrow = false, bool openOnDoubleClick = false, bool selected = false, bool showBackground = false, bool defaultOpen = false, bool spanWidth = true)
        {
            using NativeString l = label;
            using NativeString a = assetPath;
            using NativeString ag = assetGuid;
            return EditorGUI_BeginAssetTreeNode(l.Data, a.Data, ag.Data, isLeaf, openOnArrow, openOnDoubleClick, selected, showBackground, defaultOpen, spanWidth);
        }

        public static bool MarchObjectField<T>(StringView label, StringView tooltip, ref T? asset) where T : MarchObject
        {
            MarchObject? obj = asset;

            if (MarchObjectField(label, tooltip, typeof(T), ref obj))
            {
                asset = (T?)obj;
                return true;
            }

            return false;
        }

        private enum MarchObjectState : int
        {
            Null = 0,
            Persistent = 1,
            Temporary = 2
        };

        public static bool MarchObjectField(StringView label, StringView tooltip, Type assetType, ref MarchObject? asset)
        {
            MarchObjectState state;
            string? path;

            if (asset == null)
            {
                state = MarchObjectState.Null;
                path = string.Empty;
            }
            else if (asset.PersistentGuid == null)
            {
                state = MarchObjectState.Temporary;
                path = string.Empty;
            }
            else
            {
                state = MarchObjectState.Persistent;
                path = AssetManager.GetPathByGuid(asset.PersistentGuid);

                if (path == null)
                {
                    LabelField(label, tooltip, "Asset is invalid");
                    return false;
                }
            }

            using NativeString l = label;
            using NativeString t = tooltip;
            using NativeString ty = assetType.Name;
            using NativeString p = path;
            nint pNewGuid = nint.Zero;

            if (EditorGUI_MarchObjectField(l.Data, t.Data, ty.Data, p.Data, &pNewGuid, state))
            {
                string newGuid = NativeString.GetAndFree(pNewGuid);
                MarchObject? newAsset = AssetManager.LoadByGuid<MarchObject>(newGuid);

                if (newAsset != asset)
                {
                    if (newAsset == null || assetType.IsAssignableFrom(newAsset.GetType()))
                    {
                        asset = newAsset;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }

            return false;
        }

        public static float CollapsingHeaderOuterExtend => EditorGUI_GetCollapsingHeaderOuterExtend();

        #region PropertyField

        private static readonly DrawerCache<IPropertyDrawer> s_PropertyDrawerCache = new(typeof(IPropertyDrawerFor<>));

        /// <summary>
        /// 
        /// </summary>
        /// <param name="label">以 <c>##</c> 开头可以隐藏标签</param>
        /// <param name="tooltip"></param>
        /// <param name="property"></param>
        /// <returns></returns>
        public static bool PropertyField(string label, string tooltip, in EditorProperty property)
        {
            using (new GroupScope())
            {
                Type? propertyType = property.PropertyType;

                if (propertyType == null)
                {
                    LabelField(label, tooltip, "Can not determine property type");
                    return false;
                }

                if (s_PropertyDrawerCache.TryGetSharedInstance(propertyType, out IPropertyDrawer? drawer))
                {
                    return drawer.Draw(label, tooltip, in property);
                }

                if (PersistentManager.ResolveJsonContract(propertyType) is JsonObjectContract contract)
                {
                    // Draw as a nested object
                    object? nestedTarget = property.GetValue<object>();

                    if (nestedTarget == null)
                    {
                        LabelField(label, tooltip, "Null");
                        return false;
                    }

                    bool isLabelHidden = IsLabelHidden(label);

                    if (!isLabelHidden && !Foldout(label, tooltip))
                    {
                        return false;
                    }

                    using (new IDScope(label))
                    using (new IndentedScope(isLabelHidden ? 0u : 1u))
                    {
                        bool changed = ObjectPropertyFields(nestedTarget, contract);

                        if (changed && propertyType.IsValueType)
                        {
                            // 值类型转成 object 会装箱生成一份拷贝，所以需要重新设置回去，把在拷贝上的修改同步到原对象上
                            property.SetValue(nestedTarget);
                        }

                        return changed;
                    }
                }

                // Fallback
                using (new DisabledScope())
                {
                    LabelField(label, tooltip, $"Type {propertyType} is not supported");
                    return false;
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="label">以 <c>##</c> 开头可以隐藏标签</param>
        /// <param name="property"></param>
        /// <returns></returns>
        public static bool PropertyField(string label, in EditorProperty property)
        {
            return PropertyField(label, property.Tooltip, in property);
        }

        public static bool PropertyField(in EditorProperty property)
        {
            return PropertyField(property.DisplayName, in property);
        }

        public static bool ObjectPropertyFields(object target, JsonObjectContract contract, out int propertyCount)
        {
            bool changed = false;
            propertyCount = 0;

            for (int i = 0; i < contract.Properties.Count; i++)
            {
                EditorProperty property = contract.GetEditorProperty(target, i);

                if (property.IsHidden)
                {
                    continue;
                }

                using (new IDScope(i))
                {
                    changed |= PropertyField(in property);
                    propertyCount++;
                }
            }

            return changed;
        }

        public static bool ObjectPropertyFields(object target, JsonObjectContract contract)
        {
            return ObjectPropertyFields(target, contract, out _);
        }

        public static bool ObjectPropertyFields(object target, out int propertyCount)
        {
            Type type = target.GetType();

            if (PersistentManager.ResolveJsonContract(type) is not JsonObjectContract contract)
            {
                LabelField($"##{nameof(ObjectPropertyFields)}Error", string.Empty,
                    $"Failed to resolve json object contract for {type}.");
                propertyCount = 0;
                return false;
            }

            return ObjectPropertyFields(target, contract, out propertyCount);
        }

        public static bool ObjectPropertyFields(object target)
        {
            return ObjectPropertyFields(target, out _);
        }

        private static bool IsLabelHidden(string label)
        {
            return label.StartsWith("##");
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

        public readonly ref struct GroupScope
        {
            public GroupScope()
            {
                EditorGUI_BeginGroup();
            }

            public void Dispose()
            {
                EditorGUI_EndGroup();
            }
        }

        #endregion

        #region Native

        [NativeFunction]
        private static partial void EditorGUI_PrefixLabel(nint label, nint tooltip);

        [NativeFunction]
        private static partial bool EditorGUI_IntField(nint label, nint tooltip, int* v, float speed, int minValue, int maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_FloatField(nint label, nint tooltip, float* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_Vector2Field(nint label, nint tooltip, Vector2* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_Vector3Field(nint label, nint tooltip, Vector3* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_Vector4Field(nint label, nint tooltip, Vector4* v, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_ColorField(nint label, nint tooltip, Color* v);

        [NativeFunction]
        private static partial bool EditorGUI_FloatSliderField(nint label, nint tooltip, float* v, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_CollapsingHeader(nint label, bool defaultOpen);

        [NativeFunction]
        private static partial bool EditorGUI_Combo(nint label, nint tooltip, int* currentItem, nint itemsSeparatedByZeros);

        [NativeFunction]
        private static partial bool EditorGUI_CenterButton(nint label, float width);

        [NativeFunction(Name = "EditorGUI_Space")]
        public static partial void Space();

        [NativeFunction]
        private static partial void EditorGUI_SeparatorText(nint label);

        [NativeFunction]
        private static partial bool EditorGUI_TextField(nint label, nint tooltip, nint text, nint* outNewText, nint charBlacklist);

        [NativeFunction]
        private static partial bool EditorGUI_Checkbox(nint label, nint tooltip, bool* v);

        [NativeFunction]
        private static partial void EditorGUI_BeginDisabled(bool disabled);

        [NativeFunction]
        private static partial void EditorGUI_EndDisabled();

        [NativeFunction]
        private static partial void EditorGUI_LabelField(nint label1, nint tooltip, nint label2);

        [NativeFunction]
        private static partial void EditorGUI_PushIDString(nint id);

        [NativeFunction]
        private static partial void EditorGUI_PushIDInt(int id);

        [NativeFunction]
        private static partial void EditorGUI_PopID();

        [NativeFunction]
        private static partial bool EditorGUI_Foldout(nint label, nint tooltip, bool defaultOpen);

        [NativeFunction]
        private static partial bool EditorGUI_FoldoutClosable(nint label, nint tooltip, bool* pVisible);

        [NativeFunction]
        private static partial void EditorGUI_Indent(uint count);

        [NativeFunction]
        private static partial void EditorGUI_Unindent(uint count);

        [NativeFunction(Name = "EditorGUI_SameLine")]
        public static partial void SameLine(float offsetFromStartX = 0.0f, float spacing = -1.0f);

        [NativeFunction]
        private static partial Vector2 EditorGUI_GetContentRegionAvail();

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

        [NativeFunction]
        private static partial bool EditorGUI_FloatRangeField(nint label, nint tooltip, float* currentMin, float* currentMax, float speed, float minValue, float maxValue);

        [NativeFunction]
        private static partial bool EditorGUI_BeginTreeNode(nint label, bool isLeaf, bool openOnArrow, bool openOnDoubleClick, bool selected, bool showBackground, bool defaultOpen, bool spanWidth);

        /// <summary>
        /// only call EndTreeNode() if BeginTreeNode() returns true!
        /// </summary>
        [NativeFunction(Name = "EditorGUI_EndTreeNode")]
        public static partial void EndTreeNode();

        [NativeFunction]
        private static partial bool EditorGUI_IsTreeNodeOpen(nint id);

        [NativeFunction]
        private static partial ItemClickResult EditorGUI_IsItemClicked(int button, ItemClickOptions options);

        [NativeFunction]
        private static partial bool EditorGUI_IsWindowClicked(int button, bool ignorePopup);

        [NativeFunction(Name = "EditorGUI_BeginPopupContextWindow")]
        internal static partial bool BeginPopupContextWindow();

        [NativeFunction]
        private static partial bool EditorGUI_BeginPopupContextItem(nint id);

        [NativeFunction]
        private static partial void EditorGUI_DrawTexture(nint texture);

        [NativeFunction]
        private static partial bool EditorGUI_Button(nint label);

        [NativeFunction]
        private static partial void EditorGUI_BeginGroup();

        [NativeFunction]
        private static partial void EditorGUI_EndGroup();

        [NativeFunction]
        private static partial float EditorGUI_CalcButtonWidth(nint label);

        [NativeFunction]
        private static partial Vector2 EditorGUI_GetItemSpacing();

        [NativeFunction]
        private static partial float EditorGUI_GetCursorPosX();

        [NativeFunction]
        private static partial void EditorGUI_SetCursorPosX(float localX);

        [NativeFunction]
        private static partial bool EditorGUI_BeginAssetTreeNode(nint label, nint assetPath, nint assetGuid, bool isLeaf, bool openOnArrow, bool openOnDoubleClick, bool selected, bool showBackground, bool defaultOpen, bool spanWidth);

        [NativeFunction]
        private static partial bool EditorGUI_MarchObjectField(nint label, nint tooltip, nint type, nint persistentPath, nint* outNewPersistentGuid, MarchObjectState currentObjectState);

        [NativeFunction]
        private static partial float EditorGUI_GetCollapsingHeaderOuterExtend();

        [NativeFunction(Name = "EditorGUI_BeginMainMenuBar")]
        public static partial bool BeginMainMenuBar();

        /// <summary>
        /// only call EndMainMenuBar() if BeginMainMenuBar() returns true!
        /// </summary>
        [NativeFunction(Name = "EditorGUI_EndMainMenuBar")]
        public static partial void EndMainMenuBar();

        #endregion
    }
}

using March.Core;
using March.Core.Interop;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.AssetPipeline;
using Newtonsoft.Json.Serialization;
using System.Numerics;
using System.Text;

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
            AllowWhenDisabled = 1 << 4,
        }

        public enum ItemClickResult : int
        {
            False = 0,
            TreeNodeArrow = 1,
            True = 2,
        }

        [NativeMethod]
        public static partial void PrefixLabel(StringLike label, StringLike tooltip);

        [NativeMethod]
        public static partial bool IntField(StringLike label, StringLike tooltip, ref int value, int speed = 1, int minValue = 0, int maxValue = 0);

        [NativeMethod]
        public static partial bool FloatField(StringLike label, StringLike tooltip, ref float value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f);

        [NativeMethod]
        public static partial bool Vector2Field(StringLike label, StringLike tooltip, ref Vector2 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f);

        [NativeMethod]
        public static partial bool Vector3Field(StringLike label, StringLike tooltip, ref Vector3 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f);

        [NativeMethod]
        public static partial bool Vector4Field(StringLike label, StringLike tooltip, ref Vector4 value, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f);

        [NativeMethod]
        public static partial bool ColorField(StringLike label, StringLike tooltip, ref Color value);

        [NativeMethod]
        public static partial bool FloatSliderField(StringLike label, StringLike tooltip, ref float value, float minValue, float maxValue);

        [NativeMethod]
        public static partial bool CollapsingHeader(StringLike label, bool defaultOpen = false);

        #region Enum

        [NativeMethod]
        private static partial bool Combo(StringLike label, StringLike tooltip, ref int currentItem, StringLike itemsSeparatedByZeros);

        private static void AppendEnumNames(StringBuilder builder, ReadOnlySpan<string> names)
        {
            for (int i = 0; i < names.Length; i++)
            {
                if (i > 0)
                {
                    builder.Append('\0');
                }

                builder.Append(names[i]);
            }

            builder.Append('\0', 2);
        }

        public static bool EnumField(StringLike label, StringLike tooltip, ref Enum value)
        {
            TypeCache.GetInspectorEnumData(value.GetType(), out ReadOnlySpan<string> names, out ReadOnlySpan<Enum> values);

            int index = -1;

            for (int i = 0; i < values.Length; i++)
            {
                if (values[i].Equals(value))
                {
                    index = i;
                    break;
                }
            }

            using var itemsBuilder = StringBuilderPool.Get();
            AppendEnumNames(itemsBuilder, names);

            if (Combo(label, tooltip, ref index, itemsBuilder) && index >= 0 && index < values.Length)
            {
                value = values[index];
                return true;
            }

            return false;
        }

        public static bool EnumField<T>(StringLike label, StringLike tooltip, ref T value) where T : struct, Enum
        {
            TypeCache.GetInspectorEnumData(out ReadOnlySpan<string> names, out ReadOnlySpan<T> values);

            int index = -1;

            for (int i = 0; i < values.Length; i++)
            {
                if (EqualityComparer<T>.Default.Equals(values[i], value))
                {
                    index = i;
                    break;
                }
            }

            using var itemsBuilder = StringBuilderPool.Get();
            AppendEnumNames(itemsBuilder, names);

            if (Combo(label, tooltip, ref index, itemsBuilder) && index >= 0 && index < values.Length)
            {
                value = values[index];
                return true;
            }

            return false;
        }

        #endregion

        [NativeMethod]
        public static partial bool CenterButton(StringLike label, float width);

        [NativeMethod]
        public static partial void CenterText(StringLike text);

        [NativeMethod]
        public static partial void Space();

        [NativeMethod]
        public static partial void SeparatorText(StringLike label);

        [NativeMethod]
        public static partial void SameLine(float offsetFromStartX = 0.0f, float spacing = -1.0f);

        [NativeMethod]
        public static partial void SetNextItemWidth(float width);

        [NativeMethod]
        public static partial void Separator();

        #region Text

        [NativeMethod]
        private static partial bool TextField(StringLike label, StringLike tooltip, StringLike text, ref nint newText, StringLike charBlacklist);

        public static bool TextField(StringLike label, StringLike tooltip, ref string text)
        {
            return TextField(label, tooltip, ref text, string.Empty);
        }

        public static bool TextField(StringLike label, StringLike tooltip, ref string text, StringLike charBlacklist)
        {
            nint newText = nint.Zero;

            if (TextField(label, tooltip, text, ref newText, charBlacklist))
            {
                text = NativeString.GetAndFree(newText);
                return true;
            }

            return false;
        }

        #endregion

        [NativeMethod]
        public static partial bool Checkbox(StringLike label, StringLike tooltip, ref bool value);

        [NativeMethod]
        public static partial void LabelField(StringLike label1, StringLike tooltip, StringLike label2);

        [NativeMethod]
        public static partial bool Foldout(StringLike label, StringLike tooltip, bool defaultOpen = false);

        [NativeMethod]
        public static partial bool FoldoutClosable(StringLike label, StringLike tooltip, ref bool visible);

        [NativeMethod]
        internal static partial bool BeginPopup(StringLike id);

        /// <summary>
        /// only call EndPopup() if BeginPopupXXX() returns true!
        /// </summary>
        [NativeMethod]
        internal static partial void EndPopup();

        [NativeMethod]
        internal static partial bool MenuItem(StringLike label, bool selected = false, bool enabled = true);

        [NativeMethod]
        internal static partial bool BeginMenu(StringLike label, bool enabled = true);

        /// <summary>
        /// only call EndMenu() if BeginMenu() returns true!
        /// </summary>
        [NativeMethod]
        internal static partial void EndMenu();

        [NativeMethod]
        internal static partial void OpenPopup(StringLike id);

        [NativeMethod]
        public static partial bool FloatRangeField(StringLike label, StringLike tooltip, ref float currentMin, ref float currentMax, float speed = 0.1f, float minValue = 0.0f, float maxValue = 0.0f);

        [NativeMethod]
        public static partial bool BeginTreeNode(StringLike label, bool isLeaf = false, bool openOnArrow = false, bool openOnDoubleClick = false, bool selected = false, bool showBackground = false, bool defaultOpen = false, bool spanWidth = true);

        /// <summary>
        /// only call EndTreeNode() if BeginTreeNode() returns true!
        /// </summary>
        [NativeMethod]
        public static partial void EndTreeNode();

        [NativeMethod]
        public static partial bool IsTreeNodeOpen(StringLike id, bool defaultValue);

        [NativeMethod]
        public static partial ItemClickResult IsItemClicked(MouseButton button, ItemClickOptions options);

        public static ItemClickResult IsTreeNodeClicked(bool isOpen, bool isLeaf, bool allowWhenDisabled = false)
        {
            ItemClickOptions options = ItemClickOptions.TreeNodeItem;
            if (isOpen) options |= ItemClickOptions.TreeNodeIsOpen;
            if (isLeaf) options |= ItemClickOptions.TreeNodeIsLeaf;
            if (allowWhenDisabled) options |= ItemClickOptions.AllowWhenDisabled;

            ItemClickResult result1 = IsItemClicked(MouseButton.Left, options);
            ItemClickResult result2 = IsItemClicked(MouseButton.Right, options | ItemClickOptions.IgnorePopup); // 在 item 上打开 context menu 也算 click
            return (ItemClickResult)Math.Max((int)result1, (int)result2);
        }

        [NativeMethod]
        public static partial bool IsWindowClicked(MouseButton button, bool ignorePopup);

        public static bool IsWindowClicked()
        {
            return IsWindowClicked(MouseButton.Left, ignorePopup: false)
                || IsWindowClicked(MouseButton.Right, ignorePopup: true); // 在 window 上打开 context menu 也算 click
        }

        internal static bool BeginPopupContextItem()
        {
            return BeginPopupContextItem(string.Empty);
        }

        [NativeMethod]
        internal static partial bool BeginPopupContextItem(StringLike id);

        [NativeMethod]
        internal static partial bool BeginPopupContextWindow();

        [NativeMethod]
        internal static partial bool BeginMainMenuBar();

        /// <summary>
        /// only call EndMainMenuBar() if BeginMainMenuBar() returns true!
        /// </summary>
        [NativeMethod]
        internal static partial void EndMainMenuBar();

        [NativeMethod]
        public static partial void DrawTexture(Texture texture);

        [NativeMethod]
        public static partial bool Button(StringLike label);

        [NativeMethod]
        public static partial float CalcButtonWidth(StringLike label);

        [NativeProperty("ContentRegionAvail")]
        public static partial Vector2 ContentRegionAvailable { get; }

        [NativeProperty]
        public static partial Vector2 ItemSpacing { get; }

        [NativeProperty]
        public static partial float CursorPosX { get; set; }

        [NativeProperty]
        public static partial float CollapsingHeaderOuterExtend { get; }

        public static bool ButtonRight(StringLike label)
        {
            // https://github.com/ocornut/imgui/issues/4157

            CursorPosX += ContentRegionAvailable.X - CalcButtonWidth(label);
            return Button(label);
        }

        #region Object

        public static bool MarchObjectField<T>(StringLike label, StringLike tooltip, ref T? asset) where T : MarchObject
        {
            MarchObject? obj = asset;

            if (MarchObjectField(label, tooltip, typeof(T), ref obj))
            {
                asset = (T?)obj;
                return true;
            }

            return false;
        }

        public static bool MarchObjectField(StringLike label, StringLike tooltip, Type assetType, ref MarchObject? asset)
        {
            using var label2 = StringBuilderPool.Get();

            if (asset == null)
            {
                label2.Value.Append("None (").Append(assetType.Name).Append(')');
            }
            else if (asset.PersistentGuid == null)
            {
                label2.Value.Append("Runtime Object (").Append(assetType.Name).Append(')');
            }
            else
            {
                AssetImporter importer = AssetDatabase.GetAssetImporter(asset)!;
                label2.Value.AppendAssetPath(importer, asset.PersistentGuid);
            }

            LabelField(label, tooltip, label2);

            bool isChanged = false;

            if (DragDrop.BeginTarget(DragDropArea.Item, out MarchObject? newAsset, out bool isDelivery))
            {
                if (assetType.IsAssignableFrom(newAsset.GetType()))
                {
                    if (isDelivery && newAsset != asset)
                    {
                        asset = newAsset;
                        isChanged = true;
                    }

                    DragDrop.EndTarget(DragDropResult.Accept);
                }
                else
                {
                    DragDrop.EndTarget(DragDropResult.Reject);
                }
            }

            return isChanged;
        }

        #endregion

        #region Property

        private static readonly DrawerCache<IPropertyDrawer> s_PropertyDrawerCache = new(typeof(IPropertyDrawerFor<>));

        /// <summary>
        /// 
        /// </summary>
        /// <param name="label">以 <c>##</c> 开头可以隐藏标签</param>
        /// <param name="tooltip"></param>
        /// <param name="property"></param>
        /// <returns></returns>
        public static bool PropertyField(StringLike label, StringLike tooltip, in EditorProperty property)
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
        public static bool PropertyField(StringLike label, in EditorProperty property)
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
                using var label1 = StringBuilderPool.Get();
                label1.Value.Append("##").Append(nameof(ObjectPropertyFields)).Append("Error");

                using var label2 = StringBuilderPool.Get();
                label2.Value.Append("Failed to resolve json object contract for ").Append(type).Append('.');

                LabelField(label1, string.Empty, label2);
                propertyCount = 0;
                return false;
            }

            return ObjectPropertyFields(target, contract, out propertyCount);
        }

        public static bool ObjectPropertyFields(object target)
        {
            return ObjectPropertyFields(target, out _);
        }

        private static bool IsLabelHidden(StringLike label)
        {
            return label.Length >= 2 && label[0] == '#' && label[1] == '#';
        }

        #endregion

        #region Scope

        public readonly ref struct DisabledScope
        {
            private readonly bool m_AllowInteraction;

            public DisabledScope() : this(true) { }

            public DisabledScope(bool disabled, bool allowInteraction = false)
            {
                m_AllowInteraction = allowInteraction;
                BeginDisabled(disabled, m_AllowInteraction);
            }

            public void Dispose() => EndDisabled(m_AllowInteraction);
        }

        [NativeMethod]
        private static partial void BeginDisabled(bool disabled, bool allowInteraction);

        [NativeMethod]
        private static partial void EndDisabled(bool allowInteraction);

        public readonly ref struct IDScope
        {
            [Obsolete("Use other constructors", error: true)]
            public IDScope() { }

            public IDScope(StringLike id) => PushIDString(id);

            public IDScope(int id) => PushIDInt(id);

            public void Dispose() => PopID();
        }

        [NativeMethod]
        private static partial void PushIDString(StringLike id);

        [NativeMethod]
        private static partial void PushIDInt(int id);

        [NativeMethod]
        private static partial void PopID();

        public readonly ref struct IndentedScope
        {
            private readonly uint m_IndentCount;

            public IndentedScope() : this(1) { }

            public IndentedScope(uint count) => Indent(m_IndentCount = count);

            public void Dispose() => Unindent(m_IndentCount);
        }

        [NativeMethod]
        private static partial void Indent(uint count);

        [NativeMethod]
        private static partial void Unindent(uint count);

        public readonly ref struct GroupScope
        {
            public GroupScope() => BeginGroup();

            public void Dispose() => EndGroup();
        }

        [NativeMethod]
        private static partial void BeginGroup();

        [NativeMethod]
        private static partial void EndGroup();

        #endregion
    }
}

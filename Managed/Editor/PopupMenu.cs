using System.Diagnostics.CodeAnalysis;

namespace DX12Demo.Editor
{
    public class PopupMenu<T>(string popupId)
    {
        public delegate void MenuAction(ref T? arg);

        public delegate void MenuAction<TArg1>(ref T? arg, TArg1 arg1);

        public delegate TReturn MenuFunc<TReturn>(ref T? arg);

        private interface IMenuEntry
        {
            void Draw(ref T? arg);
        }

        private sealed class TrieNode
        {
            /// <summary>
            /// 保存所有子菜单
            /// </summary>
            public Dictionary<string, TrieNode> Children { get; } = new();

            /// <summary>
            /// 保存当前菜单的绘制顺序
            /// </summary>
            public List<IMenuEntry> MenuEntries { get; } = new();

            public TrieNode GetOrAddChild(string key)
            {
                if (!Children.TryGetValue(key, out TrieNode? child))
                {
                    child = new TrieNode();
                    Children.Add(key, child);

                    // 在子菜单第一次出现的地方绘制它
                    MenuEntries.Add(new SubMenuEntry { Label = key, Node = child });
                }

                return child;
            }

            public void DrawMenuEntries(ref T? arg)
            {
                for (int i = 0; i < MenuEntries.Count; i++)
                {
                    using (new EditorGUI.IDScope(i))
                    {
                        MenuEntries[i].Draw(ref arg);
                    }
                }
            }

            public void Clear()
            {
                Children.Clear();
                MenuEntries.Clear();
            }
        }

        private readonly TrieNode m_MenuTree = new(); // 字典树维护菜单项

        private bool FindEntryList(string path, [NotNullWhen(true)] out string? label, [NotNullWhen(true)] out List<IMenuEntry>? entries)
        {
            string[] segments = path.Split('/', StringSplitOptions.RemoveEmptyEntries);

            if (segments.Length == 0)
            {
                label = null;
                entries = null;
                return false;
            }

            TrieNode node = m_MenuTree;

            // 最后一个 segment 需要加到上一个 segment 的 MenuEntries 中，保存绘制顺序
            for (int i = 0; i < segments.Length - 1; i++)
            {
                node = node.GetOrAddChild(segments[i]);
            }

            label = segments[^1];
            entries = node.MenuEntries;
            return true;
        }

        public void AddMenuItem(string path, MenuAction? callback = null, MenuFunc<bool>? selected = null, MenuFunc<bool>? enabled = null)
        {
            if (FindEntryList(path, out string? label, out List<IMenuEntry>? entries))
            {
                entries.Add(new MenuItemEntry
                {
                    Label = label,
                    Callback = callback,
                    Selected = selected,
                    Enabled = enabled,
                });
            }
        }

        public void AddCustomItem(string path, MenuAction<string> itemGUI)
        {
            if (FindEntryList(path, out string? label, out List<IMenuEntry>? entries))
            {
                entries.Add(new CustomItemEntry
                {
                    Label = label,
                    ItemGUI = itemGUI
                });
            }
        }

        public void AddSeparator(string path)
        {
            AddCustomItem($"{path}/---", (ref T? arg, string _) => EditorGUI.Separator());
        }

        public void AddSeparatorText(string path, string text)
        {
            AddCustomItem($"{path}/{text}", (ref T? arg, string label) => EditorGUI.SeparatorText(label));
        }

        public void Clear()
        {
            m_MenuTree.Clear();
        }

        public void Open()
        {
            EditorGUI.OpenPopup(popupId);
        }

        public T? Draw(T? arg = default)
        {
            if (EditorGUI.BeginPopup(popupId))
            {
                m_MenuTree.DrawMenuEntries(ref arg);
                EditorGUI.EndPopup();
            }

            return arg;
        }

        public T? DoWindowContext(T? arg = default)
        {
            if (EditorGUI.BeginPopupContextWindow())
            {
                m_MenuTree.DrawMenuEntries(ref arg);
                EditorGUI.EndPopup();
            }

            return arg;
        }

        public T? DoItemContext(T? arg = default)
        {
            if (EditorGUI.BeginPopupContextItem())
            {
                m_MenuTree.DrawMenuEntries(ref arg);
                EditorGUI.EndPopup();
            }

            return arg;
        }

        public T? DoItemContext(string customIdAppend, T? arg = default)
        {
            if (EditorGUI.BeginPopupContextItem(popupId + customIdAppend))
            {
                m_MenuTree.DrawMenuEntries(ref arg);
                EditorGUI.EndPopup();
            }

            return arg;
        }

        private sealed class MenuItemEntry : IMenuEntry
        {
            public required string Label { get; init; }

            public required MenuAction? Callback { get; init; }

            public required MenuFunc<bool>? Selected { get; init; }

            public required MenuFunc<bool>? Enabled { get; init; }

            public void Draw(ref T? arg)
            {
                bool isSelected = Selected?.Invoke(ref arg) ?? false;
                bool isEnabled = (Enabled?.Invoke(ref arg) ?? true) && (Callback != null);

                if (EditorGUI.MenuItem(Label, isSelected, isEnabled))
                {
                    Callback?.Invoke(ref arg);
                }
            }
        }

        private sealed class SubMenuEntry : IMenuEntry
        {
            public required string Label { get; init; }

            public required TrieNode Node { get; init; }

            public void Draw(ref T? arg)
            {
                if (Node.MenuEntries.Count > 0 && EditorGUI.BeginMenu(Label))
                {
                    Node.DrawMenuEntries(ref arg);
                    EditorGUI.EndMenu();
                }
            }
        }

        private sealed class CustomItemEntry : IMenuEntry
        {
            public required string Label { get; init; }

            public required MenuAction<string> ItemGUI { get; init; }

            public void Draw(ref T? arg)
            {
                ItemGUI(ref arg, Label);
            }
        }
    }

    public class PopupMenu(string popupId) : PopupMenu<object>(popupId) { }
}

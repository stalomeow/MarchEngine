using System.Diagnostics.CodeAnalysis;

namespace DX12Demo.Editor
{
    public class PopupMenu(string popupId)
    {
        private interface IMenuEntry
        {
            void Draw();
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

            public void DrawMenuEntries()
            {
                for (int i = 0; i < MenuEntries.Count; i++)
                {
                    using (new EditorGUI.IDScope(i))
                    {
                        MenuEntries[i].Draw();
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

        public void AddMenuItem(string path, Action? callback = null, Func<bool>? selected = null, Func<bool>? enabled = null)
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

        public void AddCustomItem(string path, Action<string> itemGUI)
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
            AddCustomItem($"{path}/---", _ => EditorGUI.Separator());
        }

        public void AddSeparatorText(string path, string text)
        {
            AddCustomItem($"{path}/{text}", EditorGUI.SeparatorText);
        }

        public void Clear()
        {
            m_MenuTree.Clear();
        }

        public void Open()
        {
            EditorGUI.OpenPopup(popupId);
        }

        public void Draw()
        {
            if (EditorGUI.BeginPopup(popupId))
            {
                m_MenuTree.DrawMenuEntries();
                EditorGUI.EndPopup();
            }
        }

        public void DoWindowContext()
        {
            if (EditorGUI.BeginPopupContextWindow())
            {
                m_MenuTree.DrawMenuEntries();
                EditorGUI.EndPopup();
            }
        }

        public void DoItemContext()
        {
            if (EditorGUI.BeginPopupContextItem())
            {
                m_MenuTree.DrawMenuEntries();
                EditorGUI.EndPopup();
            }
        }

        private sealed class MenuItemEntry : IMenuEntry
        {
            public required string Label { get; init; }

            public required Action? Callback { get; init; }

            public required Func<bool>? Selected { get; init; }

            public required Func<bool>? Enabled { get; init; }

            public void Draw()
            {
                bool isSelected = Selected?.Invoke() ?? false;
                bool isEnabled = (Enabled?.Invoke() ?? true) && (Callback != null);

                if (EditorGUI.MenuItem(Label, isSelected, isEnabled))
                {
                    Callback?.Invoke();
                }
            }
        }

        private sealed class SubMenuEntry : IMenuEntry
        {
            public required string Label { get; init; }

            public required TrieNode Node { get; init; }

            public void Draw()
            {
                if (Node.MenuEntries.Count <= 0)
                {
                    return;
                }

                if (EditorGUI.BeginMenu(Label))
                {
                    Node.DrawMenuEntries();
                    EditorGUI.EndMenu();
                }
            }
        }

        private sealed class CustomItemEntry : IMenuEntry
        {
            public required string Label { get; init; }

            public required Action<string> ItemGUI { get; init; }

            public void Draw()
            {
                ItemGUI(Label);
            }
        }
    }
}

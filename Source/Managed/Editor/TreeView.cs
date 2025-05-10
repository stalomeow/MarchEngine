using March.Core;
using March.Core.Pool;
using System.Text;

namespace March.Editor
{
    public readonly ref struct TreeViewItemDesc
    {
        public required bool IsDisabled { get; init; }

        public required MarchObject? SelectionObject { get; init; }

        public bool IsOpenByDefault { get; init; }
    }

    public abstract class TreeView
    {
        private readonly struct ItemState
        {
            public required object? Parent { get; init; }

            public required int SiblingIndex { get; init; }

            public required bool IsOpen { get; init; }

            public required MarchObject? SelectionObject { get; init; }
        }

        private readonly List<object> m_Items = [];
        private readonly Dictionary<object, ItemState> m_ItemStates = [];
        private readonly List<EditorGUI.SelectionRequest> m_SelectionRequests = [];

        public void Draw()
        {
            try
            {
                using (new EditorGUI.TreeNodeMultiSelectScope(m_SelectionRequests))
                {
                    int count = GetChildCount(null);

                    for (int i = 0; i < count; i++)
                    {
                        DrawItem(GetChildItem(null, i), null, i);
                    }
                }

                foreach (EditorGUI.SelectionRequest request in m_SelectionRequests)
                {
                    HandleSelectionRequest(in request);
                }
            }
            finally
            {
                m_Items.Clear();
                m_ItemStates.Clear();
                m_SelectionRequests.Clear();
            }
        }

        private void DrawItem(object item, object? parent, int siblingIndex)
        {
            using (new EditorGUI.IDScope(siblingIndex))
            {
                using var label = StringBuilderPool.Get();
                TreeViewItemDesc desc = GetItemDesc(item, label);

                using (new EditorGUI.DisabledScope(desc.IsDisabled, allowInteraction: true))
                {
                    int itemIndex = m_Items.Count; // 当前 item 在 m_Items 中的索引，用于后面处理 selection requests
                    int childCount = GetChildCount(item);
                    bool isLeaf = childCount == 0;
                    bool isSelected = IsItemSelected(ref desc);
                    bool isOpen = EditorGUI.BeginTreeNode(label, itemIndex, isLeaf, isSelected, desc.IsOpenByDefault);

                    m_Items.Add(item);
                    m_ItemStates[item] = new ItemState
                    {
                        Parent = parent,
                        SiblingIndex = siblingIndex,
                        IsOpen = isOpen,
                        SelectionObject = desc.SelectionObject,
                    };

                    if (isOpen)
                    {
                        for (int i = 0; i < childCount; i++)
                        {
                            DrawItem(GetChildItem(item, i), item, i);
                        }

                        EditorGUI.EndTreeNode();
                    }
                }
            }
        }

        private void HandleSelectionRequest(in EditorGUI.SelectionRequest request)
        {
            switch (request.Type)
            {
                case EditorGUI.SelectionRequestType.Nop:
                    break;
                case EditorGUI.SelectionRequestType.SetAll:
                    SelectAllItems();
                    break;
                case EditorGUI.SelectionRequestType.ClearAll:
                    Selection.Clear(context: this);
                    break;
                case EditorGUI.SelectionRequestType.SetRange:
                    SelectItemRange(request.StartIndex, request.EndIndex, unselect: false);
                    break;
                case EditorGUI.SelectionRequestType.ClearRange:
                    SelectItemRange(request.StartIndex, request.EndIndex, unselect: true);
                    break;
                default:
                    throw new ArgumentOutOfRangeException($"Unknown selection request type: {request.Type}");
            }
        }

        private bool IsItemSelected(ref readonly TreeViewItemDesc desc)
        {
            return desc.SelectionObject != null
                && Selection.Context == this
                && Selection.Contains(desc.SelectionObject);
        }

        private void SelectAllItems()
        {
            foreach (KeyValuePair<object, ItemState> kv in m_ItemStates)
            {
                MarchObject? obj = kv.Value.SelectionObject;

                if (obj != null)
                {
                    Selection.Add(obj, context: this);
                }
            }
        }

        private void SelectItemRange(int startIndex, int endIndex, bool unselect)
        {
            object firstItem = m_Items[startIndex]; // inclusive
            object lastItem = m_Items[endIndex];    // inclusive

            for (object? item = firstItem; item != null; item = GetNextVisibleItem(item, lastItem))
            {
                MarchObject? obj = m_ItemStates[item].SelectionObject;

                if (obj != null)
                {
                    if (unselect)
                    {
                        Selection.Remove(obj, context: this);
                    }
                    else
                    {
                        Selection.Add(obj, context: this);
                    }
                }
            }
        }

        private object? GetNextVisibleItem(object item, object lastItem)
        {
            // 按照 dfs 的顺序，返回下一个可见的 item

            if (item == lastItem)
            {
                return null;
            }

            ItemState state = m_ItemStates[item];

            // 如果能继续向下遍历子结点，就返回第一个子结点
            if (state.IsOpen && GetChildCount(item) > 0)
            {
                return GetChildItem(item, 0);
            }

            // 如果没有子结点，就返回下一个兄弟结点
            while (true)
            {
                if (state.SiblingIndex + 1 < GetChildCount(state.Parent))
                {
                    return GetChildItem(state.Parent, state.SiblingIndex + 1);
                }

                if (state.Parent == null)
                {
                    return null;
                }

                state = m_ItemStates[state.Parent];
            }
        }

        // item 为 null 表示获取根结点
        protected abstract int GetChildCount(object? item);

        // item 为 null 表示获取根结点
        protected abstract object GetChildItem(object? item, int index);

        protected abstract TreeViewItemDesc GetItemDesc(object item, StringBuilder labelBuilder);
    }
}

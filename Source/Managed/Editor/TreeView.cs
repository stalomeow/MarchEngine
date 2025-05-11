using March.Core;
using March.Core.Pool;
using System.Numerics;
using System.Text;

namespace March.Editor
{
    public readonly ref struct TreeViewItemDesc
    {
        public required bool IsDisabled { get; init; }

        public required MarchObject? SelectionObject { get; init; }

        public bool IsOpenByDefault { get; init; }
    }

    public enum TreeViewDragDropPosition
    {
        AboveItem,
        OnItem,
        BelowItem,
    }

    public abstract class TreeView
    {
        private readonly struct ItemState
        {
            public required object? Parent { get; init; }

            public required MarchObject? SelectionObject { get; init; }
        }

        private readonly struct MoveRequest
        {
            public required object MovingItem { get; init; }

            public required object AnchorItem { get; init; }

            public required TreeViewDragDropPosition Position { get; init; }
        }

        private readonly List<object> m_Items = [];
        private readonly Dictionary<object, ItemState> m_ItemStates = [];
        private readonly List<EditorGUI.SelectionRequest> m_SelectionRequests = [];
        private readonly List<MoveRequest> m_MoveRequests = [];

        public void Draw()
        {
            try
            {
                // 为了实现更复杂的 Drag & Drop，需要手动绘制 ItemSpacing
                Vector2 originalSpacing = EditorGUI.ItemSpacing;
                Vector2 tempSpacing = new(originalSpacing.X, 0);

                using (new EditorGUI.ItemSpacingScope(tempSpacing))
                using (new EditorGUI.TreeNodeMultiSelectScope(m_SelectionRequests))
                {
                    int count = GetChildCount(null);

                    for (int i = 0; i < count; i++)
                    {
                        DrawItem(GetChildItem(null, i), null, i, originalSpacing.Y);
                    }
                }

                foreach (EditorGUI.SelectionRequest request in m_SelectionRequests)
                {
                    HandleSelectionRequest(in request);
                }

                // 移动会使 Item 的索引失效，所以放在最后处理
                foreach (MoveRequest request in m_MoveRequests)
                {
                    MoveItem(request.MovingItem, request.AnchorItem, request.Position);
                }
            }
            finally
            {
                m_Items.Clear();
                m_ItemStates.Clear();
                m_SelectionRequests.Clear();
                m_MoveRequests.Clear();
            }
        }

        private void DrawItem(object item, object? parent, int siblingIndex, float spacing)
        {
            using (new EditorGUI.IDScope(siblingIndex))
            {
                using var label = StringBuilderPool.Get();
                TreeViewItemDesc desc = GetItemDesc(item, label);

                using (new EditorGUI.DisabledScope(desc.IsDisabled, allowInteraction: true))
                {
                    int itemIndex = m_Items.Count; // 当前 item 在 m_Items 中的索引，用于后面处理 selection requests
                    m_Items.Add(item);
                    m_ItemStates[item] = new ItemState
                    {
                        Parent = parent,
                        SelectionObject = desc.SelectionObject,
                    };

                    if (siblingIndex == 0)
                    {
                        // 模拟一次 TreePush
                        using (new EditorGUI.IndentedScope())
                        {
                            DrawItemSpacing(spacing, item, TreeViewDragDropPosition.AboveItem);
                        }
                    }

                    int childCount = GetChildCount(item);
                    bool isLeaf = childCount == 0;
                    bool isSelected = IsItemSelected(ref desc);
                    bool isOpen = EditorGUI.BeginTreeNode(label, itemIndex, isLeaf, isSelected, desc.IsOpenByDefault);

                    HandleItemDragDrop(item);

                    if (!isOpen)
                    {
                        // 模拟一次 TreePush
                        using (new EditorGUI.IndentedScope())
                        {
                            DrawItemSpacing(spacing, item, TreeViewDragDropPosition.BelowItem);
                        }
                    }
                    else if (childCount == 0)
                    {
                        DrawItemSpacing(spacing, item, TreeViewDragDropPosition.BelowItem);
                    }

                    if (isOpen)
                    {
                        for (int i = 0; i < childCount; i++)
                        {
                            DrawItem(GetChildItem(item, i), item, i, spacing);
                        }

                        EditorGUI.EndTreeNode();
                    }
                }
            }
        }

        private void HandleItemDragDrop(object item)
        {
            if (DragDrop.BeginSource())
            {
                using var tooltip = StringBuilderPool.Get();
                GetItemDragDropTooltip(item, tooltip);

                DragDrop.EndSource(tooltip, item, this);
            }

            HandleDragDropTarget(item, TreeViewDragDropPosition.OnItem, DragDropResult.AcceptByRect);
        }

        private void DrawItemSpacing(float height, object item, TreeViewDragDropPosition position)
        {
            // 绘制 ItemSpacing
            EditorGUI.Dummy(EditorGUI.ContentRegionAvailable.X, height);
            HandleDragDropTarget(item, position, DragDropResult.AcceptByLine);
        }

        private void HandleDragDropTarget(object item, TreeViewDragDropPosition position, DragDropResult acceptResult)
        {
            if (!DragDrop.BeginTarget(DragDropArea.Item, out DragDropData data))
            {
                return;
            }

            if (data.Context == this)
            {
                if (CanMoveItem(data.Payload, item, position))
                {
                    if (data.IsDelivery)
                    {
                        m_MoveRequests.Add(new MoveRequest
                        {
                            AnchorItem = item,
                            MovingItem = data.Payload,
                            Position = position,
                        });
                    }

                    DragDrop.EndTarget(acceptResult);
                }
                else
                {
                    DragDrop.EndTarget(DragDropResult.Reject);
                }
            }
            else
            {
                // TODO 写一个虚函数来处理
                DragDrop.EndTarget(DragDropResult.Reject);
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
            // [startIndex, endIndex] 闭区间
            for (int i = startIndex; i <= endIndex; i++)
            {
                object item = m_Items[i];
                MarchObject? obj = m_ItemStates[item].SelectionObject;

                if (obj == null)
                {
                    continue;
                }

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

        // item 为 null 表示获取根结点
        protected abstract int GetChildCount(object? item);

        // item 为 null 表示获取根结点
        protected abstract object GetChildItem(object? item, int index);

        protected abstract TreeViewItemDesc GetItemDesc(object item, StringBuilder labelBuilder);

        protected abstract void GetItemDragDropTooltip(object item, StringBuilder tooltipBuilder);

        protected virtual bool CanMoveItem(object movingItem, object anchorItem, TreeViewDragDropPosition position)
        {
            // 如果把 movingItem 移动到自己或者子结点上，返回 false
            object? parent = anchorItem;

            while (parent != null)
            {
                if (parent == movingItem)
                {
                    return false;
                }

                parent = m_ItemStates[parent].Parent;
            }

            return true;
        }

        protected abstract void MoveItem(object movingItem, object anchorItem, TreeViewDragDropPosition position);
    }
}

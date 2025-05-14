using March.Core;
using March.Core.Pool;
using System.Numerics;
using System.Text;

namespace March.Editor
{
    public enum TreeViewDropPosition
    {
        AboveItem,
        OnItem,
        BelowItem,
    }

    public readonly ref struct TreeViewItemDesc
    {
        public required bool IsDisabled { get; init; }

        /// <summary>
        /// 如果不为 <see langword="null"/> 则可以被选择和拖拽
        /// </summary>
        public required MarchObject? EngineObject { get; init; }

        public bool IsOpenByDefault { get; init; }
    }

    public readonly struct TreeViewItemMoveData
    {
        public required object MovingItem { get; init; }

        public required object TargetItem { get; init; }

        public required TreeViewDropPosition Position { get; init; }
    }

    public readonly struct TreeViewExternalDropData
    {
        public required List<MarchObject> Objects { get; init; }

        public required List<string> Paths { get; init; }

        public required List<object> Extras { get; init; }

        public required object? Context { get; init; }

        public required object TargetItem { get; init; }

        public required TreeViewDropPosition Position { get; init; }
    }

    public abstract class TreeView
    {
        private readonly struct ItemData
        {
            public required object Item { get; init; }

            public required int? ParentIndex { get; init; }

            public required MarchObject? EngineObject { get; init; }
        }

        private readonly List<ItemData> m_Items = [];
        private readonly Dictionary<object, MarchObject> m_SelectionData = [];
        private readonly List<EditorGUI.SelectionRequest> m_SelectionRequests = [];
        private readonly List<TreeViewItemMoveData> m_MoveRequests = [];
        private readonly List<TreeViewExternalDropData> m_ExternalDropRequests = [];

        public void Draw()
        {
            try
            {
                FetchSelectionData();

                // 为了实现更复杂的 Drag & Drop，需要手动绘制 ItemSpacing
                Vector2 originalSpacing = EditorGUI.ItemSpacing;
                Vector2 tempSpacing = new(originalSpacing.X, 0);

                using (new EditorGUI.ItemSpacingScope(tempSpacing))
                using (new EditorGUI.TreeNodeMultiSelectScope(m_SelectionRequests))
                {
                    int count = GetChildCount(null);

                    for (int i = 0; i < count; i++)
                    {
                        object childItem = GetChildItem(null, i);
                        DrawItem(childItem, null, i, originalSpacing.Y);
                    }
                }

                foreach (EditorGUI.SelectionRequest request in m_SelectionRequests)
                {
                    HandleSelectionRequest(in request);
                }

                // 移动会使 Item 的索引失效，所以放在最后处理
                foreach (TreeViewItemMoveData moveData in m_MoveRequests)
                {
                    OnMoveItem(in moveData);
                }

                // 外部拖拽也可能使 Item 的索引失效，所以放在最后处理
                foreach (TreeViewExternalDropData dropData in m_ExternalDropRequests)
                {
                    OnHandleExternalDrop(in dropData);
                    FreeExternalDropData(in dropData);
                }
            }
            finally
            {
                m_Items.Clear();
                m_SelectionData.Clear();
                m_SelectionRequests.Clear();
                m_MoveRequests.Clear();
                m_ExternalDropRequests.Clear();
            }
        }

        private void DrawItem(object item, int? parentIndex, int siblingIndex, float spacing)
        {
            using (new EditorGUI.IDScope(siblingIndex))
            {
                using var label = StringBuilderPool.Get();
                TreeViewItemDesc desc = GetItemDesc(item, label);

                using (new EditorGUI.DisabledScope(desc.IsDisabled, allowInteraction: true))
                {
                    // 记录当前 item 在 m_Items 中的索引，用于后面处理 selection requests
                    int itemIndex = m_Items.Count;
                    m_Items.Add(new ItemData
                    {
                        Item = item,
                        ParentIndex = parentIndex,
                        EngineObject = desc.EngineObject,
                    });

                    if (siblingIndex == 0)
                    {
                        // 模拟一次 TreePush
                        using (new EditorGUI.IndentedScope())
                        {
                            DrawItemSpacing(spacing, item, itemIndex, TreeViewDropPosition.AboveItem);
                        }
                    }

                    int childCount = GetChildCount(item);
                    bool isLeaf = childCount == 0;
                    bool isSelected = m_SelectionData.ContainsKey(item);
                    bool isOpen = EditorGUI.BeginTreeNode(label, itemIndex, isLeaf, isSelected, desc.IsOpenByDefault);

                    HandleItemDragDrop(item, itemIndex, in desc);

                    if (!isOpen)
                    {
                        // 模拟一次 TreePush
                        using (new EditorGUI.IndentedScope())
                        {
                            DrawItemSpacing(spacing, item, itemIndex, TreeViewDropPosition.BelowItem);
                        }
                    }
                    else
                    {
                        if (childCount == 0)
                        {
                            DrawItemSpacing(spacing, item, itemIndex, TreeViewDropPosition.BelowItem);
                        }

                        for (int i = 0; i < childCount; i++)
                        {
                            object childItem = GetChildItem(item, i);
                            DrawItem(childItem, itemIndex, i, spacing);
                        }

                        EditorGUI.EndTreeNode();
                    }
                }
            }
        }

        private void HandleItemDragDrop(object item, int itemIndex, in TreeViewItemDesc desc)
        {
            if (desc.EngineObject != null && DragDrop.BeginSource())
            {
                DragDrop.Context = this;

                if (m_SelectionData.ContainsKey(item))
                {
                    // 如果拖动了一个选中的对象，拖动所有选中的对象
                    foreach (KeyValuePair<object, MarchObject> kv in m_SelectionData)
                    {
                        DragDrop.Objects.Add(kv.Value);
                        DragDrop.Extras.Add(kv.Key);
                    }
                }
                else
                {
                    DragDrop.Objects.Add(desc.EngineObject);
                    DragDrop.Extras.Add(item);
                }

                using var tooltip = StringBuilderPool.Get();
                GetDragTooltip(DragDrop.Extras, tooltip);
                DragDrop.EndSource(tooltip);
            }

            HandleDragDropTarget(item, itemIndex, TreeViewDropPosition.OnItem, DragDropResult.AcceptByRect);
        }

        private void DrawItemSpacing(float height, object item, int itemIndex, TreeViewDropPosition position)
        {
            // 利用 Dummy 绘制 ItemSpacing
            EditorGUI.Dummy(EditorGUI.ContentRegionAvailable.X, height);
            HandleDragDropTarget(item, itemIndex, position, DragDropResult.AcceptByLine);
        }

        private void HandleDragDropTarget(object item, int itemIndex, TreeViewDropPosition position, DragDropResult acceptResult)
        {
            if (!DragDrop.BeginTarget(DragDropArea.Item))
            {
                return;
            }

            if (DragDrop.Context == this)
            {
                bool accept = true;

                // 检查是否全部可以移动
                foreach (object movingItem in DragDrop.Extras)
                {
                    var data = new TreeViewItemMoveData
                    {
                        MovingItem = movingItem,
                        TargetItem = item,
                        Position = position
                    };

                    // 不允许将结点移动到自己或者子结点上
                    if (IsChildOf(itemIndex, movingItem) || !CanMoveItem(data))
                    {
                        accept = false;
                        break;
                    }
                }

                if (accept)
                {
                    if (DragDrop.IsDelivery)
                    {
                        foreach (object movingItem in DragDrop.Extras)
                        {
                            m_MoveRequests.Add(new TreeViewItemMoveData
                            {
                                MovingItem = movingItem,
                                TargetItem = item,
                                Position = position
                            });
                        }
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
                var data = new TreeViewExternalDropData
                {
                    Objects = DragDrop.Objects,
                    Paths = DragDrop.Paths,
                    Extras = DragDrop.Extras,
                    Context = DragDrop.Context,
                    TargetItem = item,
                    Position = position,
                };

                if (CanHandleExternalDrop(in data))
                {
                    if (DragDrop.IsDelivery)
                    {
                        // 需要把数据复制一份，避免之后处理时数据失效
                        m_ExternalDropRequests.Add(CopyExternalDropData(in data));
                    }

                    DragDrop.EndTarget(acceptResult);
                }
                else
                {
                    DragDrop.EndTarget(DragDropResult.Reject);
                }
            }
        }

        private static TreeViewExternalDropData CopyExternalDropData(in TreeViewExternalDropData data)
        {
            List<MarchObject> objects = ListPool<MarchObject>.Shared.Rent();
            List<string> paths = ListPool<string>.Shared.Rent();
            List<object> extras = ListPool<object>.Shared.Rent();

            objects.AddRange(data.Objects);
            paths.AddRange(data.Paths);
            extras.AddRange(data.Extras);

            return new TreeViewExternalDropData
            {
                Objects = objects,
                Paths = paths,
                Extras = extras,
                Context = data.Context,
                TargetItem = data.TargetItem,
                Position = data.Position,
            };
        }

        private static void FreeExternalDropData(in TreeViewExternalDropData data)
        {
            ListPool<MarchObject>.Shared.Return(data.Objects);
            ListPool<string>.Shared.Return(data.Paths);
            ListPool<object>.Shared.Return(data.Extras);
        }

        private bool IsChildOf(int childItemIndex, object parentItem)
        {
            int? index = childItemIndex;

            while (index != null)
            {
                ItemData data = m_Items[index.Value];

                if (data.Item == parentItem)
                {
                    return true;
                }

                index = data.ParentIndex;
            }

            return false;
        }

        private void FetchSelectionData()
        {
            foreach (MarchObject obj in Selection.Objects)
            {
                object? item = GetItemByEngineObject(obj);

                if (item != null)
                {
                    m_SelectionData.Add(item, obj);
                }
            }
        }

        private void HandleSelectionRequest(in EditorGUI.SelectionRequest request)
        {
            if (request.Type == EditorGUI.SelectionRequestType.ClearAll)
            {
                Selection.Objects.Clear();
                m_SelectionData.Clear();
            }
            else if (request.Type == EditorGUI.SelectionRequestType.SetAll)
            {
                foreach (ItemData data in m_Items)
                {
                    if (data.EngineObject != null && m_SelectionData.TryAdd(data.Item, data.EngineObject))
                    {
                        Selection.Objects.Add(data.EngineObject);
                    }
                }
            }
            else if (request.Type == EditorGUI.SelectionRequestType.ClearRange)
            {
                SelectItemRange(request.StartIndex, request.EndIndex, unselect: true);
            }
            else if (request.Type == EditorGUI.SelectionRequestType.SetRange)
            {
                SelectItemRange(request.StartIndex, request.EndIndex, unselect: false);
            }
            else
            {
                throw new NotSupportedException($"Unsupported selection request type: {request.Type}");
            }
        }

        private void SelectItemRange(int startIndex, int endIndex, bool unselect)
        {
            // 闭区间
            for (int i = startIndex; i <= endIndex; i++)
            {
                ItemData data = m_Items[i];

                if (data.EngineObject == null)
                {
                    continue;
                }

                if (unselect)
                {
                    if (m_SelectionData.Remove(data.Item))
                    {
                        Selection.Objects.Remove(data.EngineObject);
                    }
                }
                else
                {
                    if (m_SelectionData.TryAdd(data.Item, data.EngineObject))
                    {
                        Selection.Objects.Add(data.EngineObject);
                    }
                }
            }
        }

        // item 为 null 表示获取根结点
        protected abstract int GetChildCount(object? item);

        // item 为 null 表示获取根结点
        protected abstract object GetChildItem(object? item, int index);

        protected abstract object? GetItemByEngineObject(MarchObject obj);

        protected abstract TreeViewItemDesc GetItemDesc(object item, StringBuilder labelBuilder);

        protected abstract void GetDragTooltip(IReadOnlyList<object> items, StringBuilder tooltipBuilder);

        protected virtual bool CanMoveItem(in TreeViewItemMoveData data) => false;

        protected virtual void OnMoveItem(in TreeViewItemMoveData data) { }

        protected virtual bool CanHandleExternalDrop(in TreeViewExternalDropData data) => false;

        protected virtual void OnHandleExternalDrop(in TreeViewExternalDropData data) { }
    }
}

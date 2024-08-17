using System.Collections;
using System.Diagnostics;

namespace DX12Demo.Editor.Drawers
{
    internal class IListDrawer : IPropertyDrawerFor<IList>
    {
        private record struct MenuArg
        {
            public required IList List { get; set; }

            public required int Index { get; set; }

            public bool IsChanged { get; set; }
        }

        private readonly PopupMenu<MenuArg> s_ItemContextMenu = new("IListDrawerItemContextMenu");

        public IListDrawer()
        {
            s_ItemContextMenu.AddMenuItem("Remove", (ref MenuArg arg) =>
            {
                arg.List.RemoveAt(arg.Index);
                arg.IsChanged = true;
            });
        }

        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            var list = property.GetValue<IList>();

            if (!EditorGUI.Foldout($"{label} ({list.Count})###{label}", tooltip))
            {
                return false;
            }

            bool changed = false;

            using (new EditorGUI.IndentedScope())
            {
                for (int i = 0; i < list.Count; i++)
                {
                    using (new EditorGUI.IDScope(i))
                    {
                        changed |= EditorGUI.PropertyField($"Element{i}", string.Empty, property.GetListItemProperty(i));

                        if (!list.IsFixedSize)
                        {
                            MenuArg result = s_ItemContextMenu.DoItemContext($"{label}{i}", new MenuArg { List = list, Index = i });

                            if (result.IsChanged)
                            {
                                DX12Demo.Core.Debug.LogInfo(result.List);

                                changed = true;
                                i--;
                                continue;
                            }
                        }
                    }
                }

                if (!list.IsFixedSize)
                {
                    if (EditorGUI.ButtonRight("Add"))
                    {
                        Type elementType = GetElementType(list);
                        list.Add(Activator.CreateInstance(elementType, nonPublic: true));
                        changed = true;
                    }
                }
            }

            return changed;
        }

        private static Type GetElementType(IList list)
        {
            if (list.Count > 0)
            {
                return list[^1]?.GetType() ?? throw new NotSupportedException("IListItem is null");
            }

            foreach (var i in list.GetType().GetInterfaces())
            {
                if (i.IsGenericType && i.GetGenericTypeDefinition() == typeof(IEnumerable<>))
                {
                    return i.GetGenericArguments()[0];
                }
            }

            throw new NotSupportedException("Cannot determine element type of IList");
        }
    }
}

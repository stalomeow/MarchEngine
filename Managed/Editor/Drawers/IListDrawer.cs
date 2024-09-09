using System.Collections;

namespace DX12Demo.Editor.Drawers
{
    internal class IListDrawer : IPropertyDrawerFor<IList>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            IList? list = property.GetValue<IList>();

            if (list == null)
            {
                EditorGUI.LabelField(label, tooltip, "null");
                return false;
            }

            if (!EditorGUI.Foldout($"{label} ({list.Count})###{label}", tooltip))
            {
                return false;
            }

            bool changed = false;

            using (new EditorGUI.IndentedScope())
            {
                for (int i = 0; i < list.Count; i++)
                {
                    bool isOpen;

                    if (list.IsFixedSize)
                    {
                        isOpen = EditorGUI.Foldout($"Element{i}", string.Empty);
                    }
                    else
                    {
                        bool isVisible = true;
                        isOpen = EditorGUI.FoldoutClosable($"Element{i}", string.Empty, ref isVisible);

                        if (!isVisible)
                        {
                            list.RemoveAt(i);
                            i--;
                            changed = true;
                            continue;
                        }
                    }

                    if (isOpen)
                    {
                        using (new EditorGUI.IndentedScope())
                        {
                            changed |= EditorGUI.PropertyField($"##Element{i}", string.Empty, property.GetListItemProperty(i));
                        }
                    }
                }

                if (!list.IsFixedSize)
                {
                    if (EditorGUI.ButtonRight("Add"))
                    {
                        list.Add(CreateInstance(GetElementType(list)));
                        changed = true;
                    }
                }
            }

            return changed;
        }

        private static object CreateInstance(Type type)
        {
            if (type == typeof(string))
            {
                return string.Empty;
            }

            return Activator.CreateInstance(type, nonPublic: true)!;
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

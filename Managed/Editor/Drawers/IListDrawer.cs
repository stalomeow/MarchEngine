using DX12Demo.Core;
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
                            EditorProperty itemProperty = property.GetListItemProperty(i);
                            changed |= EditorGUI.PropertyField($"##Element{i}", string.Empty, in itemProperty);
                        }
                    }
                }

                if (!list.IsFixedSize)
                {
                    if (EditorGUI.ButtonRight("Add"))
                    {
                        Type? itemType = EditorPropertyUtility.GetListItemType(property.PropertyType);
                        list.Add(CreateInstance(itemType));
                        changed = true;
                    }
                }
            }

            return changed;
        }

        private static object? CreateInstance(Type? type)
        {
            if (type == null)
            {
                throw new NotSupportedException("Cannot determine the type of list item");
            }

            if (type == typeof(string))
            {
                return string.Empty;
            }

            if (type.IsSubclassOf(typeof(EngineObject)))
            {
                return null;
            }

            return Activator.CreateInstance(type, nonPublic: true)!;
        }
    }
}

using March.Core.Serialization;

namespace March.Editor.Drawers
{
    internal class StringDrawer : IPropertyDrawerFor<string>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            bool changed = false;
            string? value = property.GetValue<string>();
            var attr = property.GetAttribute<StringDrawerAttribute>();

            if (value == null)
            {
                value = string.Empty;
                property.SetValue(value);
                changed = true;
            }

            string charBlacklist = attr?.CharBlacklist ?? string.Empty;
            if (EditorGUI.TextField(label, tooltip, ref value, charBlacklist))
            {
                property.SetValue(value);
                changed = true;
            }

            return changed;
        }
    }
}

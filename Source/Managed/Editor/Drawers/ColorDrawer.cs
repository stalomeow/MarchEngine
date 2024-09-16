using March.Core;

namespace March.Editor.Drawers
{
    internal class ColorDrawer : IPropertyDrawerFor<Color>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            var value = property.GetValue<Color>();

            if (EditorGUI.ColorField(label, tooltip, ref value))
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}

namespace DX12Demo.Editor.Drawers
{
    internal class StringDrawer : IPropertyDrawerFor<string>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            bool changed = false;
            string value = property.GetValue<string>();

            if (EditorGUI.TextField(label, tooltip, ref value))
            {
                property.SetValue(value);
                changed = true;
            }

            return changed;
        }
    }
}

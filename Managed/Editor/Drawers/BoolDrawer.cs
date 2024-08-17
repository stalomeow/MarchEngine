namespace DX12Demo.Editor.Drawers
{
    internal class BoolDrawer : IPropertyDrawerFor<bool>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            var value = property.GetValue<bool>();

            if (EditorGUI.Checkbox(label, tooltip, ref value))
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}

namespace DX12Demo.Editor.Drawers
{
    internal class EnumDrawer : IPropertyDrawerFor<Enum>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            bool changed = false;
            Enum value = property.GetValue<Enum>();

            if (EditorGUI.EnumField(label, tooltip, ref value))
            {
                property.SetValue(value);
                changed = true;
            }

            return changed;
        }
    }
}

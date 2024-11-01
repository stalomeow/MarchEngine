using March.Core.Interop;

namespace March.Editor.Drawers
{
    internal class BoolDrawer : IPropertyDrawerFor<bool>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
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
